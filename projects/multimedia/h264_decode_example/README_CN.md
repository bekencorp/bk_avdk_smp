# H264解码示例工程

* [English](./README.md)

## 1. 项目概述

本项目是一个H264解码测试模块，用于测试Beken平台上的H264解码功能。该模块提供了命令行接口(CLI)，支持H264解码。

* 有关H264解码的详细信息，请参阅：

  - [H264软件解码概述](../../../developer-guide/video_codec/h264_decoding_sw.html)

### 1.1 测试环境

   * 硬件配置：
      * 核心板，**BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * 支持H264解码
      * 输入：H264码流（`bk_test_h264d.c` 内置 demo 码流）
      * 输出：解码帧回调与基础信息日志

.. warning::

    请使用参考外设，进行demo工程的熟悉和学习。如果外设规格不一样，代码可能需要重新配置。

## 2. 目录结构

项目采用AP-CP双核架构，主要源代码位于AP目录下。项目结构如下：

```
h264_decode_example/
├── .ci                   # CI配置目录
├── .gitignore            # Git忽略文件
├── CMakeLists.txt        # 项目级CMake构建文件
├── Makefile              # Make构建文件
├── README.md             # 项目说明文档（英文）
├── README_CN.md          # 项目说明文档（中文）
├── ap/                   # AP端代码
│   ├── CMakeLists.txt    # AP端CMake构建文件
│   ├── Kconfig.projbuild # Kconfig配置
│   ├── ap_main.c         # AP主入口文件
│   ├── config/           # AP配置目录
│   └── h264_decode/      # H264解码实现
│       ├── include/      # 头文件
│       └── src/          # 源代码文件
├── cp/                   # CP端代码
│   ├── CMakeLists.txt    # CP端CMake构建文件
│   ├── cp_main.c         # CP主入口文件
│   └── config/           # CP配置目录
├── it.yaml               # 集成测试配置
├── partitions/           # 分区配置
└── pj_config.mk          # 项目配置
```

## 3. 功能说明

### 3.1 主要功能

- 支持H264解码
- 提供命令行接口进行解码测试

## 4. 编译与运行

### 4.1 编译方法

使用以下命令编译项目：

```
make bk7259 PROJECT=multimedia/h264_decode_example
```

### 4.2 运行方法

编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试H264解码功能：

命令执行成功打印："CMDRSP:OK"

命令执行失败打印："CMDRSP:ERROR"

#### 4.2.1 基础解码命令

```
h264_decode h264d
h264_decode jpegd
```

## 7. 注意事项

1. 运行前请确保已使能 `CONFIG_BK_H264D`。
2. 运行前请确保已使能 `CONFIG_BK_H264D_DEMO`。
3. `h264_decode h264d` 调用 `bk_test_h264d.c` 中的 `h264_decoder_test()`。
4. CLI 命令仅负责创建任务；解码在该任务中异步执行。
