# JPEG解码示例工程

* [English](./README.md)

## 1. 项目概述

本工程演示 Beken 平台上的 JPEG 解码，包含两层 API：

| 层次 | 函数 / CLI | 说明 |
|------|-----------|------|
| 平台 JPEG | `jpeg_decode jpegd` / `jpegd_flexa` | 平台 JPEG 解码示例 |
| `bk_decoder` | `jpeg_decode vcdec_jpegd` / `vcdec_jpegd_frame_rgb` / `vcdec_jpegd_flexa` | 经 `bk_decoder` 的 JPEG 解码 |

使能 `CONFIG_BK_DECODER` 时，`main()` 自动执行开机自检：先 `vcdec_jpeg_flexa_test()`，再 `vcdec_jpeg_test()`。

* 开发者指南：

  - [JPEG 解码（VPU）](../../../developer-guide/vpu/jpeg_decode.html)
  - [Frame 与 Flexa 模式](../../../developer-guide/vpu/flexa_frame.html)

### 1.1 测试环境

- 硬件：**BK7259_QF128_12.3X12.3_V4.0**，PSRAM 32M
- 输入：内置 JPEG 测试图片
- 输出：解码日志；`bk_decoder` 路径打印 `[RESULT][PASS]` / `[RESULT][FAIL]`

.. warning::

    请使用参考外设。外设规格不同时需调整代码与配置。

## 2. 目录结构

```text
jpeg_decode_example/
├── ap/
│   ├── ap_main.c
│   └── jpeg_decode/
│       ├── include/
│       └── src/
│           ├── jpeg_decode_cli.c
│           └── vcdec_jpeg_test.c
├── cp/
├── partitions/
└── pj_config.mk
```

## 3. 功能说明

- `CONFIG_BK_DECODER=y` 时上电自动 `vcdec` 自检
- 单任务保护：同一时刻仅允许一个解码任务
- 裁剪/缩放用例带单帧解码耗时统计（`[RESULT][SPEED]`）

`CONFIG_BK_DECODER` 下可用用例：

| CLI 子命令 | 模式 | 用例内容 | 输出 |
|-----------|------|---------|------|
| `vcdec_jpegd` | frame | 整帧解码 | 176x144 NV12 |
| `vcdec_jpegd_flexa` | flexa | 分段解码 + CPU 拼帧 | 176x144 NV12 |
| `vcdec_jpegd_frame_rgb` | frame | PP 输出 RGB565 / RGB888 | 176x144 RGB |
| `vcdec_jpegd_frame_scale` | frame | 整帧缩放（原图 / 3/4 / 1/2 / 1/4） | 1920x1080 / 1440x810 / 960x540 / 480x270 |
| `vcdec_jpegd_frame_crop` | frame | 整帧裁剪（原图 / 3/4 / 1/2 / 1/4） | 1920x1080 / 1440x810 / 960x540 / 480x270 |
| `vcdec_jpegd_frame_scale_crop` | frame | 裁剪 3/4 × 缩放(1/1,1/2,1/4)、裁剪 1/2 × 缩放(1/1,3/4,1/4)、裁剪 1/4 × 缩放(1/1,3/4,1/2) | 裁剪区域 × 缩放比 |
| `vcdec_jpegd_flexa_crop` | flexa | 裁剪（原图 / 3/4 / 1/2 / 1/4） | 1920x1080 / 1440x810 / 960x540 / 480x270 |
| `jpegd_uvc_gpu_display [crop]` | flexa | UVC MJPEG → PP → GPU → LCD | 1080p / 1280x720 |

> `jpegd` / `jpegd_flexa`（平台 JPEG）为 legacy 用例，默认由 `USE_LEGACY_JPEGD 0` 关闭。

## 4. 编译与运行

### 4.1 编译

```bash
make bk7259 PROJECT=multimedia/jpeg_decode_example
```

### 4.2 CLI 命令

```text
jpeg_decode help
jpeg_decode vcdec_jpegd          # 需 CONFIG_BK_DECODER
jpeg_decode vcdec_jpegd_frame_rgb # 需 CONFIG_BK_DECODER
jpeg_decode vcdec_jpegd_flexa    # 需 CONFIG_BK_DECODER
jpeg_decode vcdec_jpegd_frame_scale      # 整帧缩放 + 速度
jpeg_decode vcdec_jpegd_frame_crop       # 整帧裁剪 + 速度
jpeg_decode vcdec_jpegd_frame_scale_crop # 整帧裁剪+缩放 + 速度
jpeg_decode vcdec_jpegd_flexa_crop       # flexa 裁剪 + 速度
jpeg_decode jpegd_uvc_gpu_display [crop]  # UVC MJPEG -> PP -> GPU -> LCD
jpeg_decode jpegd_uvc_gpu_display close           # 关闭显示链路
```

### 4.3 通过判定

`bk_decoder` 路径 — 搜索 `RESULT`：

```text
[RESULT][PASS] vcdec_jpeg_test success, rounds=5/5
[RESULT][PASS] vcdec_jpeg_frame_rgb_test success
[RESULT][PASS] vcdec_jpeg_flexa_test success, rounds=5/5
[RESULT][SPEED] vcdec_jpeg_frame_scale rounds=5 avg=xxxx us fps=xx.xx
```

`[RESULT][SPEED]` 为裁剪/缩放用例的耗时输出，格式见 4.5。

平台 `jpegd` 路径无统一 `[RESULT]` 行，通常以无 init/malloc 错误为通过。

### 4.4 集成测试（`.it.csv`）

```text
ap_cmd jpeg_decode vcdec_jpegd
ap_cmd jpeg_decode vcdec_jpegd_frame_rgb
ap_cmd jpeg_decode vcdec_jpegd_flexa
ap_cmd jpeg_decode vcdec_jpegd_frame_scale
ap_cmd jpeg_decode vcdec_jpegd_frame_crop
ap_cmd jpeg_decode vcdec_jpegd_frame_scale_crop
ap_cmd jpeg_decode vcdec_jpegd_flexa_crop
```

### 4.5 裁剪/缩放速度

以下用例在每轮解码前后用 AON RTC 计时（精度约 ±30us，只统计 `bk_jpeg_decode_frame()`，不含 buffer 清零），
用例结束后打印一行耗时汇总。输入统一为内置的 **1920x1080 JPEG（215105 字节，> 200KB，纹理复杂）**，
保证单帧解码耗时能被 32K AON RTC 分辨；`vcdec_jpegd_frame_rgb` 等小图用例仍使用 176x144 输入。

| CLI 子命令 | 模式 | PP 处理 | 输出 | 计时 |
|-----------|------|--------|------|------|
| `vcdec_jpegd_frame_scale` | frame | 原图 / 3/4 / 1/2 / 1/4 缩放 | 1920x1080 / 1440x810 / 960x540 / 480x270 | 是 |
| `vcdec_jpegd_frame_crop` | frame | 原图 / 3/4 / 1/2 / 1/4 裁剪 | 1920x1080 / 1440x810 / 960x540 / 480x270 | 是 |
| `vcdec_jpegd_frame_scale_crop` | frame | 裁剪 × 缩放组合（9 组） | 原图尺寸 × {1/1, 3/4, 1/2, 1/4} | 是 |
| `vcdec_jpegd_flexa_crop` | flexa | 原图 / 3/4 / 1/2 / 1/4 裁剪 | 1920x1080 / 1440x810 / 960x540 / 480x270 | 是 |

`full` 组不做 PP 缩放/裁剪，直接整帧解码（1920x1080），作为速度基线；其余三组按原图
3/4、1/2、1/4 分别设置。裁剪+缩放用例中裁剪比和缩放比都相对**原始输入**，输出尺寸 =
原图尺寸 × 缩放比（因此裁剪区域较大时会构成上采样）：裁剪 3/4 × 缩放 {1/1, 1/2, 1/4}、
裁剪 1/2 × 缩放 {1/1, 3/4, 1/4}、
裁剪 1/4 × 缩放 {1/1, 3/4, 1/2}，输出宽高向下取偶。其中「裁剪 1/4 × 缩放 1/1」需要 4 倍上采样，
超出 PP 3 倍缩放上限，运行时跳过；其余 frame 组合均会实际执行。缩放 4 组、裁剪 4 组、
裁剪+缩放 9 组，另加 flexa 裁剪 4 组，共 21 个子用例（实际执行 20 个），
每个子用例单独打印一行 `[RESULT][SPEED]`。

日志格式：

```text
[RESULT][SPEED] <case_name> rounds=<n> avg=<us> us fps=<xx.xx>
```

- `avg`：单帧平均解码耗时，单位 us
- `fps`：按 `avg` 折算的理论帧率
- `rounds`：统计轮数，固定为 `VCDEC_JPEG_DECODE_CNT`（5）；首轮包含解码器预热，均值偏大
- 每组子用例单独打印一行，`case_name` 区分裁剪/缩放组合

实测值随芯片版本、PSRAM 带宽与 cache 配置变化，以板端实际 log 为准。

## 5. 注意事项

1. `vcdec` 命令与开机自检依赖 `CONFIG_BK_DECODER`。
2. 开机自检结束后再手动触发新的 `vcdec` 命令，避免争用解码器实例。
3. RGB565/RGB888 输出格式转换仅由 `vcdec_jpegd_frame_rgb` 覆盖；flexa 模式不支持 RGB 输出。
4. flexa 模式仅支持 PP 裁剪。
5. 速度统计仅覆盖 `bk_jpeg_decode_frame()`，buffer 清零、buffer 申请与解码器 open 不计入。
