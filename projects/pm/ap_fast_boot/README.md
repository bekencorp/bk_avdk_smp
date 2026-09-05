# BK7259 AP Fast Boot Example

* [中文](./README_CN.md)

## 1. Overview

`ap_fast_boot` is based on `projects/pm/pm`. It verifies that the AP resumes its saved FreeRTOS execution point after a power cycle instead of repeating the complete cold-boot flow.

Its continuously running AP test task verifies that:

1. AP `main()` is entered only once.
2. Task-local and heap data survive an AP OFF/ON cycle.
3. The sequence number continues increasing after resume.
4. The CP can control AP power through the public vote API.

### 1.1 How Fast Resume Works

Before AP power-down, the system saves the CPU architectural context, copies power-gated DTCM contents into retained AP SRAM, and keeps PSRAM data valid. When the AP is powered up, DTCM and CPU context are restored and FreeRTOS continues from the saved execution point.

The current implementation retains only the CPU2 execution context. CPU2 and CPU3 start together on cold boot. Before AP OFF, the PM task hotplugs CPU3 offline; after resume, AP0 brings CPU3 online again.

> Fast boot restores a software execution point, not every register in power-gated peripherals. Stop non-resumable DMA, mailbox, interrupt, and peripheral activity before AP OFF, and reinitialize powered-down hardware as required.

## 2. Directory Structure

```text
ap_fast_boot/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c                 # Creates the resume-continuity test task
│   ├── CMakeLists.txt
│   └── config/bk7259_ap/
│       ├── defconfig             # AP fast-boot and PSRAM-retention options
│       └── usr_gpio_cfg.h
├── cp/
│   ├── cp_main.c                 # AP voting and ap_fast_boot CLI
│   ├── vnd_cal.h
│   ├── CMakeLists.txt
│   └── config/bk7259/
│       ├── defconfig             # CP low-power and fast-boot options
│       └── usr_gpio_cfg.h
└── partitions/bk7259/            # Flash and RAM layouts
```

## 3. Expected Output

At startup, the CP votes to power on the AP and registers:

```text
ap_fast_boot on
ap_fast_boot off
ap_fast_boot cycle
```

The AP creates `ap_resume_test`, which prints every two seconds:

```text
RESUME_PROOF seq=... stack=0x12345678 heap=... heap_value=0xa55a5aa5 main_entries=1 core=...
```

After a successful AP OFF/ON cycle:

| Field | Expected Result |
| --- | --- |
| `seq` | Continues from its previous value instead of restarting at 1 |
| `stack` | Remains `0x12345678` |
| `heap_value` | Remains `0xA55A5AA5` |
| `main_entries` | Remains `1` |
| `core` | Reports the core running the resumed test task |

If no valid saved CPU context is published or PSRAM retention fails, the fast-resume flag is cleared and the system falls back to AP cold boot.

## 4. Quick Start

### 4.1 Build

Run from the SDK root:

```bash
make bk7259 PROJECT=pm/ap_fast_boot
```

Or use the Docker command in `.ci`:

```bash
./dbuild.sh make bk7259 PROJECT=pm/ap_fast_boot
```

### 4.2 Flash And Serial Console

Flash the generated BK7259 firmware. Enter test commands through CP UART0:

| Function | GPIO |
| --- | --- |
| UART0 RX | `GPIO_10` |
| UART0 TX | `GPIO_11` |

Observe both AP and CP logs when possible to verify voting, AP power-down, and resume.

### 4.3 Run One Resume Test

The simplest test is:

```text
ap_fast_boot cycle
```

It votes the AP OFF, waits one second, and votes it ON. You can also run the steps separately:

```text
ap_fast_boot off
ap_fast_boot on
```

Each operation prints `AP vote ON/OFF ret=...`. `BK_OK` means that the vote call succeeded; the AP can power down only if no other module still holds an AP ON vote.

## 5. Startup And Resume Flow

### 5.1 CP Side

1. `main()` registers `user_app_main()`.
2. `bk_init()` initializes the CP.
3. `user_app_main()` submits the APP vote to start the AP.
4. The `ap_fast_boot` CLI is registered.
5. CLI operations call `bk_pm_module_vote_boot_ap_ctrl()` with ON or OFF.

### 5.2 AP Side

1. Cold boot enters `main()` and calls `bk_init()`.
2. `s_ap_main_entry_count` is incremented.
3. A 2048-byte-stack `ap_resume_test` task is created.
4. The task stores one canary on its stack and allocates another on the heap.
5. It increments `sequence` and prints `RESUME_PROOF` every two seconds.
6. Before AP OFF, CPU2 context and DTCM are saved; after AP ON, they are restored and the task loop continues.

## 6. Key Configuration

Both CP and AP must enable:

| Option | Description |
| --- | --- |
| `CONFIG_PM_AP_FAST_BOOT_ENABLE=y` | Enable AP context save/restore and CP coordination |
| `CONFIG_PSRAM_DATA_RETENTION_ENABLE=y` | Preserve PSRAM data while the AP is off |

The CP also configures:

| Option | Description |
| --- | --- |
| `CONFIG_PM_AP_POWERDOWN_WHEN_LV=y` | Allow AP power-down in low-voltage sleep |
| `CONFIG_PM_ONLY_CP_ENABLE=y` | Keep the CP as the resident low-power controller |
| `CONFIG_DEEP_LV=y` | Enable Deep Low Voltage |
| `CONFIG_FREERTOS_ALLOW_OS_API_IN_IRQ_DISABLED=y` | Support RTOS usage required by the resume flow |
| `CONFIG_AON_RTC=y` | Enable the AON RTC |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | Enable GPIO wakeup |

The AP also enables Tickless Idle, dynamic GPIO wakeup, and Bluetooth coordination while the AP is fully powered down.

## 7. Memory Retention

| Memory Region | Handling While AP Is Off |
| --- | --- |
| AP SRAM | Remains powered |
| AP DTCM | Copied to a 64-KiB `.noinit` buffer in retained AP SRAM and restored |
| PSRAM data | Preserved in place by PSRAM data retention |
| PSRAM staging | AP memory is not staged through PSRAM |

PSRAM retention increases standby current. Validate resume latency, data integrity, repeated-cycle stability, and low-power current on the target board.

## 8. Integration Notes

1. Keep fast-boot and PSRAM-retention options consistent on the CP and AP.
2. Stop or suspend peripheral accesses that cannot survive an AP power cycle.
3. Reconfigure power-gated peripherals and AP sub-power domains after resume.
4. Do not interpret task continuation as automatic retention of all hardware state.
5. Preserve the AP SRAM/DTCM backup space when modifying the RAM layout.
6. If execution-point retention is unnecessary, use `pm/pm` to avoid PSRAM-retention overhead.

## 9. FAQ

### 9.1 `seq` Restarts At 1 Or `main_entries` Increases

The AP followed the cold-boot path. Check that CP and AP fast-boot options match, PSRAM retention succeeded, and no invalid-context or resume-fallback message appears in the log.

### 9.2 The AP Does Not Turn Off

AP power is controlled by votes from multiple modules. Check whether Wi-Fi, Bluetooth, multimedia, or another module still holds an ON vote.

### 9.3 A Peripheral Fails After Resume

Fast boot restores CPU, DTCM, and retained-memory software state. Restore powered-down peripherals explicitly and clean up DMA, interrupts, mailbox traffic, and shared resources before AP OFF.

### 9.4 Repeated Cycles Fail Occasionally

Run more cycles and inspect PSRAM retention, AP SRAM, CPU3 hotplug, wakeup timing, and power stability. Avoid excessive logging during formal low-power measurements.
