# 以太网示例工程

* [English](./README.md)

## 工程概述

`eth_example` 是 BK7258 以太网示例工程。工程在默认工程 `app` 的基础上使能以太网 MAC 与 PHY，并按有线网络的吞吐需求放大 LWIP 缓冲区。

- AP 侧使能以太网 MAC 驱动（`CONFIG_ETH=y`），配合 SMSC PHY 使用，引脚使用 group0。
- 使能以太网校验和硬件卸载和电源管理回调。
- AP 启动时以 ENET 名义将 CPU 频率投票到 480MHz。
- LWIP 内存与 TCP 窗口相比 `app` 明显放大，网络报文使用 PSRAM，默认关闭 IPv6。
- 提供以太网 IP 状态轮询打印示例，默认未启用。

## 工程目录

- `ap/ap_main.c`：AP 应用入口、ENET 频率投票和以太网状态监控示例
- `cp/cp_main.c`：CP 应用入口和 CP1 启动逻辑
- `ap/config/bk7258_ap/config`：使能以太网的 AP 配置
- `cp/config/bk7258/config`：CP 配置
- `partitions/bk7258/auto_partitions.csv`：Flash 分区表
- `partitions/bk7258/ram_regions.csv`：SRAM/PSRAM 区域划分

## 编译

在 SDK 根目录执行：

```text
make bk7258 PROJECT=eth_example
```

## 运行

接好网线，烧录生成的 AP 和 CP 镜像，连接对应串口并复位开发板。观察日志确认 PHY link up 并取得 IP，再用 ping、iperf 等已启用的 CLI 命令验证连通性与吞吐，发往 AP 的命令需加 `ap_cmd` 前缀。

`ap/ap_main.c` 中的 `eth_status_monitor()` 每 5 秒打印一次以太网 IP、掩码、网关和 DNS。该线程的创建语句默认是注释状态，需要周期性观察时取消注释并重新编译：

```text
//rtos_create_thread(NULL, 1, "eth_mon", eth_status_monitor, 2048, NULL);
```

## 注意事项

- 本工程按 SMSC PHY 与 group0 引脚组配置，更换 PHY 型号或引脚组时需同步修改 `CONFIG_PHY_*` 与 `CONFIG_ETH_PIN_GROUP*`，硬件也要匹配。
- LWIP 缓冲区与 `ram_regions.csv` 的划分是配套的，调整时需一起评估。
- ENET 的 480MHz 频率投票会影响功耗，低功耗场景需重新评估。
