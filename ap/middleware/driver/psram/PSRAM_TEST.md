# PSRAM 测试命令说明

## 概述

本文档描述 `psram_test` 和 `psram_test_ext` CLI 命令中与 PSRAM 数据正确性、DMA 搬移校验、PSRAM 栈压力测试相关的三类测试功能。

三类测试使用**互不重叠的 PSRAM 物理地址区间**，支持**同时并行运行**。

所有测试均为**后台持续运行**模式：
- 无错时每 **5 秒**打印一次运行状态
- 出错时**立即打印**错误详情
- 启动时打印**测试地址空间区域**
- 需主动执行 **stop** 命令才会停止

---

## PSRAM 完整地址使用分布（BK7259，双 PSRAM 各 16MB）

### 硬件配置

- **PSRAM0**：16MB，基址 0x60000000，chip_id = 0x9A08
- **PSRAM1**：16MB，基址 0x64000000，chip_id = 0x9A08
- **访问模式**：Non-cacheable（当前硬件不支持 cacheable 地址配置）
- **CPU 核心**：AP 核（ap0/ap1）运行测试命令，CP 核（cp1）负责初始化

### PSRAM0 地址分区（0x60000000 ~ 0x61000000，16MB）

| 区域名 | 起始地址 | 结束地址 | 大小 | 用途 | 测试命令使用 |
|--------|----------|----------|------|------|-------------|
| `PSRAM_TEST_CPU` | 0x60000000 | 0x60400000 | 4MB | **测试区** | `psram_test start cpu` — CPU 逐 word 写入/回读校验 |
| `PSRAM_TEST_TASK` | 0x60400000 | 0x60600000 | 2MB | **测试区** | `psram_soak_start`（静置回读）或 `psram_rapid_start`（快速写读），二选一 |
| `PSRAM_MEM_SLAB_UNCODED` | 0x60600000 | 0x60C00000 | 6MB | 系统 slab | 通用媒体/缓冲 slab（非测试） |
| `PSRAM_TEST_CPU_DMA` | 0x60C00000 | 0x60D00000 | 1MB | **测试区** | `psram_test_ext cpu_dma_verify` — CPU 写 + DMA 搬移 + CPU 回读校验 |
| `PSRAM0_REMAIN` | 0x60D00000 | 0x61000000 | 3MB | 预留 | 余量，后续扩展 |

### PSRAM1 地址分区（0x64000000 ~ 0x65000000，16MB）

| 区域名 | 起始地址 | 结束地址 | 大小 | 用途 | 测试命令使用 |
|--------|----------|----------|------|------|-------------|
| `PSRAM_MEM_SLAB_CODED` | 0x64000000 | 0x64E00000 | 14MB | 系统 slab | 编码媒体 slab（非测试） |
| `CP_PSRAM_HEAP` | 0x64E00000 | 0x64E20000 | 128KB | CP 堆 | CP 核 psram_malloc（非测试） |
| `AP_PSRAM_HEAP` | 0x64E20000 | 0x64EC0000 | 640KB | **AP 堆** | `psram_test_ext stack_stress_start` — 任务栈从此堆分配（TCB + stack） |
| `AP_PSRAM_DATA_SECTION` | 0x64EC0000 | 0x64F00000 | 256KB | AP 数据段 | 系统 .data（非测试） |
| `AP_PSRAM_CODE_SECTION` | 0x64F00000 | 0x65000000 | 1MB | AP 代码段 | 系统 .text（非测试） |

### 测试命令与 PSRAM 地址对应关系

| 测试命令 | 测试内容 | PSRAM 芯片 | 地址范围 | 大小 | 访问方式 |
|----------|----------|-----------|----------|------|---------|
| `psram_test start cpu 0 500 0 0` | CPU 逐 word 写 + 读 + 比对 | PSRAM0 | 0x60000000–0x60400000 | 4MB | CPU 直接读写 |
| `psram_test_ext cpu_dma_verify` | CPU 写源区 → HPDMA 搬移到目的区 → CPU 读目的区比对 | PSRAM0 | 0x60C00000–0x60D00000 | 1MB（src 512KB + dst 512KB） | CPU + DMA |
| `psram_test_ext stack_stress_start` | PSRAM 栈任务创建/销毁压力测试（父+10子） | PSRAM1 | 0x64E20000–0x64EC0000 内分配 | 峰值 ~496KB | CPU（任务栈读写） |
| `psram_test_ext psram_soak_start` | 写入 pattern → 静置 30s → 回读校验 bit-flip | PSRAM0 | 0x60400000–0x60600000 | 2MB | CPU 直接读写 |
| `psram_test_ext psram_rapid_start` | 写入 pattern → 立即回读校验（无延时） | PSRAM0 | 0x60400000–0x60600000 | 2MB | CPU 高速连续读写 |

> Case 1/2/3 与 Case 4 或 Case 5 可并行运行。**Case 4 和 Case 5 共用区域，不能同时运行**。

### 测试 Case 概要

| # | Case 名称 | 测试目的 | 覆盖的故障类型 |
|---|-----------|---------|---------------|
| 1 | CPU 读写校验 | 验证 CPU 直接访问 PSRAM 的数据完整性 | stuck-at、地址译码错误、读写时序问题 |
| 2 | CPU+DMA 校验 | 验证 DMA 引擎（HPDMA）搬移 PSRAM 数据的正确性 | DMA 通道错误、burst 传输异常、源/目的地址对齐 |
| 3 | PSRAM 栈压力 | 验证 PSRAM 作为 FreeRTOS 任务栈时的可靠性，模拟频繁任务创建/销毁 | 栈空间读写、堆碎片、任务调度下的内存一致性 |
| 4 | 静置回读 bit-flip | 检测 PSRAM 数据在无访问期间的保持能力 | 刷新失败、电压波动、温度导致的 bit 翻转、coupling |
| 5 | 快速写读校验 | 检测 PSRAM 在高速连续写入时硬件响应能力 | 写建立时间不足、写恢复时间不足、连续写命令丢失 |

---

## 命令 1：CPU 读写 PSRAM 并校验

### 功能

CPU 直接写入 PSRAM 测试区，然后读回并逐 word 比对，验证数据正确性。后台持续循环运行。

### 命令格式

```
psram_test start cpu 0 [delay_ms] [silent 0|1] [data_type] [psram_id 0|1]
psram_test stop
```

### 参数说明

| 参数 | 说明 |
|------|------|
| 第1个参数 | 固定传 `0`（non-cacheable，当前硬件不支持 cacheable） |
| `delay_ms` | 每轮测试后延时（ms），默认 500 |
| `silent` | 0/1（本版本中出错才打印详细 log，无错每 5s 打印状态） |
| `data_type` | 0 = 默认 pattern，1 = TRNG 随机，2 = 递增混合 |
| `psram_id` | 0 = PSRAM0，1 = PSRAM1 |

### 使用示例

```
psram_test start cpu 0 500 0 0
psram_test stop
```

### 运行行为

- **启动时**打印：`cpu_test started: region [60000000-60400000] (4096KB)`
- **无错时**每 5 秒打印：`cpu_test running [60000000-60400000]: pass=N, fail=0`
- **出错时**立即打印错误详情（每轮最多 10 条）：
  `cpu_test ERR @60001000: got AABBCCDD expect 11223344 xor BBAA99FF`
  并打印轮次汇总：`cpu_test FAIL: error_num=X (pass=Y, fail=Z)`
- 执行 `psram_test stop` 停止

### 测试地址

固定使用 `PSRAM_TEST_CPU` 区：0x60000000 ~ 0x60400000（4MB）。

---

## 命令 2：CPU 写 + DMA 搬移 + CPU 读校验

### 功能

1. CPU 向 PSRAM 源半区写入确定性 pattern。
2. DMA（HPDMA 或 GDMA）将源半区数据搬移到目的半区。
3. CPU 读目的半区并逐 word 与期望值比对。
4. 后台持续循环运行，直到主动 stop。

### 命令格式

```
psram_test_ext cpu_dma_verify [half_size_kb]
psram_test_ext cpu_dma_verify_stop
```

### 参数说明

| 参数 | 说明 |
|------|------|
| `half_size_kb` | 可选，半区大小（KB）。默认为区域总大小的一半（512KB）。最大不超过 512KB。 |

### 使用示例

```
psram_test_ext cpu_dma_verify
psram_test_ext cpu_dma_verify 256
psram_test_ext cpu_dma_verify_stop
```

### 运行行为

- **启动时**打印：`cpu_dma_verify started: src=[60C00000-60C80000] dst=[60C80000-60D00000] half=512KB`
- **无错时**每 5 秒打印：`cpu_dma_verify running [60C00000-60D00000]: pass=N, fail=0`
- **DMA 搬移失败时**立即打印：`cpu_dma_verify: DMA copy failed (-1), fail=X`，跳过本轮校验，100ms 后重试
- **数据比对出错时**立即打印错误详情（每轮最多 10 条）：
  `cpu_dma_verify ERR @60C80004: got AABBCCDD expect A5A5A5A5 xor 0F1E698C`
  并打印轮次汇总：`cpu_dma_verify FAIL: error_num=X (pass=Y, fail=Z)`
- 执行 `cpu_dma_verify_stop` 停止，打印最终统计：`cpu_dma_verify stopped: pass=Y, fail=Z`

### 测试地址

固定使用 `PSRAM_TEST_CPU_DMA` 区：0x60C00000 ~ 0x60D00000（1MB）。

- 源半区：0x60C00000 ~ 0x60C80000（默认 512KB）
- 目的半区：0x60C80000 ~ 0x60D00000（默认 512KB）

---

## 命令 3：PSRAM 栈压力测试（stack_stress）

### 功能

1. 创建一个**使用 PSRAM 作为栈空间**的父任务。
2. 父任务**持续循环**，每轮随机对称地创建和销毁 **10 个**子任务（也使用 PSRAM 栈）。
3. 每个子任务命名为 `ss_w0` ~ `ss_w9`，创建和销毁时均有日志输出。
4. 每个子任务在栈上创建两个 4KB 局部数组 `buf_a` 和 `buf_b`（位于 PSRAM 上），写入 pattern 后 memcpy 并逐字节比对，验证 PSRAM 栈空间的读写正确性。
5. 创建与销毁的时序是随机的，但保证**对称**（每轮 10 次创建 + 10 次销毁）。

### 命令格式

```
psram_test_ext stack_stress_start
psram_test_ext stack_stress_stop
```

### 使用示例

```
psram_test_ext stack_stress_start
psram_test_ext stack_stress_stop
```

### 运行行为

- **启动时**打印：`stack_stress started: task_region [60400000-60600000] (2048KB), worker_stack=48KB, local_buf=4KB`
- **子任务创建/销毁时**打印：
  `stack_stress: create ss_w3 (round=5)`
  `stack_stress: destroy ss_w3 (round=5)`
- **无错时**每 5 秒打印：`stack_stress running [60400000-60600000]: rounds=N, errors=0`
- **数据比对出错时**立即打印 worker 编号和出错偏移：
  `worker42: mismatch @1024: a5 vs 00`
- **子任务创建失败时**（通常是 PSRAM 堆空间不足）：
  `stack_stress: create ss_w0 failed -1`
- **CD 序列异常时**（内部逻辑异常，不应出现）：
  `stack_stress: no free slot (bug)` — 需要创建但无空闲 slot
  `stack_stress: no active slot (bug)` — 需要销毁但无活跃 slot
- 执行 `stack_stress_stop` 停止，打印最终统计：`stack_stress stopped: rounds=N, errors=M`

### 测试地址与 PSRAM 栈分配范围

`rtos_create_psram_thread` 内部通过 `xTaskCreateInPsram` → `psram_malloc` 从
**`AP_PSRAM_HEAP`** 分配任务栈。启用 AP fast boot 时，TCB 及其调度器链表节点
保留在 SRAM，避免 PSRAM 恢复异常直接破坏 FreeRTOS ready/delayed list：

| 堆区域 | 起始地址 | 大小 | 说明 |
|--------|----------|------|------|
| `AP_PSRAM_HEAP` | **0x64E20000** | **640KB**（0xA0000） | PSRAM 堆，`psram_malloc` 的内存来源 |

因此，`stack_stress` 创建的 PSRAM 栈任务，其 **栈实际物理地址范围在 0x64E20000 ~ 0x64EC0000（AP_PSRAM_HEAP）内**，位于 PSRAM1 空间；启用 AP fast boot 时 TCB 不在此范围内。

### 栈空间预算

| 任务 | 栈大小 | 说明 |
|------|--------|------|
| 父任务（ss_parent） | 16KB | 无大局部变量 |
| 子任务（ss_w0~ss_w9） x10 | 48KB x10 = 480KB | 每个含 2 x 4KB 局部数组 + 调用链余量 |
| **合计（峰值）** | ~496KB | 需确保 `AP_PSRAM_HEAP`（640KB）有足够空闲 |

**注意**：若其它模块也在使用 `psram_malloc` 占用 `AP_PSRAM_HEAP`，并发运行时可能导致堆空间不足。建议运行 `stack_stress` 时尽量减少其它 `psram_malloc` 调用。

---

## 命令 4：PSRAM 静置回读 bit-flip 检测（psram_soak）

### 功能

向 PSRAM 指定区域写入 pattern，**静置一段时间**后回读校验，检测因刷新失败、电压波动等原因导致的 **bit 翻转**。

1. 使用 `PSRAM_TEST_TASK` 区域（0x60400000，2MB）作为测试内存。
2. 每轮测试：写入当前 pattern → **静置 `soak_ms`**（默认 30 秒） → 逐 word 回读并与期望值比较。
3. 循环使用 8 种经典内存测试 pattern，持续运行直到 stop。
4. 发现 bit-flip 时立即打印详细信息（错误地址、实际值、期望值、XOR 差异位）。

### 测试 Pattern

| # | Pattern 值 | 典型用途 |
|---|-----------|---------|
| 0 | `0x55555555` | 棋盘格（偶数位 1） |
| 1 | `0xAAAAAAAA` | 反棋盘格（奇数位 1） |
| 2 | `0x00000000` | 全 0 |
| 3 | `0xFFFFFFFF` | 全 1 |
| 4 | `0x12345678` | 递增混合 |
| 5 | `0xA5A5A5A5` | 交错字节 |
| 6 | `0x0F0F0F0F` | 高低半字节交替 |
| 7 | `0xF0F0F0F0` | 反半字节交替 |

每种 pattern 写入时与 word 索引异或（`pattern ^ index`），确保相邻 word 值不同，能有效检测 stuck-at 和 coupling 故障。

### 命令格式

```
psram_test_ext psram_soak_start [soak_ms]
psram_test_ext psram_soak_stop
```

### 参数说明

| 参数 | 说明 |
|------|------|
| `soak_ms` | 可选，每轮写入后的静置时间（ms）。默认 **30000**（30 秒）。值越大，数据在 PSRAM 上驻留越久，越容易暴露刷新相关的 bit 翻转。 |

### 使用示例

```
psram_test_ext psram_soak_start          # 默认静置 30s
psram_test_ext psram_soak_start 60000    # 静置 60s，更敏感
psram_test_ext psram_soak_stop
```

### 运行行为

- **启动时**打印：`psram_soak started: region [60400000-60600000] (2048KB), soak_ms=30000, patterns=8`
- **无错时**每 5 秒打印：`psram_soak running [60400000-60600000]: pat[N]=XXXXXXXX, pass=P, fail=0, flip_words=0`
- **出错时**立即打印 bit-flip 详情（每轮最多 10 条）：
  `soak bit-flip @60401000: got AABBCCDD expect 55555155 xor FF6699E8 (pat[0]=55555555)`
  并打印轮次汇总：`soak FAIL pat[0]=55555555: 3 words flipped (total_pass=12, total_fail=1, total_flip=3)`
- **启动失败时**（线程创建失败）：`psram_soak: create task failed -1`
- 执行 `psram_soak_stop` 停止，打印最终统计：`psram_soak stopped: pass=Y, fail=Z, flip_words=W`

### 测试地址

固定使用 `PSRAM_TEST_TASK` 区：0x60400000 ~ 0x60600000（2MB）。

> **注意**：`psram_soak` 与 `psram_rapid` 共用同一段 PSRAM 区域，**不能同时运行**。启动 `psram_rapid` 时若 `psram_soak` 正在运行，会拒绝启动并提示错误。

---

## 命令 5：PSRAM 快速写读校验（psram_rapid）

### 功能

对 PSRAM 区域执行**连续写入后立即回读**，不做任何静置等待。用于检测 PSRAM 在高速连续写操作时，硬件是否能正确响应每次写命令（验证写建立时间、写恢复时间等时序是否满足）。

1. 使用 `PSRAM_TEST_TASK` 区域（0x60400000，2MB）作为测试内存。
2. 每轮测试：用当前 pattern 填充整个 2MB → **写完立即回读校验**（无任何延时）。
3. 循环使用 8 种 pattern（与 soak 相同），持续运行直到 stop。
4. 发现错误时立即打印详细信息。

### 与 soak 的对比

| 对比项 | psram_soak（Case 4） | psram_rapid（Case 5） |
|-------|---------------------|----------------------|
| 写后等待 | 静置 30 秒 | **立即回读，无等待** |
| 检测目标 | 数据保持能力（刷新失败、bit-flip） | **写入时序可靠性**（建立时间、恢复时间） |
| 写入频率 | 低（每 30 秒写一次） | **极高**（连续不停写→读→写→读） |
| 总线压力 | 低 | **高**（持续占用总线带宽） |

### 命令格式

```
psram_test_ext psram_rapid_start
psram_test_ext psram_rapid_stop
```

### 使用示例

```
psram_test_ext psram_rapid_start
psram_test_ext psram_rapid_stop
```

### 运行行为

- **启动时**打印：`psram_rapid started: region [60400000-60600000] (2048KB), patterns=8`
- **无错时**每 5 秒打印：`psram_rapid running [60400000-60600000]: pat[N]=XXXXXXXX, pass=P, fail=0, err_words=0`
- **出错时**立即打印错误详情（每轮最多 10 条）：
  `rapid err @60401000: got AABBCCDD expect 55555155 xor FF6699E8 (pat[0]=55555555)`
  并打印轮次汇总：`rapid FAIL pat[0]=55555555: 3 words err (pass=Y, fail=Z, total_err=W)`
- **启动冲突时**（psram_soak 正在运行）：`psram_rapid: cannot start while psram_soak is running (shared region)`
- **启动失败时**（线程创建失败）：`psram_rapid: create task failed -1`
- 执行 `psram_rapid_stop` 停止，打印最终统计：`psram_rapid stopped: pass=Y, fail=Z, err_words=W`

### 测试地址

固定使用 `PSRAM_TEST_TASK` 区：0x60400000 ~ 0x60600000（2MB）。与 `psram_soak` 共用区域，**不能同时运行**。

---

## 并行测试

以下命令可以**同时运行**，因为它们使用的 PSRAM 物理地址互不重叠：

```
psram_test start cpu 0 500 0 0
psram_test_ext cpu_dma_verify
psram_test_ext stack_stress_start
psram_test_ext psram_soak_start        # 或 psram_rapid_start（二选一）
```

停止时分别执行：

```
psram_test stop
psram_test_ext cpu_dma_verify_stop
psram_test_ext stack_stress_stop
psram_test_ext psram_soak_stop         # 或 psram_rapid_stop
```

> **`psram_soak` 与 `psram_rapid` 共用 `PSRAM_TEST_TASK` 区域，不能同时运行**，请根据测试目的二选一。

---

## 注意事项

1. **PSRAM 初始化**：运行 `psram_test start cpu` 前会自动调用 `bk_psram_init()`。运行其他 `psram_test_ext` 命令前，请确保 PSRAM 已初始化（可先执行 `psram_test_ext init`）。

2. **DMA 引擎**：`cpu_dma_verify` 默认使用 HPDMA（`CONFIG_PSRAM_TEST_USE_HPDMA=1`）。HPDMA 的 debug 级别日志（`hpdma_memcpy cpy_chn`）已屏蔽，不会在高频循环中刷屏。

3. **栈溢出防护**：`stack_stress` 子任务栈为 48KB，局部数组为 2 x 4KB。若需增大局部数组，请同步增大 `STACK_STRESS_WORKER_STACK` 宏（位于 `psram_test.c`）。

4. **与 write-through 测试互斥**：`wt_start` / `wt_verify_*` 使用 `psram_malloc` 动态分配，可能与 `stack_stress` 的 PSRAM 栈分配竞争。建议不要同时运行 `wt_*` 和 `stack_stress`。

5. **地址分区依赖**：测试区地址来自 `ram_regions.h`（由 `ram_regions.csv` 生成）。若修改分区表，需重新构建以更新头文件。

6. **PSRAM interleave**：若工程开启 `CONFIG_PSRAM_INTERLEAVE=y`，地址会被映射到 0x80/0x81 空间，测试区地址也会相应变化。

7. **日志行为**：所有测试均为「出错即打印、无错每 5 秒打印状态」模式，避免大量无用日志刷屏。正常状态日志使用 `CLI_LOGD`（D 级别），错误日志使用 `CLI_LOGE`（E 级别），可通过日志级别过滤快速定位异常。

8. **psram_soak 静置时间选择**：默认 30 秒；若怀疑存在低概率 bit-flip，可设置 60 秒甚至更长。静置期间 PSRAM 区域不会被其他任务访问，数据完全依赖 PSRAM 自身刷新保持。

9. **错误日志速查**：各测试命令的错误日志关键词汇总如下，可直接在串口 log 中搜索：

| 测试命令 | 错误关键词 | 含义 |
|----------|-----------|------|
| cpu_test | `cpu_test ERR` | CPU 读写比对失败，打印错误地址、实际值、期望值、XOR |
| cpu_test | `cpu_test FAIL` | 本轮测试存在错误，打印 error_num |
| cpu_dma_verify | `DMA copy failed` | HPDMA/GDMA 搬移返回错误 |
| cpu_dma_verify | `cpu_dma_verify ERR` | DMA 搬移后目的区数据比对失败 |
| cpu_dma_verify | `cpu_dma_verify FAIL` | 本轮测试存在错误，打印 error_num |
| stack_stress | `mismatch` | worker 子任务 PSRAM 栈上数据比对失败 |
| stack_stress | `create.*failed` | 子任务创建失败（通常是 PSRAM 堆不足） |
| stack_stress | `no free slot` / `no active slot` | CD 序列与 slot 状态不一致（内部逻辑异常） |
| psram_soak | `soak bit-flip` | 静置后回读发现 bit 翻转，打印地址和 XOR |
| psram_soak | `soak FAIL` | 本轮 pattern 存在 bit-flip，打印翻转 word 数 |
| psram_rapid | `rapid err` | 快速写读后比对失败，打印地址和 XOR |
| psram_rapid | `rapid FAIL` | 本轮 pattern 存在错误，打印错误 word 数 |

---

## PSRAM 常规系统使用（非测试命令）

### 开启的 CONFIG 宏

当前工程 (`projects/app/ap/config/bk7259_ap/defconfig`) 开启了以下与 PSRAM 相关的宏：

| CONFIG 宏 | 作用 |
|-----------|------|
| `CONFIG_ALL_CODE_IN_PSRAM=y` | AP 核代码段加载到 PSRAM（`AP_PSRAM_CODE_SECTION`：0x64F00000，1MB） |
| `CONFIG_TASK_STACK_IN_PSRAM=y` | **所有** `rtos_create_thread` 创建的任务栈自动使用 PSRAM（通过 `psram_malloc` 从 `AP_PSRAM_HEAP` 分配） |
| `CONFIG_QUEUE_IN_PSRAM=y` | FreeRTOS 队列对象分配到 PSRAM |
| `CONFIG_MEMDUMP_ALL=y` | 支持全量内存 dump |
| `CONFIG_DUMP_UART_MEM_ENCODING_BASE64=y` | dump 通过 UART 以 Base64 编码输出 |

### 常规任务栈的 PSRAM 地址范围

由于 `CONFIG_TASK_STACK_IN_PSRAM=y`，`rtos_create_thread()` 内部直接调用 `rtos_create_psram_thread()`，所有常规系统任务（不只是测试命令中的任务）的 TCB 和栈都从 **`AP_PSRAM_HEAP`** 分配：

| 堆区域 | 起始地址 | 结束地址 | 大小 | PSRAM 芯片 |
|--------|----------|----------|------|-----------|
| `AP_PSRAM_HEAP` | 0x64E20000 | 0x64EC0000 | 640KB | PSRAM1 |

实际启动 log 中可以看到系统任务栈地址均落在此区间：

```
ap1: create ipc thread,    tcb=64e20f50, stack=[64e20728-64e20f28:2048]
ap0: create tcp/ip,        tcb=64e21ef0, stack=[64e216c8-64e21ec8:2048]
ap1: create wdrv_thread,   tcb=64e23628, stack=[64e22600-64e23600:4096]
ap0: create event,         tcb=64e21ef0, stack=[64e216c8-64e21ec8:...]
```

这意味着 **PSRAM1 在系统正常运行期间，始终承载着所有 AP 核任务的栈读写操作**，而不仅仅是在运行测试命令时才有 PSRAM 访问。

### AP 核 PSRAM 数据段

由于 `CONFIG_ALL_CODE_IN_PSRAM=y`，AP 核的代码和数据段也位于 PSRAM1：

| 区域 | 起始地址 | 大小 | 说明 |
|------|----------|------|------|
| `AP_PSRAM_DATA_SECTION` | 0x64EC0000 | 256KB | AP .data / .bss |
| `AP_PSRAM_CODE_SECTION` | 0x64F00000 | 1MB | AP .text（代码段） |

启动 log 验证：`copy data_dst: 0x64F00000, data_wlen: 0x00015345`（AP 代码拷贝到 PSRAM）。

---

## 测试覆盖分析

### 维度覆盖矩阵

| 测试维度 | Case1 CPU读写 | Case2 CPU+DMA | Case3 栈压力 | Case4 静置回读 | Case5 快速写读 | 系统常规运行 |
|----------|:-:|:-:|:-:|:-:|:-:|:-:|
| CPU 直接读写正确性 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| DMA 搬移正确性 | — | ✅ | — | — | — | — |
| 数据保持 / bit-flip | — | — | — | ✅ | — | — |
| 写入时序可靠性 | — | — | — | — | ✅ | — |
| 任务栈可靠性（创建/销毁） | — | — | ✅ | — | — | ✅ |
| 多 pattern 覆盖 | 单一 | 单一 | 单一 | 8种 | 8种 | — |
| 持续运行压力 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 高速总线带宽压力 | 中 | 高 | 低 | 低 | **极高** | 中 |
| 多 master 并发（CPU+DMA） | — | ✅ | — | — | — | — |

### PSRAM 芯片地址覆盖

| PSRAM 芯片 | 测试命令直接覆盖 | 系统常规使用覆盖 |
|-----------|----------------|----------------|
| **PSRAM0**（0x60000000，16MB） | 7MB（CPU 4MB + soak/rapid 2MB + DMA 1MB） | 6MB slab 区被媒体子系统间接使用 |
| **PSRAM1**（0x64000000，16MB） | stack_stress 间接覆盖 ~496KB（AP_PSRAM_HEAP） | 全部常规任务栈 + 代码段 + 数据段 + slab（约 16MB） |

### 覆盖结论

五个测试命令（Case 4/5 二选一并行）配合系统常规任务，已构成较完整的压力测试：

- **PSRAM0**：被多个测试命令以不同方式（CPU 顺序读写、DMA 搬移、pattern 静置回读 或 快速写读）持续施压
- **PSRAM1**：被系统全部任务栈 + stack_stress 高频创建/销毁持续施压
- **并发维度**：多命令 + 系统常规任务同时运行，CPU 和 DMA 同时访问 PSRAM，构成总线竞争压力
- **数据保持**（Case 4）：soak 静置 30 秒，检测刷新失败或电压波动导致的 bit-flip
- **写入时序**（Case 5）：rapid 连续写→立即读，无任何延时，检测高速写入时硬件建立/恢复时间是否充足
