# Bluetooth Polar 模式

* [English](./README.md)

本工程在 BK7259 上开启蓝牙 RF Polar 模式。CP 侧 Bluetooth Controller 使用 BT
Polar 发射通道和 BT PLL，AP 侧运行 Bluetooth Host，AP CLI 命令需加 `ap_cmd`
前缀。

## 支持的芯片

| 芯片 | 状态 |
| --- | --- |
| BK7259 | 支持 |

## 编译与烧录

在 SDK 根目录编译：

```bash
make bk7259 PROJECT=bluetooth/polar
```

使用 Docker 编译：

```bash
./dbuild.sh make bk7259 PROJECT=bluetooth/polar
```

烧录产物：

```text
build/bk7259/polar/package/all-app.bin
```

## Polar 模式开启方式

Polar 模式由 CP 侧配置中的 `CONFIG_BLUETOOTH_RF_MODE_POLAR=y` 开启：

```text
projects/bluetooth/polar/cp/config/bk7259/defconfig
```

也可在 menuconfig 的 `Bk_ble` > `Select Bluetooth RF Mode` 中选择 `Polar Mode`。
