<div align="center">

# StackmatLink

🌐 [English](./README.md) | [简体中文](./README_ZH.md)

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

## 📂 Project Structure

```text
.
├── GEMINI.md                   # Gemini AI instructions & project overview
├── LICENSE                     # MIT License
├── README.md                   # English Documentation
├── README_ZH.md                # Chinese Documentation
├── stackmatlink.ino            # Main Arduino source code
└── hardware/
    ├── Case/                   # 3D printed case designs (STL, Blender)
    │   ├── caes-render.jpg
    │   ├── ... .stl
    │   └── ... .blend
    ├── PCB/                    # PCB design and production files (Gerber, Project)
    │   ├── Gerber_PCB1_... .zip
    │   ├── PCB.png
    │   └── ... .epro2
    └── Wiring/                 # Circuit diagrams and references
        └── Wiringconnection.png
```

## 🛠 Hardware & Build Guide

### 1. Requirements (BOM)
- **MCU**: ESP32-S3 (Tested on N16R8, PCB uses **ESP32-S3-SuperMini**, but compatible with all S3 variants).
- **Signal Conditioning**: **LM393 Voltage Comparator (SMD/SOP-8 version)**.
- **Status LED**: Onboard or external NeoPixel (WS2812) LED.
- **Interface**: 3.5mm Audio Jack (Tip: Signal, Sleeve: GND).
- **Electronic Components**: 
    - **10kΩ Resistor Network** (3x 10kΩ resistors: 1x pull-up, 2x voltage divider for 1.65V ref).

### 2. Wiring Diagram
<img src="./hardware/Wiring/Wiringconnection.png" width="600" alt="Wiring Diagram">

**Breadboard wiring reference for LM393 and ESP32-S3.**

| Component | ESP32-S3 Pin | Description |
| :--- | :--- | :--- |
| **LM393 VCC** | 3.3V | Power Supply |
| **LM393 GND** | GND | Common Ground |
| **LM393 Output** | GPIO 4 | **Requires 10kΩ Pull-up to 3.3V** |
| **NeoPixel DI** | GPIO 48 | Status LED (Onboard on SuperMini) |
| **3.5mm Tip** | LM393 IN+ | Raw Timer Signal |
| **Reference GND**| LM393 IN- | 1.65V Ref (via Voltage Divider) |

### 3. Signal Shaping Logic
Since the timer outputs a weak analog audio signal (sine-ish wave) with noise:
1. **Analog Stage**: The signal (~0.7V–2.5V) enters the LM393 `IN+`.
2. **Comparison**: The LM393 compares it against the fixed `IN-` (1.65V).
3. **Digital Stage**: Signal > 1.65V -> Output 3.3V; Signal < 1.65V -> Output 0V.
4. **Result**: A clean digital square wave is generated for the ESP32 UART to parse.

### 4. Design Files
<img src="./hardware/Case/caes-render.jpg" width="400" alt="Case Render"> <img src="./hardware/PCB/PCB.png" width="350" alt="PCB Preview">

- **PCB Design** (`hardware/PCB/`): Gerber files and project source for the custom PCB.
- **3D Printed Case** (`hardware/Case/`): STL files and original Blender source.

## 💻 Software & Setup

### 1. Installation
1. Install [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the `esp32` (by Espressif) board package.
3. Install the `NimBLE-Arduino` and `Adafruit_NeoPixel` libraries via the Library Manager.
4. Open `stackmatlink.ino`, select your ESP32-S3 board, and click **Upload**.
   - *Note: If `USB CDC On Boot` is enabled, use the Native USB port for serial debugging.*

### 2. Usage Guide
1. **Power Up**: Connect the ESP32-S3 to power and plug the audio cable into the timer.
2. **Verify Signal**: Open Serial Monitor (115200 baud). The NeoPixel LED will turn **Blue** when the timer is running.
3. **Connect to csTimer**:
    - Go to [csTimer.net](https://cstimer.net) (Chrome/Edge recommended).
    - Settings -> Timer -> Entering Type -> **Bluetooth Timer**.
    - Click the Bluetooth icon at the top of csTimer and pair with **"GAN-Timer"**.
4. **Go!**: Your physical timer is now synced with the digital solve.

## ⚠️ Troubleshooting

- **No Data**: If the Serial Monitor shows activity but no time is parsed, try toggling the `inverted` parameter in the `stackmatTask` code.
- **Connectivity**: Use **Chrome** (Windows/macOS/Android) or **Bluefy** (iOS) for the best Web Bluetooth experience.
- **LED Status**: **Blue** (Running), **White/Blinking** (Reset/Ready).

## 📜 License

Copyright (c) 2026 liusonwood.
MIT License. Contributions and PRs are welcome!

---

### Acknowledgments
- The `NimBLE-Arduino` and `Adafruit_NeoPixel` communities.
- The [`csTimer`](https://github.com/cs0x7f/cstimer) project and [GAN Timer driver](https://github.com/afedotov/gan-web-bluetooth) reference.

---
**⚠️ Security Warning:** 
StackmatLink is an open-source project. This is the only official repository. Be careful of fake 'clones' or 'installers' on GitHub that provide .zip or .exe files. This project is provided as source code; I will never ask you to download a setup.exe.
---
This project was initially conceptualized by me and implemented in 2 hours with the help of AI pair programming.
本项目由我最初构思，并在 AI 对编程的辅助下耗时两小时实现。
