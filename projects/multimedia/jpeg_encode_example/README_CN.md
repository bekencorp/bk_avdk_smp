# JPEG 编码示例工程

* [English](./README.md)

## 1. 项目概述

本工程演示 Beken 平台 **JPEG 硬件编码**，基于 `bk_jpeg_encode_*` 控制器（VCENC JPEG，`bk_encoder`），覆盖 Frame 整帧模式、软件 Flexa 低延迟模式、OSD 叠加测试，以及 OSD 编码后解码上屏的完整链路。

* 开发者指南：

  - [JPEG 编码（VPU）](../../../developer-guide/vpu/jpeg_encode.html)
  - [Frame 与 Flexa 模式](../../../developer-guide/vpu/flexa_frame.html)

* API 参考：[JPEG 编码器 API](../../../api-reference/multimedia/bk_jpeg_encode.html)（如有）

当前工程提供：

- Frame 模式：`jpeg_encode frame`
- 软件 Flexa 模式：`jpeg_encode flexa`
- OSD 叠加测试：`jpeg_encode osd`（依次覆盖 frame、sw flexa、sw frame 三种模式，ARGB8888 / NV12 / Bitmap 共 8 个通道）
- 软件整帧 OSD 已包含在 `jpeg_encode osd` 与 `jpeg_encode osd_display` 中，无需单独命令
- OSD 编码 → 解码 → 上屏：`jpeg_encode osd_display [frame|flexa|sw_frame|all]`（可单模式上屏，OSD 会标明 HW FRAME / SW FLEXA / SW FRAME）
- `CONFIG_BK_ENCODER` 使能时的上电自动编码自检
- `.it.csv` 集成测试入口

### 1.1 测试环境

- 硬件：**BK7259_QF128_12.3X12.3_V4.0**，PSRAM 32M，接 HX8399C MIPI 屏（1080×1920，reset GPIO_60，背光 GPIO_7）
- 输入：内置 256×128 NV12 小图，测试时平铺扩展为 **1920×1080** NV12 缓冲
- 输出：JPEG 码流；每轮 **10 帧**，硬件编码（frame/flexa）质量档 **5**，软件整帧（sw_frame）质量 **75**

.. warning::

    请使用参考板卡进行学习。分辨率或内存配置不同时可能需要调整 `defconfig` 与测试参数。

## 2. 目录结构

```text
jpeg_encode_example/
├── .it.csv
├── ap/
│   ├── ap_main.c
│   └── jpeg_encode/
│       ├── common/jpeg_encode_nv12_256x128.*   # 内置测试 NV12
│       ├── include/jpeg_encode_test.h
│       ├── include/jpeg_encode_display.h       # 解码上屏测试接口
│       ├── include/jpeg_encode_osd.h           # OSD 测试接口
│       └── src/
│           ├── jpeg_encode_cli.c
│           ├── jpeg_encode_display.c           # LCD / GPU / JPEG 解码链路
│           ├── jpeg_encode_osd.c               # OSD 绘制与 8 通道下发
│           └── bk_jpeg_encode_test.c
├── cp/
└── partitions/
```

## 3. 功能说明

### 3.1 CLI 命令

```text
jpeg_encode help
jpeg_encode frame          # Frame 模式，bk_jpeg_encode_frame_new
jpeg_encode flexa          # 软件 Flexa，bk_jpeg_encode_sw_flexa_new
jpeg_encode osd            # 依次跑 frame / sw flexa / sw frame 的 8 通道 OSD 叠加
jpeg_encode osd_display            # 依次跑 frame / flexa / sw_frame 三种模式上屏
jpeg_encode osd_display frame      # 只跑硬件 frame 模式上屏（OSD 显示 HW FRAME）
jpeg_encode osd_display flexa      # 只跑软件 flexa 模式上屏（OSD 显示 SW FLEXA）
jpeg_encode osd_display sw_frame   # 只跑软件整帧模式上屏（OSD 显示 SW FRAME）
jpeg_encode osd_bench              # 软件整帧 OSD 每帧耗时对比（无 OSD 基线 + ARGB8888/BITMAP 各 4 档尺寸），默认 640x360
jpeg_encode osd_bench 1920x1080    # 同上，改用 1920x1080 输入（跑得更慢）
jpeg_encode osd_bench 1080p        # 1920x1080 的简写
```

- `CMDRSP:OK` 仅表示测试线程创建成功，最终结果看 `[RESULT]` 日志
- Flexa 模式按 16 行 block 推进输入，在 `flexa_done` 回调中通过 IOCTL 通知下一批行就绪
- OSD 模式依次运行 frame、sw flexa、sw frame 三条链路，每种模式在 `open` 后通过
  `bk_jpeg_encode_set_osd` 下发相同的 8 个通道，按 4 列 × 2 行排布：
  row0 为 ARGB8888 模式文本（HW FRAME / SW FLEXA / SW FRAME）、NV12 文本、Bitmap 文本、ARGB8888 文本；
  row1 为 4 路 ARGB8888 文本。
  frame/sw flexa 使用 NV12 输入，sw frame 使用 YUYV422 输入（由内置 NV12 小图转换并平铺）
- OSD 上屏按模式执行，每个模式 5 轮（`all` 时依次跑 frame、flexa、sw_frame 共 15 轮）：每轮把带 OSD 的 JPEG
  （输出到 512KB 缓冲）交给 JPEG flexa 解码器（HSRAM 环形缓冲，3 段 × 16 行），解码出的 NV12
  经 GPU 旋转 90°、压缩为 ARGB8888 后由 DPU 刷到 HX8399C 屏；最后一帧保留 3 秒便于观察
- 软件 OSD 合成方式由 `jpeg_encode_test.h` 中的 `JPEG_SW_OSD_IN_PLACE`（或 SDK 侧
  `bk_jpeg_encode_sw_frame_config_t::osd_in_place`）配置：
  - `1`（默认）：输入源可写，直接在输入 buffer 上叠加 OSD，无整帧拷贝，开销只有 OSD 区域本身；
  - `0`：输入源只读/需保留原图，先整帧拷贝到内部 staging buffer 再叠加，源图保持不变
- OSD 耗时对比（`jpeg_encode osd_bench [<w>x<h>|1080p]`）只在软件整帧链路上进行，使用独立的 YUYV 输入
  （与其他命令的 1920×1080 输入互不影响），默认 640×360，也可在命令行直接指定 1920×1080，
  换分辨率不需要重新编译；宽高都必须是 8 的倍数（BITMAP 的硬性要求）。
  每个档位跑 1 帧预热 + 4 帧计时，计时窗口是外层调用 `bk_jpeg_encode_frame()` 到返回的整个过程，
  dcache flush、OSD 合成（`copy` 模式下还包括整帧拷贝）和全部软件编码耗时都统计在内。
  OSD 用单个色块从 (0,0) 开始铺，档位按输入尺寸自动推导：每维 1/16、1/4、9/16、整帧（各维向下取 8 的倍数），
  每个尺寸分别用 **ARGB8888** 和 **BITMAP**（1bpp）两种格式各跑一遍，
  因此可以同时比较两种格式的合成耗时；两种格式覆盖的面积完全一致。
  最后输出每个档位相对无 OSD 基线的增量（ms 与百分比），
  并在 `copy`（整帧拷贝后叠加）与 `inplace`（就地叠加到输入 buffer）两种模式下各跑一遍做并排对比

#### 各模式的编码器与 OSD 处理位置

| 模式 | 编码器 | OSD 由谁处理 | 输入格式 | 说明 |
| --- | --- | --- | --- | --- |
| `frame`（`bk_jpeg_encode_frame_new`） | 硬件 JPEG 编码器，整帧提交 | 硬件编码器内部的 OSD 单元，`bk_jpeg_encode_set_osd` 直接配到 vcenc | NV12 | 编码与叠加都在硬件完成，CPU 只负责下发通道配置 |
| `flexa`（`bk_jpeg_encode_sw_flexa_new`） | 硬件 JPEG 编码器，按 16 行 slice 推进 | 同上，硬件编码器 OSD 单元 | NV12（按 flexa 对齐，1080 行需要 1088 行缓冲） | “sw flexa”里的 sw 只表示由软件按块喂数据、用 `SET_FLEXA_LINES_READY` 通知下一批就绪，编码和叠加仍由硬件完成 |
| `sw_frame`（`bk_jpeg_encode_sw_frame_new`） | 软件 tjpeg_enc，质量因子 75 | 软件合成（`jpeg_encode_osd.c`，16 行 HSRAM strip 按像素对处理，ARGB8888/BITMAP 共用） | YUYV422 | 唯一完全走 CPU 的路径，OSD 开销随 OSD 像素数线性增长，ARGB8888 约 0.13 µs/像素，见下方实测数据 |
| `hw flexa` | 硬件 JPEG 编码器 + ISP 直连 | 硬件 | ISP 输出 | 需要 ISP 参与（camera 直连编码），本示例不提供 |

三条 OSD 测试链路的差别只体现在 `sw_frame` 上：`frame` 和 `flexa` 的 OSD 都由硬件编码器完成，
CPU 侧不存在可比较的合成耗时，所以 `osd_bench` 只统计软件整帧这一条路径。

#### 软件整帧 OSD 耗时实测数据

1920×1080 输入（`jpeg_encode osd_bench 1080p`，1 帧预热 + 4 帧平均）：

| OSD 尺寸 | OSD 像素数 | 无 OSD 基线 | ARGB copy | BITMAP copy | ARGB inplace | BITMAP inplace |
| --- | --- | --- | --- | --- | --- | --- |
| 无 OSD | 0 | 2548.4 ms | — | — | — | — |
| 120×64（每维 1/16） | 7680 | 2548.4 ms | 2632.5 ms (+3.3%) | 2632.1 ms (+3.3%) | 2549.6 ms (+0.0%) | 2549.1 ms (+0.0%) |
| 480×264（每维 1/4） | 126720 | 2548.4 ms | 2648.1 ms (+3.9%) | 2644.7 ms (+3.8%) | 2565.0 ms (+0.7%) | 2561.8 ms (+0.5%) |
| 1080×600（每维 9/16） | 648000 | 2548.4 ms | 2715.1 ms (+6.5%) | 2697.5 ms (+5.8%) | 2632.4 ms (+3.3%) | 2615.0 ms (+2.6%) |
| 1920×1080（整帧） | 2073600 | 2548.4 ms | 2899.9 ms (+13.8%) | 2842.9 ms (+11.6%) | 2816.7 ms (+10.5%) | 2761.5 ms (+8.4%) |

640×360 输入（bench 默认输入）：

| OSD 尺寸 | OSD 像素数 | 无 OSD 基线 | ARGB copy | BITMAP copy | ARGB inplace | BITMAP inplace |
| --- | --- | --- | --- | --- | --- | --- |
| 无 OSD | 0 | 284.5 ms | — | — | — | — |
| 40×16（每维 1/16） | 640 | 284.5 ms | 293.8 ms (+3.3%) | 293.7 ms (+3.2%) | 284.5 ms (+0.0%) | 284.5 ms (+0.0%) |
| 160×88（每维 1/4） | 14080 | 284.5 ms | 295.2 ms (+3.8%) | 294.9 ms (+3.7%) | 286.1 ms (+0.6%) | 285.7 ms (+0.4%) |
| 360×200（每维 9/16） | 72000 | 284.5 ms | 303.5 ms (+6.7%) | 301.5 ms (+6.0%) | 294.4 ms (+3.5%) | 292.4 ms (+2.8%) |
| 640×360（整帧） | 230400 | 284.5 ms | 322.4 ms (+13.3%) | 316.2 ms (+11.1%) | 313.5 ms (+10.2%) | 307.2 ms (+8.0%) |

两组数据一致：`inplace` 的合成开销约 ARGB8888 **0.130 µs/像素**、BITMAP **0.100 µs/像素**，
与画面大小无关（都随 OSD 像素数严格线性增长），BITMAP 因为源数据只有 1bpp、且整对位全置时
不做任何混合运算，比 ARGB8888 快约 21%。`copy` 比 `inplace` 多出的固定成本就是一次整帧拷贝，
640×360 约 9.0 ms、1920×1080 约 82.8 ms，两种格式一致。

### 3.2 上电自检

使能 `CONFIG_BK_ENCODER` 时，`main()` 启动 `vcenc_jpeg_run_boot_demo()`，延时后依次运行 Frame 与 Flexa 自检。

## 4. 编译与运行

### 4.1 编译

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/jpeg_encode_example -j$(nproc)
```

### 4.2 预期结果

成功示例：

```text
[RESULT][PASS] bk_jpeg_encode_frame_test success, frames=10/10 encoded_size=...
[RESULT][PASS] bk_jpeg_encode_sw_flexa_test success, frames=10/10 encoded_size=...
[RESULT][PASS] bk_jpeg_encode_osd_frame_test success, frames=10/10 encoded_size=...
[RESULT][PASS] bk_jpeg_encode_osd_flexa_test success, frames=10/10 encoded_size=...
[RESULT][PASS] bk_jpeg_encode_osd_sw_frame_test success, frames=3/3 encoded_size=...
[RESULT][PASS] bk_jpeg_encode_osd_test success, frame=1 flexa=1 sw_frame=1
[RESULT][PASS] bk_jpeg_encode_osd_display_test success, mode=frame frames=5/5
[OSD-BENCH] mode=copy no_osd fmt=argb8888 osd=0x0 px=0 frames=4 avg=xxx.xms fps=x.x
[OSD-BENCH] mode=copy argb_360x200 fmt=argb8888 osd=360x200 px=72000 frames=4 avg=xxx.xms fps=x.x
[OSD-BENCH] overhead mode=copy argb_360x200 fmt=argb8888 vs no_osd: +x.xms (+x.x%)
[OSD-BENCH] mode=copy bitmap_360x200 fmt=bitmap osd=360x200 px=72000 frames=4 avg=xxx.xms fps=x.x
[OSD-BENCH] overhead mode=copy bitmap_360x200 fmt=bitmap vs no_osd: +x.xms (+x.x%)
[OSD-BENCH] mode=inplace argb_360x200 fmt=argb8888 osd=360x200 px=72000 frames=4 avg=xxx.xms fps=x.x
[OSD-BENCH] overhead mode=inplace argb_360x200 fmt=argb8888 vs no_osd: +x.xms (+x.x%)
[OSD-BENCH] mode=inplace bitmap_360x200 fmt=bitmap osd=360x200 px=72000 frames=4 avg=xxx.xms fps=x.x
[OSD-BENCH] overhead mode=inplace bitmap_360x200 fmt=bitmap vs no_osd: +x.xms (+x.x%)
[OSD-BENCH] ===== summary: sw_frame 640x360, average of one bk_jpeg_encode_frame() call =====
[OSD-BENCH] baseline no_osd: xxx.xms
[OSD-BENCH] argb_360x200 fmt=argb8888 px=72000 copy=xxx.xms(xxx.x% of base) inplace=xxx.xms(xxx.x% of base) copy-inplace=+x.xms
[OSD-BENCH] bitmap_360x200 fmt=bitmap px=72000 copy=xxx.xms(xxx.x% of base) inplace=xxx.xms(xxx.x% of base) copy-inplace=+x.xms
[RESULT][PASS] bk_jpeg_encode_osd_bench_test success, baseline=xxx.xms
```

失败示例：

```text
[RESULT][FAIL] bk_jpeg_encode_frame_test failed at encode_frame ret=... frames=...
```

### 4.3 集成测试

`.it.csv`：

```text
ap_cmd jpeg_encode frame
ap_cmd jpeg_encode osd
ap_cmd jpeg_encode osd_display
ap_cmd jpeg_encode osd_bench
```

Frame 用例期望匹配 `[RESULT][PASS] bk_jpeg_encode_frame_test success`，
OSD 用例依次执行 frame / sw flexa / sw frame，期望匹配汇总行
`[RESULT][PASS] bk_jpeg_encode_osd_test success`（任一链路失败则汇总为 FAIL）。
OSD 上屏用例需要板上接 HX8399C MIPI 屏，交替跑 frame 与 sw frame 各 5 轮，期望匹配
`[RESULT][PASS] bk_jpeg_encode_osd_display_test success`。
OSD 耗时用例期望匹配 `[RESULT][PASS] bk_jpeg_encode_osd_bench_test success`。

## 5. 配置与注意事项

| 参数 | 值 |
|------|-----|
| 编码分辨率 | 1920×1080 |
| 源图尺寸 | 256×128（平铺填充） |
| 帧数 / 质量 | Frame/Flexa 10 帧、SW Frame 3 帧 / 硬件档 5、软件 75 |
| OSD 通道 | 8 路（ARGB8888 / NV12 / Bitmap），覆盖 frame / sw flexa / sw frame |
| SW Frame 输入 | YUYV422（由内置 NV12 源转换） |
| 输入缓冲 | `MEM_SLAB_HEAP_UNCODED` |
| 输出缓冲 | `MEM_SLAB_HEAP_CODED` |
| 上屏链路 | JPEG flexa 解码 → GPU（NV12→ARGB8888，旋转 90°，压缩）→ DPU HX8399C 1080×1920 |
| 上屏轮数 / 保留时间 | 每模式 5 轮（all 共 15 轮），最后一帧保留 3 秒 |
| OSD 耗时对比 | sw frame 链路，每尺寸 1 帧预热 + 4 帧计时（无 OSD / 480×270 / 960×540 / 1440×810 / 1920×1080），copy 与 inplace 各一遍，用 `bk_aon_rtc_get_us()` 计时 |
| 软件 OSD 合成 | `JPEG_SW_OSD_IN_PLACE`=1 就地叠加（默认，无拷贝）；=0 整帧拷贝到 staging buffer 后叠加 |
| 上屏相关配置 | `CONFIG_DPU_DRIVER`、`CONFIG_DSI_DRIVER`、`CONFIG_BK_DISPLAY`、`CONFIG_MIPI_DSI`、`CONFIG_LCD_DSI`、`CONFIG_LCD_HX8399C_MIPI_1080x1920`、`CONFIG_VG_LITE_GPU`、`CONFIG_APP_GPU`、`CONFIG_BK_VCDEC`、`CONFIG_BK_DECODER` |

1. 上电 demo 与手动 CLI 共用编码硬件，建议等待上电自检完成后再手动测试。
2. 回调中避免阻塞操作，仅做轻量通知（信号量/标志位）。
3. 硬件 Flexa 路径（`bk_jpeg_encode_hw_flexa_new`）本工程未覆盖，可与 ISP 采集链路配合参考 VPU 文档。
4. `jpeg_encode osd_display` 依赖 HX8399C MIPI 屏及对应 GPIO（reset GPIO_60、背光 GPIO_7），未接屏时只会上报失败，不影响其它命令。
