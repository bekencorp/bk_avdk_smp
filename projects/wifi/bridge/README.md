# bridge

* [中文](./README_CN.md)

## Overview

Wi-Fi bridge demo: STA connects upstream, then exposes a bridged SoftAP. Controlled via AP-core CLI.

- Commands: ``bridge open <sta_ssid> [key] [bridge_ssid]``, ``bridge close``

## Build

```text
make bk7259 PROJECT=wifi/bridge
```

## Run

Use bridge CLI on AP UART after flashing.
