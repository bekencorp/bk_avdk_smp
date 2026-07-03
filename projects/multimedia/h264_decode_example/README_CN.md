# H264解码示例工程

* [English](./README.md)

## 1. 项目概述

本工程演示 Beken 平台上基于 **`bk_decoder` / VCDEC** 的 H264 解码流程。

当前支持路径：

| 路径 | CLI | 说明 |
|------|-----|------|
| VCDEC 帧模式 | `h264_decode vcdec_h264d [1280x720\|256x128]` | `bk_decoder` 整帧解码 |
| VCDEC FLEXA | `h264_decode vcdec_h264d_flexa [1280x720\|256x128]` | `bk_decoder` FLEXA 解码 |
| 直接寄存器 | `vcdec_h264_driver frame\|flexa [1280x720\|256x128]` | 底层 VCDEC 验证 |

使能 `CONFIG_BK_DECODER` 时，`main()` 自动执行 boot demo：先 `vcdec_h264_flexa_test(1280x720)`，再 `vcdec_h264_test(1280x720)`。默认 `defconfig` 已开启 `CONFIG_BK_DECODER`。

legacy 的 `h264_decode_flexa`、`h264_decode_stress` 已从活跃工程移除，源码归档于 `verisilicon_nano/legacy/projects/h264_decode_example/`（恢复方法见 `verisilicon_nano/legacy/README.md`）。

* 开发者指南：[H264软件解码概述](../../../developer-guide/video_codec/h264_decoding_sw.html)

### 1.1 测试环境

- 硬件：**BK7259_QF128_12.3X12.3_V4.0**，PSRAM 32M
- 输入：内置 H264 码流（`1280x720`、`256x128`）
- 输出：解码日志及 `[RESULT][PASS]` 结果行

.. warning::

    请使用参考外设。外设规格不同时需调整代码与配置。

## 2. 目录结构

```text
h264_decode_example/
├── ap/
│   ├── ap_main.c
│   └── h264_decode/
│       ├── common/          # Annex-B 解析 + 内置码流
│       └── src/
│           ├── h264_decode_cli.c
│           ├── vcdec_h264_driver_test.c
│           └── vcdec_h264_test.c   # VCDEC 示例与 boot demo
├── cp/
├── partitions/
└── .it.csv
```

## 3. 功能说明

- `h264_decode vcdec_h264d` / `vcdec_h264d_flexa`，可选码流参数
- `vcdec_h264_driver frame` / `flexa` 寄存器级测试
- 公共 Annex-B 解析模块，供 VCDEC case 复用
- `CONFIG_BK_DECODER=y` 时上电自动 boot demo

## 4. 编译与运行

### 4.1 编译

```bash
make bk7259 PROJECT=multimedia/h264_decode_example -j32
```

### 4.2 CLI 命令

```text
h264_decode help
h264_decode vcdec_h264d [1280x720|256x128]
h264_decode vcdec_h264d_flexa [1280x720|256x128]
vcdec_h264_driver frame [1280x720|256x128]
vcdec_h264_driver flexa [1280x720|256x128]
```

命令提交成功 / 失败：`CMDRSP:OK` / `CMDRSP:ERROR`。

### 4.3 通过判定

- VCDEC 路径：日志含 `[RESULT][PASS] vcdec_h264_test success` 或 `vcdec_h264_flexa_test success`
- 寄存器路径：以 `vcdec h264 frame test PASS` 等结尾，`ret=0`

### 4.4 集成测试（`.it.csv`）

```text
ap_cmd h264_decode vcdec_h264d 1280x720
ap_cmd h264_decode vcdec_h264d_flexa 1280x720
ap_cmd h264_decode vcdec_h264d 256x128
ap_cmd h264_decode vcdec_h264d_flexa 256x128
```

## 5. 注意事项

1. `h264_decode`、`vcdec_h264_driver` 在独立线程运行；`CMDRSP:OK` 仅表示任务已创建。
2. 未指定码流参数时默认 `1280x720`。
3. boot demo 与手动 CLI 共用解码硬件，建议等 boot demo 结束后再手动触发。
4. legacy 压测 / FLEXA 示例已归档，需要时按 `verisilicon_nano/legacy/README.md` 恢复。
