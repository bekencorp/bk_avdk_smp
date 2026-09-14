# 平台测试工程

* [English](./README.md)

## 工程概述

`test/platform` 是 BK7258 平台驱动集成和回归测试工程。工程配置启用了常用外设和安全功能的 SDK CLI 测试。

当前配置覆盖：

- CP 侧看门狗、ADC/SADC、Flash、GPIO、PSRAM、PUF、Touch 和 UART 测试
- AP 侧 PWM、UART、I2C、SPI、Flash、GPIO、SARADC 和密码算法测试，并使能完整 mbedTLS
- 通过 CP 串口转发到 AP 的测试命令

## 工程目录

- `ap/ap_main.c`：AP 初始化和可选 SMP 测试初始化
- `cp/cp_main.c`：CP 初始化和 CP1 启动逻辑
- `ap/config/bk7258_ap/config`：已启用的 AP 驱动测试功能
- `cp/config/bk7258/config`：已启用的 CP 驱动测试功能
- `.it.csv`：集成测试命令和预期输出，共 649 条用例
- `partitions/bk7258/`：测试分区和 RAM 区域定义

## 编译

在 SDK 根目录执行：

```text
make bk7258 PROJECT=test/platform
```

## 运行

烧录生成的 AP 和 CP 镜像，连接测试所需的外设线路和串口，然后复位开发板。从 `.it.csv` 中选择适用命令执行，并将串口输出与预期结果进行比较。不带前缀的命令在 CP 串口直接执行，带 `ap_cmd` 前缀的命令由 CP 转发到 AP。

示例命令：

```text
wdt_driver init
sadc 1 config 1 64 64 1
flash_test R 0x3da000 0x1000
ap_cmd pwm_driver init
ap_cmd uart_driver init
ap_cmd mbedtls_selftest
```

## 测试注意事项

部分用例需要外部接线或两块开发板（`.it.csv` 第三列的设备号为 `2` 的共 140 条，涉及 UART 收发与 SPI 主从对测）。Flash 测试会擦写 `0x3da000` 起的地址，看门狗重启测试会主动复位设备。在保存有重要数据的开发板上运行前，请先检查对应的 `.it.csv` 用例。
