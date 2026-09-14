# ISP Display Example (TP2863 dual-VC preview)

* [中文](./README_CN.md)

## Overview

TP2863 dual AHD input, ISP MP (frame mode), VC mux, GPU rotate/scale, MIPI LCD 1080×1920:

```
TP2863 ch1/ch2 -> MIPI VC0/VC1 -> ISP MP (frame) -> vc_mux -> GPU -> LCD
```

Display path matches **doorbell_lp**: frame-mode MP + vc_mux peek + frame GPU (not flexa bond).

## Startup order

### 1. Firmware init order (same as doorbell)

TP2863 outputs `YUYV_SWAP`; MIPI/ISP timing matters. Required sequence:

1. Camera LDO on
2. I2C bus enable, sensor detect
3. Fill `input_pixel_fmt` from sensor format table (`YUYV_SWAP` for TP2863)
4. **`bk_camera_sensor_init` + `set_format`** (TP2863 / MIPI CSI config)
5. Then ISP `port_init` → MP `channel_open`
6. Display: vc_mux + frame GPU + LCD

Do **not** call `set_format` for the first time after ISP channel is already open.

#### Configuring “swap” at sensor open

TP2863 has **no** separate sensor byte-swap register. Color-related swap is driven by **ISP input pixel format**:

| Layer | Field | TP2863 value | Effect |
|-------|--------|--------------|--------|
| Sensor format table | `output_pixel_fmt` | `BK_PIXEL_FORMAT_YUYV_SWAP` | Declares MIPI byte order |
| ISP port | `input_pixel_fmt` | Copied from format table | Applied in `port_init` |
| ISP MI (MP → NV12) | `data_swap` | Auto `3` | Set in `vsi_isp_miv10.c` when port is `YUYV_SWAP` and channel outputs NV12 |

So “configure swap when opening sensor” means: before `port_init`, query matched (w,h,fps) and copy `output_pixel_fmt` into `input_pixel_fmt`. This project does that before sensor init.

You **cannot** fix wrong colors by adding swap after ISP is already running: `set_format` resets MIPI while ISP is active. Swap bits are fixed at `port_init`; changing them later requires closing the channel.

### 2. Recommended CLI sequence

#### Dual VC (recommended)

`open_vc_route` sets up camera + display only; you must **enable** each VC input explicitly:

```text
isp detect
isp open_vc_route 1280 720 25 1280 720 0
isp vc_route_enable 0
isp vc_route_vc 0
isp vc_route_enable 1
isp vc_route_vc 1
isp close_vc_route
```

| Step | Command | Notes |
|------|---------|--------|
| 1 | `isp open_vc_route [sw] [sh] [fps] [iw] [ih] [vc]` | Open MP(frame) + mux + GPU + LCD |
| 2 | `isp vc_route_enable <0\|1> [discard]` | **Required** — start MIPI input for that VC |
| 3 | `isp vc_route_vc <0\|1>` | Select which VC is shown on LCD |
| 4 | `isp close_vc_route` | Tear down before switching to single-MP mode |

#### Single MP preview

Mutually exclusive with dual VC — run `close_vc_route` first if needed:

```text
isp open mp 1280 720 25 1280 720 frame
isp close mp
```

### 3. Mode exclusivity

| Mode | Open | Close |
|------|------|-------|
| Single MP + LCD | `isp open mp ... frame` | `isp close mp` |
| Dual VC + LCD | `isp open_vc_route ...` | `isp close_vc_route` |

## Build

```bash
cd <SDK_ROOT>
CCACHE_DISABLE=1 make bk7259 PROJECT=multimedia/isp_display_example -j$(nproc)
```

## Board / Kconfig

- Pins (doorbell_lp): I2C1 GPIO69/70, reset GPIO71, `pin_xclk=INVALID`
- Kconfig: `CONFIG_CSI_TP2863`, GPU, DPU, hx8399c 1080×1920

## Auto test

See `.it.csv` in this project directory. Configure `armino_ai_coding.env` at workspace root (remote BKFIL + serial port), then:

```bash
bash .cursor/skills/multimedia-tm/auto-test/scripts/run_auto_test.sh \
  --project multimedia/isp_display_example \
  --port COM18 \
  --sdk-dir bk_avdk_smp_release_4.0.1
```

Example remote host: `BKFIL_REMOTE_HOST=192.168.31.35`, `BKFIL_PORT=COM18`.
