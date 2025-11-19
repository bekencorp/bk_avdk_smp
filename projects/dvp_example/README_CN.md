# DVP示例工程

* [English](./README.md)

## 1. 工程概述

本工程是一个DVP示例工程，用于实现DVP设备功能。该模块提供CLI测试命令，通过发送命令可以实现打开和关闭DVP，获取实时DVP输出的图像。DVP输出的格式是YUV422，但是此工程不仅可以输出YUV422，还支持输出编码后的数据MJPEG，或H.264，但是不能同时输出两种编码数据。另外还举例如何适配新的DVP sensor，参考dvp_gc0001_test.c的添加方式。

* 有关DVP使用方法的详细说明，请参考如下链接：

  - [DVP使用方法](../../../developer-guide/camera/dvp.html)

* 有关DVP API和数据结构的详细说明，请参考如下链接：

  - [DVP API](../../../api-reference/multimedia/bk_camera.html)


* 有关如何添加支持新的DVP sensor，请参考当前工程，或者如下链接：

  - [DVP ADD](../../../developer-guide/camera/dvp.html)

### 1.1 测试环境

   * 硬件配置：
      * 核心板，**BK7258_QFN88_9X9_V3.2**
      * PSRAM 8M/16M
   * 支持，DVP GC2145 最大分辨率为 1280x720，输出格式为 YUV422

   * 已经支持的DVP设备，参考：[支持的DVP外设](../../../support_peripherals/index.html#dvp-camera)

.. note::

    请使用参考外设，进行demo工程的熟悉和学习。如果外设规格不一样，代码可能需要重新配置。

## 2. 目录结构

项目采用AP-CP双核架构，主要源代码位于AP目录下。项目结构如下：

```
dvp_example/
├── .ci/                  # CI配置文件目录
├── .gitignore            # Git忽略文件配置
├── .it.csv               # 测试用例配置文件
├── CMakeLists.txt        # 项目级CMake构建文件
├── Makefile              # Make构建文件
├── README.md             # 英文项目说明文档
├── README_CN.md          # 中文项目说明文档
├── ap/                   # AP端代码目录
│   ├── CMakeLists.txt    # AP端CMake构建文件
│   ├── ap_main.c         # AP端主入口文件
│   ├── config/           # AP端配置文件目录
│   │   └── bk7258_ap/    # BK7258 AP端配置
│   │       ├── config     # 系统配置文件
│   │       └── usr_gpio_cfg.h  # 用户GPIO配置头文件
│   └── dvp_test/         # DVP测试代码目录
│       ├── include/      # 测试头文件目录
│       │   ├── dvp_cli.h       # DVP命令行接口头文件
│       │   └── dvp_frame_list.h # DVP帧列表管理头文件
│       └── src/          # 测试源代码目录
│           ├── dvp_api_test.c  # DVP API测试代码
│           ├── dvp_frame_list.c # DVP帧列表管理实现
│           ├── dvp_func_test.c # DVP功能测试代码
│           ├── dvp_gc0001_test.c # GC0001传感器测试代码
│           └── dvp_main.c      # DVP测试主程序
├── app.rst               # 应用说明文档
├── cp/                   # CP端代码目录
│   ├── CMakeLists.txt    # CP端CMake构建文件
│   ├── cp_main.c         # CP端主文件
│   └── config/           # CP端配置文件目录
│       └── bk7258/       # BK7258 CP端配置
│           ├── config     # 系统配置文件
│           └── usr_gpio_cfg.h  # 用户GPIO配置头文件
├── it.yaml               # 集成测试配置文件
├── partitions/           # 分区配置目录
│   └── bk7258/           # BK7258分区配置
│       ├── auto_partitions.csv  # 自动分区配置
│       └── ram_regions.csv      # RAM区域配置
└── pj_config.mk          # 项目配置Makefile
```

## 3.功能说明

### 3.1 主要功能

- 支持DVP设备的打开和关闭
- 支持只输出YUV422格式数据
- 支持只输出MJPEG格式数据
- 支持只输出H264格式数据
- 支持输出YUV422和MJPEG格式数据
- 支持输出YUV422和H264格式数据

### 3.2 主要代码

- `dvp_frame_list.c`: DVP帧列表管理实现，用于管理DVP输出的帧数据
- `dvp_api_test.c`: DVP控制器接口的测试
- `dvp_func_test.c`: DVP功能测试，只区分开关操作

### 3.3 CLI命令使用
```bash
dvp open [width] [height] [type]  # 打开DVP设备
dvp close             # 关闭DVP设备
```

参数说明：

- `width`: 视频宽度
- `height`: 视频高度
- `type`: 视频格式(jpeg/h264/h265/yuv/enc_yuv)

    `jpeg`: 输出MJPEG编码视频数据

    `h264`: 输出H264编码视频数据

    `yuv`: 输出YUV422格式数据

    `enc_yuv`: 输出编码后的数据和YUV422格式数据同时输出

### 3.4 用户参考文件
- `projects/dvp_example/ap/dvp_test/src/dvp_main.c` : DVP测试主函数
- `projects/dvp_example/ap/dvp_test/src/dvp_frame_list.c` : DVP帧列表管理实现
- `projects/dvp_example/ap/dvp_test/src/dvp_api_test.c` : DVP API测试代码
- `projects/dvp_example/ap/dvp_test/src/dvp_func_test.c` : DVP功能测试代码

## 4. 编译和运行

### 4.1 编译方法

使用以下命令编译项目：

```
make bk7258 PROJECT=dvp_example
```

### 4.2 运行方法

编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试DVP功能：

命令执行成功打印："CMDRSP:OK"

命令执行失败打印："CMDRSP:ERROR"

### 4.3 配置参数

- `BK_DVP_864X480_30FPS_MJPEG_CONFIG`: 默认分辨率配置

### 4.4 测试示例

测试命令需要添加前缀：“ap_cmd”，如 `ap_cmd dvp open 864 480 jpeg`

#### 4.4.1  - DVP打开只出JPEG图测试
- CASE命令:

```bash
dvp open 864 480 jpeg
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
dvp:27[322 8KB 2010Kbps] 每大概4-6秒打一次统计信息，27为出图帧率
```
- CASE失败标准：

```
CMDRSP:ERROR
```
- CASE失败日志：

```
上面的log不打印，或者seq突然不增大了，或者统计的帧率突然变为0，都表示异常
```

#### 4.4.2  - DVP关闭测试
- CASE命令:

```bash
dvp close
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

#### 4.4.3  - DVP打开只出H264图测试
- CASE命令:

```bash
dvp open 864 480 h264
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
dvp:27[322 8KB 2010Kbps] 每大概4-6秒打一次统计信息，27为出图帧率
```
- CASE失败标准：

```
CMDRSP:ERROR
```

#### 4.4.4  - DVP打开只出YUV图测试
- CASE命令:

```bash
dvp open 864 480 yuv
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
dvp:27[322 8KB 2010Kbps] 每大概4-6秒打一次统计信息，27为出图帧率
```
- CASE失败标准：

```
CMDRSP:ERROR
```
- CASE失败日志：

```
上面的log不打印，或者seq突然不增大了，或者统计的帧率突然变为0，都表示异常
```

#### 4.4.5  - DVP打开出H264&YUV图测试
- CASE命令:

```bash
dvp open 864 480 h264 enc_yuv
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
dvp:27[322 8KB 2010Kbps] 每大概4-6秒打一次统计信息，27为出图帧率
```
- CASE失败标准：

```
CMDRSP:ERROR
```
- CASE失败日志：

```
上面的log不打印，或者seq突然不增大了，或者统计的帧率突然变为0，都表示异常
```

#### 4.4.6  - DVP打开出JPEG和YUV图测试
- CASE命令:

```bash
dvp open 864 480 jpeg enc_yuv
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
dvp:27[322 8KB 2010Kbps] 每大概4-6秒打一次统计信息，27为出图帧率
```
- CASE失败标准：

```
CMDRSP:ERROR
```
- CASE失败日志：

```
上面的log不打印，或者seq突然不增大了，或者统计的帧率突然变为0，都表示异常
```

#### 4.4.7  - DVP打开不支持的分辨率测试
- CASE命令:

```bash
dvp open 1024 600 jpeg
```

- CASE预期结果：

```
失败
```
- CASE成功标准：

```
CMDRSP:ERROR
```

#### 4.4.8  - DVP打开不支持的格式测试
- CASE命令:

```bash
dvp open 864 480 h265
```

- CASE预期结果：

```
失败
```
- CASE成功标准：

```
CMDRSP:ERROR
```

### 4.5 正常打印日志
- CMDRSP:OK 打印命令执行成功
- CMDRSP:ERROR 打印命令执行失败
- dvp:27[322 8KB 2010Kbps] 打印DVP出图信息dvp:帧率[当前帧号 当前图像大小 码率大小]