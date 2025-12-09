# UVC双流示例工程

* [English](./README.md)

## 1. 工程概述

本工程是一个UVC双流示例工程，用于实现USB Video Class(UVC)设备的双流输出功能。该模块提供CLI测试命令，通过发送命令可以实现打开和关闭UVC设备，配置主流和子流参数，并获取实时UVC输出的图像数据。

* 有关UVC使用方法的详细说明，请参考如下链接：

  - [UVC使用方法](../../../developer-guide/camera/uvc.html)

* 有关UVC API和数据结构的详细说明，请参考如下链接：

  - [UVC API](../../../api-reference/multimedia/bk_camera.html)

### 1.1 测试环境

   * 硬件配置：
      * 核心板，**BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * 支持，USB2.0/UVC1.5，输出MJPEG/H264/H265

   * 已经支持的UVC设备，参考：[支持的UVC外设](../../../support_peripherals/index.html#uvc-camera)

.. note::

    请使用参考外设，进行demo工程的熟悉和学习。如果外设规格不一样，代码可能需要重新配置。

## 2. 目录结构

项目采用AP-CP双核架构，主要源代码位于AP目录下。项目结构如下：

```
uvc_dualstream_example
├── .ci                       # CI配置目录
├── .gitignore                # Git忽略文件配置
├── .it.csv                   # 测试用例配置文件
├── CMakeLists.txt            # CMake构建脚本
├── Makefile                  # Make构建脚本
├── README.md                 # 英文README
├── README_CN.md              # 中文README
├── ap/                       # AP核代码目录
│   ├── CMakeLists.txt        # AP CMake构建脚本
│   ├── ap_main.c             # AP主入口文件
│   ├── config/               # AP配置目录
│   │   └── bk7258_ap/        # BK7258 AP配置
│   └── uvc_test/             # UVC测试代码目录
│       ├── include/          # 头文件
│       │   ├── uvc_cli.h     # UVC CLI命令头文件
│       │   └── uvc_common.h  # 通用UVC定义和函数头文件
│       └── src/              # 源文件
│           ├── uvc_common.c  # 通用UVC实现
│           └── uvc_main.c    # 主要UVC测试实现
├── app.rst                   # 应用描述文件
├── cp/                       # CP核代码目录
│   ├── CMakeLists.txt        # CP CMake构建脚本
│   ├── config/               # CP配置目录
│   │   └── bk7258/           # BK7258 CP配置
│   └── cp_main.c             # CP主入口文件
├── it.yaml                   # 测试配置文件
├── partitions/               # 分区配置目录
│   └── bk7258/               # BK7258分区配置
│       ├── auto_partitions.csv  # 自动分区配置
│       └── ram_regions.csv      # RAM区域配置
└── pj_config.mk              # 项目配置文件
```

## 3.功能说明

### 3.1 主要功能

- 支持UVC上电操作
- 支持UVC端口枚举和查询
- 支持UVC设备打开和关闭
- 支持UVC视频格式配置和查询
- 支持UVC视频数据获取和解析
- 支持主流和子流双路同时输出
- 支持不同分辨率和格式的双流配置

### 3.2 双流功能详解

本项目实现了UVC双流技术，允许在同一USB连接上同时传输两种不同格式或分辨率的视频流，满足不同场景的需求：

#### 3.2.1 流类型定义
- **主流(Main Stream)**: 使用MJPEG格式，提供高分辨率、高质量的视频，适合需要详细图像信息的场景
- **子流(Sub Stream)**: 使用H264或H265格式，提供较低分辨率的视频，适合网络传输等场景

#### 3.2.2 支持的流组合方式
- 单独打开/关闭MJPEG主流
- 单独打开/关闭H264子流
- 单独打开/关闭H265子流
- 同时打开MJPEG主流和H264子流，可同时关闭或者单独关闭MJPEG主流或H264子流
- 同时打开MJPEG主流和H265子流，可同时关闭或者单独关闭MJPEG主流或H265子流

#### 3.2.3 双流实现机制
- 使用独立的流信息结构体分别管理主流和子流
- 通过统一的流操作接口(uvc_open_stream_helper/uvc_close_stream_helper)处理不同类型的流
- 支持单独关闭特定类型的流，提高灵活性

### 3.3 主要代码

- `uvc_test_info_t`: 测试信息结构体，包含解码缓冲区和相机句柄数组
- `uvc_common.c`: UVC上下电基本操作，端口信息解析检查
- `uvc_main.c`: CLI命令处理函数，支持API测试和整个功能测试
- `g_uvc_main_stream_info`: 主流信息结构体
- `g_uvc_sub_stream_info`: 子流信息结构体

### 3.4 CLI命令使用
```bash
uvc open_single port img_format width height   # 打开单流UVC设备
uvc open_dual port [options]                  # 打开双流UVC设备
uvc close port [stream_type]                  # 关闭UVC设备
```

#### 3.4.1 open_single命令参数说明

- `port`: UVC端口号，默认是1，范围 [1-4]
- `width`: 图像宽度，默认是864
- `height`: 图像高度，默认是480
- `img_format`: 图像格式，默认是MJPEG，支持MJPEG、H264、H265

#### 3.4.2 open_dual命令参数说明

- `port`: UVC端口号，默认是1，范围 [1-4]
- `options`: 流配置的可选参数

  支持的参数组合示例：
  ```
  uvc open_dual 1 MJPEG 864 480                     # 只打开MJPEG主流
  uvc open_dual 1 H264 1280 720                     # 只打开H264子流
  uvc open_dual 1 H265 1280 720                     # 只打开H265子流
  uvc open_dual 1 MJPEG 864 480 H264 1280 720       # 同时打开MJPEG主流和H264子流
  uvc open_dual 1 MJPEG 864 480 H265 1280 720       # 同时打开MJPEG主流和H265子流
  uvc close 1 all                                   # 关闭UVC端口1所有流
  uvc close 1 H26X                                  # 关闭UVC端口1 H264/H265子流
  uvc close 1 MJPEG                                 # 关闭UVC端口1 MJPEG主流
  ```

#### 3.4.3 close命令参数说明

- `port`: UVC端口号
- `stream_type`: 可选，指定关闭特定流，可以是'H26X'或'MJPEG'

### 3.5 用户参考文件
- `projects/uvc_dualstream_example/ap/uvc_test/src/uvc_main.c` : UVC测试主函数，包含双流处理逻辑
- `projects/uvc_dualstream_example/ap/uvc_test/src/uvc_common.c` : UVC电源管理和端口枚举信息检查功能

## 4. 编译和运行

### 4.1 编译方法

使用以下命令编译项目：

```
make bk7258 PROJECT=uvc_dualstream_example
```

### 4.2 运行方法

编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试UVC功能：

命令执行成功打印："CMDRSP:OK"

命令执行失败打印："CMDRSP:ERROR"

### 4.3 配置参数

- `BK_UVC_864X480_30FPS_MJPEG_CONFIG`: 默认MJPEG分辨率配置

- `BK_UVC_1920X1080_30FPS_H26X_CONFIG`: 默认H264/H265分辨率配置

### 4.4 测试示例

测试命令需要添加前缀："ap_cmd"，如 `ap_cmd uvc open_single 1 MJPEG 864 480`.

#### 4.4.1 UVC打开864X480 MJPEG单流测试
- CASE命令:

```bash
uvc open_single 1 MJPEG 864 480
```

- CASE预期结果：

```
成功
```
- CASE成功标准：

```
CMDRSP:OK
```
- CASE成功日志：

```
uvc_id1:30[463 23KB], uvc_id2:0[0 0KB], uvc_id3:0[0 0KB], uvc_id4:0[0 0KB], packets[all:127896, err:8]
如果是jpeg格式，打印mjpeg格式的帧信息，seq会逐渐增大，当达到最大值又变为0，length会动态变化，一般大于10KB至少
uvc_frame_complete: seq:0, length:8303, format:mjpeg, h264_type:0 打印mjpeg格式的帧信息
```
- CASE失败标准：

```
CMDRSP:ERROR
```
- CASE失败日志：

```
没有帧信息打印
```
#### 4.4.2 UVC打开864X480 MJPEG 1280X720 H264双流测试
- CASE命令:

```bash
uvc open_dual 1 MJPEG 864 480 H264 1280 720
```

- CASE预期结果：

```
成功
```
- CASE成功标准：

```
CMDRSP:OK
```
- CASE成功日志：

```
uvc_id1:19[77 23KB], uvc_id2:19[77 4KB], uvc_id3:0[0 0KB], uvc_id4:0[0 0KB], packets[all:4432, err:0]

uvc_frame_complete: port:1, seq:150, length:23676, format:mjpeg, h264_type:0, result:0
uvc_frame_complete: port:1, seq:150, length:36084, format:h264, h264_type:0, result:0
```
- CASE失败标准：

```
CMDRSP:ERROR
```
- CASE失败日志：

```
没有帧信息打印
```

#### 4.4.3 UVC关闭测试
- CASE命令:

```bash
uvc close 1 all
```

- CASE预期结果：

```
成功
```
- CASE成功标准：

```
CMDRSP:OK
```

- CASE失败标准：

```
CMDRSP:ERROR
```

### 4.5 注意事项

1. 端口号必须在1-4范围内
2. 分辨率需与硬件支持匹配
3. 格式类型需与设备能力匹配
4. **双码流的功能是建立在单流功能正常的基础上的，若测试双码流有异常，可先测试单流功能是否正常**

### 4.6 正常打印日志

- CMDRSP:OK 打印命令执行成功
- CMDRSP:ERROR 打印命令执行失败
- uvc_id1:19[77 23KB], uvc_id2:19[77 4KB], uvc_id3:0[0 0KB], uvc_id4:0[0 0KB], packets[all:4432, err:0] ，其中uvc_id1代表第一条开启的流的帧率，uvc_id2代表第二条流开启的帧率
- uvc_frame_complete: port:1, seq:150, length:23676, format:mjpeg, h264_type:0, result:0 打印mjpeg格式的帧信
- uvc_frame_complete: port:1, seq:150, length:36084, format:h264, h264_type:0, result:0 打印h264/h265格式的帧信
- 在双流模式下，主流(MJPEG)和子流(H264/H265)会分别显示各自的帧率和图像信息，同时输出不同格式的帧信息