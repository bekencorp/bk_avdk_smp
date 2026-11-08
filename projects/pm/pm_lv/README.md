# BK7259 PM Low-Voltage Sleep Example

* [中文](./README_CN.md)

## 1. Overview

`pm_lv` is the low-voltage (LV) sleep variant of the BK7259 `pm/pm` power-management example. It demonstrates a resident CP, on-demand AP power control, Tickless Idle, low-voltage sleep, GPIO/RTC wakeup, and Wi-Fi/BLE low-power configuration.

In short:

1. The CP stays active as the power-management controller while the AP can be powered down.
2. At startup, the CP votes for the AP through `PM_BOOT_AP_MODULE_NAME_APP`.
3. This project uses normal AP cold boot. After an AP power cycle, AP initialization and `main()` run again.
4. Compared with `pm/pm`, `CONFIG_PM_LV_SLEEP_ENABLE=y` makes `sys_hal_enter_low_voltage()` call `sys_hal_enter_lv()` directly, so the CP always takes the LV path instead of choosing deep LV from the bootloader version. Wakeup returns in place rather than re-entering through the deep-LV reset flow.
5. `CONFIG_DEEP_LV_DEBUG_GPIO=n` removes the debug GPIO toggles around sleep entry and exit, removing debug-pin activity from current measurements.

> Building this project alone does not guarantee the lowest possible current. A product must also shut down unused peripherals and manage power, clock, sleep, and wakeup-source votes correctly.

## 2. Directory Structure

```text
pm_lv/
├── CMakeLists.txt
├── Makefile
├── .ci                           # CI build command
├── app.rst
├── README.md
├── README_CN.md
├── pj_config.mk                  # Project-level build options (empty for now)
├── ap/
│   ├── ap_main.c                 # AP entry and optional SMP/provisioning/IPC tests
│   ├── CMakeLists.txt
│   └── config/bk7259_ap/
│       ├── defconfig             # AP low-power options
│       └── usr_gpio_cfg.h        # AP default GPIO configuration table
├── cp/
│   ├── cp_main.c                 # CP entry; votes to start the AP
│   ├── vnd_cal.c/h               # Optional calibration override
│   ├── CMakeLists.txt
│   └── config/bk7259/
│       ├── defconfig             # CP low-power options
│       └── usr_gpio_cfg.h        # CP UART0 and wakeup-GPIO declarations
└── partitions/bk7259/
    ├── auto_partitions.csv       # Flash layout (8 MB)
    └── ram_regions.csv           # SRAM/PSRAM layout
```

Start with `cp/cp_main.c`, `ap/ap_main.c`, and the two `defconfig` files.

## 3. Default Runtime Behavior

1. The CP registers `user_app_main()` and calls `bk_init()`.
2. `user_app_main()` calls `bk_pm_module_vote_boot_ap_ctrl()` and submits the APP vote to start the AP.
3. The AP powers up and calls `bk_init()`.
4. SMP, BLE provisioning, and IPC unit-test code runs only when its corresponding option is enabled (`CONFIG_FREERTOS_SMP_TEST`, `CONFIG_BK_NETWORK_PROVISIONING_BLE_EXAMPLE`, `BK_IPC_UT_TEST`), and none of them is enabled by default.

The project itself does not create a continuously running application task or automatically execute a complete sleep test. It mainly provides the LV low-power configuration and an application skeleton.

## 4. Quick Start

### 4.1 Build

Run from the SDK root:

```bash
make bk7259 PROJECT=pm/pm_lv
```

The Docker command recorded in `.ci` is:

```bash
./dbuild.sh make bk7259 PROJECT=pm/pm_lv
```

### 4.2 Flash And Serial Console

Flash the generated BK7259 firmware to the board. The CP CLI uses UART0 with these defaults:

| Function | GPIO | Notes |
| --- | --- | --- |
| UART0 RX | `GPIO_10` | Pull-up, also the SoC default static wakeup source (falling edge) |
| UART0 TX | `GPIO_11` | Pull-up |
| Declared falling-edge inputs | `GPIO_8`, `GPIO_17` | Interrupt disabled at power-on, low-power IO discarded |

`GPIO_8` and `GPIO_17` in `cp/config/bk7259/usr_gpio_cfg.h` only declare possible wakeup pins. Because their low-power IO control is `GPIO_LOW_POWER_DISCARD_IO_STATUS` and their interrupt is disabled, they cannot wake the system until the table and the runtime registration are updated (see Section 6).

The always-registered static wakeup source comes from `GPIO_STATIC_WAKEUP_SOURCE_MAP` in `cp/middleware/soc/bk7259/soc/gpio_map.h`, where `GPIO_10` is mapped to `GPIO_INT_TYPE_FALLING_EDGE`. UART0 console input can therefore wake the CP.

### 4.3 Control AP Power Manually

The current CP configuration enables the debug PM CLI (`CONFIG_DEBUG_VERSION=y`). Use the CP UART0 console:

```text
pm_boot_ap 9 1
pm_boot_ap 9 0
```

Module `9` is `PM_BOOT_AP_MODULE_NAME_APP`; state `1` means OFF and state `0` means ON. Because AP fast boot is not enabled, powering the AP on again follows the cold-boot path.

To exercise AP power cycling repeatedly:

```text
pm_boot_ap stress [start_module] [task_count] [loop_count] [on_hold_ms] [off_hold_ms]
```

## 5. Key Configuration

### 5.1 CP Side

Important options in `cp/config/bk7259/defconfig`:

| Option | Description |
| --- | --- |
| `CONFIG_SOC_SMP=n`, `CONFIG_FREERTOS_SMP=n` | Run the CP in non-SMP mode |
| `CONFIG_PSRAM=y`, `CONFIG_PSRAM_AS_SYS_MEMORY=n` | Enable PSRAM without using it as system memory |
| `CONFIG_PM_ONLY_CP_ENABLE=y` | Keep the CP as the resident low-power core |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | Enable Tickless Idle |
| `CONFIG_PM_AP_POWERDOWN_WHEN_LV=y` | Power down the AP when the CP enters low-voltage sleep |
| `CONFIG_DEEP_LV=y` | Keep the deep-LV support code built (shared IRQ-mask restore and related paths) |
| `CONFIG_PM_LV_SLEEP_ENABLE=y` | Enter low-voltage sleep instead of deep low voltage |
| `CONFIG_DEEP_LV_DEBUG_GPIO=n` | Disable the deep-LV debug GPIO toggle markers |
| `CONFIG_DEEP_LV_DEBUG_LOG=n` | Disable the AON snapshot debug log |
| `CONFIG_AON_RTC=y` | Enable the AON RTC |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | Enable GPIO wakeup |
| `CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT=y` | Allow GPIO wakeup sources to be registered at runtime |
| `CONFIG_TOUCH=y`, `CONFIG_TOUCH_TEST=y` | Enable touch and its test in the current project |
| `CONFIG_CKMN_TEST=y` | Enable the clock-monitor test |
| `CONFIG_OPTIMIZE_AON_CURRENT=y`, `CONFIG_AON_LDO_SEL=2` | Optimize AON current and select the AON LDO setting |
| `CONFIG_PM_CURRENT_OPTIMIZE_XTAL_RX_TX_ANABUF=y` | Optimize current through the XTAL RX/TX analog buffers |
| `CONFIG_CPU_DEFAULT_FREQ_60M=y` | Set the default CP frequency to 60 MHz |
| `CONFIG_PM_AP_VOTE_CP_CPU_FREQ_DEFAULT=2` | CP frequency voted by the AP at boot-up (level 2 = 80 MHz) |
| `CONFIG_PM_CP_PERI_CLK_DEFAULT_OFF=y` | Disable unused CP peripheral clocks by default |
| `CONFIG_PM_XTAL_RX_TX_ANABUF_DEFAULT_OFF=y` | Disable unused XTAL RX/TX analog buffers by default |
| `CONFIG_SYS_PM_ANA_VANALDO_STEP_CTRL=y` | Enable stepped ANA/VANALDO control during sleep |
| `CONFIG_STA_PS=y` | Enable Wi-Fi STA power saving |
| `CONFIG_BLUETOOTH=y`, `CONFIG_BLUETOOTH_BLE_SLAVE_ONLY=y` | Enable Bluetooth with BLE slave-only mode |
| `CONFIG_BLE_LV_SUPPORT=y` | Enable BLE low-voltage sleep support |
| `CONFIG_BLUETOOTH_SUPPORT_LPO_ROSC=y` | Support the RC-oscillator LPO clock for Bluetooth |
| `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL=y` | Support Bluetooth operation while the AP is fully off |
| `CONFIG_DEBUG_VERSION=y` | Keep the debug CLI and debug build options enabled |

Sleep path selection:

- `CONFIG_PM_LV_SLEEP_ENABLE=y` makes `sys_hal_enter_low_voltage()` call `sys_hal_enter_lv()` unconditionally. The bootloader-version check that selects deep LV is skipped.
- `CONFIG_DEEP_LV=y` is still kept because shared deep-LV code (for example the saved IRQ-mask restore after wakeup) is compiled under it.
- Before formal current measurements, evaluate the effect of `CONFIG_DEBUG_VERSION=y`, logs, and test modules such as `CONFIG_TOUCH_TEST=y` and `CONFIG_CKMN_TEST=y`.

### 5.2 AP Side

Important options in `ap/config/bk7259_ap/defconfig`:

| Option | Description |
| --- | --- |
| `CONFIG_BLUETOOTH_AP=y` | Enable the Bluetooth AP-side support |
| `CONFIG_AT=y`, `CONFIG_DEFAULT_AT_PORT=5` | Enable AT commands on port 5 |
| `CONFIG_WIFI_AT_ENABLE=y`, `CONFIG_MISC_AT_ENABLE=y` | Enable Wi-Fi and miscellaneous AT commands |
| `CONFIG_AT_MAX_PORT_CNT=1` | Allow one AT port |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | Enable AP Tickless Idle |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y` | Enable AP GPIO wakeup |
| `CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT=y` | Allow AP GPIO wakeup sources to be registered at runtime |
| `CONFIG_PM_AP_SUBPOWER_DOMAIN_DEFAULT_DISABLE=y` | Disable VIDEO_POST, H26E, ISP, and NPU domains by default |
| `CONFIG_PM_AP_CPU_FRQ_DEFAUL=y` | Vote for the default AP CPU frequency after initialization |
| `CONFIG_PM_AP_WFI_ENABLE=y` | Allow the AP to enter WFI when idle |
| `CONFIG_SLAVE_HEART_BEAT=n` | Disable the AP heartbeat configuration |
| `CONFIG_BLUETOOTH_HOST_ONLY=n` | Do not build the AP as a Bluetooth host-only image |
| `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL=y` | Support Bluetooth operation while the AP is fully off |

The AP GPIO table in `ap/config/bk7259_ap/usr_gpio_cfg.h` also assigns default functions to many pins: SDIO host (`GPIO_2`–`GPIO_4`), I2S0 (`GPIO_6`, `GPIO_7`, `GPIO_9`, `GPIO_28`), USB0 DN (`GPIO_13`), RGB LCD (`GPIO_14`–`GPIO_26`, `GPIO_40`–`GPIO_55`), JPEG (`GPIO_27`, `GPIO_29`–`GPIO_39`), and UART5 (`GPIO_64`, `GPIO_65`). `GPIO_8` and `GPIO_17` are kept as inputs in low power. Remove or reconfigure the functions your product does not use before measuring current.

Before using an AP sub-power domain that is disabled by default, the application must power it up through the module-specific flow.

### 5.3 Memory Layout

Flash (8 MB, `partitions/bk7259/auto_partitions.csv`):

| Partition | Size | Type |
| --- | --- | --- |
| `primary_bootloader` | 64 KB | code |
| `primary_cp_app` | 1428 KB | code |
| `primary_ap_app` | 1156 KB | code |
| `ota` | 1360 KB | data |
| `usr_config` | 60 KB | data |
| `easyflash`, `easyflash_ap` | 8 KB each | data |
| `sys_rf`, `sys_net` | 4 KB each | data |

RAM (`partitions/bk7259/ram_regions.csv`): `CP_RAM` (0x05F000, 380 KB at `0x28000000`), `SWAP` (4 KB), `AP_HSRAM_HEAP` (512 KB at `0x28100000`), `AP_RAM` (384 KB), plus PSRAM regions `PSRAM_MEM_SLAB_UNCODED`, `PSRAM_MEM_SLAB_CODED`, `CP_PSRAM_HEAP` (128 KB), `AP_PSRAM_DATA_SECTION` (256 KB), `AP_PSRAM_HEAP` (512 KB), `AP_PSRAM_CACHE_HEAP` (128 KB), and `AP_PSRAM_CODE_SECTION` (1 MB).

## 6. Add GPIO Wakeup

For example, to use a falling edge on `GPIO_17` as a CP wakeup source:

1. In `cp/config/bk7259/usr_gpio_cfg.h`, set `GPIO_17` to `GPIO_LOW_POWER_KEEP_INPUT_STATUS` and enable the interrupt with `GPIO_INT_TYPE_FALLING_EDGE`. The current table uses `GPIO_LOW_POWER_DISCARD_IO_STATUS` and disables the interrupt, so the pin cannot wake the system as-is.
2. Register the pin at runtime and enable the PM GPIO wakeup source:

```c
bk_gpio_register_wakeup_source(GPIO_17, GPIO_INT_TYPE_FALLING_EDGE);
bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
```

3. Submit the required sleep vote so the system can enter low-voltage sleep.

The trigger type in the GPIO table must match the runtime registration, and the pin must not already be at a continuously active wakeup level before sleep. `CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT=y` allows up to `CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT` (4 by default) dynamically registered sources; use `bk_gpio_unregister_wakeup_source()` to release one when it is no longer needed.

The CP/AP split rule still applies: each core configures its own GPIOs, and the same pin must not be configured by both cores at the same time.

## 7. Difference From pm/pm

| Item | `pm/pm` | `pm/pm_lv` |
| --- | --- | --- |
| Sleep entry | Deep LV when the bootloader supports it | Always LV (`CONFIG_PM_LV_SLEEP_ENABLE=y`) |
| Deep-LV debug GPIO | `CONFIG_DEEP_LV_DEBUG_GPIO=y` | `CONFIG_DEEP_LV_DEBUG_GPIO=n` |
| Wakeup flow | Deep LV wakes through the reset/bootloader flow | LV returns in place |
| Debugging | Sleep entry/exit visible on a debug GPIO | No debug GPIO activity |
| AP resume behavior | Cold boot | Cold boot |
| AP cycle command | Generic `pm_boot_ap` | Generic `pm_boot_ap` |

## 8. FAQ

### 8.1 The System Does Not Enter Low Power

Check for unreleased power, clock, or sleep votes; active peripherals or DMA; and a wakeup GPIO that is already at its active level. Also verify which wakeup sources are registered when the sleep vote is submitted.

### 8.2 The AP Does Not Power Down

AP power is controlled by votes from multiple modules. The AP can power down only after every relevant module releases its ON vote. Inspect the PM votes and current AP state.

### 8.3 Measured Current Is Too High

Disable unnecessary debug logs and test modules, and remove unused GPIO functions from the AP table (LCD, JPEG, I2S, SDIO, UART5). Verify that audio, video, LCD, Wi-Fi, BLE, PSRAM, and peripheral clocks enter the intended low-power state. Confirm that no debug GPIO is toggled and no debugger or UART traffic keeps waking the CP.

### 8.4 GPIO Interrupt Works But Does Not Wake The System

`usr_gpio_cfg.h` only declares the pin. Also set the low-power IO control to `GPIO_LOW_POWER_KEEP_INPUT_STATUS`, enable the interrupt with the correct trigger type, register the GPIO wakeup source, enable the PM GPIO wakeup source, and submit the proper sleep vote.

### 8.5 Why Is `CONFIG_DEEP_LV` Still Enabled

`CONFIG_DEEP_LV=y` keeps the shared deep-LV support code in the build. Because `CONFIG_PM_LV_SLEEP_ENABLE=y` takes precedence in `sys_hal_enter_low_voltage()`, the CP still enters LV sleep and never selects the deep-LV entry.
