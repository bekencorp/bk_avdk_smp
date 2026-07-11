# PSRAM Data Retention（PSRAM 掉电数据保持）

> 目标：在 AP（M55）核掉电、或 CP（M52）核进入 Low-Voltage / Deep Sleep 时，把外置 PSRAM 芯片的 cell 数据保留下来，使 AP 重新上电 / CP 唤醒后能够**继续访问之前的内存内容**，而不需要重新从 flash 拷贝或从远端重传。

本文档面向 `pm_doorbell` 这种"CP 主、AP 多媒体"双核场景下的 PSRAM data retention 功能开发与维护。

---

## 一、设计目标与场景

| 场景 | 不开 retention 的行为 | 开 retention 之后 |
|---|---|---|
| `pm_boot_ap 9 1` — AP 整核掉电 | `bk_psram_deinit()`：控制器复位 + I/O pad 释放 + cell 自然失数 | `bk_psram_data_retention()`：flush 写缓冲 → snapshot 模式寄存器 → 锁存 I/O pad → 保持 LDO ON，cell **数据不丢** |
| `pm_boot_ap 9 0` — AP 重新上电 | `bk_psram_init()`：full init（chip-id 探测、cal、mode 重写），cell 内容被破坏 | `bk_psram_data_retention_recover()`：释放 pad → 控制器轻量恢复（ckg-bypass + clk + soft-reset + mode 重载），cell 内容保留 |
| `pm_vote 1 12 1 0` — CP 进 LV sleep | `sys_ll_set_ana_reg14_enpsram(0)`：把 PSRAM LDO 关掉，cell 自然失数 | 该调用被宏跳过，LDO 保持，cell 数据保留 |
| CP 进 deep sleep | 同上，PSRAM LDO 被关 | LDO 保持，cell 数据保留 |

典型使用价值：
- AP 关电期间 PSRAM1 上的 **AP heap / .data / .text 段** 不丢
- AP 关电期间 PSRAM0 上的 **视频原始帧 / 业务缓冲** 不丢
- CP 进 LV 期间不需要把 PSRAM 上的状态重新初始化

---

## 二、整体路径与时序

PSRAM retention 是 **CP 单核驱动**的能力，AP 侧只是"被关电 / 被上电"的对象，本身不参与控制。

```
┌────────────────── CP 侧（M52）─────────────────┐         ┌─── AP（M55）───┐
│                                                │         │                │
│  pm_boot_ap 9 1                                │         │                │
│       │                                        │         │                │
│       ▼                                        │         │                │
│  bk_pm_module_vote_boot_ap_ctrl(OFF)           │         │                │
│       │                                        │         │                │
│       ▼                                        │         │                │
│  bk_pm_module_vote_psram_ctrl(OFF)             │         │                │
│       │                                        │         │                │
│       ├─ [probe enable?] probe_write(seq++)    │         │                │
│       ▼                                        │         │                │
│  bk_psram_data_retention()                     │         │                │
│       │  (flush, snapshot mode, latch pads)    │         │                │
│       ▼                                        │         │                │
│  pm_module_shutdown_cpu1():                    │         │                │
│       ├─ pm_ap_first_boot_set(false)           │         │   核掉电      │
│       └─ M55 LDO/reset/iso 控制                │   ──►   │   PSRAM cells │
│                                                │         │   仍带电      │
│       ── 时间过去 ──                           │         │                │
│                                                │         │                │
│  pm_boot_ap 9 0                                │         │                │
│       │                                        │         │                │
│       ▼                                        │         │                │
│  bk_pm_module_vote_psram_ctrl(ON)              │         │                │
│       │                                        │         │                │
│       ├─ is_first_boot = bk_pm_ap_first_boot_get()       │                │
│       │                                        │         │                │
│       ├─ first_boot=true  -> bk_psram_init()   │         │                │
│       └─ first_boot=false -> bk_psram_data_retention_recover()
│                              │ if 失败 -> fallback bk_psram_init()        │
│       │                                        │         │                │
│       ├─ [probe enable && !first_boot] probe_verify()    │                │
│       ▼                                        │         │                │
│  bk_pm_module_vote_boot_ap_ctrl(ON)            │   ──►   │   AP 上电跑   │
│                                                │         │                │
└────────────────────────────────────────────────┘         └────────────────┘
```

关键不变量：

- `pm_ap_work_state.first_boot` 在 `reset_reason.c` 中根据 `RESET_SOURCE_POWERON` 初始化为 `true`，第一次 AP 关电后被 `pm_module_shutdown_cpu1()` 清成 `false`。后续每次 AP 上电都看到 `false`，自动走 recover 路径。
- `s_psram_retention_active[PSRAM_ID]` 在 retention 时置 `true`，recover 完成后清 `false`。无 active flag 时 recover 会 `LOGW` 并 fallback 到 `bk_psram_init()`。
- mode 寄存器值是**动态 snapshot** 的（`psram_ll_get_mode_value()`），不是硬编码。`PSRAM_MODE9` 仅作为找不到 snapshot 时的兜底常量。

---

## 三、涉及的代码与文件

按"接口层 / 驱动层 / 服务层 / 配置层"四层组织。

### 3.1 接口层

| 文件 | 用途 |
|---|---|
| `cp/include/driver/psram.h` | 公开 `bk_psram_data_retention()` / `bk_psram_data_retention_recover()` 两个驱动 API |
| `cp/include/modules/pm.h` | 公开 `bool bk_pm_ap_first_boot_get(void)` |

### 3.2 驱动层

| 文件 | 内容 |
|---|---|
| `cp/middleware/driver/psram/psram_driver.c` | retention/recover 的真正实现：<br>- `s_psram_retention_active[]` / `s_psram_retention_saved_mode[]` 静态状态<br>- `psram_retention_flush()` / `psram_m55pwd_save_mode()`<br>- `bk_psram_data_retention()`：iterate PSRAM0/1，已 init 的 bank 才做 retention<br>- `bk_psram_data_retention_recover()`：iterate active bank，做 ckg-bypass + clk-sel + soft-reset + mode 重载 + pad 释放 |
| `cp/middleware/soc/bk7259/hal/sys_pm_hal.c` | LV/deep-sleep 路径下用 `CONFIG_PSRAM_DATA_RETENTION_ENABLE` 跳过 PSRAM LDO 关电（行 989 / 行 1428） |

### 3.3 服务层

| 文件 | 内容 |
|---|---|
| `cp/components/bk_pm/src/services/bk_pm_psram.c` | PM 框架挂钩：<br>- `bk_pm_module_vote_psram_ctrl()` 在 vote-ON 时根据 first-boot 选 init 或 recover；vote-OFF 时调 retention<br>- retention probe 写入 / 校验（可选）<br>- 异常 fallback：recover 失败 → init；retention 失败 → deinit |
| `cp/components/bk_pm/src/services/bk_pm_ap_ctrl.c` | `bk_pm_ap_first_boot_get()` 从共享内存读 `pm_ap_work_state.first_boot`，带 cache invalidate |

### 3.4 配置层

| 文件 | 内容 |
|---|---|
| `cp/middleware/driver/pwr_clk/Kconfig` | 定义 `CONFIG_PSRAM_DATA_RETENTION_ENABLE`（CP 侧主控） |
| `ap/middleware/driver/pwr_clk/Kconfig` | 镜像同名 CONFIG（保持对称，AP 侧不直接使用但允许工程统一定义） |
| `projects/pm/pm_doorbell/cp/config/bk7259/defconfig` | `CONFIG_PSRAM_DATA_RETENTION_ENABLE=y` |
| `projects/pm/pm_doorbell/partitions/bk7259/ram_regions.csv` | 预留 `PSRAM0_RETENTION_PROBE` / `PSRAM1_RETENTION_PROBE` 两个 4 KB 专属区段（供 probe 使用） |

---

## 四、CONFIG / 宏开关一览

### 4.1 Kconfig 级别（`menuconfig` 可改）

| 宏 | 默认 | 作用 |
|---|---|---|
| `CONFIG_PSRAM_DATA_RETENTION_ENABLE` | `n` | retention 主开关。开启后：<br>① CP LV / deep sleep 不关 PSRAM LDO（`sys_pm_hal.c`）<br>② vote-ON 走 `bk_psram_data_retention_recover()`（非首启时）<br>③ vote-OFF 走 `bk_psram_data_retention()`<br>关闭后：完全回退到改造前的 `bk_psram_init/deinit` 路径 |
| `CONFIG_PSRAM_POWER_DOMAIN_LV_DISABLE` | `n` | 是否允许 LV sleep 关 PSRAM LDO。与 retention 联动判断（见 `sys_pm_hal.c:1428`） |

### 4.2 文件级本地宏（`bk_pm_psram.c` 内）

| 宏 | 默认 | 作用 |
|---|---|---|
| `PM_PSRAM_RETENTION_PROBE_ENABLE` | `0` | 是否编译 probe 写入 / 校验代码。生产默认 OFF（零运行开销），需要诊断 retention 时本地改 1 重编 |
| `PM_PSRAM_RETENTION_PROBE_WORDS` | `256`（即 1024 B） | 单个 bank 的 probe 块大小，单位 word（4 B） |
| `PM_PSRAM0_RETENTION_PROBE_ADDR` | `CONFIG_PSRAM0_RETENTION_PROBE_ADDR` 优先；否则 `0x60FFFC00`（slab 末尾 1 KB 兜底） | PSRAM0 探针物理地址 |
| `PM_PSRAM1_RETENTION_PROBE_ADDR` | `CONFIG_PSRAM1_RETENTION_PROBE_ADDR` 优先；否则 `0x64D7FC00`（slab 末尾 1 KB 兜底） | PSRAM1 探针物理地址 |

### 4.3 分区表生成的 CONFIG（来自 `ram_regions.csv`）

| CONFIG | 工程中的取值 |
|---|---|
| `CONFIG_PSRAM0_RETENTION_PROBE_ADDR` | `0x60FFF000` |
| `CONFIG_PSRAM0_RETENTION_PROBE_SIZE` | `0x00001000` (4 KB) |
| `CONFIG_PSRAM1_RETENTION_PROBE_ADDR` | `0x64D7F000` |
| `CONFIG_PSRAM1_RETENTION_PROBE_SIZE` | `0x00001000` (4 KB) |

`bk_pm_psram.c` 通过 `#include "ram_regions.h"` 自动拿到这些 CONFIG。配合 `_Static_assert` 编译期校验"probe 块不超过分区大小"。

---

## 五、分区表的预留约定

`projects/pm/pm_doorbell/partitions/bk7259/ram_regions.csv`：

```
PSRAM_MEM_SLAB_UNCODED, PSRAM, 0x60000000, 0xFFF000   # 16MB - 4KB
PSRAM0_RETENTION_PROBE, PSRAM,           , 0x001000   # 0x60FFF000, 4KB
PSRAM_MEM_SLAB_CODED,   PSRAM, 0x64000000, 0xD7F000   # 13.5MB - 4KB
PSRAM1_RETENTION_PROBE, PSRAM,           , 0x001000   # 0x64D7F000, 4KB
CP_PSRAM_HEAP,          PSRAM,           , 0x020000
AP_PSRAM_HEAP,          PSRAM,           , 0x0a0000
AP_PSRAM_DATA_SECTION,  PSRAM,           , 0x040000
AP_PSRAM_CODE_SECTION,  PSRAM,           , 0x180000
```

为什么这么划：

- 探针挂在两个 slab 池的末尾各 4 KB，物理上跟业务 slab/heap/数据/代码段**完全隔离**，无论业务跑成什么样都不会撞 probe。
- 4 KB 比 probe 实际用的 1 KB 大 3 KB，留余量将来扩 pattern 或加多 bank 探针。
- 其它项目要复用：把这两行 CSV 同步过去即可，不动驱动代码。

---

## 六、Probe（探针）机制

只有 `PM_PSRAM_RETENTION_PROBE_ENABLE=1` 时才会编译。开发期建议打开，做完测试再关。

### 6.1 探针布局

每个 bank 256 个 word（1024 B），按 `(idx, seq, bank)` 三元组生成确定性 pattern：

| word index | 内容 |
|---|---|
| 0 | `MAGIC_HEAD = 0xA5A5C3C3` |
| 1 | `seq`（每次 retention 自增） |
| 2 | `bank id`（0 = PSRAM0, 1 = PSRAM1） |
| 3 | `PAT = 0xDEADBEEF` |
| 4 | `~PAT` |
| 5 ~ N-4 | **多 pattern body**，`probe_word_pattern(i, seq, bank)`，按 `i & 7` 切 8 种模式：XOR、补码、mul-add、ROTL1、ROTR1、`base^(base<<16)`、`base+0xFEEDFACE` |
| N-3 | walking-1 = `1 << (seq & 31)` |
| N-2 | `checksum`（xor of words 0..N-3） |
| N-1 | `MAGIC_TAIL = 0xC3C3A5A5` |

`base = (bank<<28) | (seq<<16) | idx`，使得同一地址不同 seq / 不同 bank 期望值都不同，能区分"地址固定残留"与"真正保留"。

### 6.2 校验输出

PASS（每个 bank 一行）：

```
pm_psram:I(....):retention_probe verify PASS bank0 @0x60FFF000 seq=2 words=256
pm_psram:I(....):retention_probe verify PASS bank1 @0x64D7F000 seq=2 words=256
```

FAIL（带细节）：

```
pm_psram:E(....):retention_probe verify FAIL bank<N> @<addr> seq_exp=<S>
                  head=0/1 seq=0/1 bank=0/1 pat=0/1 walk1=0/1 tail=0/1 cks=0/1 mismatch=<M>
pm_psram:E(....):retention_probe first_bad bank<N> idx=<I> exp=0x<E> got=0x<G>
```

七个布尔位指示哪个校验项失败，`mismatch` 是 body 区出错 word 总数，`first_bad` 给出第一个不匹配的 word（用于定位故障 cell）。

### 6.3 调用时机

- **写入**：`bk_pm_module_vote_psram_ctrl(OFF)` 在调 `bk_psram_data_retention()` **之前**写入。
- **校验**：`bk_pm_module_vote_psram_ctrl(ON)` 在 recover 成功（`ret == BK_OK`）且**非首次启动**时调。首次启动跳过（PSRAM 含垃圾，校验必失败无意义）。
- 首次启动会在 vote-OFF 路径也写一次 probe，给下次 vote-ON 校验用。

---

## 七、异常路径与 Fallback

| 失败点 | 处理 |
|---|---|
| `bk_psram_data_retention_recover()` 返回非 OK | LOGE → **fallback `bk_psram_init()`**（控制器至少能用，AP 还能起；cell 数据放弃） |
| `bk_psram_data_retention()` 返回非 OK | LOGE → **fallback `bk_psram_deinit()`**（避免控制器残留半 latched 状态污染下次 init；cell 数据放弃） |
| `bk_psram_init()`（首启 or fallback）返回非 OK | 当前只 LOGE，原始注释中"3 次重试 + 强制 reboot"逻辑保留在注释里待启用 |
| `_recover()` 时发现 `s_psram_retention_active[*]==false`（比如 retention 没执行过） | LOGW → fallback `bk_psram_init()` |
| probe verify 失败 | 只 LOGE，不阻塞主流程（probe 仅为诊断手段，不影响业务） |
| probe 块大小超过分区 | **编译期** `_Static_assert` 直接报错，构建失败 |

---

## 八、测试方案

### 方案 A：AP 关电 + AP 上电（最常用）

```
pm_boot_ap 9 1      # AP 关电，触发 retention
pm_boot_ap 9 0      # AP 上电，触发 recover + probe verify
```

期望 log：

```
pm_psram:I(...):retention_probe write bank0 @0x60FFF000 seq=1 words=256
pm_psram:I(...):retention_probe write bank1 @0x64D7F000 seq=1 words=256
psram:I(...):psram_data_retention: pads latched, p0=1 p1=1, mode0=0xbc0f4049 mode1=0xbc0f4049
... ap power off ...
... 一段时间 ...
pm_psram:I(...):vote_on: not first boot -> bk_psram_data_retention_recover()
psram:I(...):psram_data_retention_recover: done
pm_psram:I(...):retention_probe verify PASS bank0 @0x60FFF000 seq=1 words=256
pm_psram:I(...):retention_probe verify PASS bank1 @0x64D7F000 seq=1 words=256
... ap system started ...
bk_init:D(...):First Boot: 0    # AP 收到 first_boot=0
M55 main running...
```

### 方案 B：AP 关电 + CP LV sleep + 唤醒 + AP 上电

```
pm_boot_ap 9 1            # AP 关电，retention
pm_vote 1 12 1 0          # CP 投 APP vote 进 LV
# CP 真正进 LV；用 UART/GPIO/RTC 把 CP 拉起来
pm_vote 1 12 0 0          # 撤 APP vote
pm_boot_ap 9 0            # AP 上电，recover + probe verify
```

注意事项见**第九节**。

### 方案 C：关掉 retention 验证回归

```
# 把 defconfig 里 CONFIG_PSRAM_DATA_RETENTION_ENABLE 改成 n，重编
```

期望：所有路径走 `bk_psram_init()` / `bk_psram_deinit()`，行为与改造前一致。

---

## 九、已知限制 / 排错指南

### 9.1 LV sleep 无 RTC 自动唤醒

`bk_pm_module_vote_sleep_ctrl(APP, 1, sleep_time)` 中第 3 个参数 **对 `PM_SLEEP_MODULE_NAME_APP=12` 不生效**（只 `PM_SLEEP_MODULE_NAME_BTSP=8` 会消费 sleep_time）。所以：

```
pm_vote 1 12 1 5000       # 第 4 个 5000 被无视
```

CP 进 LV 后没有任何自带唤醒源，约 12~13 秒后会被 `AON_WDT` 复位。这是 **PM 框架的固有行为，跟 PSRAM retention 无关**。要做无人值守的 LV 自动唤醒测试，必须额外配置 RTC tick / GPIO wakeup / BLE wakeup。

### 9.2 复位后的 first-boot 标志

任何硬复位（WDT、外部 reset）会清掉 CP RAM 中的 `pm_ap_work_state`，导致 `first_boot` 重新变 `true`。复位后下一次 AP 上电走 `bk_psram_init()`，**probe 内容被 init 流程覆盖，无法用来判断 retention 是否在 LV 期间生效**。

要验证 LV 期间 PSRAM 是否保住，必须让 CP "从 LV 干净醒来"，不被 WDT 重置。

### 9.3 PSRAM 容量 vs `CONFIG_PSRAM_CAPACITY`

硬件实际是 16 MB / chip，但 `CONFIG_PSRAM_CAPACITY = 0x00800000` (8 MB) 是 SDK 缺省值。`bk_psram_init` 会打印：

```
psram:W(...):psram type(16MB) not match CONFIG_PSRAM_CAPACITY 0X00800000
```

属于已知告警，不影响 retention。

### 9.4 验证 probe 通过 ≠ 整个 PSRAM 全保住

probe 是 cell 级抽检：每 bank 1 KB / 16 MB ≈ 0.006%。但 retention 是 cell 全局物理行为（LDO + 自刷新），所以 1 KB 通过基本能代表整块 16 MB 通过。如果你想更全面，可以：

- 增大 `PM_PSRAM_RETENTION_PROBE_WORDS`（最多到 1024 = 4 KB，分区上限）
- 加多个 probe 地址（修改 `s_pm_psram_retention_probe_base[]`）

### 9.5 移植到其它项目

最小步骤：

1. 把 `ram_regions.csv` 里的 `PSRAM0_RETENTION_PROBE` / `PSRAM1_RETENTION_PROBE` 两行同步过去。
2. 在 cp `defconfig` 加 `CONFIG_PSRAM_DATA_RETENTION_ENABLE=y`。
3. （可选）开发期把 `PM_PSRAM_RETENTION_PROBE_ENABLE` 临时改 1 验证。

不需要改 retention 驱动 / 服务层代码。

---

## 十、相关文档

| 文档 | 内容 |
|---|---|
| `cp/components/bk_pm/README.md` | PM 模块整体架构 |
| `cp/components/bk_pm/PM_SLEEP_CALLBACK_README.md` | sleep enter/exit 回调机制（业务模块挂钩低功耗的标准方式） |
| `docs/bk7259/zh_CN/developer-guide/architecture/index.rst` "PM 与低功耗" 一节 | 双核 PM 协同的官方架构说明 |
| `projects/ap_powerdown_keepalive/README_CN.md` | AP 下电保活示例 |

---

## 附录：关键 API 速查

```c
/* cp/include/driver/psram.h */
bk_err_t bk_psram_data_retention(void);
bk_err_t bk_psram_data_retention_recover(void);

/* cp/include/modules/pm.h */
bool bk_pm_ap_first_boot_get(void);

/* cp/components/bk_pm/src/services/bk_pm_psram.c (内部) */
static bk_err_t bk_pm_psram_init_and_check(void);
bk_err_t bk_pm_module_vote_psram_ctrl(pm_power_psram_module_name_e module,
                                      pm_power_module_state_e power_state);
```
