# scan_example

* [English](./README.md)

## 1. 概述

演示 ``bk_wifi_scan_start`` 与 ``EVENT_WIFI_SCAN_DONE`` 处理，打印扫描结果。

- 源码：`ap/ap_main.c`
- 开发者指南：[Wi-Fi 扫描](../../../../developer-guide/wifi/bk_wifi.html)（「Armino Wi-Fi 扫描」章节）

## 2. 编译

```text
make bk7259 PROJECT=wifi/scan_example
```

## 3. 运行

AP UART：

```text
scan
```

CP UART：

```text
ap_cmd scan
```
