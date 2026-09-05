# Flash / PSRAM 硬件验证工程

* [English](./README.md)

## 工程概述

`flash_psram_test` 基于 `test/ram_test`，用于**新 Flash / 新 PSRAM 导入时的硬件验证**。

工程默认：

- Flash：**16M**（`CONFIG_FLASH_CAPACITY_16M=y`，`auto_partitions_16M.csv`，RF/NET 在 `0xFFE000`/`0xFFF000`）
- PSRAM：**32M / 颗**（`CONFIG_PSRAM_CAPACITY_32M=y`，`ram_regions_32M.csv`，高 16MB 为 `PSRAM0_TEST_HIGH` / `PSRAM1_TEST_HIGH`）

产物目录：`build/bk7259/flash_psram_test_16M_psram32M/`。

AP 使用单核 FreeRTOS 跑 RAM 测试，CP 负责系统初始化并启动 AP。CP 打开 `CONFIG_FLASH_TEST`。

## 编译

```text
make bk7259 PROJECT=flash_psram_test
```

## 运行

```text
flash_test ID
psram_test start
psram_test stop
sram_test_all
sram_bus_bench all 32768 10
```
