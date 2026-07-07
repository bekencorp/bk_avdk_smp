# DVP Example

* [中文](./README_CN.md)

## Overview

This project splits the DVP camera tests out of `multimedia/isp_example`. It focuses on BK7259 DVP sensor detection, MP/SP channel open, single-frame read, and frame callback validation.

Default setup:

- Board: BK7259_QF128_12.3X12.3_V4.0
- PSRAM: 32M
- DVP sensor: GC2145, default 1280x720@30fps

Required options include `CONFIG_ISP`, `CONFIG_BK_CAMERA`, `CONFIG_FRAME_BUFFER`, `CONFIG_DVP_CAMERA`, `CONFIG_DVP_GC2145`, and `CONFIG_MEDIA_SERVICE`. The current SDK GC2145 DVP raw path still links against MIPI/CSI helper symbols, so the defconfig keeps `CONFIG_MIPI_CSI`, `CONFIG_CSI_CAMERA`, and `CONFIG_CSI_GC2053` as low-level link dependencies while the CLI remains DVP-only.

## CLI

```text
dvp detect
dvp open <mp|sp> <sensor_w> <sensor_h> <fps> <out_w> <out_h> <frame|flexa> [output_fmt]
dvp read <mp|sp>
dvp cb <on|off> [mp|sp]
dvp close <mp|sp>
```

Example:

```text
dvp detect
dvp open mp 1280 720 30 1280 720 frame
dvp cb on mp
dvp cb off
dvp close mp
```

`dvp cb on` registers the demo callback and starts a background capture task. Continuous `dvp_cb frame[...]` logs indicate that frames are delivered to the application callback. The frame buffer is released after the callback returns, so production code should consume or copy it quickly.

## Build

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/dvp_example -j$(nproc)
```

See `DVP_TEST_CASES.md` for the test matrix.
