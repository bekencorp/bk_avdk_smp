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

client 连上并推流时的预期日志：

```text
cis_request ase=0x01 ...
cis_established ase=0x01 cis_handle=0x....
iso_path_ready ase=0x01 ... -> auto rx_ready
receiver start ready ase=1
```

## 说明

- 设计上单 CIS / 单流。
- codec/QoS 由 client 决定，本侧只负责接受并播放。
