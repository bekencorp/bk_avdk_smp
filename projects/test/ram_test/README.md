# RAM Test Project

* [中文](./README_CN.md)

## Overview

`test/ram_test` is the BK7259 SRAM and PSRAM validation project. It reserves dedicated memory regions and enables CLI tests for data integrity, boundary access, performance, concurrent access, CPU/DMA interaction, task stacks, queues, and long-running PSRAM stress.

The AP uses the single-core FreeRTOS configuration for RAM testing. The CP initializes the system and starts the AP.

## Project layout

- `ap/ap_main.c`: AP application entry
- `cp/cp_main.c`: CP application entry and AP startup
- `ap/config/bk7259_ap/defconfig`: SRAM, PSRAM, and memory-dump test configuration
- `cp/config/bk7259/defconfig`: CP SRAM and memory-dump test configuration
- `partitions/bk7259/ram_regions.csv`: dedicated SRAM and PSRAM test regions
- `bk7259_ap_bsp.ld` and `bk7259_bsp.ld`: test linker layouts
- SDK `ap/components/bk_cli/cli_sram.c`: SRAM integrity-test commands
- SDK `ap/components/bk_cli/cli_sram_bus_bench.c`: SRAM/PSRAM bus benchmark
- SDK `ap/middleware/driver/psram/psram_test.c`: PSRAM test commands

## Build

Run from the SDK root:

```text
make bk7259 PROJECT=test/ram_test
```

## Run

Flash the generated AP and CP images, connect the serial consoles, and reset the board.

Run all SRAM integrity cases on all reserved SRAM regions:

```text
sram_test_all
```

Run one test on one region, where the region ID is `0` through `6` for `SRAM0` through `SRAM6`:

```text
sram_test_unit 0
sram_test_perf 3
sram_test_boundary 6
```

Run the SRAM/PSRAM bus benchmark or selected PSRAM tests:

```text
sram_bus_bench all 32768 10
psram_test start
psram_test stop
psram_test_ext speed
```

Other SRAM commands include `sram_test_multi`, `sram_test_full`, `sram_test_conflict`, `sram_test_random`, and `sram_test_concurrent`. Use CLI help to view the additional `psram_test_ext` stress and verification options.

## Test caution

The RAM tests overwrite the dedicated regions defined in `ram_regions.csv`, and some stress tests continue until explicitly stopped. Run this firmware only with its matching linker and RAM-region configuration, and do not place application data in the test regions.
