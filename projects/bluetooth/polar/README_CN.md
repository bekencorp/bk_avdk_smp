# 蓝牙 BLE Polar RF 模式工程

* [English](./README.md)

## 工程简介

`polar` 是一个面向 BK7258 的蓝牙低功耗（BLE）控制器工程，用于选择 **Polar 模式**。相对于标准 IQ 模式配置，它面向发射功耗敏感的 BLE 低功耗应用场景。

本工程是平台配置工程，而非完整的终端 BLE profile demo。它提供 BLE 5.2 controller-only 基础配置；AP 应用仅进行基础启动，默认不提供 GATT 服务、配网流程或产品 CLI。

> Polar 模式属于控制器级 RF 配置，不是应用命令，也不用于针对单个用户数据包切换。

## Polar 模式与功耗行为

默认工程配置提供以下能力：

- 选择 Polar 模式的 BLE 低功耗运行；
- 与 Polar 模式匹配的 BLE 发射功率配置；
- 低功耗时钟和 PHY 电源管理支持；
- 供系统集成使用的 RF 共存模式切换能力。

实际功耗和 RF 性能与板卡、供电、天线、发射功率、RF 信道、BLE 角色、广播间隔、连接间隔及业务流量有关。应在目标产品和目标工作条件下实测；本示例不定义相对于 IQ 模式的确定电流降幅或固定省电比例。

应使用正常应用镜像而非 ATE/产测镜像评估产品的 Polar 模式行为。

## 默认配置

| 模块 | 配置 |
| --- | --- |
| 目标平台 | BK7258 AP/CP SMP 平台 |
| 蓝牙类型 | BLE；经典蓝牙关闭 |
| 控制器 | BLE 5.2 controller-only 模式 |
| RF 模式 | 默认 CP 配置启用 Polar 模式 |
| 低功耗支持 | 启用低功耗时钟和 PHY 电源管理选项 |
| 共存能力 | 启用 RF 共存模式切换选项 |
| Wi-Fi | 正常启动后关闭，以避免不必要的系统功耗 |
| 应用行为 | 默认不启动 GATT/profile 应用或产品 CLI |

## Polar 模式开启方法

工程提供的默认 CP 配置已开启 Polar 模式，配置文件位于：

```text
projects/bluetooth/polar/cp/config/bk7258/config
```

如需在定制工程中配置，请在 SDK 根目录通过 menuconfig 修改 CP 镜像：

```bash
CCACHE_DISABLE=1 make bk7258_cp_menuconfig PROJECT=bluetooth/polar
```

进入 **Bk_ble**，在 **Select Bluetooth RF Mode** 中选择 **Polar Mode**。同时保持 BLE 使能，并按产品需求选择 BLE 控制器版本。若需要与本示例一致的低功耗配置，还应开启：

```text
support lpo rosc
support coex rf mode switch
support sleep phy switch
```

保存配置后重新编译工程。建议使用 menuconfig 配置，不要直接修改自动生成的配置文件，以确保相关依赖项保持一致。

## 目录结构

```text
polar/
├── ap/
│   ├── ap_main.c                         # AP 入口；完成平台初始化
│   ├── config/bk7258_ap/config           # 含 CLI 的 AP 配置
│   ├── config/bk7258_ap/no_cli.config    # 不含 CLI 的 AP 配置
│   └── config/bk7258_ap/lwipopts_custom.h
├── cp/
│   ├── cp_main.c                         # CP 启动；拉起 CP1 并关闭 Wi-Fi
│   ├── config/bk7258/config              # CP BLE 控制器/RF 配置
│   └── config/bk7258/no_cli.config       # 不含 CLI 的 CP 配置
└── partitions/bk7258/                    # BK7258 分区和 RAM 区域定义
```

## 编译与烧录

在 SDK 根目录执行：

```bash
CCACHE_DISABLE=1 make bk7258 PROJECT=bluetooth/polar -j$(nproc)
```

按 BK7258 标准流程烧录生成镜像。以上命令使用默认的 `config` 配置文件。

如需构建随工程提供的无 CLI 配置，必须显式指定：

```bash
CCACHE_DISABLE=1 make bk7258 PROJECT=bluetooth/polar BK_CONFIG_FILE=no_cli -j$(nproc)
```

> `no_cli.config` 是独立的配置变体，并非仅在默认配置上关闭 CLI 输出。使用前请确认其蓝牙和低功耗能力满足产品需求。

## 校准与测试

本工程没有 Polar 专用功能测试命令，也没有 profile 层测试用例。编译、烧录后应先确认系统正常启动，再测试目标 BLE 应用，或使用已批准的 RF 测试环境。

在产品板上评估 RF 性能或功耗前，需要完成 RF 校准。请使用已经过 BK7258 标准产测/RF 校准流程的板卡，并保留 `partitions/bk7258/auto_partitions.csv` 中的 `sys_rf` 和 `sys_net` 分区；它们是 RF 校准和网络配置使用的系统预留分区，大小和偏移不可修改。

本工程不能替代产测校准流程，也不提供独立校准命令。应通过已批准的量产校准流程完成校准后，再按目标产品的实际工作条件测量。

## 扩展工程

工程默认不注册 BLE 应用、GATT 服务或用户 CLI。如需在此基础上开发应用：

1. 开启所需 BLE 应用能力。
2. 在 AP 侧添加应用初始化和回调。
3. 增加应用源文件和依赖。
4. 保持 AP/CP 蓝牙配置兼容。

若需要 BLE 配网，需启用并集成配网应用，再验证完整用户流程。

## 验证说明

默认工程没有可见的 BLE 服务或产品 CLI，无法通过 profile 层命令序列进行功能验证。建议先确认系统正常启动，再结合外部测试环境或新增应用验证目标功能。

对比 Polar 与 IQ 功耗时，应在仅更改 RF 模式的两份镜像间，保持 BLE 角色、广播/连接间隔、负载、RF 信道、TX 功率、时钟源和板端供电条件一致，再测量平均电流和峰值电流。不能仅依据本工程推导出确定的省电百分比。
