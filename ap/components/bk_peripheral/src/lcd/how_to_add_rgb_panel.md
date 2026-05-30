# 如何新增 RGB Panel（BK7259）

> 本文面向 **新增 RGB 屏驱动** 的开发者。MIPI 屏请参考同目录下的 `how_to_add_mipi_panel.md`。
> 全过程只需要：**新建 1 个 .c 文件 + 改 2 个配置文件 + 应用层引用 1 个符号**，不需要改任何驱动代码。

---

## 目录

1. [5 分钟概览](#1-5-分钟概览)
2. [总流程](#2-总流程)
3. [Step 1：准备 datasheet 信息](#3-step-1准备-datasheet-信息)
4. [Step 2：新建 panel 驱动文件](#4-step-2新建-panel-驱动文件)
5. [Step 3：在 Kconfig 中声明](#5-step-3在-kconfig-中声明)
6. [Step 4：在 config.cmake 中加入构建](#6-step-4在-configcmake-中加入构建)
7. [Step 5：在工程中启用并使用](#7-step-5在工程中启用并使用)
8. [描述符字段速查表](#8-描述符字段速查表)
9. [初始化命令编写规则](#9-初始化命令编写规则)
10. [`custom_reset` 用法](#10-custom_reset-用法)
11. [现有 panel 一览（可参考）](#11-现有-panel-一览可参考)
12. [应用代码模板（拷贝即用）](#12-应用代码模板拷贝即用)
13. [验证清单](#13-验证清单)
14. [常见问题速查](#14-常见问题速查)

---

## 1. 5 分钟概览

新增一块 RGB 屏需要做什么？

```
你的工作                       系统已经做好的事
────────────────────────────  ────────────────────────────
新建 1 个 .c 文件          →   bk_display 自动识别、初始化、点亮
   (panel 描述符 + init cmds)
改 1 个 Kconfig             →   menuconfig 中可勾选
改 1 个 config.cmake        →   编译期把 .c 拉进来
应用层引用 1 个 panel 符号  →   通过 RGB bus + DPU + flush 完成显示
```

完整跑通的代码可以参考 `projects/multimedia/rgb_lcd_example/`。

### RGB 屏在 BK7259 上的两种"通道"

请先理解这个概念，否则阅读后续内容会困惑：

| 通道 | 用途 | 物理形式 |
|---|---|---|
| **像素通道**（24-bit RGB） | 持续输出像素数据 | 24 根 RGB 数据线 + HSYNC/VSYNC/DE/PCLK，由 DPU 驱动 |
| **配置通道**（3-wire SPI） | 上电时下发屏厂初始化命令 | CSX / SDA / CLK + RESET 4 根 GPIO，软件 bit-bang |

像素通道由 DPU 在 `bk_display_init()` 中自动 pinmux（24 条数据线 + 同步信号是固定 IO_FUNCTION，不需要配置）；
配置通道是一条 **SW 模式的 SPI bus**（`bk_display_spi_bus_new` + `BK_DISPLAY_SPI_BUS_MODE_SW`），4 根 GPIO 由你在 `bk_display_spi_bus_config_t` 中指定。屏 RESET 引脚走 `bk_lcd_panel_dev_config_t.reset_pin`。

**你不需要做的**：
- ❌ 不需要写任何寄存器配置代码
- ❌ 不需要修改 `bk_display` 任何源码
- ❌ 不需要维护 ID 表

---

## 2. 总流程

```
┌─────────────────────────────────────────────────────────┐
│ Step 1  从屏厂拿 datasheet，整理：                       │
│   分辨率 / PCLK / 时序 porch / 命令格式 / 初始化命令序列 │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ Step 2  新建：                                           │
│   ap/components/bk_peripheral/src/lcd/rgb/               │
│       lcd_rgb_<vendor>_<WxH>.c                           │
│   填写 bk_display_rgb_panel_t 结构体                     │
│   末尾用 BK_LCD_PANEL_DEVICE_SECTION 注册                │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ Step 3  在 rgb/Kconfig 中加 CONFIG_LCD_<NAME>            │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ Step 4  在 rgb/config.cmake 中加条件编译                 │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ Step 5  工程 config 中启用 + 应用代码引用 panel 符号     │
└─────────────────────────────────────────────────────────┘
                          ↓
                       验证 & 调试
```

---

## 3. Step 1：准备 datasheet 信息

| 项 | 描述 | 写入字段 |
|---|---|---|
| 分辨率 | 如 480×854 | `timing.h_size` / `timing.v_size` |
| PCLK 频率 | 一般 20~50MHz，取屏厂典型值 | `pixel_clock_hz`（用 `BK_RGB_PIXEL_CLK_HZ(MHz)`） |
| 水平时序 | hsync 脉宽 / 后沿 / 前沿 | `timing.hsync_pulse_width` / `hsync_back_porch` / `hsync_front_porch` |
| 垂直时序 | vsync 脉宽 / 后沿 / 前沿 | `timing.vsync_pulse_width` / `vsync_back_porch` / `vsync_front_porch` |
| 配置命令位宽 | 8-bit 或 16-bit SPI | `spi_cmd_16bit`（0/1） |
| 初始化命令序列 | 厂家命令表 | `init_cmds` |
| ID 寄存器 | 用于 read_id 校验，可选 | `read_id_regs` / `read_id_bytes` / `id` |

**估算公式**：

```
fps = pixel_clock_hz
      ÷ (h_size + h_pulse + h_back + h_front)
      ÷ (v_size + v_pulse + v_back + v_front)
```

例：`30 MHz, 480×854, 时序之和 H=576/V=904`，fps ≈ 30 000 000 / 576 / 904 ≈ 57Hz。

> ⚠ 与 MIPI 屏不同，**RGB 屏的 `pixel_clock_hz` 必须填**（DPI 像素时钟），否则 fps 不对会撕裂或闪屏。
>
> 另外：**RGB 屏不需要应用层选择 `clk_src`**。RGB 通路没有 DSI PHY，DPI 像素时钟只能由 DPU 从系统时钟阶梯（SYSCLK）分频得到，`DPU_CLK_SRC_DPHY_DPLL` 在 RGB 路径上没有物理意义。因此 `bk_lcd_rgb_panel_new()` 内部会**无条件把 panel 句柄的时钟源锁定为 `DPU_CLK_SRC_SYSCLK`**：
>
> - `bk_lcd_panel_dev_config_t.clk_src` 留空 (`DPU_CLK_SRC_UNKNOWN`) 是推荐写法；
> - 仍然显式填 `DPU_CLK_SRC_SYSCLK` 也合法，无副作用；
> - 若误填 `DPU_CLK_SRC_DPHY_DPLL`，driver 会忽略并打印一行 `LOGW("RGB panel ...: ignoring clk_src=%d, forcing DPU_CLK_SRC_SYSCLK")`。
>
> 时钟原理详见 MIPI 文档 §15（与 RGB 屏的差异：RGB 屏无 PHY，没有 "lane:pclk" 约束，公式不适用）。

---

## 4. Step 2：新建 panel 驱动文件

文件路径规范：

```
ap/components/bk_peripheral/src/lcd/rgb/lcd_rgb_<vendor>_<WxH>.c
```

例：`lcd_rgb_st7701sn_480x854.c`、`lcd_rgb_nt35510_480x854.c`

### 完整模板（拷贝替换 `XXX` / `<...>`）

```c
// Copyright 2020-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License"); ...

/**
 * @file lcd_rgb_<vendor>_<WxH>.c
 * @brief <Vendor> RGB Panel Driver (<W>x<H>)
 */

#include <components/bk_display_types.h>
#include <components/bk_lcd_types.h>

#if CONFIG_LCD_<VENDOR_TAG>

/* ---- 1) 初始化命令序列（通过 3-wire SPI 下发，按 datasheet 顺序）---- */
static const lcd_rgb_spi_init_cmd_t <vendor>_rgb_<WxH>_init_cmds[] = {
    {0xFF, (const uint8_t []){0x77,0x01,0x00,0x00,0x13}, 5},
    {0xEF, (const uint8_t []){0x08}, 1},
    /* ... 中间命令省略 ... */

    {0x3A, (const uint8_t []){0x55}, 1},          /* Pixel Format: 0x55 = RGB565 */
    {0x36, (const uint8_t []){0x10}, 1},          /* MADCTL */
    {0x11, NULL, 0},                              /* Sleep Out（无参） */
    {0x00, (const uint8_t []){120}, 0xFF},       /* delay 120ms */
    {0x29, NULL, 0},                              /* Display On（无参） */

    {0x00, NULL, 0}                               /* 结束标记，必填 */
};

/* ---- 2) ID 寄存器列表（可选）---- */
static const uint8_t <vendor>_rgb_<WxH>_read_id_regs[] = {0xA1, 0};

/* ---- 3) Panel 描述符 ---- */
const bk_display_rgb_panel_t lcd_device_<vendor>_rgb_<WxH> = {
    .id              = 0x9903,                      /* 屏 IC ID */
    .name            = "<vendor>_rgb_<WxH>",        /* CLI 按此查找 */
    .pixel_clock_hz  = BK_RGB_PIXEL_CLK_HZ(30),     /* DPI PCLK，单位 Hz；30 MHz */
    .timing = {
        .h_size            = 480,
        .v_size            = 854,
        .hsync_pulse_width = 2,
        .vsync_pulse_width = 2,
        .hsync_back_porch  = 46,
        .hsync_front_porch = 48,
        .vsync_back_porch  = 24,
        .vsync_front_porch = 24,
    },
    .init_cmds     = <vendor>_rgb_<WxH>_init_cmds,
    .spi_cmd_16bit = 0,                           /* 0=8-bit cmd, 1=16-bit cmd（看屏型）*/
    .read_id_regs  = <vendor>_rgb_<WxH>_read_id_regs,
    .read_id_bytes = 2,
    .custom_reset  = NULL,                        /* 一般留 NULL */
};

/* ---- 4) 段注册（必须）：第 3 参数 0 表示 RGB panel ---- */
BK_LCD_PANEL_DEVICE_SECTION(lcd_device_<vendor>_rgb_<WxH>,
                            "<vendor>_rgb_<WxH>", 0);

#endif /* CONFIG_LCD_<VENDOR_TAG> */
```

> **变量命名规范**（强烈建议）：
> - 文件名 `lcd_rgb_<vendor>_<WxH>.c`
> - init_cmds 数组 `<vendor>_rgb_<WxH>_init_cmds`
> - read_id_regs 数组 `<vendor>_rgb_<WxH>_read_id_regs`
> - panel 描述符 `lcd_device_<vendor>_rgb_<WxH>`
> - section 注册 name `"<vendor>_rgb_<WxH>"`
> - Kconfig 项 `CONFIG_LCD_<VENDOR>`（参考现有命名约定）

---

## 5. Step 3：在 Kconfig 中声明

文件：`ap/components/bk_peripheral/src/lcd/rgb/Kconfig`

加一段：

```kconfig
	config LCD_<VENDOR_TAG>
		depends on LCD_RGB
		bool "Enable LCD_<VENDOR_TAG> API"
		default n
```

`depends on LCD_RGB` 必须，否则 menuconfig 中看不到。

---

## 6. Step 4：在 config.cmake 中加入构建

文件：`ap/components/bk_peripheral/src/lcd/rgb/config.cmake`

加一段：

```cmake
if (CONFIG_LCD_<VENDOR_TAG>)
	list(APPEND RGB_LCD_DEVICE_FILES ${RGB_LCD_PATH}/lcd_rgb_<vendor>_<WxH>.c)
endif()
```

---

## 7. Step 5：在工程中启用并使用

### 7.1 启用配置

工程 defconfig（如 `projects/multimedia/rgb_lcd_example/ap/config/bk7259_ap/config`）中加：

```bash
CONFIG_LCD_<VENDOR_TAG>=y
```

或交互式启用：

```bash
cd build/bk7259/<project>/bk7259_ap
make menuconfig
# 进入 BK_PERIPHERAL → RGB LCD → 勾选你的 panel
```

### 7.2 在应用代码中引用 panel 符号

两种方式（推荐第二种）：

**方式 A：直接引用结构体（适合固定单屏）**

```c
extern const bk_display_rgb_panel_t lcd_device_<vendor>_rgb_<WxH>;
const bk_display_rgb_panel_t *panel = &lcd_device_<vendor>_rgb_<WxH>;
```

**方式 B：通过段注册按 name 动态查找（适合 CLI/可换屏）**

```c
const bk_display_rgb_panel_t *list[8];
uint32_t n = bk_lcd_get_rgb_panel_list(list, 8);
const bk_display_rgb_panel_t *panel = NULL;
for (uint32_t i = 0; i < n; i++) {
    if (os_strcmp(list[i]->name, "<vendor>_rgb_<WxH>") == 0) {
        panel = list[i];
        break;
    }
}
```

完整启动序列见第 12 节。

---

## 8. 描述符字段速查表

`bk_display_rgb_panel_t`（定义在 `ap/include/components/bk_display_types.h`）：

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `id` | `uint32_t` | 推荐 | 屏 IC ID，`bk_lcd_panel_read_id` 校验用 |
| `name` | `const char *` | **必填** | CLI / 段查找用，建议 `<vendor>_rgb_<WxH>` |
| `pixel_clock_hz` | `uint32_t` | **必填** | DPI PCLK 频率（Hz），用 `BK_RGB_PIXEL_CLK_HZ(MHz)` 宏。如 `BK_RGB_PIXEL_CLK_HZ(30)` |
| `timing.h_size` | `uint16_t` | **必填** | 水平有效像素 |
| `timing.v_size` | `uint16_t` | **必填** | 垂直有效像素 |
| `timing.hsync_pulse_width` | `uint16_t` | **必填** | hsync 脉宽（单位 pclk） |
| `timing.hsync_back_porch` | `uint16_t` | **必填** | hsync 后沿 |
| `timing.hsync_front_porch` | `uint16_t` | **必填** | hsync 前沿 |
| `timing.vsync_pulse_width` | `uint16_t` | **必填** | vsync 脉宽（单位 line） |
| `timing.vsync_back_porch` | `uint16_t` | **必填** | vsync 后沿 |
| `timing.vsync_front_porch` | `uint16_t` | **必填** | vsync 前沿 |
| `init_cmds` | `const lcd_rgb_spi_init_cmd_t *` | **必填** | 初始化命令序列，必须以 `{0x00, NULL, 0}` 结尾 |
| `spi_cmd_16bit` | `uint8_t` | **必填** | `0` = 命令码 8-bit；`1` = 命令码 16-bit。多数屏是 0 |
| `read_id_regs` | `const uint8_t *` | 可选 | ID 寄存器地址数组，以 `0` 结尾 |
| `read_id_bytes` | `uint8_t` | 与 `read_id_regs` 配套 | 1~3，读几字节拼成 ID |
| `custom_reset` | 函数指针 | 可选 | NULL 用通用 reset；特殊屏见 §10 |

> **`bk_lcd_panel_dev_config_t`**（`bk_lcd_rgb_panel_new` 第 2 参）现在只需要填 `reset_pin` 与 `reset_active_level` 两个字段；`clk_src` 对 RGB 屏不必填（driver 内部强制为 `DPU_CLK_SRC_SYSCLK`，详见上一节）。其它历史字段（`clk_pin` / `csx_pin` / `sda_pin` / `rgb_ele_order` / `data_endian` / `bits_per_pixel` 等）已从结构体里移除，旧代码若仍引用需一并删除。`clk_src` 在 panel 创建时立即推送到 bus，DPU 控制器随后从 panel 句柄读回。

---

## 9. 初始化命令编写规则

`lcd_rgb_spi_init_cmd_t` 定义（`ap/include/components/bk_lcd_types.h`）：

```c
typedef struct {
    uint16_t      cmd;       /* 8-bit 模式用低 8 位；16-bit 模式用全 16 位 */
    const void   *data;      /* 参数指针，无参传 NULL */
    uint8_t       data_len;  /* 参数长度；0xFF 时表示这是 delay */
} lcd_rgb_spi_init_cmd_t;
```

### 9.1 三种合法形式

| 形式 | 写法 | 含义 |
|---|---|---|
| **带参命令** | `{0x36, (const uint8_t []){0x10}, 1}` | 发命令 0x36，1 字节参数 0x10 |
| **无参命令** | `{0x11, NULL, 0}` | 发命令 0x11，无参 |
| **延迟（无命令）** | `{0x00, (const uint8_t []){120}, 0xFF}` | 等待 120ms，不发任何命令 |
| **结束标记** | `{0x00, NULL, 0}` | 必填 |

### 9.2 16-bit 命令模式

少数屏（如部分定制 IC）使用 16-bit 命令格式。此时：

- `spi_cmd_16bit` 设为 `1`
- 命令字写完整 16 位（如 `0xF000`）
- `data` 数组通常仍是 `uint8_t`（少数屏要求 16-bit data，按 datasheet）
- delay 命令格式不变（仍是 `cmd=0x00, data_len=0xFF`）

### 9.3 ⚠ delay 常见错误

- **不要**用 `{0x11, (const uint8_t []){120}, 0xFF}` 想表达"先发 0x11 再延迟"——格式不被支持。
  正确写法：拆成两条
  ```c
  {0x11, NULL, 0},                          /* Sleep Out */
  {0x00, (const uint8_t []){120}, 0xFF},   /* delay 120ms */
  ```
- delay 命令的 `data` **不能是 NULL**
- delay 命令的 `cmd` 必须是 `0x00`（无论是 8-bit 还是 16-bit 模式）
- 普通命令的 `data_len` **不能等于 `0xFF`**（会被误判为 delay）

### 9.4 实战示例（截取自 `lcd_rgb_st7701sn_480x854.c`）

```c
static const lcd_rgb_spi_init_cmd_t st7701sn_rgb_480x854_init_cmds[] = {
    {0xFF, (const uint8_t []){0x77,0x01,0x00,0x00,0x13}, 5},
    {0xEF, (const uint8_t []){0x08}, 1},
    /* ... 数十条厂家配置命令 ... */
    {0x3A, (const uint8_t []){0x55}, 1},          /* Pixel Format = RGB565 */
    {0x36, (const uint8_t []){0x10}, 1},          /* MADCTL */
    {0x11, NULL, 0},                              /* Sleep Out */
    {0x00, (const uint8_t []){120}, 0xFF},        /* delay 120ms */
    {0x29, NULL, 0},                              /* Display On */
    {0x00, NULL, 0}                               /* 结束 */
};
```

---

## 10. `custom_reset` 用法

通用 reset 时序：`reset_pin` 高 → 低 → 高，每段 ≥ 10ms。

只有当屏厂时序明显不同时才需要自定义：

```c
static bk_err_t my_reset(bk_avdk_lcd_panel_t *panel, void *priv)
{
    bk_gpio_set_output_high(MY_RST_PIN);
    rtos_delay_milliseconds(50);
    bk_gpio_set_output_low(MY_RST_PIN);
    rtos_delay_milliseconds(20);
    bk_gpio_set_output_high(MY_RST_PIN);
    rtos_delay_milliseconds(200);
    return BK_OK;
}

const bk_display_rgb_panel_t lcd_device_xxx = {
    /* ... */
    .custom_reset = my_reset,
};
```

> 注意：RGB 描述符 **没有** `custom_init` 字段（与 MIPI 不同）。RGB 屏的额外初始化通常通过给屏 IC 发 SPI 命令完成，写在 `init_cmds` 里即可。

---

## 11. 现有 panel 一览（可参考）

| 文件 | 屏 IC | 分辨率 | PCLK | Kconfig |
|---|---|---|---|---|
| `lcd_rgb_aml01_720x1280.c` | AML01 | 720×1280 | - | `LCD_AML01` |
| `lcd_rgb_fpga272p_480x384.c` | FPGA272P | 480×384 | - | `LCD_FPGA272P` |
| `lcd_rgb_gc9503v_480x800.c` | GC9503V | 480×800 | - | `LCD_GC9503V` |
| `lcd_rgb_h050iwv_800x480.c` | H050IWV | 800×480 | - | `LCD_H050IWV` |
| `lcd_rgb_hx8282_1024x600.c` | HX8282 | 1024×600 | - | `LCD_HX8282` |
| `lcd_rgb_md0430r_800x480.c` | MD0430R | 800×480 | - | `LCD_MD0430R` |
| `lcd_rgb_md0700r_1024x600.c` | MD0700R | 1024×600 | - | `LCD_MD0700R` |
| `lcd_rgb_nt35510_480x854.c` | NT35510 | 480×854 | - | `LCD_NT35510` |
| `lcd_rgb_nt35512_480x800.c` | NT35512 | 480×800 | - | `LCD_NT35512` |
| `lcd_rgb_st7282_480x272.c` | ST7282 | 480×272 | - | `LCD_ST7282` |
| `lcd_rgb_st7701s_480x480.c` | ST7701S | 480×480 | - | `LCD_ST7701S` |
| `lcd_rgb_st7701sn_480x854.c` | ST7701SN | 480×854 | 30M | `LCD_ST7701SN` |

> **建议**：先找一块分辨率最接近你目标屏的现有文件作为模板复制。

---

## 12. 应用代码模板（拷贝即用）

参考 `projects/multimedia/rgb_lcd_example/ap/src/lcd_example_rgb.c`，简化版：

```c
#include <components/bk_display.h>
#include <components/bk_display_bus.h>
#include <components/bk_display_dpu_ctlr.h>
#include <components/bk_lcd_panel.h>
#include <components/bk_lcd_types.h>

extern const bk_display_rgb_panel_t lcd_device_<vendor>_rgb_<WxH>;

static struct {
    bk_display_bus_handle_t    dis_bus_handle;
    bk_avdk_lcd_panel_handle_t panel_handle;
    bk_display_ctlr_handle_t   dpu_ctlr_handle;
} ctx;

int my_rgb_lcd_open(void)
{
    /* 1. 配置 SPI bus（SW 模式：bit-bang 下发 init 命令）。
     *    24-bit 并行 RGB 像素引脚由 DPU 直接驱动，不归本 bus 管。 */
    bk_display_spi_bus_config_t rgb_cfg_bus = {
        .mode       = BK_DISPLAY_SPI_BUS_MODE_SW,
        .clk_pin    = GPIO_8,    /* 配置 SPI 的 CLK */
        .csx_pin    = GPIO_28,   /* 配置 SPI 的 CSX */
        .sda_pin    = GPIO_9,    /* 配置 SPI 的 SDA */
        .cmd_width  = 8,         /* 等 panel 选好后改为 panel->spi_cmd_16bit ? 16 : 8 */
    };
    bk_display_spi_bus_new(&ctx.dis_bus_handle, &rgb_cfg_bus);

    /* 2. 创建 panel 句柄。RGB 屏 *不需要* 传 clk_src：
     *    RGB 通路没有 DSI PHY，DPI pclk 只能由 DPU 从 SYSCLK 阶梯分频得到，
     *    bk_lcd_rgb_panel_new() 内部会无条件把 panel 句柄的时钟源锁定为
     *    DPU_CLK_SRC_SYSCLK。若你照旧填 DPU_CLK_SRC_SYSCLK 也合法、无副作用；
     *    若误填 DPU_CLK_SRC_DPHY_DPLL，会被忽略并打印一行 LOGW。 */
    bk_lcd_panel_dev_config_t panel_dev_cfg = {
        .reset_pin          = GPIO_6,
        .reset_active_level = false,
        /* .clk_src 留空（=DPU_CLK_SRC_UNKNOWN）：driver 内部强制 SYSCLK */
    };
    bk_lcd_rgb_panel_new(ctx.dis_bus_handle, &panel_dev_cfg,
                         &lcd_device_<vendor>_rgb_<WxH>,
                         &ctx.panel_handle);

    bk_lcd_panel_reset(ctx.panel_handle);
    bk_lcd_panel_init(ctx.panel_handle);

    /* 3. 建 DPU 控制器：timing / pixel_clock_hz / clk_src 都在 panel
     *    句柄上，DPU 控制器内部直接读取，dpu_cfg 只承载层与像素格式意图。 */
    bk_display_dpu_config_t dpu_cfg = {
        .video.enable = true,
        .video.format = BK_PIXEL_FORMAT_RGB565,
    };

    /* 4. 启动 DPU */
    bk_display_dpu_ctlr_new(&ctx.dpu_ctlr_handle, ctx.panel_handle, &dpu_cfg);
    bk_display_init(ctx.dpu_ctlr_handle);
    bk_display_open(ctx.dpu_ctlr_handle);

    /* 5.（可选）打开背光 */
    /* bk_gpio_enable_output(GPIO_BL); bk_gpio_set_output_high(GPIO_BL); */

    return 0;
}

void my_rgb_lcd_close(void)
{
    bk_display_close (ctx.dpu_ctlr_handle);
    bk_display_deinit(ctx.dpu_ctlr_handle);
    bk_display_delete(ctx.dpu_ctlr_handle);
    bk_lcd_panel_reset(ctx.panel_handle);
    bk_lcd_panel_del  (ctx.panel_handle);
    bk_display_bus_delete(ctx.dis_bus_handle);
}
```

---

## 13. 验证清单

按顺序逐项打勾：

- [ ] 编译通过：`ninja` 无 warning（`unused-variable` 之类除外）
- [ ] `bk_lcd_get_rgb_panel_list()` 返回的列表中能看到你的 `name`
- [ ] `bk_display_bus_enable()` 后用示波器测 PCLK 引脚有稳定时钟
- [ ] `bk_lcd_panel_reset()` 后用示波器测 RESET 引脚有干净的 H/L/H 时序
- [ ] `bk_lcd_panel_init()` 返回 BK_OK；用逻辑分析仪抓 SPI（CSX/SDA/CLK）能看到完整命令流
- [ ] `bk_display_open()` 后送一帧纯色，整屏单色稳定无横纹
- [ ] 送渐变 / 测试图，无明显条纹、撕裂、颜色错位
- [ ] R/G/B 三原色顺序正确（如错位见 §14）
- [ ] 100 次 open/close 循环不死机不漏内存

---

## 14. 常见问题速查

| 现象 | 排查首项 | 排查次项 |
|---|---|---|
| 整屏黑、电流正常 | RESET 时序、init 命令是否真的下发（逻辑分析仪抓 SPI） | RGB IO pinmux 是否成功（看 PCLK 是否在跳） |
| 整屏黑、电流异常 | VCC_LCD / 屏背光 | 板子上 LDO 是否使能 |
| 整屏白底/雪花 | DPI clock 太高或 porch 偏小 | 降 `timing.clk` 或按 datasheet 重算 porch |
| 规则横纹 | `hsync_back_porch` / `hsync_front_porch` 偏小，DPU FIFO underflow | 加大 porch；确认 dpu QoS=3 |
| 规则竖纹 / 抖动条 | PCLK 极性与屏采样沿不一致 | 屏侧 datasheet 看 PCLK 极性；本平台暂不开放反相，请联系驱动同事 |
| R/B 互换 | 屏自身 BGR 模式寄存器没设对 | 检查 init_cmds 中 0x36 (MADCTL) 配置 |
| `read_id` 全 0 / 全 F | VCC_LCD / 配置 SPI 引脚错 | 用万用表测 CSX/SDA/CLK 是否到屏脚；测 VCC_LCD |
| init 序列发完屏没反应 | `spi_cmd_16bit` 配错（屏要 16-bit 你填了 0） | 抓 SPI 看实际命令字宽是否对得上屏 spec |
| init 中某条命令后屏就坏 | 该命令前缺 delay | 在该命令前插 `{0x00, (const uint8_t []){xx}, 0xFF}` |
| 撕裂 | flush 频率高于 panel 实际帧率 | 降低 flush 频率或 `timing.clk` 降一档 |
| 切像素格式后花屏 | 切换瞬间仍有 inflight flush | 切换前先停 flush 线程 → ioctl → 再启 flush |
| close 后再 open 卡死 | `bus_delete` 是否调用 | RGB pinmux 可能没释放 |
| menuconfig 看不到新 panel | Kconfig 中 `depends on LCD_RGB` 是否漏 | `config LCD_xxx` 名字是否拼对 |
| 段注册取不到 panel | `BK_LCD_PANEL_DEVICE_SECTION` 是否漏 | 第 3 参数 type 必须为 `0`（RGB） |
| 编译时找不到结构体 | `#include <components/bk_display_types.h>` 是否漏 | `#if CONFIG_LCD_xxx` 与 Kconfig 名字是否一致 |

---

## 参考

- 现有实现：`ap/components/bk_peripheral/src/lcd/rgb/lcd_rgb_st7701sn_480x854.c`（推荐作为模板）
- 通用驱动：`ap/components/bk_display/src/bus/lcd_rgb_panel_common.c`
- 数据结构：`ap/include/components/bk_display_types.h` / `ap/include/components/bk_lcd_types.h`
- 应用样例：`projects/multimedia/rgb_lcd_example/ap/src/lcd_example_rgb.c`
- RGB 调试细节：参见 `BK7259 LCD 调试文档`
