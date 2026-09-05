# BK7259 PM Low-Power Example

* [中文](./README_CN.md)

## 1. Overview

`pm` is the base BK7259 power-management example. It demonstrates a resident CP, on-demand AP power control, Tickless Idle, Deep Low Voltage, GPIO/RTC wakeup, and Wi-Fi/BLE low-power configuration.

In short:

1. The CP remains active as the power-management controller while the AP can be powered down.
2. At startup, the CP votes for the AP through `PM_BOOT_AP_MODULE_NAME_APP`.
3. This project uses normal AP cold boot. After an AP power cycle, AP initialization and `main()` run again.
4. It is the base project for fast-resume examples such as `pm/ap_fast_boot`.

> Building this project alone does not guarantee the lowest possible current. A product must also shut down unused peripherals and manage power, clock, sleep, and wakeup-source votes correctly.

## 2. Directory Structure

```text
pm/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c                 # AP entry and optional SMP/provisioning/IPC tests
│   ├── CMakeLists.txt
│   └── config/bk7259_ap/
│       ├── defconfig             # AP low-power options
│       └── usr_gpio_cfg.h        # AP default GPIO configuration
├── cp/
│   ├── cp_main.c                 # CP entry; votes to start the AP
│   ├── vnd_cal.c/h               # Optional calibration override
│   ├── CMakeLists.txt
│   └── config/bk7259/
│       ├── defconfig             # CP low-power options
│       └── usr_gpio_cfg.h        # CP UART0 and wakeup-GPIO declarations
└── partitions/bk7259/            # Flash and RAM layouts
```

Start with `cp/cp_main.c`, `ap/ap_main.c`, and the two `defconfig` files.

## 3. Default Runtime Behavior

1. The CP registers `user_app_main()` and calls `bk_init()`.
2. `user_app_main()` calls `bk_pm_module_vote_boot_ap_ctrl()` and submits the APP vote to start the AP.
3. The AP powers up and calls `bk_init()`.
4. SMP, BLE provisioning, and IPC unit-test code runs only when its corresponding option is enabled.

The project itself does not create a continuously running application task or automatically execute a complete sleep test. It mainly provides low-power configuration and an application skeleton.

## 4. Quick Start

### 4.1 Build

Run from the SDK root:

```bash
make bk7259 PROJECT=pm/pm
```

The Docker command recorded in `.ci` is:

```bash
./dbuild.sh make bk7259 PROJECT=pm/pm
```

### 4.2 Flash And Serial Console

Flash the generated BK7259 firmware to the board. The CP CLI uses UART0 with these defaults:

| Function | GPIO |
| --- | --- |
| UART0 RX | `GPIO_10` |
| UART0 TX | `GPIO_11` |
| Falling-edge input declarations | `GPIO_8`, `GPIO_17` |

`GPIO_8` and `GPIO_17` are only declared in `usr_gpio_cfg.h`. To use either pin as a system wakeup source, also register it and enable the PM GPIO wakeup source at runtime.

### 4.3 Control AP Power Manually

The current CP configuration enables the debug PM CLI. Use the CP UART0 console:

```text
pm_boot_ap 9 1
pm_boot_ap 9 0
```

Module `9` is `PM_BOOT_AP_MODULE_NAME_APP`; state `1` means OFF and state `0` means ON. Because AP fast boot is not enabled, powering the AP on again follows the cold-boot path.

## 5. Key Configuration

### 5.1 CP Side

Important options in `cp/config/bk7259/defconfig`:

| Option | Description |
| --- | --- |
| `CONFIG_SOC_SMP=n`, `CONFIG_FREERTOS_SMP=n` | Run the CP in non-SMP mode |
| `CONFIG_PM_ONLY_CP_ENABLE=y` | Keep the CP as the resident low-power core |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | Enable Tickless Idle |
| `CONFIG_PM_AP_POWERDOWN_WHEN_LV=y` | Allow AP power-down in low-voltage sleep |
| `CONFIG_DEEP_LV=y` | Enable the Deep Low Voltage flow |
| `CONFIG_AON_RTC=y` | Enable the AON RTC |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | Enable GPIO wakeup |
| `CONFIG_TOUCH=y`, `CONFIG_TOUCH_TEST=y` | Enable touch and its test in the current project |
| `CONFIG_CKMN_TEST=y` | Enable the clock-monitor test |
| `CONFIG_CPU_DEFAULT_FREQ_60M=y` | Set the default CP frequency to 60 MHz |
| `CONFIG_PM_CP_PERI_CLK_DEFAULT_OFF=y` | Disable unused CP peripheral clocks by default |
| `CONFIG_STA_PS=y` | Enable Wi-Fi STA power saving |
| `CONFIG_BLE_LV_SUPPORT=y` | Enable BLE low-voltage sleep support |

The current configuration also enables `CONFIG_DEEP_LV_DEBUG_GPIO=y` and `CONFIG_DEBUG_VERSION=y`. Evaluate the effect of debug GPIOs, logs, and test modules before making formal current measurements.

### 5.2 AP Side

Important options in `ap/config/bk7259_ap/defconfig`:

| Option | Description |
| --- | --- |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | Enable AP Tickless Idle |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | Enable AP GPIO wakeup |
| `CONFIG_PM_AP_SUBPOWER_DOMAIN_DEFAULT_DISABLE=y` | Disable VIDEO_POST, H26E, ISP, and NPU domains by default |
| `CONFIG_PM_AP_CPU_FRQ_DEFAUL=y` | Vote for the default AP CPU frequency after initialization |
| `CONFIG_SLAVE_HEART_BEAT=n` | Disable the AP heartbeat configuration |
| `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL=y` | Support Bluetooth operation while the AP is fully off |

Before using an AP sub-power domain that is disabled by default, the application must power it up through the module-specific flow.

## 6. Add GPIO Wakeup

For example, to use a falling edge on `GPIO_17`, keep its declaration in `usr_gpio_cfg.h` and configure it at runtime:

```c
bk_gpio_register_wakeup_source(GPIO_17, GPIO_INT_TYPE_FALLING_EDGE);
bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
```

Then submit the required sleep vote. The trigger type in the GPIO table must match the runtime registration, and the pin must not already be at a continuously active wakeup level before sleep.

## 7. Difference From Fast Boot

| Item | `pm/pm` | `pm/ap_fast_boot` |
| --- | --- | --- |
| AP resume behavior | Cold boot | Resume saved FreeRTOS execution point |
| `CONFIG_PM_AP_FAST_BOOT_ENABLE` | Disabled | Enabled |
| `CONFIG_PSRAM_DATA_RETENTION_ENABLE` | Disabled | Enabled |
| AP cycle command | Generic `pm_boot_ap` | Dedicated `ap_fast_boot` |
| Low-power trade-off | No fast-resume retention overhead | PSRAM retention increases standby current |

## 8. FAQ

### 8.1 The System Does Not Enter Low Power

Check for unreleased power, clock, or sleep votes; active peripherals or DMA; and a wakeup GPIO that is already at its active level.

### 8.2 The AP Does Not Power Down

AP power is controlled by votes from multiple modules. The AP can power down only after every relevant module releases its ON vote. Inspect the PM votes and current AP state.

### 8.3 Measured Current Is Too High

Disable unnecessary debug logs, test modules, and debug GPIOs. Verify that audio, video, LCD, Wi-Fi, BLE, PSRAM, and peripheral clocks enter the intended low-power state.

### 8.4 GPIO Interrupt Works But Does Not Wake The System

`usr_gpio_cfg.h` only declares the pin. Also register the GPIO wakeup source, enable the PM GPIO wakeup source, and submit the proper sleep vote.
