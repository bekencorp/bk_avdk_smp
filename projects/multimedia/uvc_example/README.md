# UVC Camera Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates **USB Host + UVC** external camera capture on BK7259, with high-level `uvc` one-shot streaming and low-level `uvc_api` step-by-step API debugging.

* Developer guide: [UVC Camera](../../../developer-guide/camera/uvc.html)

* API reference:

  - [Camera API](../../../api-reference/multimedia/bk_camera.html)
  - [Frame buffer](../../../api-reference/multimedia/bk_frame_buffer.html)

### 1.1 Test Environment

- Hardware
  - Core board: **BK7259_QF128_12.3X12.3_V4.0**
  - PSRAM: 32M
  - **USB Host port + UVC camera** (MJPEG / YUV / H264 / H265 per device capability)
- Default demo: port `1`, 864×480@30fps MJPEG (`MEDIA_UVC_MJPEG_864X480_30FPS_CONFIG()`)

.. warning::

    Use a UVC device that supports the requested format. Resolution and format must match device capabilities or `uvc_checkout_port_info` will fail.

## 2. Directory Layout

```text
uvc_example/
├── ap/
│   ├── ap_main.c
│   └── src/uvc_test.c
├── cp/
└── partitions/
```

## 3. Features

### 3.1 `uvc` High-Level Commands

```text
uvc open <port> <width> <height> <h264|h265|mjpeg|yuv>
uvc close <port>
```

Uses defaults (864×480@30 MJPEG, port 1) when arguments are omitted. Internally: `uvc_camera_power_on` → port capability check → `bk_uvc_ctrl_new/init/open`.

### 3.2 `uvc_api` Step-by-Step Commands

```text
uvc_api power_on | power_off
uvc_api new | delete
uvc_api init | deinit
uvc_api open <width> <height> <h264|h265|mjpeg|yuv>
uvc_api close | suspend | resume
```

### 3.3 Boot Behavior

**No auto demo** — CLI registered after `bk_init()` and `bk_frame_buffer_init()`.

## 4. Build and Run

### 4.1 Build

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/uvc_example -j$(nproc)
```

### 4.2 Examples

```text
uvc open 1 864 480 mjpeg
uvc close 1
```

Step-by-step:

```text
uvc_api power_on
uvc_api new
uvc_api init
uvc_api open 864 480 mjpeg
uvc_api close
uvc_api deinit
uvc_api delete
uvc_api power_off
```

## 5. Notes

1. Hub ports limited by `CONFIG_USBHOST_HUB_MAX_EHPORTS` / `UVC_PORT_MAX`.
2. Frame buffers use PSRAM by default (`CONFIG_UVC_USE_PSRAM_ALLOC`).
3. For display + H264 encode pipeline, see `uvc_display_example`.
