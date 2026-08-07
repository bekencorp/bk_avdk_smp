# bk_draw_osd 组件 API 使用文档（GPU OSD）

> 平台：BK7259（Armino / bk_avdk_smp release 4.0.1）
> 组件：`ap/components/bk_draw_osd`　公共头：`components/bk_draw_osd.h` + `components/bk_draw_osd_types.h`
> 参考示例：`projects/multimedia/draw_osd_example`（本文所有代码片段均可在此工程找到对照）
> 配套设计文档：同目录 `bk_draw_osd_design_CN.md`

本文**以 API 为主线**，讲清楚「一个 OSD 从建到销的完整调用流程」，示例工程只作对照引用。若你要把 OSD 搬进自己的工程（如 `doorbell_lp` 状态栏），照第 3、4 章的流程调 API 即可。

---

## 1. 组件模型（先建立心智模型）

`bk_draw_osd` 是 **pipeline 提交模型**：它**不自建 GPU / VG-Lite 上下文**，而是把合成好的透明 sprite 注册给一个**已经打开的视频链路 GPU**，由该 GPU 每帧对视频做 `SRC_OVER` alpha 融合。

```
你的视频 pipeline（UVC / MIPI / 播放器 …）
        │  已 vg_lite_init 的 GPU handle
        ▼
   bk_draw_osd 实例  ──合成透明 sprite──▶ 注册给该 GPU
        ▲                                     │
   blend_info[] 资产                          ▼
   （图标 + 文字）                    GPU 每帧 SRC_OVER 叠加输出
```

由此得到三条**贯穿全文的前提**：

1. **必须先有一个已打开的视频 pipeline**，能拿到它的 `bk_gpu_ctlr_handle_t`（已 `vg_lite_init`）。OSD 只是"挂"在上面。
2. **一个 `bk_draw_osd_new()` = 一个独立实例**，绑定一个 GPU。UVC / MIPI 各持一个实例，可并发、互不干扰。
3. **对外只有 3 个渲染入口**（`array` / `element` / `text`），全部一次性自包含——内部自动开 sprite、分槽、提交，**用户不碰 begin/commit/slot**。

---

## 2. API 全景

头文件 `components/bk_draw_osd.h`。按「生命周期」分组：

| 阶段 | API | 作用 |
|---|---|---|
| **建** | `bk_draw_osd_new(&h, &cfg)` | 创建实例，绑定 `cfg.gpu`，读入默认列表 `cfg.blend_info` |
| **渲染** | `bk_draw_osd_array(h, list)` | **主入口**：渲染 `blend_info[]`（`NULL`=默认列表），组件自动聚簇分槽 |
| **渲染** | `bk_draw_osd_element(h, &info)` | 一次性渲染**单个元素**（图/字通吃），用元素自带 `xpos/ypos/color/content` |
| **渲染** | `bk_draw_osd_text(h, kind, font, utf8, x, y, argb, scale)` | 一次性渲染**一段裸字体文本**（无 `bk_blend_t` 时用，如 LVGL 字库） |
| **更新** | `bk_draw_osd_add_or_update(h, name, content)` | 运行时增改动态列表某元素（**改完需再 `array(NULL)` 生效**） |
| **更新** | `bk_draw_osd_remove(h, name)` | 运行时从动态列表移除元素（同样需再 `array(NULL)`） |
| **清理** | `bk_draw_osd_clear(h)` | 清掉已注册 blit（保留视频）、复位 slot 游标 |
| **销毁** | `bk_draw_osd_delete(h)` | 清 blit + 释放 sprite + 销毁实例 |
| **查询** | `bk_draw_osd_ioctl(h, cmd, p1, p2, p3)` | 查资产表 / 当前列表（`OSD_CTLR_CMD_GET_ALL_ASSETS` / `GET_DRAW_INFO`） |

所有 API 返回 `avdk_err_t`（`AVDK_ERR_OK` 为成功）。

### 2.1 精确签名（来自 `bk_draw_osd.h`）

```c
avdk_err_t bk_draw_osd_new(bk_draw_osd_ctlr_handle_t *handle, osd_ctlr_config_t *config);
avdk_err_t bk_draw_osd_array(bk_draw_osd_ctlr_handle_t handle, const blend_info_t *list);
avdk_err_t bk_draw_osd_element(bk_draw_osd_ctlr_handle_t handle, const blend_info_t *info);
avdk_err_t bk_draw_osd_text(bk_draw_osd_ctlr_handle_t handle, osd_font_kind_t kind,
                            const void *font, const char *utf8,
                            uint16_t x, uint16_t y, uint32_t argb, uint8_t scale);
avdk_err_t bk_draw_osd_add_or_update(bk_draw_osd_ctlr_handle_t handle, const char *name, const char *content);
avdk_err_t bk_draw_osd_remove(bk_draw_osd_ctlr_handle_t handle, const char *name);
avdk_err_t bk_draw_osd_clear(bk_draw_osd_ctlr_handle_t handle);
avdk_err_t bk_draw_osd_delete(bk_draw_osd_ctlr_handle_t handle);
avdk_err_t bk_draw_osd_ioctl(bk_draw_osd_ctlr_handle_t handle, uint32_t cmd, uint32_t p1, uint32_t p2, uint32_t p3);
```

### 2.2 关键数据结构（来自 `bk_draw_osd_types.h`）

```c
/* new 时的配置 */
typedef struct {
    bk_gpu_ctlr_handle_t gpu;         // 必填：已 vg_lite_init 的 pipeline GPU handle
    uint16_t panel_w, panel_h;        // 目标显示坐标系（旋转后 buffer）的宽高，如 MIPI 竖屏 1080x1920
    bk_pixel_format_t src_format;     // sprite 源格式：MIPI=ABGR8888 / UVC=ARGB8888（补偿上游通道序）
    const blend_info_t *blend_assets; // 可选，资产总表（add_or_update 按名查找），{.addr=NULL} 结尾
    const blend_info_t *blend_info;   // 可选，new 时读入的默认显示列表（array(NULL) 渲染它）
    bool draw_in_psram;               // 兼容字段，pipeline 模型未用
} osd_ctlr_config_t;

/* 单个 OSD 元素资源 */
typedef struct {
    uint8_t version;
    blend_type_t blend_type;          // BLEND_TYPE_IMAGE / BLEND_TYPE_FONT
    const char name[20];
    uint32_t width, height;           // IMAGE: 位图宽高(必填); FONT: sprite 框, 0=按文字自动测量
    uint32_t bg_width, bg_height;
    uint16_t xpos, ypos;              // 元素左上角（显示坐标系）
    union {
        blend_image_t image;          // { uint8_t format=ARGB8888; uint32_t data_len; const uint8_t *data; }
        blend_font_t  font;           // { const gui_font_digit_struct *font_digit_type; uint32_t color; }
    };
} bk_blend_t;

/* 显示列表 / 资产表的一项（数组以 {.addr=NULL} 结尾） */
typedef struct {
    char name[20];                    // 检索名（"wifi"/"clock"…）
    const bk_blend_t *addr;           // 指向资源
    char content[31];                 // 文本内容（"12:34"），图标可留空
} blend_info_t;

/* 文本字库类型（bk_draw_osd_text 的 kind 参数） */
typedef enum { OSD_FONT_LVGL = 0, OSD_FONT_BKFONT } osd_font_kind_t;
```

---

## 3. 标准使用流程（4 步）

这是把组件用起来的**通用套路**，与具体工程无关。示例工程的 MIPI 实现见 `ap/mipi/src/osd_mipi.c`，UVC 见 `ap/uvc/src/osd_uvc.c`。

### 第 1 步：准备 `blend_info[]` 资产数组

在工程 `assets/` 里定义每个元素的 `bk_blend_t`，再用 `blend_info[]` 串成显示列表（**必须 `{.addr=NULL}` 结尾**）。对照示例：`assets/bk_img.c`（图标）、`assets/bk_font.c`（字模）、`assets/blend_dsc.c`（数组）。

```c
/* 图标：ARGB8888 字节流（UI 工具导出）*/
const bk_blend_t img_wifi = {
    .blend_type = BLEND_TYPE_IMAGE,
    .name  = "wifi",
    .width = 48, .height = 48,        // 图标必填：位图实际尺寸
    .xpos  = 8,  .ypos  = 4,
    .image = { .format = ARGB8888, .data = wifi_argb8888, .data_len = 48*48*4 },
};

/* 文本：emWin/bk_font 字模；color 是该字着色 */
extern const gui_font_digit_struct *const font_digit_black24;   // assets/bk_font.c
const bk_blend_t font_clock = {
    .blend_type = BLEND_TYPE_FONT,
    .name  = "clock",
    .width = 0, .height = 0,          // 0 = 按 content 自动测量宽高（推荐，避免手填导致截断）
    .xpos  = 64,  .ypos  = 8,
    .font  = { .font_digit_type = font_digit_black24, .color = 0xFFFFFF },
};

/* 显示列表：控制“显示哪些、什么内容、在哪” */
const blend_info_t blend_info[] = {
    { .name = "wifi",  .addr = &img_wifi,   .content = "" },       // 图标 content 可空
    { .name = "clock", .addr = &font_clock, .content = "12:34" },  // 文本用 content 覆盖
    { .addr = NULL },                                              // 结尾哨兵（必须）
};
```

- **图标**：`BLEND_TYPE_IMAGE`，`image.data` 是 ARGB8888 字节流，`content` 忽略。
- **文本**：`BLEND_TYPE_FONT`，`font.font_digit_type` 指字模表；`content` 非空显示 `content`，否则回退 `name`；`font.color` 为着色。
- **坐标**：各元素用自身 `xpos/ypos`。图标宽高必填(位图尺寸)；文本 `width/height` 可填 0，由组件按字库+content 自动测量，非 0 则作为固定框/裁剪上限。
- **生命周期**：数组、`bk_blend_t`、图标/字模数据**均不深拷贝**，指针须在 OSD 实例存活期一直有效（一般定义为 `const` 全局）。

### 第 2 步：创建实例（绑定 pipeline GPU）

先确保视频链路已开、GPU 已 `vg_lite_init`，取到它的 handle，再 `new`。对照示例 `osd_mipi.c` 的 `mipi_osd_create()`。

```c
static bk_draw_osd_ctlr_handle_t s_osd = NULL;

/* 取本工程 pipeline GPU handle：
 *   示例 MIPI → mipi_pipeline_get_gpu_handle()
 *   示例 UVC  → display_get_gpu_handle()
 *   你的工程 → 显示/编码 pipeline 暴露的 get_gpu_handle() */
bk_gpu_ctlr_handle_t gpu = mipi_pipeline_get_gpu_handle();
if (gpu == NULL) return;   // pipeline 未开，GPU 还没初始化

osd_ctlr_config_t cfg = {0};
cfg.gpu          = gpu;
cfg.panel_w      = 1080;                      // 旋转后显示坐标系（背景 buffer）宽
cfg.panel_h      = 1920;                      // 高
cfg.src_format   = BK_PIXEL_FORMAT_ABGR8888;  // MIPI 用 ABGR8888（否则红蓝反）；UVC 用 ARGB8888
cfg.blend_assets = blend_assets;              // 资产总表：add_or_update 按名查找
cfg.blend_info   = blend_info;                // 默认列表：array(NULL) 渲染它

if (bk_draw_osd_new(&s_osd, &cfg) != AVDK_ERR_OK) { s_osd = NULL; return; }
```

> `panel_w/panel_h` 是**目标显示坐标系（旋转后 buffer）的宽高**，用于把 sprite/blit 夹在屏内；组件不关心上游是否旋转，按最终显示朝向填即可。

### 第 3 步：渲染 —— 一次 `bk_draw_osd_array()` 搞定

传 `blend_info[]`（或 `NULL` 用默认列表），组件按各元素 `xpos/ypos/width/height` **自动空间聚簇**分到多个 GPU slot，用户**不碰 slot / 多 blit / begin / commit**。对照示例 `osd_mipi_show()` 里的 `bk_draw_osd_array(osd, NULL)`。

```c
bk_draw_osd_array(s_osd, blend_info);   // 传 NULL 则用 new 时的默认列表
```

自动聚簇规则：

- **元素分散** → 分到不同 slot，各 slot 只覆盖自身小区域（`flexa` 逐块融合时中间空白 block 被跳过，不浪费 GPU）。
- **元素数超 slot 上限**（`BK_GPU_BLIT_SLOT_MAX`，当前 4）→ 按「合并后新增空白面积最小」就近并簇，直到 ≤ 上限，**绝不丢元素**。

**追加单个/临时元素**（`array` 之后 slot 游标停在已用簇之后，可继续追加）：

```c
/* a) 有 bk_blend_t 资产的单元素（图/字），用元素自带坐标/颜色/内容 */
bk_draw_osd_element(s_osd, &(blend_info_t){ .addr = &font_clock, .content = "12:35" });

/* b) 没有 bk_blend_t 的裸字体文本（如 LVGL 抗锯齿时间，塞不进 bk_font 的 blend_info） */
bk_draw_osd_text(s_osd, OSD_FONT_LVGL, &lv_font_montserrat_24, "12:35",
                 /*x*/64, /*y*/8, /*argb*/0xFFFFFF, /*scale*/1);
```

两者都自动占下一个空闲 slot，超过 `BK_GPU_BLIT_SLOT_MAX` 返回 `AVDK_ERR_NOMEM`。

> 示例 `draw_osd_example` 已收敛为**纯 `blend_info`** 渲染（只调 `bk_draw_osd_array(s_osd, NULL)`），不再演示 LVGL 文本；但 `bk_draw_osd_text` 及 LVGL 驱动（`bk_osd_lv_font.*`）完整保留，其它工程需要时按上面调用即可。

### 第 4 步：运行时更新 / 清理 / 销毁

```c
/* 秒级刷新时间：改动态列表内容 → 必须再 array(NULL) 重画 */
bk_draw_osd_add_or_update(s_osd, "clock", "12:35");
bk_draw_osd_array(s_osd, NULL);

/* 移除某元素：同样改列表后重画 */
bk_draw_osd_remove(s_osd, "wifi");
bk_draw_osd_array(s_osd, NULL);

/* 只撤叠加、保留视频（息屏 / 切页面）*/
bk_draw_osd_clear(s_osd);

/* 彻底销毁（切通路 / 退出）*/
bk_draw_osd_delete(s_osd); s_osd = NULL;
```

> 每次"重画"组件内部会重开 sprite、合成、注册给 GPU；旧 sprite 由引擎注册的 `free` 回调在 GPU 换帧时释放，无需手动管理。`add_or_update` / `remove` **只改内存里的动态列表**，一定要再调 `array(NULL)` 才生效。

---

## 4. 融合时机：frame vs flexa（可选调优）

「何时把 OSD 叠到视频帧上」是**绑定 GPU 的属性**，不是 OSD API 参数，用 `bk_gpu_ioctl` 设，运行时可切、无需重编：

```c
bool osd_by_flexa = false;   // false=整帧末一次 SRC_OVER（默认）；true=按 flexa 逐块分散
bk_gpu_ioctl(gpu, BK_GPU_IOCTL_SET_OSD_BY_FLEXA, &osd_by_flexa);
```

| 模式 | 语义 | 适用 |
|---|---|---|
| `frame`（默认，`osd_by_flexa=false`） | 整帧结束后对 OSD 覆盖区做一次 SRC_OVER | 元素少而集中（如状态栏） |
| `flexa`（`osd_by_flexa=true`） | 按 flexa 逐块分散 SRC_OVER，成本摊到整帧 | 元素多/分散，或出现 `flexa waits frame start` 掉帧 |

示例把它封装进 `osd_mipi_show(mode)` / `osd_uvc_show(mode)`（CLI 的 `frame`/`flexa`）。详见设计文档 §8。

---

## 5. 编译、烧录与 CLI 体验（示例工程）

```bash
# 在 SDK 根目录执行
make bk7259 PROJECT=multimedia/draw_osd_example -j32
```

产物 `build/bk7259/draw_osd_example/package/all-app.bin`。上电串口打印 `draw_osd_example m55 running...`，约 5s 后自动开 flexa OSD 并保持出图。

OSD 命令跑在 **AP** 侧，从 CP 串口发命令要加 `ap_cmd` 前缀：

```text
ap_cmd osd <mipi|uvc> <frame|flexa|update|remove|clear|close> [参数]
```

| 动作 | 对应 API | 说明 |
|---|---|---|
| `frame` / `flexa` | `bk_draw_osd_array` + `bk_gpu_ioctl(...SET_OSD_BY_FLEXA)` | 显示 OSD 并选融合时机（首次自动拉起 pipeline） |
| `update <name> <content>` | `bk_draw_osd_add_or_update` + `array` | 改图标/文本（必带 content） |
| `remove <name>` | `bk_draw_osd_remove` + `array` | 移除元素 |
| `clear` | `bk_draw_osd_clear` | 只撤 OSD，保留视频 |
| `close` | `bk_draw_osd_delete` + 关 pipeline | 撤 OSD 并释放 GPU/显存（切摄像头前用） |

> mipi 与 uvc 共用 GPU/显存，不能同时开。切通路先 `osd <当前源> close` 再开另一路，否则 `alloc flexa buffer ... failed`（OOM）。

典型序列：

```text
ap_cmd osd mipi flexa                          # 开 MIPI，逐块融合
ap_cmd osd mipi update wifi_group wifi_rssi_full
ap_cmd osd mipi update text1 12:53
ap_cmd osd mipi remove wifi_group
ap_cmd osd mipi clear                          # 只撤 OSD，摄像头继续
ap_cmd osd mipi close                          # 关 MIPI 释放资源
ap_cmd osd uvc flexa                           # 切到 UVC
```

---

## 6. 资源准备

资源放 `ap/assets/`：

| 文件 | 内容 | 生成方式 |
|---|---|---|
| `bk_img.c` | ARGB8888 图标 / 大图（喂 `blend_image_t.data`） | UI 工具导出 → C 数组 |
| `bk_font.c` | emWin 字模（`gui_font_digit_struct`） | FontCvt.exe |
| `blend_dsc.c` | `blend_assets` / `blend_info` 数组 | 手工/工具 |
| `blend.h` | 资源 `extern` 声明 | — |

> 示例默认不带 LVGL 字库。若要 LVGL 抗锯齿字体：另加 LVGL 字库 `.c`，用 `lv_font_conv --range` 只打包所需字符（如仅 `0-9:`）省内存；字库 `.c` 的 `#include "lvgl.h"` 改为 `#include "bk_osd_lv_font.h"`，并把 `lv_font_t` 里 `.get_glyph_dsc` / `.get_glyph_bitmap` 置 `NULL`。组件侧 LVGL 解码驱动（`bk_osd_lv_font.c/.h`）已内置。

---

## 7. 移植清单（以 doorbell_lp 状态栏为例）

1. **依赖**：`CMakeLists.txt`/`config` 加 `bk_draw_osd`（及依赖 `bk_gpu`）。
2. **资产**：把 `bk_img.c` / `bk_font.c` / `blend_dsc.c` / `blend.h`（或等价资产）放进 `assets/`。
3. **GPU handle**：把示例的 `mipi_pipeline_get_gpu_handle()` 换成本工程显示链路取 pipeline GPU 的实际接口（须已 `vg_lite_init`）。
4. **颜色格式**：按输出通路设 `cfg.src_format`（MIPI=ABGR8888 / UVC=ARGB8888），否则红蓝反。
5. **融合策略**：状态栏少量集中元素用默认 `frame`；元素分散掉帧则切 `flexa`（`bk_gpu_ioctl`）。
6. **渲染**：准备好 `blend_info[]` 后一行 `bk_draw_osd_array(osd, blend_info)`，分槽自动完成。

---

## 8. 注意事项

1. **必须绑定 pipeline GPU**：`cfg.gpu` 须是已 `vg_lite_init` 的 pipeline GPU handle；pipeline 未开时 `new` 失败。
2. **颜色格式**：MIPI sprite 用 `ABGR8888`（否则红蓝反），UVC 用 `ARGB8888`。MIPI 下指定颜色时 `argb` 按 `0x00BBGGRR` 填（R/B 对调）；R=B 的色（纯白/纯绿）不受影响。
3. **双实例独立**：UVC / MIPI 各 `new` 一个实例、各绑自己的 GPU，可并发；切通路前先 `delete` 旧实例再重建。
4. **资源生命周期**：`blend_assets/blend_info/bk_blend_t/图标/字模` 均**不深拷贝**，指针须在 handle 生命周期内有效；数组必须 `{.addr=NULL}` 结尾。
5. **槽位上限**：GPU 最多 `BK_GPU_BLIT_SLOT_MAX`（当前 4）个 blit 区域。`array` 自动聚簇并入 ≤4 个 slot（元素多不丢，靠近的合到一张 sprite）；`array` 后再用 `element`/`text` 追加占后续 slot，超限返回 `AVDK_ERR_NOMEM`。
6. **更新必须重画**：`add_or_update` / `remove` 只改列表，需再 `bk_draw_osd_array` 才生效。
7. **坐标越界**：越出 sprite 的像素自动裁剪；真正 blit 面积由内部包围盒收紧（见 `OSD_COMMIT ... area=` 日志）。
8. **MIPI 摄像头依赖**：需 1.8V VDDIO（`PM_AUXLDO_USER_DISPLAY`）、`CONFIG_XTAL_FREQ=12000000`；冷启动识别不到先查这两项。

---

## 9. 常见问题（FAQ）

- **`new` 返回失败 / GPU handle NULL**：对应视频链路还没开，`cfg.gpu` 是 NULL；先开视频再叠 OSD。
- **图标/文字红蓝反**：MIPI 实例 `src_format` 设 `BK_PIXEL_FORMAT_ABGR8888`；文本颜色按 `0x00BBGGRR` 预对调。
- **改了内容不刷新**：`add_or_update`/`remove` 后忘了再调 `bk_draw_osd_array(h, NULL)`。
- **屏不亮**：查背光 GPIO_7 / 复位 GPIO_60 是否配为输出、面板是否 `hx8399c_mipi_1080x1920`、VDDIO 是否上电。
- **字体有锯齿**：用 4bpp 抗锯齿字库（LVGL `lv_font_*` / bk_font 4bpp 字模）。
- **`flexa waits frame start` 掉帧**：`frame` 模式单帧 OSD 总面积偏大（经验 > ~50k px）或元素太分散；切 `flexa` 策略。
- **切通路 `alloc flexa buffer ... failed`（OOM）**：MIPI 与 UVC 共用显存，切换前先把当前通路 `close`。
- **`undefined reference to lv_font_get_bitmap_fmt_txt`**：字库 `.c` 仍指向 `lv_font_get_*`；改 `#include "bk_osd_lv_font.h"` 并把 `.get_glyph_dsc` / `.get_glyph_bitmap` 置 `NULL`。
- **`multiple definition of lv_font_get_bitmap_fmt_txt`**：同固件也链接完整 LVGL，且字库 `.c` 仍保留标准回调指针；OSD-only 字库请按上条置 `NULL`，或改由 LVGL 提供符号。
