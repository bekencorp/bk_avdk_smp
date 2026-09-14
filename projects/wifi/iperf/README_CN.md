# Wi-Fi iPerf 工程中文说明

* [English](./README.md)

本工程是 BK7258 SMP 的综合 Wi-Fi 测试示例，集成 STA / SoftAP / 扫描 / iPerf 等 CLI。先通过 CLI 完成 STA 或 SoftAP 联网，再跑 iPerf 测吞吐。

开发者指南：`ap/docs/bk7258/zh_CN/developer-guide/wifi/bk_wifi_iperf.rst`、`ap/docs/bk7258/en/developer-guide/wifi/bk_wifi_mode.rst`。

## 1. 目录结构
```
iperf/
├── CMakeLists.txt                 # 顶层 CMake 构建入口
├── Makefile                       # Make 构建入口
├── app.rst                        # Sphinx 工程说明
├── ap/                            # AP 核业务代码
│   ├── ap_main.c                  # AP 入口；STA / SoftAP / iPerf 由 CLI 驱动
│   └── config/                    # BK7258 AP 侧配置（CONFIG_IPERF_TEST=y）
├── cp/                            # CP 核代码
│   ├── cp_main.c                  # CP 入口；拉起 AP 核
│   └── config/                    # BK7258 CP 侧配置
└── partitions/                    # 分区与 RAM 区域配置
```

## 2. 功能特点
- STA、SoftAP、扫描与 `state` 状态查询。
- iPerf：TCP / UDP 的 client、server 四种模式。
- AP 侧默认打开 `CONFIG_IPERF_TEST=y`、`CONFIG_WIFI_SOFTAP=y`。

## 3. 硬件与配置
- 硬件：BK7258 SMP 开发板；UART0 作为 AP 核 CLI。对端可以是 PC 上的 iPerf，或另一块开发板。
- AP：`ap/config/bk7258_ap/config` 中 `CONFIG_IPERF_TEST=y`。
- Flash / RAM：使用本工程默认分区即可。

## 4. 编译与烧录
```
make bk7258 PROJECT=wifi/iperf
```

按 SDK 烧录工具烧录 AP/CP 镜像后复位开发板。

## 5. 运行流程
在 **AP 核** 串口操作。先联网，再跑 iPerf。

1. STA 入网，或开启 SoftAP：

       sta <ssid> [password]
       ap <ssid> [password]
       scan [ssid]
       state

2. iPerf（默认 TCP；加 `-u` 为 UDP）：

       iperf -s                      # TCP server
       iperf -s -u                   # UDP server
       iperf -c <host>               # TCP client
       iperf -c <host> -u            # UDP client
       iperf --stop
       iperf -h

典型流程：板子 `sta` 连上路由器后，PC 跑 `iperf -s`，板上执行 `iperf -c <PC_IP>`。

## 6. 常见问题
- **没有 `iperf` 命令**：确认烧录的是本工程，且 AP 已打开 `CONFIG_IPERF_TEST=y`。
- **client 连不上**：先 `state` 确认已拿到 IP；对端 server 已启动，且与板子在同一网段。
- **吞吐偏低**：尽量用 5 GHz / 少干扰信道；UDP 可再调 `-u` 与对端带宽参数。
