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
15. [DPU 时钟源（`clk_src`）选择 与 DPHY PLL 工作区间](#15-dpu-时钟源clk_src选择-与-dphy-pll-工作区间)
    - 15.1 90 秒结论（小白看这一段就够）
    - 15.2 边界提醒
    - 15.3 想自己算一下（公式 + 例子，进阶）
    - 15.4 DPHY PLL 物理参数（参考）
    - 15.5 注意事项
16. [常见问题速查](#16-常见问题速查)

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
    .reset_active_level = false,              /* RST 引脚有效电平：false=低有效 */
    .reset         = bk_lcd_mipi_default_reset, /* 默认 GPIO H/L/H；NULL=跳过；见 §10 */
    .init          = bk_lcd_mipi_default_init,  /* 默认下发 init_cmds；NULL=跳过；见 §10 */
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
| `init_cmds` | `const lcd_mipi_init_cmd_t *` | **必填** | 初始化命令序列，必须以 `{0x00, NULL, 0}` 结尾 |
| `read_id_regs` | `const uint8_t *` | 可选 | ID 寄存器地址数组，以 `0` 结尾。NULL 则 `bk_lcd_panel_read_id` 返回错 |
| `read_id_bytes` | `uint8_t` | 与 `read_id_regs` 配套 | 1~3，表示读几个字节拼成 ID |
| `reset_active_level` | `bool` | **必填** | RST 引脚有效电平：`false` = active-low（绝大多数屏），`true` = active-high |
| `reset_timing.idle_ms` / `.active_ms` / `.release_ms` | `uint16_t` | 可选 | 自定义复位三段时序，0 = 用 `BK_DISPLAY_RESET_*_MS_DEFAULT` |
| `reset` | 函数指针 | **必填** | 标准屏填 `bk_lcd_mipi_default_reset`；不需要复位填 `NULL`；自定义见 §10 |
| `init` | 函数指针 | **必填** | 标准屏填 `bk_lcd_mipi_default_init`；不需要 init 填 `NULL`；bridge IC 见 §10/§11 |

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

## 10. `reset` 与 `init` 用法

### 10.1 通用流程

`bk_display_init(ctlr)` 在 DPU 控制器拉起前会按以下顺序驱动 panel 钩子：

```
bk_display_init(ctlr) → descriptor.reset            （NULL 时跳过）
                     → 框架 set_clock（含 DPHY_DPLL→SYSCLK 自动回退）
                     → descriptor.init              （NULL 时跳过）
                     → DPU build_core_config + dpu_core_init
```

> ⚠ `reset` / `init` 为 `NULL` **明确表示"跳过"**，不再隐式使用默认实现。
> 标准屏请显式赋 `bk_lcd_mipi_default_reset` / `bk_lcd_mipi_default_init`。
> 应用层不再单独调用 `bk_lcd_panel_reset/init`，bring-up 与销毁全程由 `bk_display_init/deinit` 负责。

### 10.2 自定义 `reset`

屏厂复位时序如果就是 H→delay→L→delay→H→delay 这种三段式，**不要写自定义函数**，
直接填 `.reset = bk_lcd_mipi_default_reset` 并通过 `reset_timing` / `reset_active_level`
调节三段时长和有效电平即可。只有真正需要更复杂时序（如多次脉冲、I2C 触发等）
才需要自己写：

```c
static bk_err_t my_reset(bk_avdk_lcd_panel_t *panel)
{
    /* 先复用默认 GPIO 复位 */
    bk_err_t ret = bk_lcd_mipi_default_reset(panel);
    if (ret != BK_OK) return ret;
    /* 再做一些额外动作，比如通过 DSI 发个 stub 命令 */
    return bk_lcd_panel_tx_param(panel, 0x00, NULL, 0);
}

const bk_display_dsi_panel_t lcd_device_xxx = {
    /* ... */
    .reset_active_level = false,
    .reset              = my_reset,
};
```

### 10.3 自定义 `init`

适用场景：
- 通过 I2C/SPI 配置 bridge IC（如 LT8912B、TC358775）
- 在标准 `init_cmds` 跑完后，还要做一些"非 DCS"的额外初始化
- 切分辨率、切扫描方向等需要外部配合的逻辑

```c
static bk_err_t my_init(bk_avdk_lcd_panel_t *panel)
{
    /* 先把 descriptor.init_cmds 跑完 */
    bk_err_t ret = bk_lcd_mipi_default_init(panel);
    if (ret != BK_OK) return ret;
    /* 再追加一两条特殊命令 */
    return bk_lcd_panel_tx_param(panel, 0x35, NULL, 0); /* TE on, e.g. */
}
```

> 自定义函数从 `bk_avdk_lcd_panel_t *panel` 只能访问公共能力：
> `bk_lcd_panel_tx_param() / bk_lcd_panel_rx_param()` 用来发送总线命令。
> 板级 GPIO 用 `bk_gpio_*` 直接操作即可。

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

`init` 钩子在新设计里只接收 `bk_avdk_lcd_panel_t *panel`，因此 bridge IC
所需的额外 bus 句柄通常通过 **文件静态变量**（参考 `lcd_mipi_lt8912b_bridge.c`
的 `s_lt8912b_i2c` 模式）或独立的 setter（`bk_lcd_lt8912b_set_io_pins()`）
向 panel 驱动注入，应用层在 `bk_lcd_mipi_panel_new()` 之前调用一次即可。
panel 自身的命令通道一律用 `bk_lcd_panel_tx_param(panel, ...)`。

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

    /* 2. （可选）需要强制时钟源时，在 panel_new 之前调用：
     *      bk_display_bus_set_clock_src(ctx.dis_bus_handle, DPU_CLK_SRC_SYSCLK);
     *    不调用 = 默认走 DPU_CLK_SRC_DPHY_DPLL，PHY PLL 反推不出来时
     *    驱动自动回退到 SYSCLK（详见 §15）。 */

    /* 3. 建 panel 句柄。reset_active_level 与 reset_timing 都来自 panel
     *    描述符，应用层只需告诉框架 RST 引脚是哪一根。 */
    bk_lcd_panel_dev_config_t panel_cfg = {
        .reset_pin = GPIO_60,                  /* 改成你板子上的实际 GPIO */
    };
    bk_lcd_mipi_panel_new(ctx.dis_bus_handle, &panel_cfg,
                          &lcd_device_<vendor>_mipi_<WxH>,
                          &ctx.panel_handle);

    /* 3. DPU 控制器：timing / pixel_clock_hz / clk_src 都已缓存在
     *    panel 句柄上，DPU 控制器内部直接读取，dpu_cfg 只承载层
     *    与像素格式意图。 */
    bk_display_dpu_config_t dpu_cfg = {
        .video.enable = true,
        /* video.format / video.decompress 按工程约定填 */
    };

    /* 4. 建 DPU 控制器并启动。bk_display_init 会顺次驱动
     *    descriptor.reset / set_clock / descriptor.init，再做 DPU 拉起。 */
    bk_display_dpu_ctlr_new(&ctx.dpu_ctlr_handle, ctx.panel_handle, &dpu_cfg);
    bk_display_init(ctx.dpu_ctlr_handle);
    bk_display_open(ctx.dpu_ctlr_handle);
    return 0;
}

void my_lcd_close(void)
{
    bk_display_close (ctx.dpu_ctlr_handle);
    bk_display_deinit(ctx.dpu_ctlr_handle);
    bk_display_delete(ctx.dpu_ctlr_handle);
    bk_lcd_panel_delete (ctx.panel_handle);
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
- [ ] `bk_display_init()` 期间用示波器测 RESET 引脚有干净的 H/L/H 时序
- [ ] `bk_display_init()` 返回 BK_OK；若失败串口会按 `panel reset err / panel init err / dpu core init err` 分阶段定位
- [ ] （若 `read_id_regs` 已配）`bk_display_init()` 之后调用 `bk_lcd_panel_read_id()` 与 `panel.id` 一致
- [ ] `bk_display_open()` 后送一帧纯红 / 纯绿 / 纯蓝 frame，整屏单色稳定无横纹
- [ ] 送渐变 / 测试图，无明显条纹、撕裂、颜色错位
- [ ] `BK_DISPLAY_IOCTL_DPU_PIXEL_FORMAT` 切换 RGB565/RGB888/NV12/ARGB8888 都正常
- [ ] 100 次 open/close 循环不死机不漏内存

---

## 15. DPU 时钟源 与 DPHY PLL 工作区间

DSI 总线创建时默认 `DPU_CLK_SRC_DPHY_DPLL`（精确求解 PHY PLL，让其内部
dpi_clk 等于 panel pclk）。当 panel 的 lane:pclk 比超过 PHY `pixdiv`
能给出的最大分频比（4-bit 字段，上限 17）时，`mipi_dsi_clock_set()`
**自动回退**到 SYSCLK 路径，串口打印 `WARN`，并把实际选择写回 bus 与
panel 句柄，DPU 控制器后续读到正确的 mux 源。

应用层无需关心，所以 `bk_lcd_panel_dev_config_t` 不再有 `clk_src`
字段。如需强制（比如出于 EMI 考虑想固定走 SYSCLK），在
`bk_display_dsi_bus_new()` 之后、`bk_lcd_mipi_panel_new()` 之前调一次：

```c
bk_display_dsi_bus_new(&bus, NULL);
bk_display_bus_set_clock_src(bus, DPU_CLK_SRC_SYSCLK);
bk_lcd_mipi_panel_new(bus, &panel_cfg, panel_desc, &panel);
```

### 15.1 90 秒结论

24bpp DSI 屏（绝大多数 panel）：

| panel `.n_lanes` | 框架选择 | 说明 |
|---|---|---|
| `DSI_ACTIVE_LANES_1`（1 lane） | 自动落 SYSCLK | DPHY_DPLL 反推无解（`lane:pclk` 需 > 17），打印一行 WARN |
| `DSI_ACTIVE_LANES_2/3/4`（≥ 2 lane） | DPHY_DPLL | 精度更高，PLL 路径可用 |

> **以前**：客户需要手填 `clk_src`，填错就硬失败。
> **现在**：默认就行，1 lane 屏（如 `lcd_mipi_jd9855_360x390.c`）开机会看到：
>
> ```
> dsi_core: PHY PLL miss for pclk=... lanes=1 ...
>           auto-fallback to DPU_CLK_SRC_SYSCLK
> dsi_core: using DPU_CLK_SRC_SYSCLK ...
> ```

### 15.2 边界提醒（30 秒）

**单 lane 屏的物理上限**：PHY 单 lane 最高 1600 Mbps，SYSCLK 兜底固定 800 Mbps。24bpp 24 × pclk_MHz ≤ 800 ⇒ **1 lane × 24bpp 实际可驱 pclk ≤ ~33 MHz**，对应 60fps 下大约 **480×854** 上限。再大的分辨率屏，**屏厂规格本身就会要求 ≥ 2 lane**，不会给到你 1 lane × 720p 的 datasheet。

**所以高分辨率 1-lane 这条路径不存在**，规则表 §15.1 已覆盖所有"屏厂真实可能给的组合"。

### 15.3 想自己算一下（公式 + 例子，进阶）

如果你对屏的参数不放心，或者屏比较奇葩（30fps、低 bpp、超高刷新率），按下面两步算一下：

**关卡 A：路径能不能走 PLL（lane:pclk 比例）**

```
ratio = bpp × (1 + overhead) / n_lanes      // bpp=24, overhead≈0.30
ratio ≤ 16  → 可以走 DPU_CLK_SRC_DPHY_DPLL  （PHY 把 pclk 精确锁到 panel 要求）
ratio > 16  → 必须走 DPU_CLK_SRC_SYSCLK     （DPU 自己分 sysclk，PHY 锁固定档）
```

24bpp 屏 ratio 表（前面 §15.1 推出的同一张）：

| n_lanes | ratio (24×1.3/n) | 走哪条路 |
|---|---|---|
| 1 | 31.2 | SYSCLK |
| 2 | 15.6 | DPHY_DPLL |
| 3 | 10.4 | DPHY_DPLL |
| 4 |  7.8 | DPHY_DPLL |

**关卡 B：物理带宽够不够（链路总速率）**

```
pclk_hz            = (h_size + h_porch_sum) × (v_size + v_porch_sum) × fps
need_per_lane_mbps = pclk_hz × bpp × (1 + overhead) / n_lanes / 1e6

要求：
  走 DPHY_DPLL → need_per_lane_mbps ≤ 1600
  走 SYSCLK    → need_per_lane_mbps ≤ 800
```

公式里的 `1e6` 就是 **10⁶ = 1,000,000**，作用是把 `bit/s` 换算成 `Mbit/s`（驱动里也是这么写的，见 `mipi_dsi_hal.c` 的 `den = lane_cnt * 1000000ULL * 1000ULL`，前者把 bps 转 Mbps，后者把 `overhead_permille` 千分比转倍数）。

**Worked example：ST7701S 412×960 / 2 lane / 60fps / 24bpp**

```
h_total = 412 + (10 + 28 + 50)  ≈ 500          // 实际查 datasheet 填
v_total = 960 + ( 2 +  8 + 10)  ≈ 980
pclk_hz = 500 × 980 × 60       ≈ 29.4 MHz
ratio   = 24 × 1.3 / 2          = 15.6  ≤ 16   → 可走 DPHY_DPLL ✓
need    = 29.4e6 × 24 × 1.3 / 2 / 1e6 ≈ 459 Mbps ≤ 1600 → 带宽 ✓
结论    : .clk_src = DPU_CLK_SRC_DPHY_DPLL
```

**Worked example：JD9855 360×390 / 1 lane / 60fps / 24bpp**

```
pclk_hz ≈ 360 × 390 × 60 × 1.x (porch) ≈ 13.3 MHz   // 实测打印为 13307520 Hz
ratio   = 24 × 1.3 / 1 = 31.2  > 16   → DPHY_DPLL 解不出，必须 SYSCLK
need    = 13.3e6 × 24 × 1.3 / 1 / 1e6 ≈ 415 Mbps ≤ 800  → SYSCLK 800 Mbps 带宽够 ✓
结论    : .clk_src = DPU_CLK_SRC_SYSCLK
```

### 15.4 DPHY PLL 物理参数（参考）

```
FVCO ∈ [1.2 GHz, 3.2 GHz]
lane_hs_bitrate = FVCO / 2^rate          (rate ∈ 0..7)
pclk            = lane_hs_bitrate / (pixdiv + 2)    (pixdiv ∈ 0..14 ⇒ 分频比 [2, 16])
工程有效区间    : 100 Mbps ~ 1600 Mbps（analog timing 查表覆盖范围）
SYSCLK 兜底     : 单 lane 固定 800 Mbps；其余分辨率走查表挡位
```

### 15.5 注意事项

- **不要**自己往 `dpu_cfg` 里塞 `.timing` / `.pixel_clock_hz` / `.clk_src`——这三者都在 panel 句柄里，DPU 控制器内部直接读取。
- 改 `clk_src` **不需要**改 panel 描述符；同一份 panel 描述符可同时支持两条时钟路径，应用层只需切 `panel_dev_config.clk_src`。
- DSI 命令模式（low-power）发 `init_cmds` 时由 byteclk 驱动，与 `clk_src` 选择无关；这一段只影响进 video 模式之后的 HS 链路。
- 看到串口 `internal PLL cannot satisfy pclk=…` 报错，直接切 SYSCLK；看到 `no table entry for clk=… defaulting to 800 Mbps`，是 SYSCLK 兜底没在查表挡位里——一般无害，但若 EMI 敏感可让屏厂调 `fps` / 改 2 lane。

---

## 16. 常见问题速查

| 现象 | 排查首项 | 排查次项 |
|---|---|---|
| 整屏黑、电流正常 | RESET 时序 / init 命令是否真的下发 | `n_lanes` 与 panel 是否匹配；`bk_display_bus_enable` 是否成功 |
| 整屏黑、电流异常 | VCC_LCD / 屏背光 | AUXLDO 2.8V/3V vote 是否打开 |
| `read_id` 返回全 0 / 0xFF | VCC_LCD 是否上电 | `read_id_regs` 与 `read_id_bytes` 是否填错 |
| 闪屏 / 帧率不稳 | DSI lane bit-rate 偏离屏 spec 推荐范围 | 调 `fps` 或 `n_lanes` 重新算 |
| 启动报 `no PLL: pclk:... min_lane_mbps:...` | `panel_dev_config.clk_src=DPHY_DPLL` 但 panel 超出 PLL 求解范围 | 改 `DPU_CLK_SRC_SYSCLK`，详见 §15.1 |
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
