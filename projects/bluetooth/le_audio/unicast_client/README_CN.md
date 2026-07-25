# BK7259 LE Audio Unicast Client

* [English](./README.md)

## 支持的芯片

| 芯片 | 状态 | LE Audio 角色 | Host / Controller 划分 |
| --- | --- | --- | --- |
| BK7259 | 支持 | Unicast Client + Broadcast Assistant（CIS 发送） | Host 在 AP，Controller 在 CP |

本工程提供两个角色：

- **Unicast Client**（CIS 发起端）——手机/中心那一侧。扫描可连接的 LE Audio server，
  连接后驱动完整 ASCS 建流，通过 CIS 把本地 tone 推给
  [unicast_server](../unicast_server/)。
- **Broadcast Assistant**（BASS client）——替 Scan Delegator
  （[broadcast_sink](../broadcast_sink/)）扫描 Broadcast Source，再用 BASS Add Source
  远程控制它，并通过 PAST 转移周期广播同步。

## 工作原理

两个角色启动后都全自动，demo 状态机线程会在对应 GA/BASS 回调里逐步推进。

Unicast Client —— 一条 `connect` 触发整条链：

```text
connect <peer> ──▶ ACL 连接 ──▶ setup ──▶ 取能力 ──▶ 发现 ASE
   ──▶ 配置 codec ──▶ 设置 CIG ──▶ QoS ──▶ enable ──▶ 建立 CIS
   ──▶ ISO path ready ──▶ 发送 tone ──▶ STREAMING
```

Broadcast Assistant —— 你用几条高层命令驱动，PAST 自动完成：

```text
assistant_scan on ──▶ （发现 source，扫描期间即刻 PA-sync）
assistant_connect <deleg> ──▶ 连接 delegator
assistant_discover <deleg> ──▶ 发现 BASS
assistant_add <src> <bis> ──▶ Add Source ──▶ （delegator 请求 PAST）
                                          ──▶ PAST -> delegator
```

关键点：Assistant 在**扫描期间**就 PA-sync source，而不是在 `assistant_add` 时。
这颗控制器只有在 ext scan 开启时 PA-create-sync 才会成功，所以提前拿到 sync handle，
等 delegator 请求 PAST 时就已就绪（若仍在进行，则在 associate 回调里补发 PAST）。

## 编译

```bash
make bk7259 PROJECT=bluetooth/le_audio/unicast_client
```

## CLI

命令都在 AP 核；从 CP 串口输入时加 `ap_cmd` 前缀。

### Unicast Client

| 命令 | 作用 |
| --- | --- |
| `le_audio preset list` / `le_audio preset <name>` | 列出 / 选择 LC3 preset（connect 前） |
| `le_audio scan on` / `off` | 扫描可连接的 LE Audio server |
| `le_audio peers` | 打印扫描到的 peer 及序号 |
| `le_audio connect <peer 序号>` | 按序号连接并自动跑完整建流 |
| `le_audio connect <addr> [type]` | 按 MAC 连接（`type` 0=public，1=random） |
| `le_audio tone stop` | 停止发送 tone |

```text
ap_cmd le_audio scan on
ap_cmd le_audio peers          # [0] addr=.. name=..
ap_cmd le_audio connect 0
# 自动：setup -> caps -> discover -> config -> cig -> qos -> enable -> cis -> tone
```

### Broadcast Assistant

| 命令 | 作用 |
| --- | --- |
| `le_audio assistant_scan on` / `off` | 扫描 Broadcast Source（对第一个自动 PA-sync） |
| `le_audio assistant_sources` | 打印扫描到的广播源 |
| `le_audio assistant_connect <deleg_addr> [type]` | 连接 Scan Delegator |
| `le_audio assistant_discover <deleg_addr> [type]` | 发现其 BASS |
| `le_audio assistant_add <src 序号> [bis_mask]` | Add Source，触发 PAST（`bis_mask` 默认 `0x1`） |
| `le_audio assistant_remove <source_id>` | 从 delegator 移除某个 source |
| `le_audio assistant_stop` | 停止 assistant 活动并复位 |
| `le_audio broadcast_code <hex32>` / `clear` | 设置 / 清除通过 BASS 下发的 Broadcast Code |

```text
ap_cmd le_audio assistant_scan on
# 日志：source[0] ... ; PA-sync source[0] for PAST ret=0
ap_cmd le_audio assistant_scan off
ap_cmd le_audio assistant_connect <deleg_addr> 0
ap_cmd le_audio assistant_discover <deleg_addr> 0
ap_cmd le_audio assistant_add 0 1
# 日志：PAST -> delegator sync=0x.... ret=0
```

## 说明

- 设计上单流。ASCS 链由状态机驱动，因此没有手动的
  `setup/caps/discover/config/cig/qos/enable/cis` 命令——`connect` 一条全包。
- `bis_mask` 是位掩码（BIS 1 = `0x1`）；`type` 是对端地址类型。
