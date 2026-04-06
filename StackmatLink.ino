#include <NimBLEDevice.h>
#include <ctype.h>


#define RX_PIN 4
#define STACKMAT_LEN 10 
#define BAUDRATE 1200
#define SERVICE_UUID        "0000fff0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000fff5-0000-1000-8000-00805f9b34fb"

#include <Adafruit_NeoPixel.h>
#define PIN        48
#define NUMPIXELS   1
Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
unsigned long ledTurnOffTime = 0;
bool isLedOn = false;
void handleStatusLED(int state) {
  uint32_t color = strip.Color(0, 0, 0); // 默认关闭
  bool permanent = false; // 是否常亮（不触发自动熄灭）

  switch (state) {
    case 3: // GAN_RUNNING
      color = strip.Color(0, 0, 40);   // 蓝色（计时中）
      permanent = true;
      break;
    case 5: // GAN_RESET
      color = strip.Color(50, 50, 30); // 白色
      permanent = false;               // 闪烁模式
      break;
    default:
      color = strip.Color(0, 0, 0);
      permanent = false;
      break;
  }

  strip.setPixelColor(0, color);
  strip.show();

  if (!permanent) {
    ledTurnOffTime = millis() + 80; // 80ms 后熄灭（进入等待状态）
    isLedOn = true;
  } else {
    isLedOn = false; // 常亮状态，不需要自动关灯计时器
  }
}


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

TaskHandle_t xBleTaskHandle = NULL;

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
        // Wait for notification from stackmatTask
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

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

            size_t connectedCount = pServer->getConnectedCount();
            if (connectedCount > 0) {
                pNotifyCharacteristic->setValue(packet, 10);
                pNotifyCharacteristic->notify();
                // Serial.printf is slow, removed or conditional for speed
            }
        }
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

    // 核心逻辑恢复：
    // 1. 状态发生切换时通知（例如：HANDS_OFF -> RUNNING 发送一次开始信号）
    // 2. 状态为 STOPPED 且时间有更新时通知（发送最终成绩）
    bool shouldNotify = false;
    if (newState != lastInferredState) {
        shouldNotify = true;
    } else if (newState == GAN_STOPPED && currentTotalMs != lastTotalMs) {
        shouldNotify = true;
    }

    if (shouldNotify) {
        portENTER_CRITICAL(&dataMux);
        currentTimer.min = m; currentTimer.sec = s; currentTimer.ms = ms;
        currentTimer.state = newState;
        currentTimer.needsNotify = true;
        portEXIT_CRITICAL(&dataMux);
        if (xBleTaskHandle != NULL) xTaskNotifyGive(xBleTaskHandle);
        
        if (newState != lastInferredState) {
            handleStatusLED(newState);
        }
    }

    lastTotalMs = currentTotalMs;
    lastInferredState = newState;
}

void stackmatTask(void* pvParameters) {
    bool inverted = true;
    StackmatSerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, -1, inverted);
    uint8_t buf[STACKMAT_LEN];
    int count = 0;
    unsigned long lastByteTime = 0;
    unsigned long lastPacketTime = millis();
    
    while (true) {
        if (StackmatSerial.available()) {
            while (StackmatSerial.available()) {
                uint8_t c = StackmatSerial.read();
                unsigned long now = millis();
                
                // 1. 基于时间间隔的同步：1200波特率下每字节约8.3ms，15ms是安全的间隔
                if (now - lastByteTime > 15 && count > 0) {
                    count = 0; 
                }
                lastByteTime = now;

                if (count == 0) {
                    // 2. 预测性处理：识别到 Header 立即推断部分状态
                    if (c == 'A' || c == 'S' || c == ' ' || c == 'L' || c == 'R' || c == 'C' || c == 'I') {
                        buf[count++] = c;
                        
                        // 对于能够从 Header 直接确定的状态，立即预通知蓝牙
                        uint8_t predictedState = 255;
                        if (c == 'A') predictedState = GAN_GET_SET;
                        else if (c == 'L' || c == 'R' || c == 'C') predictedState = GAN_HANDS_ON;
                        else if (c == 'S') predictedState = GAN_STOPPED;
                        
                        if (predictedState != 255 && predictedState != currentTimer.state) {
                            portENTER_CRITICAL(&dataMux);
                            currentTimer.state = predictedState;
                            currentTimer.needsNotify = true;
                            portEXIT_CRITICAL(&dataMux);
                            if (xBleTaskHandle != NULL) xTaskNotifyGive(xBleTaskHandle);
                        }
                    }
                } else {
                    buf[count++] = c;
                    if (count == STACKMAT_LEN) {
                        processStackmatPacket(buf);
                        count = 0;
                        lastPacketTime = now;
                    }
                }
            }
        } else {
            vTaskDelay(1);
        }

        if (millis() - lastPacketTime > 3000) {
            inverted = !inverted;
            StackmatSerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, -1, inverted);
            lastPacketTime = millis();
            count = 0;
        }
    }
}

void setup() {
    Serial.begin(115200);
    // BLE Task on Core 0, Priority 2 (Higher)
    xTaskCreatePinnedToCore(bleTask, "BLE_Task", 4096, NULL, 2, &xBleTaskHandle, 0);
    // Stackmat Task on Core 1, Priority 3 (Highest on this core)
    xTaskCreatePinnedToCore(stackmatTask, "Stackmat_Task", 4096, NULL, 3, NULL, 1);
    strip.begin();
    strip.show();
}

void loop() {
    
    vTaskDelay(100); 

    if (isLedOn && millis() > ledTurnOffTime) {
    strip.setPixelColor(0, strip.Color(0, 0, 0)); // 关灯
    strip.show();
    isLedOn = false;
  }    
}
