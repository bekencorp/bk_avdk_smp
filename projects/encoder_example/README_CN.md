## Encoder示例工程

- [English](./README.md)

## 1. 工程概述

本工程是一个编码器示例工程，用于演示在 BK7258 上将内置的 YUV422（YUYV）测试图像编码为 **MJPEG** 数据。
工程提供 CLI 测试命令，可通过串口命令完成编码器控制器的 **open/encode/get_compress/set_compress/close** 流程。

### 1.1 测试环境

- **硬件配置**：
  - 核心板：BK7258_QFN88_9X9_V3.2
  - PSRAM 8M/16M
- **输入图像**：
  - 内置 YUV422（YUYV）测试 buffer：`projects/encoder_example/ap/yuv_test_buf.c`
  - 默认分辨率：640x480（见 `projects/encoder_example/ap/encoder_cli.h`）

## 2. 目录结构

```
encoder_example/
├── .it.csv               # 测试用例配置文件
├── CMakeLists.txt        # 项目级CMake构建文件
├── Makefile              # Make构建文件
├── README.md
├── README_CN.md
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.c         # 注册CLI命令：jpeg_encode
│   ├── encoder_cli.h
│   ├── yuv_test_buf.c    # get_yuv_test_buf()
│   ├── jpeg_encoder_test.c
├── cp/
├── partitions/
└── pj_config.mk
```

## 3. CLI命令使用

工程在 `ap/ap_main.c` 中注册以下命令：

- **jpeg_encode**：`open | encode | set_compress | get_compress | close`

说明：

- **open**：创建并打开对应编码器控制器（使用默认分辨率、默认 fps、默认 yuv_format）
- **encode**：对内置 YUV 测试 buffer 进行编码，并通过 `stack_mem_dump()` 打印输出数据
- **set_compress**：更新压缩参数（期望输出 JPEG 大小范围）
- **get_compress**：获取当前压缩参数（期望输出 JPEG 最小/最大 size）
- **close**：关闭并销毁控制器句柄

测试命令需要添加前缀 `ap_cmd`，示例：

```bash
ap_cmd jpeg_encode open
ap_cmd jpeg_encode get_compress
ap_cmd jpeg_encode encode
ap_cmd jpeg_encode set_compress 10240 40960
ap_cmd jpeg_encode get_compress
ap_cmd jpeg_encode close
```

命令执行成功打印：`CMDRSP:OK`
命令执行失败打印：`CMDRSP:ERROR`

## 4. 编译和运行

### 4.1 编译方法

```bash
make bk7258 PROJECT=encoder_example
```

### 4.2 运行方法

编译完成后烧录固件，通过串口终端执行上述 CLI 命令测试编码流程。

## 5 注意事项

1.输入图像的格式为YUV422，默认为YUYV（大端模式）
2.encode命令执行耗时可能较长，自动化测试用例超时时间建议设置为20秒
3.关于默认压缩率，可以通过命令行（接口）来获取，不同的分辨率配置的默认输出图像范围是不一样的，比如640X480，默认是