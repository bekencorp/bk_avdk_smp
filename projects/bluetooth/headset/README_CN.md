# 蓝牙 Headset 示例工程

* [English](./README.md)

## 工程简介

本工程将 BK7258 开发板作为经典蓝牙音箱和免提设备使用。手机可通过 A2DP 将音乐推送至开发板，开发板可通过 AVRCP 控制手机播放器，并以 HFP Hands-Free（HF）角色处理通话。

工程默认配置开启 A2DP Sink、HFP HF 和 PBAP 电话簿客户端（PCE）demo。

## 支持功能

- A2DP Sink：接收手机或其他 A2DP Source 发送的音乐并本地播放
- AVRCP Controller：控制远端播放、切歌、快进/快退、音量、延时值和媒体属性查询
- HFP HF：语音识别控制、拨号、接听/拒接、SCO 通话语音和自定义 AT 命令
- PBAP PCE：电话簿访问 demo，详细说明见 `ap/pbap_pce/README.txt` 和 `README_CN.txt`
- 配对、主动连接/断开和串口 CLI 控制

> 本工程为经典蓝牙（BR/EDR）工程，不是 BLE 应用。

## 硬件与测试环境

- BK7258 参考开发板
- 用于 A2DP 播放的扬声器，以及用于 HFP 通话的麦克风
- 支持经典蓝牙 A2DP、AVRCP 和 HFP 的手机
- 用于查看日志和输入命令的串口终端

## 目录结构

```text
headset/
├── ap/
│   ├── ap_main.c                         # AP 入口和 demo 初始化
│   ├── bt_manager.c                      # 设备名、配对和重连策略
│   ├── headset_user_config.h             # 本地设备名和音频相关默认配置
│   ├── a2dp_sink/                        # A2DP Sink 音频/解码处理
│   ├── a2dp_sink_demo_cli.c              # `headset` CLI
│   ├── hfp_hf/                           # HFP HF 和 SCO 语音处理
│   ├── pbap_pce/                         # PBAP PCE demo 及其文档
│   └── config/bk7258_ap/config           # AP 配置
├── cp/                                   # CP 启动和控制器配置
└── partitions/bk7258/                    # BK7258 分区定义
```

启动时，AP 会初始化 SDK、媒体服务、蓝牙管理、A2DP Sink、HFP HF、PBAP PCE 及相应 CLI。A2DP AAC 和 HFP mSBC 初始化参数均为 `0`，因此验证时默认使用 SBC 和 CVSD 编解码。

## 编译与烧录

在 SDK 根目录执行：

```bash
CCACHE_DISABLE=1 make bk7258 PROJECT=bluetooth/headset -j$(nproc)
```

按 BK7258 常规流程烧录固件，连接音频外设后打开串口终端。

## 快速上手

1. 使开发板进入可发现状态：

   ```text
   headset pair_mode
   ```

2. 在手机端搜索并连接 `soundbar_XXYYZZ`；后缀来自本机蓝牙 MAC 地址。
3. 在手机上播放音乐，声音应从开发板扬声器输出。
4. 如需串口控制播放：

   ```text
   headset pause
   headset play
   headset next
   ```

5. 来电时可接听或拒接：

   ```text
   headset answer 1
   headset answer 0
   ```

## CLI 命令

以下为 AP 侧 CLI 命令。通过默认 CP 串口发送时，需在命令前加 `ap_cmd`，例如 `ap_cmd headset pair_mode`；直接使用 AP CLI 时，则按下表原样输入。以下命令表为准：内置 `headset -h` 只列出基础媒体命令，并未列出全部已支持子命令。

| 命令 | 说明 |
| --- | --- |
| `headset pair_mode` | 进入可配对/可发现状态。 |
| `headset connect <MAC>` / `disconnect <MAC>` | 主动连接或断开远端设备。 |
| `headset play` / `pause` / `prev` / `next` | 发送 AVRCP 播放控制。 |
| `headset rewind [ms]` / `fast_forward [ms]` | 快退/快进；默认时长为 500 ms。 |
| `headset vol_up` / `vol_down` | 发送 AVRCP 音量控制。 |
| `headset set_delay_value <value>` / `get_delay_value` | 设置或读取 16 位 A2DP 延时值。 |
| `headset get_attr <id>` | 查询 AVRCP 媒体属性。 |
| `headset vr <0\|1>` | 关闭或打开 HFP 语音识别。 |
| `headset dial <0\|1> [number]` | 控制拨号；默认号码为 `112`。 |
| `headset answer <0\|1>` | `1` 接听，`0` 拒接或挂断。 |
| `headset hfpcmd <AT command>` | 发送自定义 HFP AT 命令。 |

`<MAC>` 必须使用 `XX:XX:XX:XX:XX:XX` 格式。

## 验证方法

`CMDRSP:OK` 表示命令已被接受；请通过 profile 和音频日志确认业务完成：

- 手机可发现并连接 `soundbar_XXYYZZ`；
- A2DP 连接完成后，手机音乐可从本地扬声器播放；
- 拨号、接听或拒接产生预期的 HFP 事件，通话时 SCO 链路的麦克风和扬声器正常工作；
- PBAP PCE 请按模块 README 中的步骤验证。

## 注意事项

- `ap/headset_user_config.h` 的 `LOCAL_NAME` 控制 `soundbar` 设备名前缀。
- 默认配置位于 `ap/config/bk7258_ap/config`，demo 开关位于 `ap/Kconfig.projbuild`。
- 音频外设接线、通道数和增益需与参考板配置匹配。
