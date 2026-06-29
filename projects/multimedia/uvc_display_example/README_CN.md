# UVC 显示与编码流水线示例工程

* [English](./README.md)

## 1. 项目概述

本工程在 `uvc_example` 基础上扩展 **UVC 采集 → MJPEG 硬件解码 → GPU 合成 → MIPI 屏显示 → H264 硬件编码** 端到端流水线，演示多媒体模块协同与 Flexa 低延迟路径。

* 相关文档：

  - [UVC 摄像头](../../../developer-guide/camera/uvc.html)
  - [JPEG 解码（VPU）](../../../developer-guide/vpu/jpeg_decode.html)
  - [H.264 编码（VPU）](../../../developer-guide/vpu/h264_encode.html)
  - [Frame 与 Flexa 模式](../../../developer-guide/vpu/flexa_frame.html)

### 1.1 测试环境

- 硬件配置
  - 核心板：**BK7259_QF128_12.3X12.3_V4.0**
  - PSRAM：32M
  - **USB UVC 摄像头**（流水线固定 MJPEG）
  - **MIPI DSI 屏**：默认 `LCD_HX8399C_MIPI_1080x1920`
- 软件模块：USB UVC、`CONFIG_BK_VCDEC`（MJPEG 解码）、VG-Lite GPU、`CONFIG_BK_H264E`（H264 编码）、DPU 显示

.. warning::

    需同时连接 UVC 摄像头与参考 MIPI 屏。屏型号、分辨率或 UVC 格式不同时需修改 Kconfig 与源码参数。

## 2. 目录结构

```text
uvc_display_example/
├── ap/
│   ├── ap_main.c              # 入口 + CLI 注册
│   └── src/
│       ├── uvc_test.c         # UVC 采集
│       ├── decode_test.c      # MJPEG 硬件解码（Flexa / Frame）
│       ├── display_test.c     # MIPI LCD + DPU
│       ├── gpu_test_v2.c      # VG-Lite GPU
│       ├── pipeline_test.c    # 端到端流水线 + H264 编码
│       └── encode_frame_que.c # UVC 帧队列
├── cp/
└── partitions/
```

## 3. 功能说明

### 3.1 分模块 CLI

| 命令 | 子命令 | 说明 |
|------|--------|------|
| **uvc** | `open` / `close` | 同 `uvc_example`：`uvc open <port> <w> <h> <format>` |
| **decode** | `open` | 打开 MJPEG 解码，默认 **Flexa** 模式，1920×1080 |
| | `open frame` | 打开 MJPEG 解码 **Frame** 模式 |
| | `close` | 关闭解码器 |
| **display** | `open` / `close` | 打开 / 关闭 HX8399C MIPI 屏 + DPU + GPU |
| **pipeline** | `open` | 一键启动端到端流水线 |
| | `close` | 关闭流水线 |

`pipeline open` 可选参数：

```text
pipeline open [<port> <width> <height> <fps>]
```

缺省：`port=1`，1920×1080@30。

### 3.2 流水线步骤

`pipeline open` 按以下顺序启动：

1. MJPEG 硬件解码（Flexa，`decode_test_open`）
2. 显示 + GPU（`display_test_open_with_gpu`）
3. 获取解码 Flexa 环形缓冲，启动 H264 Flexa 编码线程
4. UVC 开流，MJPEG 帧送入共享队列（`uvc_camera_turn_on`）

### 3.3 启动行为

上电后**无自动 demo**，需手动执行 `pipeline open` 或分模块命令。

## 4. 编译与运行

### 4.1 编译

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/uvc_display_example -j$(nproc)
```

### 4.2 推荐调试顺序

**方式一：端到端**

```text
pipeline open 1 1920 1080 30
pipeline close
```

**方式二：分步**

```text
decode open
display open
uvc open 1 864 480 mjpeg
...
uvc close 1
display close
decode close
```

## 5. 注意事项

1. 相对 `uvc_example`，本工程**不提供** `uvc_api` 分步命令。
2. Pipeline 固定 MJPEG 输入；UVC 分辨率需与解码器配置协调。
3. H264 编码输出可通过日志观察首帧 I/P 帧信息（默认 GOP 30）。
4. Flexa 环缓、GPU Bond 与读指针同步详见 VPU Flexa 文档。
