# JPEG 编码示例工程

## 概述

基于 **`bk_jpeg_encode_*`**（VCENC JPEG，`bk_encoder`）的静态 JPEG 编码示例。

- 串口命令：`jpeg_encode frame`，运行 `bk_jpeg_encode_frame_test()`（256×128 NV12，10 帧，质量档 5）。
- 上电：若使能 `CONFIG_BK_ENCODER`，`main()` 会启动 `vcenc_jpeg_run_boot_demo()`（延时跑一次测试）。

成功日志示例：

```text
[RESULT][PASS] bk_jpeg_encode_frame_test success, frames=10/10 encoded_size=...
```

## 编译

```bash
make bk7259 PROJECT=multimedia/jpeg_encode_example
```

## 目录说明

- `ap/jpeg_encode/`：CLI、`bk_jpeg_encode_frame_test`、内置 `jpeg_encode_nv12_256x128` NV12 数据。

## 集成测试

见 `.it.csv`：`ap_cmd jpeg_encode frame`，期望包含 `[RESULT][PASS] bk_jpeg_encode_frame_test success`。
