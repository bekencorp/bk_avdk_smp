# BK7259 AP Fast Boot 示例工程说明

* [English](./README.md)

## 1. 工程简介

`ap_fast_boot` 基于 `projects/pm/pm`，用于验证 AP 掉电后恢复原有 FreeRTOS 执行点，而不是重新执行完整冷启动流程。

工程通过一个持续运行的 AP 测试线程验证：

1. AP 主函数是否只进入一次。
2. 线程局部变量和堆数据是否跨 AP OFF/ON 保持。
3. 恢复后线程序号是否继续递增。
4. CP 是否能通过投票接口控制 AP 上下电。

### 1.1 快速恢复原理

AP 掉电前，系统保存 CPU 架构上下文，将掉电的 DTCM 备份到保持供电的 AP SRAM，并使 PSRAM 数据保持有效。AP 再次上电后恢复 DTCM 和 CPU 上下文，从保存的 FreeRTOS 执行点继续运行。

当前实现只保存 CPU2 的执行上下文。冷启动时 CPU2 和 CPU3 一起启动；AP OFF 前 PM 线程会将 CPU3 hotplug offline，恢复后由 AP0 再将 CPU3 拉起。

> 快速启动保留软件执行点，但不会自动恢复所有掉电外设寄存器。业务必须停止不可跨掉电恢复的 DMA、Mailbox、中断和外设访问，并按模块要求重新初始化掉电硬件。

## 2. 目录结构

```text
ap_fast_boot/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c                 # 创建快速恢复连续性验证线程
│   ├── CMakeLists.txt
│   └── config/bk7259_ap/
│       ├── defconfig             # AP fast boot 和 PSRAM 保持配置
│       └── usr_gpio_cfg.h
├── cp/
│   ├── cp_main.c                 # AP 投票及 ap_fast_boot CLI
│   ├── vnd_cal.h
│   ├── CMakeLists.txt
│   └── config/bk7259/
│       ├── defconfig             # CP 低功耗及 fast boot 配置
│       └── usr_gpio_cfg.h
└── partitions/bk7259/            # Flash/RAM 分区配置
```

## 3. 默认运行效果

CP 启动时自动投票打开 AP，并注册以下命令：

```text
ap_fast_boot on
ap_fast_boot off
ap_fast_boot cycle
```

AP 创建 `ap_resume_test` 线程，每两秒打印一次：

```text
RESUME_PROOF seq=... stack=0x12345678 heap=... heap_value=0xa55a5aa5 main_entries=1 core=...
```

成功执行 AP OFF/ON 后，应看到：

| 字段 | 预期结果 |
| --- | --- |
| `seq` | 从掉电前的数值继续递增，不从 1 开始 |
| `stack` | 保持 `0x12345678` |
| `heap_value` | 保持 `0xA55A5AA5` |
| `main_entries` | 始终为 `1` |
| `core` | 显示恢复后测试线程所在核 |

如果保存上下文未发布或 PSRAM 保持失败，系统会清除 fast-resume 标志并回退到 AP 冷启动路径。

## 4. 快速上手

### 4.1 编译

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=pm/ap_fast_boot
```

或者使用 `.ci` 中的 Docker 构建命令：

```bash
./dbuild.sh make bk7259 PROJECT=pm/ap_fast_boot
```

### 4.2 烧录与串口

烧录生成的 BK7259 固件。测试命令通过 CP UART0 输入：

| 功能 | GPIO |
| --- | --- |
| UART0 RX | `GPIO_10` |
| UART0 TX | `GPIO_11` |

建议同时观察 AP 和 CP 日志，以确认投票结果、AP 下电和恢复过程。

### 4.3 执行一次恢复测试

最简单的方式：

```text
ap_fast_boot cycle
```

该命令先投票关闭 AP，等待 1 秒，再投票打开 AP。也可以分步执行：

```text
ap_fast_boot off
ap_fast_boot on
```

每次投票都会打印 `AP vote ON/OFF ret=...`。返回值为 `BK_OK` 只表示投票调用成功，最终是否下电还取决于其他模块是否仍持有 AP ON 票。

## 5. 启动与恢复流程

### 5.1 CP 侧

1. `main()` 注册 `user_app_main()`。
2. 调用 `bk_init()` 初始化 CP。
3. `user_app_main()` 以 APP 模块身份投票启动 AP。
4. 注册 `ap_fast_boot` CLI。
5. CLI 通过 `bk_pm_module_vote_boot_ap_ctrl()` 提交 AP ON/OFF 票。

### 5.2 AP 侧

1. 冷启动进入 `main()` 并调用 `bk_init()`。
2. `s_ap_main_entry_count` 加 1。
3. 创建栈大小为 2048 的 `ap_resume_test` 线程。
4. 线程在栈上保存 canary，并在堆上申请和写入另一个 canary。
5. 每两秒递增 `sequence` 并打印 `RESUME_PROOF`。
6. AP OFF 前保存 CPU2 上下文和 DTCM；AP ON 后恢复并继续线程循环。

## 6. 关键配置

CP 与 AP 两侧都必须同时开启：

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_PM_AP_FAST_BOOT_ENABLE=y` | 启用 AP 上下文保存、恢复和 CP 协同 |
| `CONFIG_PSRAM_DATA_RETENTION_ENABLE=y` | AP 掉电期间保持 PSRAM 数据 |

CP 侧还配置了：

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_PM_AP_POWERDOWN_WHEN_LV=y` | 低压睡眠时允许 AP 掉电 |
| `CONFIG_PM_ONLY_CP_ENABLE=y` | CP 作为常驻低功耗控制核 |
| `CONFIG_DEEP_LV=y` | 启用 Deep Low Voltage |
| `CONFIG_FREERTOS_ALLOW_OS_API_IN_IRQ_DISABLED=y` | 支持恢复流程所需的 RTOS 使用场景 |
| `CONFIG_AON_RTC=y` | 启用 AON RTC |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | 启用 GPIO 唤醒 |

AP 侧还启用了 Tickless Idle、GPIO 动态唤醒以及 AP 完全掉电时的蓝牙协同配置。

## 7. 内存保持说明

| 内存区域 | AP 掉电期间的处理 |
| --- | --- |
| AP SRAM | 保持供电 |
| AP DTCM | 备份到 AP SRAM 中 64 KiB 的 `.noinit` 缓冲区，恢复时还原 |
| PSRAM 数据 | 通过 PSRAM data retention 原位保持 |
| PSRAM 中转 | 不使用 PSRAM 暂存 AP 内存 |

PSRAM 保持会增加待机功耗。应在目标板上同时验证恢复时间、数据完整性、重复循环稳定性和低功耗电流。

## 8. 二次开发注意事项

1. CP 和 AP 两侧的 fast-boot/PSRAM retention 配置必须保持一致。
2. AP 掉电前停止或挂起不可恢复的外设访问。
3. 恢复后重新配置已掉电的外设和 AP 子电源域。
4. 不要把“线程继续运行”理解为“所有硬件状态自动保持”。
5. 修改 RAM 分区时必须保留快速恢复所需的 AP SRAM/DTCM 备份空间。
6. 若业务不需要恢复执行点，使用 `pm/pm` 冷启动方案可避免 PSRAM 保持开销。

## 9. 常见问题

### 9.1 `seq` 从 1 重新开始或 `main_entries` 增加

这表示 AP 走了冷启动路径。检查 CP/AP 两侧 fast boot 配置是否一致、PSRAM retention 是否成功，以及日志中是否出现上下文无效或恢复回退信息。

### 9.2 执行 `off` 后 AP 没有关闭

AP 电源由多模块投票共同决定。检查是否有 Wi-Fi、蓝牙、多媒体或其他模块仍持有 ON 票。

### 9.3 恢复后外设异常

快速启动只恢复 CPU/DTCM/保留内存相关的软件上下文。对掉电外设执行恢复初始化，并在 AP OFF 前清理 DMA、中断、Mailbox 和共享资源。

### 9.4 循环测试偶发失败

增加循环次数，检查 PSRAM 保持、AP SRAM、CPU3 hotplug、唤醒时序和电源稳定性；同时避免在正式功耗测试中保留过多调试日志。
