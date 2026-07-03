# H264解码示例工程

* [English](./README.md)

## 1. 项目概述

本工程演示 Beken 平台上基于 **`bk_decoder`** 的 H264 整帧解码流程，覆盖三种
`bk_h264_decode_ctlr` 控制器，并内置两条 1280x720 码流（无 B 帧的 1I30P 与含 B 帧的 IBBP）。

所有能力都通过唯一的 `h264_decode` CLI 暴露：

| 控制器 | CLI | 是否支持 B 帧 | 兼容码流 |
|--------|-----|--------------|----------|
| frame（整帧） | `h264_decode vcdec_h264d [1280x720_1i30p\|1280x720_ibbp]` | 否 | `1280x720_1i30p` |
| flexa（分段） | `h264_decode vcdec_h264d_flexa [1280x720_1i30p\|1280x720_ibbp]` | 否 | `1280x720_1i30p` |
| frame-zerocopy（零拷贝） | `h264_decode vcdec_h264d_frame_zerocopy [1280x720_1i30p\|1280x720_ibbp]` | 是 | `1280x720_1i30p` 与 `1280x720_ibbp` |

- 不带码流参数时默认使用 `1280x720_ibbp`；`1280x720` 为其别名。
- frame、flexa 为非 B 控制器，请仅配合 `1280x720_1i30p` 使用；frame-zerocopy 是唯一能解码
  B 帧（`1280x720_ibbp`）的控制器。
- 使能 `CONFIG_BK_DECODER`（默认 `defconfig` 已开启）时，`main()` 会启动上电自检 demo
  （`vcdec_h264_run_boot_demo()`），在独立线程中依次各跑一次 frame(1i30p)、flexa(1i30p)、
  frame-zerocopy(ibbp)。

* 开发者指南：[H264软件解码概述](../../../developer-guide/video_codec/h264_decoding_sw.html)

### 1.1 测试环境

- 硬件：**BK7259_QF128_12.3X12.3_V4.0**，PSRAM 32M
- 输入：内置 H264 码流（`1280x720_1i30p`、`1280x720_ibbp`）
- 输出：解码日志及 `[RESULT][PASS]` 结果行

.. warning::

    请使用参考外设。外设规格不同时需调整代码与配置。

## 2. 目录结构

```text
h264_decode_example/
├── ap/
│   ├── ap_main.c                                  # AP 主入口，注册 h264_decode CLI 并启动 boot demo
│   ├── config/bk7259_ap/                          # AP 侧 defconfig / GPIO 配置
│   └── h264_decode/
│       ├── common/
│       │   ├── h264_decode_h264_parser.c/.h       # 公共 Annex-B AU / 帧类型解析
│       │   ├── h264_decode_stream_1280x720.c/.h   # 1I30P 码流（无 B 帧）
│       │   └── h264_decode_stream_1280x720_ibbp.c # IBBP 码流（含 B 帧）
│       ├── include/
│       │   ├── h264_decode_test.h                 # CLI 入口 / 码流枚举声明
│       │   └── vcdec_h264_test_common.h           # 共享测试辅助声明
│       └── src/
│           ├── h264_decode_cli.c                  # h264_decode CLI 分发
│           ├── vcdec_h264_test_common.c           # 共享辅助函数 + 码流表 + 结果行
│           ├── vcdec_h264_frame_test.c            # frame 控制器测试（非 B）
│           ├── vcdec_h264_flexa_test.c            # flexa 控制器测试（非 B）
│           ├── vcdec_h264_frame_zerocopy_test.c   # 零拷贝 / B 帧控制器测试
│           └── vcdec_h264_boot_demo.c             # 上电自动运行 demo
├── cp/
├── partitions/
└── .it.csv
```

## 3. 功能说明

- 三个 `bk_decoder`（`bk_h264_decode_ctlr`）控制器，每个测试用例位于独立源文件：
  - `h264_decode vcdec_h264d [stream]` —— frame 控制器（整帧，**非 B**），`vcdec_h264_frame_test.c`
  - `h264_decode vcdec_h264d_flexa [stream]` —— flexa/分段控制器（**非 B**），`vcdec_h264_flexa_test.c`
  - `h264_decode vcdec_h264d_frame_zerocopy [stream]` —— 零拷贝 / **B 帧** 控制器，`vcdec_h264_frame_zerocopy_test.c`
- 两条 1280x720 码流同时编入固件（各自独立符号）：
  - `1280x720_1i30p` —— 1 IDR + 30 P，无 B 帧（符号 `h264_decode_stream_1280x720_1i30p[]`）
  - `1280x720_ibbp` —— Main profile 含 B 帧（符号 `h264_decode_stream_1280x720_ibbp[]`，`1280x720` 为其别名）
- 控制器 / 码流兼容性：frame、flexa 为非 B，仅可配合 `1280x720_1i30p`；frame-zerocopy 支持
  两条码流，且是唯一能解码 B 帧的控制器。
- 公共 H264 parser：解析 Annex-B access unit、帧类型与 frame table，三个测试文件通过
  `vcdec_h264_test_common.c` 复用。
- 上电自检 demo（`vcdec_h264_boot_demo.c` 中的 `vcdec_h264_run_boot_demo()`）：在独立线程中
  各执行一次 frame(1i30p)、flexa(1i30p)、frame-zerocopy(ibbp)。

## 4. 编译与运行

### 4.1 编译

```bash
make bk7259 PROJECT=multimedia/h264_decode_example -j32
```

### 4.2 CLI 命令

```text
h264_decode help
h264_decode vcdec_h264d 1280x720_1i30p
h264_decode vcdec_h264d_flexa 1280x720_1i30p
h264_decode vcdec_h264d_frame_zerocopy 1280x720_1i30p
h264_decode vcdec_h264d_frame_zerocopy 1280x720_ibbp
```

说明：frame / flexa 控制器为非 B，仅可使用 `1280x720_1i30p`；frame-zerocopy 控制器同时支持
`1280x720_1i30p` 与 `1280x720_ibbp`。不带码流参数时默认 `1280x720_ibbp`（`1280x720` 为其别名）。

命令提交成功 / 失败：`CMDRSP:OK` / `CMDRSP:ERROR`。

### 4.3 如何判断测试成功或失败

`CMDRSP:OK` 仅表示 CLI 成功创建了测试线程，不代表测试已经通过。运行结束时按控制器查看对应结果行：

```text
[RESULT][PASS] vcdec_h264_test success, decoded_aus=..., rounds=...                 # vcdec_h264d (frame)
[RESULT][PASS] vcdec_h264_flexa_test success, decoded_aus=..., rounds=...           # vcdec_h264d_flexa
[RESULT][PASS] vcdec_h264_frame_zerocopy_test success, decoded_aus=..., rounds=...  # vcdec_h264d_frame_zerocopy
```

失败时对应打印 `[RESULT][FAIL] <case> failed at <stage>, ret=...`。

## 5. 注意事项

1. `h264_decode` CLI 提供 `vcdec_h264d`、`vcdec_h264d_flexa` 和 `vcdec_h264d_frame_zerocopy`，
   均可携带码流参数（`1280x720_1i30p` 或 `1280x720_ibbp`）。
2. 控制器 / 码流兼容性：frame、flexa 为非 B，仅可解码 `1280x720_1i30p`；frame-zerocopy 支持
   两条码流，且是唯一能解码 B 帧 `1280x720_ibbp` 的控制器。
3. 所有测试用例与上电 boot demo 都在独立线程运行，避免阻塞 CLI 线程；boot demo 与手动 CLI
   共用解码硬件，建议等 boot demo 结束后再手动触发。
4. 不带码流参数时默认使用 `1280x720_ibbp`；`1280x720` 为其别名。
5. `bk7259_ap` 默认配置已开启 `CONFIG_BK_DECODER=y`、`CONFIG_FRAME_BUFFER=y`。
6. 两条 1280x720 码流同时编入 flash，各自独立符号（`h264_decode_stream_1280x720_1i30p[]` 与
   `h264_decode_stream_1280x720_ibbp[]`），运行时按码流 id / CLI 参数选择，空间充足。
7. 旧版本的 `vcdec_h264_driver`（寄存器级）、`h264_decode_flexa`、`h264_decode_stress` 及
   `256x128` 码流已不在本工程中；legacy 源码归档于
   `ap/properties/modules/verisilicon_nano/legacy/projects/h264_decode_example/`。
