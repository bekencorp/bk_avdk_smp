# CPU and SRAM Performance Test Project

* [中文](./README_CN.md)

## Overview

`test/cpu_perf_test` is a BK7259 engineering test project for measuring SRAM and PSRAM read/write time with the processor cycle counter.

The `CONFIG_CPU_PERF_TEST` option registers a `cpu_perf_test` CLI command. It benchmarks the test regions generated from `partitions/bk7259/ram_regions.csv` and can select the AP execution core when SMP is enabled.

## Project layout

- `ap/ap_main.c`: AP application entry
- `cp/cp_main.c`: CP initialization and AP startup
- SDK `ap/components/bk_cli/cli_cpu_perf.c`: shared CPU performance CLI implementation
- `ap/config/bk7259_ap/defconfig`: AP performance-test configuration
- `cp/config/bk7259/defconfig`: CP SMP performance-test configuration
- `bk7259_ap_bsp.ld`: AP test linker layout
- `partitions/bk7259/ram_regions.csv`: SRAM and PSRAM test-region definitions

## Build

Run from the SDK root:

```text
make bk7259 PROJECT=test/cpu_perf_test
```

## Run

Flash the AP and CP images, connect the serial consoles, and reset the board. From the AP CLI, run all configured regions on the current core:

```text
cpu_perf_test
```

When SMP is enabled, select AP core index `0` or `1` and optionally specify one or more region IDs:

```text
cpu_perf_test 0
cpu_perf_test 1 0 3
```

Region IDs follow the order compiled in SDK `ap/components/bk_cli/cli_cpu_perf.c`: `SRAM2_TEST`, `SRAM5_TEST`, `SRAM6_TEST`, `PSRAM0_TEST`, and `PSRAM1_TEST`. The output reports write and read cycle counts and the number of tested words.

## Test caution

The command writes test patterns to as much as the first 32 KiB of each selected region and temporarily disables local interrupts while measuring. Run it only with the project-provided memory layout and without unrelated application workloads.
