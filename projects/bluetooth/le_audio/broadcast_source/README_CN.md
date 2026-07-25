# BK7259 LE Audio Broadcast Source

* [English](./README.md)

## 支持的芯片

| 芯片 | 状态 | LE Audio 角色 | Host / Controller 划分 |
| --- | --- | --- | --- |
| BK7259 | 支持 | Broadcast Source（BIS 发送） | Host 在 AP，Controller 在 CP |

**Broadcast Source**（Auracast 发送端）。它建立 BIS，把本地生成的正弦 tone 编码成
LC3 并无连接地发送出去，任意多个 [broadcast_sink](../broadcast_sink/) 都能同步播放。

## 工作原理

demo 跑一个状态机线程；`le_audio start` 启动整条链，之后每个 GA 回调自动推进：

```text
start ──▶ 申请 session ──▶ 配置 ──▶ 注册 SEP ──▶ setup announcement
                                                    │ (announcement 完成)
                                                    ▼
                                创建 BIG ──▶ (BIG 建立) ──▶ 发送 tone ──▶ STREAMING
stop  ──▶ 挂起 BIG ──▶ end announcement ──▶ 释放 session ──▶ IDLE
```

- 周期广播（PA）携带 BASE（codec/流布局），sink 据此解码；扩展广播携带带 Broadcast ID
  的 Broadcast Audio Announcement。
- 音频来自 `../common/audio_tone.c`（正弦 tone）→ `audio_tx.c`
  （LC3 编码 + `bk_dm_bap_broadcast_data_send`）。

## 编译

```bash
make bk7259 PROJECT=bluetooth/le_audio/broadcast_source
```

## CLI

命令都在 AP 核；从 CP 串口输入时加 `ap_cmd` 前缀。

| 命令 | 作用 |
| --- | --- |
| `le_audio preset list` / `le_audio preset <name>` | 列出 / 选择 LC3 preset（`start` 前） |
| `le_audio start` | 建立广播并开始发送 tone |
| `le_audio stop` | 拆除广播（suspend → end → free） |
| `le_audio broadcast_code <hex32>` / `clear` | 设置 / 清除 16 字节 Broadcast Code（加密广播） |

典型流程：

```text
ap_cmd le_audio preset list
ap_cmd le_audio start
# ... broadcast_sink 同步并播放 ...
ap_cmd le_audio stop
```

start 后预期日志：

```text
setup announcement pending session=0 sep=0
create BIG session=0 ret=0
broadcast started big=0 bis_handle=0x....
tx started; stop with ap_cmd le_audio stop
```

## 说明

- 设计上单 BIS / 单流。
- 加密广播时，sink 侧要设置相同的 Broadcast Code。
- LC3 流参数来自 `ap/le_audio_user_config.h`（默认 48 kHz / 10 ms / 120 字节）与
  `../common/audio_codec.c` 中的共享 preset 表。
