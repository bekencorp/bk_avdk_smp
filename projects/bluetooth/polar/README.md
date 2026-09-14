# Bluetooth BLE Polar RF Mode Project

* [中文](./README_CN.md)

## Overview

`polar` is a BK7258 Bluetooth Low Energy controller-oriented project that selects **Polar mode**. It is intended as a low-power BLE RF configuration for use cases where transmit-power consumption is a key consideration, compared with the standard IQ-mode configurations.

The project is a platform configuration, rather than a complete end-user BLE profile demo. It provides a BLE 5.2 controller-only baseline; the AP application performs only basic startup and does not expose a default GATT service, provisioning flow, or product CLI.

> Polar mode is selected as a controller-level RF configuration. It is not an application command and is not intended to be switched for individual user packets.

## Polar mode and power behavior

The default project configuration provides the following behavior:

- BLE low-power operation with Polar mode selected for the controller;
- BLE transmit-power configuration appropriate for Polar mode;
- low-power clock support and PHY power-management support;
- RF coexistence-mode-switch capability for system integration.

Actual power consumption and RF performance depend on the board, supply, antenna, selected transmit power, RF channel, BLE role, advertising interval, connection interval, and traffic. Measure the target product under its intended operating conditions; this example does not define a guaranteed current reduction or a fixed power-saving percentage relative to IQ mode.

Use the normal application image—not an ATE/production-test image—to evaluate the product's Polar-mode behavior.

## Default configuration

| Area | Configuration |
| --- | --- |
| Target | BK7258 AP/CP SMP platform |
| Bluetooth type | BLE; Classic Bluetooth is disabled |
| Controller | BLE 5.2 controller-only mode |
| RF mode | Polar mode enabled in the default CP configuration |
| Low-power support | Low-power clock and PHY power-management options enabled |
| Coexistence | RF coexistence-mode-switch option enabled |
| Wi-Fi | Disabled after normal startup to avoid unnecessary system power consumption |
| Application behavior | No default GATT/profile application or product CLI |

## Enable Polar mode

The supplied default CP configuration already enables Polar mode. Its project configuration is located at:

```text
projects/bluetooth/polar/cp/config/bk7258/config
```

For a customized project, configure the CP image through menuconfig from the SDK root:

```bash
CCACHE_DISABLE=1 make bk7258_cp_menuconfig PROJECT=bluetooth/polar
```

Under **Bk_ble**, select **Polar Mode** in **Select Bluetooth RF Mode**. Keep BLE enabled and select the BLE controller version required by the product. For the same low-power configuration as this example, also enable:

```text
support lpo rosc
support coex rf mode switch
support sleep phy switch
```

Save the configuration and rebuild the project. Use menuconfig instead of manually editing generated configuration files so that dependent options remain consistent.

## Directory structure

```text
polar/
├── ap/
│   ├── ap_main.c                         # AP entry; initializes the platform
│   ├── config/bk7258_ap/config           # AP configuration with CLI
│   ├── config/bk7258_ap/no_cli.config    # AP configuration without CLI
│   └── config/bk7258_ap/lwipopts_custom.h
├── cp/
│   ├── cp_main.c                         # CP startup; starts CP1 and disables Wi-Fi
│   ├── config/bk7258/config              # CP BLE controller/RF configuration
│   └── config/bk7258/no_cli.config       # CP configuration without CLI
└── partitions/bk7258/                    # BK7258 partition and RAM-region definitions
```

## Build and flash

Build from the SDK root:

```bash
CCACHE_DISABLE=1 make bk7258 PROJECT=bluetooth/polar -j$(nproc)
```

Flash the resulting image with the standard BK7258 procedure. The command above uses the default `config` files.

To build the supplied no-CLI configuration, explicitly select it:

```bash
CCACHE_DISABLE=1 make bk7258 PROJECT=bluetooth/polar BK_CONFIG_FILE=no_cli -j$(nproc)
```

> `no_cli.config` is a separate configuration variant, not merely the default configuration with CLI output removed. Confirm its enabled Bluetooth and power-management capabilities meet the product requirement before using it.

## Calibration and test

This project provides no Polar-specific functional test command or profile-level test case. After building and flashing, first confirm normal boot, then test the intended BLE application or use the approved RF test environment.

RF calibration is required before evaluating RF performance or power consumption on a product board. Use a board that has completed the standard BK7258 production/RF calibration flow. Keep the `sys_rf` and `sys_net` entries in `partitions/bk7258/auto_partitions.csv`; they are reserved system partitions used by RF calibration and networking configuration, and their size and offset must not be changed.

This example does not replace the production calibration procedure and does not provide a standalone calibration command. Perform calibration through the approved manufacturing process, then measure the target product under its intended operating conditions.

## Extending the project

This project does not register a BLE application, GATT service, or user CLI by default. To turn it into an application:

1. Enable the required BLE application capabilities.
2. Add the application initialization and callbacks on the AP side.
3. Add the application source files and dependencies.
4. Keep AP and CP Bluetooth configuration compatible.

If BLE provisioning is required, enable and integrate the provisioning application, then validate its complete user flow.

## Verification

The supplied project has no default user-visible BLE service or product CLI, so it cannot be functionally verified through a profile-level command sequence. Verify normal boot first, then use the external test setup or the application added for the intended product use case.

For a Polar-versus-IQ power comparison, build otherwise identical images with the respective RF-mode choice, hold BLE role, advertising/connection intervals, payload, RF channel, TX power, clock source, and board supply conditions constant, then measure average and peak current. Do not use this project alone as evidence of a specific power-saving percentage.
