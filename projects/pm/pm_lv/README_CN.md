# BK7259 PM 低压睡眠示例工程说明

* [English](./README.md)

## 1. 工程简介

`pm_lv` 是 BK7259 `pm/pm` 电源管理示例的低压（LV）睡眠版本。它演示 CP 常驻、AP 按需上下电、Tickless Idle、低压睡眠、GPIO/RTC 唤醒，以及 Wi-Fi/BLE 低功耗配置。

简单理解：

1. CP 是低功耗期间的常驻控制核，负责电源管理和 AP 上下电。
2. 工程启动时，CP 使用 `PM_BOOT_AP_MODULE_NAME_APP` 投票启动 AP。
3. 本工程使用普通 AP 冷启动；AP 掉电后再次上电会重新执行 AP 初始化和 `main()`。
4. 与 `pm/pm` 相比，`CONFIG_PM_LV_SLEEP_ENABLE=y` 使 `sys_hal_enter_low_voltage()` 直接调用 `sys_hal_enter_lv()`，CP 固定走低压睡眠路径，不再根据 bootloader 版本选择 Deep LV；唤醒后原地返回，不经过 Deep LV 的复位流程。
5. `CONFIG_DEEP_LV_DEBUG_GPIO=n` 关闭睡眠进出时的调试 GPIO 翻转，测量电流时不再有调试引脚动作。

> 仅编译本工程并不会自动得到最低功耗。实际产品还需要关闭未使用的外设，并正确管理电源票、时钟票、睡眠票和唤醒源。

## 2. 目录结构

```text
pm_lv/
├── CMakeLists.txt
├── Makefile
├── .ci                           # CI 构建命令
├── app.rst
├── README.md
├── README_CN.md
├── pj_config.mk                  # 工程级编译选项（当前为空）
├── ap/
│   ├── ap_main.c                 # AP 入口和可选 SMP/配网/IPC 测试
│   ├── CMakeLists.txt
│   └── config/bk7259_ap/
│       ├── defconfig             # AP 侧低功耗配置
│       └── usr_gpio_cfg.h        # AP 侧 GPIO 默认配置表
├── cp/
│   ├── cp_main.c                 # CP 入口，投票启动 AP
│   ├── vnd_cal.c/h               # 可选校准覆盖
│   ├── CMakeLists.txt
│   └── config/bk7259/
│       ├── defconfig             # CP 侧低功耗配置
│       └── usr_gpio_cfg.h        # CP UART0 和唤醒 GPIO 声明
└── partitions/bk7259/
    ├── auto_partitions.csv       # Flash 分区（8 MB）
    └── ram_regions.csv           # SRAM/PSRAM 分区
```

建议先阅读 `cp/cp_main.c`、`ap/ap_main.c` 和两侧的 `defconfig`。

## 3. 默认运行行为

1. CP 执行 `main()`，注册 `user_app_main()` 后调用 `bk_init()`。
2. `user_app_main()` 调用 `bk_pm_module_vote_boot_ap_ctrl()`，以 APP 模块身份投票启动 AP。
3. AP 上电后调用 `bk_init()`。
4. SMP、BLE 配网和 IPC 单元测试代码只有在对应宏（`CONFIG_FREERTOS_SMP_TEST`、`CONFIG_BK_NETWORK_PROVISIONING_BLE_EXAMPLE`、`BK_IPC_UT_TEST`）打开时才会运行，当前工程默认均未开启。

当前工程自身不创建持续运行的业务线程，也不自动发起完整睡眠测试，主要用于提供 LV 低功耗配置和二次开发骨架。

## 4. 快速上手

### 4.1 编译

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=pm/pm_lv
```

也可以使用 `.ci` 中的 Docker 构建方式：

```bash
./dbuild.sh make bk7259 PROJECT=pm/pm_lv
```

### 4.2 烧录与串口

将生成的 BK7259 固件烧录到开发板。CP CLI 使用 UART0，工程默认将：

| 功能 | GPIO | 说明 |
| --- | --- | --- |
| UART0 RX | `GPIO_10` | 上拉，同时是 SoC 默认静态唤醒源（下降沿） |
| UART0 TX | `GPIO_11` | 上拉 |
| 下降沿输入声明 | `GPIO_8`、`GPIO_17` | 上电时中断关闭，低功耗时丢弃 IO 状态 |

`cp/config/bk7259/usr_gpio_cfg.h` 中的 `GPIO_8` 和 `GPIO_17` 仅声明为可能的唤醒引脚。由于它们当前的低功耗 IO 控制为 `GPIO_LOW_POWER_DISCARD_IO_STATUS` 且中断关闭，若不修改配置表和运行时注册，无法作为系统唤醒源（见第 6 节）。

始终注册的静态唤醒源来自 `cp/middleware/soc/bk7259/soc/gpio_map.h` 中的 `GPIO_STATIC_WAKEUP_SOURCE_MAP`，其中 `GPIO_10` 配置为 `GPIO_INT_TYPE_FALLING_EDGE`。因此 UART0 控制台输入可以唤醒 CP。

### 4.3 手动控制 AP

当前 CP 配置打开了调试版 PM CLI（`CONFIG_DEBUG_VERSION=y`），可通过 UART0 使用：

```text
pm_boot_ap 9 1
pm_boot_ap 9 0
```

其中模块号 `9` 对应 `PM_BOOT_AP_MODULE_NAME_APP`，状态 `1` 表示 OFF，状态 `0` 表示 ON。由于本工程未开启 AP fast boot，重新上电后 AP 按冷启动流程运行。

如需反复做 AP 上下电压力测试：

```text
pm_boot_ap stress [start_module] [task_count] [loop_count] [on_hold_ms] [off_hold_ms]
```

## 5. 关键配置

### 5.1 CP 侧

关键配置位于 `cp/config/bk7259/defconfig`：

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_SOC_SMP=n`、`CONFIG_FREERTOS_SMP=n` | CP 使用非 SMP 模式 |
| `CONFIG_PSRAM=y`、`CONFIG_PSRAM_AS_SYS_MEMORY=n` | 启用 PSRAM，但不作为系统内存使用 |
| `CONFIG_PM_ONLY_CP_ENABLE=y` | 低功耗期间以 CP 为常驻核 |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | 启用 Tickless Idle |
| `CONFIG_PM_AP_POWERDOWN_WHEN_LV=y` | CP 进入低压睡眠时允许 AP 掉电 |
| `CONFIG_DEEP_LV=y` | 保留 Deep LV 支持代码（如唤醒后的 IRQ 屏蔽恢复等公共流程） |
| `CONFIG_PM_LV_SLEEP_ENABLE=y` | 进入低压睡眠，而不是 Deep Low Voltage |
| `CONFIG_DEEP_LV_DEBUG_GPIO=n` | 关闭 Deep LV 调试 GPIO 翻转标记 |
| `CONFIG_DEEP_LV_DEBUG_LOG=n` | 关闭 AON 快照调试日志 |
| `CONFIG_AON_RTC=y` | 启用 AON RTC |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | 启用 GPIO 唤醒 |
| `CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT=y` | 允许运行时动态注册 GPIO 唤醒源 |
| `CONFIG_TOUCH=y`、`CONFIG_TOUCH_TEST=y` | 当前工程默认启用 Touch 及测试 |
| `CONFIG_CKMN_TEST=y` | 当前工程默认启用时钟监测测试 |
| `CONFIG_OPTIMIZE_AON_CURRENT=y`、`CONFIG_AON_LDO_SEL=2` | 优化 AON 电流并选择 AON LDO 档位 |
| `CONFIG_PM_CURRENT_OPTIMIZE_XTAL_RX_TX_ANABUF=y` | 通过 XTAL RX/TX 模拟 buffer 优化电流 |
| `CONFIG_CPU_DEFAULT_FREQ_60M=y` | CP 默认频率为 60 MHz |
| `CONFIG_PM_AP_VOTE_CP_CPU_FREQ_DEFAULT=2` | AP 启动时为其投票的 CP 频率档位（2 = 80 MHz） |
| `CONFIG_PM_CP_PERI_CLK_DEFAULT_OFF=y` | 默认关闭未使用的 CP 外设时钟 |
| `CONFIG_PM_XTAL_RX_TX_ANABUF_DEFAULT_OFF=y` | 默认关闭未使用的 XTAL RX/TX 模拟 buffer |
| `CONFIG_SYS_PM_ANA_VANALDO_STEP_CTRL=y` | 睡眠期间启用 ANA/VANALDO 分级控制 |
| `CONFIG_STA_PS=y` | 启用 Wi-Fi STA 省电 |
| `CONFIG_BLUETOOTH=y`、`CONFIG_BLUETOOTH_BLE_SLAVE_ONLY=y` | 启用蓝牙，BLE 仅从机模式 |
| `CONFIG_BLE_LV_SUPPORT=y` | 启用 BLE 低压睡眠支持 |
| `CONFIG_BLUETOOTH_SUPPORT_LPO_ROSC=y` | 蓝牙支持 RC 振荡器 LPO 时钟 |
| `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL=y` | 支持 AP 完全掉电时的蓝牙协同 |
| `CONFIG_DEBUG_VERSION=y` | 保留调试 CLI 和调试编译选项 |

睡眠路径说明：

- `CONFIG_PM_LV_SLEEP_ENABLE=y` 使 `sys_hal_enter_low_voltage()` 无条件调用 `sys_hal_enter_lv()`，跳过根据 bootloader 版本选择 Deep LV 的判断。
- 仍保留 `CONFIG_DEEP_LV=y`，因为部分 Deep LV 公共代码（例如唤醒后的 IRQ 屏蔽恢复）在该宏下编译。
- 正式测量功耗前，应评估 `CONFIG_DEBUG_VERSION=y`、日志以及 `CONFIG_TOUCH_TEST=y`、`CONFIG_CKMN_TEST=y` 等测试模块对结果的影响。

### 5.2 AP 侧

关键配置位于 `ap/config/bk7259_ap/defconfig`：

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_BLUETOOTH_AP=y` | 启用 AP 侧蓝牙支持 |
| `CONFIG_AT=y`、`CONFIG_DEFAULT_AT_PORT=5` | 在端口 5 启用 AT 命令 |
| `CONFIG_WIFI_AT_ENABLE=y`、`CONFIG_MISC_AT_ENABLE=y` | 启用 Wi-Fi 和杂项 AT 命令 |
| `CONFIG_AT_MAX_PORT_CNT=1` | 允许一个 AT 端口 |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | 启用 AP Tickless Idle |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | 启用 AP GPIO 唤醒 |
| `CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT=y` | 允许 AP 运行时动态注册 GPIO 唤醒源 |
| `CONFIG_PM_AP_SUBPOWER_DOMAIN_DEFAULT_DISABLE=y` | 默认关闭 VIDEO_POST、H26E、ISP、NPU 子电源域 |
| `CONFIG_PM_AP_CPU_FRQ_DEFAUL=y` | AP 初始化后投票设置默认 CPU 频率 |
| `CONFIG_PM_AP_WFI_ENABLE=y` | 允许 AP 空闲时进入 WFI |
| `CONFIG_SLAVE_HEART_BEAT=n` | 关闭 AP heartbeat 配置 |
| `CONFIG_BLUETOOTH_HOST_ONLY=n` | AP 不编译为纯蓝牙 Host 镜像 |
| `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL=y` | 支持 AP 完全掉电时的蓝牙协同 |

`ap/config/bk7259_ap/usr_gpio_cfg.h` 中的 AP GPIO 表还为大量引脚分配了默认功能：SDIO Host（`GPIO_2`–`GPIO_4`）、I2S0（`GPIO_6`、`GPIO_7`、`GPIO_9`、`GPIO_28`）、USB0 DN（`GPIO_13`）、RGB LCD（`GPIO_14`–`GPIO_26`、`GPIO_40`–`GPIO_55`）、JPEG（`GPIO_27`、`GPIO_29`–`GPIO_39`）和 UART5（`GPIO_64`、`GPIO_65`）。`GPIO_8`、`GPIO_17` 在低功耗时保持输入。正式测流前应删除或重新配置产品未使用的功能。

业务使用被默认关闭的 AP 子电源域前，必须按对应模块流程重新上电。

### 5.3 内存布局

Flash（8 MB，`partitions/bk7259/auto_partitions.csv`）：

| 分区 | 大小 | 类型 |
| --- | --- | --- |
| `primary_bootloader` | 64 KB | code |
| `primary_cp_app` | 1428 KB | code |
| `primary_ap_app` | 1156 KB | code |
| `ota` | 1360 KB | data |
| `usr_config` | 60 KB | data |
| `easyflash`、`easyflash_ap` | 各 8 KB | data |
| `sys_rf`、`sys_net` | 各 4 KB | data |

RAM（`partitions/bk7259/ram_regions.csv`）：`CP_RAM`（0x05F000，380 KB，起始 `0x28000000`）、`SWAP`（4 KB）、`AP_HSRAM_HEAP`（512 KB，起始 `0x28100000`）、`AP_RAM`（384 KB），以及 PSRAM 区域 `PSRAM_MEM_SLAB_UNCODED`、`PSRAM_MEM_SLAB_CODED`、`CP_PSRAM_HEAP`（128 KB）、`AP_PSRAM_DATA_SECTION`（256 KB）、`AP_PSRAM_HEAP`（512 KB）、`AP_PSRAM_CACHE_HEAP`（128 KB）、`AP_PSRAM_CODE_SECTION`（1 MB）。

## 6. 添加 GPIO 唤醒

以 `GPIO_17` 下降沿作为 CP 唤醒源为例：

1. 在 `cp/config/bk7259/usr_gpio_cfg.h` 中把 `GPIO_17` 的低功耗 IO 控制改为 `GPIO_LOW_POWER_KEEP_INPUT_STATUS`，并启用中断、设置 `GPIO_INT_TYPE_FALLING_EDGE`。当前配置表使用 `GPIO_LOW_POWER_DISCARD_IO_STATUS` 且关闭中断，因此无法直接唤醒。
2. 在运行时注册该引脚并启用 PM GPIO 唤醒源：

```c
bk_gpio_register_wakeup_source(GPIO_17, GPIO_INT_TYPE_FALLING_EDGE);
bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
```

3. 按业务要求提交睡眠票，系统才能进入低压睡眠。

GPIO 配置中的触发类型必须与运行时注册参数一致，并确保睡眠前引脚未持续处于有效唤醒电平。`CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT=y` 允许最多 `CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT`（默认 4）个动态唤醒源，不再需要时可通过 `bk_gpio_unregister_wakeup_source()` 释放。

CP/AP 分工规则仍然适用：各自配置自己的 GPIO，同一个引脚不能同时被两个核配置。

## 7. 与 `pm/pm` 的区别

| 项目 | `pm/pm` | `pm/pm_lv` |
| --- | --- | --- |
| 睡眠入口 | bootloader 支持时进入 Deep LV | 始终进入 LV（`CONFIG_PM_LV_SLEEP_ENABLE=y`） |
| Deep LV 调试 GPIO | `CONFIG_DEEP_LV_DEBUG_GPIO=y` | `CONFIG_DEEP_LV_DEBUG_GPIO=n` |
| 唤醒流程 | Deep LV 经过复位/bootloader 流程唤醒 | LV 原地返回 |
| 调试手段 | 睡眠进出可在调试 GPIO 上观察 | 无调试 GPIO 动作 |
| AP 恢复方式 | 冷启动 | 冷启动 |
| AP 上下电命令 | 通用 `pm_boot_ap` | 通用 `pm_boot_ap` |

## 8. 常见问题

### 8.1 系统无法进入低功耗

检查是否仍有未释放的电源票、时钟票或睡眠票，是否有外设/DMA 持续工作，以及唤醒 GPIO 是否已处于有效电平。同时确认提交睡眠票时已注册正确的唤醒源。

### 8.2 AP 无法关闭

AP 上电由多个模块共同投票控制。只有所有相关模块都撤销 ON 票后，AP 才能真正下电。先检查 PM 投票记录和当前 AP 电源状态。

### 8.3 测得电流偏高

关闭不需要的调试日志和测试模块，并从 AP GPIO 表中删除未使用的 LCD、JPEG、I2S、SDIO、UART5 等功能。确认音频、视频、LCD、Wi-Fi、BLE、PSRAM 及外设时钟均按业务要求进入低功耗，并确认没有调试 GPIO 翻转、调试器或串口输入持续唤醒 CP。

### 8.4 GPIO 可以触发中断但不能唤醒

`usr_gpio_cfg.h` 只完成引脚声明。还需要把低功耗 IO 控制设为 `GPIO_LOW_POWER_KEEP_INPUT_STATUS`，按正确触发类型使能中断，注册 GPIO wakeup source，启用 PM GPIO 唤醒源，并提交正确的睡眠票。

### 8.5 为什么仍然保留 `CONFIG_DEEP_LV`

`CONFIG_DEEP_LV=y` 用于保留 Deep LV 公共支持代码。由于 `CONFIG_PM_LV_SLEEP_ENABLE=y` 在 `sys_hal_enter_low_voltage()` 中优先，CP 实际仍进入 LV 睡眠，不会选择 Deep LV 入口。
