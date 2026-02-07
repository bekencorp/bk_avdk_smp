# AP Powerdown Keepalive 示例工程

* [English](./README.md)

## 1. 工程概述

本工程演示 AP（Application core）与 CP（Communication core）之间的 keepalive（心跳）与电源管理交互。目标是展示在 AP 掉电或进入低功耗时，CP 如何继续与远端 Server 保持心跳并在需要时唤醒 AP。该示例提供：

- AP 侧通过 IPC 向 CP 下发 keepalive 启动/停止/控制命令（示例：ap_cmd lp start_ka / ap_cmd lp stop_ka）
- CP 侧实现 keepalive client/server、心跳检测、以及基于消息的唤醒与上/下电控制
- 示例演示如何在两块设备间进行 AP 掉电保活验证

更多背景与接口说明请参考工程内 `lp_ipc_msg` 与 `keepalive` 目录下源文件。

## 2. 适用硬件与测试环境

* 推荐开发板：BK7258 系列（或兼容平台）
* 需要两块开发板用于 Server/Client 验证（或一块与远端服务端模拟）
* 开发板需连接到同一路由器，能够在局域网内互相访问
* 串口终端：用于查看日志与输入 CLI 命令

## 3. 目录结构

```
ap_powerdown_keepalive/
├── .ci/
├── ap/
│   ├── ap_main.c
│   ├── keepalive/          # AP -> CP IPC 接口（ap/keepalive/keepalive.c）
│   └── lp_ipc_msg/         # AP 侧 IPC CLI 与消息封装
├── cp/
│   ├── cp_main.c
│   ├── keepalive/          # CP 端 keepalive client/server 与示例 CLI
│   ├── lp_ipc_msg/         # CP 侧 IPC 处理
│   └── powerctrl/          # 电源控制与唤醒逻辑
├── CMakeLists.txt
├── Makefile
└── pj_config.mk
```

## 4. 功能说明

### 4.1 主要功能

- AP 侧通过 IPC 启动/停止 CP 的 keepalive（保持与 Server 的心跳）
- CP 实现 keepalive client 与 server，可作为心跳发起方或接收方
- CP 支持在 AP 掉电后继续向 Server 发送心跳，并支持远端唤醒（Server -> Client -> 唤醒 AP）
- 提供 CLI 示例，方便手工验证

### 4.2 主要文件

- `ap/keepalive/keepalive.c`：AP 侧接口，包含 `keepalive_send_keepalive_cmd()`、`keepalive_stop_cp_keepalive()` 等
- `cp/keepalive/keepalive_client.c`：CP 端 client 实现与 CLI 入口
- `cp/keepalive/keepalive_server.c`：CP 端 server 实现与 CLI 入口
- `cp/powerctrl/powerctrl.c`：电源控制与唤醒相关实现
- `ap/lp_ipc_msg/lp_ipc_msg.c`：AP 侧 CLI 与 IPC 命令解析（提供 `lp start_ka` / `lp stop_ka` 等）

## 5. 编译与烧录

### 5.1 编译

在工程根目录执行：

```
make bk7258 PROJECT=ap_powerdown_keepalive
```

### 5.2 烧录与启动

1. 使用你们常用的烧录工具将固件烧录到两块开发板（Device-A, Device-B）。
2. 启动板子并通过串口打开控制台，注意分别查看 AP 与 CP 串口（视板子配置而定）。

## 6. CLI 使用与测试示例（AP 掉电保活）

下面给出完整的手工验证流程，适合快速上手。

### 6.1 场景描述

Device-A：作为 keepalive Server（CP 端运行 server）

Device-B：作为 keepalive Client，AP 侧通过 IPC 发起 keepalive，随后 AP 掉电，验证 CP 继续保活并可被唤醒

### 6.2 步骤

1) 确认网络：两台设备接入同一路由器并能互相 ping 通。

2) 启动 Server（Device-A，CP 串口）：

```
lp server -p 8000
```

3) 在 Device-B 上通过 AP 串口下发 IPC 命令启动 CP 的 keepalive（AP -> CP）：

```
ap_cmd lp start_ka <server_ip> 8000
```

此命令会让 AP 通过 IPC 请求 CP 启动对指定 server 的 keepalive。默认心跳间隔与超时配置位于 `cp/keepalive/keepalive_msg.h`（例如 `KEEPALIVE_TX_INTERVAL_SEC`）。

4) 确认 Device-B 的 CP（或 Device-A 的 Server）日志出现心跳交互信息（日志 TAG：`KEEPALIVE` / `PL`）。

5) 模拟 AP 掉电（在 Device-B 的 CP 串口执行）：

```
lp client pwrdown_host
```

执行后，AP 会被设置为下电状态，而 CP 应继续与 Device-A 保持心跳；在 Device-A 日志可观察到心跳仍在到达。

6) 唤醒 AP（两种方法）：

- 在 Device-A（Server）发送唤醒消息给 Client：

```
lp server -m wakeup_host
```

- 或在 Device-B（Client）上由 CP 直接发起唤醒：

```
lp client wakeup_host
```

唤醒成功后，AP 串口会出现唤醒原因日志（查看 `PL` 日志），并恢复正常交互。

7) 停止 keepalive：

```
# 从 AP 发起 IPC 停止
ap_cmd lp stop_ka

# 或在 CP 串口直接停止 client
lp client stop
```

## 7. 常见日志与诊断

- 查看 AP/CP 串口日志，关注 `KEEPALIVE`、`PL`、`LP_IPC` 标签，确认命令下发与心跳状态。
- 若遇到连接失败，请确认网络配置、Server IP/port 是否正确，以及防火墙/路由是否阻止 UDP/TCP（取决于实现）。

## 8. 扩展与注意事项

- 若需要把示例脚本化，我可以提供一个用于自动化验证的 shell 脚本（基于串口自动交互）。
- 如需示例串口日志片段，我也可以把典型的成功/失败日志加入文档以便参考。

