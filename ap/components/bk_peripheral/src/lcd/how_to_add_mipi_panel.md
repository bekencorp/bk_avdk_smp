# 如何新增 MIPI DSI Panel（BK7259）

> 本文面向 **新增 MIPI 屏驱动** 的开发者。RGB 屏请参考同目录下的 `how_to_add_rgb_panel.md`。
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
10. [`custom_reset` 与 `custom_init` 用法](#10-custom_reset-与-custom_init-用法)
11. [Bridge IC（如 LT8912B）的 `vendor_config` 用法](#11-bridge-ic如-lt8912b-的-vendor_config-用法)
12. [现有 panel 一览（可参考）](#12-现有-panel-一览可参考)
13. [应用代码模板（拷贝即用）](#13-应用代码模板拷贝即用)
14. [验证清单](#14-验证清单)
15. [常见问题速查](#15-常见问题速查)

---

## 1. 5 分钟概览

新增一块 MIPI 屏需要做什么？

```
你的工作                       系统已经做好的事
────────────────────────────  ────────────────────────────
新建 1 个 .c 文件          →   bk_display 自动识别、初始化、点亮
   (panel 描述符 + init cmds)
改 1 个 Kconfig             →   menuconfig 中可勾选
改 1 个 config.cmake        →   编译期把 .c 拉进来
应用层引用 1 个 panel 符号  →   通过 bus + dpu + flush 完成显示
```

完整流程已经走通的代码可以参考 `projects/multimedia/doorbell/` 与 `projects/multimedia/mipi_lcd_example/`。

**你不需要做的**：
- ❌ 不需要写任何寄存器配置代码（DSI 时钟、PHY、lane 等都由 `bk_display` 自动算）
- ❌ 不需要修改 `bk_display` 任何源码
- ❌ 不需要维护 ID 表（链接段自动收集）
- ❌ 不需要修改链接脚本

---

## 2. 总流程

```
┌─────────────────────────────────────────────────────────┐
│ Step 1  从屏厂拿 datasheet，整理：                       │
│   分辨率 / 帧率 / lane 数 / 时序 porch / 初始化命令序列  │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ Step 2  新建：                                           │
│   ap/components/bk_peripheral/src/lcd/dsi/               │
│       lcd_mipi_<vendor>_<WxH>.c                          │
│   填写 bk_display_dsi_panel_t 结构体                     │
│   末尾用 BK_LCD_PANEL_DEVICE_SECTION 注册                │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ Step 3  在 dsi/Kconfig 中加 CONFIG_LCD_<NAME>_MIPI_<WxH> │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ Step 4  在 dsi/config.cmake 中加条件编译                 │
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

下手写代码前，请从屏厂拿到下面 7 项信息：

| 项 | 描述 | 写入字段 |
|---|---|---|
| 分辨率 | 如 480×640 | `timing.h_size` / `timing.v_size` |
| 帧率 | 如 30fps、60fps | `fps` |
| MIPI lane 数 | 1 / 2 / 3 / 4 | `n_lanes` |
| 水平时序 | hsync 脉宽 / 后沿 / 前沿 | `timing.hsync_pulse_width` / `hsync_back_porch` / `hsync_front_porch` |
| 垂直时序 | vsync 脉宽 / 后沿 / 前沿 | `timing.vsync_pulse_width` / `vsync_back_porch` / `vsync_front_porch` |
| 初始化命令序列 | DCS/MCS 命令表 | `init_cmds` |
| ID 寄存器 | 用于 read_id 校验，可选 | `read_id_regs` / `read_id_bytes` / `id` |

**经验值**（屏厂没给时的默认推算公式）：

- DPI 像素时钟 ≈ `(h_size + h_sum_porch) × (v_size + v_sum_porch) × fps`
- DSI 单 lane 速率 ≈ `DPI clock × bits_per_pixel / n_lanes`，必须落在屏 spec 的 lane bit-rate 范围内
- 如果不确定 porch，先按现有相近分辨率屏的值试

---

## 4. Step 2：新建 panel 驱动文件

文件路径规范：

```
ap/components/bk_peripheral/src/lcd/dsi/lcd_mipi_<vendor>_<WxH>.c
```

例：`lcd_mipi_st7701sn_480x640.c`、`lcd_mipi_jd9855_360x390.c`

### 完整模板（拷贝替换 `XXX` / `<...>`）

```c
// Copyright 2020-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License"); ...

/**
 * @file lcd_mipi_<vendor>_<WxH>.c
 * @brief <Vendor> MIPI DSI Panel Driver (<W>x<H>)
 */

#include <components/bk_display_types.h>
#include <components/bk_lcd_types.h>
#include <driver/mipi_dsi_types.h>
#include <common/avdk_pixel_types.h>

#if CONFIG_LCD_<VENDOR>_MIPI_<WxH>

/* ---- 1) 初始化命令序列（按 datasheet 顺序逐条填入）---- */
static const lcd_mipi_init_cmd_t <vendor>_mipi_<WxH>_init_cmds[] = {
    {0xFF, (const uint8_t []){0x77,0x01,0x00,0x00,0x13}, 5},
    {0xEF, (const uint8_t []){0x08}, 1},
    /* ... 中间命令省略 ... */

    /* 需要等待时插入 delay：cmd=0, data_len=0xFF, data[0]=毫秒数 */
    {0x00, (const uint8_t []){120}, 0xFF},   /* delay 120ms */

    {0x11, NULL, 0},                          /* Sleep Out（无参） */
    {0x00, (const uint8_t []){120}, 0xFF},   /* delay 120ms */
    {0x29, NULL, 0},                          /* Display On（无参） */

    {0x00, NULL, 0}                           /* 结束标记，必填 */
};

/* ---- 2) ID 寄存器列表（可选，省略则 read_id 不可用）---- */
static const uint8_t <vendor>_mipi_<WxH>_read_id_regs[] = {0xA1, 0};

/* ---- 3) Panel 描述符（结构体常量，链接进只读段）---- */
const bk_display_dsi_panel_t lcd_device_<vendor>_mipi_<WxH> = {
    .id            = 0x9903,                  /* 屏 IC ID，read_id 校验用 */
    .name          = "<vendor>_mipi_<WxH>",   /* 字符串名，CLI 按此查找 */
    .n_lanes       = DSI_ACTIVE_LANES_1,      /* 1/2/3/4 */
    .fps           = 30,
    .timing = {
        .h_size            = PIXEL_480,
        .v_size            = PIXEL_640,
        .hsync_pulse_width = 5,
        .vsync_pulse_width = 5,
        .hsync_back_porch  = 30,
        .hsync_front_porch = 30,
        .vsync_back_porch  = 20,
        .vsync_front_porch = 20,
    },
    .init_cmds     = <vendor>_mipi_<WxH>_init_cmds,
    .read_id_regs  = <vendor>_mipi_<WxH>_read_id_regs,
    .read_id_bytes = 2,                       /* 实际读多少字节，0~3 */
    .custom_reset  = NULL,                    /* 一般留 NULL，使用通用 reset */
    .custom_init   = NULL,                    /* 一般留 NULL，特殊场景见 §10 */
};

/* ---- 4) 段注册（必须）：让 bk_display 自动发现 ---- */
/* 第三参数 1 表示 DSI panel；0 表示 RGB panel */
BK_LCD_PANEL_DEVICE_SECTION(lcd_device_<vendor>_mipi_<WxH>,
                            "<vendor>_mipi_<WxH>", 1);

#endif /* CONFIG_LCD_<VENDOR>_MIPI_<WxH> */
```

> **变量命名规范**（强烈建议）：
> - 文件名 `lcd_mipi_<vendor>_<WxH>.c`
> - init_cmds 数组 `<vendor>_mipi_<WxH>_init_cmds`
> - read_id_regs 数组 `<vendor>_mipi_<WxH>_read_id_regs`
> - panel 描述符 `lcd_device_<vendor>_mipi_<WxH>`
> - section 注册 name `"<vendor>_mipi_<WxH>"`
> - Kconfig 项 `CONFIG_LCD_<VENDOR>_MIPI_<WxH>`
>
> 命名一致才能在所有 CLI/日志/grep 中保持可追踪。

---

## 5. Step 3：在 Kconfig 中声明

文件：`ap/components/bk_peripheral/src/lcd/dsi/Kconfig`

在合适位置加一段（保持字母序便于查找）：

```kconfig
	config LCD_<VENDOR>_MIPI_<WxH>
		depends on LCD_DSI
		bool "Enable LCD_<VENDOR>_MIPI_<WxH> API"
		default n
```

`depends on LCD_DSI` 是必须的，否则 menuconfig 中看不到。

---

## 6. Step 4：在 config.cmake 中加入构建

文件：`ap/components/bk_peripheral/src/lcd/dsi/config.cmake`

加一段：

```cmake
if (CONFIG_LCD_<VENDOR>_MIPI_<WxH>)
	list(APPEND DSI_LCD_DEVICE_FILES ${DSI_LCD_PATH}/lcd_mipi_<vendor>_<WxH>.c)
endif()
```

---

## 7. Step 5：在工程中启用并使用

### 7.1 启用配置

工程 defconfig（如 `projects/multimedia/doorbell/ap/config/bk7259_ap/config`）中加：

```bash
CONFIG_LCD_<VENDOR>_MIPI_<WxH>=y
```

或在工程目录下交互式启用：

```bash
cd build/bk7259/<project>/bk7259_ap
make menuconfig
# 进入 BK_PERIPHERAL → DSI LCD → 勾选你的 panel
```

### 7.2 在应用代码中引用 panel 符号

```c
extern const bk_display_dsi_panel_t lcd_device_<vendor>_mipi_<WxH>;

/* 然后把 &lcd_device_<vendor>_mipi_<WxH> 传给 bk_lcd_mipi_panel_new() */
/* 完整调用链见第 13 节模板 */
```

或通过段注册按 name 动态查找（适合 CLI、可换屏的场景）：

```c
const bk_display_dsi_panel_t *list[16];
uint32_t n = bk_lcd_get_mipi_panel_list(list, 16);
const bk_display_dsi_panel_t *panel = NULL;
for (uint32_t i = 0; i < n; i++) {
    if (os_strcmp(list[i]->name, "<vendor>_mipi_<WxH>") == 0) {
        panel = list[i];
        break;
    }
}
```

---

## 8. 描述符字段速查表

`bk_display_dsi_panel_t`（定义在 `ap/include/components/bk_display_types.h`）：

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `id` | `uint32_t` | 推荐 | 屏 IC ID。`bk_lcd_panel_read_id` 拿这个值校验，调试时一眼能看出是否真的连上了正确的屏 |
| `name` | `const char *` | **必填** | CLI / 日志 / 段查找用，建议格式 `<vendor>_mipi_<WxH>` |
| `n_lanes` | `uint8_t` | **必填** | 取值 `DSI_ACTIVE_LANES_1` ~ `DSI_ACTIVE_LANES_4`，必须与屏一致 |
| `fps` | `uint8_t` | **必填** | 期望帧率，与 timing 一起决定 PHY bit-rate |
| `timing.h_size` | `uint16_t` | **必填** | 水平有效像素，常用 `PIXEL_480` / `PIXEL_720` 等枚举 |
| `timing.v_size` | `uint16_t` | **必填** | 垂直有效像素 |
| `timing.hsync_pulse_width` | `uint16_t` | **必填** | hsync 脉宽，单位 pclk |
| `timing.hsync_back_porch` | `uint16_t` | **必填** | hsync 后沿 |
| `timing.hsync_front_porch` | `uint16_t` | **必填** | hsync 前沿 |
| `timing.vsync_pulse_width` | `uint16_t` | **必填** | vsync 脉宽，单位 line |
| `timing.vsync_back_porch` | `uint16_t` | **必填** | vsync 后沿 |
| `timing.vsync_front_porch` | `uint16_t` | **必填** | vsync 前沿 |
| `timing.clk` | `uint32_t` | 留 0 | DSI 屏不需要填，DPI 时钟由 fps × 总像素自动算；只对 RGB 屏有效 |
| `init_cmds` | `const lcd_mipi_init_cmd_t *` | **必填** | 初始化命令序列，必须以 `{0x00, NULL, 0}` 结尾 |
| `read_id_regs` | `const uint8_t *` | 可选 | ID 寄存器地址数组，以 `0` 结尾。NULL 则 `bk_lcd_panel_read_id` 返回错 |
| `read_id_bytes` | `uint8_t` | 与 `read_id_regs` 配套 | 1~3，表示读几个字节拼成 ID |
| `custom_reset` | 函数指针 | 可选 | NULL 用通用 reset；特殊屏需要自定义时见 §10 |
| `custom_init` | 函数指针 | 可选 | NULL 用通用 init；bridge IC、需要 I2C 配置的屏见 §10/§11 |

---

## 9. 初始化命令编写规则

`lcd_mipi_init_cmd_t` 定义（`ap/include/components/bk_lcd_types.h`）：

```c
typedef struct {
    uint8_t       cmd;       /* DCS / MCS 命令字 */
    const void   *data;      /* 参数指针，无参传 NULL */
    uint8_t       data_len;  /* 参数长度（字节）；0xFF 时表示这是 delay */
} lcd_mipi_init_cmd_t;
```

### 9.1 三种合法形式

| 形式 | 写法 | 含义 |
|---|---|---|
| **带参命令** | `{0x36, (const uint8_t []){0x10}, 1}` | 发命令 0x36，1 字节参数 0x10 |
| **无参命令** | `{0x11, NULL, 0}` | 发命令 0x11，无参 |
| **延迟（无命令）** | `{0x00, (const uint8_t []){120}, 0xFF}` | 等待 120ms，不发任何命令 |
| **结束标记** | `{0x00, NULL, 0}` | 必填，告诉驱动序列到头了 |

### 9.2 ⚠ 关于 delay 的常见错误

- **不要**用 `{0x11, (const uint8_t []){120}, 0xFF}` 想表达"先发 0x11 再延迟 120ms"——这格式不被支持。
  正确写法：拆成两条
  ```c
  {0x11, NULL, 0},                          /* Sleep Out */
  {0x00, (const uint8_t []){120}, 0xFF},   /* delay 120ms */
  ```
- delay 命令的 `data` **不能是 NULL**，且 `data[0]` 必须 > 0
- delay 命令的 `cmd` 必须是 `0x00`
- 普通命令的 `data_len` **不能等于 `0xFF`**（会被误判为 delay）

### 9.3 实战示例（截取自 `lcd_mipi_st7701sn_480x640.c`）

```c
static const lcd_mipi_init_cmd_t st7701sn_mipi_480x640_init_cmds[] = {
    {0xFF, (const uint8_t []){0x77,0x01,0x00,0x00,0x13}, 5},  /* CMD2 BK0 enable */
    {0xEF, (const uint8_t []){0x08}, 1},
    /* ... 数十条厂家配置命令 ... */
    {0xFF, (const uint8_t []){0x77,0x01,0x00,0x00,0x00}, 5},  /* exit CMD2 */
    {0x11, (const uint8_t []){0x00}, 0},                       /* Sleep Out */
    {0x29, (const uint8_t []){0x00}, 0},                       /* Display On */
    {0x00, NULL, 0}                                            /* 结束 */
};
```

---

## 10. `custom_reset` 与 `custom_init` 用法

### 10.1 通用流程

`bk_lcd_panel_init()` 内部按以下顺序执行：

```
custom_reset (NULL 时用通用 reset：reset_pin H→L→H + 120ms)
        ↓
DSI clock / PHY 拉起（按 timing + n_lanes 自动计算）
        ↓
逐条下发 init_cmds（含其中的 delay）
        ↓
custom_init (NULL 时跳过)
```

### 10.2 何时用 `custom_reset`

只有当屏厂的 reset 时序与默认（H/120ms → L/10ms → H/120ms）不一致时才需要：

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

const bk_display_dsi_panel_t lcd_device_xxx = {
    /* ... */
    .custom_reset = my_reset,
};
```

### 10.3 何时用 `custom_init`

适用场景：
- 通过 I2C/SPI 配置 bridge IC（如 LT8912B、TC358775）
- 在标准 `init_cmds` 跑完后，还要做一些"非 DCS"的额外初始化
- 切分辨率、切扫描方向等需要外部配合的逻辑

```c
static bk_err_t my_extra_init(bk_avdk_lcd_panel_t *panel, void *priv)
{
    /* priv 即应用层通过 panel_dev_config.vendor_config 传进来的指针，
       通常是另一条 bus（如 I2C bus）的 handle。详见 §11。*/
    bk_display_bus_handle_t i2c_bus = *(bk_display_bus_handle_t *)priv;
    /* ... 用 i2c_bus 配 bridge IC 寄存器 ... */
    return BK_OK;
}
```

---

## 11. Bridge IC（如 LT8912B）的 `vendor_config` 用法

`bk_lcd_mipi_panel_new` 第 2 个参数 `bk_lcd_panel_dev_config_t` 中有一个 `vendor_config` 指针，专门用来给 panel 传"额外的 bus 句柄"。

> **注意**：`bk_lcd_panel_dev_config_t` 中除了 `reset_pin` 和 `vendor_config` 外的字段
> （`rgb_ele_order` / `data_endian` / `bits_per_pixel` 等）当前版本**不再使用**，
> 保留只是为了向后兼容。新代码请只填 `reset_pin` 和（按需）`vendor_config`。

### 完整调用模板（doorbell 中的 LT8912B 用法）

```c
/* 1) 创建 DSI 主总线 */
bk_display_bus_handle_t dsi_bus;
bk_display_dsi_bus_new(&dsi_bus, NULL);
bk_display_bus_enable(dsi_bus);

/* 2) 创建配套的 I2C 总线（bridge IC 配置通道） */
bk_display_bus_handle_t cfg_bus = NULL;
if (panel_needs_bridge_config) {
    bk_display_i2c_bus_config_t i2c_cfg = {
        .scl_pin = GPIO_X,
        .sda_pin = GPIO_Y,
    };
    bk_display_i2c_bus_new(&cfg_bus, &i2c_cfg);
}

/* 3) 把 cfg_bus 通过 vendor_config 传给 panel */
bk_lcd_panel_dev_config_t panel_cfg = {
    .reset_pin     = GPIO_RESET,
    .vendor_config = (cfg_bus != NULL) ? &cfg_bus : NULL,
};

bk_avdk_lcd_panel_handle_t panel;
bk_lcd_mipi_panel_new(dsi_bus, &panel_cfg,
                      &lcd_device_lt8912b_mipi_bridge,  /* 你的 panel 描述符 */
                      &panel);
```

`custom_init` 内通过 `priv` 参数拿到 `cfg_bus`，再调 `bk_display_bus_send_command()` 配 bridge 寄存器即可。

---

## 12. 现有 panel 一览（可参考）

| 文件 | 屏 IC | 分辨率 | lane | fps | Kconfig |
|---|---|---|---|---|---|
| `lcd_mipi_hx8394f_720x1280.c` | HX8394F | 720×1280 | 4 | - | `LCD_HX8394F_MIPI_720x1280` |
| `lcd_mipi_hx8399c_1080x1920.c` | HX8399C | 1080×1920 | 4 | - | `LCD_HX8399C_MIPI_1080x1920` |
| `lcd_mipi_fl7703np_720x1280.c` | FL7703NP | 720×1280 | 4 | - | `LCD_FL7703NP_MIPI_720x1280` |
| `lcd_mipi_gc9702_720x1280.c` | GC9702 | 720×1280 | 4 | - | `LCD_GC9702_MIPI_720x1280` |
| `lcd_mipi_jd9522z_1080x1920.c` | JD9522Z | 1080×1920 | 4 | - | `LCD_JD9522Z_MIPI_1080x1920` |
| `lcd_mipi_jd9855_360x390.c` | JD9855 | 360×390 | 1 | 60 | `LCD_JD9855_MIPI_360x390` |
| `lcd_mipi_st7701s_412x960.c` | ST7701S | 412×960 | 2 | - | `LCD_ST7701S_MIPI_412x960` |
| `lcd_mipi_st7701sn_360x640.c` | ST7701SN | 360×640 | 1 | - | `LCD_ST7701SN_MIPI_360x640` |
| `lcd_mipi_st7701sn_480x640.c` | ST7701SN | 480×640 | 1 | 30 | `LCD_ST7701SN_MIPI_480x640` |
| `lcd_mipi_st7701sn_480x854.c` | ST7701SN | 480×854 | 1 | - | `LCD_ST7701SN_MIPI_480x854` |
| `lcd_mipi_lt8912b_bridge.c` | LT8912B(MIPI→HDMI) | 配置项决定 | - | - | `LCD_LT8912B_MIPI_BRIDGE` |

> **建议**：先找一块分辨率/lane 数最接近你目标屏的现有文件作为模板复制。

---

## 13. 应用代码模板（拷贝即用）

### 13.1 模板 A：单一固定屏（适合产品代码）

参考 `projects/common_components/doorbell_device_service/src/app_display.c`：

```c
#include <components/bk_display.h>
#include <components/bk_display_bus.h>
#include <components/bk_display_dpu_ctlr.h>
#include <components/bk_lcd_panel.h>
#include <components/bk_lcd_types.h>

extern const bk_display_dsi_panel_t lcd_device_<vendor>_mipi_<WxH>;

static struct {
    bk_display_bus_handle_t   dis_bus_handle;
    bk_display_bus_handle_t   cfg_bus_handle;     /* bridge IC 用，可选 */
    bk_avdk_lcd_panel_handle_t panel_handle;
    bk_display_ctlr_handle_t  dpu_ctlr_handle;
} ctx;

int my_lcd_open(void)
{
    /* 1. 建总线（DSI 主，可选 I2C 配置 bus） */
    bk_display_dsi_bus_new(&ctx.dis_bus_handle, NULL);
    bk_display_bus_enable(ctx.dis_bus_handle);

    /* 2. 建 panel 句柄 */
    bk_lcd_panel_dev_config_t panel_cfg = {
        .reset_pin     = GPIO_60,                /* 改成你板子上的实际 GPIO */
        .vendor_config = NULL,                    /* 没 bridge 时 NULL */
    };
    bk_lcd_mipi_panel_new(ctx.dis_bus_handle, &panel_cfg,
                          &lcd_device_<vendor>_mipi_<WxH>,
                          &ctx.panel_handle);

    bk_lcd_panel_reset(ctx.panel_handle);
    bk_lcd_panel_init(ctx.panel_handle);

    /* 3. 拿 panel timing 喂给 DPU */
    bk_display_dpu_config_t dpu_cfg = { /* clk_src / video 按工程约定填 */ };
    bk_lcd_panel_get_disp_timing(ctx.panel_handle, &dpu_cfg.timing);

    /* 4. 建 DPU 控制器并启动 */
    bk_display_dpu_ctlr_new(&ctx.dpu_ctlr_handle, &dpu_cfg);
    bk_display_init(ctx.dpu_ctlr_handle);
    bk_display_open(ctx.dpu_ctlr_handle);
    return 0;
}

void my_lcd_close(void)
{
    bk_display_close (ctx.dpu_ctlr_handle);
    bk_display_deinit(ctx.dpu_ctlr_handle);
    bk_display_delete(ctx.dpu_ctlr_handle);
    bk_lcd_panel_del (ctx.panel_handle);
    bk_display_bus_disable(ctx.dis_bus_handle);
    bk_display_bus_delete (ctx.dis_bus_handle);
}
```

### 13.2 模板 B：CLI 动态选屏（适合 example/调试）

```c
const bk_display_dsi_panel_t *list[16];
uint32_t n = bk_lcd_get_mipi_panel_list(list, 16);
const bk_display_dsi_panel_t *panel = NULL;

for (uint32_t i = 0; i < n; i++) {
    if (list[i]->name && os_strcmp(list[i]->name, name_from_cli) == 0) {
        panel = list[i];
        break;
    }
}
if (panel == NULL) { LOGE(TAG, "panel not found"); return; }

/* 之后流程同模板 A */
```

---

## 14. 验证清单

按顺序逐项打勾：

- [ ] 编译通过：`ninja` 无 warning（`unused-variable` 之类除外）
- [ ] `bk_lcd_get_mipi_panel_list()` 返回的列表中能看到你的 `name`
- [ ] `bk_lcd_panel_reset()` 后用示波器测 RESET 引脚有干净的 H/L/H 时序
- [ ] `bk_lcd_panel_init()` 返回 BK_OK，无 `AVDK_ERR_TIMEOUT/IO_ERROR`
- [ ] （若 `read_id_regs` 已配）`bk_lcd_panel_read_id()` 返回值与 `panel.id` 一致
- [ ] `bk_display_open()` 后送一帧纯红 / 纯绿 / 纯蓝 frame，整屏单色稳定无横纹
- [ ] 送渐变 / 测试图，无明显条纹、撕裂、颜色错位
- [ ] `BK_DISPLAY_IOCTL_DPU_PIXEL_FORMAT` 切换 RGB565/RGB888/NV12/ARGB8888 都正常
- [ ] 100 次 open/close 循环不死机不漏内存

---

## 15. 常见问题速查

| 现象 | 排查首项 | 排查次项 |
|---|---|---|
| 整屏黑、电流正常 | RESET 时序 / init 命令是否真的下发 | `n_lanes` 与 panel 是否匹配；`bk_display_bus_enable` 是否成功 |
| 整屏黑、电流异常 | VCC_LCD / 屏背光 | AUXLDO 2.8V/3V vote 是否打开 |
| `read_id` 返回全 0 / 0xFF | VCC_LCD 是否上电 | `read_id_regs` 与 `read_id_bytes` 是否填错 |
| 闪屏 / 帧率不稳 | DSI lane bit-rate 偏离屏 spec 推荐范围 | 调 `fps` 或 `n_lanes` 重新算 |
| 整屏黄/绿/蓝条带 | DPU FIFO underflow（PSRAM 带宽不够） | 确认 dpu QoS=3；其它高带宽模块（camera/h264）是否同时跑 |
| 撕裂 | flush 频率高于 panel 实际帧率 | 降低 flush 频率或 fps |
| 切像素格式后花屏 | 切换瞬间仍有 inflight flush | 切换前先停 flush 线程 → ioctl → 再启 flush |
| close 后再 open 卡死 | `bus_disable` / `bus_delete` 是否调用 | 子域电源没释放 |
| Bridge IC（LT8912B）无信号 | `vendor_config` 是否把 i2c bus 传进来 | `bk_display_i2c_bus_new` 是否成功 |
| menuconfig 看不到新 panel | 没在 Kconfig 加 `config LCD_xxx` | `depends on LCD_DSI` 是否漏写 |
| 段注册取不到 panel | `BK_LCD_PANEL_DEVICE_SECTION` 是否漏写 | 第 3 参 type 是否填错（必须为 1） |
| 编译时找不到结构体 | `#include <components/bk_display_types.h>` 是否漏 | `#if CONFIG_LCD_xxx` 与 Kconfig 名字是否一致 |

---

## 参考

- 现有实现：`ap/components/bk_peripheral/src/lcd/dsi/lcd_mipi_st7701sn_480x640.c`（推荐作为模板）
- 通用驱动：`ap/components/bk_display/src/bus/lcd_mipi_panel_common.c`
- 数据结构：`ap/include/components/bk_display_types.h` / `ap/include/components/bk_lcd_types.h`
- 应用样例：`projects/common_components/doorbell_device_service/src/app_display.c`、`projects/multimedia/mipi_lcd_example/`
- DSI 调试细节：参见 `BK7259 LCD 调试文档`
