# QSPI LCD 示例工程

* [English](./README.md)

> 面向新手：本文从“这个工程是干什么的”开始讲起，跟着第 4 章一步步操作，就能点亮一块 QSPI 接口的屏幕。

## 1. 项目概述

本工程演示 Beken 平台上 **QSPI 接口 LCD** 的最小显示流程：基于统一的 `bk_display_*` 显示控制器接口，完成显示控制器创建、初始化、打开屏幕、点亮背光、申请帧缓冲、把整屏填成纯色并刷新到屏幕。

QSPI 使用 **4 线（四数据线）模式**，带宽比普通 SPI 更高，适合分辨率更大、刷新更快的屏幕。

简单来说，它做两件事：

1. **上电自动跑马灯**：开机后自动让屏幕在 **红 → 绿 → 蓝** 之间循环切换，每种颜色停留 3 秒。看到屏幕循环变色，就说明屏幕点亮成功。
2. **串口手动刷色**：通过串口输入 `qspi_lcd` 命令，可以把整屏刷成你指定的任意颜色。

### 1.1 测试环境

* 硬件配置：
    * 核心板 **BK7259_QF128_12.3X12.3_V4.0**
    * PSRAM 32M
* 默认 QSPI 屏：`spd2010`（分辨率 **412 x 412**）
    * 像素格式：`RGB565`（每像素 2 字节）
    * 接口：QSPI（4 线模式 `CONFIG_QSPI_LINE_MODE=4`，启用 TE 同步）

> ⚠️ 注意：请使用上述参考外设来熟悉和学习本 demo。如果你的屏幕型号 / 分辨率 / 引脚不同，需要相应修改代码和配置（见第 6 章）。

## 2. 目录结构

工程采用 AP-CP 双核架构，主要业务代码在 AP 目录下。结构如下：

```
qspi_lcd_example/
├── CMakeLists.txt            # 工程级 CMake 构建文件
├── Makefile                  # Make 构建入口
├── README.md                 # 项目说明文档（英文）
├── README_CN.md              # 项目说明文档（中文，本文件）
├── app.rst                   # 工程说明（rst 文档骨架）
├── ap/                       # AP 端代码（核心业务）
│   ├── CMakeLists.txt        # AP 端 CMake 构建文件
│   ├── ap_main.c             # AP 主入口：显示流程、背光、CLI 命令、上电自动刷色任务
│   └── config/bk7259_ap/
│       ├── defconfig         # AP 端功能开关（启用 QSPI LCD、显示、帧缓冲等）
│       └── usr_gpio_cfg.h    # AP 端 GPIO 默认配置（屏幕引脚在这里）
├── cp/                       # CP 端代码（系统启动、UART0 等）
└── partitions/               # 分区配置
```

新手只需重点关注 `ap/ap_main.c` 和 `ap/config/bk7259_ap/`。

## 3. 硬件连接

代码默认使用以下引脚（定义在 `ap/config/bk7259_ap/usr_gpio_cfg.h` 与 `ap/ap_main.c`）。接线时把屏幕对应引脚接到下表的 GPIO 上：

| 屏幕引脚 | 说明 | 默认 GPIO | 配置位置 |
| --- | --- | --- | --- |
| CLK | QSPI 时钟 | GPIO_22 | `usr_gpio_cfg.h`（QSPI0_CLK） |
| CS / CSN | 片选 | GPIO_23 | `usr_gpio_cfg.h`（QSPI0_CSN） |
| IO0 | 数据线 0 | GPIO_24 | `usr_gpio_cfg.h`（QSPI0_IO0） |
| IO1 | 数据线 1 | GPIO_25 | `usr_gpio_cfg.h`（QSPI0_IO1） |
| IO2 | 数据线 2 | GPIO_26 | `usr_gpio_cfg.h`（QSPI0_IO2） |
| IO3 | 数据线 3 | GPIO_27 | `usr_gpio_cfg.h`（QSPI0_IO3） |
| RESET / RST | 复位 | GPIO_40 | `ap_main.c` 中 `QSPI_LCD_RESET_PIN` |
| TE | 撕裂同步 | GPIO_41 | `ap_main.c` 中 `QSPI_LCD_TE_PIN` |
| BLK / 背光 | 背光使能 | GPIO_7 | `ap_main.c` 中 `QSPI_LCD_BACKLIGHT_PIN` |
| VCC / GND | 供电 | 按屏幕模块要求接 3.3V / GND | 模块自带 |

> 背光由 `lcd_backlight_open()` 在显示初始化时拉高 GPIO_7 点亮。

## 4. 快速上手（编译、烧录、运行）

### 4.1 编译

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=multimedia/qspi_lcd_example
```

编译成功后，固件位于 `build/qspi_lcd_example/bk7259/` 目录下。

### 4.2 烧录

使用 Beken 烧录工具，将生成的固件烧录到开发板。

### 4.3 运行

烧录完成后上电（或在串口输入 `reboot` 重启）：

* **现象**：屏幕会自动循环显示 **红 → 绿 → 蓝**，每种颜色停留约 3 秒（由 `qspi_lcd_auto_refresh_task` 驱动）。
* 看到屏幕循环变色，就代表 QSPI 屏已经成功点亮、背光已打开。

## 5. 串口命令

通过串口终端可以手动把屏幕刷成指定颜色：

| 命令 | 作用 |
| --- | --- |
| `qspi_lcd` | 把整屏刷成默认红色（`0xF800`） |
| `qspi_lcd <RGB565十六进制>` | 把整屏刷成指定颜色 |

颜色为 **RGB565** 格式的 16 进制值。常用颜色：

```text
qspi_lcd F800   # 红
qspi_lcd 07E0   # 绿
qspi_lcd 001F   # 蓝
qspi_lcd FFFF   # 白
qspi_lcd 0000   # 黑
```

> 提示：上电的自动刷色任务会一直循环运行；你手动输入 `qspi_lcd` 设置的颜色会在下一次自动循环到来时被覆盖。这是正常现象，方便快速验证屏幕。

## 6. 关键配置说明

### 6.1 功能开关（`ap/config/bk7259_ap/defconfig`）

| 配置项 | 含义 |
| --- | --- |
| `CONFIG_BK_DISPLAY=y` | 启用统一显示框架 `bk_display_*` |
| `CONFIG_FRAME_BUFFER=y` | 启用帧缓冲管理 |
| `CONFIG_LCD_QSPI=y` | 启用 QSPI LCD 驱动 |
| `CONFIG_LCD_QSPI_SPD2010=y` | 启用 spd2010 屏驱动 |
| `CONFIG_LCD_QSPI_COLOR_DEPTH_BYTE=2` | 每像素 2 字节（RGB565） |
| `CONFIG_QSPI=y` / `CONFIG_QSPI_LINE_MODE=4` | QSPI 四线模式 |
| `CONFIG_LCD_QSPI_TE=y` | 启用 TE 撕裂同步信号 |
| `CONFIG_LCD_QSPI_REFRESH_WITH_MAPPING=y` | 使用映射方式刷新 |
| `CONFIG_MEDIA_SERVICE=y` | 启用多媒体服务 |

### 6.2 显示参数（`ap/ap_main.c`）

```c
#define QSPI_LCD_BACKLIGHT_PIN GPIO_7
#define QSPI_LCD_RESET_PIN     GPIO_40
#define QSPI_LCD_TE_PIN        GPIO_41

static bk_display_qspi_ctlr_config_t qspi_ctlr_config = {
    .lcd_panel = &lcd_device_spd2010,        // 屏驱动
    .qspi_id   = 0,                          // 使用 QSPI0
    .reset_pin = QSPI_LCD_RESET_PIN,         // 复位脚
    .te_pin    = QSPI_LCD_TE_PIN,            // TE 同步脚
};
```

* 换屏：把 `lcd_device_spd2010` 换成对应屏驱动，并在 `defconfig` 中开启对应 `CONFIG_LCD_QSPI_xxx`。
* 改引脚：修改 `QSPI_LCD_RESET_PIN` / `QSPI_LCD_TE_PIN` / `QSPI_LCD_BACKLIGHT_PIN`，并同步修改 `usr_gpio_cfg.h` 中 QSPI0 各数据线的 GPIO 分配。
* 改自动刷色节奏：修改 `QSPI_LCD_AUTO_REFRESH_INTERVAL_MS`（默认 3000ms）和 `s_qspi_lcd_auto_colors[]` 颜色表。

## 7. 显示流程（代码原理）

`ap_main.c` 中第一次刷新时会完成一次性初始化（`qspi_lcd_display_init`）：

1. `bk_display_qspi_ctlr_new()`：根据 `qspi_ctlr_config` 创建 QSPI 显示控制器
2. `bk_display_init()` → `bk_display_open()`：初始化并打开屏幕
3. `bk_frame_buffer_malloc()`：按屏驱动 `qspi->frame_len` 申请帧缓冲
4. `lcd_backlight_open()`：拉高 GPIO_7 点亮背光
5. 之后每次刷新：`qspi_lcd_display_fill_pure_color()` 填色 → `bk_display_flush()` 刷新到屏幕

初始化只做一次（`is_display_init` 标志），失败时会走 `fail` 分支释放帧缓冲与控制器。

## 8. 注意事项

1. 上电自动刷色任务会持续运行，手动 `qspi_lcd` 命令的颜色会被下一轮自动循环覆盖，属正常现象。
2. 显示控制器只初始化一次，全程复用同一块帧缓冲；初始化失败会自动回收资源。
3. 颜色按 **RGB565** 解析，输入 16 进制（如 `F800`），超过 16 位的部分会被截断。
4. 若屏幕不亮：优先检查接线（尤其 RESET / 背光 GPIO_7）、6 根 QSPI 数据线、屏型号是否与 `defconfig` 一致。
5. QSPI 屏对时序与走线较敏感，4 线模式下请确保 IO0~IO3、CLK、CS 接线正确且等长。
6. 如果你的屏规格与参考外设不同，代码与配置需要重新适配。
