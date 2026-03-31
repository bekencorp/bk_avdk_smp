# H264编码示例工程

* [English](./README.md)

## 1. 项目概述

本项目是一个H264编码测试模块，用于测试Beken平台上的H264编码功能。该模块提供了命令行接口(CLI)，支持H264硬件编码。

* 有关H264编码的详细信息，请参阅：

  - [H264编码概述](../../../developer-guide/video_codec/h264_encoding.html)

* 有关API参考，请参阅：

  - [H264编码器API](../../../api-reference/multimedia/bk_h264_encode.html)

### 1.1 测试环境

   * 硬件配置：
      * 核心板，**BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * 支持H264硬件编码
      * YUV420、YUV422格式
      * 输入：来自帧缓冲区的YUV帧
      * 输出：H264编码流

.. warning::

    请使用参考外设，进行demo工程的熟悉和学习。如果外设规格不一样，代码可能需要重新配置。

## 2. 目录结构

项目采用AP-CP双核架构，主要源代码位于AP目录下。项目结构如下：

```
h264_encode_example/
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
│   └── h264_encode/      # H264编码实现
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

- 支持H264硬件编码
- 提供命令行接口进行编码测试
- 实现了帧缓冲管理机制
- 提供了常规场景和异常场景的编码测试功能
- 支持异步编码

### 3.2 H264编码流程

1. 初始化H264编码器
2. 打开编码器
3. 执行编码操作：
   - 从帧缓冲区获取输入YUV帧
   - 请求输出缓冲区用于编码数据
   - 执行编码（编码是异步的，结果在回调中返回）
   - 在回调中释放缓冲区
4. 关闭编码器
5. 删除编码器实例

## 4. 编译与运行

### 4.1 编译方法

使用以下命令编译项目：

```
make bk7258 PROJECT=h264_encode_example
```

### 4.2 运行方法

编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试H264编码功能：

命令执行成功打印："CMDRSP:OK"

命令执行失败打印："CMDRSP:ERROR"

#### 4.2.1 基础编码命令

1. 初始化H264编码器：
```
h264_encode init
```

2. 打开编码器：
```
h264_encode open
```

3. 执行编码操作：
```
h264_encode encode
```

4. 强制IDR帧：
```
h264_encode force_idr
```

5. 关闭编码器：
```
h264_encode close
```

6. 删除编码器实例：
```
h264_encode delete
```

#### 4.2.2 常规测试命令

1. 正常编码测试：
```
h264_encode_regular_test normal_test
```

2. 异步编码测试：
```
h264_encode_regular_test async_test
```

#### 4.2.3 异常测试命令

1. NULL句柄测试：
```
h264_encode_error_test null_handle_test
```

2. 无效配置测试：
```
h264_encode_error_test invalid_config_test
```

## 5. 测试示例

### 5.1 基础编码测试

```
h264_encode init
h264_encode open
h264_encode encode
h264_encode close
h264_encode delete
```

正常log：
```
h264_enc_cli, XX, h264 encode init success!
h264_enc_cli, XX, h264 encode open success!
h264_enc_common, XX, h264 encode success! Encode time: XX ms
h264_enc_cli, XX, h264 encode close success!
h264_enc_cli, XX, h264 encode delete success!
```

### 5.2 常规测试

#### 5.2.1 正常编码测试

```
h264_encode_regular_test normal_test
```

预期log：
```
h264_enc_regular, XX, H264 encode normal scenario test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 5.2.2 异步编码测试

```
h264_encode_regular_test async_test
```

预期log：
```
h264_enc_regular, XX, H264 encode async test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

## 6. 配置选项

### 6.1 编码器配置

H264编码器提供了以下配置选项：

- **buffer_request_cb**: 请求输出缓冲区的回调函数
  - 当编码器需要缓冲区用于编码数据时调用
  - 应返回分配的缓冲区指针，失败时返回NULL

- **buffer_complete_cb**: 编码完成的回调函数
  - 编码完成时调用
  - 参数：缓冲区指针和结果代码

- **chnl_id**: 编码器的通道ID
  - 默认值：0

- **param**: 用户自定义参数
  - 可用于向回调传递上下文

## 7. 注意事项

1. 确保在使用编码器前正确初始化
2. 编码操作完成后，记得释放相关资源
3. H264编码需要来自帧缓冲区的YUV输入帧
4. 帧缓冲资源有限，请避免同时占用过多缓冲区
5. 编码默认是异步的；结果在buffer_complete_cb回调中返回
6. 输入帧应包含有效的YUV数据，并具有正确的宽度和高度
7. **回调函数使用注意事项**：
   - 回调函数中不建议执行阻塞操作（如长时间等待、sleep等），以避免影响编码性能和系统响应
   - 建议在回调函数中仅进行轻量级操作，如设置标志位、发送消息/信号量等，将耗时操作放到其他任务中执行
8. **缓冲区管理**：
   - 输入缓冲区从帧缓冲区显示队列获取
   - 输出缓冲区通过buffer_request_cb回调分配
   - 使用后应释放输入和输出缓冲区
9. **强制IDR帧**：
   - 使用force_idr命令强制下一帧编码为IDR帧
   - 可用于流同步或错误恢复
