# UVC 摄像头示例工程

* [English](./README.md)

## 1. 项目概述

本工程演示 BK7259 **USB Host + UVC** 外接摄像头采集，提供高层 `uvc` 一键开流命令与底层 `uvc_api` 分步 API 调试入口。

* 开发者指南：[UVC 摄像头](../../../developer-guide/camera/uvc.html)

* API 参考：

  - [Camera API](../../../api-reference/multimedia/bk_camera.html)
  - [图像内存管理](../../../api-reference/multimedia/bk_frame_buffer.html)

### 1.1 测试环境

- 硬件配置
  - 核心板：**BK7259_QF128_12.3X12.3_V4.0**
  - PSRAM：32M
  - **USB Host 口 + UVC 摄像头**（支持 MJPEG / YUV / H264 / H265，视设备能力）
- 默认示例参数：port `1`，864×480@30fps，MJPEG（`MEDIA_UVC_MJPEG_864X480_30FPS_CONFIG()`）

.. warning::

    请使用支持所需格式的 UVC 设备。分辨率与格式须与摄像头实际能力匹配，否则 `uvc_checkout_port_info` 会失败。

## 2. 目录结构

```text
uvc_example/
├── ap/
│   ├── ap_main.c
│   └── src/uvc_test.c       # uvc / uvc_api 全部逻辑
├── cp/
└── partitions/
```

## 3. 功能说明

### 3.1 `uvc` 高层命令

一键完成 USB 上电、控制器创建与开流：

```text
uvc open <port> <width> <height> <h264|h265|mjpeg|yuv>
uvc close <port>
```

参数不足时使用默认 864×480@30 MJPEG、port 1。内部流程：`uvc_camera_power_on` → 端口能力检查 → `bk_uvc_ctrl_new/init/open`。

### 3.2 `uvc_api` 分步命令

用于逐步验证 `bk_uvc_*` 控制器 API：

```text
uvc_api power_on
uvc_api power_off
uvc_api new
uvc_api delete
uvc_api init
uvc_api deinit
uvc_api open <width> <height> <h264|h265|mjpeg|yuv>
uvc_api close
uvc_api suspend
uvc_api resume
```

### 3.3 启动行为

上电后**无自动 demo**，注册 CLI 后等待串口命令。

## 4. 编译与运行

### 4.1 编译

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/uvc_example -j$(nproc)
```

### 4.2 使用示例

```text
uvc open 1 864 480 mjpeg
uvc close 1
```

或分步调试：

```text
uvc_api power_on
uvc_api new
uvc_api init
uvc_api open 864 480 mjpeg
uvc_api close
uvc_api deinit
uvc_api delete
uvc_api power_off
```

## 5. 注意事项

1. Hub 端口数受 `CONFIG_USBHOST_HUB_MAX_EHPORTS` 限制，与 `UVC_PORT_MAX` 一致。
2. 帧缓冲默认从 PSRAM 分配（`CONFIG_UVC_USE_PSRAM_ALLOC`）。
3. 端到端显示与 H264 编码流水线请参考 `uvc_display_example` 工程。
