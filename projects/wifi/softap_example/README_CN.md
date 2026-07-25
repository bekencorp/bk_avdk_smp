# softap_example

* [English](./README.md)

## 1. 概述

演示 SoftAP 启动与 STA 上下线事件。

- 源码：`ap/ap_main.c`
- 开发者指南：[Wi-Fi 模式与 CLI](../../../../developer-guide/wifi/bk_wifi_mode.html)

## 2. 编译

```text
make bk7259 PROJECT=wifi/softap_example
```

## 3. 运行

AP UART：

```text
ap myssid 12345678
```

CP UART：

```text
ap_cmd ap myssid 12345678
```
