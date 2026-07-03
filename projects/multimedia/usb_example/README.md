# USB Example

* [中文](./README_CN.md)

## 1. Overview

`usb_example` is a USB device/host dual-role example for BK7259. After boot, the firmware starts as a USB device MSC mass-storage gadget by default. It also provides CLI commands to switch to USB host mode at runtime, verify USB device enumeration, run U-disk read/write tests, receive MJPEG frames from a UVC camera, and switch to MTP device mode so a PC can browse files on the board-side SD card.

This example includes:

- USB device MSC: the default U-disk gadget started after boot.
- USB host enumeration: prints standard descriptors of an attached USB device.
- USB host MSC: enumerates an attached U-disk, mounts FatFs drive `2:`, writes a file and verifies read-back.
- USB host UVC: enumerates a UVC camera, opens an MJPEG stream, receives and validates complete JPEG frames.
- USB device MTP: stops the default MSC gadget, mounts SD card `/sd0`, and exposes files to the PC through MTP.
- CherryUSB v1.6 and RISC-V USB bridge path verification.

## 2. Hardware Requirements

- SoC/board: BK7259 series development board.
- USB device connection: connect the USB device port to a PC for MSC or MTP device mode.
- USB host connection: connect a U-disk, UVC camera or another USB device to the USB host port.
- USB host power: make sure VBUS is supplied correctly in host mode.
- SD card: required by MTP mode. The filesystem mount path is `/sd0`.
- Debug interface: UART console for CLI commands and logs.

Note: the same USB controller can only work in one role at a time. After switching to host mode, the default MSC device connection is no longer used as a U-disk gadget. Starting MTP also stops the default MSC gadget first.

## 3. Project Structure

```text
usb_example/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c      # USB MSC/MTP/U-disk CLI and default MSC startup
│   ├── uvc_test.c     # UVC camera MJPEG receive test
│   └── config/
├── cp/
│   ├── cp_main.c
│   └── config/
└── partitions/
```

## 4. Build And Flash

Run the build command from the SDK root directory:

```bash
make bk7259 PROJECT=multimedia/usb_example -j
```

After the build completes, the firmware image is generated at:

```text
build/bk7259/usb_example/package/all-app.bin
```

Flash `all-app.bin` to the board. After flashing, reset the board and use the UART console to run commands.

AP-side commands must be sent from the main console with the `ap_cmd` prefix, for example:

```text
ap_cmd udisk status
ap_cmd mtp status
ap_cmd uvc test
```

## 5. USB Device MSC Mode

After boot, the project calls `msc_storage_init()` automatically and enumerates as a USB MSC device. The PC should detect a U-disk device.

Check current status:

```text
ap_cmd udisk status
```

The default state usually contains:

```text
mode=device, driver_init=0, host_media=not_ready
```

To check MTP status, run:

```text
ap_cmd mtp status
```

If the board was previously switched to host mode, switch back to the device role with:

```text
ap_cmd udisk dev
```

If MTP has been started, `ap_cmd mtp stop` only stops the MTP gadget and does not restart the default MSC gadget. Reset the board to restore the boot-time default MSC device mode.

## 6. USB Host Device Enumeration

`udisk enum` switches the controller to host mode, waits for an attached USB device to enumerate, and prints the device, configuration, interface and endpoint descriptors. Use this command to quickly check whether a U-disk, UVC camera or another USB device can be recognized by the host controller.

Steps:

1. Make sure VBUS is supplied on the USB host port.
2. Connect the USB device to the host port.
3. Run the enumeration command.

```text
ap_cmd udisk enum
```

Expected logs:

```text
==== USB host enumeration BEGIN ====
HOST mode active
VID:PID=....
interfaces=...
==== USB host enumeration PASS ====
```

## 7. USB Host U-Disk Read/Write Test

`udisk test` switches to host mode, waits for U-disk enumeration, mounts FatFs drive `2:`, writes a test file and verifies read-back.

Steps:

1. Make sure VBUS is supplied on the host port.
2. Connect the U-disk to the host port.
3. Run the test command.

```text
ap_cmd udisk test
```

Test flow:

1. Switch from device mode to host mode.
2. Wait for U-disk enumeration and MSC class registration.
3. Mount FatFs drive `2:`.
4. Print the file list before writing.
5. Write `2:/bk_udisk_test.txt`.
6. Read back 512 bytes and verify the data.
7. Print the file list after writing.
8. Unmount `2:`.

Expected logs:

```text
U-disk media READY
mounted 2:
---- U-disk file list (before write) ----
wrote 512 bytes -> 2:/bk_udisk_test.txt
read-back PASS
---- U-disk file list (after write) ----
==== U-disk host R/W test PASS ====
```

Related commands:

- `ap_cmd udisk host`: switch to USB host and wait for U-disk media ready.
- `ap_cmd udisk dev`: switch back to USB device MSC.
- `ap_cmd udisk ls`: print the mounted U-disk `2:` root directory. This is mainly for debugging when the drive is already mounted.

## 8. USB Device MTP Mode

MTP is another USB device gadget beside MSC. When MTP starts, the project first stops the default MSC gadget, mounts the SD card at `/sd0`, and re-enumerates to the PC as an MTP device.

Start MTP:

```text
ap_cmd mtp start
```

Check MTP status:

```text
ap_cmd mtp status
```

Stop MTP:

```text
ap_cmd mtp stop
```

Expected logs:

```text
MTP: deinit MSC gadget
[mtp] mounted SD card at /sd0
MTP: usb_mtp_init ret=0
==== MTP device active (browse the SD card on the PC) ====
mtp_notify_handler:7
```

PC-side behavior:

- Windows: File Explorer should show a portable device named `BekenMTP` by default.
- Linux: use file manager MTP/GVFS integration, or tools such as `mtp-detect` and `mtp-files`.
- macOS: MTP is not supported natively. Use an Android File Transfer style tool.

MTP-related configuration:

```text
CONFIG_USBD_MTP=y
CONFIG_USBD_MTP_PRODUCT_NAME="BekenMTP"
CONFIG_USBD_MTP_DEVICE_TYPE="1"
```

`CONFIG_USBD_MTP_DEVICE_TYPE` maps to the MTP `PerceivedDeviceType`. The default value `1` means still image camera.

## 9. USB Host UVC Camera Test

The `uvc` command verifies USB host UVC camera MJPEG receive flow. It releases the default MSC device gadget, switches the USB controller to host mode, enumerates the UVC camera on the selected port, opens an MJPEG stream and validates complete JPEG frames.

Default parameters:

- Host port: `1`
- Resolution: `1920x1080`
- Frame rate: `30fps`
- Self-test target: at least 20 complete MJPEG frames

Run the default test:

```text
ap_cmd uvc test
```

Specify port, resolution and frame rate:

```text
ap_cmd uvc test 1 1280 720 30
```

Open a stream and keep receiving frames:

```text
ap_cmd uvc open 1 1920 1080 30
```

Close the stream:

```text
ap_cmd uvc close 1
```

Expected logs:

```text
==== UVC MJPEG receive test BEGIN (port=1 1920x1080@30, target=20 frames) ====
host prepared (MSC gadget released, host class drivers registered)
camera ready on port 1 after ... ms
camera VID:PID=....
UVC opened: port=1 MJPEG 1920x1080@30
MJPEG frame #1 OK ...
MJPEG frame #20 OK ...
==== UVC MJPEG receive test PASS ====
```

If the camera does not list the requested fps, the example falls back to the first fps reported for that resolution. If the camera does not support the default `1920x1080@30` mode, use an MJPEG mode supported by the camera.

## 10. Command Reference

All AP-side commands require the `ap_cmd` prefix when sent from the main console.

- `ap_cmd udisk status`: print current USB mode, driver init state and host media-ready state.
- `ap_cmd udisk host`: switch to USB host and wait for U-disk media ready.
- `ap_cmd udisk dev`: switch back to USB device MSC gadget.
- `ap_cmd udisk enum`: switch to host, wait for any USB device to enumerate and print descriptors.
- `ap_cmd udisk test`: switch to host, enumerate a U-disk, mount, list files, write, read back and verify.
- `ap_cmd udisk ls`: print the mounted U-disk `2:` root directory.
- `ap_cmd mtp start`: stop MSC, start MTP device and mount SD card `/sd0`.
- `ap_cmd mtp stop`: stop MTP device. This command does not restore the default MSC device automatically.
- `ap_cmd mtp status`: print MTP active state.
- `ap_cmd uvc test [port] [w] [h] [fps]`: open UVC MJPEG and validate at least 20 complete JPEG frames.
- `ap_cmd uvc open [port] [w] [h] [fps]`: open a UVC MJPEG stream and keep running.
- `ap_cmd uvc close [port]`: stop and close the UVC stream on the selected port.

## 11. Key Configuration

This example depends on the following main configuration options:

```text
CONFIG_USB=y
CONFIG_USB_DEVICE=y
CONFIG_USBD_MSC=y
CONFIG_USBD_MTP=y
CONFIG_USB_HOST=y
CONFIG_USB_HUB=y
CONFIG_USBH_MSC=y
CONFIG_USBH_UVC=y
CONFIG_USB_CAMERA=y
CONFIG_BK_USB_CHERRYUSB_V1_6=y
CONFIG_USB_RISCV_BRIDGE=y
CONFIG_SDCARD=y
CONFIG_FATFS=y
CONFIG_FATFS_SDCARD=y
```

## 12. Notes

1. USB device and USB host cannot use the same controller at the same time. Host or MTP commands change the current USB role.
2. Host mode requires external VBUS power. Incorrect power supply can prevent enumeration.
3. `udisk test` writes `bk_udisk_test.txt` to the U-disk root directory. Do not run it on a disk that contains important data without backup.
4. MTP uses SD card `/sd0` as backend storage. Make sure the SD card is inserted and can be mounted before starting MTP.
5. `mtp stop` only stops the MTP gadget. Reset the board to restore the boot-time default MSC device mode.
6. Windows may cache MTP names and icons by VID, PID and Serial. After changing the product name or device type, uninstall the old portable-device record or use another USB port to re-enumerate.

## 13. Troubleshooting

### Command Not Found

Make sure the command uses the `ap_cmd` prefix, for example:

```text
ap_cmd udisk status
ap_cmd mtp status
```

### USB Host Enumeration Fails

Check host-port VBUS, USB cable, device connection and device power. Run `ap_cmd udisk enum` first to confirm whether any USB device can enumerate.

### U-Disk Read/Write Test Fails

Make sure the U-disk is connected to the host port and that the filesystem can be recognized by FatFs. If the log shows `NO_FILESYSTEM`, format the U-disk as FAT/FAT32 and retry.

### UVC Camera Open Fails

Make sure the camera is a UVC device, the port argument is correct, and the selected MJPEG resolution and fps are supported by the camera. Run `ap_cmd udisk enum` first to check whether the device can enumerate.

### MTP Shows No Files

Make sure the SD card is inserted and the log contains:

```text
[mtp] mounted SD card at /sd0
```

### MTP Name Or Icon Does Not Refresh

Windows may cache old device information. Uninstall the old portable-device record in Device Manager, or use another USB port and replug the device.
