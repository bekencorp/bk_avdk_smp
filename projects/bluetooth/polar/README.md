# Bluetooth Polar Mode

* [中文](./README_CN.md)

This project enables Bluetooth RF Polar mode on BK7259. The CP-side Bluetooth
controller uses the BT polar transmit path together with the BT PLL, while the
AP side runs the Bluetooth host. AP CLI commands must use the `ap_cmd` prefix.

## Supported Targets

| Target | Status |
| --- | --- |
| BK7259 | Supported |

## Build and Flash

Build from the SDK root:

```bash
make bk7259 PROJECT=bluetooth/polar
```

Docker build:

```bash
./dbuild.sh make bk7259 PROJECT=bluetooth/polar
```

Firmware output:

```text
build/bk7259/polar/package/all-app.bin
```

## How Polar Mode Is Enabled

Polar mode is enabled by `CONFIG_BLUETOOTH_RF_MODE_POLAR=y` in the CP defconfig:

```text
projects/bluetooth/polar/cp/config/bk7259/defconfig
```

It can also be selected in menuconfig under `Bk_ble` > `Select Bluetooth RF Mode`
by choosing `Polar Mode`.
