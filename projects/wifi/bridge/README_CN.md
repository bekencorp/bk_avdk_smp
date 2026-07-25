# bridge

* [English](./README.md)

## 1. 概述

Wi-Fi 桥接示例：STA 连接上游 AP 后创建桥接 SoftAP。通过 AP 核 CLI 控制。

- 主要命令：``bridge open <sta_ssid> [key] [bridge_ssid]``、``bridge close``

## 2. 编译

```text
make bk7259 PROJECT=wifi/bridge
```

## 3. 运行

烧录后在 AP UART 执行 bridge 相关 CLI。STA 获取 IP 后继续桥接建链。
