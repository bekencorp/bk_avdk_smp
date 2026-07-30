# SPP Demo

* [English](./README.md)

## 工程概述

本工程演示如何使用 Bluetooth SPP (Serial Port Profile) 协议在两块 Beken
开发板之间进行数据交互。

当前工程以 `bluetooth/headset` 的 AP/CP 工程结构为模板创建，但 AP 应用只
初始化蓝牙并注册 `spp` CLI demo，不接入 headset 的 A2DP/HFP/PBAP 业务。

## 硬件要求

- 两块 Beken 开发板
- UART0 用于下载镜像、查看日志和输入 CLI 命令

## 编译和烧录

在 Armino SDK 根目录下，使用当前 SDK 的工程构建方式，并将工程路径设置为
`bluetooth/spp`。

编译完成后，烧录 `spp` 工程生成的 `all-app.bin` 到开发板，可使用 BKFIL 或
项目标准烧录工具。

## 工作流程

使用一块开发板作为 SPP server，另一块开发板作为 SPP client：

1. 两块板都执行 `ap_cmd spp init`。
2. server 板执行 `ap_cmd spp start_server`。
3. 从 server 日志中记录 server channel 和 handle。
4. client 板执行 `ap_cmd spp conn <server_addr>`。
5. 从 client 日志中记录连接 handle。
6. 任意一侧执行 `ap_cmd spp write <handle> <data>` 发送数据。
7. 执行 `ap_cmd spp rate <handle> <hex_length>` 进行吞吐测试。
8. 执行 `ap_cmd spp disconn <remote_addr> [handle]` 或
   `ap_cmd spp stop_server <local_channel>` 关闭连接或 server。



## CLI 命令

本工程通过 UART0 支持以下 CLI 命令：

```text
ap_cmd spp help
ap_cmd spp init
ap_cmd spp deinit
ap_cmd spp start_server
ap_cmd spp stop_server <local_channel>
ap_cmd spp conn <remote_addr>
ap_cmd spp conn1 <remote_addr> <remote_channel>
ap_cmd spp disconn <remote_addr> [spp_handle]
ap_cmd spp write <handle> <data>
ap_cmd spp rate <handle> <hex_length>
ap_cmd spp status <handle>
```



### 命令说明

- `ap_cmd spp init`：初始化 SPP 协议。
- `ap_cmd spp deinit`：反初始化 SPP 协议。
- `ap_cmd spp start_server`：启动本机 SPP server，并注册到 SDP 数据库。
- `ap_cmd spp stop_server <local_channel>`：停止指定 channel 的 SPP server，
  `local_channel` 从 `ap_cmd spp start_server` 日志中获取。
- `ap_cmd spp conn <remote_addr>`：发现远端 SPP server channel 并连接远端设备，
  地址格式为 `xx:xx:xx:xx:xx:xx`。
- `ap_cmd spp conn1 <remote_addr> <remote_channel>`：直接连接已知的远端 server
  channel。
- `ap_cmd spp disconn <remote_addr> [spp_handle]`：断开已建立的 SPP 连接。
  如果省略 handle，demo 会断开第一个匹配该远端地址的已连接 SPP 设备。
- `ap_cmd spp write <handle> <data>`：通过已建立的 SPP 连接发送数据。
- `ap_cmd spp rate <handle> <hex_length>`：发送随机数据做吞吐测试，长度参数按十六进制解析。
- `ap_cmd spp status <handle>`：打印 demo 侧记录的连接 pending 状态。



## 示例

Server 板：

```text
ap_cmd spp init
ap_cmd spp start_server
```

关键日志：

```text
bk_cli_bt_spp_callback spp init status:0
bk_cli_bt_spp_callback, spp start_server success, chnl:1, spp_handle:0x00
```

Client 板：

```text
ap_cmd spp init
ap_cmd spp conn C8:47:8C:0B:DC:08
```

关键日志：

```text
bk_cli_bt_spp_callback, spp discover success, chnl0:1, cnt:1 !!
bk_cli_bt_spp_callback, spp conn success to 0x08:0xdc:0x0b:0x8c:0x47:0xc8
HANDLE: 0x00
```

发送数据：

```text
ap_cmd spp write 00 111122221111
```

对端关键日志：

```text
===========DATA IND===========
bk_cli_bt_spp_callback, spp data ind, handle:0x00, len:12
111122221111
==============================
```

吞吐测试：

```text
ap_cmd spp rate 00 7ffff
```

关键日志：

```text
========spp tx start total_length: 524287 ========
spp tx length: 524287, speed: <speed>KB/s
========spp tx finish tx_length: 524287, crc:<crc> ========
======== spp rx start ========
========spp rx finish tx_length: 524287, speed: <speed>KB/s, crc:<crc>========
```

