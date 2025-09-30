# UVC示例工程

* [English](./README.md)

## 1. 工程概述

本工程是一个UVC示例工程，用于实现USB Video Class(UVC)设备功能。该模块提供CLI测试命令，通过发送命令可以实现打开和关闭UVC，获取实时UVC输出的图像。

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
uvc_example
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

### 3.2 主要代码

- `uvc_test_info_t`: 测试信息结构体，包含解码缓冲区和相机句柄数组
- `uvc_common.c`: UVC上下电基本操作，端口信息解析检查
- `uvc_main.c`: CLI命令处理函数，支持api测试和整个功能测试

### 3.3 CLI命令使用
```bash
uvc open [port] [ppi] [type]  # 打开UVC设备
uvc close [port]              # 关闭UVC设备
```

参数说明：

- `port`: 端口号(1-4)
- `ppi`: 分辨率(如640X480/1280X720)
- `type`: 视频格式(jpeg/h264/h265/yuv/dual)

### 3.4 用户参考文件
- `projects/uvc_example/ap/uvc_test/src/uvc_main.c` : UVC测试主函数
- `projects/uvc_example/ap/uvc_test/src/uvc_common.c` : UVC电源管理和端口枚举信息检查功能

## 4. 编译和运行

### 4.1 编译方法

使用以下命令编译项目：

```
make bk7258 PROJECT=uvc_example
```

### 4.2 运行方法

编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试UVC功能：

命令执行成功打印："CMDRSP:OK"

命令执行失败打印："CMDRSP:ERROR"

### 4.3 配置参数

- `BK_UVC_864X480_30FPS_MJPEG_CONFIG`: 默认分辨率配置

### 4.4 测试示例

测试命令需要添加前缀：“ap_cmd”，如 `ap_cmd uvc open 1`.

#### 4.4.1 UVC打开864X480 MJPEG测试
- CASE命令:

```bash
uvc open 1 864 480
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
不论何种格式，uvc会打印如下log：uvc_idx中x表示port号，默认最大支持3，紧接的数值为出图帧率，一般大于等于10；
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

#### 4.4.2 UVC关闭测试
- CASE命令:

```bash
uvc close 1
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

#### 4.4.3 UVC打开port 2, 1280X720 MJPEG测试

.. note::
        当测试其他port时，USB应该接上HUB，且保证port2上接有UVC设备

- CASE命令:

```bash
uvc open 2 1280 720
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
不论何种格式，uvc会打印如下log：uvc_idx中x表示port号，默认最大支持3，紧接的数值为出图帧率，一般大于等于10；
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

#### 4.4.4 UVC打开不支持的分辨率测试
- CASE命令:

```bash
uvc open 1 1024 600
```

- CASE预期结果：

```
失败
```
- CASE失败标准：

```
CMDRSP:ERROR
```
- CASE失败日志：

```
uvc_camera_stream_check_config, not support this resolution:1024X600
uvc_camera_stream_rx_config, not support this solution, please retry...
```

#### 4.4.5 UVC打开不支持的图像格式测试
- CASE命令:

```bash
uvc open 1 864 480 h264
```

- CASE预期结果：

```
失败
```
- CASE失败标准：

```
CMDRSP:ERROR
```
- CASE失败日志：

```
uvc_camera_stream_check_config, please check usb output format:xxx
```

### 4.5 注意事项

1. 端口号必须在1-4范围内
2. 分辨率需与硬件支持匹配
3. 格式类型需与设备能力匹配

#### 4.6 正常打印日志

- CMDRSP:OK 打印命令执行成功
- CMDRSP:ERROR 打印命令执行失败
- uvc_id1:30[463 23KB], uvc_id2:0[0 0KB], uvc_id3:0[0 0KB], uvc_id4:0[0 0KB], packets[all:127896, err:8] 打印各个端口的出图帧率和图像大小
- uvc_frame_complete: seq:0, length:8303, format:mjpeg, h264_type:0 打印mjpeg格式的帧信息