# Frame Buffer 示例工程

* [English](./README.md)

## 1. 项目概述

`frame_buffer_example` 是 BK7259 平台的 frame buffer 内存管理示例。工程演示 AVDK frame buffer 模块的初始化、buffer 申请、数据写入、内存 dump、buffer 释放和 heap 状态查看流程，适合用于了解多媒体 buffer 的 slab heap 分配机制和调试方法。

本示例包含：

- `bk_frame_buffer_init()` 初始化流程。
- `bk_frame_buffer_malloc()` 和 `bk_frame_buffer_free()` 基本用法。
- `MEM_SLAB_HEAP_UNCODED` heap 的分配示例。
- `bk_mem_slab_dump_heap()` heap 状态打印。
- `avdk_hex_dump()` 内存内容打印。
- 可选的 frame buffer 越界检测验证用例。

## 2. 硬件需求

- SoC/开发板：BK7259 系列开发板。
- 调试接口：串口控制台，用于查看日志。

本示例不依赖 LCD、摄像头、USB 设备或 SD 卡等外设。

## 3. 目录结构

```text
frame_buffer_example/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── ap_main.c             # AP 主入口，启动 frame buffer 测试
│   ├── frame_buffer_test.c   # frame buffer 分配、释放和越界检测用例
│   ├── frame_buffer_test.h
│   └── config/
├── cp/
│   ├── cp_main.c
│   └── config/
└── partitions/
```

## 4. 编译与烧录

在 SDK 根目录执行编译命令：

```bash
make bk7259 PROJECT=multimedia/frame_buffer_example -j
```

编译完成后，固件位于：

```text
build/bk7259/frame_buffer_example/package/all-app.bin
```

将 `all-app.bin` 烧录到开发板。烧录完成后复位开发板，并打开串口终端查看日志。

## 5. 运行说明

开发板上电后会自动运行默认测试，无需输入 CLI 命令。默认流程如下：

1. `bk_init()` 初始化系统。
2. `frame_buffer_test()` 初始化 frame buffer 模块。
3. `frame_buffer_mem_dump_test()` 申请 3 块 64-byte buffer。
4. 分别向 3 块 buffer 写入 `0x11`、`0x22`、`0x33`。
5. 打印 heap 状态和 buffer 附近的十六进制内存内容。
6. 释放 3 块 buffer。
7. 再次打印 heap 状态和 buffer 附近内存内容。

串口日志应包含类似输出：

```text
AP main running...
frame_buffer_mem_dump_test: start
frame_buffer_mem_dump_test: frame1: 0x...
frame_buffer_mem_dump_test: frame2: 0x...
frame_buffer_mem_dump_test: frame3: 0x...
frame_buffer_mem_dump_test: end
```

日志中还会输出 `bk_mem_slab_dump_heap()` 的 heap 信息，以及 `avdk_hex_dump()` 打印的内存数据。

## 6. 越界检测用例

`ap/frame_buffer_test.c` 中预留了两个越界写检测用例：

- `frame_buffer_mem_overflow_test1()`：写越界后释放目标 buffer，通过 free 流程触发检查。
- `frame_buffer_mem_overflow_test2()`：写越界后调用 `bk_mem_slab_check_all_heaps()` 检查所有 heap。

这两个用例默认关闭：

```c
//frame_buffer_mem_overflow_test1();
//frame_buffer_mem_overflow_test2();
```

如需验证越界检测能力，可在 `frame_buffer_test()` 中取消其中一个函数调用的注释，然后重新编译烧录。建议一次只打开一个越界用例。

注意：越界检测用例会故意破坏 buffer 保护区，可能触发错误日志、assert 或系统异常。这属于该测试的预期行为，不代表默认 frame buffer 分配流程异常。

## 7. 关键配置

本示例依赖以下主要配置：

```text
CONFIG_FRAME_BUFFER=y
CONFIG_SYS_PRINT_DEV_MAILBOX=y
```

工程默认关闭网络和部分无关外设配置，以减少对其他模块的依赖。

## 8. 注意事项

1. 默认测试只用于观察 frame buffer 分配和释放过程，不会持续运行后台任务。
2. `avdk_hex_dump()` 打印范围包含用户 buffer 前后的保护区，便于观察内存布局。
3. 越界检测用例需要修改源码开启，建议仅在调试或验证内存保护机制时使用。
4. 如果系统中加入其他多媒体模块，frame buffer heap 可用空间会受到影响。

## 9. 常见问题

### 日志提示 `malloc failed`

请检查 frame buffer heap 是否有足够空间。如果工程中增加了其他模块，也需要确认这些模块是否已经占用了 slab heap 内存。

### 串口没有看到 hex dump

请确认串口输出配置和日志等级。示例主要通过 `BK_LOGI` 输出日志，并使用 mailbox 输出系统日志。

### 开启越界用例后系统异常

越界用例会主动写坏保护区，出现 assert、异常或错误日志是预期现象。恢复默认测试时，请重新注释越界用例调用。
