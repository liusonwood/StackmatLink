# Project Overview

This project is a Bluetooth (BLE) bridge for the **GAN Halo (星环)** cube timer. It runs on an **ESP32-S3** and acts as an intermediary that converts standard Stackmat signals (received via an analog audio port and shaped by an LM393 comparator) into the proprietary **GAN Smart Timer BLE protocol**. This allows non-Bluetooth timers to work with applications like **csTimer** that support GAN's official Bluetooth timers.

### Key Technologies
- **Hardware**: ESP32-S3, LM393 Comparator, 3.5mm Audio Jack.
- **Framework**: Arduino (C++).
- **Libraries**: `NimBLE-Arduino` for high-performance Bluetooth LE communication.
- **Operating System**: FreeRTOS (Dual-core task management).

### Architecture
- **Core 1 (Stackmat Task)**: High-priority task dedicated to sampling and parsing the 1200 baud Stackmat signal from GPIO 4. It handles signal inversion detection and state inference.
- **Core 0 (BLE Task)**: Manages the Bluetooth LE server, advertising, and characteristic notifications. It emulates the GAN Smart Timer GATT service.

# Building and Running

### Prerequisites
- **Arduino IDE** or **PlatformIO**.
- **ESP32 Board Support**: Install the `esp32` board package in the Arduino Boards Manager.
- **Library**: Install `NimBLE-Arduino` via the Library Manager.

### Configuration
- **GPIO**: The signal input is expected on **GPIO 4**.
- **UUIDs**: Uses GAN's official Service UUID `0000fff0-...` and Characteristic UUID `0000fff5-...`.

### Key Commands
- **Compile/Upload**: Use the standard Arduino "Upload" button (ensure "ESP32S3 Dev Module" or similar is selected).
- **Monitor**: Serial monitor at **115200 baud**.

# Development Conventions

### Coding Style
- **Task Pinning**: Tasks are explicitly pinned to cores using `xTaskCreatePinnedToCore` to prevent interference between time-sensitive signal parsing and radio activity.
- **Concurrency Control**: Shared state between cores is protected using `portMUX_TYPE` and `portENTER_CRITICAL / portEXIT_CRITICAL` sections.
- **State Inference**: Since some timers (like GAN Halo) send non-standard status codes (e.g., `'I'`), the code infers states like `RUNNING` or `STOPPED` by monitoring time delta between packets.
- **Bluetooth Optimization**:
    - Uses `NimBLE` for lower memory footprint and better stability compared to the standard ESP32 BLE library.
    - Employs `getConnectedCount()` for robust connection tracking, bypassing potential callback delays.
    - Includes a rolling `pkgIndex` in the BLE packet to prevent duplicate packet filtering on the receiver side (e.g., csTimer).
