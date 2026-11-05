# dhrystone_test 示例工程

* [English](./README.md)

## 1. 工程概述

本工程演示在 BK7259 平台的 AP 侧运行 Dhrystone 2.1 基准测试，用于测量 CPU 的
DMIPS。AP 侧 defconfig 中打开了 `CONFIG_DHRYSTONE_TEST`，从而注册
`dhrystone` CLI 命令。

当前工程提供：

- AP 侧测试命令 `ap_cmd dhrystone`（默认双核并行，`contexts 2`）
- 每次运行前的升频、看门狗处理与 DWT 频率自检，保证计时可复现
- 完整的分项输出：Flash 时钟、DWT 自检、并行墙钟、DMIPS 与平均 cycles/run

CP 侧同样注册了 `dhrystone`，在 CP 串口直接执行即可，其余流程与 AP 侧一致。

## 2. 测试环境

- 开发板：BK7259 系列或兼容平台
- 调试串口：115200 8N1，默认接在 CP 侧（UART0）
- 串口终端：用于查看日志并输入 CLI 命令
- 编译环境：已按 SDK 要求导出 `ARMINO_TOOLS_PATH`

## 3. 目录结构

```text
<SDK_ROOT>/
├── projects/dhrystone_test/
│   ├── ap/
│   │   ├── ap_main.c                       # AP 入口（调用 dhry_cli_init）
│   │   ├── CMakeLists.txt                  # 源码加入与按文件宏
│   │   ├── Kconfig                         # CONFIG_DHRYSTONE_TEST 开关
│   │   ├── dhrystone/                      # Dhrystone 2.1 源码、移植层与 CLI 命令
│   │   │   ├── dhry.h
│   │   │   ├── dhry_1.c / dhry_2.c         # 官方源码
│   │   │   ├── dhry_1_ctx1.c / dhry_2_ctx1.c  # 双核第二份实例
│   │   │   ├── dhry_glue.c                 # 计时、printf/scanf 重定向、SMP 调度
│   │   │   ├── dhry_cli.c                  # dhrystone 命令注册与运行前环境处理
│   │   │   └── dhry_port.h                 # 计时基准、HZ、DMIPS 换算基数
│   │   └── config/bk7259_ap/defconfig      # AP 配置（打开 CONFIG_DHRYSTONE_TEST）
│   ├── cp/                                 # CP 侧入口、配置与 dhrystone 源码，结构同 AP
│   ├── partitions/bk7259/
│   │   ├── auto_partitions.csv
│   │   └── ram_regions.csv
│   ├── CMakeLists.txt
│   ├── Makefile
│   ├── pj_config.mk
│   └── README.md / README_CN.md
```

DMIPS 测试代码、移植层与 CLI 命令都随工程放在 `projects/dhrystone_test/ap/dhrystone/`
与 `cp/dhrystone/` 下，`bk_cli` 不再参与 Dhrystone 的编译与命令注册。

## 4. 编译与烧录

在 SDK 根目录执行：

```bash
cd <SDK_ROOT>
export ARMINO_TOOLS_PATH=$PWD/tools/build_tools
make bk7259 PROJECT=dhrystone_test
```

编译产物：

| 文件 | 说明 |
| --- | --- |
| `build/bk7259/dhrystone_test/package/all-app.bin` | 烧录镜像 |
| `build/bk7259/dhrystone_test/bk7259_ap/app.elf` | AP 核 ELF，可用于 map/符号分析 |

## 5. 使用步骤

### 5.1 运行 AP 侧测试

在 CP 串口控制台输入：

```text
ap_cmd dhrystone              # 默认 5,000,000 次
ap_cmd dhrystone 20000000     # 自定义运行次数
ap_cmd debug help             # 查看 AP 侧全部 debug 子命令
```

### 5.2 执行前后的环境处理

AP 侧在运行前会依次执行：

1. 停 WWDT/TaskWDT
2. 禁止 idle 休眠投票
3. 升频到 480MHz
4. 打印 Flash 时钟并做 DWT 频率自检
5. 在独立的 dhrystone 线程中运行（默认核 0 / 核 1 各一份实例，各 4096 字节栈）
6. 恢复 480MHz 设置、休眠投票与看门狗

目的是避免 CPU 时钟被门控或降频、以及被其他任务抢占，导致计时失真。测完后系统
状态与测试前一致，不影响后续功能。

默认即为双核并行（`contexts 2`），由 `dhry_1_ctx1.c` / `dhry_2_ctx1.c` 重新
`#include` 官方源码并重命名符号得到第二份独立实例，官方源码本身不做修改。

### 5.3 运行 CP 侧测试（可选）

在 CP 串口直接输入 `dhrystone` 即可，升频、看门狗与计时流程由 CP 侧自行处理。

## 6. 输出解读

```text
Flash clk: cksel=... ckdiv=... => source=... div=/...
DWT self-test 100ms: DWT=... cy (0.100s@480M)  Timer0=... tk (0.100s@26M)  ratio=1.00
Dhrystone 2.1, runs 5000000, SMP 2 cores (ctx0 on AP core 0, ctx1 on AP core 1)
Config: dhrystone -Os, code FLASH-XIP, data SRAM-HEAP, 480 MHz, contexts 2
[c0] Execution starts, 5000000 runs through Dhrystone
[c1] Execution starts, 5000000 runs through Dhrystone
...
[c0] Final values of the variables used in the benchmark:
...
Parallel 2 cores: 2 x 5000000 runs in 15.612 s wall
Dhrystones per Second (2 cores): 640533.0
DMIPS (2 cores): 364.56   DMIPS/MHz (2 cores): 0.759   avg cycles/run: 1502
```

- `Flash clk`：Flash 时钟源与分频
- `DWT self-test`：用 DWT 周期数与 26MHz Timer0 交叉验证，`ratio` 接近 1.00
  说明内核确实运行在 480MHz；偏离则说明频率未生效，本次成绩不可信
- `Config`：本次测试的编译选项、代码/数据落位与运行频率
- `Parallel 2 cores: ... runs in ... s wall`：整机并行墙钟窗口，起点在线程创建
  之前、终点在最后一个核结束之后
- `Dhrystones per Second (2 cores)`：整机成绩，
  即 **总工作量 ÷ 并行墙钟时间** = `contexts × runs` ÷ window，而不是各核速率相加
- `DMIPS (2 cores)`：整机 Dhrystones/s ÷ 1757
- `DMIPS/MHz (2 cores)`：整机 DMIPS ÷ 480MHz，便于与不同主频下的结果横向比较
- `avg cycles/run`：两核每 run 周期数的平均值，用于定位取指/访存效率，不参与报分
- `[c0]` / `[c1]`：两核实例的原始输出前缀，只用于确认两核都跑完、Final values
  与 `should be:` 一致

DMIPS 计算方式：

```text
Dhrystones per Second = (contexts × runs) ÷ 并行墙钟时间(s)
DMIPS                 = Dhrystones per Second ÷ 1757
DMIPS/MHz             = DMIPS ÷ 480
```

其中 1757 是 VAX 11/780 的 Dhrystones/s，定义为 1 DMIPS；`contexts` 为实例数
（默认 2），`runs` 为每个实例的运行次数。

官方程序还会打印 `Final values of the variables used in the benchmark`，
每个值后面的 `should be:` 必须与实际值一致，否则说明编译/移植破坏了运算语义，
本次结果不可信。

## 7. 相关配置与代码

| 位置 | 说明 |
| --- | --- |
| `CONFIG_DHRYSTONE_TEST` | Kconfig 开关，位于 `projects/dhrystone_test/ap/Kconfig`（CP 侧为 `cp/Kconfig`），默认关闭 |
| `projects/dhrystone_test/ap/dhrystone/` | 官方源码 + 移植层（CP 侧为 `cp/dhrystone/`） |
| `projects/dhrystone_test/ap/dhrystone/dhry_port.h` | 计时基准、HZ、DMIPS 换算基数、默认运行次数 |
| `projects/dhrystone_test/ap/dhrystone/dhry_cli.c` | `dhrystone` 命令注册与运行前环境处理（升频、停看门狗等） |
| `projects/dhrystone_test/ap/CMakeLists.txt` | 源码加入、按文件宏与编译选项 |

如需关闭该命令，在项目 defconfig 中删掉 `CONFIG_DHRYSTONE_TEST=y` 或改为
`# CONFIG_DHRYSTONE_TEST is not set` 后重新编译。

## 8. 快速开始

### 8.1 代码配置

数据位置与编译优化等级在编译前通过环境变量配置，由
`projects/dhrystone_test/ap/CMakeLists.txt` 读取；Flash Cache 通过项目 defconfig 配置。

**数据位置**

两个 `Rec_Type` 记录所在位置，默认放在片内 SRAM（SRAM5/SRAM6，0x2c180000 起）：

| 数据位置 | 配置方式 |
| --- | --- |
| 片内 SRAM（默认） | 不设置任何 `DHRYSTONE_*_DATA` |
| 高速 SRAM3（0x2c100000） | `DHRYSTONE_SRAM3_DATA=1` |
| 高速 SRAM4（0x2c140000） | `DHRYSTONE_SRAM4_DATA=1` |
| DTCM | `DHRYSTONE_DTCM_DATA=1` |

**编译优化等级**

由 `DHRYSTONE_OPT_FLAG` 控制，默认 `-Os`，可改为 `-O3`。

**Flash Cache**

默认 L1 I-cache 开、L1 D-cache 关、L2 cache 开。需要调整时修改
`ap/config/bk7259_ap/defconfig`：

```text
CONFIG_ICACHE=y
CONFIG_DCACHE=n
CONFIG_L2_CACHE_ENABLE=y
```

修改后重新编译并烧录。

### 8.2 编译与烧录

1、编译命令
`make bk7259 PROJECT=dhrystone_test`
2、烧录固件
`build/bk7259/dhrystone_test/package/all-app.bin`

### 8.3 AP 侧 Dhrystone 测试

```text
# AP 侧默认双核并行，5,000,000 次
ap_cmd dhrystone

# 若实测偏快、提示测量窗口过小，则加大次数
ap_cmd dhrystone 20000000
```

### 8.4 查看结果

确认 `DWT self-test` 的 `ratio` 接近 1.00、`Final values` 与 `should be:` 全部
一致后，读取最后一行：

```text
DMIPS (2 cores): 364.56   DMIPS/MHz (2 cores): 0.759   avg cycles/run: 1502
```

## 9. 参考测试数据

下表为 BK7259（Cortex-M55，双核 480 MHz）的实测数据，测试条件为代码位于 Flash、
L1 I-cache 开、L1 D-cache 关、L2 cache 开（库上默认配置）。数据可用于横向对比
不同数据位置与优化等级的差异，不代表其他板卡或固件版本的绝对成绩。

| 数据位 | 优化 | DMIPS  | DMIPS/MHz |
| ---    | ---  | ---    | ---       | 
| heap   | -Os  | 357.68 | 0.745     |
| heap   | -O3  | 600.84 | 1.252     |
| SRAM3  | -Os  | 484.32 | 1.009     |
| SRAM3  | -O3  | 646.07 | 1.346     |
| DTCM   | -Os  | 382.14 | 0.796     |
| DTCM   | -O3  | 723.28 | 1.507     |
