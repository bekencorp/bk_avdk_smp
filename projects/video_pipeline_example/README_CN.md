# Video Pipeline 示例项目

* [English](./README.md)

## 1. 项目概述

本项目是一个Video Pipeline示例模块，旨在测试Beken平台上的Video Pipeline功能。它提供了一个命令行接口（CLI），支持H.264编码、JPEG解码以及图像旋转（硬件旋转和软件旋转）。

* 有关Video Pipeline的详细信息，请参阅：

  - `Video Pipeline 开发指南 <../../../developer-guide/video_codec/video_pipeline.html>`_

* 有关Video Pipeline的API参考，请参阅：

  - `Video Pipeline API <../../../api-reference/multimedia/bk_video_pipeline.html>`_

### 1.1 测试环境

	* 硬件配置：
		* 核心板，**BK7258_QFN88_9X9_V3.2**
		* PSRAM 8M/16M
	* 支持，硬件/软件旋转
		* 0°，90°，180°，270°
	* 支持，MJPEG硬件解码
		* YUV422
	* 支持，MJPEG软件解码
		* YUV420
	* 支持，H264硬件编码

.. warning::
  请使用参考外设，进行demo工程的熟悉和学习。如果外设规格不一样，代码可能需要重新配置。

.. warning::

  YUV422的图像的H264编码出来的数据是旋转前的图像，而YUV420的图像的H264编码出来的数据是旋转后的图像。

  在硬解码时，先解码，解码后的数据同时给到旋转和H264编码，因此H264编码的数据是旋转前的数据；

  在软解码时，解码和旋转同时进行，解码后数据再进行H264编码，因此H264编码的数据是旋转后的数据；

## 2. 目录结构

项目采用AP-CP双核架构，主要源代码位于AP目录中。项目结构如下：

```
video_pipeline_example/
├── .ci                        # CI配置文件
├── .gitignore                 # Git忽略文件
├── CMakeLists.txt             # 项目级CMake构建文件
├── Makefile                   # Make构建文件
├── README.md                  # 英文项目文档
├── README CN.md               # 中文项目文档
├── ap/                        # 应用处理器代码
│   ├── CMakeLists.txt         # AP CMake构建文件
│   ├── Kconfig.projbuild      # AP Kconfig配置
│   ├── ap_main.c              # AP主入口文件
│   ├── config/                # AP配置文件
│   └── video_pipeline/        # Video pipeline实现
│       ├── data/              # 测试数据（JPEG图像）
│       ├── include/           # 头文件
│       └── src/               # 源代码文件
├── cp/                        # 协处理器代码
│   ├── CMakeLists.txt         # CP CMake构建文件
│   ├── config/                # CP配置文件
│   └── cp_main.c              # CP主入口文件
├── partitions/                # 内存分区配置
│   └── bk7258/                # BK7258芯片特定分区
└── pj_config.mk               # 项目配置Makefile
```

## 3. 功能说明

### 3.1 主要功能

- 支持H.264视频编码
- 支持JPEG图像解码
- 提供硬件和软件图像旋转功能
- 提供常规和异常场景测试功能
- 提供命令行接口（CLI）用于测试各种功能

### 3.2 Video Pipeline处理流程

Video Pipeline的典型使用流程如下：

1. 初始化Video Pipeline：`video_pipeline init`

2. 打开特定功能模块：

    - H.264编码器：`video_pipeline open_h264e`
    - 旋转模块：`video_pipeline open_rotate`

3. 通过相应的API执行操作

4. 关闭功能模块：
    - H.264编码器：`video_pipeline close_h264e`
    - 旋转模块：`video_pipeline close_rotate`

5. 反初始化Video Pipeline：`video_pipeline deinit`

## 4. 编译与执行

### 4.1 编译方法

使用以下命令编译项目：

```
make bk7258 PROJECT=video_pipeline_example
```

### 4.2 执行方法

编译成功后，将生成的固件烧录到开发板上，并通过串口终端使用以下命令测试Video Pipeline功能：

命令执行成功会打印："CMDRSP:OK"

命令执行失败会打印："CMDRSP:ERROR"

#### 4.2.1 基本Video Pipeline命令

1. 初始化Video Pipeline：

```
video_pipeline init
```

2. 反初始化Video Pipeline：

```
video_pipeline deinit
```

3. 打开H.264编码器：

```
video_pipeline open_h264e
```

4. 关闭H.264编码器：

```
video_pipeline close_h264e
```

5. 打开旋转模块：

```
video_pipeline open_rotate
```

6. 关闭旋转模块：

```
video_pipeline close_rotate
```

## 5. 常规测试命令

项目提供了各种常规场景测试功能，用于验证Video Pipeline在正常条件下的功能。以下是常规测试命令：

命令执行成功会打印："CMDRSP:OK"

命令执行失败会打印："CMDRSP:ERROR"

1. 硬件旋转测试：

```
video_pipeline_regular_test hardware_rotate
```

2. 软件旋转测试：

```
video_pipeline_regular_test software_rotate
```

3. H.264编码测试：

```
video_pipeline_regular_test h264_encode
```

4. H.264编码+硬件旋转组合测试：

```
video_pipeline_regular_test h264_encode_and_hw_rotate
```

5. H.264编码+软件旋转组合测试：

```
video_pipeline_regular_test h264_encode_and_sw_rotate
```

## 6. 测试数据

项目包含存储在 `ap/video_pipeline/data/` 目录中的不同格式的JPEG测试图像，包括：

* **jpeg_data_422_864_480.c** ：864x480分辨率的YUV422格式JPEG图像
* **jpeg_data_420_864_480.c** ：864x480分辨率的YUV420格式JPEG图像
* **jpeg_data_422_864_479.c** ：864x479分辨率的YUV422格式JPEG图像
* **jpeg_data_420_864_479.c** ：864x479分辨率的YUV420格式JPEG图像
* **jpeg_data_422_865_480.c** ：865x480分辨率的YUV422格式JPEG图像
* **jpeg_data_420_865_480.c** ：865x480分辨率的YUV420格式JPEG图像

## 7. 配置选项

### 7.1 Pipeline线程栈大小配置

Pipeline相关任务的线程栈大小可以通过Kconfig进行配置：

- **CONFIG_JPEG_DECODE_PIPELINE_TASK_STACK_SIZE**: JPEG解码pipeline任务的线程栈大小（字节）
  - 默认值：4096 字节
  - 配置路径：menuconfig -> Media -> JPEG decode pipeline task stack size

- **CONFIG_JPEG_GET_PIPELINE_TASK_STACK_SIZE**: JPEG获取pipeline任务的线程栈大小（字节）
  - 默认值：1024 字节
  - 配置路径：menuconfig -> Media -> JPEG get pipeline task stack size

- **CONFIG_YUV_ROTATE_PIPELINE_TASK_STACK_SIZE**: YUV旋转pipeline任务的线程栈大小（字节）
  - 默认值：2048 字节
  - 配置路径：menuconfig -> Media -> YUV rotate pipeline task stack size

- **CONFIG_H264_ENCODE_PIPELINE_TASK_STACK_SIZE**: H264编码pipeline任务的线程栈大小（字节）
  - 默认值：2048 字节
  - 配置路径：menuconfig -> Media -> H264 encode pipeline task stack size

说明：根据实际使用场景和内存资源调整栈大小，如遇到栈溢出可适当增大此值。

## 8. 注意事项

1. 使用前确保Video Pipeline已正确初始化
2. 操作完成后记得释放相关资源
3. 不同功能模块有特定的要求和限制，请参考API文档了解详情
4. 帧缓冲区资源有限，避免同时占用过多缓冲区
5. **回调函数使用注意事项**：
   - 回调函数中不建议执行阻塞操作（如长时间等待、sleep等），以避免影响解码性能和系统响应
   - 建议在回调函数中仅进行轻量级操作，如设置标志位、发送消息/信号量等，将耗时操作放到其他任务中执行