# SRAM Lite 工程

* [English](./README.md)

## 工程概述

`sram_lite` 是 BK7258 的精简应用工程，与默认工程 `app` 共用同一套 AP/CP 启动流程和应用入口代码，通过裁剪功能和重新划分内存把更多 SRAM 留给 AP 侧，适合内存紧张的方案做基线评估。

- 裁掉 AT 命令服务、蓝牙 host、IPv6、iperf 及一系列驱动自测项，蓝牙仅保留 BLE slave。
- 关闭 `CONFIG_OTA_FUNCTION`（`app` 中为 `y`），`ap/components/ota` 的 OTA 实现不参与编译。
- LWIP 使用精简内存策略：AP 侧 `CONFIG_LWIP_MEM_DEFAULT`，CP 侧 `CONFIG_LWIP_MEM_REDUCE`。
- SRAM 划分向 AP 倾斜，相比 `app` 把 48K 从 CP 移给 AP（`AP_RAM=0x05c000`、`CP_RAM=0x033700`）。
- 保留 Wi-Fi、PSRAM、FATFS/SD、LittleFS、EasyFlash、VFS 等主要功能。
- 支持 BK7258 和 BK7257 两个编译目标。

应用入口还包含可选的 SMP、IPC 和 BLE 配网测试初始化，仅在启用对应配置时参与编译。

## 工程目录

- `ap/ap_main.c`：AP 初始化和可选测试代码
- `cp/cp_main.c`：CP 初始化和 CP1 上电投票
- `ap/config/bk7258_ap/config`：裁剪后的 AP 工程配置
- `cp/config/bk7258/config`：裁剪后的 CP 工程配置
- `partitions/bk7258/auto_partitions.csv`：Flash 分区表
- `partitions/bk7258/ram_regions.csv`：偏向 AP 的 SRAM/PSRAM 区域划分

## 编译

在 SDK 根目录执行：

```text
make bk7258 PROJECT=sram_lite
```

编译 BK7257 目标：

```text
make bk7257 PROJECT=sram_lite
```

## 运行

烧录生成的 AP 和 CP 镜像，连接对应串口并复位开发板，确认两侧应用入口均能完成初始化，再用已保留的 Wi-Fi、文件系统等 CLI 命令做基本验证。

## 注意事项

- 本工程的价值在于内存基线，重新打开被裁剪的功能（尤其是蓝牙 host、AT、IPv6）前，应先确认 `ram_regions.csv` 的 SRAM 划分是否还够用。
- CP 侧 SRAM 已压缩到 `0x033700`，在 CP 上新增功能时容易先撞到上限。
- `CONFIG_OTA_HTTP` 仍为 `y`，但 `CONFIG_OTA_FUNCTION` 为 `n`。需要 OTA 时要同时打开 `CONFIG_OTA_FUNCTION`，只开 `CONFIG_OTA_HTTP` 不会编出 OTA 实现。
- AP 与 CP 的 `CONFIG_MAX_COMMANDS` 均为 200，新增大量 CLI 命令时需同步上调。
