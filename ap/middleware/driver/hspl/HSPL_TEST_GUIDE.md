# HSPL (Hardware Spin Lock) 测试指南

## 目录
1. [概述](#概述)
2. [测试环境准备](#测试环境准备)
3. [CLI 测试命令](#cli-测试命令)
4. [自动化压力测试](#自动化压力测试)
5. [测试结果分析](#测试结果分析)
6. [故障排查](#故障排查)
7. [测试用例](#测试用例)

---

## 概述

HSPL 测试指南提供了完整的测试方法和用例，用于验证 HSPL 驱动在多核环境下的正确性和性能。

### 测试类型
- **功能测试**：验证基本的加锁/解锁功能
- **压力测试**：验证多核竞争场景下的正确性
- **性能测试**：评估锁操作的性能指标
- **稳定性测试**：长时间运行验证稳定性

---

## 测试环境准备

### 1. 配置启用

确保在项目配置中启用了 HSPL 测试功能：

```
CONFIG_HSPL=y                    # 启用 HSPL 驱动
CONFIG_HSPL_TEST=y               # 启用 HSPL 测试功能
CONFIG_CLI=y                     # 启用 CLI（测试命令需要）
CONFIG_DRIVER_TEST=y             # 启用驱动测试框架
```

如果希望超时统计依赖 AON RTC，请开启：

```
CONFIG_AON_RTC=y                 # 使用 AON RTC 作为超时基准
```

未开启 `CONFIG_AON_RTC` 时，超时基准自动回退为 `rtos_get_time()`。

### 2. 驱动初始化

在测试前，确保 HSPL 驱动和 IPI 驱动已初始化：

```bash
# 初始化 HSPL 驱动
hspl_driver init

# 初始化 IPI 驱动（压力测试需要）
ipi_driver init
```

### 3. 测试环境

- **硬件平台**：BK7259
- **CPU 核心**：CP M52 CPU0 和 AP M55 CPU2
- **通信方式**：IPI（Inter-Processor Interrupt）

---

## CLI 测试命令

### 1. 驱动管理

#### 初始化驱动
```bash
hspl_driver init
```
- 初始化 HSPL 驱动
- 注册中断处理函数
- 注册超时回调（可选）

#### 反初始化驱动
```bash
hspl_driver deinit
```
- 清理驱动资源
- 注销中断处理函数

### 2. 底层锁操作测试

#### 尝试加锁
```bash
# 格式：hspl lock {hspl_id} {ch}
hspl lock 0 0        # 在 HSPL 实例 0 的通道 0 上加锁
hspl lock 1 5        # 在 HSPL 实例 1 的通道 5 上加锁

# 兼容格式（默认 hspl_id=0）
hspl lock 0          # 等同于 hspl lock 0 0
```

**预期结果**：
- 如果锁成功：显示 `HSPL lock ok: hspl_id=0 ch=0`
- 如果锁失败：显示 `HSPL lock fail: hspl_id=0 ch=0 owner=X`

#### 释放锁
```bash
# 格式：hspl unlock {hspl_id} {ch}
hspl unlock 0 0      # 释放 HSPL 实例 0 的通道 0 的锁
hspl unlock 1 5      # 释放 HSPL 实例 1 的通道 5 的锁

# 兼容格式（默认 hspl_id=0）
hspl unlock 0        # 等同于 hspl unlock 0 0
```

**预期结果**：
- 成功：显示 `HSPL unlock ok: hspl_id=0 ch=0`
- 失败：显示 `HSPL unlock fail: hspl_id=0 ch=0`

#### 查询锁状态
```bash
# 查询单个通道
hspl state 0 0      # 查询 HSPL 实例 0 的通道 0 状态
hspl state 1 5      # 查询 HSPL 实例 1 的通道 5 状态

# 查询所有通道
hspl state 0 all    # 查询 HSPL 实例 0 的所有通道状态
hspl state 1 all    # 查询 HSPL 实例 1 的所有通道状态

# 兼容格式（默认 hspl_id=0）
hspl state 0        # 等同于 hspl state 0 0
hspl state all      # 等同于 hspl state 0 all
```

**输出示例**：
```
hspl_id=0 ch=0 locked=1 owner_valid=1 owner_id=2
hspl_id=0 ch=1 locked=0 owner_valid=0 owner_id=0
```

### 3. 资源级锁操作测试

#### 资源加锁
```bash
# 格式：hspl res_lock {resource} {timeout_us}
hspl res_lock flash 1000      # 获取 Flash 锁，超时 1000 微秒
hspl res_lock clock 500       # 获取 Clock 锁，超时 500 微秒
hspl res_lock os 2000         # 获取 OS 锁，超时 2000 微秒
hspl res_lock user0 0         # 获取 User0 锁，立即尝试（不等待）
```

**支持的资源**：
- `flash` - Flash 操作
- `clock` - Clock 配置
- `os` - OS 资源
- `user0`, `user1`, `user2` - 用户资源

**预期结果**：
- 成功：显示 `res_lock ok: flash (hspl_id=0 ch=0)`
- 失败：显示 `res_lock fail: -1`（超时或其他错误）

#### 资源解锁
```bash
# 格式：hspl res_unlock {resource}
hspl res_unlock flash
hspl res_unlock clock
hspl res_unlock os
hspl res_unlock user0
```

**预期结果**：
- 成功：显示 `res_unlock ok: flash`
- 失败：显示 `res_unlock fail: -1`

### 4. 调试命令

#### 读取原始寄存器值
```bash
# 读取 STA 寄存器（不影响锁状态）
hspl raw_sta 0 0     # 读取 HSPL 实例 0 通道 0 的 STA 寄存器
hspl raw_sta 1 5     # 读取 HSPL 实例 1 通道 5 的 STA 寄存器

# 读取 LOCK 寄存器（注意：读取会触发锁尝试）
hspl raw_lock 0      # 读取 HSPL 实例 0 通道 0 的 LOCK 寄存器（默认实例）
hspl raw_lock2 1 5   # 读取 HSPL 实例 1 通道 5 的 LOCK 寄存器
```

#### 超时配置
```bash
# 格式：hspl timeout_cfg {hspl_id} {ch} {th_cycles} {en}
hspl timeout_cfg 0 0 1000000 1    # 配置实例 0 通道 0，阈值 1000000 时钟周期，启用
hspl timeout_cfg 1 5 500000 0     # 配置实例 1 通道 5，阈值 500000 时钟周期，禁用
```

#### 超时中断控制
```bash
# 格式：hspl timeout_irq {hspl_id} {enable|disable|clear}
hspl timeout_irq 0 enable     # 启用实例 0 的超时中断
hspl timeout_irq 0 disable    # 禁用实例 0 的超时中断
hspl timeout_irq 0 clear      # 清除实例 0 的超时中断
```

---

## 自动化压力测试

### 1. 压力测试命令

#### 自动并行压力测试（推荐）

```bash
# 格式：hspl stress_auto {hspl_id} {ch} {iter} {hold_ms}
hspl stress_auto 0 0 10000 2
```

**参数说明**：
- `hspl_id`: HSPL 实例 ID（0 或 1）
- `ch`: 通道号（0-15）
- `iter`: 每个核心的迭代次数
- `hold_ms`: 每次持有锁的时间（毫秒）

**功能**：
- 自动在 **CP M52 CPU0** 和 **AP M55 CPU2** 上启动并行压力测试
- 两个 CPU 同时竞争同一个 HSPL 通道
- 自动统计成功/失败次数

**使用示例**：
```bash
# 在 CPU0 或 CPU2 上执行（推荐在 CPU0 上执行）
hspl stress_auto 0 0 10000 2
# 参数说明：
# - hspl_id=0: 使用 HSPL 实例 0
# - ch=0: 使用通道 0
# - iter=10000: 每个 CPU 执行 10000 次锁/解锁操作
# - hold_ms=2: 每次持有锁 2ms
```

#### 手动压力测试

```bash
# 格式：hspl stress {hspl_id} {ch} {iter} {hold_ms}
hspl stress 0 0 1000 10
```

**功能**：
- 在当前 CPU 上启动压力测试
- 通过 IPI 通知另一个 CPU 启动测试
- 需要分别在 CPU0 和 CPU2 上执行命令

**使用示例**：
```bash
# 在 CPU0 上执行
hspl stress 0 0 1000 10

# 在 CPU2 上执行（几乎同时）
hspl stress 0 0 1000 10
```

### 2. 查看测试统计

```bash
# 在对应的 CPU 上执行
hspl stress_stat
```

**输出示例**：
```
=== Stress Test Statistics (Core 0) ===
Running: No
Lock Success: 3561
Lock Fail: 6439
Unlock Success: 3561
Unlock Fail: 0
Timeout Count: 0
Success Rate: 35%
```

**说明**：
- `Running`: 测试是否正在运行
- `Lock Success`: 成功加锁次数
- `Lock Fail`: 加锁失败次数（被其他核心占用）
- `Unlock Success`: 成功解锁次数
- `Unlock Fail`: 解锁失败次数（应该为 0）
- `Timeout Count`: 超时次数
- `Success Rate`: 成功率百分比

### 3. 停止测试

```bash
# 在对应的 CPU 上执行
hspl stress_stop
```

**功能**：
- 停止当前 CPU 的测试
- 通过 IPI 通知另一个 CPU 停止测试

---

## 测试结果分析

### 1. 正常测试结果

#### 典型输出（CPU0）
```
hspl_tes:I: Core 0: Starting stress test (hspl_id=0 ch=0 iter=10000 hold=2ms)
hspl_tes:I: Core 0: Stress test completed in 17122ms
hspl_tes:I: Core 0: lock_success=3561 lock_fail=6439 unlock_success=3561 unlock_fail=0
```

#### 典型输出（CPU2）
```
hspl_tes:I: Core 2: Starting stress test (hspl_id=0 ch=0 iter=10000 hold=2ms)
hspl_tes:I: Core 2: Stress test completed in 17124ms
hspl_tes:I: Core 2: lock_success=3562 lock_fail=6438 unlock_success=3562 unlock_fail=0
```

### 2. 结果验证

#### 正确性检查
- ✅ **迭代次数正确**：`lock_success + lock_fail = iter`
- ✅ **解锁正确**：`unlock_success = lock_success`，`unlock_fail = 0`
- ✅ **同步良好**：两个 CPU 的测试时间几乎相同（差异 < 100ms）
- ✅ **竞争公平**：两个 CPU 的成功率接近（差异 < 5%）

#### 性能指标
- **成功率**：在激烈竞争下，成功率通常在 30-50% 之间
- **测试时间**：取决于迭代次数和持有锁时间
- **无超时**：`Timeout Count = 0` 表示没有超时发生

### 3. 异常情况

#### 解锁失败
```
unlock_fail > 0
```
**可能原因**：
- 锁状态异常
- 驱动实现问题

**处理**：
- 检查驱动实现
- 查看详细日志

#### 成功率异常
```
Success Rate < 10% 或 > 90%
```
**可能原因**：
- 一个 CPU 占主导地位
- 锁机制异常

**处理**：
- 检查 CPU 负载
- 验证锁的公平性

#### 测试时间差异大
```
|time_cpu0 - time_cpu2| > 1000ms
```
**可能原因**：
- 同步机制问题
- CPU 负载不均衡

**处理**：
- 检查 IPI 通信
- 验证同步机制

---

## 故障排查

### 1. 命令未找到

**问题**：`$cmd NOT found: hspl`

**原因**：`CONFIG_HSPL_TEST` 未启用

**解决**：
1. 检查配置文件：`projects/app/ap/config/bk7259_ap/config`
2. 确保 `CONFIG_HSPL_TEST=y`
3. 重新编译：`make bk7259`

### 2. 参数验证失败

**问题**：`Invalid hspl_id: 2 (must be 0 or 1)`

**原因**：使用了无效的 `hspl_id`

**解决**：
- 使用有效的 `hspl_id`（0 或 1）
- 检查通道号范围（0-15）

### 3. IPI 通信失败

**问题**：压力测试无法启动另一个 CPU

**原因**：IPI 驱动未初始化或通信异常

**解决**：
1. 初始化 IPI 驱动：`ipi_driver init`
2. 检查 IPI 配置
3. 验证 CPU 间通信

### 4. 任务异常退出

**问题**：`OS:E: ==> Task exits abnormally!`

**原因**：任务退出方式不正确（已修复）

**解决**：
- 确保使用最新代码
- 任务会正确调用 `rtos_delete_thread(NULL)` 退出

### 5. 锁操作失败

**问题**：所有锁操作都失败

**原因**：
- 使用了无效的 `hspl_id` 或 `channel`
- 驱动未正确初始化

**解决**：
1. 检查参数有效性
2. 确认驱动已初始化：`hspl_driver init`
3. 查看驱动初始化日志

---

## 测试用例

### 测试用例 1：基本功能测试

**目标**：验证基本的加锁/解锁功能

**步骤**：
1. 初始化驱动：`hspl_driver init`
2. 尝试加锁：`hspl lock 0 0`
3. 查询状态：`hspl state 0 0`
4. 释放锁：`hspl unlock 0 0`
5. 再次查询状态：`hspl state 0 0`

**预期结果**：
- 步骤 2：加锁成功
- 步骤 3：显示 `locked=1`
- 步骤 4：解锁成功
- 步骤 5：显示 `locked=0`

### 测试用例 2：资源级 API 测试

**目标**：验证资源级 API 的功能

**步骤**：
1. 获取 Flash 锁：`hspl res_lock flash 1000`
2. 查询映射：检查 Flash 资源映射到哪个实例和通道
3. 释放锁：`hspl res_unlock flash`

**预期结果**：
- 步骤 1：加锁成功
- 步骤 2：显示正确的映射信息
- 步骤 3：解锁成功

### 测试用例 3：多核竞争测试

**目标**：验证多核竞争场景下的正确性

**步骤**：
1. 在 CPU0 上启动压力测试：`hspl stress_auto 0 0 10000 2`
2. 等待测试完成
3. 在 CPU0 和 CPU2 上分别查看统计：`hspl stress_stat`

**预期结果**：
- 两个 CPU 都完成了 10000 次迭代
- 成功率在 30-50% 之间
- 所有成功的锁都正确解锁（`unlock_fail=0`）
- 测试时间接近

### 测试用例 4：长时间稳定性测试

**目标**：验证长时间运行的稳定性

**步骤**：
1. 启动长时间压力测试：`hspl stress_auto 0 0 100000 1`
2. 定期查看统计：`hspl stress_stat`
3. 观察是否有错误或异常

**预期结果**：
- 测试正常运行，无崩溃
- 统计信息正常
- 无超时或错误

### 测试用例 5：多通道测试

**目标**：验证多个通道的独立性

**步骤**：
1. 在通道 0 上加锁：`hspl lock 0 0`
2. 在通道 1 上加锁：`hspl lock 0 1`（应该成功）
3. 查询所有通道状态：`hspl state 0 all`
4. 释放所有锁

**预期结果**：
- 步骤 2：加锁成功（通道独立）
- 步骤 3：显示通道 0 和 1 都被锁定

### 测试用例 6：超时测试

**目标**：验证超时机制

**步骤**：
1. 在一个 CPU 上长时间持有锁
2. 在另一个 CPU 上尝试获取锁（设置短超时）
3. 观察超时行为

**预期结果**：
- 超时后返回 `BK_ERR_TIMEOUT`
- 超时计数增加

---

## 测试报告模板

### 测试环境
- **硬件平台**：BK7259
- **软件版本**：xxx
- **测试日期**：yyyy-mm-dd
- **测试人员**：xxx

### 测试结果

#### 功能测试
- [ ] 基本加锁/解锁功能
- [ ] 资源级 API 功能
- [ ] 多通道独立性
- [ ] 超时机制

#### 压力测试
- [ ] 1000 次迭代测试
- [ ] 10000 次迭代测试
- [ ] 100000 次迭代测试

#### 性能指标
- **平均成功率**：xx%
- **平均测试时间**：xx ms
- **无错误运行时间**：xx 小时

### 问题记录
1. 问题描述
2. 复现步骤
3. 预期结果
4. 实际结果
5. 解决方案

---

## 版本历史

- **v1.0** (2025-01): 初始版本
  - 基本功能测试
  - 自动化压力测试
  - 测试结果分析

---

## 相关文档

- [HSPL 使用指南](HSPL_USER_GUIDE.md)
- [HSPL 驱动 API 参考](hspl_driver.h)
- [HSPL 资源锁 API 参考](hspl_res_lock.h)
