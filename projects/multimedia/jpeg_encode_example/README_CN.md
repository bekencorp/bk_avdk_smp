# JPEG 编码示例工程

* [English](./README.md)

## 1. 项目概述

本工程演示 Beken 平台 **JPEG 硬件编码**，基于 `bk_jpeg_encode_*` 控制器（VCENC JPEG，`bk_encoder`），覆盖 Frame 整帧模式与软件 Flexa 低延迟模式。

* 开发者指南：

  - [JPEG 编码（VPU）](../../../developer-guide/vpu/jpeg_encode.html)
  - [Frame 与 Flexa 模式](../../../developer-guide/vpu/flexa_frame.html)

* API 参考：[JPEG 编码器 API](../../../api-reference/multimedia/bk_jpeg_encode.html)（如有）

当前工程提供：

- Frame 模式：`jpeg_encode frame`
- 软件 Flexa 模式：`jpeg_encode flexa`
- `CONFIG_BK_ENCODER` 使能时的上电自动编码自检
- `.it.csv` 集成测试入口

### 1.1 测试环境

- 硬件：**BK7259_QF128_12.3X12.3_V4.0**，PSRAM 32M
- 输入：内置 256×128 NV12 小图，测试时平铺扩展为 **1920×1080** NV12 缓冲
- 输出：JPEG 码流；每轮 **10 帧**，质量档 **5**

.. warning::

    请使用参考板卡进行学习。分辨率或内存配置不同时可能需要调整 `defconfig` 与测试参数。

## 2. 目录结构

```text
jpeg_encode_example/
├── .it.csv
├── ap/
│   ├── ap_main.c
│   └── jpeg_encode/
│       ├── common/jpeg_encode_nv12_256x128.*   # 内置测试 NV12
│       ├── include/jpeg_encode_test.h
│       └── src/
│           ├── jpeg_encode_cli.c
│           └── bk_jpeg_encode_test.c
├── cp/
└── partitions/
```

## 3. 功能说明

### 3.1 CLI 命令

```text
jpeg_encode help
jpeg_encode frame          # Frame 模式，bk_jpeg_encode_frame_new
jpeg_encode flexa          # 软件 Flexa，bk_jpeg_encode_sw_flexa_new
```

- `CMDRSP:OK` 仅表示测试线程创建成功，最终结果看 `[RESULT]` 日志
- Flexa 模式按 16 行 block 推进输入，在 `flexa_done` 回调中通过 IOCTL 通知下一批行就绪

### 3.2 上电自检

使能 `CONFIG_BK_ENCODER` 时，`main()` 启动 `vcenc_jpeg_run_boot_demo()`，延时后依次运行 Frame 与 Flexa 自检。

## 4. 编译与运行

### 4.1 编译

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/jpeg_encode_example -j$(nproc)
```

### 4.2 预期结果

成功示例：

```text
[RESULT][PASS] bk_jpeg_encode_frame_test success, frames=10/10 encoded_size=...
[RESULT][PASS] bk_jpeg_encode_sw_flexa_test success, frames=10/10 encoded_size=...
```

失败示例：

```text
[RESULT][FAIL] bk_jpeg_encode_frame_test failed at encode_frame ret=... frames=...
```

### 4.3 集成测试

`.it.csv`：

```text
ap_cmd jpeg_encode frame
```

期望匹配 `[RESULT][PASS] bk_jpeg_encode_frame_test success`。

## 5. 配置与注意事项

| 参数 | 值 |
|------|-----|
| 编码分辨率 | 1920×1080 |
| 源图尺寸 | 256×128（平铺填充） |
| 帧数 / 质量 | 10 帧 / 质量档 5 |
| 输入缓冲 | `MEM_SLAB_HEAP_UNCODED` |
| 输出缓冲 | `MEM_SLAB_HEAP_CODED` |

1. 上电 demo 与手动 CLI 共用编码硬件，建议等待上电自检完成后再手动测试。
2. 回调中避免阻塞操作，仅做轻量通知（信号量/标志位）。
3. 硬件 Flexa 路径（`bk_jpeg_encode_hw_flexa_new`）本工程未覆盖，可与 ISP 采集链路配合参考 VPU 文档。
