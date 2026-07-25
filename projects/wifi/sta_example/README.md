# sta_example

* [中文](./README_CN.md)

## Overview

STA connect demo on the AP core: register Wi-Fi/Netif events, set SSID/password, call ``bk_wifi_sta_start``, use sockets after ``EVENT_NETIF_GOT_IP4``.

- Source: `ap/ap_main.c`
- Guides: [Quick Get-Started](../../../../developer-guide/wifi/bk_wifi_get_started.html), [Wi-Fi Mode & CLI](../../../../developer-guide/wifi/bk_wifi_mode.html)

## Build

```text
make bk7259 PROJECT=wifi/sta_example
```

Flash both AP and CP images.

## Run

Edit ``WIFI_STA_EXAMPLE_SSID`` / ``WIFI_STA_EXAMPLE_PASSWORD`` in ``ap/ap_main.c``, or use AP UART CLI:

```text
sta your_ssid your_password
```

On CP UART:

```text
ap_cmd sta your_ssid your_password
```
