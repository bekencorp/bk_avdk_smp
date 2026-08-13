# 动画 → .baf 转换工具（BAF 资产工具）

*[English](README.md) | 中文*

把 **APNG / 动画 WebP / GIF / MP4 / MOV** 一键转成 **`.baf`**（Beken Animation Format）——
带真 alpha 的双路 H.264（RGB + 灰度 Alpha）动画格式，供 BK7259 的 BAF(`lv_baf`) 控件使用。

## 一键用法

```bash
python3 to_baf.py --input <动画文件>
```

自动识别输入类型，产出两样东西(默认在输入同目录):

| 产物 | 说明 |
|---|---|
| `<name>.baf` | **自包含二进制容器**(header + RGB H.264 + Alpha H.264 + AU 表 + 每帧时长),可放文件系统/flash 分发 |
| `<name>_baf_asset.c` | **C 资产**(`bk_baf_source_t`),直接编译进固件,当前设备就能用 |

常用参数:
```bash
python3 to_baf.py --input my_anim.apng \
    --outdir out --name my_anim \
    --symbol my_anim_bk_baf_source   # C 资产里的符号名
    # --no-c    只出 .baf
    # --no-baf  只出 C 资产
```

### 在工程里用 C 资产

```c
extern const bk_baf_source_t my_anim_bk_baf_source;   /* 由 <name>_baf_asset.c 提供 */

lv_obj_t *anim = lv_baf_create(scr);
lv_baf_set_gpu_overlay(anim, true);              /* 可选:GPU overlay */
lv_baf_set_src(anim, &my_anim_bk_baf_source);
```
把 `<name>_baf_asset.c` 加进工程 `CMakeLists.txt` 的 srcs 即可。

> 注:当前设备端消费的是 **C 资产**(编译内联)。`.baf` 二进制容器已经产出,但**设备端运行时 `.baf` 加载器尚未实现**(需要时可另做,从文件系统/flash 直接加载,免重编固件)。

## 支持的输入

- **PIL 路径**(逐帧 + 每帧时长 + alpha 通道):`.png/.apng`、`.gif`、`.webp`
- **ffmpeg 路径**(视频容器):`.mp4/.mov/.mkv/.webm/.m4v/.avi`;若像素格式带 alpha(如 rgba/yuva)会自动 `alphaextract` 取 alpha,否则按不透明处理
- 依赖:`python3 + Pillow`、`ffmpeg/ffprobe`

## 处理流程

```
源 → 抽每帧 RGB + Alpha(灰度) + 时长
   → H.264 编码 RGB(无 B 帧, refs=1) + H.264 编码 Alpha 为灰度(可选)
   → Annex-B + 逐帧 AU 切分 + 去 AUD
   → 打包 .baf(二进制) 和 _baf_asset.c(C 数组)
```
Alpha 用灰度编码,与 BK7259 BAF 设备端解码一致。

## .baf 二进制格式(小端)

```
magic        char[8]  "BAFANIM1"
version      u32      = 1
width        u16      RGB 宽
height       u16      RGB 高
alpha_width  u16      0 => 与 width 相同(全分辨率 alpha)
alpha_height u16      0 => 与 height 相同
frame_count  u32
flags        u32      bit0 = HAS_ALPHA
rgb_size     u32
alpha_size   u32      无 alpha 时为 0
reserved     u32[4]   = 0
--- 变长段 ---
durations    u32[frame_count]                    每帧时长(ms)
rgb_aus      (u32 offset,u32 size)[frame_count]  指向 rgb blob 的 AU 表
alpha_aus    (u32 offset,u32 size)[frame_count]  仅 HAS_ALPHA
rgb_data     u8[rgb_size]                         RGB H.264 Annex-B
alpha_data   u8[alpha_size]                       Alpha H.264 Annex-B(若有)
```

## 目录里的脚本

| 脚本 | 角色 |
|---|---|
| **`to_baf.py`** | ⭐ 一键转换入口(APNG/WebP/GIF/MP4/MOV → .baf + C 资产) |
| `mp4_to_bk_baf_asset.py` | 打包核心库(Annex-B/AU 切分/去 AUD/C 资产生成),被 `to_baf.py` 复用;也可单独从 RGB/Alpha mp4 打包 |
