# H7-MediaCube

> STM32H743IIT6 多功能智能媒体终端
> 基于 LVGL v8 + FreeRTOS

---

## 功能清单

| # | 功能 | 状态 |
|---|------|------|
| 1 | 主界面（仿手机桌面，3×3图标网格） | 🔲 开发中 |
| 2 | 实时时钟（DS1302，NTP自动校时） | 🔲 开发中 |
| 3 | 文本阅读器（SD卡 .txt，UTF-8，书签） | 🔲 开发中 |
| 4 | 图片浏览器（BMP/JPG，缩略图+全屏） | 🔲 开发中 |
| 5 | 音乐播放器（WAV/MP3，I2S输出） | 🔲 开发中 |
| 6 | 天气显示（实时+3天预报，图标化） | 🔲 开发中 |
| 7 | AI对话（DeepSeek API，流式输出） | 🔲 开发中 |
| 8 | WiFi管理（配网、自动重连） | 🔲 开发中 |
| 9 | 温湿度监测（图表+数值） | 🔲 开发中 |
| 10 | 设置界面（WiFi/城市/亮度/时间） | 🔲 开发中 |

---

## 硬件平台

### 核心板

| 项目 | 参数 |
|------|------|
| MCU | STM32H743IIT6，Cortex-M7，480MHz |
| Flash | 2MB（内部） |
| SRAM | 1MB（内部） |
| 外扩SDRAM | 起始地址 0xC0000000，用于LCD帧缓冲 |

### 显示 & 触摸

| 项目 | 参数 |
|------|------|
| 屏幕 | 7寸，800×480，LTDC RGB接口，RGB565 |
| 驱动芯片 | SSD1963 |
| 帧缓冲 | SDRAM，800×480×2 = 750KB |
| 背光 | PB5，PWM控制 |
| 触摸 | 电容触摸，GT9XXX，软件模拟I2C |
| 触摸引脚 | SCL=PH6，SDA=PI3，INT=PH7，RST=PI8 |

### 存储 & 外设

| 项目 | 参数 |
|------|------|
| SD卡 | SDMMC1，4线，FAT32，FATFS，驱动字母 'S' |
| RTC | DS1302，3线串行，CLK=PB9，DAT=PB8，RST=PB7 |
| WiFi | ESP32-S3，UART AT指令，支持HTTPS/TLS |
| 音频 | PCM5102A，I2S接口 |
| 温湿度 | AHT20，I2C（SCL=PH4，SDA=PH5） |

---

## 软件架构

### FreeRTOS 任务划分

| 任务 | 优先级 | 栈大小 | 职责 |
|------|--------|--------|------|
| GUI_Task | 3 | 4096B | LVGL渲染 |
| Audio_Task | 4 | 8192B | MP3/WAV解码 + I2S DMA |
| Net_Task | 2 | 4096B | ESP32-S3 AT通信 |
| File_Task | 2 | 2048B | SD卡文件操作 |
| Sensor_Task | 1 | 1024B | 温湿度采集（每5秒） |

### GUI 配置

| 项目 | 参数 |
|------|------|
| 框架 | LVGL v8.2.0 |
| 颜色深度 | RGB565（16位） |
| 刷新周期 | 8ms |
| GPU加速 | DMA2D 已启用 |
| LVGL堆 | 64KB（内部SRAM） |

---

## 项目结构

```
├── Drivers/
│   ├── BSP/          # 板级驱动（LCD、触摸、SDRAM、DS1302等）
│   ├── CMSIS/        # ARM CMSIS 接口层
│   ├── STM32H7xx_HAL_Driver/  # ST HAL 库
│   └── SYSTEM/       # 系统基础功能
├── Middlewares/
│   ├── FreeRTOS/     # FreeRTOS 操作系统
│   ├── FATFS/        # 文件系统
│   ├── LVGL/         # LVGL 图形库 v8
│   └── USMART/       # 串口调试工具
├── Projects/         # Keil 工程文件
└── User/             # 用户应用代码
    ├── main.c
    ├── app_gui.c     # GUI 主界面
    ├── app_music.c   # 音乐播放器
    ├── app_weather.c # 天气显示
    └── ...
```

---

## 开发工具

| 工具 | 版本 |
|------|------|
| 编译器 | Keil MDK v5，ARMCC V5.06 |
| 调试器 | ST-Link V2 |
| 串口工具 | ESP32-S3 AT调试 |

---

## 开发进度

- [x] LVGL 基础移植（显示 + 触摸）
- [x] SDRAM 初始化
- [x] 音乐播放器 Demo（LVGL 官方示例）
- [ ] FreeRTOS 集成
- [ ] DS1302 RTC 驱动
- [ ] FATFS + SD卡
- [ ] 主界面 UI 设计
- [ ] 各功能模块开发
