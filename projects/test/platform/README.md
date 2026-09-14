# Platform Test Project

* [中文](./README_CN.md)

## Overview

`test/platform` is the BK7258 platform-driver integration and regression test project. Its project configuration enables SDK CLI tests for common peripherals and security functions.

The configured test coverage includes:

- watchdog, ADC/SADC, flash, GPIO, PSRAM, PUF, touch, and UART tests on the CP
- PWM, UART, I2C, SPI, flash, GPIO, SARADC, and cryptography tests on the AP, with the full mbedTLS build enabled
- CP-to-AP command forwarding, so the AP tests are reachable from the CP console

## Project layout

- `ap/ap_main.c`: AP initialization and optional SMP test setup
- `cp/cp_main.c`: CP initialization and CP1 startup
- `ap/config/bk7258_ap/config`: enabled AP driver test features
- `cp/config/bk7258/config`: enabled CP driver test features
- `.it.csv`: integration-test commands and expected output, 649 cases in total
- `partitions/bk7258/`: test partition and RAM-region definitions

## Build

Run from the SDK root:

```text
make bk7258 PROJECT=test/platform
```

## Run

Flash the generated AP and CP images, connect the required peripheral wiring and serial consoles, and reset the board. Execute an applicable command from `.it.csv` and compare the serial output with its expected result. Commands without a prefix run directly on the CP console; commands with the `ap_cmd` prefix are forwarded by the CP to the AP.

Examples include:

```text
wdt_driver init
sadc 1 config 1 64 64 1
flash_test R 0x3da000 0x1000
ap_cmd pwm_driver init
ap_cmd uart_driver init
ap_cmd mbedtls_selftest
```

## Test caution

Some cases require external wiring or two boards: 140 cases are marked device `2` in the third column of `.it.csv`, covering UART transmit/receive and SPI master/slave pairs. Flash tests erase and write the addresses starting at `0x3da000`, and watchdog reboot tests intentionally reset the device. Review each `.it.csv` case before running it on a board containing valuable data.
