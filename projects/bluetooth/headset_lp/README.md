# Bluetooth Headset Low-Power Example (AP power-down / RAM retained)

* [中文](./README_CN.md)

## What is this project?

This is the **low-power variant** of the [`headset`](../headset/README.md) project. It demonstrates the classic-Bluetooth headset/speaker application (A2DP Sink + AVRCP + HFP HF) under the **AP power-down (RAM retained)** low-power scheme.

The application features, CLI commands, code structure and data-flow are **identical to `headset`**. Please refer to:

> 📖 **[../headset/README.md](../headset/README.md)**

This file only documents the **relationship** to `headset` and the **differences**.

## Relationship: shared source

This project copies no `.c/.h`. Its `ap/` and `cp/` `CMakeLists.txt` reference the files under `../../headset` directly (`set(HS_AP .../headset/ap)`, `set(HS_CP .../headset/cp)`). Therefore:

- Change functional code only in `headset/`; this project follows automatically.
- Only the parts that differ from `headset` are kept here:

| Kept file | Purpose |
| --- | --- |
| `ap/config/bk7259_ap/defconfig` | AP-side config (low-power switches, see below) |
| `cp/config/bk7259/defconfig` | CP-side config (low-power switches, see below) |
| `ap/Kconfig.projbuild` | `A2DP_SINK_DEMO` / `HFP_HF_DEMO` options (Kconfig is scanned per directory, needs a local copy) |
| `ap/config/.../usr_gpio_cfg.h`, `cp/config/.../usr_gpio_cfg.h` | Board GPIO config |
| `cp/vnd_cal.c`, `cp/vnd_cal.h` | Local copies from `headset/cp` (RF calibration). `bk_init` hard-codes `$PROJECT_DIR/cp` as the search path for `vnd_cal.h` when `CONFIG_OVERRIDE_VND_CAL=y`, so these two files must physically exist here and cannot be shared |
| `partitions/` | Partition tables |

## Difference: low-power (AP power-down / RAM retained) config

Relative to a plain headset setup, this project enables the AP power-down (RAM retained) low-power scheme via defconfig. Key switches:

**AP side (`ap/config/bk7259_ap/defconfig`)**

```text
CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y   # AP power-down (RAM retained) scheme
CONFIG_PM_AP_FAST_BOOT_ENABLE=y               # AP fast resume (skips full soc_init)
CONFIG_PSRAM_DATA_RETENTION_ENABLE=y          # keep PSRAM data during power-down
CONFIG_FREERTOS_USE_TICKLESS_IDLE=2
CONFIG_GPIO_WAKEUP_SUPPORT=y
CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT=y
CONFIG_PM_AP_SUBPOWER_DOMAIN_DEFAULT_DISABLE=y
CONFIG_PM_AP_CPU_FRQ_DEFAUL=y
```

**CP side (`cp/config/bk7259/defconfig`)**

```text
CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y
CONFIG_PM_AP_FAST_BOOT_ENABLE=y
CONFIG_PSRAM_DATA_RETENTION_ENABLE=y
CONFIG_PM_AP_POWERDOWN_WHEN_LV=y              # allow AP power-down on low-voltage entry
CONFIG_DEEP_LV=y                              # Deep-LV context save/restore
CONFIG_PM_ONLY_CP_ENABLE=y
CONFIG_BLUETOOTH_SUPPORT_LPO_ROSC=y
CONFIG_BLE_LV_SUPPORT=y
```

> Note: `headset` is the plain (non-low-power) baseline; `headset_lp` enables the AP power-down (RAM retained) low-power scheme on top of it via defconfig. The CP-side difference is larger: `headset_lp` additionally turns on a full low-power set (`DEEP_LV`, tickless, AON current optimization, CP peripheral-clock default-off, `STA_PS`, `BLE_LV_SUPPORT`, ...), while `headset`'s CP defconfig keeps only the basic BT-controller config.

## Build

```bash
make bk7259 PROJECT=bluetooth/headset_lp
```

For flashing, CLI usage and test criteria, see [../headset/README.md](../headset/README.md).
