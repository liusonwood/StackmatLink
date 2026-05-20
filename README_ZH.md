<div align="center">

# ⏱️ StackmatLink

**让经典 Stackmat 计时器拥抱无线时代**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-orange.svg)]()
[![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-00979D.svg?logo=arduino&logoColor=white)]()
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)]()

🌐 [English](./README.md) | [简体中文](./README_ZH.md)

</div>

---

**StackmatLink** 是一个基于 **ESP32-S3** 的开源软硬件项目。它能够通过音频口读取标准 Stackmat 计时器（例如 **GAN Halo 星环** 或其他兼容计时器）的模拟信号，并将其低延迟地实时转换为 **GAN Smart Timer 蓝牙协议**。

通过该项目，您可以将任何非蓝牙版标准计时器无线连接到 **csTimer** 等支持官方蓝牙计时器的应用，省去购买昂贵原厂蓝牙硬件的费用。

## ✨ 核心特性

- **协议无缝转换**：实时采集 Stackmat Gen4 (1200 波特率) 信号，并封装为 GAN 官方蓝牙协议（完美包含 `0xFE` 魔数包及 CRC16/CCITT-FALSE 校验）。
- **智能状态推断**：专门针对 GAN Halo 特有的 `'I'` (Idle) 状态码进行优化。通过监听数据包的时间轴动态判定 `Running`（运行）和 `Stopped`（停止）状态，彻底解决 csTimer 的状态识别障碍。
- **双核并发，极致低延迟**：
    - **Core 1**：专职负责 1200 波特率的高优先级、高精度信号实时采样与 UART 解析。
    - **Core 0**：独立处理 NimBLE 蓝牙堆栈和特征值 Notify 通知，确保数据高速传输且绝不干扰信号解析。
- **自动相位纠正**：内置逻辑可自动检测并纠正反相信号（Inverted），完美兼容不同 LM393 电路的接线方式。
- **卓越的连接鲁棒性与可见性**：采用底层的连接数轮询 (`getConnectedCount`) 机制解决蓝牙回调延迟问题。优化了 BLE 广播参数，确保在 Mac、Windows Chrome 及 iOS 设备上实现“秒搜秒连”。

## 📂 项目结构

```text
.
├── GEMINI.md                   # AI 助手指令与项目设计核心文档
├── LICENSE                     # MIT 开源协议
├── README.md                   # 英文说明文档
├── README_ZH.md                # 中文说明文档
├── stackmatlink.ino            # Arduino 主程序源码
└── hardware/
    ├── Case/                   # 3D 打印外壳设计文件 (STL, Blender)
    ├── PCB/                    # PCB 原理图与生产文件 (Gerber, epro2 工程)
    └── Wiring/                 # 硬件接线与电路参考图
```

## 📦 所需部件 (BOM)

制作 StackmatLink 需要以下硬件材料：

| 组件 | 说明 | 备注 |
| :--- | :--- | :--- |
| **微控制器 (MCU)** | ESP32-S3 | 测试基于 N16R8。PCB 采用 **ESP32-S3-SuperMini**，但兼容全系 S3。 |
| **状态指示灯** | NeoPixel (WS2812) | 可选。SuperMini 自带板载灯，其他开发板可外接。 |
| **电压比较器** | LM393 | 负责信号整形 (贴片/SMD/SOP-8 版本)。 |
| **电阻** | 3× 10kΩ 电阻 | 1个用于上拉，2个用于构建 1.65V 的分压参考。 |
| **音频接口** | 3.5mm 音频母座 | Tip (尖端) 接信号，Sleeve (套筒) 接地。 |
| **定制 PCB** | StackmatLink 专属电路板 | 包含在 `hardware/PCB/` 中的 Gerber 文件，可直接打样。 |
| **设备外壳** | 3D 打印外壳 | 包含在 `hardware/Case/` 中的 STL 模型文件，可直接 3D 打印。 |

## 🛠️ 硬件与构建指南

### 1. 电路连接参考

<div align="center">
  <img src="./hardware/Wiring/Wiringconnection.png" width="600" alt="电路连接图">
  <p><em>LM393 与 ESP32-S3 的面包板接线参考。</em></p>
</div>

| 物理组件 | ESP32-S3 引脚 | 说明 |
| :--- | :--- | :--- |
| **LM393 VCC** | `3.3V` | 比较器供电 |
| **LM393 GND** | `GND` | 共地 |
| **LM393 Output** | `GPIO 4` | **必须接 10kΩ 上拉电阻到 3.3V** |
| **NeoPixel DI** | `GPIO 48` | 状态指示灯 (SuperMini 自带) |
| **3.5mm Tip** | `LM393 IN+` | 接入计时器原始信号 |
| **GND 参考** | `LM393 IN-` | 接 1.65V 参考电压 (3.3V 经两电阻分压) |

### 2. 物理信号整形逻辑

由于计时器输出的原始模拟音频信号（约 0.7V~2.5V 波动，类正弦波）微弱且带有噪声，需要进行整形：
1. **模拟输入**：信号进入 LM393 的 `IN+`。
2. **硬件比较**：LM393 将其与 `IN-` 上固定的 1.65V 参考电压进行实时比较。
3. **数字输出**：当信号 > 1.65V 时，输出 `3.3V` (高电平)；当信号 < 1.65V 时，输出 `0V` (低电平)。
4. **解析阶段**：输出端产生的纯净数字方波，由 ESP32 的硬件串口 (UART) 零误差解析为 ASCII 数据。

### 3. 硬件设计文件

<div align="center">
  <img src="./hardware/Case/caes-render.jpg" width="400" alt="外壳渲染图"> 
  <img src="./hardware/PCB/PCB.png" width="350" alt="PCB 预览图">
</div>

- **PCB 设计** (`hardware/PCB/`)：包含可直接用于嘉立创打样的 Gerber 文件与工程源文件。
- **3D 打印外壳** (`hardware/Case/`)：包含可直接切片的 STL 模型，以及便于二次修改的 Blender 源文件。

## 💻 软件安装与设置

1. 安装 [Arduino IDE](https://www.arduino.cc/en/software)。
2. 在开发板管理器中安装由 Espressif 提供的 `esp32` 支持包。
3. 在库管理器中搜索并安装 `NimBLE-Arduino` 和 `Adafruit_NeoPixel` 库。
4. 打开 `stackmatlink.ino`，选择您的 **ESP32-S3 Dev Module**，然后点击 **上传 (Upload)**。
   > **注意**：如果在 IDE 中启用了 `USB CDC On Boot`，请确保使用 Native USB 端口进行调试输出。

## 📖 使用指南

1. **上电连接**：为 ESP32-S3 供电，并使用双头 3.5mm 音频线连接计时器与转接板。
2. **确认信号**：打开 Arduino 串口监视器（设置为 `115200` 波特率）。在计时器运作时，板载指示灯将亮起 **蓝色**。
3. **连接 csTimer**：
    - 使用浏览器打开 [csTimer.net](https://cstimer.net)（推荐 Chrome / Edge）。
    - 依次进入 `设置` -> `计时器` -> `输入类型` -> 选择 **蓝牙魔方/计时器**。
    - 点击 csTimer 顶部的蓝牙图标，选择名为 **"GAN-Timer"** 的设备进行配对。
4. **开始计时**：现在，您的实体计时器状态已经与网页实现了完美同步！

## ⚠️ 故障排除

- **串口有输出但无数据**：如果串口监视器显示 GPIO 电平变化，但没有解析出数字，请尝试在 `stackmatlink.ino` 的 `stackmatTask` 中切换 `inverted` 变量的初始值。
- **蓝牙连接失败**：确保使用 **Chrome** (Windows/macOS/Android) 浏览器。若在 iOS 设备上使用，请下载支持 Web Bluetooth 的 **Bluefy** 浏览器。
- **指示灯状态含义**：
  - 🔵 **蓝色常亮**：正在计时。
  - ⚪ **白色常亮或闪烁**：计时器已重置，处于就绪状态。

## 📜 开源协议

Copyright (c) 2026 liusonwood.
本项目采用 [MIT License](./LICENSE) 授权。非常欢迎提交 Issue 或 Pull Request！

## 鸣谢
- 感谢 `NimBLE-Arduino` 与 `Adafruit_NeoPixel` 社区提供的优秀库。
- 感谢 [`csTimer`](https://github.com/cs0x7f/cstimer) 项目，以及 [GAN 计时器驱动代码](https://github.com/afedotov/gan-web-bluetooth) 提供的协议参考。

---

> ⚠️ **安全提示**  
> StackmatLink 作为一个完全开源的项目，本仓库是其唯一的官方发布渠道。请警惕 GitHub 或其他平台上提供所谓 `.zip` 整合包或 `.exe` 安装程序的仿冒库。本项目仅提供源代码及硬件设计图。

---

<div align="center">
  <em>This project was initially conceptualized by me and implemented in 2 hours with the help of AI pair programming. <br> 本项目由我最初构思，并在 AI 对编程的辅助下耗时两小时实现。</em>
</div>
