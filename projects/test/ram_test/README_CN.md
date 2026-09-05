# RAM 测试工程

* [English](./README.md)

## 工程概述

`test/ram_test` 是 BK7259 SRAM 和 PSRAM 验证工程。工程预留专用内存区域，并启用数据完整性、边界访问、性能、并发访问、CPU/DMA 交互、任务栈、队列和 PSRAM 长稳压力测试 CLI。

AP 使用单核 FreeRTOS 配置执行 RAM 测试，CP 负责系统初始化并启动 AP。

## 工程目录

- `ap/ap_main.c`：AP 应用入口
- `cp/cp_main.c`：CP 应用入口和 AP 启动逻辑
- `ap/config/bk7259_ap/defconfig`：SRAM、PSRAM 和内存 Dump 测试配置
- `cp/config/bk7259/defconfig`：CP SRAM 和内存 Dump 测试配置
- `partitions/bk7259/ram_regions.csv`：专用 SRAM 和 PSRAM 测试区域
- `bk7259_ap_bsp.ld` 和 `bk7259_bsp.ld`：测试链接布局
- SDK `ap/components/bk_cli/cli_sram.c`：SRAM 完整性测试命令
- SDK `ap/components/bk_cli/cli_sram_bus_bench.c`：SRAM/PSRAM 总线性能测试
- SDK `ap/middleware/driver/psram/psram_test.c`：PSRAM 测试命令

## 编译

在 SDK 根目录执行：

```text
make bk7259 PROJECT=test/ram_test
```

## 运行

烧录生成的 AP 和 CP 镜像，连接对应串口并复位开发板。

对所有预留 SRAM 区域执行全部完整性测试：

```text
sram_test_all
```

对指定区域执行单项测试。区域 ID `0` 到 `6` 分别对应 `SRAM0` 到 `SRAM6`：

```text
sram_test_unit 0
sram_test_perf 3
sram_test_boundary 6
```

执行 SRAM/PSRAM 总线性能测试或指定 PSRAM 测试：

```text
sram_bus_bench all 32768 10
psram_test start
psram_test stop
psram_test_ext speed
```

其他 SRAM 命令包括 `sram_test_multi`、`sram_test_full`、`sram_test_conflict`、`sram_test_random` 和 `sram_test_concurrent`。更多 `psram_test_ext` 压力和校验选项可通过 CLI 帮助查看。

## 测试注意事项

RAM 测试会覆盖 `ram_regions.csv` 中定义的专用区域，部分压力测试会持续运行，直到显式停止。必须配套使用本工程的链接文件和 RAM 区域配置，不要在测试区域存放应用数据。
