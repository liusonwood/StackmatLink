<div align="center">

# ⏱️ StackmatLink

**Bridging Classic Stackmat Timers to the Wireless World**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-orange.svg)]()
[![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-00979D.svg?logo=arduino&logoColor=white)]()
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)]()

🌐 [English](./README.md) | [简体中文](./README_ZH.md)

</div>

---

**StackmatLink** is an open-source hardware and software project based on the **ESP32-S3**. It captures analog signals from a standard Stackmat timer's audio port (like the **GAN Halo** or other compatible timers) and seamlessly converts them into the **GAN Smart Timer Bluetooth Protocol** in real-time.

With StackmatLink, you can bridge any standard non-Bluetooth Stackmat timer to web applications like **csTimer** wirelessly, eliminating the need for expensive Bluetooth-native hardware.

## ✨ Key Features

- **Protocol Conversion**: Seamlessly parses Stackmat Gen4 (1200 Baud) signals and encapsulates them into the official GAN Bluetooth protocol (including the `0xFE` magic packet and CRC16/CCITT-FALSE checksums).
- **Intelligent State Inference**: Specifically handles the GAN Halo `'I'` (Idle) state. It dynamically infers `Running` and `Stopped` states based on packet timing to ensure absolute compatibility with csTimer.
- **Dual-Core Concurrency for Zero Latency**:
  - **Core 1**: Dedicated to high-priority, high-precision signal sampling and UART parsing at 1200 Baud.
  - **Core 0**: Handles the NimBLE Bluetooth stack and characteristic Notify updates to ensure rapid data transmission without interrupting signal processing.
- **Automatic Phase Correction**: Built-in logic detects inverted signals (common when using different LM393 wiring configurations) and corrects them on the fly.
- **Robust Connectivity & Visibility**: Implements a connection polling mechanism (`getConnectedCount`) to maintain stable data synchronization, with optimized BLE advertising parameters for instant discovery on macOS, Windows, and iOS.

## 📂 Project Structure

```text
.
├── GEMINI.md                   # Agent instructions & project overview
├── LICENSE                     # MIT License
├── README.md                   # English Documentation
├── README_ZH.md                # Chinese Documentation
├── stackmatlink.ino            # Main Arduino source code
└── hardware/
    ├── Case/                   # 3D printed case designs (STL, Blender)
    ├── PCB/                    # PCB design and production files (Gerber, Project)
    └── Wiring/                 # Circuit diagrams and references
```

## 📦 Required Components (BOM)

To build StackmatLink, you will need the following parts:

| Component | Description | Notes |
| :--- | :--- | :--- |
| **MCU** | ESP32-S3 | Tested on N16R8. Custom PCB uses **ESP32-S3-SuperMini**, but compatible with all S3 variants. |
| **Status LED** | NeoPixel (WS2812) | Optional. Onboard on the SuperMini or externally wired. |
| **Comparator IC** | LM393 | Voltage Comparator (SMD/SOP-8 version). |
| **Resistors** | 3× 10kΩ Resistors | 1× for pull-up, 2× for a 1.65V voltage divider reference. |
| **Audio Jack** | 3.5mm Audio Jack | Tip: Signal, Sleeve: GND. |
| **Custom PCB** | StackmatLink Board | Gerber files provided in `hardware/PCB/` for manufacturing. |
| **Enclosure** | 3D Printed Case | STL files provided in `hardware/Case/` for 3D printing. |

## 🛠️ Hardware & Build Guide

### 1. Wiring Diagram

<div align="center">
  <img src="./hardware/Wiring/Wiringconnection.png" width="600" alt="Wiring Diagram">
  <p><em>Breadboard wiring reference for LM393 and ESP32-S3.</em></p>
</div>

| Component | ESP32-S3 Pin | Description |
| :--- | :--- | :--- |
| **LM393 VCC** | `3.3V` | Power Supply |
| **LM393 GND** | `GND` | Common Ground |
| **LM393 Output**| `GPIO 4` | **Requires 10kΩ Pull-up to 3.3V** |
| **NeoPixel DI** | `GPIO 48` | Status LED (Onboard on SuperMini) |
| **3.5mm Tip** | `LM393 IN+` | Raw Timer Signal |
| **Ref GND** | `LM393 IN-` | 1.65V Ref (via Voltage Divider) |

### 2. Signal Shaping Logic

Since the timer outputs a weak analog audio signal (~0.7V–2.5V, sine-ish wave) prone to noise:
1. **Analog Stage**: The signal enters the LM393 `IN+`.
2. **Comparison**: The LM393 compares it against a fixed 1.65V reference on `IN-`.
3. **Digital Stage**: `Signal > 1.65V` → Output `3.3V` (High); `Signal < 1.65V` → Output `0V` (Low).
4. **Result**: A clean, digital square wave is generated for the ESP32 UART to read.

### 3. Design Files

<div align="center">
  <img src="./hardware/Case/caes-render.jpg" width="400" alt="Case Render"> 
  <img src="./hardware/PCB/PCB.png" width="350" alt="PCB Preview">
</div>

- **PCB Design** (`hardware/PCB/`): Production-ready Gerber files and project source.
- **3D Printed Case** (`hardware/Case/`): STL files and the original Blender source file for modifications.

## 💻 Software & Setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the `esp32` board package by Espressif.
3. Install the `NimBLE-Arduino` and `Adafruit_NeoPixel` libraries via the Arduino Library Manager.
4. Open `stackmatlink.ino`, select your **ESP32-S3 Dev Module** (or your specific variant), and click **Upload**.
   > **Note:** If `USB CDC On Boot` is enabled on your board, use the Native USB port for serial debugging.

## 📖 Usage Guide

1. **Power Up**: Connect the ESP32-S3 to power, and plug the audio cable into your timer.
2. **Verify Signal**: Open the Serial Monitor at `115200 baud`. The NeoPixel LED will glow **Blue** when the timer is actively running.
3. **Connect to csTimer**:
   - Navigate to [csTimer.net](https://cstimer.net) (Chrome/Edge recommended).
   - Go to `Settings` -> `Timer` -> `Entering Type` -> select **Bluetooth Timer**.
   - Click the Bluetooth icon at the top of csTimer and pair with **"GAN-Timer"**.
4. **Go!**: Your physical timer is now fully synced with your digital sessions.

## ⚠️ Troubleshooting

- **No Data Parsed**: If the Serial Monitor shows raw activity but no time is parsed, try toggling the `inverted` parameter in the `stackmatTask` code.
- **Connectivity Issues**: Ensure you are using **Chrome** (Windows/macOS/Android) or a WebBLE browser like **Bluefy** (iOS) for the best Web Bluetooth experience.
- **LED Status Meanings**: 
  - 🔵 **Blue**: Timer is running.
  - ⚪ **White (Solid or Blinking)**: Timer is reset and ready.

## 📜 License & Credits

Copyright (c) 2026 liusonwood.
Released under the [MIT License](./LICENSE). Contributions and PRs are always welcome!

## Acknowledgments
- The `NimBLE-Arduino` and `Adafruit_NeoPixel` communities.
- The [`csTimer`](https://github.com/cs0x7f/cstimer) project and the [GAN Timer driver](https://github.com/afedotov/gan-web-bluetooth) reference.

> ⚠️ **WARNING: Security Notice**  
> StackmatLink is an open-source project and this is the only official repository. Beware of fake clones or 'installers' on GitHub offering `.zip` or `.exe` files. This project is provided purely as source code and hardware design files.

---

<div align="center">
  <em>This project was initially conceptualized by me and implemented in 2 hours with the help of AI pair programming. <br> 本项目由我最初构思，并在 AI 对编程的辅助下耗时两小时实现。</em>
</div>
