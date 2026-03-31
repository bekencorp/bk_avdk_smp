# HSPL (Hardware Spin Lock) 驱动

## 概述

BK7259 有 **两个等价的 HSPL 硬件块**，寄存器定义一致，仅 **基地址不同**：

- **M52 侧映射**：`0x45010000`（驱动中 `BK_HSPL_ID_0`）
- **M55 侧映射**：`0x480C0000`（驱动中 `BK_HSPL_ID_1`）

上层可以把不同的共享资源分配到不同 HSPL 实例/通道上，完成 **4 核之间**的硬件自旋锁。

## 实例分工与中断归属

- **BK_HSPL_ID_0**：用于 CP/AP 跨核互斥，HSPL 超时中断由 **M52** 侧处理
- **BK_HSPL_ID_1**：用于 AP SMP 内部互斥（CPU2/CPU3），HSPL 超时中断由 **AP M55** 侧处理

## 文档

- **[HSPL 使用指南](HSPL_USER_GUIDE.md)** - 详细的 API 使用说明和示例代码
- **[HSPL 测试指南](HSPL_TEST_GUIDE.md)** - 完整的测试方法和用例

## 快速开始

### 1) 底层接口（按实例/通道）

- **Try-lock**：读 `LOCK[ch]`（读操作即尝试加锁）
  - 返回 `1`：表示 **unlock -> lock 成功**
  - 返回 `6'b1xxxx0`：表示已被锁住，`xxxx` 为 owner_id
- **Unlock**：写 `0xA55A80AF` 到 `LOCK[ch]`（释放锁）
- **STA[ch]**：只读辅助寄存器，不改变锁状态，便于调试

对应 API（见 `hspl_driver.h`）：

- `bk_hspl_try_lock(hspl_id, ch, &owner_id)`
- `bk_hspl_unlock(hspl_id, ch)`
- `bk_hspl_get_state(hspl_id, ch, &state)`

### 2) 上层接口（按资源：Device/OS）

为了让上层更好用，增加资源级 API（见 `hspl_res_lock.h/.c`），采用**自动映射规则**：

- **资源 0-15**：自动映射到 `BK_HSPL_ID_0` (M52) 的通道 0-15
- **资源 16-31**：自动映射到 `BK_HSPL_ID_1` (M55) 的通道 0-15

资源枚举示例（完整列表见 `hspl_res_lock.h`）：

```c
typedef enum {
    BK_HSPL_RES_FLASH = 0,
    BK_HSPL_RES_CLOCK,
    BK_HSPL_RES_POWER,
    BK_HSPL_RES_SYS,
    BK_HSPL_RES_RTC,
    BK_HSPL_RES_ANA,
    BK_HSPL_RES_FUSE,
    BK_HSPL_RES_TRNG,
    BK_HSPL_RES_WDT,
    BK_HSPL_RES_SPI,
    BK_HSPL_RES_GPIO,
    BK_HSPL_RES_PWM,
    BK_HSPL_RES_ADC,
    BK_HSPL_RES_DAC,
    BK_HSPL_RES_PMU,
    BK_HSPL_RES_USER0,

    BK_HSPL_RES_OS,
    BK_HSPL_RES_LVGL,
    BK_HSPL_RES_AUDIO,
    BK_HSPL_RES_VIDEO,
    BK_HSPL_RES_GPU,
    BK_HSPL_RES_NPU,
    BK_HSPL_RES_DSP,
    BK_HSPL_RES_ISP,
    BK_HSPL_RES_VDEC,
    BK_HSPL_RES_VENC,
    BK_HSPL_RES_SDIO,
    BK_HSPL_RES_SDIO_HS,
    BK_HSPL_RES_SDIO_HS_HS,
    BK_HSPL_RES_USB,
    BK_HSPL_RES_USER1,
    BK_HSPL_RES_USER2,

    BK_HSPL_RES_MAX,
} bk_hspl_res_t;
```

使用示例：

- `bk_hspl_res_lock(BK_HSPL_RES_FLASH, timeout_us)` - 资源0，映射到 HSPL_0 通道0
- `bk_hspl_res_lock(BK_HSPL_RES_OS, timeout_us)` - 资源16，映射到 HSPL_1 通道0
- `bk_hspl_res_unlock(BK_HSPL_RES_FLASH)`

### 3) 示例

```c
#include "hspl_res_lock.h"

void flash_critical_section(void)
{
	/* 1000us 超时 */
	if (bk_hspl_res_lock(BK_HSPL_RES_FLASH, 1000) != BK_OK) {
		return;
	}

	/* ... flash 操作 ... */

	bk_hspl_res_unlock(BK_HSPL_RES_FLASH);
}
```

### 4) CLI 测试

使能 `CONFIG_HSPL_TEST` 后，可以用 CLI 验证：

- **按实例/通道**：
  - `hspl lock {hspl_id} {ch}`（例如：`hspl lock 0 0` 表示 M52 实例 ch0）
  - `hspl unlock {hspl_id} {ch}`
  - `hspl state {hspl_id} {ch|all}`（例如：`hspl state 1 all` 表示 M55 实例所有通道状态）
- **按资源**：
  - `hspl res_lock flash 1000` - 资源0，映射到 HSPL_0 通道0
  - `hspl res_lock os 1000` - 资源16，映射到 HSPL_1 通道0
  - `hspl res_unlock flash`

### 4.1) AP 初始化说明

- AP 侧默认假设 `bk_hspl_driver_init()` 在 CPU2 启动 CPU3 之前完成，因此 `hspl_lazy_init()` 未加自旋锁保护。
- 若平台允许 CPU3 在未完成初始化前调用 HSPL API，需保证上层先显式调用 `bk_hspl_driver_init()`。

### 5) 并行压力测试（CPU0 vs CPU2）

支持在 **CP M52 CPU0** 和 **AP M55 CPU2** 上并行运行压力测试，验证 HSPL 在多核竞争场景下的正确性。

#### 自动化压力测试（推荐）

```bash
# 在 CPU0 或 CPU2 上执行（推荐在 CPU0 上执行）
hspl stress_auto 0 0 10000 2
```

**参数说明**：
- `hspl_id`: HSPL 实例 ID（0 或 1）
- `ch`: 通道号（0-15）
- `iter`: 每个核心的迭代次数
- `hold_ms`: 每次持有锁的时间（毫秒）

**功能**：
- 自动在 CPU0 和 CPU2 上启动并行压力测试
- 两个 CPU 同时竞争同一个 HSPL 通道
- 自动统计成功/失败次数

#### 查看测试统计

```bash
# 在对应的 CPU 上执行
hspl stress_stat
```

#### 停止测试

```bash
# 在对应的 CPU 上执行
hspl stress_stop
```

**详细说明请参考 [HSPL 测试指南](HSPL_TEST_GUIDE.md)**
