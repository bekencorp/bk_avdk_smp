# 如何新增 LCD Panel 驱动

本文档详细说明如何在系统中新增 MIPI DSI 或 RGB LCD Panel 驱动。

## 目录

1. [概述](#概述)
2. [新增步骤](#新增步骤)
3. [详细操作指南](#详细操作指南)
4. [示例：新增 MIPI DSI Panel](#示例新增-mipi-dsi-panel)
5. [示例：新增 RGB Panel](#示例新增-rgb-panel)
6. [RGB Panel 延迟命令规则](#rgb-panel-延迟命令规则)
7. [验证和测试](#验证和测试)
8. [常见问题](#常见问题)

---

## 概述

当前 LCD Panel 驱动架构采用**结构体配置方式**，新增 Panel 非常简单：

1. **创建 Panel 配置文件**：`ap/components/bk_peripheral/src/lcd/dsi/lcd_mipi_xxx.c` - 定义 `bk_display_dsi_panel_t` 结构体常量
2. **添加到构建系统**：`ap/components/bk_peripheral/src/lcd/dsi/config.cmake` - 添加到构建列表
3. **项目配置**：`projects/multimedia/doorbell/ap/config/bk7259_ap/config` - 启用配置项
4. **段注册（推荐）**：在配置文件末尾添加 `BK_LCD_PANEL_DEVICE_SECTION(...)`，用于自动枚举/CLI 按 name 动态选择

**特点**：
- ✅ 无需实现 `bk_lcd_new_panel_xxx` 函数（已由通用驱动处理）
- ✅ 无需维护 ID 枚举/映射表（通过链接段自动收集）
- ✅ 配置简单（只需定义结构体常量）
- ✅ 支持自定义 reset 函数（可选）
- ✅ 可通过结构体指针直接传入打开，或通过 `bk_lcd_get_*_panel_list()` 枚举后按 name 选择

---

## 新增步骤

### 步骤 1：创建 Panel 驱动配置文件

**文件**：`ap/components/bk_peripheral/src/lcd/dsi/lcd_mipi_xxx_800x480.c`

创建新的 panel 配置文件，参考 `lcd_mipi_hx8399c_1080x1920.c`。

#### 1.1 文件头部和包含文件

```c
// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <components/bk_display_types.h>
#include <components/bk_lcd_panel_dev.h>
#include <components/bk_lcd_types.h>
#include <driver/mipi_dsi_types.h>
#include <common/avdk_pixel_types.h>

#if CONFIG_LCD_XXX_MIPI_800x480
```

#### 1.2 定义初始化命令序列

```c
// XXX 800x480 Panel 初始化命令序列（从 panel datasheet 获取）
static const lcd_mipi_init_cmd_t xxx_mipi_800x480_init_cmds[] = {
    {0xXX, (const uint8_t []){0xXX, 0xXX, ...}, N},  // 命令格式：[命令码, 参数数组, 参数长度]
    // ... 更多初始化命令 ...
    {0x00, NULL, 0}  // 结束标记
};

// ID 寄存器地址数组（用于自动检测）
static const uint8_t xxx_mipi_800x480_read_id_regs[] = {0xDA, 0xDB, 0xDC, 0};
```

**注意事项**：
- 初始化命令序列必须以 `{0x00, NULL, 0}` 结尾作为结束标记
- ID 寄存器数组必须以 `0` 结尾

#### 1.3 定义 Panel 设备结构体

```c
// Panel 设备描述符 - 被 board config 和 CLI 引用
const bk_display_dsi_panel_t lcd_device_xxx_mipi_800x480 = {
    .id = 0xXXXX,                    // Panel 芯片 ID（从 datasheet 获取）
    .name = "xxx_800x480",           // Panel 名称
    .n_lanes = DSI_ACTIVE_LANES_2,  // MIPI lane 数量（1-4）
    .timing = {
        .clk = LCD_XXM,              // 时钟频率（如 LCD_20M, LCD_80M）
        .h_size = PIXEL_800,          // 水平像素数
        .v_size = PIXEL_480,          // 垂直像素数
        .hsync_pulse_width = XX,     // 水平同步脉冲宽度
        .vsync_pulse_width = XX,     // 垂直同步脉冲宽度
        .hsync_back_porch = XX,      // 水平后沿
        .hsync_front_porch = XX,     // 水平前沿
        .vsync_back_porch = XX,      // 垂直后沿
        .vsync_front_porch = XX,     // 垂直前沿
    },
    .init_cmds = xxx_mipi_800x480_init_cmds,  // 初始化命令序列
    .read_id_regs = xxx_mipi_800x480_read_id_regs,  // ID 寄存器地址数组
    .read_id_bytes = 3,  // 读取的 ID 字节数（1-3，0 表示自动检测）
    .custom_reset = NULL,  // 自定义 reset 函数（NULL 表示使用通用 reset）
};

// 段注册（type=1 表示 DSI Panel）
BK_LCD_PANEL_DEVICE_SECTION(lcd_device_xxx_mipi_800x480, "xxx_mipi_800x480", 1);

#endif  // CONFIG_LCD_XXX_MIPI_800x480
```

**关键参数说明**：
- **`.id`**：Panel 芯片 ID，通过读取 panel ID 寄存器获得（用于自动检测）
- **`.n_lanes`**：MIPI DSI 数据通道数，常见值：2 或 4
- **`.timing`**：显示时序参数，从 panel datasheet 获取
- **`.init_cmds`**：初始化命令序列数组
- **`.read_id_regs`**：ID 寄存器地址数组（用于自动检测）
- **`.read_id_bytes`**：读取的 ID 字节数（1-3，0 表示自动检测）
- **`.custom_reset`**：自定义 reset 函数指针（NULL 表示使用通用 reset 逻辑）

**完整文件示例**：参考 `ap/components/bk_peripheral/src/lcd/dsi/lcd_mipi_hx8399c_1080x1920.c`

---

### 步骤 2：添加到构建系统

**文件**：`ap/components/bk_peripheral/src/lcd/dsi/config.cmake`

添加构建规则：

```cmake
if (CONFIG_LCD_XXX_MIPI_800x480)
    list(APPEND DSI_LCD_DEVICE_FILES ${DSI_LCD_PATH}/lcd_mipi_xxx_800x480.c)
endif()
```

---

### 步骤 5：在项目配置中启用

**文件**：`projects/multimedia/doorbell/ap/config/bk7259_ap/config`

添加配置项：

```bash
CONFIG_LCD_XXX_MIPI_800x480=y
```

**或者通过 menuconfig 配置**：
```bash
cd build/bk7259/doorbell/bk7259_ap
make menuconfig
# 在 LCD Panel 配置菜单中启用新 panel
```

---

### 步骤 4：在代码中使用

**注意**：Panel 使用方式分为“直接引用结构体指针”和“段注册枚举按 name 选择”两种。

#### 4.1 通过 Board Config 使用（推荐，无需注册）

在 `ap_main.c` 中配置：

```c
#include <components/bk_lcd_panel_dev.h>

// 声明 panel 设备描述符
extern const bk_display_dsi_panel_t lcd_device_xxx_mipi_800x480;

const display_board_config_t display_config = {
    .mipi = {
        .enable = true,
        .pin_reset = GPIO_XX,
        .pin_backlight = GPIO_XX,
        .panel = &lcd_device_xxx_mipi_800x480,  // 直接指定 panel 结构体指针
    },
};
```

#### 4.2 通过结构体指针直接使用（无需注册）

**注意**：即使**没有注册**（未完成步骤 2 和步骤 3），也可以直接通过结构体指针打开：

```c
#include <components/bk_lcd_panel_dev.h>

// 声明 panel 设备描述符
extern const bk_display_dsi_panel_t lcd_device_xxx_mipi_800x480;

// 直接传入结构体指针打开
app_mipi_lcd_turn_on(&lcd_device_xxx_mipi_800x480);
```

这种方式**不需要注册**，适合在代码中直接指定 panel 的场景。

#### 4.3 通过段注册列表按 name 动态选择（推荐，适合 CLI）

通过 `bk_lcd_get_mipi_panel_list()` / `bk_lcd_get_rgb_panel_list()` 获取已注册的 panel 列表并按 `panel->name` 匹配：

```c
const bk_display_dsi_panel_t *panel_list[16];
uint32_t n = bk_lcd_get_mipi_panel_list(panel_list, 16);
const bk_display_dsi_panel_t *panel = NULL;
for (uint32_t i = 0; i < n; i++) {
    if (panel_list[i] && panel_list[i]->name &&
        os_strcmp(panel_list[i]->name, "xxx_mipi_800x480") == 0) {
        panel = panel_list[i];
        break;
    }
}
if (panel) {
    app_mipi_lcd_turn_on(panel);
}
```

---

## 示例：新增 MIPI DSI Panel

假设要新增一个 ILI9881 芯片的 800x480 MIPI DSI Panel。

### 1. 创建配置文件

**文件**：`ap/components/bk_peripheral/src/lcd/dsi/lcd_mipi_ili9881_800x480.c`

```c
// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <components/bk_display_types.h>
#include <components/bk_lcd_panel_dev.h>
#include <components/bk_lcd_types.h>
#include <driver/mipi_dsi_types.h>
#include <common/avdk_pixel_types.h>

#if CONFIG_LCD_ILI9881_MIPI_800x480

// ILI9881 800x480 Panel 初始化命令序列
static const lcd_mipi_init_cmd_t ili9881_mipi_800x480_init_cmds[] = {
    {0xFF, (const uint8_t []){0x98,0x81,0x03}, 3},
    {0x01, (const uint8_t []){0x00}, 1},
    // ... 更多初始化命令（根据 datasheet）...
    {0x11, (const uint8_t []){0x00}, 0},  // Sleep out
    {0x29, (const uint8_t []){0x00}, 0},  // Display on
    {0x00, NULL, 0}  // 结束标记
};

// ID 寄存器地址数组
static const uint8_t ili9881_mipi_800x480_read_id_regs[] = {0xDA, 0xDB, 0xDC, 0};

// Panel 设备描述符
const bk_display_dsi_panel_t lcd_device_ili9881_mipi_800x480 = {
    .id = 0x9881,                    // ILI9881 芯片 ID
    .name = "ili9881_800x480",
    .n_lanes = DSI_ACTIVE_LANES_4,   // 4 lane MIPI
    .timing = {
        .clk = LCD_80M,
        .h_size = PIXEL_800,
        .v_size = PIXEL_480,
        .hsync_pulse_width = 20,
        .vsync_pulse_width = 3,
        .hsync_back_porch = 20,
        .hsync_front_porch = 20,
        .vsync_back_porch = 5,
        .vsync_front_porch = 7,
    },
    .init_cmds = ili9881_mipi_800x480_init_cmds,
    .read_id_regs = ili9881_mipi_800x480_read_id_regs,
    .read_id_bytes = 3,  // 读取 3 个字节
    .custom_reset = NULL,  // 使用通用 reset
};

BK_LCD_PANEL_DEVICE_SECTION(lcd_device_ili9881_mipi_800x480, "ili9881_mipi_800x480", 1);

#endif  // CONFIG_LCD_ILI9881_MIPI_800x480
```

### 2. 添加到构建系统

**文件**：`ap/components/bk_peripheral/src/lcd/dsi/config.cmake`

```cmake
if (CONFIG_LCD_ILI9881_MIPI_800x480)
    list(APPEND DSI_LCD_DEVICE_FILES ${DSI_LCD_PATH}/lcd_mipi_ili9881_800x480.c)
endif()
```

### 3. 在配置中启用

**文件**：`projects/multimedia/doorbell/ap/config/bk7259_ap/config`

```bash
CONFIG_LCD_ILI9881_MIPI_800x480=y
```

### 4. 在代码中使用

**文件**：`ap_main.c`

```c
#include <components/bk_lcd_panel_dev.h>

// 声明 panel 设备描述符
extern const bk_display_dsi_panel_t lcd_device_ili9881_mipi_800x480;

const display_board_config_t display_config = {
    .mipi = {
        .enable = true,
        .pin_reset = GPIO_60,
        .pin_backlight = GPIO_7,
        .panel = &lcd_device_ili9881_mipi_800x480,
    },
};
```

---

## 示例：新增 RGB Panel

假设要新增一个 ST7701SN 芯片的 480x854 RGB Panel。

### 1. 创建配置文件

**文件**：`ap/components/bk_peripheral/src/lcd/rgb/lcd_rgb_st7701sn_480x854.c`

```c
// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <components/bk_display_types.h>
#include <components/bk_lcd_types.h>

#if CONFIG_LCD_ST7701SN_RGB_480x854

// ST7701SN 480x854 RGB Panel 初始化命令序列
static const lcd_rgb_spi_init_cmd_t st7701sn_rgb_480x854_init_cmds[] = {
    {0, (const uint8_t []){10}, 0xFF},  // 纯延迟 10ms
    {0xFF, (const uint8_t []){0x77,0x01,0x00,0x00,0x13}, 5},
    {0xEF, (const uint8_t []){0x08}, 1},
    // ... 更多初始化命令 ...
    {0x3A, (const uint8_t []){0x55}, 1},  // Pixel format: RGB565
    {0x36, (const uint8_t []){0x10}, 1},
    {0x11, NULL, 0},  // Sleep out（无数据）
    {0, (const uint8_t []){120}, 0xFF},  // 延迟 120ms（等待 Sleep out 完成）
    {0x29, NULL, 0},  // Display on（无数据）
    {0x00, NULL, 0}  // 结束标记
};

// ID 寄存器地址数组
static const uint8_t st7701sn_rgb_480x854_read_id_regs[] = {0xA1, 0};

// Panel 设备描述符
const bk_display_rgb_panel_t st7701sn_rgb_panel = {
    .id = 0x9903,                    // ST7701SN 芯片 ID
    .name = "st7701sn_480x854",
    .timing = {
        .clk = LCD_30M,
        .h_size = 480,
        .v_size = 854,
        .hsync_pulse_width = 2,
        .vsync_pulse_width = 2,
        .hsync_back_porch = 46,
        .hsync_front_porch = 48,
        .vsync_back_porch = 24,
        .vsync_front_porch = 24,
    },
    .init_cmds = st7701sn_rgb_480x854_init_cmds,
    .spi_cmd_16bit = 0,  // 使用 8-bit 命令格式
    .read_id_regs = st7701sn_rgb_480x854_read_id_regs,
    .read_id_bytes = 2,  // 读取 2 个字节
    .custom_reset = NULL,  // 使用通用 reset
};

// 段注册（type=0 表示 RGB Panel）
BK_LCD_PANEL_DEVICE_SECTION(st7701sn_rgb_panel, "st7701sn_480x854", 0);

#endif  // CONFIG_LCD_ST7701SN_RGB_480x854
```

### 2. 添加到构建系统

**文件**：`ap/components/bk_peripheral/src/lcd/rgb/config.cmake`（如果存在）或相应的 CMakeLists.txt

```cmake
if (CONFIG_LCD_ST7701SN_RGB_480x854)
    list(APPEND RGB_LCD_DEVICE_FILES ${RGB_LCD_PATH}/lcd_rgb_st7701sn_480x854.c)
endif()
```

### 3. 在配置中启用

**文件**：`projects/multimedia/rgb_lcd_example/ap/config/bk7259_ap/config`

```bash
CONFIG_LCD_ST7701SN_RGB_480x854=y
```

### 4. 在代码中使用

**文件**：`lcd_example_rgb.c`

```c
#include <components/bk_lcd_panel_dev.h>
#include <components/bk_lcd_types.h>

// 方式 1：直接引用结构体指针
extern const bk_display_rgb_panel_t st7701sn_rgb_panel;
bk_lcd_rgb_panel_create(bus_handle, &panel_dev_config, &st7701sn_rgb_panel, &panel_handle);

// 方式 2：通过段注册列表按 name 选择
const bk_display_rgb_panel_t *panel_list[16];
uint32_t n = bk_lcd_get_rgb_panel_list(panel_list, 16);
const bk_display_rgb_panel_t *panel = NULL;
for (uint32_t i = 0; i < n; i++) {
    if (panel_list[i] && panel_list[i]->name &&
        os_strcmp(panel_list[i]->name, "st7701sn_480x854") == 0) {
        panel = panel_list[i];
        break;
    }
}
if (panel) {
    bk_lcd_rgb_panel_create(bus_handle, &panel_dev_config, panel, &panel_handle);
}
```

---

## RGB Panel 延迟命令规则

RGB Panel 的初始化命令序列支持**延迟命令**，用于在命令之间插入等待时间。延迟命令采用简化的格式，仅支持纯延迟。

### 延迟命令格式

```c
{0x0, (const uint8_t []){delay_ms}, 0xFF}
```

**参数说明**：
- `cmd`：**必须为 `0x0`**，表示这是一个纯延迟命令，不发送任何命令
- `data`：延迟时间数组，`data[0]` 表示延迟的毫秒数（8-bit 格式）
- `data_len`：**必须为 `0xFF`**，表示这是一个延迟命令

### 使用示例

```c
static const lcd_rgb_spi_init_cmd_t init_cmds[] = {
    // 示例 1：纯延迟 10ms（不发送任何命令）
    {0, (const uint8_t []){10}, 0xFF},
    
    // 示例 2：发送普通命令
    {0xFF, (const uint8_t []){0x77,0x01,0x00,0x00,0x13}, 5},
    
    // 示例 3：纯延迟 120ms
    {0, (const uint8_t []){120}, 0xFF},
    
    // 示例 4：普通命令（非延迟命令）
    {0x3A, (const uint8_t []){0x55}, 1},  // 发送 0x3A 命令，数据为 0x55
    
    // 示例 5：无数据命令
    {0x29, NULL, 0},  // 发送 0x29 命令，无数据
    
    // 示例 6：纯延迟 60ms
    {0, (const uint8_t []){60}, 0xFF},
    
    // 结束标记
    {0x00, NULL, 0}
};
```

### 执行逻辑

通用驱动 `lcd_rgb_panel_common_init()` 的处理逻辑：

1. **检查是否为延迟命令**：`cmd == 0 && data_len == 0xFF && data != NULL`
2. **提取延迟时间**：从 `data[0]` 读取延迟毫秒数（8-bit 格式）
3. **执行延迟**：调用 `rtos_delay_milliseconds(delay_ms)`，然后 `continue` 跳过后续命令发送逻辑

**注意**：延迟命令只执行延迟，不会发送任何命令。如果需要"发送命令后延迟"，请将命令和延迟分开写：

```c
// 正确方式：先发送命令，再单独延迟
{0x11, NULL, 0},           // 发送 0x11 命令
{0, (const uint8_t []){10}, 0xFF},  // 延迟 10ms

// 错误方式：不支持 cmd != 0 的延迟命令
// {0x11, (const uint8_t []){10}, 0xFF},  // 这种方式不再支持
```

### 注意事项

- ✅ **延迟命令格式固定为：`{0x0, (const uint8_t []){delay_ms}, 0xFF}`**
- ✅ **`cmd` 必须为 `0x0`**，表示纯延迟，不发送任何命令
- ✅ **延迟时间从 `data[0]` 读取**（8-bit 格式，统一使用 `uint8_t`）
- ✅ **`data` 不能为 `NULL`**，必须提供延迟时间数组
- ✅ **`data_len` 必须为 `0xFF`**，用于标识延迟命令
- ❌ **不支持 `cmd != 0` 的延迟命令**（不再支持"发送命令后延迟"的格式）
- ❌ **不要将普通命令的 `data_len` 设置为 `0xFF`**（会被误判为延迟命令）

---

## 验证和测试

### 1. 编译验证

```bash
cd build/bk7259/doorbell/bk7259_ap
ninja
```

确保编译无错误。

### 2. 功能测试

#### 通过 Board Config 测试

在 `ap_main.c` 中配置 panel 后，启动系统，panel 应该能正常显示。

#### 通过 CLI/动态选择测试（推荐）

**示例代码**：

```c
#include <components/bk_lcd_panel_dev.h>
#include <components/bk_lcd_types.h>

// 枚举所有已注册的 MIPI Panel
const bk_display_dsi_panel_t *panel_list[16];
uint32_t panel_count = bk_lcd_get_mipi_panel_list(panel_list, 16);

LOGI("Available MIPI panels (%d):\n", panel_count);
for (uint32_t i = 0; i < panel_count; i++) {
    if (panel_list[i] && panel_list[i]->name) {
        LOGI("  [%d] %s\n", i, panel_list[i]->name);
    }
}

// 通过 name 选择 panel
const char *target_name = "hx8399c_mipi_1080x1920";
const bk_display_dsi_panel_t *selected_panel = NULL;
for (uint32_t i = 0; i < panel_count; i++) {
    if (panel_list[i] && panel_list[i]->name &&
        os_strcmp(panel_list[i]->name, target_name) == 0) {
        selected_panel = panel_list[i];
        LOGI("Selected panel: %s\n", target_name);
        break;
    }
}

if (selected_panel) {
    // 使用选中的 panel
    app_mipi_lcd_turn_on(selected_panel);
} else {
    LOGE("Panel not found: %s\n", target_name);
}
```

**RGB Panel 的枚举方式类似**：

```c
const bk_display_rgb_panel_t *rgb_panel_list[16];
uint32_t rgb_count = bk_lcd_get_rgb_panel_list(rgb_panel_list, 16);
// ... 类似的枚举和选择逻辑 ...
```

---

## 常见问题

### Q1: Panel 无法显示或显示异常

**可能原因**：
1. 初始化命令序列不正确
2. 时序参数不匹配
3. MIPI lane 数量配置错误
4. 时钟频率设置不当

**解决方法**：
- 检查 panel datasheet，确认初始化命令序列
- 验证时序参数（porch, pulse width 等）
- 确认 MIPI lane 数量和时钟频率

### Q2: 编译错误：未定义的引用

**可能原因**：
- 忘记在 `config.cmake` 中添加源文件
- 忘记在配置文件中启用 `CONFIG_LCD_XXX_MIPI_XXX`

**解决方法**：
- 检查 `config.cmake` 中是否添加了源文件
- 检查配置文件中的 `CONFIG_LCD_XXX_MIPI_XXX=y`

### Q3: 如何获取 Panel 初始化命令序列？

**方法**：
1. 查看 panel datasheet 的初始化章节
2. 参考厂商提供的参考代码
3. 使用示波器或逻辑分析仪抓取正常工作的初始化序列

### Q4: 如何确定时序参数？

**方法**：
1. 查看 panel datasheet 的时序规格章节
2. 参考现有类似分辨率的 panel 配置
3. 根据公式计算：
   - `总时钟 = (h_size + hsync_pulse_width + hsync_back_porch + hsync_front_porch) * (v_size + vsync_pulse_width + vsync_back_porch + vsync_front_porch) * fps`

### Q5: 如何实现自定义 Reset 函数？

如果 panel 的 reset 逻辑不符合通用 reset 函数，可以实现自定义 reset：

```c
// 自定义 reset 函数
static bk_err_t custom_reset_func(bk_avdk_lcd_panel_t *panel, void *priv)
{
    // 自定义 reset 逻辑
    // ...
    return BK_OK;
}

// 在 panel 结构体中指定
const bk_display_dsi_panel_t lcd_device_xxx_mipi_800x480 = {
    // ... 其他字段 ...
    .custom_reset = custom_reset_func,  // 指定自定义 reset 函数
};
```

详细示例请参考：`docs/lcd_panel_custom_reset_example.md`

### Q6: RGB Panel 的延迟命令不生效？

**可能原因**：
1. `cmd` 未设置为 `0x0`（延迟命令必须 `cmd == 0`）
2. `data_len` 未设置为 `0xFF`
3. `data` 为 `NULL`（延迟时间无法读取）
4. 延迟时间 `data[0]` 为 0

**解决方法**：
- 确保延迟命令格式为：`{0x0, (const uint8_t []){delay_ms}, 0xFF}`
- 检查 `cmd` 是否为 `0x0`（纯延迟，不发送命令）
- 检查 `delay_ms` 是否大于 0
- 如果需要"发送命令后延迟"，请将命令和延迟分开写：
  ```c
  {0x11, NULL, 0},           // 先发送命令
  {0, (const uint8_t []){10}, 0xFF},  // 再延迟
  ```
- 参考 `lcd_rgb_st7701sn_480x854.c` 中的延迟命令示例

### Q7: 段注册后无法通过 `bk_lcd_get_*_panel_list()` 枚举到？

**可能原因**：
1. 忘记添加 `BK_LCD_PANEL_DEVICE_SECTION(...)` 宏
2. 链接脚本未定义 `.lcd_panel_device_list` 段
3. Panel 配置文件未被编译进工程

**解决方法**：
- 检查配置文件末尾是否添加了 `BK_LCD_PANEL_DEVICE_SECTION(panel_symbol, "panel_name", type)`
- 检查链接脚本（`bk7259_ap_bsp.ld` / `bk7258_ap_bsp.ld`）是否包含 `.lcd_panel_device_list` 段定义
- 确认 `config.cmake` 中已添加源文件，且配置项已启用

---

## 文件清单

新增 panel 需要修改的文件：

| 文件路径 | 修改内容 |
|---------|---------|
| `ap/components/bk_peripheral/src/lcd/dsi/lcd_mipi_xxx.c` | 新建：定义 panel 配置结构体 |
| `ap/components/bk_peripheral/src/lcd/dsi/lcd_mipi_xxx.c` | 添加：`BK_LCD_PANEL_DEVICE_SECTION(...)` 段注册 |
| `ap/components/bk_peripheral/src/lcd/dsi/config.cmake` | 添加到构建系统 |
| `projects/.../config/bk7259_ap/config` | 启用配置项 |
| `ap_main.c`（可选） | 声明并使用 panel 结构体 |

---

## 快速检查清单

新增 panel 后，请确认：

- [ ] Panel 配置文件已创建，包含完整的 `bk_display_dsi_panel_t` 结构体
- [ ] 初始化命令序列正确，以 `{0x00, NULL, 0}` 结尾
- [ ] 已添加 `BK_LCD_PANEL_DEVICE_SECTION(...)`（确保可被枚举到）
- [ ] 构建配置中已添加源文件
- [ ] 项目配置中已启用 `CONFIG_LCD_XXX_MIPI_XXX`
- [ ] `ap_main.c` 中已声明 panel 结构体（如需要）
- [ ] 编译通过，无错误
- [ ] 功能测试通过，panel 能正常显示
- [ ] 通过 CLI/按 name 选择测试通过（可选）

---

## 参考

### MIPI DSI Panel
- **现有实现参考**：`ap/components/bk_peripheral/src/lcd/dsi/lcd_mipi_hx8399c_1080x1920.c`
- **通用驱动**：`ap/components/bk_display/src/bus/lcd_mipi_panel_common.c`
- **创建接口**：`ap/components/bk_display/src/panel/bk_lcd_panel.c` - `bk_lcd_mipi_panel_create()`

### RGB Panel
- **现有实现参考**：`ap/components/bk_peripheral/src/lcd/rgb/lcd_rgb_st7701sn_480x854.c`
- **通用驱动**：`ap/components/bk_display/src/bus/lcd_rgb_panel_common.c`
- **创建接口**：`ap/components/bk_display/src/panel/bk_lcd_panel.c` - `bk_lcd_rgb_panel_create()`

### 数据结构定义
- **结构体定义**：`ap/include/components/bk_display_types.h` - `bk_display_dsi_panel_t` / `bk_display_rgb_panel_t`
- **段注册宏**：`ap/include/components/bk_lcd_types.h` - `BK_LCD_PANEL_DEVICE_SECTION()`
- **枚举接口**：`ap/components/bk_peripheral/src/lcd/lcd_panel_devices.c` - `bk_lcd_get_mipi_panel_list()` / `bk_lcd_get_rgb_panel_list()`

---

## 补充说明

### 关于段注册与 `lcd_panel_devices.h`

#### 段注册机制

**段注册是推荐路径**：Panel 配置文件末尾添加 `BK_LCD_PANEL_DEVICE_SECTION(...)` 后，就可以被 `bk_lcd_get_*_panel_list()` 自动枚举到。

**段注册宏格式**：
```c
BK_LCD_PANEL_DEVICE_SECTION(panel_symbol, "panel_name", type)
```

**参数说明**：
- `panel_symbol`：Panel 配置结构体的符号名（如 `lcd_device_hx8399c_mipi_1080x1920`）
- `"panel_name"`：Panel 名称字符串（用于运行时按 name 匹配）
- `type`：Panel 类型，`0` = RGB Panel，`1` = MIPI DSI Panel

**工作原理**：
1. 宏将 panel 信息放入链接器的 `.lcd_panel_device_list` 段
2. 链接脚本定义段的起始和结束符号（`__lcd_panel_device_array_start` / `__lcd_panel_device_array_end`）
3. 运行时通过 `bk_lcd_get_*_panel_list()` 遍历段中的所有 panel

#### `lcd_panel_devices.h` 的 extern 声明

**是否需要**：如果工程里仍有代码直接 `extern const bk_display_*_panel_t xxx;` 并引用这些符号（例如固定 board config / 旧工程兼容），可以保留；否则可逐步减少直接引用，转为“枚举 + name 匹配”。

**建议**：
- **新代码**：优先使用 `bk_lcd_get_*_panel_list()` + name 匹配的方式
- **旧代码兼容**：保留 `lcd_panel_devices.h` 中的 extern 声明，确保编译通过
- **逐步迁移**：将直接引用改为动态枚举，提高代码灵活性
