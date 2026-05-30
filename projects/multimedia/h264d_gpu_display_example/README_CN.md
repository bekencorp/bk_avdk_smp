# H264解码GPU显示示例工程

* [English](./README.md)

## 1. 项目概述

本项目用于演示 Beken 平台上的 H264 解码、GPU 图像处理和 MIPI 屏显示流程。当前测试路径基于 `bk_h264_decode_*` 解码控制器、GPU blit/FLEXA 处理链路和 DPU/MIPI 显示后端。

当前工程提供：

- 内置 H264 流解码并通过 GPU 输出到显示帧
- 可选 MIPI 屏显示输出
- 可选 ISP 小窗叠加显示（PIP），通过 GPU blit 叠加到 H264 大图
- `CONFIG_BK_DECODER` 使能时的上电自动运行 case
- `.it.csv` 集成测试入口

### 1.1 测试环境

   * 硬件配置：
      * 核心板，**BK7259_QF128_12.3X12.3_V4.0**
      * PSRAM 32M
      * MIPI DSI 屏，默认配置为 `LCD_HX8399C_MIPI_1080x1920`
      * PIP 测试默认使用 CSI GC2053 sensor
   * 支持模块：
      * H264 硬件解码
      * VG-Lite GPU
      * DPU/MIPI 显示
      * 可选 ISP/CSI camera 小窗输入
   * 默认输入：
      * `720x1280` 内置 H264 码流
      * 可通过 Kconfig 切换为 `1280x720` 内置 H264 码流

.. warning::

    请使用参考外设，进行 demo 工程的熟悉和学习。如果屏、sensor 或外设规格不一样，代码和 Kconfig 可能需要重新配置。

## 2. 目录结构

项目采用 AP-CP 双核架构，主要源代码位于 AP 目录下。项目结构如下：

```
h264d_gpu_display_example/
├── .ci                         # CI 配置
├── .it.csv                     # 集成测试命令
├── CMakeLists.txt              # 项目级 CMake 构建文件
├── Makefile                    # Make 构建文件
├── README_CN.md                # 项目说明文档（中文）
├── ap/                         # AP 端代码
│   ├── CMakeLists.txt          # AP 端 CMake 构建文件
│   ├── Kconfig.projbuild       # 工程配置
│   ├── ap_main.c               # AP 主入口文件
│   ├── config/                 # AP 默认配置
│   ├── include/                # 头文件
│   └── src/                    # H264 解码、GPU 显示、DPU、ISP PIP 和 boot case
├── cp/                         # CP 端代码
│   ├── CMakeLists.txt          # CP 端 CMake 构建文件
│   ├── cp_main.c               # CP 主入口文件
│   └── config/                 # CP 默认配置
├── main/                       # 双核入口构建配置
├── partitions/                 # 分区和内存区域配置
└── pj_config.mk                # 项目配置
```

## 3. 功能说明

### 3.1 主要功能

- 支持内置 H264 码流解码显示
- 支持 `720x1280` 和 `1280x720` 两种测试码流配置
- 支持 GPU 对解码帧进行缩放、旋转和格式处理
- 支持将 GPU 输出帧送到 DPU/MIPI 屏显示
- 支持 ISP 小窗通过 GPU blit 叠加到 H264 大图
- 支持 CLI 指定循环次数，`0` 表示持续循环直到执行 `stop`
- 提供统一 `[RESULT][PASS]` / `[RESULT][FAIL]` 结果日志
- `CONFIG_BK_DECODER` 使能时，上电后自动运行一次默认 H264 解码显示 case

### 3.2 H264解码显示流程

1. 将内置 H264 码流复制到 coded frame buffer
2. 创建并打开 H264 解码控制器
3. 初始化 GPU 输出帧池和 GPU blit/FLEXA 处理链路
4. 可选初始化 DPU/MIPI 显示输出
5. 解析 H264 NALU 并送入解码器
6. 解码输出帧通过 GPU 处理为显示尺寸
7. 可选将 ISP 小窗叠加到 GPU 输出帧
8. 可选送 DPU/MIPI 刷屏
9. 达到指定循环次数或收到 `stop` 请求后释放资源并打印 `[RESULT]` 日志

### 3.3 ISP小窗叠加流程

开启 `CONFIG_H264D_GPU_DISPLAY_ENABLE_ISP_PIP` 后，可以通过 `isp_open` 启动 sensor、ISP channel 和小窗叠加任务。默认配置为：

- sensor 输入：`1280x720@20fps`
- ISP 小窗输出：`640x360` NV12
- PIP 旋转：默认 90 度
- PIP 显示位置：默认 `dst=(696,32)`，适配 1088x1920 竖屏

`isp_close` 会关闭 ISP channel、sensor 和 bus，并释放 PIP 相关资源。

## 4. 编译与运行

### 4.1 编译方法

使用以下命令编译项目：

```bash
make bk7259 PROJECT=multimedia/h264d_gpu_display_example
```

### 4.2 运行方法

编译完成后，将生成的固件烧录到开发板上。如果 `CONFIG_BK_DECODER` 使能，`main()` 会在上电后自动启动一次 H264 解码 GPU 显示 boot demo。也可以通过串口终端手动触发以下命令：

命令执行成功打印："CMDRSP:OK"

命令执行失败打印："CMDRSP:ERROR"

#### 4.2.1 当前CLI命令

```text
h264d_gpu_display help
h264d_gpu_display start [loops]
h264d_gpu_display stop
h264d_gpu_display isp_open
h264d_gpu_display isp_open <sensor_w> <sensor_h> <fps> <isp_w> <isp_h>
h264d_gpu_display isp_open <sensor_w> <sensor_h> <fps> <isp_w> <isp_h> <dst_x> <dst_y>
h264d_gpu_display isp_close
```

- `start [loops]`：启动 H264 解码 GPU 显示任务；`loops` 省略或为 `0` 时持续循环，非 0 时运行指定轮数后自动退出
- `stop`：请求当前解码显示任务在当前帧结束后退出
- `isp_open`：启动 ISP 小窗叠加功能，未传参时使用默认 sensor/ISP/PIP 配置
- `isp_close`：关闭 ISP 小窗叠加功能
- `CMDRSP:OK` 只表示命令被接受或测试线程创建成功，最终是否通过请看 `[RESULT]` 日志

## 5. 测试示例

### 5.1 解码显示一次

```text
h264d_gpu_display start 1
```

预期最终日志：

```text
[RESULT][PASS] decoded_frames=...
```

### 5.2 持续解码显示并停止

```text
h264d_gpu_display start
h264d_gpu_display stop
```

预期最终日志：

```text
[RESULT][PASS] decoded_frames=...
```

### 5.3 启动ISP小窗叠加

```text
h264d_gpu_display isp_open
h264d_gpu_display start
h264d_gpu_display isp_close
h264d_gpu_display stop
```

也可以指定 sensor、ISP 输出尺寸和小窗位置：

```text
h264d_gpu_display isp_open 1280 720 20 640 360 696 32
```

### 5.4 集成测试命令

`.it.csv` 包含：

```text
ap_cmd h264d_gpu_display start 2
ap_cmd h264d_gpu_display start 20
ap_cmd h264d_gpu_display stop
ap_cmd h264d_gpu_display isp_open
ap_cmd h264d_gpu_display start
ap_cmd h264d_gpu_display isp_close
ap_cmd h264d_gpu_display stop
```

期望结果匹配 `CMDRSP:OK`、`loop 1 start` 或 `[RESULT][PASS]` 日志。

## 6. 配置选项

### 6.1 显示与码流配置

- `CONFIG_H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY`：使能 MIPI 屏显示输出
- `CONFIG_H264D_GPU_DISPLAY_TEST_STREAM_720X1280`：使用 `720x1280` 内置 H264 码流
- `CONFIG_H264D_GPU_DISPLAY_TEST_STREAM_1280X720`：使用 `1280x720` 内置 H264 码流

默认显示输出尺寸由工程配置定义为：

- GPU 显示宽度：`1088`
- GPU 显示高度：`1920`
- `720x1280` 码流默认不旋转
- `1280x720` 码流默认旋转 90 度后适配竖屏显示

### 6.2 ISP PIP配置

- `CONFIG_H264D_GPU_DISPLAY_ENABLE_ISP_PIP`：使能 ISP 小窗叠加
- `CONFIG_H264D_GPU_DISPLAY_ENABLE_ISP_PIP_ROTATE`：使能 ISP 小窗旋转
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_ROTATE_90`：ISP 小窗旋转 90 度
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_ROTATE_270`：ISP 小窗旋转 270 度
- `CONFIG_H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_WIDTH`：默认 sensor 输入宽度
- `CONFIG_H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_HEIGHT`：默认 sensor 输入高度
- `CONFIG_H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_FPS`：默认 sensor 帧率
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_WIDTH`：默认 ISP 小窗输出宽度
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_HEIGHT`：默认 ISP 小窗输出高度
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_DST_X`：默认小窗 X 坐标
- `CONFIG_H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_DST_Y`：默认小窗 Y 坐标

## 7. 注意事项

1. `h264d_gpu_display start` 只负责创建测试线程，最终结果以 `[RESULT][PASS]` 或 `[RESULT][FAIL]` 为准。
2. 上电 boot demo 会自动运行一次默认码流，建议等待其结束后再手动触发 CLI 测试。
3. `start` 和 `isp_open` 使用 GPU、frame buffer、DPU、ISP 等共享资源，建议按需顺序执行并及时 `stop` / `isp_close`。
4. `isp_open` 默认参数适配 GC2053 和 1088x1920 竖屏；更换 sensor 或屏幕时，需要同步调整 Kconfig、GPIO 和显示坐标。
5. PIP 小窗输出为 NV12，宽高需要保持偶数；建议宽度按 16 像素对齐，便于 GPU/DMA 访问。
6. 回调和中断上下文中不建议执行阻塞操作，耗时处理应放到任务中完成。
7. 帧缓冲资源有限，测试异常退出后可以通过 `memshow` / `memleak` 检查资源是否释放完整。
