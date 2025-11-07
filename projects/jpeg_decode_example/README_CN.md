# JPEG解码示例工程

* [English](./README.md)

## 1. 项目概述

本项目是一个JPEG解码测试模块，用于测试Beken平台上的JPEG解码功能。该模块提供了命令行接口(CLI)，支持硬件解码和软件解码两种方式，并支持软解码在DTCM上运行（解码速度有所提升）。

* 有关JPEG解码的详细信息，请参阅：

  - [JPEG解码概述](../../../developer-guide/video_codec/jpeg_decoding.html)

  - [JPEG硬件解码指南](../../../developer-guide/video_codec/jpeg_decoding_hw.html)

  - [JPEG软件解码指南](../../../developer-guide/video_codec/jpeg_decoding_sw.html)

* 有关API参考，请参阅：

  - [JPEG硬件解码API](../../../api-reference/multimedia/bk_jpegdec_hw.html)

  - [JPEG软件解码API](../../../api-reference/multimedia/bk_jpegdec_sw.html)

### 1.1 测试环境

   * 硬件配置：
      * 核心板，**BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * 支持，MJPEG硬件解码
      * YUV422
   * 支持，MJPEG软件解码
      * YUV420, YUV444, YUV400, YUV422
      * 输出YUYV格式时，可配置旋转角度（0°，90°，180°，270°）

.. warning::

    请使用参考外设，进行demo工程的熟悉和学习。如果外设规格不一样，代码可能需要重新配置。

## 2. 目录结构

项目采用AP-CP双核架构，主要源代码位于AP目录下。项目结构如下：

```
jpeg_decode_example/
├── .ci                   # CI配置目录
├── .gitignore            # Git忽略文件
├── CMakeLists.txt        # 项目级CMake构建文件
├── Makefile              # Make构建文件
├── README.md             # 项目说明文档（英文）
├── README CN.md          # 项目说明文档（中文）
├── ap/                   # AP端代码
│   ├── CMakeLists.txt    # AP端CMake构建文件
│   ├── Kconfig.projbuild # Kconfig配置
│   ├── ap_main.c         # AP主入口文件
│   ├── config/           # AP配置目录
│   └── jpeg_decode/      # JPEG解码实现
│       ├── data/         # 测试用JPEG图像数据
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

- 支持硬件JPEG解码和软件JPEG解码
- 提供命令行接口进行解码测试
- 支持获取JPEG图像的尺寸信息
- 提供了常规场景和异常场景的解码测试功能
- 支持硬件异步解码和突发模式测试
- 支持在DTCM上运行软件解码器以获得更快的性能

### 3.2 JPEG解码流程

1. 初始化JPEG解码器(硬件或软件)
2. 打开解码器
3. 执行解码操作：
   - 分配输入缓冲区并填充JPEG数据
   - 获取图像尺寸信息
   - 配置输入输出帧尺寸：将解析得到的图像宽度和高度设置到输入帧和输出帧结构中
   - 分配输出缓冲区
   - 执行解码（如果配置了旋转角度，软解码过程中会根据旋转角度重新调整输出帧的宽度和高度）
   - 释放缓冲区
4. 关闭解码器
5. 删除解码器实例

## 4. 编译与运行

### 4.1 编译方法

使用以下命令编译项目：

```
make bk7258 PROJECT=jpeg_decode_example
```

### 4.2 运行方法

编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试JPEG解码功能：

命令执行成功打印："CMDRSP:OK"

命令执行失败打印："CMDRSP:ERROR"

#### 4.2.1 基础解码命令

1. 初始化硬件JPEG解码器：
```
jpeg_decode init_hw
```

2. 初始化软件JPEG解码器：
```
jpeg_decode init_sw
```

3. 初始化在DTCM上运行的软件JPEG解码器(可选指定核心ID)：
```
jpeg_decode init_sw_on_dtcm [1|2]
```

4. 初始化硬件优化JPEG解码器：
```
jpeg_decode init_hw_opt
```

1,2,3,4命令中根据测试场景选择对应的命令进行初始化即可

5. 打开解码器：
```
jpeg_decode open
```

6. 执行解码操作：
YUV422格式图像解码：
```
jpeg_decode dec 422_864_480
```
YUV420格式图像解码：
```
jpeg_decode dec 420_864_480
```

其他支持的图像格式：
```
jpeg_decode dec 422_865_480
jpeg_decode dec 422_864_479
jpeg_decode dec 420_865_480
jpeg_decode dec 420_864_479
```

7. 关闭解码器：
```
jpeg_decode close
```

8. 删除解码器实例：
```
jpeg_decode delete
```

#### 4.2.2 常规测试命令

1. 硬件解码器常规测试：
```
jpeg_decode_regular_test hardware_test
```

2. 硬件解码器异步测试：
```
jpeg_decode_regular_test hardware_async_test
```

3. 硬件解码器异步突发测试(连续10次)：
```
jpeg_decode_regular_test hardware_async_burst_test
```

4. 软件解码器常规测试：
```
jpeg_decode_regular_test software_test
```

5. DTCM上的软件解码器(CP1)常规测试：
```
jpeg_decode_regular_test software_dtcm_cp1_test
```

6. DTCM上的软件解码器(CP2)常规测试：
```
jpeg_decode_regular_test software_dtcm_cp2_test
```

7. DTCM上的软件解码器(CP1)异步测试：
```
jpeg_decode_regular_test software_dtcm_cp1_async_test
```

8. DTCM上的软件解码器(CP1)异步突发测试(连续10次)：
```
jpeg_decode_regular_test software_dtcm_cp1_async_burst_test
```

9. DTCM上的软件解码器(CP2)异步测试：
```
jpeg_decode_regular_test software_dtcm_cp2_async_test
```

10. DTCM上的软件解码器(CP2)异步突发测试(连续10次)：
```
jpeg_decode_regular_test software_dtcm_cp2_async_burst_test
```

11. DTCM上的软件解码器(CP1+CP2)异步测试：
```
jpeg_decode_regular_test software_dtcm_cp1_cp2_async_test
```

12. DTCM上的软件解码器(CP1+CP2)异步突发测试(连续10次)：
```
jpeg_decode_regular_test software_dtcm_cp1_cp2_async_burst_test
```

13. 硬件优化解码器常规测试（可选参数：0=单缓冲模式，1=Ping-Pong模式）：
```
jpeg_decode_regular_test hardware_opt_test [0|1]
```
示例：
```
jpeg_decode_regular_test hardware_opt_test       # 使用单缓冲模式（默认）
jpeg_decode_regular_test hardware_opt_test 0     # 使用单缓冲模式
jpeg_decode_regular_test hardware_opt_test 1     # 使用Ping-Pong模式
```

14. 硬件优化解码器异步测试（可选参数：0=单缓冲模式，1=Ping-Pong模式）：
```
jpeg_decode_regular_test hardware_opt_async_test [0|1]
```

15. 硬件优化解码器异步突发测试（可选参数：count=突发次数，默认10次；0=单缓冲模式，1=Ping-Pong模式）：
```
jpeg_decode_regular_test hardware_opt_async_burst_test [count] [0|1]
```

## 5. 测试数据

项目中包含了不同格式的JPEG测试图像，存储在 `ap/jpeg_decode/data/` 目录下。主要包括:

   * **422_864_480** : YUV422格式的864x480分辨率JPEG图像
   * **420_864_480** : YUV420格式的864x480分辨率JPEG图像
   * **422_865_480** : YUV422格式的865x480分辨率JPEG图像
   * **422_864_479** : YUV422格式的864x479分辨率JPEG图像
   * **420_865_480** : YUV420格式的865x480分辨率JPEG图像
   * **420_864_479** : YUV420格式的864x479分辨率JPEG图像

硬解码仅支持解码YUV422格式的图像，YUV420格式的图像会解码失败。

硬解码图像需要宽度为16的倍数，高度为8的倍数，否则会解码失败。

软解码支持解码YUV420和YUV422格式的图像。

软解码图像需要宽度为2的倍数，高度无限制，否则会解码失败。

## 6. 测试示例

### 6.1 基础测试

#### 6.1.1 硬解码测试

```
jpeg_decode init_hw
jpeg_decode open
jpeg_decode dec 422_864_480
jpeg_decode close
jpeg_decode delete
```

正常log：
```
cli_jpeg_decode_cmd, XX, bk_hardware_jpeg_decode_new success!
cli_jpeg_decode_cmd, XX, jpeg decode open success!
cli_jpeg_decode_cmd, XX, jpeg decode get img dimensions success! 864x480 2
cli_jpeg_decode_cmd, XX, jpeg decode start success! Decode time: XX ms
cli_jpeg_decode_cmd, XX, jpeg decode delete success!
```

**支持的JPEG图像格式**：422_864_480（其他格式在硬解码时会失败，详见5. 测试数据部分的限制说明）

#### 6.1.2 软解码测试

```
jpeg_decode init_sw
jpeg_decode open
jpeg_decode dec 420_864_480
jpeg_decode close
jpeg_decode delete
```

正常log：
```
cli_jpeg_decode_cmd, XX, bk_software_jpeg_decode_new success!
cli_jpeg_decode_cmd, XX, jpeg decode open success!
cli_jpeg_decode_cmd, XX, jpeg decode get img dimensions success! 864x480 2
cli_jpeg_decode_cmd, XX, jpeg decode start success! Decode time: XX ms
cli_jpeg_decode_cmd, XX, jpeg decode delete success!
```

**支持的JPEG图像格式**：420_864_480、422_864_480、422_864_479、420_864_479（需满足软解码格式限制，详见5. 测试数据部分）

#### 6.1.3 使用CP1上的DTCM进行软解码测试

```
jpeg_decode init_sw_on_dtcm 1
jpeg_decode open
jpeg_decode dec 420_864_480
jpeg_decode close
jpeg_decode delete
```

正常log：
```
cli_jpeg_decode_cmd, XX, bk_software_jpeg_decode_on_dtcm_new success!
cli_jpeg_decode_cmd, XX, jpeg decode open success!
cli_jpeg_decode_cmd, XX, jpeg decode get img dimensions success! 864x480 2
cli_jpeg_decode_cmd, XX, jpeg decode start success! Decode time: XX ms
cli_jpeg_decode_cmd, XX, jpeg decode delete success!
```

**支持的JPEG图像格式**：与软解码测试相同

#### 6.1.4 使用CP2上的DTCM进行软解码测试

```
jpeg_decode init_sw_on_dtcm 2
jpeg_decode open
jpeg_decode dec 420_864_480
jpeg_decode close
jpeg_decode delete
```

正常log：
```
cli_jpeg_decode_cmd, XX, bk_software_jpeg_decode_on_dtcm_new success!
cli_jpeg_decode_cmd, XX, jpeg decode open success!
cli_jpeg_decode_cmd, XX, jpeg decode get img dimensions success! 864x480 2
cli_jpeg_decode_cmd, XX, jpeg decode start success! Decode time: XX ms
cli_jpeg_decode_cmd, XX, jpeg decode delete success!
```

**支持的JPEG图像格式**：与软解码测试相同

#### 6.1.5 使用硬件优化解码器进行测试

```
jpeg_decode init_hw_opt
jpeg_decode open
jpeg_decode dec 422_864_480
jpeg_decode close
jpeg_decode delete
```

正常log：
```
cli_jpeg_decode_cmd, XX, bk_hardware_jpeg_decode_opt_new success!
cli_jpeg_decode_cmd, XX, jpeg decode open success!
cli_jpeg_decode_cmd, XX, jpeg decode get img dimensions success! 864x480 2
cli_jpeg_decode_cmd, XX, jpeg decode start success! Decode time: XX ms
cli_jpeg_decode_cmd, XX, jpeg decode delete success!
```

**支持的JPEG图像格式**：422_864_480（与硬解码限制相同）

**特点说明**：
- 使用SRAM缓冲进行优化解码，降低峰值内存占用
- 支持Ping-Pong缓冲模式，提高解码效率
- 可配置拷贝方法（MEMCPY或DMA）

### 6.2 常规测试

项目提供了多种常规场景的解码测试功能，用于验证解码器在正常情况下的工作性能。以下是各种常规测试命令和预期结果：

#### 6.2.1 硬件解码测试

该命令为同步解码命令，解码完成后，函数才返回；先打印jpeg_decode_out_complete，再打印perform_jpeg_decode_async_test。


```
jpeg_decode_regular_test hardware_test
```

预期log：
```
ap1:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
ap0:jdec_com:D(XX):perform_jpeg_decode_async_test, XX, jpeg async decode success! Decode time: 30 ms
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.2 硬件解码异步测试

该命令为异步解码命令，命令发送成功后，函数立即返回，先打印perform_jpeg_decode_async_test，再打印jpeg_decode_out_complete。


```
jpeg_decode_regular_test hardware_async_test
```

预期log：
```
ap0:jdec_com:D(XX):perform_jpeg_decode_async_test, XX, jpeg async decode success!
ap1:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.3 硬件解码异步突发测试

该命令为异步突发测试命令，一次性调用多次异步解码函数，图像会先存到队列中，再从队列中依次获取数据进行解码。
连续打印多次perform_jpeg_decode_async_burst_test后，再打印jpeg_decode_out_complete。

```
jpeg_decode_regular_test hardware_async_burst_test
```

预期log：

```
ap0:jdec_com:I(XX):perform_jpeg_decode_async_burst_test, XX, Start hardware_test with 10 bursts!
ap0:jdec_com:D(XX):perform_jpeg_decode_async_burst_test, XX, Burst test 1/10
...
ap0:jdec_com:D(XX):perform_jpeg_decode_async_burst_test, XX, Burst test 10/10
ap0:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
...
ap1:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.4 软件解码测试

```
jpeg_decode_regular_test software_test
```

预期log：
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg decode Normal scenario JPEG decoding test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.5 DTCM上的软件解码测试(CP1)

```
jpeg_decode_regular_test software_dtcm_cp1_test
```

预期log：
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg decode Normal scenario JPEG decoding test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.6 DTCM上的软件解码测试(CP2)

```
jpeg_decode_regular_test software_dtcm_cp2_test
```

预期log：
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg decode Normal scenario JPEG decoding test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.7 DTCM上的软件解码器(CP1)异步测试

```
jpeg_decode_regular_test software_dtcm_cp1_async_test
```

预期log：
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async decode on CP1 test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.8 DTCM上的软件解码器(CP1)异步突发测试

```
jpeg_decode_regular_test software_dtcm_cp1_async_burst_test
```

预期log：
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async burst decode on CP1 test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.9 DTCM上的软件解码器(CP2)异步测试

```
jpeg_decode_regular_test software_dtcm_cp2_async_test
```

预期log：
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async decode on CP2 test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.10 DTCM上的软件解码器(CP2)异步突发测试

```
jpeg_decode_regular_test software_dtcm_cp2_async_burst_test
```

预期log：
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async burst decode on CP2 test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.11 DTCM上的软件解码器(CP1+CP2)异步测试

```
jpeg_decode_regular_test software_dtcm_cp1_cp2_async_test
```

预期log：
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async decode on CP1+CP2 test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.12 DTCM上的软件解码器(CP1+CP2)异步突发测试

```
jpeg_decode_regular_test software_dtcm_cp1_cp2_async_burst_test
```

预期log：
```
cli_jpeg_decode_regular_test_cmd, XX, software jpeg async burst decode on CP1+CP2 test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

#### 6.2.13 硬件优化解码器常规测试

该测试支持可选参数来选择缓冲模式：

单缓冲模式测试（默认）：
```
jpeg_decode_regular_test hardware_opt_test
```
或
```
jpeg_decode_regular_test hardware_opt_test 0
```

Ping-Pong缓冲模式测试：
```
jpeg_decode_regular_test hardware_opt_test 1
```

预期log：
```
cli_jpeg_decode_regular_test_cmd, XX, Using single buffer mode (or Using pingpong mode)
cli_jpeg_decode_regular_test_cmd, XX, hardware opt jpeg decode Normal scenario JPEG decoding test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

**说明**：
- 参数0或不指定参数：使用单缓冲模式，内存占用更低
- 参数1：使用Ping-Pong双缓冲模式，解码效率更高

#### 6.2.14 硬件优化解码器异步测试

该测试支持可选参数来选择缓冲模式：

单缓冲模式测试（默认）：
```
jpeg_decode_regular_test hardware_opt_async_test
```
或
```
jpeg_decode_regular_test hardware_opt_async_test 0
```

Ping-Pong缓冲模式测试：
```
jpeg_decode_regular_test hardware_opt_async_test 1
```

预期log：
```
ap0:jdec_com:D(XX):perform_jpeg_decode_async_test, XX, jpeg async decode success!
ap1:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
cli_jpeg_decode_regular_test_cmd, XX, hardware opt jpeg async decode test completed!
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

**说明**：
- 参数0或不指定参数：使用单缓冲模式，内存占用更低
- 参数1：使用Ping-Pong双缓冲模式，解码效率更高

#### 6.2.15 硬件优化解码器异步突发测试

该命令为异步突发测试命令，一次性调用多次异步解码函数。

```
jpeg_decode_regular_test hardware_opt_async_burst_test
```

或指定突发次数：
```
jpeg_decode_regular_test hardware_opt_async_burst_test [count] [0|1]
```

预期log：
```
ap0:jdec_com:I(XX):perform_jpeg_decode_async_burst_test, XX, Start hardware_opt_async_burst_test with 10 bursts!
ap0:jdec_com:D(XX):perform_jpeg_decode_async_burst_test, XX, Burst test 1/10
...
ap0:jdec_com:D(XX):perform_jpeg_decode_async_burst_test, XX, Burst test 10/10
ap0:jdec_com:D(XX):jpeg_decode_out_complete, XX, jpeg decode success! format_type: 5, out_frame: 0xXX
...
```

异常log（表示测试失败）：
```
CMDRSP:ERROR
```

**说明**：
- count参数可选，默认为10
- 参数0或不指定参数：使用单缓冲模式，内存占用更低
- 参数1：使用Ping-Pong双缓冲模式，解码效率更高

## 7. 配置选项

### 7.1 线程栈大小配置

解码器的线程栈大小可以通过Kconfig进行配置：

- **CONFIG_HW_JPEG_DECODE_TASK_STACK_SIZE**: 硬件JPEG解码任务的线程栈大小（字节）
  - 默认值：1024 字节
  - 配置路径：menuconfig -> JPEG Decoder -> Hardware JPEG decode task stack size

- **CONFIG_HW_JPEG_DECODE_OPT_TASK_STACK_SIZE**: 硬件优化JPEG解码任务的线程栈大小（字节）
  - 默认值：2048 字节
  - 配置路径：menuconfig -> JPEG Decoder -> Hardware optimized JPEG decode task stack size

- **CONFIG_SW_JPEG_DECODE_TASK_STACK_SIZE**: 软件JPEG解码任务的线程栈大小（字节）
  - 默认值：1024 字节
  - 配置路径：menuconfig -> JPEG Decoder -> Software JPEG decode task stack size

说明：根据实际解码场景和内存资源调整栈大小，如遇到栈溢出可适当增大此值。

### 7.2 硬件优化解码器配置

硬件优化解码器提供了以下配置选项：

- **is_pingpong**: 是否启用Ping-Pong缓冲模式
  - true: 启用双缓冲，提高并行度
  - false: 使用单缓冲，降低内存占用

- **copy_method**: 数据拷贝方法
  - JPEG_DECODE_OPT_COPY_METHOD_MEMCPY: 使用os_memcpy进行数据拷贝
  - JPEG_DECODE_OPT_COPY_METHOD_DMA: 使用DMA进行数据拷贝（当前版本会回退到MEMCPY）

- **sram_buffer**: SRAM缓冲区指针
  - NULL: 自动分配SRAM缓冲区
  - 非NULL: 使用指定的SRAM缓冲区

- **lines_per_block**: 每次解码的行数
  - 必须为8或16
  - 建议值：16（适用于大多数场景）

- **image_max_width**: 图像最大宽度
  - 用于计算SRAM缓冲区大小
  - 默认值：864

## 8. 注意事项

1. 确保在使用解码器前正确初始化
2. 解码操作完成后，记得释放相关资源
3. 硬件解码和软件解码功能有所差异，请根据实际需求选择合适的解码方式：
   - 硬件解码仅支持YUV422格式图像,且需要图像宽度为16的倍数，高度为8的倍数
   - 硬件优化解码与硬件解码格式限制相同，但使用SRAM缓冲优化，内存峰值更低
   - 软件解码支持YUV420和YUV422格式图像，且需要图像宽度为2的倍数，高度无限制
4. 在DTCM上运行的软件解码器通常比普通软件解码器提供更快的解码速度
5. 帧缓冲资源有限，请避免同时占用过多缓冲区
6. 输入frame中，结构体内的length需要设置为实有效数据的长度，输出frame中，结构体内的size需要设置为最大的可存放的大小；
   输入frame的length为0或输出frame的size为0均会出现解码错误；
7. **图像尺寸和旋转处理说明**：

   - 解码器在获取JPEG图像信息后，会将解析得到的宽度和高度设置到输入帧和输出帧结构中
   - 对于软件解码，如果配置了旋转角度，输出帧的宽度和高度会在解码内部根据旋转角度重新配置：

     * 旋转90度或270度时：输出帧的宽度和高度会互换（width = 原height，height = 原width）
     * 旋转0度或180度时：输出帧保持原始的宽度和高度

   - 硬件解码和硬件优化解码不支持旋转功能，输出帧尺寸与输入帧尺寸保持一致

8. **硬件优化解码器使用建议**：
   - 对于内存受限的应用场景，推荐使用硬件优化解码器
   - Ping-Pong模式适用于需要高吞吐量的场景
   - 单缓冲模式适用于内存紧张的场景
   - lines_per_block建议设置为16以获得最佳性能
   - SRAM缓冲区可以复用，避免频繁分配和释放

9. **回调函数使用注意事项**：
   - 回调函数中不建议执行阻塞操作（如长时间等待、sleep等），以避免影响解码性能和系统响应
   - 建议在回调函数中仅进行轻量级操作，如设置标志位、发送消息/信号量等，将耗时操作放到其他任务中执行