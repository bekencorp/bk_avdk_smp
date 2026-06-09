# USB Example (BK7259)

* [中文](./README_CN.md)

## 1. Overview

`usb_example` verifies BK7259 USB device/host dual-role behavior. The firmware boots as a USB device MSC mass-storage gadget by default. It also provides CLI commands to switch to USB host mode at runtime, enumerate attached USB devices, run U-disk file read/write tests, and switch to an MTP device gadget so a PC can browse the board-side SD card.

The example covers:

- USB device MSC:default boot U-disk gadget.
- USB host enumeration:enumerate any attached USB device and print descriptors.
- USB host MSC:detect an attached U-disk, mount FatFs drive `2:`, print the file list, write a test file and verify read-back.
- USB device MTP:switch from the default MSC gadget to an MTP gadget, mount SD card `/sd0`, and expose files to a PC as a portable MTP device.
- CherryUSB v1.6 + RISC-V USB bridge:the host and device low-level paths can run through the RISC-V CP bridge.

## 2. Build And Flash

This is an SDK-internal project. Build it from the SDK root; do not enter the project directory and run make there.

```bash
cd <workspace>/bk_avdk_smp_dev_7259v2_bringup_25W4801
make bk7259 PROJECT=multimedia/usb_example -j32
```

Main outputs:

```text
build/bk7259/usb_example/package/all-app.bin
build/bk7259/usb_example/package/app_pack.rbl
```

Flash `all-app.bin` to BK7259, then connect to the CLI UART. Commands registered on the AP side must be sent from the main console with the `ap_cmd` prefix, for example:

```text
ap_cmd udisk status
ap_cmd mtp start
```

## 3. Hardware Setup

### Device Mode

Connect the board USB device port to the PC with a USB data cable. After boot, the firmware starts the default MSC U-disk gadget automatically.

### Host Mode

Before switching to host mode, make sure:

- External VBUS power is supplied to the USB host port.
- The U-disk or UVC camera is connected to the host port.
- After switching from device to host, the previous PC device connection is no longer used as MSC/MTP.

### MTP Storage

The MTP backend uses the SD card filesystem. Make sure the SD card is inserted before starting MTP. Startup log should contain:

```text
[mtp] mounted SD card at /sd0
```

## 4. USB Device:Default U-Disk Mode

On boot the project runs `msc_storage_init()` automatically and enumerates as a USB MSC device. The PC should see a U-disk device.

Common check:

```text
ap_cmd udisk status
```

Expected default state:

```text
mode=device
mtp active=0
```

If the board was previously switched to host or MTP, restore device MSC with:

```text
ap_cmd mtp stop
ap_cmd udisk dev
```

## 5. USB Host:Device Enumeration

`udisk enum` switches the controller to host mode, waits for a device to enumerate and prints the standard device/config/interface/endpoint descriptors. It is useful for checking whether a U-disk, UVC camera or another USB device can enumerate.

```text
ap_cmd udisk enum
```

Expected key logs:

```text
==== USB host enumeration BEGIN ====
HOST mode active
New high-speed device ...
VID:PID=....
interfaces=...
==== USB host enumeration PASS ====
```

## 6. USB Host:U-Disk Read/Write Test

Insert a U-disk and make sure host VBUS is supplied, then run:

```text
ap_cmd udisk test
```

Test flow:

1. Switch from device mode to host mode.
2. Wait for U-disk enumeration and MSC class registration.
3. Mount FatFs drive `2:`.
4. Print the file list before writing.
5. Write `2:/bk_udisk_test.txt`.
6. Read back 512 bytes and verify the content.
7. Print the file list after writing.
8. Unmount `2:`.

Expected key logs:

```text
U-disk media READY
mounted 2:
---- U-disk file list (before write) ----
wrote 512 bytes -> 2:/bk_udisk_test.txt
read-back PASS
---- U-disk file list (after write) ----
==== U-disk host R/W test PASS ====
```

Switch to host and wait for media ready only:

```text
ap_cmd udisk host
```

Print the mounted U-disk root directory manually:

```text
ap_cmd udisk ls
```

Switch back to device MSC:

```text
ap_cmd udisk dev
```

## 7. USB Device:MTP Mode

MTP is an alternative device gadget beside MSC. Starting MTP first stops the default MSC gadget, mounts the SD card at `/sd0`, and re-enumerates to the PC as an MTP device.

Start MTP:

```text
ap_cmd mtp start
```

Check status:

```text
ap_cmd mtp status
```

Stop MTP:

```text
ap_cmd mtp stop
```

Expected key logs:

```text
MTP: deinit MSC gadget
[mtp] mounted SD card at /sd0
mtp_notify_handler:11
[bk_v1_6] usb device use riscv CP bridge path
MTP: usb_mtp_init ret=0
==== MTP device active (browse the SD card on the PC) ====
mtp_notify_handler:1
mtp_notify_handler:7
```

Event meanings:

- `11`:USBD init
- `1`:USB bus reset
- `7`:configured, the PC has finished configuring the device

PC side:

- Windows:File Explorer should show a portable device. The default name is `BekenMTP`.
- Linux:use the file manager MTP/GVFS integration, or tools such as `mtp-detect` and `mtp-files`.
- macOS:does not support MTP natively; use an Android File Transfer style tool.

MTP-related configuration:

```text
CONFIG_USBD_MTP=y
CONFIG_USBD_MTP_PRODUCT_NAME="BekenMTP"
CONFIG_USBD_MTP_DEVICE_TYPE="1"
```

`CONFIG_USBD_MTP_DEVICE_TYPE` maps to MTP `PerceivedDeviceType`:

- `0`:generic
- `1`:still image camera, current default
- `2`:media player
- `3`:phone
- `4`:video camera
- `5`:PIM
- `6`:audio recorder

Note:Windows may cache MTP names and icons by VID/PID/Serial. After changing the name or type, uninstall the old portable-device record in Device Manager, use another USB port, or change the serial and re-enumerate.

## 8. Command Reference

All commands should use the `ap_cmd` prefix when sent from the main console.

| Command | Description |
| --- | --- |
| `ap_cmd udisk status` | Print current USB mode, driver init state and host media-ready state |
| `ap_cmd udisk host` | Switch to USB host and wait for U-disk media ready |
| `ap_cmd udisk dev` | Switch back to USB device MSC gadget |
| `ap_cmd udisk enum` | Switch to host, wait for any USB device to enumerate and print descriptors |
| `ap_cmd udisk test` | Switch to host, enumerate U-disk, mount, list files, write, read back and verify |
| `ap_cmd udisk ls` | Print the mounted U-disk `2:` root directory |
| `ap_cmd mtp start` | Stop MSC, start MTP device and mount SD `/sd0` |
| `ap_cmd mtp stop` | Stop MTP device |
| `ap_cmd mtp status` | Print MTP active state |

## 9. Troubleshooting

### Command Not Found

Make sure the `ap_cmd` prefix is used:

```text
ap_cmd udisk status
ap_cmd mtp status
```

### U-Disk Host Enumeration Fails

Check host-port VBUS, U-disk connection and USB data cable. Run `ap_cmd udisk enum` first to see whether any USB device can enumerate.

### MTP Shows No Files

Check that the SD card is inserted and the log contains:

```text
[mtp] mounted SD card at /sd0
```

### MTP Name Or Icon Does Not Refresh

Windows may cache old device information. Uninstall the old portable-device record in Device Manager, use another USB port, or change the serial/PID and replug.
