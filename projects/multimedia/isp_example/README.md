# ISP Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates **MIPI CSI / DVP** camera capture and **ISP** processing on BK7259, including sensor auto-detect, MP/SP dual channels, Frame / Flexa output modes, and ISP tuning/dump utilities.

* Developer guide:

  - [Camera overview](../../../developer-guide/camera/index.html)
  - [MIPI CSI](../../../developer-guide/camera/mipi_csi.html)
  - [DVP](../../../developer-guide/camera/dvp.html)

* API reference:

  - [Camera API](../../../api-reference/multimedia/bk_camera.html)
  - [Frame buffer](../../../api-reference/multimedia/bk_frame_buffer.html)

### 1.1 Test Environment

- Hardware
  - Core board: **BK7259_QF128_12.3X12.3_V4.0**
  - PSRAM: 32M
  - Default MIPI CSI sensor: GC2053 (1920×1080@20fps)
  - Default DVP sensor: GC2145 (1280×720@30fps)
- Software: `CONFIG_ISP`, `CONFIG_BK_CAMERA`, `CONFIG_FRAME_BUFFER`, `CONFIG_MEDIA_SERVICE`

.. warning::

    Use reference sensors and boards for evaluation. Different peripherals may require GPIO, I2C, resolution, and Kconfig changes. ISP **does not upscale** — sensor max resolution must be ≥ ISP output resolution.

## 2. Directory Layout

```text
isp_example/
├── ISP_TEST_CASES.md
├── ap/
│   ├── ap_main.c
│   └── src/
│       ├── isp_cli.c
│       ├── isp_func_test.c
│       ├── isp_api_test.c
│       ├── isp_tuning_test.c
│       ├── isp_dump_test.c
│       └── isp_mini_code.c
├── cp/
└── partitions/
```

## 3. Features

### 3.1 CLI Commands

| Command | Subcommands | Description |
|---------|-------------|-------------|
| **isp** | `detect` | Scan DVP / CSI sensors |
| | `open` | Open a channel (see syntax below) |
| | `close` | `isp close <mp\|sp>` |
| | `read` | `isp read <mp\|sp>` — read one frame, hex dump |
| | `sensor_open` | MIPI mini-code sensor init |
| | `init` / `soft_reset` | Controller init / soft reset |
| **isp_api** | many | Step-by-step Camera API (stub, returns `AVDK_ERR_UNSUPPORTED`) |
| **isp_tuning** | `start` / `stop` | ISP tuning server |
| **isp_dump** | `start` / `stop` | ISP dump server |
| | `sns_read` / `sns_write` | Sensor register I2C read/write |

`isp open` syntax:

```text
isp open <mipi|dvp> <mp|sp> <sensor_w> <sensor_h> <fps> <out_w> <out_h> <frame|software|hardware> [output_fmt]
```

- `frame`: full-frame output; `software` / `hardware`: Flexa segmented output
- Optional `output_fmt`, default `23` (NV12)

Examples:

```text
isp detect
isp open mipi mp 1920 1080 20 1920 1080 frame
isp open dvp mp 1280 720 30 1280 720 frame
isp close mp
```

Responses: `CMDRSP:OK` / `CMDRSP:ERROR`. Watch ISR stats: `MP:fps[...] | SP:fps[...]`.

### 3.2 Boot Behavior

**No auto demo** on boot — waits for serial CLI after init.

## 4. Build and Run

### 4.1 Build

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/isp_example -j$(nproc)
```

### 4.2 Test Cases

See `ISP_TEST_CASES.md` for the full matrix (MIPI/DVP, MP/SP, Frame/Flexa, stress, boundaries).

## 5. Notes

1. `isp_api` is an API stepping skeleton — not wired to real handlers yet.
2. Dual-channel use needs enough PSRAM / frame buffers.
3. For Flexa vs Frame, see [Frame vs Flexa Mode](../../../developer-guide/vpu/flexa_frame.html).
