# BK7259 LE Audio Unicast Server

* [English](./README.md)

## 支持的芯片

| 芯片 | 状态 | LE Audio 角色 | Host / Controller 划分 |
| --- | --- | --- | --- |
| BK7259 | 支持 | Unicast Server（CIS 接收） | Host 在 AP，Controller 在 CP |

**Unicast Server**（CIS 接收端）——单播 LE Audio 里耳机/音箱那一侧。它暴露 PACS + ASCS，
做可连接广播，接受 [unicast_client](../unicast_client/) 建立的 CIS，然后把收到的 ISO
做 LC3 解码并在本地扬声器播放。

## 工作原理

server 是被动响应的：开机即广播；由 client 驱动全部 ASCS 配置（codec/QoS/enable），
demo 状态机只对等时相关事件做反应：

```text
开机 ──▶ 可连接广播
     ──▶ （对端连接，client 通过 ASCS 配置 ASE）
     ──▶ CIS request ──▶ CIS established ──▶ ISO path ready
                                                 │ 自动
                                                 ▼
                                        receiver start ready ──▶ 播放
对端断开 ──▶ 停播放 ──▶ 重新广播
```

- ISO 数据通路就绪时**自动**下发 `receiver start ready`，无需任何 CLI 步骤即可出声。
- 广播是幂等且自恢复的：开机使能一次，连接后由控制器自动终止，断开后再次使能。已在广播时
  重复 `adv on` 是安全的空操作。

## 编译

```bash
make bk7259 PROJECT=bluetooth/le_audio/unicast_server
```

## CLI

命令都在 AP 核；从 CP 串口输入时加 `ap_cmd` 前缀。常规情况下你什么都不用敲，直接从 client
连过来即可。

| 命令 | 作用 |
| --- | --- |
| `le_audio preset list` / `le_audio preset <name>` | 列出 / 选择 LC3 preset |
| `le_audio adv on` / `off` | 手动开/关广播（带守卫、幂等） |
| `le_audio rx_ready <ase>` | 手动下发 Receiver Start Ready（通常自动完成） |
| `le_audio release <ase>` | 释放某个 ASE 并停播放 |

### 音量控制（VCP renderer，`CONFIG_LE_AUDIO_VCP`，默认开）

本端是 Volume Renderer，client 侧的 Volume Controller 改音量时会回调上来；下面两条是
本端主动改，改完会通知已订阅的 controller。

| 命令 | 作用 |
| --- | --- |
| `le_audio volume <0-255>` | 设置绝对音量 |
| `le_audio mute on` / `off` | 静音 / 取消静音 |

### 媒体控制（MCP，`CONFIG_LE_AUDIO_MCP`，默认开）

播放器状态在对端，本端作为 Media Control Client 下发控制点操作。

| 命令 | 作用 |
| --- | --- |
| `le_audio play` / `pause` / `stop` | 播放 / 暂停 / 停止 |
| `le_audio next` / `prev` | 下一曲 / 上一曲 |
| `le_audio mcp_notify on` / `off` | 订阅 / 取消订阅播放状态通知 |
| `le_audio mcp_read` | 读取当前播放状态 |

client 连上并推流时的预期日志：

```text
cis_request ase=0x01 ...
cis_established ase=0x01 cis_handle=0x....
iso_path_ready ase=0x01 ... -> auto rx_ready
receiver start ready ase=1
```

## 说明

- 默认双流立体声：注册两个 sink ASE，PACS 宣告前左 + 前右，由**同一个 CIG 的两条 CIS**
  承载，收到的两路解码后合成立体声播放。流数由 `CONFIG_LE_AUDIO_STREAM_COUNT` 决定。
- PACS 里 `supported_channel_counts` 故意只报 1：每条流承载单声道。若报 2，对端可能
  改成只建一条 CIS、把左右声道塞进同一个数据包。
- ASE 里宣告的 QoS 只是 preferred 值，最终参数由 client 决定。
- codec/QoS 由 client 决定，本侧只负责接受并播放。
