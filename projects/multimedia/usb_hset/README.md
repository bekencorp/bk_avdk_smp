# USB HSET CDC Example

* [中文](./README_CN.md)

## 1. Overview

`usb_hset` is a USB 2.0 device compliance-test example for the BK7259. On
power-up it enumerates as a **USB High-Speed CDC-ACM device** so USBHSET / the
USB HS Electrical Test Tool can trigger the USB 2.0 high-speed test modes for
**signal-quality / eye-diagram** compliance testing.

How it works:

- On boot the AP brings up an HS CDC-ACM device (`bk_usb_hset_cdc_device_init()`).
- After the host enumerates it, the host issues the standard request
  `SET_FEATURE(TEST_MODE)`.
- The CherryUSB device core, once the control-transfer status stage completes,
  calls `usbd_execute_test_mode()` in the SDK MHDRC port, which writes the MUSB
  `TESTMODE` register to enter `Test_J` / `Test_K` / `Test_SE0_NAK` / `Test_Packet`.
- Capture D+/D- on a scope and overlay the USB-IF HS eye template.

## 2. Hardware

- SoC/board: BK7259 series development board.
- USB: connect the USB device port (MHDRC / high-speed PHY) to a PC or compliance host.
- Oscilloscope: to capture the D+/D- differential for eye / signal-quality measurement.
- Serial console: to view enumeration and test logs.

## 3. Layout

```text
usb_hset/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── ap/
│   ├── ap_main.c        # start the USB HSET CDC device on boot
│   ├── usb_hset_cdc.c   # HS CDC-ACM device descriptors + enumeration logic
│   └── config/
├── cp/
│   ├── cp_main.c
│   └── config/
└── partitions/
```

The low-level test-mode capability `usbd_execute_test_mode()` lives in the SDK
(`ap/components/bk_usb/CherryUSB_v1_6/port/beken_musb/usb_dc_beken_musb_mhdrc.c`),
gated by `CONFIG_USB_HSET`. The CDC enumeration logic (descriptors, class
interfaces, `usbd_initialize`) lives in this project's `ap/usb_hset_cdc.c`.

## 4. Build and flash

From the SDK root:

```bash
make bk7259 PROJECT=multimedia/usb_hset -j
```

Firmware output:

```text
build/bk7259/usb_hset/package/all-app.bin
```

Flash `all-app.bin` to the board and reset.

## 5. Usage

1. Flash the firmware and reset the board.
2. Connect the board's USB device port to a PC / compliance host.
3. The PC should enumerate a High-Speed CDC device named `BK7259 USB HSET CDC`
   (a virtual COM port appears: COMx on Windows, `/dev/ttyACM0` on Linux).
4. Serial log shows:
   ```text
   [usb-hset] HS CDC device up; waiting for host SET_FEATURE(TEST_MODE)
   [usb-hset] device CONFIGURED (HS)
   ```
5. In the compliance tool, select the root port this device is on, pick
   `TEST_PACKET` (eye diagram) or `Test_J` / `Test_K` / `Test_SE0_NAK`; the tool
   issues `SET_FEATURE(TEST_MODE)`.
6. Capture D+/D- and overlay the HS eye template.

Test selector to MUSB `TESTMODE` register mapping:

| Selector (HI byte of wIndex) | Test mode | TESTMODE value |
| --- | --- | --- |
| 1 | Test_J | 0x02 |
| 2 | Test_K | 0x04 |
| 3 | Test_SE0_NAK | 0x01 |
| 4 | Test_Packet (eye) | 0x08 (load the 53-byte standard packet into the EP0 FIFO first, then set TxPktRdy) |

## 6. Key configuration

```text
CONFIG_USB=y
CONFIG_BK_USB_CHERRYUSB_V1_6=y
CONFIG_USB_DEVICE=y
CONFIG_USB_HSET=y
```

- `CONFIG_USB_HSET=y` makes the SDK define `CONFIG_USBDEV_TEST_MODE` (enabling
  the CherryUSB device core dispatch of `SET_FEATURE(TEST_MODE)`), compiles
  `usbd_execute_test_mode()` in the MHDRC port, and compiles the device-side
  CDC-ACM class driver `usbd_cdc_acm.c`.
- Device builds force `CONFIG_USB_HS`, so the device enumerates at high speed
  (required for the eye test).
- `CONFIG_USB_HOST` is left at its default (on): host is not used at runtime, but
  the device-mode PHY/clock bring-up helpers and the RISC-V USB bridge live in the
  host controller file on this SDK, so the device link needs them.

## 7. Notes

1. Eye / test modes are only meaningful at USB High-Speed; confirm the PC
   enumerates the device as High-Speed (`480M` in USB Tree View / `lsusb -t`).
2. This project implements no host functionality; use `usb_example` for host.
3. VID/PID default to placeholder `0xFFFF/0xFFFF`, which does not affect the eye
   test; edit the descriptors in `ap/usb_hset_cdc.c` for a proper device name.
