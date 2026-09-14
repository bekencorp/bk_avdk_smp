# 蓝牙 A2DP Source 示例工程

* [English](./README.md)

## 工程简介

本工程将 BK7258 开发板作为经典蓝牙 A2DP 音源（Source）使用：从 SD 卡读取 MP3 文件，解码为 PCM，进行 SBC 编码后发送给蓝牙音箱或耳机（A2DP Sink）播放。

工程提供 `a2dp_player` 串口 CLI，可用于搜索设备、连接/断开、控制播放及设置 AVRCP 绝对音量。

> 本工程基于经典蓝牙（BR/EDR），不是 BLE 示例；开发板作为主动方连接蓝牙音箱或耳机。

## 支持功能

- 搜索、连接和断开蓝牙音箱/耳机
- 播放 SD 卡中的 MP3 文件（FATFS 盘符为 `1:/`）
- 播放、暂停、继续、停止、上一曲和下一曲控制
- AVRCP 播放状态上报与对端绝对音量控制
- A2DP 性能测试命令

## 硬件与测试环境

- BK7258 参考开发板
- 支持 A2DP Sink 的蓝牙音箱或耳机；验证遥控和绝对音量时，对端还需支持 AVRCP
- 已按 FATFS 格式准备且存有 MP3 文件的 SD 卡
- 用于查看日志和输入命令的串口终端

## 目录结构

```text
central/
├── ap/
│   ├── ap_main.c                         # AP 入口：初始化 SDK/媒体服务并注册 CLI
│   ├── a2dp_source/
│   │   ├── a2dp_source_demo.c            # A2DP Source 连接和 MP3 播放逻辑
│   │   ├── a2dp_source_demo_avrcp.c      # AVRCP 控制和状态上报
│   │   └── a2dp_source_demo_cli.c        # `a2dp_player` CLI
│   ├── storage/                          # 蓝牙存储辅助代码
│   └── config/bk7258_ap/config           # AP 配置
├── cp/
│   ├── cp_main.c                         # CP 入口：启动 AP 系统
│   └── config/bk7258/config              # CP 配置
└── partitions/bk7258/                    # BK7258 分区和 RAM 区域定义
```

AP 侧运行媒体服务和蓝牙业务 demo；CP 侧负责启动 AP，并提供蓝牙控制器配置。

## 编译与烧录

在 SDK 根目录执行：

```bash
CCACHE_DISABLE=1 make bk7258 PROJECT=bluetooth/central -j$(nproc)
```

按 BK7258 常规烧录流程烧录生成的固件，再插入 SD 卡。

## 快速上手

1. 上电后打开串口终端。
2. 搜索周边蓝牙设备：

   ```text
   a2dp_player discover
   ```

3. 用日志中查询到的地址连接音箱或耳机：

   ```text
   a2dp_player connect XX:XX:XX:XX:XX:XX
   ```

4. 播放 SD 卡中的 MP3：

   ```text
   a2dp_player play 1:/music.mp3
   ```

5. 使用结束后停止播放并断开连接：

   ```text
   a2dp_player stop
   a2dp_player disconnect XX:XX:XX:XX:XX:XX
   ```

## CLI 命令

以下为 AP 侧 CLI 命令。通过默认 CP 串口发送时，需在命令前加 `ap_cmd`，例如 `ap_cmd a2dp_player discover`；直接使用 AP CLI 时，则按下表原样输入。可执行 `a2dp_player -h` 查看帮助。

| 命令 | 说明 |
| --- | --- |
| `a2dp_player discover [sec] [count]` | 搜索周边蓝牙设备；默认搜索 10 秒，`count=0` 表示不限上报数量。 |
| `a2dp_player discover_cancel` | 取消正在进行的搜索。 |
| `a2dp_player connect <MAC>` | 连接指定的 A2DP Sink 设备。 |
| `a2dp_player disconnect <MAC>` | 断开指定远端设备。 |
| `a2dp_player play 1:/music.mp3` | 解码并推送 SD 卡中的 MP3 文件。 |
| `a2dp_player pause` / `resume` / `stop` | 控制当前播放。 |
| `a2dp_player prev` / `next` | 请求上一曲/下一曲；本 demo 未提供播放列表管理。 |
| `a2dp_player abs_vol <0-127>` | 设置对端 AVRCP 绝对音量。 |
| `a2dp_player test_performance [cpu_mhz] [bytes] [loops] [cpu_id]` | 运行 A2DP Source 性能测试。 |

`<MAC>` 必须采用 `XX:XX:XX:XX:XX:XX` 格式。

## 验证方法

`CMDRSP:OK` 仅表示 CLI 已接受命令，不代表蓝牙业务已完成。请结合 profile 和媒体日志确认：

- 执行 `connect` 后，应看到 A2DP/AVRCP 连接事件；
- 执行 `play` 后，对端应实际播放声音，串口应有 MP3 解码和 SBC 发送日志；
- 执行 `abs_vol` 后，若对端支持 AVRCP 绝对音量，音量应发生变化。

## 注意事项

- MP3 文件路径必须使用 SD 卡 FATFS 盘符，例如 `1:/music.mp3`。
- 只要 `CONFIG_BT` 开启，A2DP Source 源文件就会参与编译。`ap_main.c` 注册 `a2dp_player`，首次执行 `connect` 时才初始化 A2DP Source 服务，并非开机初始化。工程 Kconfig 菜单仍标为“Headset Example Configuration”，且含无关的 `A2DP_SINK_DEMO` 配置项；该项不会启动本 Source demo。
- `prev` 和 `next` 用于演示播放控制处理，不会从播放列表中选择其他文件。
