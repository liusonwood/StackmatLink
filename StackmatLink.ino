#include <NimBLEDevice.h>
#include <ctype.h>

/*
 * 项目：GAN Halo -> GAN-Timer 蓝牙转换器 (强制连接版)
 * 核心修改：不再依赖 onConnect 回调，直接轮询连接数
 */

#define RX_PIN 4
#define STACKMAT_LEN 10 
#define BAUDRATE 1200
#define SERVICE_UUID        "0000fff0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000fff5-0000-1000-8000-00805f9b34fb"

enum GanState {
    GAN_DISCONNECT = 0, GAN_GET_SET = 1, GAN_HANDS_OFF = 2,
    GAN_RUNNING = 3, GAN_STOPPED = 4, GAN_RESET = 5,
    GAN_HANDS_ON = 6, GAN_FINISHED = 7
};

HardwareSerial StackmatSerial(1);
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pNotifyCharacteristic = nullptr;

struct TimerState {
    uint8_t state;
    uint8_t min; uint8_t sec; uint16_t ms;
    bool needsNotify;
} currentTimer = {GAN_RESET, 0, 0, 0, false};

portMUX_TYPE dataMux = portMUX_INITIALIZER_UNLOCKED;

uint16_t crc16ccitt(uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else crc <<= 1;
        }
    }
    return crc;
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) { 
        Serial.println(">> [BLE] onConnect 回调触发!"); 
    }
    void onDisconnect(NimBLEServer* pServer) { 
        Serial.println(">> [BLE] onDisconnect 回调触发!");
        NimBLEDevice::startAdvertising();
    }
};

void bleTask(void* pvParameters) {
    NimBLEDevice::init("GAN-Timer");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    pNotifyCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY);
    pService->start();
    
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName("GAN-Timer");
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();

    uint8_t packet[10] = {0xFE, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0};
    uint8_t pkgIndex = 0;

    while (true) {
        // 核心修改：直接查询连接的客户端数量
        size_t connectedCount = pServer->getConnectedCount();

        if (currentTimer.needsNotify) {
            portENTER_CRITICAL(&dataMux);
            packet[2] = pkgIndex++;
            packet[3] = currentTimer.state;
            packet[4] = currentTimer.min;
            packet[5] = currentTimer.sec;
            packet[6] = currentTimer.ms & 0xFF;
            packet[7] = (currentTimer.ms >> 8) & 0xFF;
            currentTimer.needsNotify = false;
            portEXIT_CRITICAL(&dataMux);

            uint16_t crc = crc16ccitt(&packet[2], 6);
            packet[8] = crc & 0xFF; packet[9] = (crc >> 8) & 0xFF;

            // 只要有客户端连接，就发送！
            if (connectedCount > 0) {
                pNotifyCharacteristic->setValue(packet, 10);
                pNotifyCharacteristic->notify(); // 发送 notify
                Serial.printf("BLE TX (Clients: %d): State %d Time %d:%02d.%03d\n", 
                    connectedCount, packet[3], packet[4], packet[5], (packet[7] << 8 | packet[6]));
            } else {
                Serial.printf("BLE DROP (Clients: 0): State %d Time %d:%02d.%03d\n", 
                    packet[3], packet[4], packet[5], (packet[7] << 8 | packet[6]));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void processStackmatPacket(uint8_t* b) {
    int check = 64;
    for (int i = 1; i < 7; i++) {
        if (isdigit(b[i])) check += (b[i] - '0');
        else return;
    }
    if (check != b[7]) return;

    static uint32_t lastTotalMs = 0;
    static uint8_t lastInferredState = GAN_RESET;
    static uint8_t lastHeader = 0;

    uint8_t m = b[1] - '0';
    uint8_t s = (b[2] - '0') * 10 + (b[3] - '0');
    uint16_t ms = (b[4] - '0') * 100 + (b[5] - '0') * 10 + (b[6] - '0');
    uint32_t currentTotalMs = m * 60000 + s * 1000 + ms;
    
    uint8_t newState = 0;
    if (b[0] == 'I' || b[0] == ' ') {
        if (currentTotalMs == 0) newState = GAN_RESET;
        else if (currentTotalMs != lastTotalMs) newState = GAN_RUNNING;
        else newState = GAN_STOPPED;
    } else {
        switch (b[0]) {
            case 'A': newState = (currentTotalMs == 0) ? GAN_RESET : GAN_GET_SET; break;
            case 'S': newState = GAN_STOPPED; break;
            case 'L': case 'R': case 'C': newState = GAN_HANDS_ON; break;
            default:  newState = GAN_HANDS_OFF; break;
        }
    }

    bool shouldNotify = false;
    if (newState != lastInferredState) shouldNotify = true;
    else if (newState == GAN_STOPPED && currentTotalMs != lastTotalMs) shouldNotify = true;

    if (shouldNotify) {
        portENTER_CRITICAL(&dataMux);
        currentTimer.min = m; currentTimer.sec = s; currentTimer.ms = ms;
        currentTimer.state = newState;
        currentTimer.needsNotify = true;
        portEXIT_CRITICAL(&dataMux);
        Serial.printf("Signal: State %d -> %d\n", lastInferredState, newState);
    }

    lastTotalMs = currentTotalMs;
    lastInferredState = newState;
    lastHeader = b[0];
}

void stackmatTask(void* pvParameters) {
    bool inverted = true;
    StackmatSerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, -1, inverted);
    uint8_t buf[STACKMAT_LEN];
    int count = 0;
    unsigned long lastPacketTime = millis();
    while (true) {
        while (StackmatSerial.available()) {
            uint8_t c = StackmatSerial.read();
            if (count == 0) {
                if (c == 'A' || c == 'S' || c == ' ' || c == 'L' || c == 'R' || c == 'C' || c == 'I') buf[count++] = c;
            } else buf[count++] = c;
            if (count == STACKMAT_LEN) {
                processStackmatPacket(buf);
                count = 0;
                lastPacketTime = millis();
            }
        }
        if (millis() - lastPacketTime > 3000) {
            inverted = !inverted;
            StackmatSerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, -1, inverted);
            Serial.printf(">> [Signal] Invert: %s\n", inverted ? "ON" : "OFF");
            lastPacketTime = millis();
            count = 0;
        }
        vTaskDelay(1);
    }
}

void setup() {
    Serial.begin(115200);
    xTaskCreatePinnedToCore(bleTask, "BLE_Task", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(stackmatTask, "Stackmat_Task", 4096, NULL, 1, NULL, 1);
}

void loop() { vTaskDelay(1000); }
