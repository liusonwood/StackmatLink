<div align="center">

# StackmatLink

🌐 [English](./README_EN.md) | [简体中文](./README.md)

</div>

---
# Stackmat魔方计时器 -> BLE 蓝牙转换器

这是一个基于 **ESP32-S3** 的开源项目，旨在将 **GAN Halo (星环)** 计时器通过音频口输出的模拟 Stackmat 信号，实时转换为 **GAN Smart Timer 蓝牙协议**。

通过该项目，您可以让任何标准 Stackmat 魔方计时器（本项目使用GAN Halo 计时器）通过蓝牙连接到的 **csTimer**，实现自动同步成绩，无需购买昂贵的蓝牙版。

## 🌟 核心特性

- **协议转换**：将 Stackmat Gen4 (1200 Baud) 信号转换为 GAN 官方蓝牙协议（包含 0xFE 魔数包及 CRC16 校验）。
- **智能状态推断**：针对 GAN Halo 特有的 `'I'` 状态码，通过时间轴动态判定 `Running`（运行）和 `Stopped`（停止）状态，解决 csTimer 识别障碍。
- **双核并发处理**：
    - **Core 1**：负责 1200 波特率的高精度信号实时采样与解析。
    - **Core 0**：负责 NimBLE 蓝牙堆栈和成绩通知（Notify），确保数据传输零延迟。
- **自动相位纠正**：具备自动检测信号反相（Inverted）并纠正的功能，极大提高不同硬件电路的兼容性。
- **增强可见性**：优化了 BLE 广播参数，确保 Mac Chrome 浏览器和 iPad csTimer 能快速发现并连接设备。
- **连接鲁棒性**：采用轮询连接数（getConnectedCount）机制，解决部分系统下蓝牙连接回调延迟的问题。

## 🛠 硬件需求

- **MCU**: ESP32-S3 (测试使用 N16R8 版本，pcb焊接使用esp32-s3-supermini版本，但全系列 S3 均可)。
- **信号整形**: LM393 比较器模块。
- **接口**: 3.5mm 音频头（Tip 信号，Sleeve 地）。
- **电子元件**: 
    - 10kΩ 上拉电阻 (1个)。
    - 10kΩ 分压电阻 (2个，用于建立 1.65V 参考电压)。

## 🔌 电路连接 (Wiring)

| 组件 | ESP32-S3 引脚 | 说明 |
| :--- | :--- | :--- |
| **LM393 VCC** | 3.3V | 供电 |
| **LM393 GND** | GND | 共地 |
| **LM393 Output** | GPIO 4 | **必须接 10kΩ 上拉电阻到 3.3V** |
| **3.5mm Tip** | LM393 IN+ | 计时器原始信号 |
| **GND 参考** | LM393 IN- | 接 1.65V 参考电压 (3.3V 经两电阻分压) |

### 物理整形逻辑说明
由于计时器输出的是微弱的模拟音频信号（正弦波），电压波动较小，且带有噪声。
1. **模拟阶段**：计时器信号（约 0.7V~2.5V 波动）进入 LM393 的 `IN+`。
2. **比较阶段**：LM393 将其与 `IN-`（固定在 1.65V）进行实时比较。
3. **数字阶段**：
    - 信号 > 1.65V -> 输出 3.3V。
    - 信号 < 1.65V -> 输出 0V。
4. **结果**：输出端产生标准的数字方波，由 ESP32 的硬件串口 (UART) 精准解析 ASCII 字符。

## 🚀 软件安装

1. 安装 [Arduino IDE](https://www.arduino.cc/en/software)。
2. 在开发板管理器中安装 `esp32` (by Espressif) 支持包。
3. 在库管理器中安装 `NimBLE-Arduino` 库。
4. 打开 `stackmattoble.ino`，选择您的 ESP32-S3 开发板型号。
5. 点击 **上传**。

## 📱 使用指南

1. **上电**：将 ESP32-S3 接入电源，并将音频线连接到计时器。
2. **确认信号**：打开串口监视器（115200 波特率），操作计时器，看到 `Time: X:XX.XXX -> GAN: X` 即表示解析成功。
3. **连接 csTimer**：
    - 打开 [csTimer.net](https://cstimer.net)。
    - 设置 -> 计时器 -> 输入类型 -> **蓝牙魔方/计时器**。
    - 点击 csTimer 顶部蓝牙图标，选择名为 **"GAN-Timer"** 的设备并配对。
    - 打开串口监视器（115200 波特率），操作计时器，看到 ` BLE TX (Clients: 1): xxx` 即表示连接成功
4. **开始计时**：计时器的操作将实时同步到网页中。

## ⚠️ 注意事项

- **LM393 调节**：如果信号无法解析，请微调 LM393 上的电位器。建议使 LM393 的阈值处于信号波动的中心位置。
- **设备连接**：cstimer推荐：Chrome（谷歌浏览器）用于Windows、macOS、Linux、Android；Bluefy用于iOS

## 📜 开源协议

MIT License. 欢迎提交 PR 优化协议逻辑！

---

### 鸣谢
- `NimBLE-Arduino` 库开发者。
- [`csTimer`](https://github.com/cs0x7f/cstimer) 项目及其 [GAN 计时器驱动代码](https://github.com/cs0x7f/cstimer/blob/master/src/js/hardware/gantimer.js)参考。

---

This project was conceptualized by me and implemented in 2 hours with the help of AI pair programming.
本项目由我构思，并在 AI 结对编程的辅助下耗时两小时实现。