# 平台测试工程

* [English](./README.md)

## 工程概述

`test/platform` 是 BK7259 平台驱动集成和回归测试工程。工程配置启用了常用外设和安全功能的 SDK CLI 测试。

当前配置覆盖：

- CP 侧看门狗和 ADC 测试
- AP 侧 PWM、UART、I2C、SPI、Flash、GPIO、ADC 和密码算法测试
- 通过 CP 串口转发到 AP 的测试命令

## 工程目录

- `ap/ap_main.c`：AP 初始化和可选 IPC 测试初始化
- `cp/cp_main.c`：CP 初始化和 AP 启动逻辑
- `ap/config/bk7259_ap/defconfig`：已启用的 AP 驱动测试功能
- `cp/config/bk7259/defconfig`：已启用的 CP 驱动测试功能
- `.it.csv`：集成测试命令和预期输出
- `partitions/bk7259/`：测试分区和 RAM 区域定义

## 编译

在 SDK 根目录执行：

```text
make bk7259 PROJECT=test/platform
```

## 运行

烧录生成的 AP 和 CP 镜像，连接测试所需的外设线路和串口，然后复位开发板。从 `.it.csv` 中选择适用命令执行，并将串口输出与预期结果进行比较。

示例命令：

```text
wdt_driver init
ap_cmd pwm_driver init
ap_cmd uart_driver init
ap_cmd mbedtls_selftest
```

## 测试注意事项

部分用例需要外部接线或两块开发板。Flash 测试会擦写命令指定的地址，看门狗重启测试会主动复位设备。在保存有重要数据的开发板上运行前，请先检查对应的 `.it.csv` 用例。
