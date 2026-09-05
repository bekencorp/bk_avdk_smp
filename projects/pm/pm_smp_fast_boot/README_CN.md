# BK7259 PM SMP Fast Boot 示例工程说明

* [English](./README.md)

## 1. 工程简介

`pm_smp_fast_boot` 是 BK7259 AP 快速恢复验证工程，用于测试 AP 多核启动拓扑下的 FreeRTOS 执行点保持、CPU3 hotplug 和 AP OFF/ON 恢复。

当前 release 4.0.1 中，本工程的 `ap_main.c`、`cp_main.c`、AP/CP `defconfig`、分区和构建配置与 `projects/pm/ap_fast_boot` 实际一致。因此当前两者的运行命令和验证结果也一致。

需要注意：

1. 工程名中的 `smp` 指向 AP 快速恢复涉及的 CPU2/CPU3 多核场景。
2. CP 侧明确配置 `CONFIG_SOC_SMP=n` 和 `CONFIG_FREERTOS_SMP=n`，CP 自身以非 SMP 模式运行。
3. 快速恢复只保留 CPU2 的执行上下文；CPU3 在 AP OFF 前下线，恢复后重新上线。
4. `app.rst` 中的构建命令仍写成 `PROJECT=pm/ap_fast_boot`，编译本工程时应使用 `PROJECT=pm/pm_smp_fast_boot`。

## 2. 工作原理

AP 掉电前，PM 流程保存 CPU2 架构上下文，并将 DTCM 备份到保持供电的 AP SRAM。PSRAM 通过 data retention 原位保持数据。AP 再次上电后恢复 DTCM 和 CPU2 上下文，从原有 FreeRTOS 执行点继续运行。

冷启动时 CPU2 和 CPU3 一起启动，`CONFIG_CPU_HOTPLUG_BOOT_OFFLINE` 保持关闭。AP OFF 前 PM 线程将 CPU3 hotplug offline；快速恢复后 AP0 再将 CPU3 拉起。

> CPU 执行点恢复不等于所有外设硬件状态恢复。DMA、Mailbox、中断、共享资源和掉电子电源域仍需由业务正确处理。

## 3. 目录结构

```text
pm_smp_fast_boot/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c                 # CPU2 线程连续性验证
│   ├── CMakeLists.txt
│   └── config/bk7259_ap/
│       ├── defconfig             # AP fast boot/PSRAM retention
│       └── usr_gpio_cfg.h
├── cp/
│   ├── cp_main.c                 # AP 投票及测试 CLI
│   ├── vnd_cal.h
│   ├── CMakeLists.txt
│   └── config/bk7259/
│       ├── defconfig             # CP 非 SMP 低功耗配置
│       └── usr_gpio_cfg.h
└── partitions/bk7259/            # Flash/RAM 分区
```

## 4. 快速上手

### 4.1 编译

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=pm/pm_smp_fast_boot
```

Docker/CI 构建命令：

```bash
./dbuild.sh make bk7259 PROJECT=pm/pm_smp_fast_boot
```

### 4.2 烧录与串口

烧录生成的 BK7259 固件。CP CLI 默认使用 UART0：

| 功能 | GPIO |
| --- | --- |
| UART0 RX | `GPIO_10` |
| UART0 TX | `GPIO_11` |

### 4.3 执行测试

CP 启动时自动投票打开 AP，并注册：

```text
ap_fast_boot on
ap_fast_boot off
ap_fast_boot cycle
```

执行一次自动 OFF/ON：

```text
ap_fast_boot cycle
```

该命令关闭 AP，等待 1 秒后重新打开。也可以分步执行 `off` 和 `on`。

## 5. 预期日志

AP 的 `ap_resume_test` 线程每两秒打印：

```text
RESUME_PROOF seq=... stack=0x12345678 heap=... heap_value=0xa55a5aa5 main_entries=1 core=...
```

成功恢复后：

| 检查项 | 预期结果 |
| --- | --- |
| `seq` | 持续递增 |
| `stack` | `0x12345678` |
| `heap_value` | `0xA55A5AA5` |
| `main_entries` | 保持为 `1` |
| CPU3 | OFF 前 offline，恢复后 online |

如果 `seq` 重新从 1 开始或 `main_entries` 增加，说明 AP 走了冷启动回退路径。

## 6. 启动与恢复流程

### 6.1 CP 侧

1. 注册 `user_app_main()` 并调用 `bk_init()`。
2. 以 `PM_BOOT_AP_MODULE_NAME_APP` 身份投票启动 AP。
3. 注册 `ap_fast_boot` CLI。
4. CLI 使用 `bk_pm_module_vote_boot_ap_ctrl()` 控制 AP 票。
5. AP 下电前与 AP fast-boot 流程协同，重新上电时选择恢复或冷启动。

### 6.2 AP 侧

1. 冷启动时执行 `bk_init()`，记录 `main()` 进入次数。
2. 创建栈大小为 2048 的测试线程。
3. 在线程栈和堆中分别写入 canary。
4. AP OFF 前 CPU3 下线，保存 CPU2 上下文和 DTCM。
5. AP ON 后恢复 CPU2 执行点，再将 CPU3 上线。
6. 测试线程从原循环继续打印。

## 7. 关键配置

### 7.1 AP/CP 共用快速恢复配置

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_PM_AP_FAST_BOOT_ENABLE=y` | 启用 AP 快速恢复 |
| `CONFIG_PSRAM_DATA_RETENTION_ENABLE=y` | AP 掉电期间保持 PSRAM 数据 |

### 7.2 CP 侧低功耗配置

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_SOC_SMP=n` | CP 不使用 SMP |
| `CONFIG_FREERTOS_SMP=n` | CP 使用 FreeRTOS 非 SMP 模式 |
| `CONFIG_PM_ONLY_CP_ENABLE=y` | 低功耗期间 CP 常驻 |
| `CONFIG_PM_AP_POWERDOWN_WHEN_LV=y` | 低压睡眠时允许 AP 掉电 |
| `CONFIG_DEEP_LV=y` | 启用 Deep Low Voltage |
| `CONFIG_FREERTOS_ALLOW_OS_API_IN_IRQ_DISABLED=y` | 支持恢复流程所需的 OS API 场景 |
| `CONFIG_AON_RTC=y` | 启用 AON RTC |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | 启用 GPIO 唤醒 |

### 7.3 AP 侧配置

AP 侧开启 Tickless Idle、GPIO 动态唤醒、PSRAM data retention、AP fast boot，以及 AP 完全掉电场景下的蓝牙协同。

## 8. 内存与多核注意事项

| 对象 | 处理方式 |
| --- | --- |
| CPU2 | 保存并恢复执行上下文 |
| CPU3 | AP OFF 前 hotplug offline，恢复后 online |
| AP SRAM | 保持供电 |
| AP DTCM | 备份到 AP SRAM 中 64 KiB `.noinit` 缓冲区 |
| PSRAM | 数据原位保持，不用于中转 AP 内存 |

修改任务绑核、CPU hotplug 流程或 RAM 分区后，需要重新验证反复 OFF/ON、线程连续性和内存完整性。

## 9. 常见问题

### 9.1 为什么 CP 配置是非 SMP

本工程的 SMP 场景在 AP 侧。CP 作为常驻电源管理核，当前 `defconfig` 明确使用非 SMP 模式。

### 9.2 为什么构建后行为与 `ap_fast_boot` 相同

当前版本中两个工程的核心源码和配置相同，这是工程实际内容决定的，不是文档遗漏。

### 9.3 AP 执行 `off` 后没有关闭

检查其他模块是否仍持有 AP ON 票。CLI 返回成功不代表所有模块的票都已释放。

### 9.4 恢复后外设或 CPU3 异常

检查 CPU3 hotplug 时序、共享中断和锁状态；同时确保 AP OFF 前停止 DMA/Mailbox/外设访问，并在恢复后重新初始化掉电模块。

### 9.5 低功耗电流偏高

PSRAM retention 会增加保持电流。关闭不必要的日志和测试功能，并在目标板上比较快速恢复收益与待机功耗代价。
