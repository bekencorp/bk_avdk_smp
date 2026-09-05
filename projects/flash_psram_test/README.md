# Flash / PSRAM Hardware Verification Project

* [中文](./README_CN.md)

## Overview

`flash_psram_test` is based on `test/ram_test`. Use it when bringing up a new Flash or PSRAM part.

Project defaults:

- Flash: **16MB** (`CONFIG_FLASH_CAPACITY_16M=y`, `auto_partitions_16M.csv`)
- PSRAM: **32MB per chip** (`CONFIG_PSRAM_CAPACITY_32M=y`, `ram_regions_32M.csv`)

Image directory: `build/bk7259/flash_psram_test_16M_psram32M/`.

## Build

```text
make bk7259 PROJECT=flash_psram_test
```

## Run

```text
flash_test ID
psram_test start
psram_test stop
sram_test_all
sram_bus_bench all 32768 10
```
