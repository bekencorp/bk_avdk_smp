# 蓝牙 A2DP Source（音乐播放器）示例工程

* [English](./README.md)

## 功能简介

本工程将 BK7259 开发板实现为蓝牙 A2DP 音源（Source）设备。开发板作为 A2DP Source（发送端）主动连接蓝牙音箱/耳机后，支持以下能力：

- 将 SD 卡中的 MP3 文件解码后，通过蓝牙推送给音箱/耳机播放；
- 通过串口命令控制播放、暂停、停止、切曲；
- 作为 AVRCP 播放器（Target），响应远端的播放控制按键，并上报当前曲目、播放进度与播放状态；
- 设置对端（音箱）的绝对音量（AVRCP）。

本文档常用缩写如下：

| 缩写 | 全称 | 在本工程里的作用 |
| --- | --- | --- |
| A2DP Source | Advanced Audio Distribution Profile（发送端） | 把本地音频编码后推送给蓝牙音箱/耳机播放 |
| AVRCP TG | Audio/Video Remote Control Profile（Target / 播放器侧） | 作为播放器响应远端的播放控制，并上报播放状态/曲目/进度 |
| AVRCP CT | Audio/Video Remote Control Profile（Controller） | 设置对端（音箱）的绝对音量、接收对端音量变化 |

> 说明：本工程基于**经典蓝牙（BR/EDR）**，非 BLE。开发板为**主动方**，连接蓝牙音箱/耳机等 A2DP Sink 设备。

数据流向（**单向下行**：开发板 → 音箱）：

```text
SD 卡 MP3 文件
    → MP3 解码 (helix)              # 解出 PCM
    → 重采样 (如源采样率≠协商采样率)  # 例如 48k → 44.1k
    → SBC / AAC-LC 编码             # 编成 A2DP 码流
    → A2DP Source 发送             # 经蓝牙链路
    → 蓝牙音箱 / 耳机 (A2DP Sink) 解码播放
```

## 快速上手（5 步）

> 前提条件：已具备 BK7259 的编译与烧录环境，并准备一台蓝牙音箱/耳机及一张存有 MP3 文件的 SD 卡。

1. **编译固件**

   ```bash
   make bk7259 PROJECT=bluetooth/central
   ```

2. **烧录固件** 到开发板，插入存有 MP3 文件的 **SD 卡**。

3. **打开串口终端**（查看启动日志、发送命令）。上电后应能看到蓝牙与媒体服务初始化完成的日志。

4. **连接蓝牙音箱/耳机**（使用目标设备的 MAC 地址）：

   ```bash
   ap_cmd a2dp_player connect XX:XX:XX:XX:XX:XX
   ```

   > 若未知音箱 MAC，可先执行 `ap_cmd a2dp_player discover` 搜索（默认 10 秒，结果打印在串口），再使用查询到的地址连接。

5. **播放音乐**：把 SD 卡里的 MP3 推给音箱播放：

   ```bash
   ap_cmd a2dp_player play 1:/music.mp3   # 1:/ 表示 SD 卡根目录
   ap_cmd a2dp_player pause               # 暂停
   ap_cmd a2dp_player resume              # 继续
   ```

至此，开发板即可作为蓝牙音乐播放器使用。更多命令与原理详见下文。

## 典型使用流程

以“连接音箱 → 播放音乐 → 遥控”为例，串联常用命令（命令详解见 [4.2.2 串口 CLI 命令](#422-串口-cli-命令)）：

```text
# 1.（可选）搜索周边蓝牙设备，获取音箱 MAC（默认搜索 10 秒，结果打印在串口）
ap_cmd a2dp_player discover
ap_cmd a2dp_player discover_cancel   # 如需提前结束搜索

# 2. 连接音箱（也可由音箱主动连接开发板，工程已做 eager init）
ap_cmd a2dp_player connect XX:XX:XX:XX:XX:XX

# 3. 播放 SD 卡中的 MP3
ap_cmd a2dp_player play 1:/music.mp3
ap_cmd a2dp_player pause
ap_cmd a2dp_player resume
ap_cmd a2dp_player next          # 当前未实现真正的曲目管理，会重播同一首

# 4. 设置音箱音量（AVRCP 绝对音量，0~0x7f）
ap_cmd a2dp_player abs_vol 60

# 5. 断开连接
ap_cmd a2dp_player disconnect XX:XX:XX:XX:XX:XX
```

> 提示：`CMDRSP:OK` 仅表示命令已被接受；能否成功建链、正常出声需查看串口的 profile 状态与音频日志（见 [4.2.3](#423-如何判断测试成功或失败)）。

## 1. 项目概述

本工程用于演示 Beken 平台上的经典蓝牙 **A2DP 音源（Source）** 类应用能力，主要包含：

- A2DP Source：读取 SD 卡里的 MP3，解码 → 重采样 → SBC/AAC 编码，通过蓝牙推送给音箱/耳机播放
- AVRCP Target（播放器）：响应远端的播放/暂停/切歌按键（passthrough），并上报播放状态、曲目变化、播放进度
- AVRCP Controller：设置对端绝对音量、接收对端音量/电量变化

工程上电后初始化媒体服务与蓝牙管理模块，并**提前**初始化 A2DP Source 与 AVRCP（eager init，以便接受音箱主动发起的连接），随后注册 `a2dp_player` 串口 CLI，用于手动触发搜索、连接与音乐播放控制。

### 1.1 测试环境

- 硬件配置
  - 目标芯片：BK7259
  - 存储：SD 卡（FATFS），用于存放待播放的 MP3 文件
- 外部设备
  - 支持 A2DP/AVRCP 的蓝牙音箱或耳机（作为 A2DP Sink）
- 输出内容
  - 蓝牙连接与 profile 状态日志
  - MP3 解码 / 重采样 / 编码 / 发送的音频链路日志
  - AVRCP 控制与上报日志

> **注意**：请使用参考板卡进行示例学习与验证。MP3 文件需存放于 SD 卡，路径以 `1:/` 开头（FATFS 盘符）。

## 2. 目录结构

项目采用 AP-CP 双核结构。AP 侧承担业务逻辑（媒体服务、蓝牙管理、A2DP Source / AVRCP demo、CLI），CP 侧仅负责拉起 AP：

```text
central/
├── ap/
│   ├── ap_main.c                       # AP 入口：bk_init → media_service_init → bt_manager_init → a2dp source demo → CLI
│   ├── a2dp_source/
│   │   ├── a2dp_source_demo.c          # A2DP Source 连接管理 + 音乐播放（启动 MP3 解码任务、向组件输送 PCM）
│   │   ├── a2dp_source_demo_cli.c      # `a2dp_player` CLI 命令实现
│   │   ├── a2dp_source_demo_avrcp.c    # AVRCP 策略：passthrough 按键处理、playback/track/position 上报、音量
│   │   ├── a2dp_source_demo.h
│   │   └── a2dp_source_demo_avrcp.h
│   └── config/bk7259_ap/defconfig      # AP 侧 Kconfig 默认覆盖（音频 / ADK / FATFS / BT）
└── cp/
    └── config/bk7259/defconfig         # CP 侧 Kconfig 默认覆盖（BT controller 等）
```

A2DP Source 收发/编码流水线与 AVRCP 逻辑位于**可复用组件**（`ap/components/bk_bluetooth/service/dm/`）；demo 仅负责“策略 + 文件读取 + 解码”：

| 组件 | 作用 |
| --- | --- |
| `service/dm/a2dp/bk_a2dp_source_service` | A2DP Source 连接状态机 + 发送流水线（ring buffer、编码回调、AVDTP start/suspend） |
| `service/dm/a2dp/bk_a2dp_source_pcm_service` | 独立 worker：重采样 + SBC/AAC 编码 |
| `service/dm/avrcp/bk_avrcp_tg_service` | AVRCP Target（播放器）：passthrough、playback/track/position 通知 |
| `service/dm/avrcp/bk_avrcp_ct_service` | AVRCP Controller：绝对音量、对端音量/电量 |
| `service/dm/bt_manager` | 单一 GAP 回调：设备名/COD/可发现性/配对/link key 存储/连接后切主 |

## 代码导读（修改 / 扩展代码参考）

demo 入口位于 `ap/ap_main.c`，上电后按固定顺序初始化：

```c
bk_init();                 // SDK 基础初始化
media_service_init();      // 媒体服务（音频播放框架）

bt_manager_init(&cfg);     // 经典蓝牙管理：设备名 a2dp_source_XXYYZZ / COD_PHONE / role=master
bt_a2dp_source_demo_init();// 提前初始化 A2DP Source + AVRCP（eager，可接住音箱主动连接）
cli_a2dp_source_demo_init();// 注册 a2dp_player 串口命令
```

各模块职责与关键函数：

| 模块 / 文件 | 关键函数 | 说明 |
| --- | --- | --- |
| `ap/ap_main.c` | `main` | 启动入口；以 `a2dp_source_` + MAC 末 3 字节生成广播名；`bt_manager_cfg_t.role=1` 使 source 在建立连接后切换为 master |
| 蓝牙管理（`bt_manager`） | `bt_manager_init(&cfg)` | 设备名、COD、page/scan 可发现性、配对 IO 能力、link key 存储、角色切换 |
| A2DP Source（`a2dp_source/a2dp_source_demo.c`） | `bt_a2dp_source_demo_init` / `bt_a2dp_source_demo_music_play` | 连接/断开、启动 MP3 解码任务、启动 AVDTP 流、向组件输送 PCM |
| AVRCP（`a2dp_source/a2dp_source_demo_avrcp.c`） | `bt_avrcp_demo_init` / `bt_avrcp_demo_report_playback` / `..._report_track_change` | 处理音箱下发的 passthrough 按键；播放状态、曲目、进度上报 |
| CLI（`a2dp_source_demo_cli.c`） | `cli_a2dp_source_demo_init` / `cmd_a2dp_player_demo` | 解析 `a2dp_player xxx` 子命令并调用上面各接口 |

常见修改点：

- **修改设备名 / 可发现性 / 角色**：`ap_main.c` 中的 `bt_manager_cfg_t`。
- **启用 AAC 编码**：默认仅使用 SBC。AAC-LC 编码由 Kconfig `BLUETOOTH_BTDM_COMPONENT_BT_A2DP_SOURCE_AAC` 控制（默认关闭，开启后会额外链接 FDK-AAC，约 230KB flash）。
- **新增自定义命令**：在 `a2dp_source_demo_cli.c` 的 `cmd_a2dp_player_demo` 中新增分支。

## 3. 功能说明

### 3.1 当前支持的功能

- 开机自动初始化经典蓝牙与媒体服务，并提前拉起 A2DP Source / AVRCP
- 搜索、连接、断开蓝牙音箱/耳机（支持开发板主动连，也支持音箱主动连）
- 播放 SD 卡上的 MP3：解码 → 重采样（按需）→ SBC 编码 → 推送播放
- 可选 AAC-LC 编码（Kconfig 开关，默认关）
- 播放控制：play / pause / resume / stop / prev / next
- AVRCP 播放器：响应远端 passthrough 按键，上报播放状态 / 曲目变化 / 播放进度
- AVRCP 绝对音量控制
- 串口 CLI 手动控制

## 4. 编译与运行

### 4.1 编译方法

```bash
make bk7259 PROJECT=bluetooth/central
```

### 4.2 运行方式

烧录固件、插入含 MP3 的 SD 卡后，通过串口终端观察启动日志，并使用 `a2dp_player` CLI 控制蓝牙连接和音乐播放。

#### 4.2.1 默认配置

AP 侧默认配置位于 `ap/config/bk7259_ap/defconfig`，关键开关如下：

```text
CONFIG_BT=y
CONFIG_BLUETOOTH_AP=y
CONFIG_MEDIA_SERVICE=y
CONFIG_AUDIO=y
CONFIG_AUDIO_PLAY=y
CONFIG_FATFS=y
CONFIG_FATFS_SDCARD=y
CONFIG_SDCARD=y
CONFIG_ADK_SBC_ENCODER=y      # SBC 编码（A2DP Source 用）
CONFIG_ADK_AAC_DECODER=y
CONFIG_BLUETOOTH_BTDM_COMPONENT_ENABLE=y
```

A2DP Source 的 AAC-LC 编码默认**关闭**（`CONFIG_BLUETOOTH_BTDM_COMPONENT_BT_A2DP_SOURCE_AAC` 默认 n，用于控制 code size）。默认验证时优先使用 SBC。

#### 4.2.2 串口 CLI 命令

本工程的 CLI 运行在 AP 侧，因此串口命令需要通过 `ap_cmd` 转发到 AP 侧执行。以下命令中的 `XX:XX:XX:XX:XX:XX` 为蓝牙音箱/耳机的 MAC 地址。

> 参数格式不确定时，可执行 `ap_cmd a2dp_player -h` 查看帮助。

**连接管理命令**

| 命令 | 说明 |
| --- | --- |
| `ap_cmd a2dp_player discover [sec] [report count]` | 搜索周边蓝牙设备；`sec` 为搜索时长（秒，默认 10），`report count` 为上报设备数上限（默认 0，即不限制）。搜索结果打印在串口 |
| `ap_cmd a2dp_player discover_cancel` | 取消正在进行的搜索 |
| `ap_cmd a2dp_player connect XX:XX:XX:XX:XX:XX` | 主动连接指定 MAC 的音箱/耳机，建立 A2DP + AVRCP 链路 |
| `ap_cmd a2dp_player disconnect XX:XX:XX:XX:XX:XX` | 断开指定 MAC 的连接 |

**音乐播放控制命令**

| 命令 | 说明 |
| --- | --- |
| `ap_cmd a2dp_player play 1:/music.mp3` | 播放 SD 卡上的 MP3 文件（`1:/` 为 SD 卡盘符），并向音箱上报“播放中” |
| `ap_cmd a2dp_player pause` | 暂停播放，并上报“暂停” |
| `ap_cmd a2dp_player resume` | 继续播放，并上报“播放中” |
| `ap_cmd a2dp_player stop` | 停止播放，并上报“停止” |
| `ap_cmd a2dp_player prev` | 上一曲（当前未实现真正曲目管理，会重播当前文件） |
| `ap_cmd a2dp_player next` | 下一曲（同上，重播当前文件） |

**音量控制命令（AVRCP）**

| 命令 | 说明 |
| --- | --- |
| `ap_cmd a2dp_player abs_vol 60` | 设置对端（音箱）绝对音量，取值 0~0x7f（0~127） |

命令提交成功时返回 `CMDRSP:OK`，失败时返回 `CMDRSP:ERROR`。

#### 4.2.3 如何判断测试成功或失败

`CMDRSP:OK` 仅表示 CLI 命令已被接受，**不代表蓝牙业务已经完成**。请结合蓝牙 profile 状态和音频链路日志判断：

- `connect` 后应能看到 A2DP / AVRCP 连接成功、协商出的 codec（SBC/AAC）与采样率日志；
- `play` 后音箱应实际出声；串口应有 MP3 解码 / 编码 / 发送相关日志；
- `abs_vol` 后音箱音量应随之变化（取决于音箱是否支持绝对音量）。

#### 4.2.4 集成测试命令

A2DP Source 业务需要配合外部蓝牙音箱与 SD 卡上的 MP3 文件，通常通过手动或专项自动化测试验证。

## 5. 注意事项与常见问题

1. **必须有蓝牙音箱/耳机**：本工程作为音源，依赖一个 A2DP Sink 对端设备。
2. **必须有 SD 卡与 MP3 文件**：`play` 的文件路径以 `1:/` 开头（FATFS SD 卡盘符），请确认卡已插好、文件存在。
3. **MAC 地址格式固定**：CLI 中的 MAC 必须写成 `XX:XX:XX:XX:XX:XX`，否则命令会解析失败。
4. **设备名**：默认前缀为 `a2dp_source`，广播名形如 `a2dp_source_XXYYZZ`（后缀为本机 BT MAC 末 3 字节），可在 `ap_main.c` 修改。
5. **AAC 编码默认关闭**：默认只推 SBC；如需 AAC-LC，请打开 Kconfig `BLUETOOTH_BTDM_COMPONENT_BT_A2DP_SOURCE_AAC`（会增加约 230KB flash），并确认音箱支持 AAC。
6. **prev / next 暂未实现真正的曲目管理**：当前会重播同一个文件，仅用于演示 AVRCP 上报链路。

**常见问题（FAQ）**

- **无法搜索到音箱**：先使音箱进入配对/可发现状态，再执行 `ap_cmd a2dp_player discover`；并确认串口日志显示蓝牙已初始化完成。
- **已连接但无声音**：确认 `play` 的 MP3 路径正确（`1:/...`）、SD 卡已插入；查看串口的解码 / 编码 / 发送日志；必要时通过 `abs_vol` 提高音量。
- **命令返回 `CMDRSP:ERROR`**：通常为参数格式错误（尤其是 MAC 地址或文件路径），可通过 `ap_cmd a2dp_player -h` 核对格式。
- **播放卡顿 / 杂音**：优先确认源文件为常规采样率（如 44.1kHz）；48kHz 会触发重采样，若音质异常需检查重采样链路与供数速率。
