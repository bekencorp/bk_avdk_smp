# AP Emulated-Boot Project

* [中文](./README_CN.md)

## Overview

`ap_emuboot` is a minimal BK7259 project for validating the AP emulated-boot configuration. It removes most communication and peripheral features so the boot path can be exercised with a small configuration.

With `CONFIG_AP_EMUBOOT` enabled, the CP starts the AP during early system initialization instead of from the normal CP application entry. The AP and CP coordinate flash initialization through system software registers, after which the AP entry performs its project-specific flash-clock setup and calls `bk_init()`.

## Project layout

- `ap/ap_main.c`: AP early clock setup and application entry
- `cp/cp_main.c`: minimal CP application entry; AP startup occurs earlier in system initialization
- `ap/config/bk7259_ap/defconfig`: reduced AP emulated-boot configuration
- `cp/config/bk7259/defconfig`: reduced CP emulated-boot configuration
- `bk7259_ap_bsp.ld`: AP linker layout
- `bk7259_bsp.ld`: CP linker layout
- `partitions/bk7259/`: project partition and RAM-region definitions

## Build

Run from the SDK root:

```text
make bk7259 PROJECT=ap_emuboot
```

## Run

Flash the generated images using the emulated-boot test setup, connect the AP and CP serial consoles, and reset the target. Confirm that both sides complete `bk_init()` without a boot fault.

This project intentionally disables many default SDK features. Enable additional features only after checking the available flash and RAM layout.
