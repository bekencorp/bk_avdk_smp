# BK7259 PM 低功耗示例工程说明

* [English](./README.md)

## 1. 工程简介

`pm` 是 BK7259 SDK 的基础电源管理参考工程，用于演示 CP 常驻、AP 按需上下电，以及 Tickless Idle、Deep Low Voltage、GPIO/RTC 唤醒、Wi-Fi/BLE 低功耗等配置。

简单理解：

1. CP 是低功耗期间的常驻控制核，负责电源管理和 AP 上下电。
2. 工程启动时，CP 使用 `PM_BOOT_AP_MODULE_NAME_APP` 投票启动 AP。
3. 本工程使用普通 AP 冷启动；AP 掉电后再次上电会重新执行 AP 初始化和 `main()`。
4. 它是 `pm/ap_fast_boot` 等快速恢复工程的基础版本。

> 仅编译本工程并不会自动得到最低功耗。实际产品还需要关闭未使用的外设，并正确管理电源票、时钟票、睡眠票和唤醒源。

## 2. 目录结构

```text
pm/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c                 # AP 入口和可选 SMP/配网/IPC 测试
│   ├── CMakeLists.txt
│   └── config/bk7259_ap/
│       ├── defconfig             # AP 侧低功耗配置
│       └── usr_gpio_cfg.h        # AP 侧 GPIO 默认配置
├── cp/
│   ├── cp_main.c                 # CP 入口，投票启动 AP
│   ├── vnd_cal.c/h               # 可选校准覆盖
│   ├── CMakeLists.txt
│   └── config/bk7259/
│       ├── defconfig             # CP 侧低功耗配置
│       └── usr_gpio_cfg.h        # CP UART0 和唤醒 GPIO 声明
└── partitions/bk7259/            # Flash 与 RAM 分区配置
```

建议先阅读 `cp/cp_main.c`、`ap/ap_main.c` 和两侧的 `defconfig`。

## 3. 默认运行行为

1. CP 执行 `main()`，注册 `user_app_main()` 后调用 `bk_init()`。
2. `user_app_main()` 调用 `bk_pm_module_vote_boot_ap_ctrl()`，以 APP 模块身份投票启动 AP。
3. AP 上电后调用 `bk_init()`。
4. 仅在相应宏开启时，AP 才运行 SMP、BLE 配网或 IPC 单元测试代码。

当前工程自身不创建持续运行的业务线程，也不自动发起完整睡眠测试，主要用于提供低功耗配置和二次开发骨架。

## 4. 快速上手

### 4.1 编译

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=pm/pm
```

也可以使用 `.ci` 中的 Docker 构建方式：

```bash
./dbuild.sh make bk7259 PROJECT=pm/pm
```

### 4.2 烧录与串口

将生成的 BK7259 固件烧录到开发板。CP CLI 使用 UART0，工程默认将：

| 功能 | GPIO |
| --- | --- |
| UART0 RX | `GPIO_10` |
| UART0 TX | `GPIO_11` |
| 下降沿输入声明 | `GPIO_8`、`GPIO_17` |

`GPIO_8` 和 `GPIO_17` 仅在 `usr_gpio_cfg.h` 中声明。若要作为系统唤醒源，还必须在运行时调用 GPIO 唤醒注册和 PM 唤醒源配置接口。

### 4.3 手动控制 AP

当前 CP 配置打开了调试版 PM CLI，可通过 UART0 使用：

```text
pm_boot_ap 9 1
pm_boot_ap 9 0
```

其中模块号 `9` 对应 `PM_BOOT_AP_MODULE_NAME_APP`，状态 `1` 表示 OFF，状态 `0` 表示 ON。由于本工程未开启 AP fast boot，重新上电后 AP 按冷启动流程运行。

## 5. 关键配置

### 5.1 CP 侧

关键配置位于 `cp/config/bk7259/defconfig`：

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_SOC_SMP=n`、`CONFIG_FREERTOS_SMP=n` | CP 使用非 SMP 模式 |
| `CONFIG_PM_ONLY_CP_ENABLE=y` | 低功耗期间以 CP 为常驻核 |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | 启用 Tickless Idle |
| `CONFIG_PM_AP_POWERDOWN_WHEN_LV=y` | 进入低压睡眠时允许 AP 掉电 |
| `CONFIG_DEEP_LV=y` | 启用 Deep Low Voltage 流程 |
| `CONFIG_AON_RTC=y` | 启用 AON RTC |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | 启用 GPIO 唤醒 |
| `CONFIG_TOUCH=y`、`CONFIG_TOUCH_TEST=y` | 当前工程默认启用 Touch 及测试 |
| `CONFIG_CKMN_TEST=y` | 当前工程默认启用时钟监测测试 |
| `CONFIG_CPU_DEFAULT_FREQ_60M=y` | CP 默认频率为 60 MHz |
| `CONFIG_PM_CP_PERI_CLK_DEFAULT_OFF=y` | 默认关闭未使用的 CP 外设时钟 |
| `CONFIG_STA_PS=y` | 启用 Wi-Fi STA 省电 |
| `CONFIG_BLE_LV_SUPPORT=y` | 启用 BLE 低压睡眠支持 |

当前配置还启用了 `CONFIG_DEEP_LV_DEBUG_GPIO=y` 和 `CONFIG_DEBUG_VERSION=y`。正式测量功耗时，应评估调试 GPIO、日志和测试模块对结果的影响。

### 5.2 AP 侧

关键配置位于 `ap/config/bk7259_ap/defconfig`：

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | 启用 AP Tickless Idle |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | 启用 AP GPIO 唤醒 |
| `CONFIG_PM_AP_SUBPOWER_DOMAIN_DEFAULT_DISABLE=y` | 默认关闭 VIDEO_POST、H26E、ISP、NPU 子电源域 |
| `CONFIG_PM_AP_CPU_FRQ_DEFAUL=y` | AP 初始化后投票设置默认 CPU 频率 |
| `CONFIG_SLAVE_HEART_BEAT=n` | 关闭 AP heartbeat 配置 |
| `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL=y` | 支持 AP 完全掉电时的蓝牙协同 |

业务使用被默认关闭的 AP 子电源域前，必须按对应模块流程重新上电。

## 6. 添加 GPIO 唤醒

以 `GPIO_17` 下降沿为例，除保留 `usr_gpio_cfg.h` 中的声明外，还需在业务代码中配置：

```c
bk_gpio_register_wakeup_source(GPIO_17, GPIO_INT_TYPE_FALLING_EDGE);
bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
```

然后按业务要求提交睡眠票。GPIO 配置中的触发类型必须与运行时注册参数一致，并确保睡眠前引脚未持续处于有效唤醒电平。

## 7. 与快速启动工程的区别

| 项目 | `pm/pm` | `pm/ap_fast_boot` |
| --- | --- | --- |
| AP 恢复方式 | 冷启动 | 恢复保存的 FreeRTOS 执行点 |
| `CONFIG_PM_AP_FAST_BOOT_ENABLE` | 未开启 | 开启 |
| `CONFIG_PSRAM_DATA_RETENTION_ENABLE` | 未开启 | 开启 |
| AP 上下电测试命令 | 通用 `pm_boot_ap` | 专用 `ap_fast_boot` |
| 低功耗取舍 | 无快速恢复保持开销 | PSRAM 保持会增加待机电流 |

## 8. 常见问题

### 8.1 系统无法进入低功耗

检查是否仍有未释放的电源票、时钟票或睡眠票，是否有外设/DMA 持续工作，以及唤醒 GPIO 是否已处于有效电平。

### 8.2 AP 无法关闭

AP 上电由多个模块共同投票控制。只有所有相关模块都撤销 ON 票后，AP 才能真正下电。先检查 PM 投票记录和当前 AP 电源状态。

### 8.3 测得电流偏高

关闭不需要的调试日志、测试模块和调试 GPIO，确认音频、视频、LCD、Wi-Fi、BLE、PSRAM及外设时钟均按业务要求进入低功耗，并在目标板上重新测量。

### 8.4 GPIO 可以触发中断但不能唤醒

`usr_gpio_cfg.h` 只完成引脚声明；还要注册 GPIO wakeup source、启用 PM GPIO 唤醒源，并正确提交睡眠票。
