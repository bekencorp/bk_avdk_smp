# PP 后处理示例工程

* [English](./README.md)

## 1. 项目概述

本工程演示 Beken 平台上 **PP（Post-Processor，后处理）模块**的 **memory-in / memory-out**
用法：把一块外部 NV12 送入 PP 硬件，验证其**缩放**与**格式转换**能力
（NV12 → NV12 / RGB565 / RGB888）。

所有能力都通过唯一的 `pp` CLI 暴露，共 8 个用例；每个用例默认连续执行 **3 轮**
（`PP_EXAMPLE_ROUNDS`），全部成功才算通过。

PP 与 H.264 / JPEG **解码器共用同一块 VCDec 硬件**，但本工程测的是 **PP 模块本身**，
不是解码流程，两者区别如下：

| 对比项 | 解码示例（如 `h264_decode_example`） | 本工程 `pp_example` |
| --- | --- | --- |
| 测试对象 | 解码器（PP 作为解码附属路径） | **PP 模块** |
| PP 触发方式 | 解码时自动走 PP pipeline | 调用 `bk_pp_process()` 独立触发 |
| 输入来源 | 码流经解码核产生 | 外部 NV12 buffer（本工程用 H.264 解码生成） |
| 典型 API | `bk_h264_decode_frame()` + `out_width/out_format` | `bk_pp_process()` + `in/out_width/format` |
| 适用场景 | 验证「解码 + 缩放 / 转格式」端到端 | 验证已有 NV12 帧的 PP 处理能力 |

本工程中的 `bk_h264_decode` **只负责生成 1280×720 测试用 NV12**，不参与 PP 被测逻辑。
实际产品中，NV12 可来自解码器、摄像头 ISP、共享内存等任意来源，只要满足 PP 总线寻址要求即可。
PP 模块与 H.264 解码模块共用同一套 VCDec 硬件，因此二者**不能同时使用**。

### 1.1 测试环境

- 硬件：**BK7259_QF128_12.3X12.3_V4.0**，PSRAM 32M，HX8399C 1080×1920 MIPI 屏
- 输入：内置 1280×720 H.264（1I30P）码流，解码首帧得到测试用 NV12
- 输出：PP 处理日志、`[RESULT][PASS]` 结果行；使能显示时刷新到 MIPI 屏

.. warning::

    请使用参考外设。外设规格不同时需调整代码与配置。

## 2. 目录结构

```text
pp_example/
├── ap/
│   ├── ap_main.c                    # AP 主入口，注册 pp CLI 并启动 boot demo
│   ├── config/bk7259_ap/            # AP 侧 defconfig / GPIO 配置
│   ├── Kconfig.projbuild            # 工程配置（PP_EXAMPLE_ENABLE_MIPI_DISPLAY）
│   └── pp/
│       ├── include/
│       │   ├── pp_config.h          # 显示分辨率 / 旋转角度等编译期配置
│       │   ├── pp_test.h            # 用例入口 / 源尺寸声明
│       │   ├── pp_dpu.h             # DPU 刷屏封装声明
│       │   └── pp_gpu_blit.h        # GPU strip blit 封装声明
│       └── src/
│           ├── pp_cli.c             # pp CLI 分发
│           ├── pp_test.c            # PP 用例主体（解码备源 + bk_pp_process + 显示）
│           ├── pp_dpu.c             # DPU 解压刷 MIPI 屏
│           └── pp_gpu_blit.c        # GPU 缩放 / 旋转
├── cp/
├── partitions/
└── .it.csv
```

## 3. 功能说明

### 3.1 数据通路

1. 解码内置 **1280×720 H.264（1I30P）** 码流首帧，得到 NV12（`bk_h264_decode`，仅备源）。
2. 将该 NV12 buffer 送入 PP 控制器，完成缩放和 / 或转成 RGB565 / RGB888。
3. 使能显示时，PP 输出再经 GPU 缩放 / 旋转、DPU 解压刷新到 MIPI 屏（见 [3.3](#33-显示通路)）。

对外接口（`components/bk_decode/bk_pp_ctlr.h`，与 `h264d` 一致的 handle 式控制器）：

- 生命周期：`bk_pp_ctlr_new()` → `bk_pp_init()` → `bk_pp_open()` →
  `bk_pp_process()`（可重复调用）→ `bk_pp_close()` → `bk_pp_deinit()` → `bk_pp_delete()`。
- `bk_pp_ctlr_new()` 传入 `bk_pp_config_t`（`timeout_ms` / 回调）；`bk_pp_process()` 传入
  `bk_pp_process_req_t`（输入 / 输出 buffer 与尺寸，`out_width/out_height` 填 0 表示与输入同尺寸）。
- 应用层控制器负责 H26D 上电与 PP 中断，底层通过 vcdec 的 `vcdec_pp_process()` 完成实际处理，
  应用侧不直接读写 PP 寄存器。

### 3.2 测试用例

| 用例（CLI 子命令） | 输入 | 输出 | 说明 |
| --- | --- | --- | --- |
| `nv12_rgb565` | 1280×720 NV12 | 1280×720 RGB565 | 同分辨率格式转换 |
| `nv12_rgb888` | 1280×720 NV12 | 1280×720 RGB888 | 同分辨率格式转换 |
| `nv12_scale_down` | 1280×720 NV12 | 640×360 NV12 | 纯缩放下采样（1/2） |
| `nv12_scale_up` | 1280×720 NV12 | 1920×1080 NV12 | 纯缩放上采样（1.5×） |
| `nv12_rgb565_down` | 1280×720 NV12 | 640×360 RGB565 | 缩放 + 格式转换 |
| `nv12_rgb888_down` | 1280×720 NV12 | 640×360 RGB888 | 缩放 + 格式转换 |
| `nv12_rgb565_up` | 1280×720 NV12 | 1920×1080 RGB565 | 缩放 + 格式转换 |
| `nv12_rgb888_up` | 1280×720 NV12 | 1920×1080 RGB888 | 缩放 + 格式转换 |

### 3.3 显示通路

PP 输出（NV12 / RGB565 / RGB888）经 GPU strip blit 缩放到 1088×1920 并旋转 90°，
再经 DPU 解压刷新到 HX8399C MIPI 面板，显示路径与 `h264d_gpu_display_example` 的
`start_dec_scale_cvt` 一致。可通过 `menuconfig` 关闭 `PP_EXAMPLE_ENABLE_MIPI_DISPLAY`
（`CONFIG_PP_EXAMPLE_ENABLE_MIPI_DISPLAY`），此时仅跑 PP 处理、不做显示。

## 4. 编译与运行

### 4.1 编译

```bash
CCACHE_DISABLE=1 make bk7259 PROJECT=multimedia/pp_example -j32
```

### 4.2 CLI 命令

在 AP 命令行输入 `pp <用例名>`：

```text
pp help
pp nv12_rgb565
pp nv12_rgb888
pp nv12_scale_down
pp nv12_scale_up
pp nv12_rgb565_down
pp nv12_rgb888_down
pp nv12_rgb565_up
pp nv12_rgb888_up
```

命令提交成功 / 失败：`CMDRSP:OK` / `CMDRSP:ERROR`。

开机 1s 后（使能 `CONFIG_BK_DECODER` 时）自动跑一次 `nv12_rgb565_down` boot demo，
PP 转换完成后经 GPU / DPU 刷新到 MIPI 屏；boot demo 与手动 CLI 共用 PP 硬件，
建议等 boot demo 结束后再手动触发。

### 4.3 如何判断测试成功或失败

`CMDRSP:OK` 仅表示 CLI 成功创建了测试线程，不代表测试已经通过。运行结束时查看结果行
（日志中的用例名带 `pp_` 前缀）：

```text
[RESULT][PASS] pp_<case> success, rounds=3/3      # 例如 pp_nv12_rgb565_down
```

失败时对应打印 `[RESULT][FAIL] pp_<case> failed at <stage>, ret=...`，`<stage>` 可能为
`decode_nv12`、`alloc_out_buf`、`pp_process`、`display` 等。

## 5. 注意事项

1. 所有用例与 boot demo 均在独立线程运行，避免阻塞 CLI 线程；同一时刻只允许一个 PP 测试线程。
2. 每个用例默认连续跑 3 轮（`PP_EXAMPLE_ROUNDS`），全部成功才判 PASS。
3. RGB 输出时行高按 16 对齐、NV12 按 2 对齐，输出 buffer 大小据此计算，勿按裸尺寸估算。
4. 所有 buffer 须位于 PP 总线可访问区（PSRAM，使用 `bk_frame_buffer_malloc` 申请）；
   PP 通过 `HWIF_PP_E` 与 `HWIF_PP_IN_*` 从内存读取 NV12，并写回缩放 / 转换结果。
5. `bk7259_ap` 默认配置已开启 `CONFIG_BK_DECODER=y`、`CONFIG_FRAME_BUFFER=y`
   及显示相关的 `CONFIG_DPU_DRIVER` / `CONFIG_MIPI_DSI` / `CONFIG_VG_LITE_GPU` 等。
6. 关闭 `CONFIG_PP_EXAMPLE_ENABLE_MIPI_DISPLAY` 后可在无屏环境仅验证 PP 处理逻辑。
7. `bk_h264_decode` 仅用于生成测试用 NV12，不属于 PP 被测路径；实际产品可替换为任意
   满足 PP 总线寻址要求的 NV12 来源。
