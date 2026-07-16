# GPU 2.5D Tiger 示例工程

* [English](./README.md)

## 1. 项目概述

`gpu_2.5D_tiger` 是 BK7259 平台的 GPU 2.5D 图形显示示例。工程演示 MIPI DSI LCD、DPU、frame buffer 和 VG-Lite GPU 的完整显示链路：系统上电后初始化显示设备，在 LCD 上绘制矢量老虎图形，并循环执行缩放、旋转和平移动画。

本示例适用于学习和验证以下能力：

- MIPI DSI LCD panel 初始化和打开流程。
- DPU ARGB8888 图层刷新流程。
- VG-Lite GPU 矢量路径绘制接口。
- 双 frame buffer 轮转显示。
- DPU release 回调与渲染线程同步。
- GPU 渲染 buffer 的 slab heap 分配方式。

## 2. 硬件需求

- SoC/开发板：BK7259 系列开发板。
- 显示屏：MIPI DSI LCD，默认使用 `lcd_device_hx8399c_mipi_1080x1920`。
- 内存：板端 PSRAM 需正常工作，显示 buffer 和 GPU 连续内存依赖 PSRAM 资源。
- 调试接口：串口控制台，用于查看启动日志和运行状态。

默认引脚配置：

- LCD reset：`GPIO_60`。
- LCD backlight：`GPIO_7`。

如果实际硬件与默认屏幕或引脚不同，需要同步调整 LCD 驱动、panel timing、初始化序列和 GPIO 配置。

## 3. 目录结构

```text
gpu_2.5D_tiger/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c          # AP 主入口，初始化 media service、monitor 和 tiger demo
│   ├── draw_tiger.c       # LCD/DPU/GPU 初始化和动画渲染逻辑
│   ├── draw_tiger.h
│   ├── tiger_paths.h      # 矢量老虎路径和颜色数据
│   └── config/
├── cp/
│   ├── cp_main.c
│   └── config/
└── partitions/
```

## 4. 编译与烧录

在 SDK 根目录执行编译命令：

```bash
make bk7259 PROJECT=multimedia/gpu_2.5D_tiger -j
```

编译完成后，固件位于：

```text
build/bk7259/gpu_2.5D_tiger/package/all-app.bin
```

将 `all-app.bin` 烧录到开发板。烧录完成后复位开发板，并打开串口终端查看日志。

## 5. 运行现象

开发板上电后会自动运行本示例，无需输入 CLI 命令。启动过程包括：

1. 初始化系统和 media service。
2. 启动 AVDK monitor。
3. 初始化 MIPI DSI bus、LCD panel 和 DPU。
4. 初始化 VG-Lite GPU。
5. 创建 `gpu` 渲染线程。
6. 在两个 frame buffer 之间循环切换并刷新显示。

串口日志应包含类似输出：

```text
AP main running...
draw_tiger
read lcd id: 0x...
render_tiger_task
tiger animation round complete
```

LCD 期望显示紫色背景和矢量老虎图形。动画按以下顺序循环：

- 缩放：老虎图形放大和缩小。
- 旋转：老虎图形围绕中心旋转。
- 平移：老虎图形在屏幕范围内移动。

## 6. 关键配置

本示例依赖以下主要配置：

```text
CONFIG_BK_DISPLAY=y
CONFIG_MIPI_DSI=y
CONFIG_DPU_DRIVER=y
CONFIG_DSI_DRIVER=y
CONFIG_FRAME_BUFFER=y
CONFIG_VG_LITE_GPU=y
CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ=0x3A000
CONFIG_LCD_HX8399C_MIPI_1080x1920=y
CONFIG_MEDIA_SERVICE=y
```

如需适配其他 LCD，请重点检查：

- `ap/config/bk7259_ap/defconfig` 中启用的 LCD 驱动。
- `ap/draw_tiger.c` 中包含的 LCD 头文件。
- `ap/draw_tiger.c` 中使用的 `lcd_device_*` 设备描述。
- LCD reset 和 backlight GPIO。
- frame buffer 宽高、stride 和像素格式。

## 7. 注意事项

1. 本示例默认使用 `hx8399c_mipi_1080x1920` MIPI 屏。不同屏幕通常需要重新配置 timing 和初始化序列。
2. GPU 渲染依赖连续内存，若 frame buffer 或 VG-Lite 初始化失败，请优先检查 PSRAM 和 `CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ`。
3. 示例使用双 buffer 刷新，渲染线程会等待 DPU release 回调后再复用 buffer。
4. 本示例上电自动运行，不提供额外 CLI 测试命令。

## 8. 常见问题

### 屏幕无显示

请检查 LCD 供电、背光、MIPI DSI 排线、reset GPIO 和 backlight GPIO。若串口没有 `read lcd id` 或显示初始化相关日志，请优先检查 panel 创建和 DSI bus 初始化。

### 显示花屏或图像位置异常

请确认实际 LCD 型号、分辨率和代码中的 `lcd_device_hx8399c_mipi_1080x1920` 一致。若屏幕不同，需要适配 panel timing、初始化序列和 frame buffer 参数。

### GPU 初始化失败

请确认 `CONFIG_VG_LITE_GPU` 已开启，并检查连续内存大小和 PSRAM 初始化状态。
