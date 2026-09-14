# 默认应用工程

* [English](./README.md)

## 工程概述

`app` 是 BK7258 的默认应用工程，提供标准的 AP/CP 启动流程和通用 SDK 开发、功能验证配置。

- CP 完成系统初始化并投票拉起 CP1，启动 SMP。
- AP 完成应用环境初始化。
- 可通过工程配置选择 SMP、IPC 和 BLE 配网测试代码。
- 默认配置启用常用的 Wi-Fi、蓝牙、存储、文件系统、安全和网络功能。
- 支持 BK7258 和 BK7257 两个编译目标。

## 工程目录

- `ap/ap_main.c`：AP 应用入口和可选测试初始化
- `cp/cp_main.c`：CP 应用入口和 CP1 启动逻辑
- `ap/config/bk7258_ap/config`：AP 工程配置
- `ap/config/bk7258_ap/no_cli.config`：关闭 CLI 的替代配置
- `cp/config/bk7258/config`：CP 工程配置
- `partitions/bk7258/auto_partitions.csv`：Flash 分区表
- `partitions/bk7258/ram_regions.csv`：SRAM/PSRAM 区域划分

## 编译

在 SDK 根目录执行：

```text
make bk7258 PROJECT=app
```

`app` 是默认工程，因此也可以执行：

```text
make bk7258
```

编译 BK7257 目标：

```text
make bk7257 PROJECT=app
```

如需使用关闭 CLI 的替代配置，执行：

```text
make bk7258 PROJECT=app BK_CONFIG_FILE=no_cli
```

## 运行

烧录生成的 AP 和 CP 镜像，连接对应串口并复位开发板。可通过已启用的 SDK CLI 命令验证配置中的功能，发往 AP 的命令需加 `ap_cmd` 前缀。

如需修改工程选项，请更新 AP 或 CP 的 `config`，也可以在重新编译前运行 `make bk7258_ap_menuconfig PROJECT=app` 或 `make bk7258_cp_menuconfig PROJECT=app`。
