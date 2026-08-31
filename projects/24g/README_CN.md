# 私有 2.4GHz（BK24G）收发示例工程

* [English](./README.md)

## 功能简介

本工程将 BK7259 开发板实现为**私有 2.4GHz（BK24G）**收发设备。BK24G 是 Beken 基于 2.4GHz ISM 频段的私有点对点协议，收发机制类似 nRF24 的 Enhanced ShockBurst（支持多接收管道、ACK 应答负载与自动重传），不经过蓝牙或 Wi-Fi 协议栈。开发板上电后自动初始化 2.4G 控制器并进入接收（RX）模式，通过串口命令行可完成：

- 初始化 / 去初始化 2.4G 控制器；
- 在发送（TX）与接收（RX）模式间切换；
- 发送数据（可选是否请求 ACK）；
- 配置射频信道、空口速率与 ACK 应答负载；
- 复位控制器并清空收发 FIFO。

> 说明：本工程为**私有 2.4G**，业务逻辑运行在 **CP 核**。由于时钟资源冲突，**2.4G 无法与蓝牙共存**，工程默认关闭蓝牙（`CONFIG_BLUETOOTH=n`）。

工作方式（点对点，一发一收）：

```text
发送端：2.4g_demo send  ──► 空口 ──►  接收端：收到数据
                        ◄── ACK ◄──   接收端：（可选）回传 ACK payload
```

## 快速上手（两板对测）

> 前提条件：两块 BK7259 开发板（一发一收）、编译与烧录环境、串口终端。

1. **编译固件**

   ```bash
   make bk7259 PROJECT=24g
   ```

2. **烧录固件** 到两块开发板（记为 A、B）。

3. **打开串口终端**。上电后两板均已自动初始化 2.4G 并进入 RX 模式。

4. **确认信道与速率一致**（两板默认相同即可；如需修改，两板须设为同值）：

   ```text
   2.4g_demo set_channel 10    # 两板设为同一信道
   2.4g_demo set_dr 0          # 两板设为 1Mbps
   ```

5. **收发验证**：B 保持接收，A 发送（`send` 内部会自动切到 TX 模式）：

   ```text
   2.4g_demo send 16           # A 板发送 16 字节
   ```

   B 板串口应打印收到的数据；A 板应打印发送 / ACK 结果。

## 典型使用流程

以“A 发 B 收 + ACK 回传”为例串联常用命令（命令详解见 [4.2.2 串口 CLI 命令](#422-串口-cli-命令)）：

```text
# 两板共同前置：统一信道与速率
2.4g_demo set_channel 10
2.4g_demo set_dr 0

# 接收端 B：进入 RX，并预置 ACK payload（收到数据后随 ACK 回传给 A）
2.4g_demo txrx 1
2.4g_demo set_ack 16

# 发送端 A：发送数据（带 ACK 请求）
2.4g_demo send 16
# 若只做单向发送、不需要 ACK：
2.4g_demo send_no_ack 16
```

> 提示：`CMDRSP:OK` 仅表示命令已被接受，不代表空口收发成功；实际是否收到数据、是否有 ACK，需查看两板串口的收发日志。

## 1. 项目概述

本工程用于演示 Beken 平台上的**私有 2.4GHz（BK24G）点对点收发**能力，主要包含：

- 2.4G 控制器的初始化 / 去初始化 / 复位；
- TX / RX 模式切换；
- 数据发送（带 ACK / 不带 ACK）；
- 射频信道、空口速率、ACK 应答负载配置。

底层收发由 SDK 的 `bk_24g` 组件（`CONFIG_BK24G=y`）提供，工程侧仅注册 `2.4g_demo` 串口命令并在上电时完成默认初始化。

### 1.1 测试环境

- 硬件配置
  - 目标芯片：BK7259 ×2（一发一收）
  - 天线：板载或外接 2.4GHz 天线
- 输出内容
  - 2.4G 初始化与模式切换日志
  - 发送结果（含 ACK 状态）与接收数据日志

> **注意**：请使用参考板卡进行示例学习与验证。两块板必须使用**相同的信道与空口速率**才能互通。

## 2. 目录结构

项目采用 AP-CP 双核结构。2.4G 业务全部位于 **CP 侧**；AP 侧仅完成系统基础初始化（不承载 2.4G 业务）：

```text
24g/
├── ap/
│   ├── ap_main.c                       # AP 入口：bk_init（AP 侧无 2.4G 业务）
│   └── config/bk7259_ap/defconfig      # AP 侧 Kconfig 默认覆盖
├── cp/
│   ├── cp_main.c                       # CP 入口：bk_init → 上电初始化 bk24 并进 RX → 注册 2.4g_demo CLI
│   ├── 2.4g/
│   │   ├── 2.4g_demo_cli.c             # `2.4g_demo` CLI 命令实现
│   │   └── 2.4g_demo.h
│   ├── vnd_cal.c / vnd_cal.h           # 射频校准参数覆盖（CONFIG_OVERRIDE_VND_CAL 时编入）
│   └── config/bk7259/defconfig         # CP 侧 Kconfig 默认覆盖（CONFIG_BK24G=y 等）
├── partitions/bk7259/                  # flash / ram 分区表
├── Makefile
└── CMakeLists.txt
```

真正的 2.4G 收发驱动位于 SDK **可复用组件**：

| 组件 | 作用 |
| --- | --- |
| `components/bk_24g`（`bk_24g_api.h` / `bk_24g.h`） | 2.4G 控制器驱动：上电/时钟/中断、TX/RX、发送（带/不带 ACK）、信道/速率/地址/重传配置、收发回调 |

## 代码导读（修改 / 扩展代码参考）

demo 入口位于 `cp/cp_main.c`，上电后完成 2.4G 初始化并注册命令：

```c
int main(void)
{
    rtos_set_user_app_entry(user_app_main);  // 用户入口
    bk_init();                               // SDK 基础初始化
    cli_24g_demo_init();                     // 注册 2.4g_demo 串口命令
    return 0;
}

static void user_app_main(void)
{
    bk_24g_os_adapter_init();                // 2.4G OS 适配层初始化
    bk24_init();                             // 初始化 2.4G 控制器（默认 TX 参数）
    bk24_switch_to_tx_rx(1);                 // 默认进入 RX 模式
}
```

完整的 2.4G 收发接口定义参见 `components/bk24/bk_24g.h`。

常见修改点：

- **默认上电模式**：`cp_main.c` 的 `user_app_main`（当前默认 `bk24_switch_to_tx_rx(1)` 进 RX）。
- **新增自定义命令**：在 `cp/2.4g/2.4g_demo_cli.c` 的 `cmd_parse` 中新增分支。
- **射频校准**：`cp/vnd_cal.c`（`CONFIG_OVERRIDE_VND_CAL` 生效时覆盖默认校准参数）。

## 3. 功能说明

### 3.1 当前支持的功能

- 上电自动初始化 2.4G 控制器并进入 RX 模式
- TX / RX 模式切换
- 数据发送：带 ACK 请求 / 不带 ACK
- 射频信道（0~127）与空口速率（1Mbps / 2Mbps）配置
- ACK 应答负载（RX 侧预置，随 ACK 回传）
- 控制器复位与 FIFO 清空
- 串口 CLI 手动控制

> `bk_24g` 组件另提供地址配置（`bk24_set_tx_addr` / `bk24_set_rx_addr`）、自动重传（`bk24_set_retran`）、收发回调等接口，本 demo 的 CLI 未全部暴露，可按需在 `2.4g_demo_cli.c` 中扩展。

## 4. 编译与运行

### 4.1 编译方法

```bash
make bk7259 PROJECT=24g
```

### 4.2 运行方式

烧录固件后，通过 CP 主串口观察启动日志，并使用 `2.4g_demo` 命令控制 2.4G 收发。

#### 4.2.1 默认配置

CP 侧默认配置位于 `cp/config/bk7259/defconfig`，关键开关如下：

```text
CONFIG_SOC_SMP=y            # AP-CP 双核
CONFIG_FREERTOS_SMP=y
CONFIG_BK24G=y             # 2.4G（bk24）驱动
CONFIG_BLUETOOTH=n         # 关闭蓝牙（本工程不使用）
CONFIG_MAC802154=n         # 关闭 802.15.4
CONFIG_AT_CMD=y
```

#### 4.2.2 串口 CLI 命令

命令行运行在 **CP 侧**，直接在 CP 主串口输入 `2.4g_demo <子命令>` 即可（无需 `ap_cmd` 转发）。

> 参数格式不确定时，可执行 `2.4g_demo -h` 查看帮助。

**控制器与模式**

| 命令 | 说明 |
| --- | --- |
| `2.4g_demo init [0\|1]` | 初始化（`1`，默认）/ 去初始化（`0`）2.4G 控制器 |
| `2.4g_demo txrx <0\|1>` | 切换模式：`0`=TX，`1`=RX；切到 RX 时会预置一段 16 字节 ACK payload |
| `2.4g_demo reset` | 复位控制器并清空收发 FIFO |

**数据发送**

| 命令 | 说明 |
| --- | --- |
| `2.4g_demo send [len]` | 发送 `len` 字节（默认 16，建议 ≤ 32），请求 ACK；命令内部会先切到 TX 模式 |
| `2.4g_demo send_no_ack [len]` | 发送 `len` 字节但不请求 ACK |

**参数配置**

| 命令 | 说明 |
| --- | --- |
| `2.4g_demo set_channel <0-127>` | 设置射频信道（2400MHz 偏移，步进 1MHz） |
| `2.4g_demo set_dr <0\|1>` | 设置空口速率：`0`=1Mbps，`1`=2Mbps |
| `2.4g_demo set_ack [len] [pipe]` | RX 模式下为指定 pipe（0~5）预置 ACK payload |

命令提交成功时返回 `CMDRSP:OK`，失败时返回 `CMDRSP:ERROR`。

#### 4.2.3 如何判断测试成功或失败

`CMDRSP:OK` 仅表示 CLI 命令已被接受，**不代表空口收发成功**。请结合两板收发日志判断：

- 发送端 `send` 后应打印发送结果；带 ACK 时可见 ACK 是否收到；
- 接收端应打印收到的数据内容与长度；
- 若接收端无任何数据，优先检查两板信道与速率是否一致、天线与距离是否正常。

## 5. 注意事项与常见问题

1. **信道与速率必须一致**：两块板的 `set_channel` 与 `set_dr` 必须相同，否则无法互通。
2. **发送需在 TX 模式**：`send` / `send_no_ack` 命令内部会切到 TX；一块板发送期间无法同时接收。
3. **数据长度限制**：单包数据一般 ≤ 32 字节。
4. **ACK payload 由接收端预置**：需在 RX 侧先 `set_ack`，其内容会随 ACK 回传给发送端。
5. **命令运行在 CP 侧**：直接输入 `2.4g_demo ...`，与需要 `ap_cmd` 转发的 AP 侧工程不同。
6. **上电默认 RX**：固件启动后默认已 `init` 并进入接收模式，发送前无需再手动 `init`。

**常见问题（FAQ）**

- **接收端收不到数据**：确认两板 `set_channel` / `set_dr` 一致；确认发送端确实处于 TX、接收端处于 RX；检查天线与收发距离。
- **`send` 报错或返回 `CMDRSP:ERROR`**：确认已完成 `init`、长度参数合法（≤ 32）、当前处于 TX 模式。
- **带 ACK 发送但收不到 ACK**：确认接收端在位、处于 RX 且已 `set_ack`；必要时降低速率（`set_dr 0`）或更换信道。
