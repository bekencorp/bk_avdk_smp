# SRAM Lite Project

* [中文](./README_CN.md)

## Overview

`sram_lite` is the trimmed-down BK7258 application project. It shares the AP/CP startup flow and application entry code with the default `app` project, but trims features and re-splits memory to leave more SRAM to the AP side, which suits memory-constrained solutions that need a baseline to evaluate against.

- Removes the AT command service, the Bluetooth host, IPv6, iperf, and a set of driver self-tests; Bluetooth is limited to BLE slave.
- Disables `CONFIG_OTA_FUNCTION` (it is `y` in `app`), so the OTA implementation under `ap/components/ota` is not compiled.
- LWIP uses a reduced memory strategy: `CONFIG_LWIP_MEM_DEFAULT` on the AP and `CONFIG_LWIP_MEM_REDUCE` on the CP.
- The SRAM split favors the AP, moving 48K from the CP compared with `app` (`AP_RAM=0x05c000`, `CP_RAM=0x033700`).
- Keeps the main features: Wi-Fi, PSRAM, FATFS/SD, LittleFS, EasyFlash, and VFS.
- Both BK7258 and BK7257 build targets are supported.

The application entry also contains optional SMP, IPC, and BLE network-provisioning test initialization, which is compiled only when the corresponding configuration is enabled.

## Project layout

- `ap/ap_main.c`: AP initialization and optional test code
- `cp/cp_main.c`: CP initialization and CP1 power-on vote
- `ap/config/bk7258_ap/config`: trimmed AP project configuration
- `cp/config/bk7258/config`: trimmed CP project configuration
- `partitions/bk7258/auto_partitions.csv`: flash partition layout
- `partitions/bk7258/ram_regions.csv`: AP-favoring SRAM/PSRAM region layout

## Build

Run from the SDK root:

```text
make bk7258 PROJECT=sram_lite
```

To build the BK7257 target:

```text
make bk7257 PROJECT=sram_lite
```

## Run

Flash the generated AP and CP images, connect the serial consoles, and reset the board. Confirm that both application entries complete initialization, then use the remaining Wi-Fi and filesystem CLI commands for basic validation.

## Notes

- The value of this project is the memory baseline. Before re-enabling trimmed features (especially the Bluetooth host, AT, or IPv6), confirm that the SRAM split in `ram_regions.csv` is still sufficient.
- CP-side SRAM is already squeezed to `0x033700`, so adding CP features tends to hit the limit first.
- `CONFIG_OTA_HTTP` is still `y` while `CONFIG_OTA_FUNCTION` is `n`. To use OTA, enable `CONFIG_OTA_FUNCTION` as well; `CONFIG_OTA_HTTP` alone does not build the OTA implementation.
- `CONFIG_MAX_COMMANDS` is 200 on both AP and CP; raise it when adding many new CLI commands.
