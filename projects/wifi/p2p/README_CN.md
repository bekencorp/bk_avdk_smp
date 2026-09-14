# Wi-Fi P2P 工程中文说明

* [English](./README.md)

本工程是 BK7258 SMP 的 Wi-Fi Direct（P2P）示例。AP、CP 均使能 `CONFIG_P2P`，通过 AP 核 CLI 开启 P2P、发现对端、协商 GO/GC 并组网。当前 SMP 分支不支持 P2P 与 WLAN STA 并发。

开发者指南：`ap/docs/bk7258/zh_CN/developer-guide/wifi/bk_wifi_p2p.rst`。

## 1. 目录结构
```
p2p/
├── CMakeLists.txt                 # 顶层 CMake 构建入口
├── Makefile                       # Make 构建入口
├── ap/                            # AP 核业务代码
│   ├── ap_main.c                  # AP 入口；P2P 由 CLI 驱动
│   └── config/                    # BK7258 AP 侧配置（CONFIG_P2P=y）
├── cp/                            # CP 核代码
│   ├── cp_main.c                  # CP 入口；拉起 AP 核
│   └── config/                    # BK7258 CP 侧配置（CONFIG_P2P、CONFIG_WPS）
└── partitions/                    # 分区与 RAM 区域配置
```

## 2. 功能特点
- 支持 P2P Device、Group Owner（GO）、Group Client（GC）。
- GO Negotiation：Intent `0` 强制 GC，`15` 强制 GO，`1`–`14` 为偏好（默认 `7`）。
- 组网前使用 WPS Push Button 认证，数据使用 WPA2 加密。
- 社交信道设备发现；支持 listen / find / connect / cancel / disable。
- 未指定 SSID 时，默认设备名为 `BEKEN SMP_P2P`。

## 3. 硬件与配置
- 硬件：两块 BK7258 SMP 开发板（或一块板 + 支持 P2P 的手机）；UART0 作为 AP 核 CLI。
- AP：`ap/config/bk7258_ap/config` 中 `CONFIG_P2P=y`。
- CP：`cp/config/bk7258/config` 中 `CONFIG_P2P=y` 且 `CONFIG_WPS=y`（P2P 依赖 WPS）。
- Flash / RAM：使用本工程默认分区即可。
- 移植时需实现并注册 P2P 事件回调（`demo_p2p_event_cb`），详见开发者指南。

## 4. 编译与烧录
```
make bk7258 PROJECT=wifi/p2p
```

按 SDK 烧录工具烧录 AP/CP 镜像后复位开发板。

## 5. 运行流程
在 **AP 核** 串口操作。本工程 CLI 为 `p2p ...`（不是文档里部分示例写的 `ap_cmd p2p`）。

1. 开启 P2P：

       p2p enable
       p2p enable p2p_ssid
       p2p enable p2p_ssid 0          # 强制 GC
       p2p enable p2p_ssid 15         # 强制 GO

2. 查找对端，或进入 listen：

       p2p find
       p2p listen

3. 连接已发现设备（MAC 为 12 位十六进制，`:` 可省略）：

       p2p connect <mac> <method> <intent>

4. 停止查找、取消当前操作（含断开）、关闭 P2P：

       p2p stop_find
       p2p cancel
       p2p disable
       p2p help

典型双板流程：两侧都 `p2p enable`；一侧 Intent `15`（GO）并 `p2p listen`，另一侧 Intent `0`（GC）并 `p2p find`，随后 connect，在 log 中确认 GO/GC IP。

## 6. 常见问题
- **没有 `p2p` 命令**：确认烧录的是本工程，且 AP 已打开 `CONFIG_P2P=y`。
- **enable 失败，或 GO 连接/断开后 listen 异常**：检查 CP 的 `CONFIG_WPS=y`；不要同时跑 STA 与 P2P（不支持并发）。
- **log 中没有 DEVICE_FOUND**：两侧先 enable 再 find/listen；设备靠近，并使用社交信道 1/6/11。
- **重连后 ioctl busy / ROC 错误**：查看 `wpa` / `wifid` 串口日志；若上次会话未干净拆除，先 cancel/disable 再 enable。
