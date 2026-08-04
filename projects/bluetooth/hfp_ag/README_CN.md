# HFP AG 示例

* [English](./README.md)

## 概述

本示例展示 BK7258 平台作为蓝牙 HFP Audio Gateway（AG）的基本使用方法。设备模拟电话侧，可与支持 HFP Hands-Free（HF）角色的蓝牙耳机或免提设备建立 Service Level Connection（SLC）和 SCO/eSCO 音频链路。

示例通过 `hfp_ag` 命令行提供呼叫状态模拟、音频链路控制、编解码器选择、电量与音量上报等功能，并演示如何使用 `bk_dm_hfp_ag.h` 中的公共 API 处理 HF 发来的 AT 命令。

固件启动后默认完成蓝牙管理器、HFP AG 和 CLI 初始化，蓝牙名称格式为 `bk_hfp_ag_XXXXXX`，并进入可连接、可发现状态。

## 命令总览

所有功能通过 `hfp_ag` 命令提供，详细用法见 [命令行接口](#命令行接口)，也可在串口执行 `hfp_ag -h` 查看。

| 命令 | 说明 |
| --- | --- |
| `hfp_ag init` | 手动初始化（启动时已自动执行，通常无需再执行） |
| `hfp_ag connect <xx:xx:xx:xx:xx:xx>` | 连接指定 HF 设备 |
| `hfp_ag disconnect` | 断开当前 HFP 连接 |
| `hfp_ag incoming [number]` | 模拟来电 |
| `hfp_ag answer` | 将当前呼叫标记为已接通 |
| `hfp_ag hangup` | 结束或拒绝当前呼叫并断开 SCO |
| `hfp_ag dial [number]` | 模拟外拨；无号码时重拨上一次号码 |
| `hfp_ag audio on\|off` | 建立或断开 SCO/eSCO 音频 |
| `hfp_ag codec cvsd\|msbc` | 设置下一次音频连接的编解码器偏好 |
| `hfp_ag battery <0-5>` | 上报 AG 电量指示值 |
| `hfp_ag vgs <0-15>` | 设置 HF 的 speaker 音量 |
| `hfp_ag vgm <0-15>` | 设置 HF 的 mic 音量 |
| `hfp_ag cmd <at-result-code>` | 发送自定义命令 |

## 支持的功能

- HFP AG RFCOMM、SLC 和 SCO/eSCO 链路管理
- CVSD（8 kHz）与 mSBC（16 kHz）编解码器协商
- AG 麦克风与 HF 之间的双向语音传输
- 模拟来电、接听、挂断、外拨和重拨
- `RING`、`+CLIP`、`+CIEV`、`+CIND`、`+COPS`、`+CLCC`、`+CNUM` 等 HFP 消息处理
- `AT+BVRA`、`AT+VTS`、`AT+NREC`、`AT+CHLD`、`AT+BTRH` 等 HF 请求处理
- AG 电量、扬声器音量和麦克风音量上报
- SDP、BRSF 和 CHLD 能力配置
- `AT+CMEE` 扩展错误码处理
- Apple `AT+XAPL` 和 `AT+IPHONEACCEV` 扩展命令处理

## 主要组件

- **HFP AG Demo**：维护示例呼叫状态并处理 HFP AG 回调
- **HFP AG CLI**：解析 `hfp_ag` 命令
- **Bluetooth Manager**：负责蓝牙初始化、配对、重连和链路状态管理
- **Audio Record / Audio Play**：提供板端麦克风采集和扬声器播放
- **HFP AG Host**：解析 HFP AT 命令、维护协议状态并调用应用回调

公共 HFP AG API 定义位于：

```text
ap/include/components/bluetooth/bk_dm_hfp_ag.h
```

## 测试环境

### 硬件

- BK7258 开发板
- 板端或外接麦克风
- 板端或外接扬声器
- 支持 HFP HF 角色的蓝牙耳机、免提设备，或另一个运行 HFP HF 示例的开发板
- 串口终端

音频 GPIO、麦克风和扬声器电路可能因开发板不同而变化，请根据实际硬件检查：

```text
ap/config/bk7258_ap/usr_gpio_cfg.h
cp/config/bk7258/usr_gpio_cfg.h
```

### 软件

- BK7258 工具链和烧录环境
- 串口终端
- 已知远端 HF 设备的蓝牙地址，或能够让远端设备进入配对模式

## 项目结构

```text
hfp_ag/
├── ap/
│   ├── hfp_ag/
│   │   ├── hfp_ag_demo.c              # HFP AG 状态机、回调和音频处理
│   │   ├── hfp_ag_demo.h              # Demo 对外接口
│   │   ├── hfp_ag_demo_cli.c          # hfp_ag CLI 命令
│   │   ├── ring_buffer_particle.c     # SCO 音频缓存
│   │   └── ring_buffer_particle.h
│   ├── storage/
│   │   ├── bluetooth_storage.c        # 蓝牙配对信息存储
│   │   └── bluetooth_storage.h
│   ├── ap_main.c                      # AP 核启动及 Demo 自动初始化
│   ├── bt_manager.c                   # 蓝牙链路与配对管理
│   ├── bt_manager.h
│   ├── bluetooth_user_config.h        # 蓝牙名称及连接参数
│   ├── Kconfig.projbuild
│   └── config/bk7258_ap/config
├── cp/
│   ├── cp_main.c
│   └── config/bk7258/
├── partitions/bk7258/                 # 分区和内存区域配置
├── README_CN.md
├── README.md
├── Makefile
└── CMakeLists.txt
```

## 编译与运行

在 SDK 根目录执行：

```bash
make bk7258 PROJECT=bluetooth/hfp_ag
```

编译完成后，将生成的固件烧录到 BK7258 开发板，并打开串口终端。

命令执行成功时返回：

```text
CMDRSP:OK
```

命令格式错误或初始化失败时返回：

```text
CMDRSP:ERROR
```

查看 CLI 帮助：

```text
hfp_ag -h
```

## 快速开始

### 1. 建立 HFP 连接

固件启动时 HFP AG 已自动初始化，无需再次执行 `hfp_ag init`。

让远端 HF 设备进入配对模式，然后使用其蓝牙地址发起连接：

```text
hfp_ag connect 11:22:33:44:55:66
```

连接成功后，日志中应出现 RFCOMM 已连接和 `SLC connected`。只有 SLC 建立后，呼叫控制和 SCO 音频命令才能正常工作。

断开连接：

```text
hfp_ag disconnect
```

### 2. 模拟来电

```text
hfp_ag incoming 10010
```

AG 向 HF 发送来电状态、`RING` 和（HF 已启用 `AT+CLIP=1` 时）号码信息。可在 HF 上接听，也可在串口执行：

```text
hfp_ag answer
hfp_ag audio on
```

挂断：

```text
hfp_ag hangup
```

`hangup` 会同时结束呼叫状态并断开 SCO 音频。

### 3. 模拟外拨

```text
hfp_ag dial 10086
```

该命令依次上报拨号和振铃状态，并自动请求建立 SCO 音频。执行不带号码的 `dial` 可重拨上一次保存的号码：

```text
hfp_ag dial
```

将外拨呼叫标记为已接通：

```text
hfp_ag answer
```

### 4. 测试双向语音

```text
hfp_ag audio on
hfp_ag audio off
```

音频链路建立后：

- BK7258 麦克风数据发送到 HF
- HF 麦克风数据播放到 BK7258 扬声器

## 命令行接口

### 初始化与连接

```text
# 手动初始化。项目默认已在启动时自动初始化，通常不需要再次执行
hfp_ag init

# 连接指定 HF 设备
hfp_ag connect <xx:xx:xx:xx:xx:xx>

# 断开当前 HFP 连接
hfp_ag disconnect
```

### 呼叫控制

```text
# 模拟来电；未提供号码时使用 10010
hfp_ag incoming [number]

# 将当前呼叫标记为已接通
hfp_ag answer

# 结束或拒绝当前呼叫，同时断开 SCO
hfp_ag hangup

# 模拟外拨；未提供号码时重拨上一次号码
hfp_ag dial [number]
```

HF 发来的 ATA、AT+CHUP、ATD 和 AT+BLDN 也会由 Demo 回调自动处理。

### 音频和编解码器

```text
# 建立或断开 SCO/eSCO 音频
hfp_ag audio on
hfp_ag audio off

# 设置下一次音频连接的编解码器偏好
hfp_ag codec cvsd
hfp_ag codec msbc
```

应在 HF 通过 `AT+BAC` 上报相应能力后选择 mSBC。若 HF 不支持所选编解码器，Host 会回退或拒绝该选择。

### 电量和音量

```text
# 上报 AG 电量指示值，范围 0-5
hfp_ag battery <0-5>

# 设置 HF 的 speaker 音量，范围 0-15
hfp_ag vgs <0-15>

# 设置 HF 的 mic 音量，范围 0-15
hfp_ag vgm <0-15>
```

超出范围的电量或音量值会被限制到有效最大值。SLC 未建立时，电量值会先保存，暂不上报。

### 调试命令

```text
# 向 HF 发送一条原始 AT result code
hfp_ag cmd <at-result-code>

# result code 含逗号或空格时，必须用双引号括起来
hfp_ag cmd "+CIEV: 2,1"
```

`cmd` 发送的是 AG 到 HF 的 result code，并不是让 AG 接收一条 AT 命令。

> **注意**：CLI 分词器会把**空格和逗号都当作分隔符**，所以当 `<at-result-code>` 含有逗号或空格时，必须用**双引号**把整串括起来（引号内的空格和逗号不再被拆分），否则命令会在第一个逗号/空格处被截断。例如 `hfp_ag cmd "+CIEV: 2,1"`。同理，从其他设备发裸 AT 时也要加引号，如 `ap_cmd headset hfpcmd "AT+IPHONEACCEV=2,1,9,2,1"`。

## 典型测试流程

### 来电与耳机接听

```text
hfp_ag connect 11:22:33:44:55:66
hfp_ag incoming 10010
```

随后在 HF 设备上按接听键。Demo 收到 ATA 后会回复 `OK` 并更新呼叫状态。若未自动建立音频，再执行：

```text
hfp_ag audio on
```

在 HF 上按挂断键，或执行：

```text
hfp_ag hangup
```

### mSBC 双向语音

```text
hfp_ag connect 11:22:33:44:55:66
hfp_ag codec msbc
hfp_ag audio on
```

连接日志中的 codec 应显示为 mSBC；若只建立 CVSD，请确认 HF 是否在 `AT+BAC` 中声明 mSBC。

## 实现说明

### 初始化与能力配置

`hfp_ag_demo_init()` 按以下顺序初始化：

1. 注册 HFP AG 回调
2. 初始化 HFP AG Host
3. 查询并设置 SDP、BRSF 和 CHLD 能力
4. 注册 Bluetooth Manager 回调
5. 注册 SCO 音频数据回调

能力配置必须在 `bk_bt_hf_ag_init()` 之后、SLC 建立之前完成。

### AT 请求与应用回复

Host 负责解析 HF 发来的 AT 命令。需要应用决定结果的请求通过 `BK_HF_AG_*_REQ_EVT` 回调上报：

- CIND、COPS、CLCC、CNUM 使用对应的 `bk_bt_hf_ag_*_response()` API 回复
- BVRA、VTS、NREC、ATA、CHUP、DIAL 和 CHLD 使用 `bk_bt_hf_ag_cmee_send()` 回复 `OK`、`ERROR` 或 `+CME ERROR`
- BTRH 先使用 `bk_bt_hf_ag_btrh_response()` 回复状态，再发送最终结果码
- 未识别 AT 命令由应用通过 `bk_bt_hf_ag_unknown_at_send()` 和 `bk_bt_hf_ag_cmee_send()` 回复

只有 HF 先发送 `AT+CMEE=1` 时才会发送 `+CME ERROR:<code>`；否则 Host 自动降级为普通 `ERROR`。

### SCO 音频路径

CVSD 使用 8 kHz，mSBC 使用 16 kHz。SCO 建立事件携带协商后的 codec、包长和传输间隔，Demo 据此启动录音与播放路径。

当前 Demo 的板端音频实现支持 CVSD 和 mSBC。Host 中的 LC3-SWB 协议支持不代表本 Demo 已实现 LC3 音频数据处理。

## 注意事项

- 本示例使用单个 peer 和简化呼叫状态，不能直接作为完整电话业务实现。
- CHLD、多方通话和电话簿拨号主要用于演示协议交互，实际产品需要接入真实呼叫管理。
- 建立 SCO 前必须先建立 SLC。
- mSBC 必须得到远端 HF 能力支持。
- 不同开发板的音频增益、GPIO 和模拟前端可能需要重新配置。
- `hfp_ag dial hangup` 会把 `hangup` 当作号码；正确的挂断命令是 `hfp_ag hangup`。
- Apple 扩展命令为非标准 HFP 功能，仅在对应设备发送命令时生效。
