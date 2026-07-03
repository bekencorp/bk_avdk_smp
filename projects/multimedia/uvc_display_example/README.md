# UVC Display Pipeline Example

* [中文](./README_CN.md)

## 1. Project Overview

This project extends `uvc_example` with an end-to-end **UVC MJPEG capture → HW decode → GPU compose → MIPI display** pipeline.

Differences from `uvc_example`:

| Item | `uvc_example` | `uvc_display_example` |
|------|---------------|------------------------|
| Goal | UVC streaming and API debug | Show UVC preview on LCD |
| CLI | `uvc` + `uvc_api` step APIs | `uvc` / `decode` / `display` / `pipeline` |
| Decode / display | None | `bk_decoder` Flexa + VG-Lite GPU + DPU |
| Recommended cmd | `uvc open ...` | `pipeline open ...` |

Data flow (`pipeline open`):

```text
UVC(MJPEG) → encode_frame_que → MJPEG Flexa decode (bk_decoder)
    → bk_flexa_mjpegd_gpu_bond → VG-Lite GPU → DPU → MIPI LCD
```

The active tree does **not** use legacy VPU APIs and does **not** include H264 encode. See `verisilicon_nano/legacy/README.md` for archived code.

* Related docs:

  - [UVC Camera](../../../developer-guide/camera/uvc.html)
  - [JPEG Decode (VPU)](../../../developer-guide/vpu/jpeg_decode.html)
  - [Frame vs Flexa Mode](../../../developer-guide/vpu/flexa_frame.html)

### 1.1 Test Environment

- Hardware
  - Core board: **BK7259_QF128_12.3X12.3_V4.0**
  - PSRAM: 32M
  - **USB UVC camera** (MJPEG required; resolution should match decoder config)
  - **MIPI DSI panel**: default `LCD_HX8399C_MIPI_1080x1920`
- Key Kconfig: `CONFIG_USB`, `CONFIG_BK_VCDEC`, `CONFIG_BK_DECODER`, `CONFIG_VG_LITE_GPU`, `CONFIG_BK_DISPLAY`, `CONFIG_DPU_DRIVER`

.. warning::

    Requires both a UVC camera and the reference MIPI panel. If the camera does not support the requested resolution/fps, `uvc_checkout_port_info` fails — adjust `pipeline open` parameters accordingly.

## 2. Directory Layout

```text
uvc_display_example/
├── ap/
│   ├── ap_main.c
│   ├── include/
│   │   └── decode_test.h
│   └── src/
│       ├── uvc_test.c         # UVC capture, feeds encode_frame_que
│       ├── decode_test.c      # MJPEG Flexa decode (bk_jpeg_decode_flexa_*)
│       ├── display_test.c     # MIPI LCD + DPU + GPU
│       ├── pipeline_test.c    # One-shot decode + display + bond + UVC
│       └── encode_frame_que.c # Frame queue between UVC and decoder
├── cp/
└── partitions/
```

## 3. Features

### 3.1 CLI Commands

| Command | Usage | Description |
|---------|-------|-------------|
| **uvc** | `uvc open <port> <w> <h> [mjpeg\|yuv\|h264\|h265]` | Start stream; defaults to `mjpeg` if format omitted |
| | `uvc close <port>` | Stop stream |
| **decode** | `decode open` / `close` | Open/close MJPEG Flexa decode (fixed 1920×1080) |
| **display** | `display open` / `close` | LCD on/off; GPU video path only when decode is open |
| **pipeline** | `pipeline open [port w h fps]` | **Recommended**: decode + display + bond + UVC |
| | `pipeline close` | Tear down full pipeline |

`pipeline open` defaults: port `1`, 1920×1080@30.

### 3.2 `pipeline open` Sequence

1. `decode_test_open`: MJPEG Flexa decoder and decode thread
2. `display_test_open_with_gpu_flexa`: display with decoder Flexa ring as GPU input
3. `bk_flexa_mjpegd_gpu_bond_start`: sync read pointers between decoder and GPU
4. `uvc_camera_turn_on`: UVC stream, MJPEG frames via `encode_frame_que` to decoder

### 3.3 Boot Behavior

**No auto demo** — run CLI commands manually after boot.

## 4. Build and Run

### 4.1 Build

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/uvc_display_example -j$(nproc)
```

### 4.2 Run Examples

**End-to-end display (recommended):**

```text
pipeline open 1 1920 1080 30
pipeline close
```

**Step-by-step debug (module check only; use `pipeline open` for full preview):**

```text
decode open
display open          # LCD only if decode is not open
uvc open 1 1920 1080 mjpeg
uvc close 1
display close
decode close
```

## 5. Notes

1. No `uvc_api` commands (see `uvc_example`).
2. UVC resolution, `pipeline open` args, and `decode open` (1920×1080) should match.
3. `display open` alone without `decode open` logs `LCD only` and shows no UVC picture.
4. See VPU Flexa docs for ring buffers and MJPEGD-GPU bond details.
