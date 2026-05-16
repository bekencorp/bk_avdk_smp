# 蓝牙 Headset 示例工程

* [English](./README.md)

## 1. 项目概述

本工程用于演示 Beken 平台上的经典蓝牙耳机/音箱类应用能力，主要包含：

- A2DP Sink：接收手机或其他蓝牙音源的音乐数据并播放
- AVRCP Controller：控制远端播放器的播放、暂停、上一曲、下一曲和音量
- HFP HF：作为免提设备接入手机通话链路，支持语音、拨号、接听和自定义 AT 命令

工程上电后会初始化蓝牙管理模块、媒体服务、A2DP Sink、HFP HF，并注册 `headset` 串口 CLI，便于手动触发连接、媒体控制和通话相关操作。

### 1.1 测试环境

- 硬件配置
  - 目标芯片：BK7259
  - 音频输出：板载或外接扬声器
  - 音频输入：板载或外接麦克风
- 外部设备
  - 支持 A2DP/AVRCP/HFP 的手机或其他经典蓝牙设备
- 输出内容
  - 蓝牙连接与 profile 状态日志
  - A2DP 音频播放日志
  - AVRCP 控制日志
  - HFP 通话与语音链路日志

.. warning::

    请使用参考板卡和音频外设进行示例学习与验证。如果外设规格不同，音频通道、增益和板级配置可能需要同步调整。

## 2. 目录结构

项目采用 AP-CP 双核结构，主要业务逻辑位于 AP 侧：

```text
headset/
├── README.md
├── README_CN.md
├── ap/
│   ├── ap_main.c                    # AP 主入口，初始化媒体服务、蓝牙 demo 和 CLI
│   ├── a2dp_sink/
│   │   ├── a2dp_sink_demo.c         # A2DP Sink 与 AVRCP 控制逻辑
│   │   ├── ring_buffer_node.c       # A2DP 播放缓存
│   │   └── mpeg4_latm_dec.c         # AAC LATM 解码辅助逻辑
│   ├── hfp_hf/
│   │   ├── hfp_hf_demo.c            # HFP HF 通话与 SCO 音频逻辑
│   │   └── ring_buffer_particle.c   # HFP 语音缓存
│   ├── a2dp_sink_demo_cli.c         # headset CLI 实现
│   ├── bt_manager.c                 # 蓝牙管理初始化与状态处理
│   └── storage/bluetooth_storage.c  # 蓝牙配对信息存储
├── cp/
├── bk7259_bsp.ld
└── .it.csv
```

## 3. 功能说明

### 3.1 当前支持的功能

- 开机自动初始化经典蓝牙与媒体服务
- A2DP Sink 音频接收与本地播放
- AVRCP 播放控制：播放、暂停、上一曲、下一曲、快退、快进、音量调节
- A2DP 延时值设置与读取
- HFP HF 通话控制：语音识别开关、拨号、接听/拒接、自定义 AT 命令
- 串口 CLI 手动控制 headset demo

## 4. 编译与运行

### 4.1 编译方法

```bash
make bk7259 PROJECT=bluetooth/headset
```

### 4.2 运行方式

烧录固件后，通过串口终端观察启动日志，并使用 `headset` CLI 控制蓝牙连接和音频业务。

#### 4.2.1 默认配置

AP 侧默认配置已使能蓝牙和音频相关能力：

```text
CONFIG_BT=y
CONFIG_BLUETOOTH_AP=y
CONFIG_MEDIA_SERVICE=y
CONFIG_AUDIO=y
CONFIG_AUDIO_PLAY=y
CONFIG_AUDIO_RECORD=y
CONFIG_A2DP_SINK_DEMO=y
CONFIG_HFP_HF_DEMO=y
```

当前启动代码中使用：

```text
a2dp_sink_demo_init(0)
hfp_hf_demo_init(0)
```

因此默认 A2DP AAC 支持和 HFP mSBC 支持没有打开，验证时优先使用 SBC 音乐播放和 CVSD 通话链路。

#### 4.2.2 串口 CLI 命令

本工程的 CLI 运行在 AP 侧，因此串口命令需要通过 `ap_cmd` 转发到 AP 侧执行。以下命令中的 `XX:XX:XX:XX:XX:XX` 为手机或其他蓝牙设备的 MAC 地址。

帮助命令：

- `ap_cmd headset -h`：打印 `headset` 命令帮助，查看当前支持的子命令和参数格式。

连接管理命令：

- `ap_cmd headset pair_mode`：让设备进入可配对/可发现状态，便于手机或其他蓝牙设备搜索并发起配对连接。
- `ap_cmd headset connect XX:XX:XX:XX:XX:XX`：主动连接指定 MAC 地址的远端蓝牙设备，常用于连接已配对手机并建立 A2DP/AVRCP/HFP 相关链路。
- `ap_cmd headset disconnect XX:XX:XX:XX:XX:XX`：断开指定 MAC 地址的远端蓝牙设备连接，用于结束当前蓝牙业务或切换测试设备。

媒体播放控制命令：

- `ap_cmd headset play`：通过 AVRCP 向远端播放器发送播放命令，通常在 A2DP 已连接后使用。
- `ap_cmd headset pause`：通过 AVRCP 向远端播放器发送暂停命令。
- `ap_cmd headset next`：通过 AVRCP 切换到下一首媒体。
- `ap_cmd headset prev`：通过 AVRCP 切换到上一首媒体。
- `ap_cmd headset rewind 500`：通过 AVRCP 触发快退操作，参数为持续时间，单位为毫秒；示例中的 `500` 表示快退 500 ms。
- `ap_cmd headset fast_forward 500`：通过 AVRCP 触发快进操作，参数为持续时间，单位为毫秒；示例中的 `500` 表示快进 500 ms。
- `ap_cmd headset vol_up`：调高远端或本地播放音量，具体效果取决于当前 AVRCP 音量同步和音频播放状态。
- `ap_cmd headset vol_down`：调低远端或本地播放音量，具体效果取决于当前 AVRCP 音量同步和音频播放状态。

A2DP 延时控制命令：

- `ap_cmd headset set_delay_value 100`：设置 A2DP Sink 上报给远端设备的音频延时值，参数为 16 位数值；示例中的 `100` 表示设置延时值为 100。
- `ap_cmd headset get_delay_value`：读取当前 A2DP Sink 延时值，用于确认延时配置是否生效。

AVRCP 属性查询命令：

- `ap_cmd headset get_attr 1`：查询远端播放器的指定媒体属性，参数为属性 ID；示例中的 `1` 表示查询 ID 为 1 的属性，具体属性含义以 AVRCP 协议和远端设备实现为准。

HFP HF 通话控制命令：

- `ap_cmd headset dial 1 10086`：发起 HFP 拨号流程，第一个参数用于控制拨号动作，第二个参数为电话号码；示例表示拨打 `10086`。
- `ap_cmd headset answer 1`：接听当前来电或通话请求。
- `ap_cmd headset answer 0`：拒接或挂断当前来电/通话请求，实际行为取决于当前 HFP 通话状态。
- `ap_cmd headset hfpcmd AT+BRSF`：发送自定义 HFP AT 命令到远端设备，用于调试 HFP 协议交互；示例发送 `AT+BRSF`。

命令提交成功时返回：

```text
CMDRSP:OK
```

命令提交失败时返回：

```text
CMDRSP:ERROR
```

#### 4.2.3 如何判断测试成功或失败

`CMDRSP:OK` 仅表示 CLI 命令已被接受，不代表蓝牙业务已经完成。请结合蓝牙 profile 状态、音频播放和通话链路日志判断。

A2DP 播放验证建议检查：

- 手机端成功连接到设备，设备名默认为 `soundbar`
- 执行播放命令后，串口无连接失败、解码失败或音频播放错误日志
- 本地扬声器能够输出远端音乐

HFP 通话验证建议检查：

- 手机端能够建立免提连接
- 拨号、接听或拒接命令后，HFP 状态日志符合预期
- 通话时本地麦克风和扬声器链路工作正常

#### 4.2.4 集成测试命令

`.it.csv` 当前只覆盖设备重启基础检查：

```text
AT+RST
```

期望结果匹配 `wakeup` 日志。A2DP/HFP 业务需要配合外部蓝牙设备，通常通过手动或专项自动化测试验证。

## 5. 注意事项

1. 本工程依赖外部经典蓝牙设备，测试前请确认手机或音源设备支持 A2DP、AVRCP 和 HFP。
2. CLI 中的 MAC 地址解析顺序与底层接口要求匹配，输入格式应固定为 `XX:XX:XX:XX:XX:XX`。
3. 默认本地设备名为 `soundbar`，可在 `headset_user_config.h` 或 HFP demo 中按需求调整。
4. A2DP 与 HFP 都会使用音频播放链路，切换业务时建议先观察当前 profile 状态，避免并发场景影响判断。
5. 如果需要验证 AAC 或 mSBC，需要同步调整初始化参数、Kconfig 和远端设备能力。
