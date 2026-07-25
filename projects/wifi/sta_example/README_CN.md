# sta_example

* [English](./README.md)

## 1. 概述

演示 AP 核 STA 模式连接：注册 Wi-Fi / Netif 事件，配置 SSID 密码后调用 ``bk_wifi_sta_start``，在 ``EVENT_NETIF_GOT_IP4`` 后可进行 socket 通信。

- 源码：`ap/ap_main.c`
- 开发者指南：[Wi-Fi 快速入门](../../../../developer-guide/wifi/bk_wifi_get_started.html)、[Wi-Fi 模式与 CLI](../../../../developer-guide/wifi/bk_wifi_mode.html)

## 2. 编译与烧录

```text
make bk7259 PROJECT=wifi/sta_example
```

烧录 AP + CP 完整镜像。

## 3. 运行

修改 ``ap/ap_main.c`` 中 ``WIFI_STA_EXAMPLE_SSID`` / ``WIFI_STA_EXAMPLE_PASSWORD``，或在 AP UART 使用 CLI：

```text
sta your_ssid your_password
```

CP UART 转发 AP CLI：

```text
ap_cmd sta your_ssid your_password
```
