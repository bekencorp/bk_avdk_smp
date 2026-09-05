# CPU 与 SRAM 性能测试工程

* [English](./README.md)

## 工程概述

`test/cpu_perf_test` 是 BK7259 工程测试项目，使用处理器周期计数器测量 SRAM 和 PSRAM 的读写耗时。

`CONFIG_CPU_PERF_TEST` 选项会注册 `cpu_perf_test` CLI 命令。该命令测试由 `partitions/bk7259/ram_regions.csv` 生成的区域，并可在启用 SMP 时选择 AP 执行核。

## 工程目录

- `ap/ap_main.c`：AP 应用入口
- `cp/cp_main.c`：CP 初始化和 AP 启动逻辑
- SDK `ap/components/bk_cli/cli_cpu_perf.c`：公共 CPU 性能测试 CLI 实现
- `ap/config/bk7259_ap/defconfig`：AP 性能测试配置
- `cp/config/bk7259/defconfig`：CP SMP 性能测试配置
- `bk7259_ap_bsp.ld`：AP 测试链接布局
- `partitions/bk7259/ram_regions.csv`：SRAM 和 PSRAM 测试区域定义

## 编译

在 SDK 根目录执行：

```text
make bk7259 PROJECT=test/cpu_perf_test
```

## 运行

烧录 AP 和 CP 镜像，连接串口并复位开发板。在 AP CLI 中执行以下命令，可在当前核上测试全部配置区域：

```text
cpu_perf_test
```

启用 SMP 时，可指定 AP 核索引 `0` 或 `1`，并可继续指定一个或多个区域 ID：

```text
cpu_perf_test 0
cpu_perf_test 1 0 3
```

区域 ID 按 SDK `ap/components/bk_cli/cli_cpu_perf.c` 中的编译顺序排列：`SRAM2_TEST`、`SRAM5_TEST`、`SRAM6_TEST`、`PSRAM0_TEST` 和 `PSRAM1_TEST`。输出会显示读写周期数和参与测试的字数。

## 测试注意事项

该命令会向每个选中区域的前 32 KiB 范围内写入测试数据，并在测量期间临时关闭本地中断。请仅使用工程自带的内存布局运行，且不要同时运行无关应用负载。
