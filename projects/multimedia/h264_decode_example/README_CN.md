# H264解码示例工程

* [English](./README.md)

## 1. 项目概述

本工程用于演示 Beken 平台上的 H264 解码流程，当前主要包含三类使用方式：

- 基础 H264 解码 CLI：`h264_decode`
- 独立 FLEXA 解码示例：`h264_decode_flexa_test`
- 带 DMA 压力的循环解码测试：`h264_decode_stress`

和 `jpeg_decode_example` 不同，当前 `h264_decode_example` 的 `main()` 仅负责初始化和注册 CLI，不会在开机时自动启动解码测试。所有测试都需要通过串口手动触发。

* 有关 H264 解码的详细信息，请参阅：

  - [H264软件解码概述](../../../developer-guide/video_codec/h264_decoding_sw.html)

### 1.1 测试环境

- 硬件配置
  - 核心板：`BK7258_QFN88_9X9_V3.2`
  - PSRAM：`8M/16M`
- 输入数据
  - 工程内置 H264 码流
- 输出内容
  - 解码过程日志
  - FLEXA 输出回调日志
  - 压力测试轮次日志

.. warning::

    请使用参考外设进行示例学习与验证。如果外设规格不同，代码和配置可能需要同步调整。

## 2. 目录结构

项目采用 AP-CP 双核结构，主要逻辑位于 AP 侧：

```text
h264_decode_example/
├── README.md
├── README_CN.md
├── ap/
│   ├── ap_main.c                           # AP 主入口，注册 h264 相关 CLI
│   ├── h264_decode/
│   │   ├── include/
│   │   └── src/h264_decode_cli.c          # h264_decode CLI 实现
│   └── h264_decode_stress/
│       ├── include/
│       └── src/
│           ├── h264_decode_flexa_test.c   # 独立 FLEXA 示例
│           ├── h264_decode_stress.c       # H264 解码压力测试
│           └── h264_decode_stress_stream.c
├── cp/
├── partitions/
└── pj_config.mk
```

## 3. 功能说明

### 3.1 当前支持的功能

- `h264_decode h264d`：执行普通 H264 解码示例
- `h264_decode h264d_flexa`：执行 H264 FLEXA 解码示例
- `h264_decode_flexa_test start`：运行独立 FLEXA 线程示例
- `h264_decode_stress`：执行循环解码压力测试，并可叠加 DMA 压力

## 4. 编译与运行

### 4.1 编译方法

```bash
make bk7259 PROJECT=multimedia/h264_decode_example
```

### 4.2 运行方式

烧录固件后，通过串口终端手动输入命令触发测试。

#### 4.2.1 CLI 命令列表

基础解码命令：

```text
h264_decode help
h264_decode h264d
h264_decode h264d_flexa
```

独立 FLEXA 示例命令：

```text
h264_decode_flexa_test start
```

压力测试命令：

```text
h264_decode_stress start
h264_decode_stress start 512
h264_decode_stress stop
h264_decode_stress dma_open
h264_decode_stress dma_close
```

其中：

- `start` 表示使用默认 DMA 搬运大小启动压力测试
- `start 512` 这类形式表示指定 `dma_copy_size_kb`
- `dma_open` / `dma_close` 用于单独打开或关闭 DMA 压力源

命令提交成功时返回：

```text
CMDRSP:OK
```

命令提交失败时返回：

```text
CMDRSP:ERROR
```

#### 4.2.2 如何判断测试成功或失败

`CMDRSP:OK` 仅表示 CLI 成功创建任务，不代表测试已经通过。当前 H264 示例没有统一的 `[RESULT][PASS]` 结果行，需要结合日志判断。

通常可按以下方式判断：

- `h264_decode h264d`
  - 提交后无立即报错，且解码流程持续输出正常日志
- `h264_decode h264d_flexa` / `h264_decode_flexa_test start`
  - 无 `h264_decoder_init failed`、`h264_decoder_decode failed` 等错误日志
  - 运行结束时可看到 `exit h264_decode_flexa_test`
- `h264_decode_stress`
  - 运行过程中无分配失败、初始化失败或解码失败日志
  - 执行 `h264_decode_stress stop` 后可看到类似如下退出日志：

```text
h264 decode stress thread exit, rounds=123 stop=1
```

常见失败日志包括：

```text
h264_decoder_init failed, ret=...
h264_decoder_decode failed at round=... ret=...
failed to allocate ... buffer
psram_dma_stress_start failed, ret=...
```

## 5. 注意事项

1. 当前 `h264_decode` CLI 实际仅支持 `h264d` 和 `h264d_flexa` 两个子命令，请以源码中的参数解析逻辑为准。
2. `h264_decode`、`h264_decode_flexa_test`、`h264_decode_stress` 都通过独立线程运行测试，避免阻塞 CLI 线程。
3. `h264_decode_stress start [dma_copy_size_kb]` 中的可选参数单位为 KB。
4. 压力测试会额外申请输出缓冲区、码流缓冲区以及 DMA 压测缓冲区，运行前请确认系统有足够内存。
5. 如果需要查看统一的 `[RESULT][PASS/FAIL]` 风格日志，请参考 `jpeg_decode_example` 中的 `vcdec` JPEG 示例。
