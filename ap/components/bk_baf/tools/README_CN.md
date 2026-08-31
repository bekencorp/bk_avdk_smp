# 动画 → BAF v1 容器 转换工具

*[English](README.md) | 中文*

把 **APNG / 动画 WebP / GIF / MP4 / MOV / MKV / WEBM** 一键转成 **BAF v1 容器**
（Beken Animation Format，规范见仓库根目录 `BAF_SPEC_CN.md`，设备端结构见
`bk_baf/include/bk_baf_container.h`）。容器内是单路 H.264（RGB）+ 可选的**灰度 H.264
alpha 流**，强制 `bframes=0 refs=1`、非 4:4:4，与 BK7259 的 VPU 硬解能力对齐。

## 一键用法

```bash
python3 to_baf.py --input <动画文件>
```

自动识别输入类型，默认在输入同目录产出**两种字节完全相同**的容器形态：

| 产物 | 说明 |
|---|---|
| `<name>.baf` | 二进制容器文件，放文件系统 / flash 分发，运行时加载 |
| `<name>_baf.c` | **同样的字节**编译进固件：`const unsigned char <name>_baf[]` + `const unsigned int <name>_baf_size` |

两者是同一份容器镜像，设备端用**同一个** `baf_parse_inplace()` 解析（见
`bk_baf_container.h`），无需任何转换。

### 参数

| 参数 | 默认 | 说明 |
|---|---|---|
| `--input <path>` | （必填） | 输入动画（APNG/WebP/GIF/MP4/MOV/MKV/WEBM…） |
| `--outdir <dir>` | 输入同目录 | 输出目录 |
| `--name <name>` | 输入文件名（stem） | 输出基名，决定 `<name>.baf` / `<name>_baf.c` |
| `--emit {file,array,both}` | `both` | 只出 `.baf` / 只出 C 数组 / 两者都出 |
| `--symbol <sym>` | `<name>_baf` | C 数组符号名 |
| `--force-opaque-alpha` | 关 | 输入无 alpha 时，生成一条全不透明的全分辨率 alpha 流 |

```bash
# 只出 .baf 文件
python3 to_baf.py --input clip.mp4 --emit file --outdir out --name hello
# 只出 C 数组，自定义符号名
python3 to_baf.py --input hello.gif --emit array --symbol hello_baf
```

## 在工程里消费（两种方式，都由 bk_baf 内部解析容器）

**① 编译进固件的 C 数组：**

```c
extern const unsigned char hello_baf[];        /* 由 <name>_baf.c 提供 */
extern const unsigned int  hello_baf_size;

/* LVGL 控件 */
lv_baf_set_src_data(anim, hello_baf, hello_baf_size);

/* 或直接用 bk_baf 播放器 */
bk_baf_decoder_t *d = bk_baf_open(&(bk_baf_config_t){
    .data = hello_baf, .data_len = hello_baf_size });
```

把 `<name>_baf.c` 加进工程 `CMakeLists.txt` 的 srcs 即可。

**② 文件系统 / flash 上的 `.baf` 文件：**

```c
lv_baf_set_src_file(anim, "S:/baf/hello.baf");   /* 经 lv_fs 读取后由 bk_baf 解析 */
```

> 注：`bk_baf` 解析时是**原地 alias 容器字节**（零拷贝），所以传入的数组 / 缓冲区
> 必须在播放期间保持有效（const flash 数组天然满足；文件路径由 `lv_baf` 内部持有并在
> 关闭时释放）。

## 支持的输入

- **PIL 路径**（逐帧 + 每帧时长 + alpha 通道）：`.png/.apng`、`.gif`、`.webp`
- **ffmpeg 路径**（视频容器）：`.mp4/.mov/.mkv/.webm/.m4v/.avi`；像素格式带 alpha
  （如 rgba/yuva）会自动 `alphaextract` 取 alpha，否则按不透明处理
- 依赖：`python3 + Pillow`、`ffmpeg / ffprobe`

## 处理流程

```
源 → 抽每帧 RGB + Alpha(灰度) + 时长(ms)
   → 尺寸对齐到 16 的倍数(VPU 宏块要求)
   → H.264 编码 RGB(无 B 帧, refs=1) + H.264 编码 Alpha 为灰度(可选)
   → Annex-B + 逐帧 AU 切分 + 去 AUD
   → 打包成 BAF v1 容器(64B FileHeader + ChunkDirectory + 64B 对齐的 IDX/DUR/DATA)
```

Alpha 用灰度（gray）编码，解出 Y 平面即 A8 掩码，与 BK7259 BAF 设备端解码一致。

## 容器格式

规范以仓库根目录 **`BAF_SPEC_CN.md`** 为准，设备端 C 结构见
**`bk_baf/include/bk_baf_container.h`**。要点：64 字节定长 `FileHeader`（magic
`"BAFANIM1"`、几何、帧数、各段目录索引、`header_crc32`）+ 扁平 `ChunkDirectory`
（16B/项 `{type, offset, size, crc32}`）+ 64B 对齐的 `'IDX '`（每帧 AU 偏移/长度）/
`'DUR '`（每帧毫秒）/ `'DATA'`（H.264 Annex-B）。

## 目录里的脚本

| 脚本 | 角色 |
|---|---|
| **`to_baf.py`** | 转换入口（APNG/WebP/GIF/MP4/MOV… → BAF v1 容器 `.baf` + `_baf.c`） |
| `mp4_to_bk_baf_asset.py` | 打包核心库（ffmpeg 调用 / Annex-B / AU 切分 / 去 AUD），被 `to_baf.py` 复用 |
