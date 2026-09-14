# Default Application Project

* [中文](./README_CN.md)

## Overview

`app` is the default BK7258 application project. It provides the standard AP/CP startup flow and a general-purpose configuration for SDK development and feature validation.

- The CP initializes the system and votes CP1 on to start SMP.
- The AP initializes the application environment.
- Optional SMP, IPC, and BLE network-provisioning test code is selected by project configuration.
- The default configuration enables common Wi-Fi, Bluetooth, storage, filesystem, security, and networking features.
- Both BK7258 and BK7257 build targets are supported.

## Project layout

- `ap/ap_main.c`: AP application entry and optional test initialization
- `cp/cp_main.c`: CP application entry and CP1 startup
- `ap/config/bk7258_ap/config`: AP project configuration
- `ap/config/bk7258_ap/no_cli.config`: alternate configuration with CLI disabled
- `cp/config/bk7258/config`: CP project configuration
- `partitions/bk7258/auto_partitions.csv`: flash partition layout
- `partitions/bk7258/ram_regions.csv`: SRAM/PSRAM region layout

## Build

Run from the SDK root:

```text
make bk7258 PROJECT=app
```

Because `app` is the default project, this is equivalent to:

```text
make bk7258
```

To build the BK7257 target:

```text
make bk7257 PROJECT=app
```

To use the alternate configuration with CLI disabled:

```text
make bk7258 PROJECT=app BK_CONFIG_FILE=no_cli
```

## Run

Flash the generated AP and CP images, connect their serial consoles, and reset the board. Use the enabled SDK CLI commands to exercise the configured features; commands targeting the AP need the `ap_cmd` prefix.

To change project options, update the AP or CP `config`, or run `make bk7258_ap_menuconfig PROJECT=app` or `make bk7258_cp_menuconfig PROJECT=app` before rebuilding.
