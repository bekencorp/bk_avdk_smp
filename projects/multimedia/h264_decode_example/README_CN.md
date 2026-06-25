# H264解码示例工程

* [English](./README.md)

## 1. 项目概述

本工程用于演示 Beken 平台上的 H264 解码流程，当前主要包含以下几类使用方式：

- 基于 `bk_decoder` 的 VCDEC CLI：`h264_decode vcdec_h264d` / `h264_decode vcdec_h264d_flexa`
- 传统 H264D FLEXA 示例 CLI：`h264_decode_flexa start`
- 直接寄存器路径 VCDEC CLI：`vcdec_h264_driver`
- 带 DMA 压力的循环解码测试：`h264_decode_stress`
- 公共 Annex-B H264 码流解析模块，供 VCDEC case 复用

当前 `main()` 除了初始化和注册 CLI 外，在使能 `CONFIG_BK_DECODER` 时还会自动创建 boot demo 线程，按顺序执行一次 `vcdec_h264_flexa_test` 和 `vcdec_h264_test`。本工程默认 `defconfig` 已开启 `CONFIG_BK_DECODER`。

* 有关 H264 解码的详细信息，请参阅：

  - [H264软件解码概述](../../../developer-guide/video_codec/h264_decoding_sw.html)

### 1.1 测试环境

- 硬件配置
  - 核心板：**BK7259_QF128_12.3X12.3_V4.0**
  - PSRAM：32M
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
│   ├── ap_main.c                           # AP 主入口，注册 H264 相关 CLI
│   ├── h264_decode/
│   │   ├── common/
│   │   │   ├── h264_decode_h264_parser.c   # 公共 Annex-B AU / 帧类型解析
│   │   │   ├── h264_decode_h264_parser.h
│   │   │   ├── h264_decode_stream_1280x720.c
│   │   │   └── h264_decode_stream_256x128.c
│   │   ├── include/
│   │   └── src/
│   │       ├── h264_decode_cli.c           # h264_decode CLI 实现
│   │       ├── h264_decode_flexa_test.c    # 传统 FLEXA CLI 示例
│   │       ├── vcdec_h264_driver_test.c    # 直接寄存器 VCDEC CLI
│   │       └── vcdec_h264_test.c           # bk_decoder VCDEC 示例与 boot demo
│   └── h264_decode_stress/
│       ├── include/
│       └── src/
│           ├── h264_decode_stress.c        # H264 解码压力测试
│           └── h264_decode_stress_stream.c
├── cp/
├── partitions/
└── .it.csv
```

## 3. 功能说明

### 3.1 当前支持的功能

- `h264_decode vcdec_h264d [1280x720|256x128]`：执行基于 `bk_decoder` 的 VCDEC 帧模式解码
- `h264_decode vcdec_h264d_flexa [1280x720|256x128]`：执行基于 `bk_decoder` 的 VCDEC FLEXA 解码
- `h264_decode_flexa start`：执行传统 `h264_decoder_*` FLEXA 示例
- `vcdec_h264_driver frame|flexa [1280x720|256x128]`：执行直接寄存器 VCDEC 解码验证
- `h264_decode_stress`：执行循环解码压力测试，并可叠加 DMA 压力
- 公共 H264 parser：解析 Annex-B access unit、帧类型和 frame table，供 `vcdec_h264_test.c`、`vcdec_h264_driver_test.c` 复用
- 上电后自动 boot demo：在 `CONFIG_BK_DECODER=y` 时自动执行一次 `vcdec_h264_flexa_test(1280x720)` 和 `vcdec_h264_test(1280x720)`

> 说明：此前新增的 `h264d_native_frame_test.c` 和 `h264d_native_flexa_test.c` 已删除，相关 `h264d_native*` CLI 入口也已移除。

## 4. 编译与运行

### 4.1 编译方法

```bash
make bk7259 PROJECT=multimedia/h264_decode_example -j32
```

### 4.2 运行方式

烧录固件后，若 `CONFIG_BK_DECODER=y`，系统会上电自动触发一次 VCDEC boot demo；其余测试可通过串口终端手动输入命令触发。

#### 4.2.1 CLI 命令列表

基础解码命令：

```text
h264_decode help
h264_decode vcdec_h264d
h264_decode vcdec_h264d 256x128
h264_decode vcdec_h264d_flexa
h264_decode vcdec_h264d_flexa 256x128
h264_decode_flexa start
```

直接寄存器 VCDEC 命令：

```text
vcdec_h264_driver frame
vcdec_h264_driver frame 256x128
vcdec_h264_driver flexa
vcdec_h264_driver flexa 256x128
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

`CMDRSP:OK` 仅表示 CLI 成功创建任务，不代表测试已经通过。不同路径的通过标志如下：

- `h264_decode vcdec_h264d` / `h264_decode vcdec_h264d_flexa`
  - 运行结束时可看到类似如下结果行：

```text
[RESULT][PASS] vcdec_h264_test success, decoded_aus=..., rounds=...
[RESULT][PASS] vcdec_h264_flexa_test success, decoded_aus=..., rounds=...
```

- `h264_decode_flexa start`
  - 无 `h264_decoder_init failed`、`h264_decoder_decode failed` 等错误日志
  - 运行结束时可看到 `exit h264_decode_flexa_test`

- `vcdec_h264_driver frame` / `vcdec_h264_driver flexa`
  - 运行过程中无 `vcdec_h264_* failed`、`unexpected output size`、`all decoded outputs sampled as zero` 等错误日志
  - 运行结束时可看到类似如下结果行：

```text
vcdec h264 frame test PASS: stream=1280x720 aus=... frame_cb=... flexa_cb=... last_wr=...
vcdec_h264_test_thread exit, mode=frame stream=1280x720 ret=0
```

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
vcdec_h264_decode_frame failed, ret=...
vcdec_h264_get_info failed, au=...
unexpected output size ... expect ...
all decoded outputs sampled as zero
failed to allocate ... buffer
psram_dma_stress_start failed, ret=...
```

#### 4.2.3 集成测试命令

`.it.csv` 当前覆盖两组内置 VCDEC H264 码流以及两种解码模式：

```text
ap_cmd h264_decode vcdec_h264d 1280x720
ap_cmd h264_decode vcdec_h264d_flexa 1280x720
ap_cmd h264_decode vcdec_h264d 256x128
ap_cmd h264_decode vcdec_h264d_flexa 256x128
```

期望结果分别匹配 `[RESULT][PASS] vcdec_h264_test success` 或 `[RESULT][PASS] vcdec_h264_flexa_test success` 日志。

## 5. 注意事项

1. 当前 `h264_decode` CLI 默认只启用 `vcdec_h264d` 和 `vcdec_h264d_flexa`；二者可额外携带 `1280x720` 或 `256x128` 码流参数。
2. `h264_decode_flexa start` 是独立 CLI 命令，用于传统 `h264_decoder_*` FLEXA 示例。
3. `h264_decode`、`h264_decode_flexa`、`vcdec_h264_driver`、`h264_decode_stress` 以及上电 boot demo 都通过独立线程运行测试，避免阻塞 CLI 线程。
4. `h264_decode_stress start [dma_copy_size_kb]` 中的可选参数单位为 KB。
5. `vcdec_h264_driver` 与 `h264_decode vcdec_h264d*` 路径都内置了 `1280x720` 和 `256x128` 两组码流；不带参数时默认使用 `1280x720`。
6. 压力测试会额外申请输出缓冲区、码流缓冲区以及 DMA 压测缓冲区，运行前请确认系统有足够内存。
7. `bk7259_ap` 默认配置已开启 `CONFIG_BK_DECODER=y`，因此首次上电时会自动输出一轮 VCDEC boot demo 日志。
8. 用于性能分析的 GPIO debug 脉冲逻辑已移除，正常运行不再通过 P30-P39 或 P34/P36 输出解码阶段标记。
