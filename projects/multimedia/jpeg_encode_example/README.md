# JPEG Encoding Example Project

* [中文](./README_CN.md)

## Overview

Demonstrates **JPEG still-image encoding** on Beken using `bk_jpeg_encode_*` (VCENC JPEG path, `bk_encoder`).

- CLI: `jpeg_encode frame` — runs `bk_jpeg_encode_frame_test()` (256×128 NV12, 10 frames, quality 5).
- Boot: if `CONFIG_BK_ENCODER` is set, `main()` starts `vcenc_jpeg_run_boot_demo()` (delayed single test).

Expected pass log:

```text
[RESULT][PASS] bk_jpeg_encode_frame_test success, frames=10/10 encoded_size=...
```

## Build

```bash
make bk7259 PROJECT=multimedia/jpeg_encode_example
```

## Layout

```
ap/
├── ap_main.c
├── jpeg_encode/
│   ├── common/          # jpeg_encode_nv12_256x128 NV12 blob
│   ├── include/
│   └── src/             # CLI + bk_jpeg_encode_frame_test
└── CMakeLists.txt
```

## Integration test (`.it.csv`)

```text
ap_cmd jpeg_encode frame
```

Success string: `[RESULT][PASS] bk_jpeg_encode_frame_test success`
