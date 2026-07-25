# BK7259 LE Audio Broadcast Sink

* [English](./README.md)

## 支持的芯片

| 芯片 | 状态 | LE Audio 角色 | Host / Controller 划分 |
| --- | --- | --- | --- |
| BK7259 | 支持 | Broadcast Sink + Scan Delegator（BIS 接收） | Host 在 AP，Controller 在 CP |

**Broadcast Sink**（Auracast 接收端），同时兼任 **BASS Scan Delegator**。它有两种方式
最终收到 BIS：

- **自主同步**：设备自己扫描 Broadcast Source，你选定 source 和 BIS，它同步并播放。
- **受托（Scan Delegator）**：暴露 BASS server 并保持被动；由 Broadcast Assistant
  （如 [unicast_client](../unicast_client/)）告知要收哪一路广播，并通过 PAST 把周期广播
  同步转移过来。

收到的 ISO 由 `../common/audio_rx.c` / `audio_playback.c` 做 LC3 解码与播放。

## 工作原理

两条路径最终汇合到同一个「enable BIS → streaming」尾部，由 demo 状态机线程推进：

```text
自主同步：
  scan on ──▶ (announcement) ──▶ sync <src> <bis>
      ──▶ PA associate ──▶ (BIGInfo) ──▶ broadcast_enable ──▶ ENABLE_CNF ──▶ STREAMING

Scan Delegator（PAST）：
  delegator on ──▶ (Assistant Add Source) ──▶ 等待 PAST
      ──▶ PA associate（经 PAST）──▶ broadcast_enable ──▶ ENABLE_CNF ──▶ STREAMING
```

两个路径相关的关键点：

- 自主同步时，扫描在 PA-associate → BIG-create-sync 期间**保持开启**。若此时停扫描，
  会留下一条尚未完成的 scan-disable HCI 命令，导致 BIG create-sync（共用同一 GA
  activity context）被以 “Context exists” 拒绝。
- PAST 路径下**不会再有单独的 BIGInfo 事件**（控制器只在 BIG create-sync 进行时把
  BIGInfo 作为 ACAD 附上），因此 demo 直接在 PA-associate 回调里调用 `broadcast_enable`。

## 编译

```bash
make bk7259 PROJECT=bluetooth/le_audio/broadcast_sink
```

## CLI

命令都在 AP 核；从 CP 串口输入时加 `ap_cmd` 前缀。

| 命令 | 作用 |
| --- | --- |
| `le_audio preset list` / `le_audio preset <name>` | 列出 / 选择 LC3 preset |
| `le_audio scan on` / `off` | 开始/停止扫描 Broadcast Source |
| `le_audio list` | 打印已扫描到的 source 及序号 |
| `le_audio sync <src 序号> <bis 序号>` | 自主同步到某 source 的 BIS（`bis 序号` 从 1 开始） |
| `le_audio delegator on` / `off` | 广播 BASS server 供 Broadcast Assistant 使用 |
| `le_audio stop` | 停止 / 拆除当前会话 |
| `le_audio broadcast_code <hex32>` / `clear` | 设置 / 清除 Broadcast Code |

自主同步流程：

```text
ap_cmd le_audio scan on
# 日志：[0] sid=1 addr=.. bcast_id=0x...
ap_cmd le_audio sync 0 1
# 日志：PA associated -> BIGInfo -> broadcast_enable ret=0 -> streaming
ap_cmd le_audio stop
```

Scan Delegator 流程（配合 Broadcast Assistant）：

```text
ap_cmd le_audio delegator on
# Assistant 执行 Add Source + PAST，预期日志：
#   Assistant Add Source ... pa_sync=1
#   awaiting PAST from Assistant
#   PA associated handle=0x....
#   broadcast_enable ... ret=0
#   broadcast event=...（ENABLE_CNF）-> streaming
```

## 说明

- 设计上单 BIS / 单流；`bis 序号` 从 1 开始。
- 加密广播需在同步前设置 Broadcast Code（或由 Assistant 通过 BASS 下发）。
