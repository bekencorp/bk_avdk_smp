# AP 仿真启动工程

* [English](./README.md)

## 工程概述

`ap_emuboot` 是用于验证 AP 仿真启动配置的 BK7259 精简工程。工程关闭了大部分通信和外设功能，以较小配置验证启动流程。

启用 `CONFIG_AP_EMUBOOT` 后，CP 会在系统早期初始化阶段启动 AP，而不是在常规 CP 应用入口中启动。AP 与 CP 通过系统软件寄存器协调 Flash 初始化，随后 AP 入口执行工程特定的 Flash 时钟配置并调用 `bk_init()`。

## 工程目录

- `ap/ap_main.c`：AP 早期时钟配置和应用入口
- `cp/cp_main.c`：精简 CP 应用入口；AP 已在更早的系统初始化阶段启动
- `ap/config/bk7259_ap/defconfig`：精简的 AP 仿真启动配置
- `cp/config/bk7259/defconfig`：精简的 CP 仿真启动配置
- `bk7259_ap_bsp.ld`：AP 链接布局
- `bk7259_bsp.ld`：CP 链接布局
- `partitions/bk7259/`：工程分区和 RAM 区域定义

## 编译

在 SDK 根目录执行：

```text
make bk7259 PROJECT=ap_emuboot
```

## 运行

使用仿真启动测试环境烧录生成的镜像，连接 AP 和 CP 串口并复位目标板，确认两侧均能正常完成 `bk_init()`，且无启动异常。

本工程有意关闭了较多 SDK 默认功能。启用其他功能前，请先检查可用的 Flash 和 RAM 布局。
