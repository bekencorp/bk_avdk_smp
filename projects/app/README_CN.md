# 默认应用工程

* [English](./README.md)

## 工程概述

`app` 是 BK7259 的默认应用工程，提供标准的 AP/CP 启动流程和通用 SDK 开发、功能验证配置。

- CP 完成系统初始化并启动 AP。
- AP 完成应用环境初始化。
- 可通过工程配置选择 SMP、IPC 和 BLE 配网测试代码。
- 默认配置启用常用的 Wi-Fi、蓝牙、存储、文件系统、安全和网络功能。

## 工程目录

- `ap/ap_main.c`：AP 应用入口和可选测试初始化
- `cp/cp_main.c`：CP 应用入口和 AP 启动逻辑
- `ap/config/bk7259_ap/defconfig`：AP 工程配置
- `cp/config/bk7259/defconfig`：CP 工程配置
- `partitions/bk7259/auto_partitions.csv`：Flash 分区表

## 编译

在 SDK 根目录执行：

```text
make bk7259 PROJECT=app
```

`app` 是默认工程，因此也可以执行：

```text
make bk7259
```

如需使用备选平台配置，执行：

```text
make bk7259 PROJECT=app BK_CONFIG_FILE=platform
```

## 运行

烧录生成的 AP 和 CP 镜像，连接对应串口并复位开发板。可通过已启用的 SDK CLI 命令验证配置中的功能。

如需修改工程选项，请更新 AP 或 CP 的 `defconfig`，也可以在重新编译前运行对应的 `menuconfig` 目标。
