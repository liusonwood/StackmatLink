<div align="center">

# StackmatLink

🌐 [English](./README_EN.md) | [简体中文](./README.md)

</div>

---

**StackmatLink** is an open-source project based on the **ESP32-S3**. It captures the analog Stackmat signals from a **GAN Halo (and other standard timers)** audio port and converts them in real-time into the **GAN Smart Timer Bluetooth Protocol**.

With this project, you can bridge any standard non-Bluetooth Stackmat timer to **csTimer** wirelessly, eliminating the need for expensive Bluetooth-native hardware.

## 🌟 Key Features

- **Protocol Conversion**: Seamlessly converts Stackmat Gen4 (1200 Baud) signals into the official GAN Bluetooth protocol (including the `0xFE` magic packet and CRC16/CCIT-FALSE checksums).
- **Intelligent State Inference**: Specifically handles the GAN Halo `'I'` (Idle) state. It dynamically determines `Running` and `Stopped` states based on the time axis to ensure compatibility with csTimer.
- **Dual-Core Concurrency**:
    - **Core 1**: Dedicated to high-precision signal sampling and parsing at 1200 Baud.
    - **Core 0**: Handles the NimBLE Bluetooth stack and Notify updates for zero-latency data transmission.
- **Automatic Phase Correction**: Built-in logic to detect inverted signals (common in different LM393 wiring) and correct them on the fly.
- **Enhanced Visibility**: Optimized BLE advertising parameters for instant discovery on Mac/PC Chrome and iPad/iOS.
- **Robust Connectivity**: Implements a connection polling mechanism (`getConnectedCount`) to ensure stable data sync even when Bluetooth callbacks are delayed.

## 🛠 Hardware Requirements

- **MCU**: ESP32-S3 (Tested on N16R8, but compatible with all S3 variants).
- **Signal Conditioning**: LM393 Voltage Comparator Module.
- **Interface**: 3.5mm Audio Jack (Tip: Signal, Sleeve: GND).
- **Electronic Components**: 
    - 1x 10kΩ Pull-up Resistor.
    - 2x 10kΩ Voltage Divider Resistors (to create a ~1.65V reference).

## 🔌 Wiring Diagram

| Component | ESP32-S3 Pin | Description |
| :--- | :--- | :--- |
| **LM393 VCC** | 3.3V | Power Supply |
| **LM393 GND** | GND | Common Ground |
| **LM393 Output** | GPIO 4 | **Requires 10kΩ Pull-up to 3.3V** |
| **3.5mm Tip** | LM393 IN+ | Raw Timer Signal |
| **Reference GND**| LM393 IN- | 1.65V Ref (via Voltage Divider) |

### Signal Shaping Logic
Since the timer outputs a weak analog audio signal (sine-ish wave) with noise:
1. **Analog Stage**: The signal (~0.7V–2.5V) enters the LM393 `IN+`.
2. **Comparison**: The LM393 compares it against the fixed `IN-` (1.65V).
3. **Digital Stage**: 
    - Signal > 1.65V -> Output 3.3V.
    - Signal < 1.65V -> Output 0V.
4. **Result**: A clean digital square wave is generated for the ESP32 UART to parse.



## 🚀 Installation

1. Install [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the `esp32` (by Espressif) board package.
3. Install the `NimBLE-Arduino` library via the Library Manager.
4. Open `stackmattoble.ino` and select your ESP32-S3 board.
5. **Note**: If `USB CDC On Boot` is enabled, use the **Native USB** port for serial debugging.
6. Click **Upload**.

## 📱 How to Use

1. **Power Up**: Connect the ESP32-S3 to power and plug the audio cable into the timer.
2. **Verify Signal**: Open Serial Monitor (115200 baud). Operate the timer; you should see `Time: X:XX.XXX -> GAN: X`.
3. **Connect to csTimer**:
    - Go to [csTimer.net](https://cstimer.net) (Chrome/Edge recommended).
    - Settings -> Timer -> Entering Type -> **Bluetooth Timer**.
    - Click the Bluetooth icon at the top of csTimer and pair with **"GAN-Timer"**.
4. **Go!**: Your physical timer is now synced with the digital solve.

## ⚠️ Troubleshooting

- **No Data**: If the Serial Monitor shows `GPIO 4 Level` changing but no time is parsed, try setting the `invert` parameter to `true` in `Serial1.begin`.
- **LM393 Adjustment**: Use a screwdriver to fine-tune the blue potentiometer on the LM393 until the signal triggers consistently.
- **Connectivity**: Use **Chrome** (Windows/macOS/Android) or **Bluefy** (iOS) for the best Web Bluetooth experience.

## 📜 License

MIT License. Contributions and PRs are welcome!

---

### Acknowledgments
- The `NimBLE-Arduino` community.
- The [`csTimer`](https://github.com/cs0x7f/cstimer) project and its excellent [GAN Timer driver](https://github.com/cs0x7f/cstimer/blob/master/src/js/hardware/gantimer.js) reference.
---

This project was conceptualized by me and implemented in 2 hours with the help of AI pair programming.
本项目由我构思，并在 AI 结对编程的辅助下耗时两小时实现。
