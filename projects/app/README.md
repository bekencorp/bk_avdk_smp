# Default Application Project

* [中文](./README_CN.md)

## Overview

`app` is the default BK7259 application project. It provides the standard AP/CP startup flow and a general-purpose configuration for SDK development and feature validation.

- The CP initializes the system and starts the AP.
- The AP initializes the application environment.
- Optional SMP, IPC, and BLE network-provisioning test code is selected by project configuration.
- The default configuration enables common Wi-Fi, Bluetooth, storage, filesystem, security, and networking features.

## Project layout

- `ap/ap_main.c`: AP application entry and optional test initialization
- `cp/cp_main.c`: CP application entry and AP startup
- `ap/config/bk7259_ap/defconfig`: AP project configuration
- `cp/config/bk7259/defconfig`: CP project configuration
- `partitions/bk7259/auto_partitions.csv`: flash partition layout

## Build

Run from the SDK root:

```text
make bk7259 PROJECT=app
```

Because `app` is the default project, this is equivalent to:

```text
make bk7259
```

To use the alternate platform configuration:

```text
make bk7259 PROJECT=app BK_CONFIG_FILE=platform
```

## Run

Flash the generated AP and CP images, connect their serial consoles, and reset the board. Use the enabled SDK CLI commands to exercise the configured features.

To change project options, update the AP or CP `defconfig`, or run the corresponding `menuconfig` target before rebuilding.
