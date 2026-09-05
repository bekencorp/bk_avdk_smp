# SRAM Lite Project

* [中文](./README_CN.md)

## Overview

`sram_lite` is a lightweight AP/CP application skeleton for resource-constrained build validation. It keeps only the application entries and component registration files, without project-specific partition tables or `defconfig` files.

The project therefore inherits the selected SoC defaults. Its application entry also contains optional SMP, IPC, and BLE network-provisioning test initialization, which is compiled only when the corresponding configuration is enabled.

## Project layout

- `ap/ap_main.c`: AP initialization and optional test code
- `cp/cp_main.c`: CP initialization and AP power-on vote
- `ap/CMakeLists.txt`: AP source and private component dependencies
- `cp/CMakeLists.txt`: CP source registration
- `CMakeLists.txt`: top-level project definition

## Build

For this SDK release, run from the SDK root:

```text
make bk7259 PROJECT=sram_lite
```

## Run

Flash the generated AP and CP images, connect the serial consoles, and reset the board. Confirm that both application entries complete initialization.

Because this project inherits SoC defaults, check the effective configuration and memory layout before enabling additional components.
