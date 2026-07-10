# HSPL (Hardware Spin Lock) 使用指南

## 目录
1. [概述](#概述)
2. [硬件架构](#硬件架构)
3. [驱动初始化](#驱动初始化)
4. [底层 API 使用](#底层-api-使用)
5. [资源级 API 使用](#资源级-api-使用)
6. [配置说明](#配置说明)
7. [注意事项](#注意事项)

---

## 概述

HSPL (Hardware Spin Lock) 是 BK7259 芯片提供的硬件自旋锁机制，用于多核（4核）之间的同步。BK7259 提供了两个等价的 HSPL 硬件块，支持 16 个独立通道，可以实现高效的跨核互斥访问。

### 主要特性
- **双实例支持**：两个 HSPL 硬件块，基地址不同
- **16 通道**：每个实例支持 16 个独立通道
- **硬件原子操作**：基于硬件实现的原子锁操作
- **资源级抽象**：提供资源级 API，简化上层使用

---

## 硬件架构

### HSPL 实例映射

BK7259 有两个 HSPL 硬件块：

| 实例 ID | 基地址 | 说明 | 驱动枚举 |
|---------|--------|------|----------|
| 0 | 0x45010000 | M52 侧映射 | `BK_HSPL_ID_0` |
| 1 | 0x480C0000 | M55 侧映射 | `BK_HSPL_ID_1` |

### 实例分工与中断归属

- **BK_HSPL_ID_0**：用于 **CP 与 AP 之间 4 个 CPU 的资源互斥**，HSPL 超时中断由 **M52** 侧处理。
- **BK_HSPL_ID_1**：用于 **AP 侧 SMP（CPU2/CPU3）内部资源互斥**，HSPL 超时中断由 **AP M55** 侧处理。

### 寄存器说明

每个通道占用两个寄存器：
- **LOCK[ch]**：读写寄存器，实现硬件自旋锁
  - **读操作**：尝试加锁
    - 返回 `0x1`：加锁成功（unlock -> lock）
    - 返回 `6'b1xxxx0`：已被锁定，`xxxx` 为 owner core_id
  - **写操作**：释放锁
    - 写入 `0xA55A80AF`：释放锁（lock -> unlock）
- **STA[ch]**：只读辅助寄存器，不改变锁状态，便于调试

### 通道分配

每个 HSPL 实例支持 16 个通道（0-15），上层可以将不同的共享资源分配到不同的实例和通道上：
- `BK_HSPL_ID_0`：建议用于 CP/AP 之间跨核互斥
- `BK_HSPL_ID_1`：建议用于 AP SMP 内部互斥

> CP 侧资源锁接口已限制使用 `BK_HSPL_ID_1`，在 CP 调用会返回 `BK_ERR_NOT_SUPPORT`。

---

## 驱动初始化

### 1. 初始化 HSPL 驱动

```c
#include "hspl_driver.h"

void app_init(void)
{
    bk_err_t ret;
    
    /* 初始化 HSPL 驱动 */
    ret = bk_hspl_driver_init();
    if (ret != BK_OK) {
        BK_LOGE("HSPL driver init failed: %d\r\n", ret);
        return;
    }
    
    /* 可选：注册超时回调（用于调试） */
    bk_hspl_register_timeout_callback(BK_HSPL_ID_0, 
                                      hspl_timeout_cb, 
                                      NULL);
}
```

### 2. 反初始化（如需要）

```c
void app_deinit(void)
{
    bk_hspl_driver_deinit();
}
```

---

## 底层 API 使用

底层 API 直接操作 HSPL 实例和通道，适合需要精确控制的场景。

### 头文件

```c
#include "hspl_driver.h"
```

### 1. 尝试加锁

```c
uint8_t owner_id = 0xFF;
bk_err_t ret = bk_hspl_try_lock(BK_HSPL_ID_0, 0, &owner_id);

if (ret == BK_OK) {
    /* 加锁成功，可以访问共享资源 */
    // ... 临界区代码 ...
    
    /* 释放锁 */
    bk_hspl_unlock(BK_HSPL_ID_0, 0);
} else {
    /* 加锁失败，通道已被其他核心占用 */
    if (owner_id != 0xFF) {
        BK_LOGI("Locked by core %u\r\n", owner_id);
    }
}
```

### 2. 释放锁

```c
bk_err_t ret = bk_hspl_unlock(BK_HSPL_ID_0, 0);
if (ret != BK_OK) {
    BK_LOGE("Unlock failed\r\n");
}
```

### 3. 查询锁状态

```c
hspl_state_t state = {0};
bk_err_t ret = bk_hspl_get_state(BK_HSPL_ID_0, 0, &state);

if (ret == BK_OK) {
    if (state.locked) {
        BK_LOGI("Channel 0 is locked by core %u\r\n", state.owner_id);
    } else {
        BK_LOGI("Channel 0 is unlocked\r\n");
    }
}
```

### 4. 完整示例：保护共享资源

```c
void access_shared_resource(void)
{
    uint8_t owner_id;
    bk_err_t ret;
    
    /* 尝试加锁 */
    ret = bk_hspl_try_lock(BK_HSPL_ID_0, 0, &owner_id);
    if (ret != BK_OK) {
        BK_LOGE("Failed to acquire lock\r\n");
        return;
    }
    
    /* 临界区：访问共享资源 */
    // ... 共享资源操作 ...
    
    /* 释放锁 */
    ret = bk_hspl_unlock(BK_HSPL_ID_0, 0);
    if (ret != BK_OK) {
        BK_LOGE("Failed to release lock\r\n");
    }
}
```

---

## 资源级 API 使用

资源级 API 提供了更高层的抽象，将逻辑资源（如 FLASH、CLOCK）映射到具体的 HSPL 实例和通道，简化上层使用。

### 头文件

```c
#include "hspl_res_lock.h"
```

### 1. 资源类型

```c
typedef enum {
    BK_HSPL_RES_FLASH = 0,  /* Flash 操作 */
    BK_HSPL_RES_CLOCK,      /* Clock 配置 */
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
    BK_HSPL_RES_UART_LOG,

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
    BK_HSPL_RES_PSRAM,
    BK_HSPL_RES_USER2,      /* 用户资源 2 */

    BK_HSPL_RES_MAX,
} bk_hspl_res_t;
```

### 2. 加锁（带超时）

```c
/* 尝试加锁，超时时间 1000 微秒 */
bk_err_t ret = bk_hspl_res_lock(BK_HSPL_RES_FLASH, 1000);
if (ret == BK_OK) {
    /* 加锁成功 */
    // ... Flash 操作 ...
    
    /* 释放锁 */
    bk_hspl_res_unlock(BK_HSPL_RES_FLASH);
} else if (ret == BK_ERR_TIMEOUT) {
    /* 超时 */
    BK_LOGE("Failed to acquire FLASH lock: timeout\r\n");
} else {
    /* 其他错误 */
    BK_LOGE("Failed to acquire FLASH lock: %d\r\n", ret);
}
```

### 3. 立即尝试加锁（无超时）

```c
/* timeout_us = 0 表示只尝试一次，不等待 */
bk_err_t ret = bk_hspl_res_lock(BK_HSPL_RES_FLASH, 0);
/* 或者使用封装的 try-lock */
ret = bk_hspl_res_try_lock(BK_HSPL_RES_FLASH);
```

### 4. IRQ-safe 包装（本地关中断）

```c
uint32_t flags;
bk_err_t ret = bk_hspl_res_lock_irqsave(BK_HSPL_RES_FLASH, &flags);
if (ret == BK_OK) {
    /* 临界区：本地中断已关闭 */
    // ...
    bk_hspl_res_unlock_irqrestore(BK_HSPL_RES_FLASH, flags);
}
```

> 注意：该接口仅 **try-lock 一次**，失败会自动恢复中断并返回 `BK_ERR_TIMEOUT`。
> 适用于 **同核 ISR 也会用同一把锁** 的短临界区。

### 5. 永久等待（任务上下文）

```c
/* 仅在任务上下文使用，ISR 中禁止 */
bk_err_t ret = bk_hspl_res_lock(BK_HSPL_RES_FLASH, BK_HSPL_WAIT_FOREVER);
if (ret == BK_OK) {
    // ... 临界区 ...
    bk_hspl_res_unlock(BK_HSPL_RES_FLASH);
}
```

### 6. 完整示例：Flash 操作保护

```c
void flash_write_protected(uint32_t addr, const void *data, size_t len)
{
    /* 获取 Flash 锁，超时 1000 微秒 */
    if (bk_hspl_res_lock(BK_HSPL_RES_FLASH, 1000) != BK_OK) {
        BK_LOGE("Failed to acquire FLASH lock\r\n");
        return;
    }
    
    /* 执行 Flash 写操作 */
    flash_write(addr, data, len);
    
    /* 释放锁 */
    bk_hspl_res_unlock(BK_HSPL_RES_FLASH);
}
```

### 5. 查询资源映射（调试用）

```c
uint8_t hspl_id, channel;
bk_err_t ret = bk_hspl_res_get_map(BK_HSPL_RES_FLASH, &hspl_id, &channel);
if (ret == BK_OK) {
    BK_LOGI("FLASH resource maps to hspl_id=%u, channel=%u\r\n", 
            hspl_id, channel);
}
```

### 6. 资源映射规则

资源映射采用**自动映射规则**，资源ID直接对应HSPL实例和通道：

- **资源 0-15**：使用 `BK_HSPL_ID_0` (M52) 的通道 0-15
- **资源 16-31**：使用 `BK_HSPL_ID_1` (M55) 的通道 0-15

#### 资源映射表

| 资源ID | 资源名称 | HSPL 实例 | 通道 | 说明 |
|--------|---------|-----------|------|------|
| 0 | FLASH | BK_HSPL_ID_0 (M52) | 0 | CP/AP共享 |
| 1 | CLOCK | BK_HSPL_ID_0 (M52) | 1 | CP/AP共享 |
| 2 | POWER | BK_HSPL_ID_0 (M52) | 2 | CP/AP共享 |
| 3 | SYS | BK_HSPL_ID_0 (M52) | 3 | CP/AP共享 |
| 4 | RTC | BK_HSPL_ID_0 (M52) | 4 | CP/AP共享 |
| 5 | ANA | BK_HSPL_ID_0 (M52) | 5 | CP/AP共享 |
| 6 | FUSE | BK_HSPL_ID_0 (M52) | 6 | CP/AP共享 |
| 7 | TRNG | BK_HSPL_ID_0 (M52) | 7 | CP/AP共享 |
| 8 | WDT | BK_HSPL_ID_0 (M52) | 8 | CP/AP共享 |
| 9 | SPI | BK_HSPL_ID_0 (M52) | 9 | CP/AP共享 |
| 10 | GPIO | BK_HSPL_ID_0 (M52) | 10 | CP/AP共享 |
| 11 | PWM | BK_HSPL_ID_0 (M52) | 11 | CP/AP共享 |
| 12 | ADC | BK_HSPL_ID_0 (M52) | 12 | CP/AP共享 |
| 13 | DAC | BK_HSPL_ID_0 (M52) | 13 | CP/AP共享 |
| 14 | PMU | BK_HSPL_ID_0 (M52) | 14 | CP/AP共享 |
| 15 | USER0 | BK_HSPL_ID_0 (M52) | 15 | CP/AP共享 |
| 16 | OS | BK_HSPL_ID_1 (M55) | 0 | **仅AP SMP** |
| 17 | LVGL | BK_HSPL_ID_1 (M55) | 1 | **仅AP SMP** |
| 18 | AUDIO | BK_HSPL_ID_1 (M55) | 2 | **仅AP SMP** |
| 19 | VIDEO | BK_HSPL_ID_1 (M55) | 3 | **仅AP SMP** |
| 20 | GPU | BK_HSPL_ID_1 (M55) | 4 | **仅AP SMP** |
| 21 | NPU | BK_HSPL_ID_1 (M55) | 5 | **仅AP SMP** |
| 22 | DSP | BK_HSPL_ID_1 (M55) | 6 | **仅AP SMP** |
| 23 | ISP | BK_HSPL_ID_1 (M55) | 7 | **仅AP SMP** |
| 24 | VDEC | BK_HSPL_ID_1 (M55) | 8 | **仅AP SMP** |
| 25 | VENC | BK_HSPL_ID_1 (M55) | 9 | **仅AP SMP** |
| 26 | SDIO | BK_HSPL_ID_1 (M55) | 10 | **仅AP SMP** |
| 27 | SDIO_HS | BK_HSPL_ID_1 (M55) | 11 | **仅AP SMP** |
| 28 | SDIO_HS_HS | BK_HSPL_ID_1 (M55) | 12 | **仅AP SMP** |
| 29 | USB | BK_HSPL_ID_1 (M55) | 13 | **仅AP SMP** |
| 30 | USER1 | BK_HSPL_ID_1 (M55) | 14 | **仅AP SMP** |
| 31 | USER2 | BK_HSPL_ID_1 (M55) | 15 | **仅AP SMP** |

> **注意**：
> - 资源ID与HSPL通道号一一对应，映射规则固定，无需手动配置
> - CP侧使用资源16-31时会返回 `BK_ERR_NOT_SUPPORT`

---

## 配置说明

### Kconfig 配置

在项目配置文件中启用 HSPL 驱动：

```
CONFIG_HSPL=y                    # 启用 HSPL 驱动
CONFIG_HSPL_TEST=y               # 启用 HSPL 测试功能（可选）
```

### 编译配置

HSPL 驱动已集成到构建系统中，启用 `CONFIG_HSPL` 后会自动编译：
- `hspl_driver.c` - 底层驱动实现
- `hspl_res_lock.c` - 资源级 API 实现

---

## 注意事项

### 0. 使用规范（建议）

- **避免默认无限等待**：请使用有限超时或 `bk_hspl_res_try_lock()`，只有明确场景才使用 `BK_HSPL_WAIT_FOREVER`。
- **ISR 中不可阻塞**：中断上下文只允许 try-lock（驱动已在 ISR 中退化为单次尝试）。
- **同核 ISR/任务复用同一锁**：若同核 ISR 也会使用同一把锁，任务侧取锁前建议 **本地关中断**，并保证临界区尽量短。
- **长临界区不要关中断**：避免中断延迟过大；此时应保证 ISR 不使用同一锁。

### 1. 锁的持有时间

- **尽量短**：锁的持有时间应该尽可能短，避免影响其他核心的访问
- **避免阻塞**：在持有锁期间，避免执行可能阻塞的操作（如长时间延时、等待外部事件等）

### 2. 死锁预防

- **顺序加锁**：如果需要在多个通道上加锁，所有核心应该按照相同的顺序加锁
- **避免嵌套**：尽量避免在持有锁的情况下再次尝试获取锁

### 3. 超时设置

- **合理设置**：根据实际场景设置合理的超时时间
- **超时处理**：超时后应该妥善处理，避免无限等待

### 4. 错误处理

- **检查返回值**：始终检查 API 的返回值
- **资源清理**：确保在错误情况下也能正确释放锁

### 5. 多核同步

- **4 核支持**：HSPL 支持 4 核（M52 CPU0/CPU1 和 M55 CPU2/CPU3）之间的同步
- **实例选择**：根据资源的使用情况选择合适的 HSPL 实例

### 6. 性能考虑

- **硬件实现**：HSPL 是硬件实现的，性能优于软件自旋锁
- **通道分配**：合理分配通道，避免热点通道竞争

---

## API 参考

### 底层 API（hspl_driver.h）

| 函数 | 说明 |
|------|------|
| `bk_hspl_driver_init()` | 初始化驱动 |
| `bk_hspl_driver_deinit()` | 反初始化驱动 |
| `bk_hspl_try_lock()` | 尝试加锁 |
| `bk_hspl_unlock()` | 释放锁 |
| `bk_hspl_get_state()` | 查询锁状态 |
| `bk_hspl_read_lock_raw()` | 读取 LOCK 寄存器原始值 |
| `bk_hspl_read_sta_raw()` | 读取 STA 寄存器原始值 |
| `bk_hspl_timeout_config()` | 配置超时监控 |
| `bk_hspl_timeout_irq_enable()` | 启用/禁用超时中断 |
| `bk_hspl_timeout_irq_clear()` | 清除超时中断 |
| `bk_hspl_register_timeout_callback()` | 注册超时回调 |

### 资源级 API（hspl_res_lock.h）

| 函数 | 说明 |
|------|------|
| `bk_hspl_res_lock()` | 资源加锁（带超时） |
| `bk_hspl_res_try_lock()` | 资源 try-lock（一次尝试） |
| `bk_hspl_res_lock_irqsave()` | 资源加锁（本地关中断，try-lock 一次） |
| `bk_hspl_res_unlock_irqrestore()` | 资源解锁并恢复中断 |
| `bk_hspl_res_unlock()` | 资源解锁 |
| `bk_hspl_res_get_map()` | 查询资源映射 |

---

## 示例代码

### 示例 1：Flash 操作保护

```c
#include "hspl_res_lock.h"

void safe_flash_write(uint32_t addr, const void *data, size_t len)
{
    /* 获取 Flash 锁 */
    if (bk_hspl_res_lock(BK_HSPL_RES_FLASH, 1000) != BK_OK) {
        BK_LOGE("Failed to acquire FLASH lock\r\n");
        return;
    }
    
    /* 执行 Flash 写操作 */
    flash_write_internal(addr, data, len);
    
    /* 释放锁 */
    bk_hspl_res_unlock(BK_HSPL_RES_FLASH);
}
```

### 示例 2：Clock 配置保护

```c
#include "hspl_res_lock.h"

void safe_clock_config(uint32_t freq)
{
    /* 获取 Clock 锁 */
    if (bk_hspl_res_lock(BK_HSPL_RES_CLOCK, 500) != BK_OK) {
        BK_LOGE("Failed to acquire CLOCK lock\r\n");
        return;
    }
    
    /* 配置时钟 */
    clock_set_frequency(freq);
    
    /* 释放锁 */
    bk_hspl_res_unlock(BK_HSPL_RES_CLOCK);
}
```

### 示例 3：多通道使用

```c
#include "hspl_driver.h"

void multi_channel_example(void)
{
    uint8_t owner_id;
    
    /* 在通道 0 上加锁 */
    if (bk_hspl_try_lock(BK_HSPL_ID_0, 0, &owner_id) == BK_OK) {
        /* 操作资源 A */
        // ...
        bk_hspl_unlock(BK_HSPL_ID_0, 0);
    }
    
    /* 在通道 1 上加锁 */
    if (bk_hspl_try_lock(BK_HSPL_ID_0, 1, &owner_id) == BK_OK) {
        /* 操作资源 B */
        // ...
        bk_hspl_unlock(BK_HSPL_ID_0, 1);
    }
}
```

---

## 常见问题

### Q1: 如何选择合适的 HSPL 实例？

**A:** 根据资源的使用情况选择：
- 如果资源主要在 M52 侧使用，选择 `BK_HSPL_ID_0`
- 如果资源主要在 M55 侧使用，选择 `BK_HSPL_ID_1`
- 如果资源在多个核心间共享，可以任意选择

### Q2: 如何避免死锁？

**A:** 
- 所有核心按照相同的顺序加锁
- 避免在持有锁的情况下再次尝试获取锁
- 使用超时机制，避免无限等待

### Q3: 锁的持有时间应该多长？

**A:** 应该尽可能短，建议：
- 简单操作：< 1ms
- 复杂操作：< 10ms
- 避免在持有锁期间执行可能阻塞的操作

### Q4: 如何处理超时？

**A:** 
- 设置合理的超时时间
- 超时后应该放弃操作或重试
- 记录超时日志，便于问题分析

---

## 版本历史

- **v1.0** (2025-01): 初始版本
  - 支持双实例 HSPL
  - 提供底层 API 和资源级 API
  - 支持 16 通道
  - 支持超时监控和中断

---

## 相关文档

- [HSPL 测试文档](HSPL_TEST_GUIDE.md)
- [HSPL 驱动 API 参考](hspl_driver.h)
- [HSPL 资源锁 API 参考](hspl_res_lock.h)
