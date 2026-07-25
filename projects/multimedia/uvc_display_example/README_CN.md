# UVC 显示流水线示例工程

* [English](./README.md)

## 1. 项目概述

本工程在 `uvc_example` 基础上，演示 **UVC 摄像头 MJPEG 采集 → 硬件解码 → GPU 合成 → MIPI 屏显示** 的端到端显示链路。

与 `uvc_example` 的差异：

| 对比项 | `uvc_example` | `uvc_display_example` |
|--------|---------------|------------------------|
| 主要目标 | UVC 开流与 API 调试 | UVC 画面在 LCD 上显示 |
| CLI | `uvc` + `uvc_api` 分步 API | `uvc` / `decode` / `display` / `pipeline` |
| 解码 / 显示 | 无 | `bk_decoder` Flexa + VG-Lite GPU + DPU |
| 推荐命令 | `uvc open ...` | `pipeline open ...` |

数据流（`pipeline open`）：

```text
UVC(MJPEG) → encode_frame_que → MJPEG Flexa 解码(bk_decoder)
    → bk_flexa_mjpegd_gpu_bond → VG-Lite GPU → DPU → MIPI LCD
```

活跃工程**不使用** legacy VPU API，也**不包含** H264 编码路径。历史实现见 `verisilicon_nano/legacy/README.md`。

* 相关文档：

  - [UVC 摄像头](../../../developer-guide/camera/uvc.html)
  - [JPEG 解码（VPU）](../../../developer-guide/vpu/jpeg_decode.html)
  - [Frame 与 Flexa 模式](../../../developer-guide/vpu/flexa_frame.html)

### 1.1 测试环境

- 硬件配置
  - 核心板：**BK7259_QF128_12.3X12.3_V4.0**
  - PSRAM：32M
  - **USB UVC 摄像头**（需支持 MJPEG，分辨率建议与解码参数一致）
  - **MIPI DSI 屏**：默认 `LCD_HX8399C_MIPI_1080x1920`
- 关键 Kconfig：`CONFIG_USB`、`CONFIG_BK_VCDEC`、`CONFIG_BK_DECODER`、`CONFIG_VG_LITE_GPU`、`CONFIG_BK_DISPLAY`、`CONFIG_DPU_DRIVER`

.. warning::

    需同时连接 UVC 摄像头与参考 MIPI 屏。摄像头不支持目标分辨率/帧率时，`uvc_checkout_port_info` 会失败；请按设备能力调整 `pipeline open` 参数。

## 2. 目录结构

```text
uvc_display_example/
├── ap/
│   ├── ap_main.c
│   ├── include/
│   │   └── decode_test.h
│   └── src/
│       ├── uvc_test.c         # UVC 采集，帧送入 encode_frame_que
│       ├── decode_test.c      # MJPEG Flexa 解码（bk_jpeg_decode_flexa_*）
│       ├── display_test.c     # MIPI LCD + DPU + GPU
│       ├── pipeline_test.c    # 一键串联解码、显示、Bond、UVC
│       └── encode_frame_que.c # UVC 与解码器之间的帧队列
├── cp/
└── partitions/
```

## 3. 功能说明

### 3.1 CLI 命令

| 命令 | 用法 | 说明 |
|------|------|------|
| **uvc** | `uvc open <port> <w> <h> [mjpeg or yuv or h264 or h265]` | 开流；省略 format 时默认 `mjpeg` |
| | `uvc close <port>` | 关流 |
| **decode** | `decode open` / `close` | 打开/关闭 MJPEG Flexa 解码（固定 1920×1080） |
| **display** | `display open` / `close` | 打开/关闭 LCD；仅当 decode 已打开时启用 GPU 视频路径 |
| **pipeline** | `pipeline open [port w h fps]` | **推荐**：一键完成解码、显示、Bond、UVC |
| | `pipeline close` | 关闭整条流水线 |

`pipeline open` 缺省参数：`port=1`，`1920×1080@30`。

### 3.2 `pipeline open` 启动顺序

1. `decode_test_open`：创建 MJPEG Flexa 解码器与解码线程
2. `display_test_open_with_gpu_flexa`：以解码 Flexa 环缓为 GPU 输入打开显示
3. `bk_flexa_mjpegd_gpu_bond_start`：建立解码器与 GPU 的 Flexa 读指针同步
4. `uvc_camera_turn_on`：UVC 开流，MJPEG 帧经 `encode_frame_que` 送给解码线程

### 3.3 启动行为

上电后**无自动 demo**，需手动执行 CLI。

## 4. 编译与运行

### 4.1 编译

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/uvc_display_example -j$(nproc)
```

### 4.2 运行示例

**端到端显示（推荐）：**

```text
pipeline open 1 1920 1080 30
pipeline close
```

**分步调试（仅验证各模块，完整出图仍需 `pipeline open`）：**

```text
decode open
display open          # 未 open decode 时仅点亮 LCD
uvc open 1 1920 1080 mjpeg
uvc close 1
display close
decode close
```

## 5. 注意事项

1. 本工程**不提供** `uvc_api` 分步命令（见 `uvc_example`）。
2. UVC 分辨率、`pipeline open` 参数与 `decode open` 的 1920×1080 需保持一致，否则解码可能失败。
3. `display open` 单独执行且 decode 未打开时，日志会提示 `LCD only`，不会出现 UVC 画面。
4. Flexa 环缓、MJPEGD-GPU Bond 机制详见 VPU Flexa 开发者文档。
