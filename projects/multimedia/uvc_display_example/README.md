# UVC Display and Encode Pipeline Example

* [中文](./README_CN.md)

## 1. Project Overview

This project extends `uvc_example` with an end-to-end pipeline: **UVC capture → MJPEG HW decode → GPU compose → MIPI display → H264 HW encode**, demonstrating multimedia module integration and Flexa low-latency paths.

* Related docs:

  - [UVC Camera](../../../developer-guide/camera/uvc.html)
  - [JPEG Decode (VPU)](../../../developer-guide/vpu/jpeg_decode.html)
  - [H.264 Encode (VPU)](../../../developer-guide/vpu/h264_encode.html)
  - [Frame vs Flexa Mode](../../../developer-guide/vpu/flexa_frame.html)

### 1.1 Test Environment

- Hardware
  - Core board: **BK7259_QF128_12.3X12.3_V4.0**
  - PSRAM: 32M
  - **USB UVC camera** (pipeline uses MJPEG)
  - **MIPI DSI panel**: default `LCD_HX8399C_MIPI_1080x1920`
- Software: USB UVC, `CONFIG_BK_VCDEC`, VG-Lite GPU, `CONFIG_BK_H264E`, DPU display

.. warning::

    Requires both a UVC camera and the reference MIPI panel. Different panel or UVC specs need Kconfig and parameter changes.

## 2. Directory Layout

```text
uvc_display_example/
├── ap/
│   ├── ap_main.c
│   └── src/
│       ├── uvc_test.c
│       ├── decode_test.c
│       ├── display_test.c
│       ├── gpu_test_v2.c
│       ├── pipeline_test.c
│       └── encode_frame_que.c
├── cp/
└── partitions/
```

## 3. Features

### 3.1 Per-Module CLI

| Command | Subcommands | Description |
|---------|-------------|-------------|
| **uvc** | `open` / `close` | Same as `uvc_example` |
| **decode** | `open` | MJPEG decode, default **Flexa**, 1920×1080 |
| | `open frame` | MJPEG decode **Frame** mode |
| | `close` | Close decoder |
| **display** | `open` / `close` | HX8399C MIPI + DPU + GPU |
| **pipeline** | `open` / `close` | End-to-end pipeline |

`pipeline open` syntax:

```text
pipeline open [<port> <width> <height> <fps>]
```

Defaults: port `1`, 1920×1080@30.

### 3.2 Pipeline Stages

On `pipeline open`:

1. MJPEG HW decode (Flexa)
2. Display + GPU
3. H264 Flexa encode thread with decode ring buffer
4. UVC streaming into shared frame queue

### 3.3 Boot Behavior

**No auto demo** — run `pipeline open` or per-module commands manually.

## 4. Build and Run

### 4.1 Build

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/uvc_display_example -j$(nproc)
```

### 4.2 Suggested Flow

**End-to-end:**

```text
pipeline open 1 1920 1080 30
pipeline close
```

**Step-by-step:**

```text
decode open
display open
uvc open 1 864 480 mjpeg
...
```

## 5. Notes

1. No `uvc_api` commands (unlike `uvc_example`).
2. Pipeline expects MJPEG from UVC; align UVC resolution with decoder config.
3. H264 output logs include first I/P frame info (default GOP 30).
4. See VPU Flexa docs for ring buffers, GPU bond, and read-pointer sync.
