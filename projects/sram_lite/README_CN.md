# SRAM Lite 工程

* [English](./README.md)

## 工程概述

`sram_lite` 是用于资源受限构建验证的轻量级 AP/CP 应用骨架。工程仅保留应用入口和组件注册文件，不包含工程专用分区表或 `defconfig`。

因此，本工程会继承所选 SoC 的默认配置。应用入口还包含可选的 SMP、IPC 和 BLE 配网测试初始化，仅在启用对应配置时参与编译。

## 工程目录

- `ap/ap_main.c`：AP 初始化和可选测试代码
- `cp/cp_main.c`：CP 初始化和 AP 上电投票
- `ap/CMakeLists.txt`：AP 源文件和私有组件依赖
- `cp/CMakeLists.txt`：CP 源文件注册
- `CMakeLists.txt`：顶层工程定义

## 编译

针对当前 SDK 版本，在 SDK 根目录执行：

```text
make bk7259 PROJECT=sram_lite
```

## 运行

烧录生成的 AP 和 CP 镜像，连接对应串口并复位开发板，确认两侧应用入口均能完成初始化。

由于本工程继承 SoC 默认配置，启用其他组件前请先检查最终生效的配置和内存布局。
