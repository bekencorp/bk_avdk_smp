# A/B OTA 应用工程

* [English](./README.md)

## 工程概述

`app_ab` 是 BK7259 位置无关 A/B OTA 升级参考工程。应用启动过程沿用标准 AP/CP 流程，工程配置和分区文件用于启用 A/B 镜像打包与升级。

主要特性：

- A/B OTA 和 HTTP OTA 支持
- 位置无关应用镜像
- OTA 镜像 Hash 校验
- 备用应用区 `s_app` 和 OTA 完成状态数据

## 工程目录

- `ap/ap_main.c`：AP 应用入口
- `cp/cp_main.c`：CP 应用入口和 AP 启动逻辑
- `ap/config/bk7259_ap/defconfig`：包含 A/B OTA 选项的 AP 配置
- `cp/config/bk7259/defconfig`：CP 配置
- `partitions/bk7259/auto_partitions.csv`：A/B Flash 分区表
- `partitions/bk7259/ab_position_independent.csv`：位置无关镜像配置
- `partitions/bk7259/ota_rbl.config`：OTA 包配置

## 编译

在 SDK 根目录执行：

```text
make bk7259 PROJECT=app_ab
```

## 运行与验证

1. 编译工程并烧录生成的初始镜像。
2. 按 SDK 打包流程使用生成的 A/B OTA 产物制作升级包。
3. 通过已启用的 OTA 接口启动升级。
4. 复位开发板，确认系统启动到更新后的镜像。

分区表和 OTA 包配置共同构成升级约束。修改镜像大小或启用更多功能前，应先检查这些文件。
