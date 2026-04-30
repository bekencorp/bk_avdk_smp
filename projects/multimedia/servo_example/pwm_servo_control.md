# BK7259 PWM 舵机控制使用说明

## 1. 概述

基于 BK7259 AP 侧 PWM V2P2 驱动，通过 CLI 命令控制标准 180° 舵机（如 SG90、MG996R 等）。

### 舵机控制原理

标准舵机使用 50Hz（周期 20ms）的 PWM 信号控制角度，通过调节高电平脉宽实现不同转角：

```
         ┌──┐                  ┌──┐
         │  │                  │  │
    ─────┘  └──────────────────┘  └──────
         |__|
         脉宽                   周期=20ms
```

| 高电平脉宽 | 对应角度 | 占空比 |
|-----------|---------|--------|
| 0.5ms     | 0°      | 2.5%   |
| 1.0ms     | 45°     | 5.0%   |
| 1.5ms     | 90°     | 7.5%   |
| 2.0ms     | 135°    | 10.0%  |
| 2.5ms     | 180°    | 12.5%  |

## 2. 编译配置

在 `projects/app/ap/config/bk7259_ap/defconfig` 中确保以下宏已开启：

```
CONFIG_PWM=y
CONFIG_PWM_V2P2=y
```

测试命令依赖以下配置（板级默认已开启）：

```
CONFIG_CLI=y
CONFIG_DRIVER_TEST=y
```

## 3. 硬件接线

### 舵机线序

| 舵机线色 | 功能   | 接线说明        |
|---------|--------|----------------|
| 红色    | VCC    | 接 5V 电源      |
| 棕色    | GND    | 接地            |
| 橙/黄色 | 信号线  | 接 PWM GPIO 引脚 |

> **注意**：舵机供电建议使用独立 5V 电源，避免大电流拖垮 MCU 供电。

### PWM 通道与 GPIO 引脚映射

| PWM 通道 | GPIO 引脚 | 舵机信号线连接 |
|----------|----------|---------------|
| 0        | GPIO_47  | 舵机信号线 → GPIO_47 引脚 |
| 1        | GPIO_48  | 舵机信号线 → GPIO_48 引脚 |
| 2        | GPIO_49  | 舵机信号线 → GPIO_49 引脚 |
| 3        | GPIO_50  | 舵机信号线 → GPIO_50 引脚 |
| 4        | GPIO_51  | 舵机信号线 → GPIO_51 引脚 |
| 5        | GPIO_52  | 舵机信号线 → GPIO_52 引脚 |
| 6        | GPIO_53  | 舵机信号线 → GPIO_53 引脚 |
| 7        | GPIO_54  | 舵机信号线 → GPIO_54 引脚 |
| 8        | GPIO_55  | 舵机信号线 → GPIO_55 引脚 |
| 9        | GPIO_8   | 舵机信号线 → GPIO_8 引脚  |
| 10       | GPIO_28  | 舵机信号线 → GPIO_28 引脚 |
| 11       | GPIO_9   | 舵机信号线 → GPIO_9 引脚  |

引脚映射定义位于：`ap/middleware/soc/bk7259_ap/soc/gpio_map.h` 中的 `GPIO_PWM_MAP_TABLE`。

### 单舵机接线示意

```
BK7259 开发板                  舵机（SG90 为例）
┌──────────┐                  ┌──────────┐
│          │                  │          │
│  GPIO_47 ├──────────────────┤ 信号线（橙/黄）
│          │                  │          │
│      GND ├──────┬───────────┤ GND（棕）│
│          │      │           │          │
└──────────┘      │           │          │
                  │     ┌─────┤ VCC（红）│
                  │     │     └──────────┘
                  │     │
              ┌───┴─────┴───┐
              │  5V 外部电源  │
              │  GND    VCC  │
              └─────────────┘
```

### 双舵机接线示意

```
BK7259 开发板                  舵机 1               舵机 2
┌──────────┐                  ┌──────────┐         ┌──────────┐
│          │                  │          │         │          │
│  GPIO_47 ├──────────────────┤ 信号线    │         │          │
│          │                  │          │         │          │
│  GPIO_48 ├──────────────────┼──────────┼─────────┤ 信号线    │
│          │                  │          │         │          │
│      GND ├──────┬───────────┤ GND      │    ┌────┤ GND      │
│          │      │           │          │    │    │          │
└──────────┘      │     ┌─────┤ VCC      │    │ ┌──┤ VCC      │
                  │     │     └──────────┘    │ │  └──────────┘
                  │     │                     │ │
              ┌───┴─────┴─────────────────────┴─┴──┐
              │          5V 外部电源（≥2A）          │
              │          GND           VCC          │
              └────────────────────────────────────┘
```

> **重要提示**：
> - 每个舵机的信号线分别接到对应 PWM 通道的 GPIO 引脚
> - 所有舵机的 VCC 和 GND 共接到同一个 5V 外部电源
> - 开发板 GND 必须与外部电源 GND 共地
> - 多舵机场景建议使用 ≥2A 的 5V 电源（每个舵机峰值约 500mA~1A）
> - 信号线无需额外上拉，PWM 驱动初始化时会自动配置 GPIO 上拉

## 4. PWM 参数说明

| 参数 | 值 | 说明 |
|------|-----|------|
| 时钟源 | 320 MHz | PWM_CLOCK_SRC_XTAL |
| 分频系数 (psc) | 249 | 有效时钟 = 320MHz / 250 = 1.28MHz |
| 周期 (period_cycle) | 25600 | 1.28MHz / 50Hz = 25600 |
| PWM 频率 | 50 Hz | 标准舵机频率 |
| 最小脉宽 duty | 640 | 0.5ms → 0° |
| 最大脉宽 duty | 3200 | 2.5ms → 180° |

### 角度到 duty 值的换算公式

```
duty = 640 + angle × (3200 - 640) / 180
     = 640 + angle × 14.22
```

| 角度 | duty 值 | 高电平脉宽 | 占空比 |
|------|---------|-----------|--------|
| 0°   | 640     | 0.50ms    | 2.5%   |
| 10°  | 782     | 0.61ms    | 3.1%   |
| 20°  | 924     | 0.72ms    | 3.6%   |
| 30°  | 1067    | 0.83ms    | 4.2%   |
| 45°  | 1280    | 1.00ms    | 5.0%   |
| 60°  | 1493    | 1.17ms    | 5.8%   |
| 90°  | 1920    | 1.50ms    | 7.5%   |
| 120° | 2347    | 1.83ms    | 9.2%   |
| 135° | 2560    | 2.00ms    | 10.0%  |
| 150° | 2773    | 2.17ms    | 10.8%  |
| 180° | 3200    | 2.50ms    | 12.5%  |

## 5. CLI 命令参考

所有舵机命令以 `servo` 为前缀，通过串口工具发送时需加 `ap_cmd` 前缀。

### 5.1 servo init — 初始化舵机

初始化指定 PWM 通道为舵机模式，默认转到 90° 位置。

```
ap_cmd servo init {chan}
```

**参数：**
- `chan`：PWM 通道号（0~11）

**示例：**
```
ap_cmd servo init 0
```

**输出：**
```
servo init chan=0, angle=90, duty=1920
```

### 5.2 servo set — 设置角度

设置舵机转到指定角度。

```
ap_cmd servo set {chan} {angle}
```

**参数：**
- `chan`：PWM 通道号（0~11）
- `angle`：目标角度（0~180）

**示例：**
```
ap_cmd servo set 0 0       # 转到 0°
ap_cmd servo set 0 90      # 转到 90°
ap_cmd servo set 0 180     # 转到 180°
ap_cmd servo set 0 45      # 转到 45°
```

**输出：**
```
servo set chan=0 angle=90 duty=1920
```

### 5.3 servo sweep — 扫描测试

舵机从 0° 扫描到 180°，再从 180° 返回 0°。

```
ap_cmd servo sweep {chan} [step] [delay_ms]
```

**参数：**
- `chan`：PWM 通道号（0~11）
- `step`：步进角度，默认 10°
- `delay_ms`：每步停留时间，默认 500ms

**示例：**
```
ap_cmd servo sweep 0              # 默认步长 10°，间隔 500ms
ap_cmd servo sweep 0 5 200        # 步长 5°，间隔 200ms（更细腻）
ap_cmd servo sweep 0 30 1000      # 步长 30°，间隔 1s（粗略）
```

### 5.4 servo stop — 停止输出

停止 PWM 信号输出，舵机保持当前位置（不再接收控制信号后可能会松弛）。

```
ap_cmd servo stop {chan}
```

**示例：**
```
ap_cmd servo stop 0
```

### 5.5 servo deinit — 释放通道

停止 PWM 并释放通道资源。

```
ap_cmd servo deinit {chan}
```

**示例：**
```
ap_cmd servo deinit 0
```

## 6. 使用示例

### 6.1 单舵机控制

```
ap_cmd servo init 0          # 初始化通道 0，舵机转到 90°
ap_cmd servo set 0 0         # 转到 0°
ap_cmd servo set 0 90        # 转到 90°
ap_cmd servo set 0 180       # 转到 180°
ap_cmd servo sweep 0 10 300  # 扫描测试
ap_cmd servo deinit 0        # 释放
```

### 6.2 双舵机独立控制

```
ap_cmd servo init 0          # 初始化通道 0（GPIO_47）
ap_cmd servo init 1          # 初始化通道 1（GPIO_48）
ap_cmd servo set 0 45        # 通道 0 转到 45°
ap_cmd servo set 1 135       # 通道 1 转到 135°
ap_cmd servo set 0 0         # 通道 0 转到 0°，通道 1 不受影响
ap_cmd servo deinit 0        # 释放通道 0
ap_cmd servo deinit 1        # 释放通道 1
```

### 6.3 多舵机控制（最多 12 路）

```
ap_cmd servo init 0          # 初始化通道 0（GPIO_47）
ap_cmd servo init 1          # 初始化通道 1（GPIO_48）
ap_cmd servo init 2          # 初始化通道 2（GPIO_49）
ap_cmd servo init 3          # 初始化通道 3（GPIO_50）
ap_cmd servo set 0 0         # 通道 0 → 0°
ap_cmd servo set 1 45        # 通道 1 → 45°
ap_cmd servo set 2 90        # 通道 2 → 90°
ap_cmd servo set 3 135       # 通道 3 → 135°
```

## 7. 示波器验证

使用示波器测量 PWM 输出引脚，验证波形是否正确：

| 检查项 | 预期值 |
|--------|--------|
| 周期   | 20.00ms（50Hz） |
| 0° 高电平脉宽  | 0.50ms（占空比 2.5%） |
| 90° 高电平脉宽 | 1.50ms（占空比 7.5%） |
| 180° 高电平脉宽 | 2.50ms（占空比 12.5%） |

## 8. 自定义 PWM GPIO 引脚

默认情况下，PWM 通道使用 `gpio_map.h` 中 `GPIO_PWM_MAP_TABLE` 定义的 GPIO 引脚。如果默认引脚与你的硬件设计冲突，可以通过以下方式将 PWM 输出重映射到其他 GPIO。

### 8.1 在 servo_example 工程中配置

在 `projects/multimedia/servo_example/ap/ap_main.c` 中，找到以下宏定义区域：

```c
// #define SERVO_USE_CUSTOM_GPIO
#ifdef SERVO_USE_CUSTOM_GPIO
#define SERVO_CUSTOM_GPIO_ID    GPIO_6
#define SERVO_CUSTOM_GPIO_DEV   GPIO_DEV_PWM0
#endif
```

**启用自定义 GPIO 的步骤：**

1. 取消注释 `#define SERVO_USE_CUSTOM_GPIO`
2. 修改 `SERVO_CUSTOM_GPIO_ID` 为目标 GPIO 编号
3. 修改 `SERVO_CUSTOM_GPIO_DEV` 为对应的 PWM 设备功能

**示例：将 PWM 通道 0 从默认 GPIO_47 改为 GPIO_6**

```c
#define SERVO_USE_CUSTOM_GPIO
#ifdef SERVO_USE_CUSTOM_GPIO
#define SERVO_CUSTOM_GPIO_ID    GPIO_6
#define SERVO_CUSTOM_GPIO_DEV   GPIO_DEV_PWM0
#endif
```

### 8.2 工作原理

`bk_pwm_init()` 会先按 `GPIO_PWM_MAP_TABLE` 初始化默认引脚，随后 `servo_remap_gpio()` 执行以下操作将 PWM 信号重定向到自定义引脚：

```c
gpio_dev_unmap(SERVO_CUSTOM_GPIO_ID);                  // 解除目标 GPIO 的旧功能
gpio_dev_map(SERVO_CUSTOM_GPIO_ID, SERVO_CUSTOM_GPIO_DEV); // 映射为 PWM 功能
bk_gpio_pull_up(SERVO_CUSTOM_GPIO_ID);                 // 设置上拉
```

### 8.3 在代码中动态重映射（通用方法）

如果不使用 servo_example 的宏方式，也可以在任意代码中通过 API 动态修改：

```c
#include <driver/gpio.h>
#include <driver/hal/hal_gpio_types.h>
#include "gpio_driver.h"

// 在 bk_pwm_init() 之后、bk_pwm_start() 之前调用
gpio_dev_unmap(GPIO_6);                    // 解除 GPIO_6 原有映射
gpio_dev_map(GPIO_6, GPIO_DEV_PWM0);       // 将 GPIO_6 映射为 PWM0 输出
bk_gpio_pull_up(GPIO_6);                   // 上拉
```

### 8.4 GPIO 与 PWM 功能对应关系

自定义 GPIO 必须在硬件 IOMUX 上支持对应的 PWM 功能。可用的映射关系定义在 `ap/middleware/soc/bk7259_ap/soc/gpio_map.h` 中各 GPIO 的功能列表里。

每个 PWM 通道对应一个 `GPIO_DEV_PWMx`：

| PWM 通道 | GPIO_DEV | 默认 GPIO | 可选 GPIO（需查 gpio_map.h） |
|----------|----------|-----------|---------------------------|
| 0        | GPIO_DEV_PWM0  | GPIO_47 | 查看 gpio_map.h 中含 GPIO_DEV_PWM0 的 GPIO |
| 1        | GPIO_DEV_PWM1  | GPIO_48 | 查看 gpio_map.h 中含 GPIO_DEV_PWM1 的 GPIO |
| 2        | GPIO_DEV_PWM2  | GPIO_49 | 查看 gpio_map.h 中含 GPIO_DEV_PWM2 的 GPIO |
| ...      | ...            | ...     | ... |

> **注意事项**：
> - 自定义 GPIO 必须支持目标 PWM 通道的 IOMUX 功能，否则无法输出正确信号
> - 重映射后，原默认 GPIO 仍处于 PWM 功能状态，建议调用 `gpio_dev_unmap()` 释放原引脚
> - 如需同时使用多路舵机且都要自定义引脚，需对每路分别调用重映射 API

## 9. 相关代码文件

| 文件 | 说明 |
|------|------|
| `projects/multimedia/servo_example/ap/ap_main.c` | 舵机自检 + 自定义 GPIO 配置 |
| `ap/middleware/driver/pwm/pwm_test.c` | 舵机 CLI 命令实现 |
| `ap/middleware/driver/pwm/v2p2/pwm_driver.c` | PWM V2P2 驱动 |
| `ap/middleware/soc/bk7259_ap/soc/gpio_map.h` | GPIO 引脚映射表（默认 + 可选引脚） |
| `ap/middleware/driver/bk7259_ap/gpio_driver.h` | `gpio_dev_map()` / `gpio_dev_unmap()` 声明 |
| `ap/middleware/soc/bk7259_ap/hal/pwm_hal_v2p2.c` | PWM HAL 层 |
| `ap/include/driver/pwm.h` | PWM API 声明 |
| `ap/include/driver/pwm_types.h` | PWM 数据类型定义 |
| `ap/include/driver/gpio.h` | GPIO API 声明（`bk_gpio_pull_up` 等） |
| `projects/multimedia/servo_example/ap/config/bk7259_ap/defconfig` | AP 侧编译配置 |

## 10. 常见问题

**Q: 舵机不转动？**
- 检查供电是否为 5V 独立电源
- 确认信号线接到了正确的 GPIO 引脚
- 确认 `CONFIG_PWM=y` 已配置

**Q: 舵机抖动？**
- 检查供电是否稳定，电流是否充足
- 避免在不需要时持续发送信号，可用 `servo stop` 停止

**Q: 如何更改默认 PWM 引脚？**
- 方法一：在 `servo_example` 工程中定义 `SERVO_USE_CUSTOM_GPIO` 宏（见第 8 节）
- 方法二：在代码中调用 `gpio_dev_unmap()` + `gpio_dev_map()` 动态重映射
- 方法三：修改 `ap/middleware/soc/bk7259_ap/soc/gpio_map.h` 中的 `GPIO_PWM_MAP_TABLE`（影响全局）
- 无论哪种方式，都需确保目标 GPIO 在 IOMUX 中支持对应的 PWM 功能
