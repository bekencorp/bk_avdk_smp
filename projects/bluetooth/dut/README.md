# Bluetooth DUT Test

* [中文](./README_CN.md)

This project provides Bluetooth DUT (Device Under Test) mode for BK7259. Use
the CLI commands to enter or exit DUT mode.

## Supported Targets

| Target | Status |
| --- | --- |
| BK7259 | Supported |

## 1. Overview

The project enters Bluetooth DUT test mode by default after startup. The
`bt_dut_test` CLI command can also be used to control DUT mode.

The advertised Classic Bluetooth device name is `BK_DUT_TEST`.

## 2. Requirements

| Item | Requirement |
| --- | --- |
| Target board | BK7259 development board |
| Test instrument | CMW or another Bluetooth DUT-compatible tester |
| UART | UART0 for flashing, logs, and CLI commands |
| Hardware setting | ATE jumper on the serial adapter board must be shorted |
| Firmware output | `build/bk7259/dut/package/all-app.bin` |

## 3. Build and Flash

Build from the SDK root:

```bash
make bk7259 PROJECT=bluetooth/dut
```

Docker build:

```bash
./dbuild.sh make bk7259 PROJECT=bluetooth/dut
```

Flash the combined image with BKFIL or the standard project flashing tool:

```text
build/bk7259/dut/package/all-app.bin
```

## 4. Test Procedure

1. Flash `all-app.bin` to the BK7259 board.
2. Short the ATE jumper on the serial adapter board.
3. Connect UART0 and power-cycle the board.
4. Wait for the following log:

   ```text
   DUT test initialized and enabled
   ```

5. Search for `BK_DUT_TEST` on the test instrument, connect to it, and run the
   required Bluetooth DUT tests.
6. Disable DUT mode after testing:

   ```text
   ap_cmd bt_dut_test disable
   ```

This project intentionally starts the AP core while the ATE strap is active so
that AP CLI commands remain available.

## 5. CLI Reference

On BK7259 SMP, the command is registered on AP and must use the `ap_cmd`
prefix.

| Command | Description |
| --- | --- |
| `ap_cmd bt_dut_test enable` | Start controller DUT mode, enable Classic Bluetooth inquiry/page scan, and send HCI opcode `0x1803`. |
| `ap_cmd bt_dut_test disable` | Stop controller DUT mode and disable Classic Bluetooth inquiry/page scan. |
| `ap_cmd bt_dut_test scan_enable` | Enable Classic Bluetooth inquiry/page scan without starting DUT mode. |

A successful command returns:

```text
DUT TEST RSP:OK
```

An invalid command or an underlying operation failure returns:

```text
DUT TEST RSP:ERROR
```

## 6. Troubleshooting

### `cmd NOT found: bt_dut_test`

Confirm that the `bluetooth/dut` image is flashed and wait for
`DUT test initialized and enabled` before entering the command. The command
must include the `ap_cmd` prefix.

### The test instrument cannot find the device

Confirm that `DUT TEST RSP:OK` is returned. Then run:

```text
ap_cmd bt_dut_test scan_enable
```

Search again for `BK_DUT_TEST`.

### DUT mode does not start

Confirm that the ATE jumper was shorted before power-on. After changing the
jumper state, power-cycle the board.

## 7. Notes

- This project supports only the BK7259 AP/CP architecture.
- Use this project firmware for DUT testing. Normal Bluetooth examples do not
  start AP while the ATE strap is active.
- After testing, run `disable`, remove the ATE jumper, and power-cycle the board
  to return to normal operation.
