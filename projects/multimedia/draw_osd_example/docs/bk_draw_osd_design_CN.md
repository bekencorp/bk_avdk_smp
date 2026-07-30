# bk_draw_osd 组件设计文档（GPU OSD）

> 平台：BK7259（Armino / bk_avdk_smp release 4.0.1）
> 组件：`ap/components/bk_draw_osd`
> 示例工程：`projects/multimedia/draw_osd_example`
> 关键词：VG-Lite GPU、SRC_OVER alpha 融合、ARGB8888、LVGL 字库、emWin/bk_font 字模、HV_SAMPLE 压缩

---

## 1. 背景与目标

早期 `bk_draw_osd` 用 CPU 逐像素（`image_scale` 的 `argb8888_to_*_blend`）把图标/文字混合到背景帧，随分辨率升高（1080x1920）CPU 开销大、也无法与已用 GPU 的显示/编码流水线自然协作。

本次重写目标：

1. **用 GPU（VG-Lite）做 ARGB8888 的 `SRC_OVER` alpha 融合**，替换 CPU 逐像素混合；
2. **统一 pipeline 提交模型**：OSD 只把合成好的 sprite 注册给**外部 pipeline GPU**（`bk_gpu_blit_set`，每帧 SRC_OVER 叠到视频上），不再自建 standalone VG-Lite 上下文。两条落地通路：
   - UVC 实时视频（板端解码 → GPU → DPU/MIPI，OSD 挂在 UVC display pipeline GPU 上）；
   - 真实 MIPI 摄像头视频（本地 GC2053 CSI + ISP + GPU，OSD 挂在 MIPI pipeline GPU 上）；
3. **分层 + 实例化**：拆成「公共薄 wrapper → 控制器（资产/显示列表/mutex/add-remove）→ 合成引擎（sprite 合成 + 提交外部 GPU）」三层；每个 `bk_draw_osd_new()` 返回**独立实例**，MIPI / UVC 各持一个、互不共享 static，可并发；
4. **字体两条并存**：LVGL 字库（`lv_font_t`，抗锯齿、可缩放）与 emWin/bk_font 字模（`gui_font_digit_struct`，预渲染 4bpp），经统一的 `draw_text(kind, ...)` 接口选择；
5. **不强开 `CONFIG_LVGL`**：把 LVGL 字库解码器最小化 vendored 进组件，只借它的字库格式；
6. 落实 P0/P1 优化：**多元素合成一张 sprite、一次 blit**，**sprite 按包围盒裁剪**。

---

## 2. 总体架构（三层 + pipeline 提交）

```
  project 侧                     bk_draw_osd 组件
 ┌───────────────┐   ┌───────────────────────────────────────────────┐
 │ osd_mipi_     │   │  bk_draw_osd.c   —— 公共薄 wrapper（校验+转发）  │
 │  overlay.c    │──▶│        │                                        │
 │ (ABGR8888,    │   │  bk_draw_osd_ctlr.c —— 控制器（__containerof,   │
 │  mipi GPU)    │   │        │              资产/显示列表/mutex/add-rm) │
 ├───────────────┤   │        ▼                                        │
 │ osd_uvc_      │──▶│  bk_osd_engine.c —— 合成引擎（实例化，零 static）：  │
 │  overlay.c    │   │     begin → put_icon/put_text → commit          │
 │ (ARGB8888,    │   │     CPU 取字/取图 → A8/ARGB → PSRAM sprite       │
 │  uvc GPU)     │   │     bk_osd_lv_font.c(LVGL 解码) / bkfont(emWin 解码)│
 └───────────────┘   └──────────────────┬────────────────────────────┘
                                         │ bk_gpu_blit_set(alpha_blend=1)
                                         ▼
                          外部 pipeline GPU（每帧 SRC_OVER 叠到视频）
```

### 2.1 pipeline 提交模型（单一模型，实例化）

VG-Lite 在本 SDK 里是**单例全局上下文**，归各自 pipeline 的 `bk_gpu` 控制器所有。OSD **不再**自建 VG-Lite 上下文，统一走 pipeline 提交模型：

- 每个 `bk_draw_osd_new(config)` 绑定一个外部 `config->gpu`（pipeline GPU handle），返回**独立实例**；
- 合成完调用 `commit`：`bk_gpu_blit_set(gpu, sprite, {alpha_blend=1, src_format, dst_x/y})` 把 sprite 注册给该 GPU，GPU 每帧对视频做一次 SRC_OVER；
- MIPI overlay 与 UVC overlay 各 `new` 一个实例（分别绑定 `mipi_pipeline_get_gpu_handle()` / `display_get_gpu_handle()`），**零模块级 static**，可并发、互不干扰；
- sprite 内存所有权在 `commit` 后转交 GPU，由引擎注册的 `free` 回调释放。

> 早期设计里"组件自持 VG-Lite 上下文的 MIPI 静态背景（旧 case1/case3）"已废弃移除——产品/双摄场景视频始终占用 GPU，唯一正确做法就是把 sprite 注册给已有 GPU 控制器。

---

## 3. 核心数据结构

均定义在 `ap/include/components/bk_draw_osd_types.h`（引擎私有类型在 `bk_draw_osd/include/bk_osd_engine.h`）。

- `osd_ctlr_config_t`：控制器配置。新增 pipeline 绑定字段：`gpu`（外部 pipeline GPU，必填）、`panel_w/panel_h`（旋转后显示坐标系）、`src_format`（sprite 源格式，MIPI=ABGR8888 / UVC=ARGB8888）；沿用 `blend_assets`（`{.addr=NULL}` 结尾）、`blend_info`（默认显示列表）、`draw_in_psram`（兼容字段，未使用）。资源指针**不深拷贝**，生命周期需 ≥ handle。
- `bk_blend_t`：单个 OSD 元素资源描述。`blend_type`（IMAGE/FONT）、`width/height`、`xpos/ypos`，`union{ blend_image_t image; blend_font_t font; }`。
- `blend_info_t`：`name` + `content` + 指向 `bk_blend_t` 的 `addr`，用于运行时按 name/content 检索。
- `osd_font_kind_t`：`OSD_FONT_LVGL`（`const lv_font_t*`）/ `OSD_FONT_BKFONT`（`const gui_font_digit_struct*`）。
- `bk_draw_osd_ctlr_t`：vtable（`draw_element/draw_text/draw_osd_array/clear/add_or_update/remove/ioctl/delete`）。三个渲染入口都**一次性自包含**（内部开 sprite/分槽/提交），begin/commit/set_slot 不再对外暴露。handle 反查用 `__containerof(handle, private_osd_ctlr_t, ops)`，`ops` 位置无关。

---

## 4. 合成引擎流水线（bk_osd_engine.c）

一次 OSD 更新的调用序：`begin → put_icon/put_text ... → commit`：

1. **begin**：`osd_engine_begin(w, h, dst_x, dst_y)` 从 PSRAM（`MEM_SLAB_HEAP_UNCODED`，`0x6000_0000`）分配一张 w×h 透明 ARGB8888 sprite（GPU 可直接访问）；内存紧张时按 `OSD_ENGINE_SHRINK_UNIT` 缩高重试（alloc-fit，即 P1-3 包围盒裁剪）；
2. **put_icon / put_text**：CPU 把图标像素/字模 A8 着色写进 sprite 的 (x,y)（越界裁剪）。字体两条路见 §5；
3. **commit**：`bk_gpu_blit_set(gpu, sprite, {src_format, dst_x/y, alpha_blend=1, free=cb, args=engine})` 一次性注册给绑定 GPU；成功后引擎放弃 sprite 所有权（转交 GPU，由 `free` 回调释放），失败则兜底释放；
4. **clear**：`osd_engine_clear()` → `bk_gpu_blit_clear(gpu)` 移除已注册 OSD，GPU 通过 `free` 回调释放旧 sprite。

> GPU 每帧只对 OSD 覆盖区域做 SRC_OVER（`blit` 只碰相交 tile），成本 ∝ OSD 面积，与整帧大小无关；且 **N 个元素合成到一张 sprite、只提交 1 次**（P0-2）。

---

## 5. 字体方案（两条并存）

两条路都经统一接口 `bk_draw_osd_text(handle, kind, font, utf8, x, y, argb, scale)` 选择，最终都在 `bk_osd_engine.c` 里把 A8 alpha 着色写进当前 sprite。

### 5.1 方案 A —— LVGL 字库（`kind = OSD_FONT_LVGL`）

"Font A"：**CPU 取字 → A8 alpha → 着色写进 sprite → 随 commit 一起 GPU SRC_OVER**（引擎内 `engine_put_lvgl`）。

- 输入 `const lv_font_t *`（如工程 assets 里的 `osd_font_unscii_8` / `osd_font_montserrat_28`）；
- `osd_font_get_glyph()` 取字形描述，`osd_font_glyph_a8()` 从位流解 alpha（`bk_osd_lv_font.c` 里为 `bpp=1/2/4/8` 连续位流写了快速路径）；
- 按 `argb`（`0x00RRGGBB`）着色，alpha 来自字模；`scale` 支持整数放大（1..8）便于小字库肉眼观察；
- 抗锯齿取决于字库 bpp：`unscii_8` 是 **1bpp（有锯齿）**，`montserrat_28` 是 **4bpp（抗锯齿）**。

### 5.2 方案 C —— emWin/bk_font 字模（`kind = OSD_FONT_BKFONT`）

`gui_font_digit_struct` 预渲染字模（FontCvt.exe 生成，`lcd_font.h`）：**CPU 取字模 → 展开 nibble 到 A8 → 着色写进 sprite → 同一次 GPU SRC_OVER**（引擎内 `engine_put_bkfont` / `bkfont_pixel_a8`）。

- 支持 4bpp（抗锯齿）与 1bpp；4bpp 时按半字节取 alpha；
- 固定尺寸预渲染，**不支持任意放大**（要更大字号需换更大的字库表）；
- 省去 LVGL 解码器，含中文（码点在表内即可显示）。

### 5.3 两种字库对比

| 维度 | LVGL `lv_font_t` | emWin `gui_font_digit_struct` |
|---|---|---|
| 生成工具 | `lv_font_conv`（可子集化，仅打包所需字符省内存） | FontCvt.exe |
| 抗锯齿 | 取决于 bpp（1/2/4/8），montserrat 为 4bpp | 常见 4bpp 抗锯齿 |
| 缩放 | 支持整数放大，也可换字号文件 | 固定尺寸，不可缩放 |
| 解码复杂度 | 需 vendored `fmt_txt` 解码器 | 简单直接（半字节展开） |
| 依赖 | 借 LVGL 字库格式（不开 `CONFIG_LVGL`） | SDK 自带 `lcd_font` |

> 对"时间/日期"这类数字，两者都行；要多字号/子集化/国际字符走 LVGL 更灵活，要极简省事走 bk_font。

---

## 6. LVGL 字库的最小化 vendoring（不开 CONFIG_LVGL）

为只借 LVGL 字库格式、不引入整套 LVGL：

- `include/bk_osd_lv_font.h`：最小类型定义（`lv_font_t`、`lv_font_fmt_txt_dsc_t`、kern/cache 等），补齐 `lvgl_v8` 抗锯齿字库（montserrat）需要的字段；
- `src/bk_osd_lv_font.c`：`fmt_txt` 解码器实现（`osd_font_get_glyph` / `osd_font_glyph_a8`），直接从 `lv_font_t->dsc` 解码，不提供 `lv_font_get_*` 全局符号；
- 字库 `.c`（如 `lv_font_montserrat_regular_48.c`）属于**工程资源**，由使用方放进自己的 `assets/`（不在组件内）。从 `lvgl_v8` 拷贝后：`#include "lvgl.h"` 改为 `#include "bk_osd_lv_font.h"`；`lv_font_t` 里 `.get_glyph_dsc` / `.get_glyph_bitmap` **置 `NULL`**（OSD 不走 LVGL 回调链）。同固件另链完整 LVGL 时，字库 `.c` 可保留标准 `lv_font_get_*` 指针。
- **本示例（`draw_osd_example`）当前不再内置任何 LVGL 字库资源**：case8 已收敛为纯 `blend_info`（bk_font + 图标）渲染，不含 LVGL 演示；但组件里的 LVGL 驱动（`bk_osd_lv_font.*` 解码器 + 引擎 LVGL 光栅路径 + `bk_draw_osd_text` 接口）**完整保留**，其它工程要用 LVGL 抗锯齿字库时，自带字库 `.c` 放进本工程 `assets/`、经 `bk_draw_osd_text(handle, OSD_FONT_LVGL, &lv_font_xxx, ...)` 调用即可。

---

## 7. 视频通路上的 OSD（pipeline 提交）与 P0/P1 优化

overlay 只是**薄封装**：持一个 `bk_draw_osd` 实例，把测试用的图标网格/文本排布喂给控制器，合成与提交全在组件里。以真实 MIPI 视频为例（`ap/mipi/src/osd_mipi.c`）：

- **P0-2 单 sprite 单次 blit**：每个 slot 内部先开一张透明 ARGB8888 sprite、逐个 `osd_engine_put_icon/put_text` 合成，再 `osd_engine_commit` **一次** `bk_gpu_blit_set()` 注册给 pipeline GPU（一簇一次提交，而非一元素一次）；
- **P1-3 包围盒裁剪**：`osd_engine_begin()` 按传入宽高分配、内存紧张时按 `OSD_ENGINE_SHRINK_UNIT` 逐步缩高重试（alloc-fit），不再整屏 1080×1920（8MB）；commit 用 `dst_x/dst_y` 定位；
- **颜色格式**：视频通路 GPU 输出为 BGRA 序，实例配置 `src_format = BK_PIXEL_FORMAT_ABGR8888` 修正红蓝互换（UVC 用 ARGB8888）；
- **生命周期**：sprite 在 GPU 交换帧时由引擎注册的 `osd_engine_free_cb` 回调释放；每次进入 case 先 `*_reset()`（`bk_draw_osd_delete` 旧实例 → 清旧 blit + 释放，再按当前 GPU 句柄 `new` 新实例），避免 stale handle 与 OOM；
- **双实例独立**：MIPI overlay 与 UVC overlay 各持一个实例、各绑自己的 GPU，切换通路无串扰，可为后期 MIPI+UVC 双摄提供一套 OSD；
- **纯 blend_info 渲染**：case8 现只调 `bk_draw_osd_array(osd, NULL)` 渲染默认动态列表（bk_font "12:35" + 图标），组件自动聚簇分槽，上层不碰 slot/begin/commit。原先并排的 LVGL montserrat_48 演示已从示例移除；组件的 LVGL 驱动（`bk_draw_osd_text` / `bk_osd_lv_font.*`）保留，需要时按 §6 引入工程字库资源即可。

---

## 8. 整帧融合 vs flexa 逐块融合（重要设计抉择）

### 8.1 现状：整帧末单次 blit

视频帧经 GPU（旋转 + NV12→ARGB8888 + HV 压缩）产出后，OSD 在 **frame_done** 对整帧做一次 `blit_rect`。因 `blit_rect` 只碰 OSD 覆盖的 tile，成本 ∝ OSD 面积。

### 8.2 per-block 逐块融合（可选模式，已实现）

除整帧末单次 blit 外，现已实现 **flexa 逐块融合**作为运行时可选模式（每实例属性 `gpu_vn_ctlr_t::osd_by_flexa`，CLI `ap_cmd osd <mipi|uvc> flexa`，经 `bk_gpu_ioctl(handle, BK_GPU_IOCTL_SET_OSD_BY_FLEXA, &en)` 设置，见 `bk_gpu_ctlr_default.c`）。开启后，OSD 在 `gpu_flex_process_line_block()` 里对**每个与 OSD 相交的 flexa block** 各做一次 `blit_rect`（`gpu_flex_osd_block_blit`：把整帧坐标平移到当前 block 的 `dst_buf` 局部坐标，vg_lite 自动裁到 block 范围），而不是等到 `frame_done` 才一次性叠整帧。

> **相交早退优化**：逐块路径按视频 `rotate_degree` 在切片轴上算出「本 block 区间 `[blk_lo, blk_hi)`」与「OSD 裁剪后 `dst_*/src_*` 覆盖区间」，`gpu_flex_osd_slot_hits_block()` 只用整型比较判是否相交。**不相交的 block 直接跳过**，连 `vg_lite_blit_rect`/`vg_lite_finish` 和 GPIO16 打点都不发——这样示波器上只有真正含 OSD 像素的那几个 block 才有 GPU 融合波形（早期未做此判断时每个 block 都会空跑一次 blit，波形看起来"每个 flexa 都有 OSD"）。判断在外层遍历（打点前置）和内层函数各有一处，共用同一 helper。

代价（下面几点仍然成立，是这条路的固有开销）：

- **省不了总工作量**：碰的 tile 一样多、blend 量一样，只是把 1 次 blit 拆成 N 次（每个相交块一次），多了 N 次提交/状态切换和逐块坐标切片（rotate90 竖条映射）开销；
- **跑在流式 GPU-mutex 路径里**：与视频 CSC 竞争同一把 `gpu_mutex`，OSD 过高/过重时可能把 GPU 拖到落后 ISP，触发 `flexa overrun`（区别于整帧模式的 `waits frame start`，见 §8.5）。

**换来的收益**：把「帧末单次大 blit」的耗时摊到整帧的多个 block 上，避免 GPU 在帧末长时间占用而错过下一帧起始同步窗口——因此能规避整帧模式在**大面积 / 多分散区域**时出现的 `flexa waits frame start` 丢帧（§8.5 有实测）。

### 8.3 人脸框为何用 flexa（对比佐证）

人脸框走 `gpu_draw_path_process()` 逐块画，是因为它**恰好适合逐块**：画的是**矢量描边**（`vg_lite_draw` stroke，只有边线几像素，逐块 ≈ 免费），坐标在**源图空间**、需跟视频一起 rotate，逐块 pass 里可白嫖已算好的 `draw_matrix` 与已打开的 `dst_buf`。这与 OSD"位图 blit、输出坐标、面积大"的负载性质相反——**不能作为 OSD 也该用 flexa 的理由**。

### 8.4 选型建议

| OSD 场景 | 推荐 |
|---|---|
| 少量小/半静态元素、且集中在一处（时间、WiFi、天气、日期一行） | **整帧单次 blit**（最优：简单、不抢 GPU），总面积 ≤ ~40k px |
| 多块分散（左上 + 右下 + 居中等 3+ 区域，各自小但合计大） | **per-block 逐块融合**（`osd <mipi\|uvc> flexa`），实测 0 丢帧（§8.5） |
| 铺满屏半透明大 OSD（单块面积就很大） | 两种都吃力；优先压面积（裁剪/降透明区域），必要时 per-block |
| 全屏高帧率动画 OSD | per-block（把成本摊开），需实测是否触发 `flexa overrun` |

**默认走整帧单次 blit**（延迟最低、最简单）；当单帧 OSD 总面积偏大或元素分散到多个角落导致 `flexa waits frame start` 时，切 **per-block**（`osd <mipi|uvc> flexa`，运行时开关，无需重编）。per-block 仍属实验/按需开启，注释清楚代价（§8.2）避免误用。

### 8.5 实测：整帧最大融合面积 & 两种模式对比

测试条件：1080x1920 竖屏，MIPI GC2053 1080p@30，视频 GPU 旋转 90° + NV12→ARGB + HV 压缩。面积口径以 `osd_engine_commit` 打印的 `OSD_COMMIT ... area=` 为准（自动包围盒裁剪后的**紧矩形**面积，即 GPU 真正 SRC_OVER 的像素数，与内容/画布无关）。

**整帧模式的最大安全融合面积**：

| 单帧 OSD 总面积（裁剪后） | 整帧模式表现 |
|---|---|
| ~31008 px（单区域） | 稳定不丢帧 |
| ~32800 px（2 区域：26752 + 6048） | 稳定 0 丢帧 |
| ~40–52k px | 进入 `flexa waits frame start` 平台，开始周期性丢帧 |
| ~59552 px（3 区域：26752×2 + 6048） | 持续 `flexa waits frame start`、明显掉帧 |

> 经验阈值：**整帧模式建议单帧 OSD 总面积 ≤ ~40k px（保守），~50k 为临界，> ~55k 明显掉帧**。注意这里是「一帧内所有区域裁剪后面积之和」，不是画布大小。

**per-block 模式**：同样 3 区域合计 ~59552 px，稳态 10s 内 `waits frame start` / `flexa overrun` 计数 = **0**，满帧不丢——把帧末单次大 blit 摊到各 block 后，越过了整帧模式的重同步窗口限制。

**两种丢帧机制（本质不同，别混淆）**：

- `flexa waits frame start`：GPU 在帧末做 OSD blit 占用太久，错过了下一帧起始的同步窗口 → 丢该帧。**整帧模式、OSD 面积大**时触发。
- `flexa overrun`：GPU 处理进度落后 ISP 超过 `flexa_buff_cnt`(=3) 行，未消费的行被 ISP 覆盖 → abort 当前帧。**per-block 下超高/超重 OSD** 或系统繁忙时才可能触发；也会在流水线冷启动首帧出现（warmup，可忽略）。

**模式对比总表**：

| 维度 | 整帧末单次 blit（默认） | per-block 逐块融合（可选） |
|---|---|---|
| 单帧最大安全面积 | ~40k px（保守），>~55k 掉帧 | 高得多（3 区域 ~59.5k 实测不丢） |
| 主要丢帧风险 | `flexa waits frame start`（面积大时） | `flexa overrun`（OSD 过重/过高时） |
| GPU 总工作量 | 1 次 blit | N 次 blit（相交块数），总 blend 量相同，多状态切换开销 |
| 延迟 | 帧末集中一次，端到端延迟略高 | 摊到整帧，无帧末集中占用 |
| 是否抢视频 GPU-mutex | 否（frame_done 里做） | 是（与视频 CSC 竞争 `gpu_mutex`） |
| 适用场景 | 少量、集中、面积小 | 多角落分散 / 单帧总面积大 |

### 8.6 多区域 multi-blit（同屏多角落）

`bk_gpu` 控制器的 blit 槽由单个升级为数组：`bk_gpu_blit_config_t.osd_slot` 选择槽位，控制器内 `update/display_blit_buffer/config[BK_GPU_BLIT_SLOT_MAX]`（`BK_GPU_BLIT_SLOT_MAX = 4`）。`gpu_blit_set` 按 `osd_slot` 写入对应槽，`frame_done` 与 per-block 路径都**遍历所有已注册槽位**依次 SRC_OVER。

- 每个区域 = **一张独立 sprite + 一个 slot**，成本 = **各区域裁剪后紧包围盒面积之和**，不含区域之间的空白；这正是「两角/多角」不必开一张跨越大半屏的大 sprite（那样会把空白也算进面积必掉帧）的关键。
- **上层用法（单入口 + 自动聚簇）**：`bk_draw_osd_array(handle, list)` 把一个 `blend_info[]` 数组按元素空间位置**自动聚簇**分到 ≤ `BK_GPU_BLIT_SLOT_MAX` 个 slot（`bk_draw_osd_ctlr.c` 的 `osd_draw_osd_array`：凝聚式合并，每次并「新增空白面积最小」的一对簇，靠近的元素并到同一 sprite、分居远处的各占一 slot；元素数超上限时就近合并、**不丢元素**）。用户无需接触 slot / 多 blit / begin / commit。渲染后内部 `next_slot` 游标停在已用簇之后。
- **追加单个元素**：`array` 之后可用一次性入口在剩余 slot 追加：`bk_draw_osd_element(handle, &info)`（有 `bk_blend_t` 资产的图/字，用元素自带坐标/颜色/内容）或 `bk_draw_osd_text(handle, kind, font, ...)`（裸字体文本，如 LVGL 时间——塞不进 bk_font 的 `blend_info`）。两者各自动占下一个空闲 slot，超上限返回 `AVDK_ERR_NOMEM`；`clear`（或下一次 `array`）复位游标。示例见 `osd_mipi.c`：仅 `bk_draw_osd_array(osd, NULL)` 渲染默认列表（`text1` bk_font "12:35" + 图标，自动分槽）。`bk_draw_osd_element` / `bk_draw_osd_text` 作为一次性追加入口保留在 API 中；示例已移除原先并排的 LVGL montserrat_48 演示，但组件 LVGL 驱动仍在，其它工程可自带 LVGL 字库经 `bk_draw_osd_text` 融合。
- 与 per-block 正交：multi-blit 决定「几块、各在哪」，per-block 决定「每块是帧末还是逐 block 融合」；两种模式都支持多槽。

---

## 9. 内存与性能

- **暂存/背景/sprite 均从 PSRAM（`MEM_SLAB_HEAP_UNCODED`）分配**，GPU 可直接访问，无需 `SOC_SRAM_PERI_ADDR` 转换；`vg_lite_allocate_with_data` 内部处理物理寻址；
- 图标 1080×1920 整帧背景约 4MB（RGB565），大图资源使 `primary_ap_app` 分区扩到 2048k；
- 单次字体光栅化 <1ms（故用微秒计时 + 200 次均值）；GPU blit 为异步提交 + `vg_lite_finish` 同步。

---

## 10. 已知限制与风险

1. OSD **不自建 VG-Lite 上下文**：`config->gpu` 必须是已 `vg_lite_init` 的 pipeline GPU handle（pipeline 未开时 `new` 会失败）；
2. sprite 颜色格式随通路：MIPI 用 `ABGR8888`（红蓝序修正），UVC 用 `ARGB8888`；
3. `begin → put* → commit` 序列不是跨调用原子的（单线程 CLI 下安全）；控制器 mutex 保护的是显示列表与 `draw_osd_array`/`add`/`remove`；
4. emWin 字模不可缩放；LVGL 字库需 vendored 解码器，字库 `.c` 回调指针置 `NULL`（见 §6）；
5. `blend_assets/blend_info` 必须 `{.addr=NULL}` 结尾且指针在 handle 生命周期内有效（不深拷贝）；
6. MIPI 摄像头冷启动依赖 1.8V VDDIO（`PM_AUXLDO_USER_DISPLAY`）与正确 `CONFIG_XTAL_FREQ=12000000`、复位时序。

---

## 附：关键源码索引

| 功能 | 文件 |
|---|---|
| 公共薄 wrapper（校验+转发） | `ap/components/bk_draw_osd/src/bk_draw_osd.c` |
| 控制器（__containerof/资产/显示列表/mutex/add-remove） | `bk_draw_osd/src/bk_draw_osd_ctlr.c`、`include/bk_draw_osd_ctlr.h` |
| 合成引擎（sprite 合成 + 字体光栅 + 提交外部 GPU） | `bk_draw_osd/src/bk_osd_engine.c`、`include/bk_osd_engine.h` |
| 公共 API / 类型 | `ap/include/components/bk_draw_osd.h`、`bk_draw_osd_types.h` |
| LVGL 字库解码器（组件驱动，保留） | `bk_draw_osd/src/bk_osd_lv_font.c`、`include/bk_osd_lv_font.h` |
| LVGL 字库资源（工程 assets） | 由使用方自带（示例已移除内置 demo 字库） |
| MIPI 视频 OSD（纯 blend_info 数组渲染、自动分槽） | `draw_osd_example/ap/mipi/src/osd_mipi.c` |
| UVC 视频 OSD | `draw_osd_example/ap/uvc/src/osd_uvc.c` |
| 共享显示层（LCD/DPU，MIPI/UVC 复用） | `draw_osd_example/ap/draw_osd/src/display.c`、`include/display.h` |
| UVC 整链（相机+MJPEG 解码+队列+bond，四合一） | `draw_osd_example/ap/uvc/src/uvc_pipeline.c`、`include/uvc_pipeline.h` |
| MIPI 摄像头 + pipeline | `draw_osd_example/ap/mipi/src/mipi_pipeline.c`（相机段已并入） |
| GPU 控制器 / flexa / draw_path | `ap/components/bk_gpu/src/bk_gpu_ctlr_default.c` |
| CLI | `draw_osd_example/ap/draw_osd/src/draw_osd_cli.c` |
