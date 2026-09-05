# BK7259 PM SMP Fast Boot Example

* [中文](./README_CN.md)

## 1. Overview

`pm_smp_fast_boot` verifies AP fast resume with the AP multicore startup topology, including FreeRTOS execution-point retention, CPU3 hotplug, and AP OFF/ON recovery.

In release 4.0.1, this project's `ap_main.c`, `cp_main.c`, AP/CP `defconfig` files, partitions, and build configuration are effectively the same as those in `projects/pm/ap_fast_boot`. Their commands and expected runtime results are therefore also the same.

Important details:

1. `smp` in the directory name refers to the AP CPU2/CPU3 fast-resume scenario.
2. The CP explicitly sets `CONFIG_SOC_SMP=n` and `CONFIG_FREERTOS_SMP=n`, so the CP itself runs in non-SMP mode.
3. Only the CPU2 execution context is retained. CPU3 is taken offline before AP OFF and brought online after resume.
4. `app.rst` still shows `PROJECT=pm/ap_fast_boot`; use `PROJECT=pm/pm_smp_fast_boot` to build this directory.

## 2. How It Works

Before AP power-down, the PM flow saves the CPU2 architectural context and copies DTCM into retained AP SRAM. PSRAM data is retained in place. When the AP is powered up again, DTCM and CPU2 context are restored and FreeRTOS continues at the saved execution point.

CPU2 and CPU3 start together on cold boot because `CONFIG_CPU_HOTPLUG_BOOT_OFFLINE` remains disabled. Before AP OFF, the PM task hotplugs CPU3 offline. After fast resume, AP0 brings CPU3 online again.

> Restoring a CPU execution point does not restore every peripheral hardware state. The application must handle DMA, mailbox, interrupts, shared resources, and power-gated AP domains correctly.

## 3. Directory Structure

```text
pm_smp_fast_boot/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c                 # CPU2 task-continuity verification
│   ├── CMakeLists.txt
│   └── config/bk7259_ap/
│       ├── defconfig             # AP fast boot and PSRAM retention
│       └── usr_gpio_cfg.h
├── cp/
│   ├── cp_main.c                 # AP voting and test CLI
│   ├── vnd_cal.h
│   ├── CMakeLists.txt
│   └── config/bk7259/
│       ├── defconfig             # Non-SMP CP low-power configuration
│       └── usr_gpio_cfg.h
└── partitions/bk7259/            # Flash and RAM layouts
```

## 4. Quick Start

### 4.1 Build

Run from the SDK root:

```bash
make bk7259 PROJECT=pm/pm_smp_fast_boot
```

Docker/CI build:

```bash
./dbuild.sh make bk7259 PROJECT=pm/pm_smp_fast_boot
```

### 4.2 Flash And Serial Console

Flash the generated BK7259 firmware. The CP CLI uses UART0:

| Function | GPIO |
| --- | --- |
| UART0 RX | `GPIO_10` |
| UART0 TX | `GPIO_11` |

### 4.3 Run The Test

At startup, the CP votes the AP ON and registers:

```text
ap_fast_boot on
ap_fast_boot off
ap_fast_boot cycle
```

Run one automatic OFF/ON cycle:

```text
ap_fast_boot cycle
```

The command powers the AP off, waits one second, and powers it on. You can also run `off` and `on` separately.

## 5. Expected Log

The AP `ap_resume_test` task prints every two seconds:

```text
RESUME_PROOF seq=... stack=0x12345678 heap=... heap_value=0xa55a5aa5 main_entries=1 core=...
```

After a successful resume:

| Check | Expected Result |
| --- | --- |
| `seq` | Continues increasing |
| `stack` | Remains `0x12345678` |
| `heap_value` | Remains `0xA55A5AA5` |
| `main_entries` | Remains `1` |
| CPU3 | Offline before AP OFF, online after resume |

If `seq` restarts at 1 or `main_entries` increases, the AP followed the cold-boot fallback path.

## 6. Startup And Resume Flow

### 6.1 CP Side

1. Register `user_app_main()` and call `bk_init()`.
2. Submit the `PM_BOOT_AP_MODULE_NAME_APP` vote to start the AP.
3. Register the `ap_fast_boot` CLI.
4. Use `bk_pm_module_vote_boot_ap_ctrl()` for AP ON/OFF votes.
5. Coordinate the fast-boot flow and select resume or cold boot when the AP powers up.

### 6.2 AP Side

1. On cold boot, call `bk_init()` and count entries into `main()`.
2. Create the test task with a 2048-byte stack.
3. Store canaries on the task stack and heap.
4. Before AP OFF, take CPU3 offline and save CPU2 context and DTCM.
5. After AP ON, restore the CPU2 execution point and bring CPU3 online.
6. Continue the existing test-task loop.

## 7. Key Configuration

### 7.1 Fast-Resume Options On AP And CP

| Option | Description |
| --- | --- |
| `CONFIG_PM_AP_FAST_BOOT_ENABLE=y` | Enable AP fast resume |
| `CONFIG_PSRAM_DATA_RETENTION_ENABLE=y` | Retain PSRAM data while the AP is off |

### 7.2 CP Low-Power Options

| Option | Description |
| --- | --- |
| `CONFIG_SOC_SMP=n` | Disable SMP on the CP |
| `CONFIG_FREERTOS_SMP=n` | Run CP FreeRTOS in non-SMP mode |
| `CONFIG_PM_ONLY_CP_ENABLE=y` | Keep the CP as the resident low-power controller |
| `CONFIG_PM_AP_POWERDOWN_WHEN_LV=y` | Allow AP power-down in low-voltage sleep |
| `CONFIG_DEEP_LV=y` | Enable Deep Low Voltage |
| `CONFIG_FREERTOS_ALLOW_OS_API_IN_IRQ_DISABLED=y` | Support OS API usage required by resume |
| `CONFIG_AON_RTC=y` | Enable the AON RTC |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | Enable GPIO wakeup |

### 7.3 AP Options

The AP enables Tickless Idle, dynamic GPIO wakeup, PSRAM data retention, AP fast boot, and Bluetooth coordination while the AP is fully powered down.

## 8. Memory And Multicore Notes

| Object | Handling |
| --- | --- |
| CPU2 | Save and restore its execution context |
| CPU3 | Hotplug offline before AP OFF and online after resume |
| AP SRAM | Remains powered |
| AP DTCM | Backed up to a 64-KiB `.noinit` buffer in retained AP SRAM |
| PSRAM | Retained in place; not used to stage AP memory |

After changing task affinity, CPU hotplug behavior, or RAM partitions, rerun repeated OFF/ON, task-continuity, and memory-integrity tests.

## 9. FAQ

### 9.1 Why Is The CP Configured As Non-SMP?

The multicore scenario is on the AP side. The CP remains the resident power-management core and explicitly runs in non-SMP mode in the current `defconfig`.

### 9.2 Why Does It Behave Like `ap_fast_boot`?

The two projects have the same core source and configuration in the current release. This behavior follows the actual project contents.

### 9.3 The AP Does Not Turn Off

Check whether another module still holds an AP ON vote. A successful CLI return does not mean that all module votes have been released.

### 9.4 A Peripheral Or CPU3 Fails After Resume

Inspect CPU3 hotplug timing, shared interrupts, and lock state. Stop DMA, mailbox, and peripheral activity before AP OFF, then reinitialize powered-down modules after resume.

### 9.5 Low-Power Current Is Too High

PSRAM retention increases retention current. Disable unnecessary logs and tests, then compare fast-resume latency benefits against standby-current cost on the target board.
