# Platform Test Project

* [中文](./README_CN.md)

## Overview

`test/platform` is the BK7259 platform-driver integration and regression test project. Its project configuration enables SDK CLI tests for common peripherals and security functions.

The configured test coverage includes:

- watchdog and ADC tests on the CP
- PWM, UART, I2C, SPI, flash, GPIO, ADC, and cryptography tests on the AP
- AP-to-CP command forwarding for tests exposed through the CP console

## Project layout

- `ap/ap_main.c`: AP initialization and optional IPC test setup
- `cp/cp_main.c`: CP initialization and AP startup
- `ap/config/bk7259_ap/defconfig`: enabled AP driver test features
- `cp/config/bk7259/defconfig`: enabled CP driver test features
- `.it.csv`: integration-test commands and expected output
- `partitions/bk7259/`: test partition and RAM-region definitions

## Build

Run from the SDK root:

```text
make bk7259 PROJECT=test/platform
```

## Run

Flash the generated AP and CP images, connect the required peripheral wiring and serial consoles, and reset the board. Execute an applicable command from `.it.csv` and compare the serial output with its expected result.

Examples include:

```text
wdt_driver init
ap_cmd pwm_driver init
ap_cmd uart_driver init
ap_cmd mbedtls_selftest
```

## Test caution

Some cases require external wiring or two boards. Flash tests erase and write the addresses specified by the command, and watchdog reboot tests intentionally reset the device. Review each `.it.csv` case before running it on a board containing valuable data.
