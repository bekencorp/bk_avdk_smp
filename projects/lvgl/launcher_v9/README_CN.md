# launcher_v9 工程说明（480x480 RGB 版）

* [English](./README.md)

## 1. 工程简介

`launcher_v9` 是 BK7259 上的**应用启动器（横滑轮播主页）+ 小游戏**工程，使用 **LVGL V9.3.0**。界面全部用 LVGL 原生对象绘制，不依赖任何图片素材，包含 9 页横滑轮播菜单，以及其中的 2048 和 Juice Rush（切水果）两个可玩小游戏。UI 默认用 JavaScript 编写（`ap/js/`），由 SDK 的 `bk_jerryscript` 组件在启动时执行；同时保留了一套等价的 C 实现，可通过一个宏切换。

它与同目录的 `widgets_v9`（LVGL 官方 Widgets Demo 工程）相互独立，两者可分别编译、分别烧录，互不影响：

| 工程 | 内容 | 编译命令 |
| --- | --- | --- |
| `widgets_v9` | LVGL 官方 Widgets Demo（保持原样） | `make bk7259 PROJECT=lvgl/widgets_v9` |
| `launcher_v9` | 轮播主页 + 2048 + Juice Rush | `make bk7259 PROJECT=lvgl/launcher_v9` |

简单理解：

1. **可直接运行的应用外壳**：开机进入轮播主页，点击中间卡片进入对应应用/游戏。
2. **跑通了 RGB 并口屏、DPU、frame buffer、LVGL vendor 适配层和触摸输入**。
3. **适合作为新 UI 项目的参考模板**：在 `ap/js/launcher.js` 的应用表里增删卡片即可扩展菜单。全部 UI（轮播、2048、Juice Rush）用 JavaScript 编写，开机由 `bk_jerryscript` 组件执行。

## 2. 硬件与显示参数

| 项目 | 默认配置 |
| --- | --- |
| 芯片平台 | BK7259 |
| LVGL 版本 | LVGL V9.3.0 |
| 屏幕接口 | RGB 并口（24bit 像素通道 + SW SPI 配置通道） |
| 默认屏驱动 | `st7701s_rgb_480x480`（`CONFIG_LCD_ST7701S=y`） |
| 默认分辨率 | `480 x 480` |
| 默认像素格式 | `ARGB8888` + DPU 解压输出 |
| 渲染模式 | `RENDER_PARTIAL_MODE` |
| 配置 SPI | CLK `GPIO_8` / CSX `GPIO_28` / SDA `GPIO_9`（软件位敲） |
| LCD Reset | `GPIO_6` |
| 背光 | `GPIO_7` |
| 触摸 | 默认开启 `CONFIG_TP=y` |

> 接线以 SDK 文档 `ap/docs/bk7259/zh_CN/developer-guide/display/how_to_add_rgb_panel.rst` 与 `projects/multimedia/rgb_lcd_example` 为准。如果你的屏幕型号、分辨率或 GPIO 与默认配置不同，需要先适配显示硬件，再调 LVGL UI，否则可能黑屏。

**触摸引脚需要按实际板子确认**：`defconfig` 里的 `CONFIG_TP_INT_PIN=6` 与 LCD Reset（`GPIO_6`）冲突，如果你的 RGB 底板触摸 INT 不是这个脚，请改成实际引脚，否则触摸中断不可用。

## 3. 目录结构

工程采用 AP + CP 双核结构，新手主要看 `ap/` 目录。

```text
launcher_v9/
├── CMakeLists.txt                 # 工程级 CMake 文件（project(launcher_v9)）
├── Makefile                       # make 构建入口
├── README_CN.md                   # 中文工程说明文档
├── ap/                            # AP 侧：LVGL、显示、触摸、UI
│   ├── ap_main.c                  # AP 主入口，初始化显示/触摸并启动 UI 后端
│   ├── js/                        # JS UI 源码（默认后端）
│   │   ├── ui_common.js           # 公共常量与绘制辅助函数
│   │   ├── launcher.js            # 横滑轮播主页（应用卡片菜单）
│   │   ├── game_2048.js           # 2048 小游戏
│   │   ├── juice_rush.js          # Juice Rush 切水果小游戏
│   │   └── main.js                # 页面路由入口
│   ├── js_bundle.c / js_bundle.h  # 由公共 js2c.py 生成，编译进固件（默认后端）
│   ├── launcher.c / game_2048.c / juice_rush.c   # 原生 C 实现（LAUNCHER_UI_USE_JS=0 时使用）
│   ├── lv_conf_custom.h           # 工程级 LVGL 配置覆盖
│   └── config/bk7259_ap/
│       ├── defconfig              # AP 侧功能开关
│       └── usr_gpio_cfg.h         # AP 侧 GPIO 默认配置
├── cp/                            # CP 侧：启动 AP 系统、基础系统逻辑
│   ├── cp_main.c
│   └── config/bk7259/
├── littlefs_js.bin                # 文件系统后端的应用镜像，烧到 usr_config
└── partitions/                    # Flash/RAM 分区配置
```

建议阅读顺序：

1. `README_CN.md`：先了解工程怎么用。
2. `ap/ap_main.c`：看 RGB 屏、触摸、LVGL 如何启动，以及 UI 后端如何被调起。
3. `ap/js/launcher.js`（或 C 版 `ap/launcher.c`）：看轮播主页的卡片布局、吸附联动和进入应用的逻辑。
4. `ap/js/game_2048.js`、`ap/js/juice_rush.js`：看两个小游戏的实现。
5. `ap/lv_conf_custom.h`：看本工程额外打开了哪些 LVGL 配置。

## 4. 运行效果

固件启动后，AP 侧会自动初始化 RGB 显示链路，再由 UI 后端创建界面：默认由 `bk_jerryscript` 执行内嵌 bundle（入口 `ap/js/main.js` 调用 `launcher_start()`），屏幕出现横滑轮播的应用主页；左右滑动切换卡片，点击中间卡片进入对应应用。

| 模块 | 说明 |
| --- | --- |
| 轮播主页 | 卡片吸附居中、顶部标题联动、底部页码点联动 |
| 卡片顺序 | MUSIC / GALLERY / WEATHER / SETTINGS / CALENDAR / VOICE NOTE / WHATSAPP / 2048 / JUICE RUSH |
| 2048 | 4x4 棋盘，四向滑动合并，含得分/最高分/结算页 |
| Juice Rush | 60 秒切水果，含连击、炸弹、刀光、果汁特效与结算页 |
| 其他卡片 | 点击后底部提示 `THIS APP IS COMING SOON` |

## 5. 快速上手

### 5.1 准备环境

1. 已进入 SDK 根目录：`bk_avdk_smp_release_4.0.1/`
2. 编译环境已经配置好。
3. 开发板连接了 ST7701S 480x480 RGB 屏和触摸屏。

### 5.2 编译

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=lvgl/launcher_v9
```

也可以按 CI 使用的方式执行：

```bash
./dbuild.sh make bk7259 PROJECT=lvgl/launcher_v9
```

编译成功后，固件会生成在 `build/bk7259/launcher_v9/` 目录下（与 `widgets_v9` 的构建目录相互独立）。

### 5.3 烧录与运行

使用 Beken 烧录工具或 SDK 对应烧录流程，将固件烧录到开发板后上电或复位：

1. 串口日志应能看到系统启动、media service 初始化、LVGL 初始化等信息。
2. 屏幕背光会被拉高（`GPIO_7`）。
3. LCD 打开后显示轮播主页。
4. 触摸正常时可直接左右滑动、点击卡片。

## 6. 启动流程

`ap/ap_main.c` 是理解本工程的核心入口。

1. `main()` 调用 `bk_init()` 初始化系统。
2. 调用 `media_service_init()` 初始化多媒体服务。
3. 调用 `bk_frame_buffer_init()` 初始化帧缓冲。
4. 注册 `launcher` CLI 命令。
5. 调用 `lvgl_app_launcher_init()` 初始化显示、触摸和 LVGL。

`lvgl_app_launcher_init()` 内部主要做这些事：

1. 按名字取出 RGB 面板描述符：`bk_lcd_find_rgb_panel_by_name("st7701s_rgb_480x480")`
2. 创建 SW SPI 配置总线：`bk_display_spi_bus_new()`（CLK/CSX/SDA 三根 GPIO）
3. 创建 RGB LCD panel：`bk_lcd_rgb_panel_new()`（reset 走 `GPIO_6`）
4. 创建 DPU 显示控制器：`bk_display_dpu_ctlr_new()`
5. 初始化并打开显示：`bk_display_init()`、`bk_display_open()`
6. 打开背光：`GPIO_7` 输出高电平
7. 配置 `lv_vnd_config_t`（480x480、`RENDER_PARTIAL_MODE`、压缩输出）
8. 按 `output_compress` 计算 frame buffer 大小（480x480 字节/块）
9. 申请 LVGL frame buffer
10. 调用 `lv_vendor_init()` 初始化 LVGL 适配层
11. 如开启触摸，调用 `drv_tp_open()`
12. 启动 LVGL 线程：`lv_vendor_start()`
13. 创建 UI：`LAUNCHER_UI_USE_JS=1` 时 `bk_js_init()` → `bk_js_eval("launcher.js", ...)` 执行内嵌 bundle（`js_bundle.c`）；`=0` 时加锁调用 C 版 `launcher_start()`

JS 与 LVGL 的线程关系：JS 代码跑在 `bk_jerryscript` 的独立任务里，组件内部用 display mutex 保证同一时刻只有一个线程在调 LVGL，所以 JS 侧不需要额外加锁。触摸事件由 LVGL 线程回调转入 JS 任务异步执行，触点坐标随事件一起传递。

退出时 `lvgl_app_launcher_deinit()` 会先 `bk_js_deinit()` 停掉 JS 引擎（它持有定时器和事件回调，必须先于 LVGL 释放），再停止 LVGL；C 后端则在 LVGL 释放后调用 `launcher_reset()` / `game_2048_reset()` / `juice_rush_reset()` 清空缓存指针，最后释放 frame buffer 和显示资源。

## 7. 关键配置说明

### 7.1 功能开关

关键配置在 `ap/config/bk7259_ap/defconfig`：

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_LVGL=y` / `CONFIG_LVGL_V9=y` | 启用 LVGL V9 |
| `CONFIG_BK_DISPLAY=y` | 启用显示框架 |
| `CONFIG_LCD_RGB=y` | 启用 RGB 并口屏支持 |
| `CONFIG_LCD_ST7701S=y` | 启用 ST7701S 480x480 RGB 屏驱动 |
| `CONFIG_DPU_DRIVER=y` | 启用 DPU 显示控制器 |
| `CONFIG_FRAME_BUFFER=y` | 启用帧缓冲 |
| `CONFIG_TP=y` | 启用触摸 |
| `CONFIG_LVGL_FRAME_BUFFER_NUM=2` | LVGL 使用 2 个 frame buffer |
| `CONFIG_LVGL_TASK_STACK_SIZE=8192` | LVGL 线程栈大小 |
| `CONFIG_PSRAM=y` | 启用 PSRAM，JS 堆从普通 PSRAM 堆 `AP_PSRAM_HEAP` 分配 |
| `CONFIG_PSRAM_AS_SYS_MEMORY` | 关闭：系统堆仍留在 SRAM，保持 LVGL 既有内存行为 |
| `CONFIG_BK_JERRYSCRIPT=y` | 启用 bk_jerryscript（JerryScript）组件 |
| `CONFIG_BK_JERRYSCRIPT_HEAP_SIZE_KB=256` | JS 堆大小，经 `psram_malloc()` 落在 PSRAM |
| `CONFIG_BK_JERRYSCRIPT_TASK_STACK_SIZE=32768` | JS 任务栈 |
| `CONFIG_BK_JERRYSCRIPT_UI=y` | 启用 `lv.*` UI 绑定 |
| `CONFIG_BK_JERRYSCRIPT_AUTO_START` | 关闭：由 `ap_main.c` 主动调起，不用组件的自动启动 |

> RGB 配置通道使用软件位敲 SPI，不需要打开 `CONFIG_SPI`。

### 7.2 LVGL 工程级配置

`ap/lv_conf_custom.h` 会先包含 SDK 默认的 `lv_conf.h`，再覆盖本工程需要的选项：

| 配置项 | 作用 |
| --- | --- |
| `LV_USE_DEMO_WIDGETS=0` | 本工程不使用官方 Widgets Demo，关闭以省 Flash |
| `LV_FONT_MONTSERRAT_12/16/18/20/24/32/40/48=1` | 启用 UI 用到的全部字号 |

如果你想启用更多 LVGL 功能，优先在这个文件里做工程级覆盖，不建议直接修改 LVGL 组件的公共配置。

### 7.3 ARGB8888 与压缩输出

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `dpu_config.video.format` | `BK_PIXEL_FORMAT_ARGB8888` | DPU 视频层像素格式 |
| `dpu_config.video.decompress` | `true` | DPU 侧开启解压 |
| `lv_vnd_config.output_compress` | `true` | LVGL 输出使用压缩路径 |
| `draw_pixel_size` | `WIDTH * 64 * sizeof(bk_color_t)` | partial mode 下绘制 buffer 大小 |
| `WIDTH` / `HEIGHT` | `480` / `480` | 与 panel timing 保持一致 |

改 UI 通常不需要动这些参数；要改显示格式、压缩策略或做性能调优时再重点看这一段。

### 7.4 480x480 布局约束

9 页轮播、2048、Juice Rush 的坐标都是按 480x480 写死的，主要布局参数：

| 模块 | 参数 |
| --- | --- |
| 轮播卡片 | 258x258，间距 26，容器 y=118 |
| 页码点 | y=420，单点 10x10，占位 26 |
| 底部面板 | y=436 起，高 44 |
| 2048 棋盘 | 格子 80、间距 8，棋盘 344x344，起点 (68, 64) |
| Juice Rush | 水果半径 34~44，重力 900 px/s²，HUD 高 82 |

### 7.5 UI 后端切换（JS / C）

两套 UI 实现同时参与编译，由 `ap/ap_main.c` 顶部的一个宏决定跑哪一套：

```c
#define LAUNCHER_UI_USE_JS 1   /* 1 = JavaScript，0 = 原生 C */
```

- `=1`：`bk_js_init()` + `bk_js_eval()` 执行 `js_bundle.c`，界面来自 `ap/js/`。
- `=0`：加锁调用 C 版 `launcher_start()`，界面来自 `ap/launcher.c`、`game_2048.c`、`juice_rush.c`。

两边内容一致，改这一个宏后重新编译即可，无需改 CMake。代价是固件里同时包含两套实现，Flash 占用略高。

### 7.6 JS 源码与打包

JS 页面存放在 `ap/js/`，打包成单个 C 字符串数组写进 `ap/js_bundle.c`，运行时用 `bk_js_eval()` 一次性执行。打包使用组件自带的公共转换工具 `ap/components/bk_jerryscript/tools/js2c.py`，工程内不再保留私有副本。合并顺序固定：

```text
ui_common.js → launcher.js → game_2048.js → juice_rush.js → main.js
```

修改任意 `.js` 后必须重新生成 bundle，否则改动不会进固件。在工程目录下执行：

```bash
python3 ../../../ap/components/bk_jerryscript/tools/js2c.py \
    ap/js/ui_common.js ap/js/launcher.js ap/js/game_2048.js \
    ap/js/juice_rush.js ap/js/main.js \
    -o ap/js_bundle.c --header ap/js_bundle.h --name js_bundle
```

这是**默认**方式：源码随固件编译，不依赖任何文件系统。如果需要运行期从文件系统加载，见 7.7。

两种方式共用同一个 JS 引擎堆：走 `psram_cache_malloc()`，对应 `partitions/bk7259/ram_regions.csv` 里的 `AP_PSRAM_CACHE_HEAP`，当前为 512KB（`0x080000`），足够容纳 256KB JS 堆 + 上下文。这块内存是可缓存 PSRAM，CPU 访问速度接近内部 SRAM。

### 7.7 从文件系统加载（可选）

把 `CONFIG_LAUNCHER_JS_FROM_FS` 打开后，启动时不再执行 `js_bundle.c`，而是从 `usr_config` 分区挂载的 littlefs 镜像里读取 `launcher` 应用。

**分区**：`partitions/bk7259/auto_partitions.csv` 中 `usr_config` 已扩到 1024K（偏移 `0x4C9000`）。

**生成镜像**（在工程目录下执行）：

```bash
mkdir -p fs/launcher
cat ap/js/ui_common.js ap/js/launcher.js ap/js/game_2048.js \
    ap/js/juice_rush.js ap/js/main.js > fs/launcher/main.js

cat > fs/launcher/manifest.json <<'EOF'
{ "id": "launcher", "name": "Launcher", "version": "1.0.0", "entry": "main.js" }
EOF

../../../ap/components/littlefs/mkimg/mklittlefs \
    -c fs -b 4096 -p 256 -s 1048576 littlefs_js.bin
```

注意镜像根目录直接放 `launcher/`，不要再套一层 `apps/`（挂载点已经是 `/apps`）。完整的目录结构、manifest 字段与参数说明见 [bk_jerryscript/README.md](../../../ap/components/bk_jerryscript/README.md)。

**烧录**：把 `littlefs_js.bin` 写到 `0x4C9000`（`usr_config` 分区）。

**验证**：启动日志应出现：

```text
ap0:js:I(xxx):mounted /apps on usr_config (0x4c9000, 1048576 bytes)
ap0:js:D(xxx):  launcher
ap0:js:I(xxx):js app 'launcher' running from fs
```

说明：

镜像大小必须等于 `usr_config` 分区大小（当前 1MB），block/page 参数必须与 littlefs 适配层一致。
- 修改 `ap/js/*.js` 后，重新打包并烧录镜像即可，**不需要重新编译固件**。
- 镜像缺失或损坏时挂载会失败，但不会阻塞启动（只打印 `file system not mounted` 警告）；此时后端为 `y` 才会报应用启动失败。
- 文件系统方式运行时，`require()` 与脚本读取的源码缓冲都在 PSRAM，不占用 SRAM 堆。

## 8. 常见修改

### 8.1 增删轮播卡片

卡片列表集中在 `ap/js/launcher.js` 的 `LC_APPS` 表里，顺序即卡片顺序：

```js
var LC_APPS = [
  { name: 'MUSIC', tag: 'PLAYER', top: 0x4B3BB0, bot: 0x171132, accent: 0xA897FF, kind: 1 },
  /* ... */
];
```

- `name` / `tag`：卡片名称与分类（Montserrat 只有 ASCII，必须用英文）。
- `top` / `bot`：卡片渐变。
- `accent`：序号、选中边框、强调线颜色。
- `kind`：图标绘制分支，对应 `lc_icon_draw()` 里的 `case`。

新增卡片后如果也想进入游戏，记得同步 `LC_APP_2048` / `LC_APP_GAME` 这两个下标常量，然后按 7.6 节的命令重新生成 `js_bundle.c`。选择 C 后端时，等价修改点在 `ap/launcher.c` 的 `s_apps[]`。

### 8.2 修改图标

JS 版图标全部在 `ap/js/launcher.js` 的 `lc_icon_draw()` 里用 `shape()` / `ring()` 拼出来，没有图片资源。

**不要使用 `transform_rotation/scale/skew`**：软件渲染的填充路径不消费矩阵，只会走 `refr_obj_matrix()` 把 clip 反变换后放大 1px，可能算出越界目标指针导致崩溃。需要旋转造型时用多个图元拼，例如 WhatsApp 听筒。

### 8.3 修改屏幕型号或分辨率

需要同时改这几处：

1. `ap/ap_main.c` 里 `bk_lcd_find_rgb_panel_by_name()` 的面板名字（当前 `st7701s_rgb_480x480`）。
2. `ap/ap_main.c` 里的 `WIDTH` / `HEIGHT` 宏。
3. `ap/config/bk7259_ap/defconfig` 中对应的 `CONFIG_LCD_xxx` 驱动开关。
4. 各页面的绝对坐标布局（见 7.4）。
5. 如有必要，调整 `lv_conf_custom.h` 里开启的字号。

### 8.4 修改背光和复位 GPIO

| 功能 | 默认值 | 修改位置 |
| --- | --- | --- |
| 背光 | `GPIO_7` | `ap/ap_main.c` 中 `bk_gpio_set_output_high(GPIO_7)` 附近 |
| LCD Reset | `GPIO_6` | `ap/ap_main.c` 中 `panel_config.reset_pin` |
| 配置 SPI | `GPIO_8/28/9` | `ap/ap_main.c` 中 `rgb_cfg_bus` |
| TP Reset/INT/I2C | `GPIO_5/6/1/0` | `ap/config/bk7259_ap/defconfig` |

## 9. 常见问题

### 9.1 编译失败，日志提示找不到面板

1. `defconfig` 是否打开了 `CONFIG_LCD_RGB=y` 与 `CONFIG_LCD_ST7701S=y`。
2. `bk_lcd_find_rgb_panel_by_name()` 的名字是否与面板文件里的 `.name` 完全一致。
3. 新增源码是否加入 `ap/CMakeLists.txt`。

### 9.2 屏幕黑屏

优先检查：

1. 面板名字与分辨率是否与实际屏匹配。
2. RGB 排线、供电、背光是否正常。
3. 配置 SPI 的 CLK/CSX/SDA 是否为 `GPIO_8/28/9`。
4. `GPIO_6` 是否真的是当前硬件的 LCD reset 脚。
5. `GPIO_7` 是否真的是背光脚。

### 9.3 触摸无反应

1. `CONFIG_TP=y` 是否开启。
2. `CONFIG_TP_RST_PIN`、`CONFIG_TP_INT_PIN`、`CONFIG_TP_I2C_SDA_PIN`、`CONFIG_TP_I2C_SCL_PIN` 是否匹配硬件。
3. **注意 `CONFIG_TP_INT_PIN=6` 与 LCD Reset 复用**，需要改成实际触摸 INT 脚。
4. 当前触摸 IC 是否在 `defconfig` 中开启，`drv_tp_open()` 是否被执行。

### 9.4 UI 偶发异常或死机

LVGL 默认不是线程安全的。凡是在 LVGL 线程以外创建或修改 UI，都要加锁：

```c
lv_vendor_disp_lock();
/* LVGL API calls */
lv_vendor_disp_unlock();
```

如果 UI 很复杂，出现栈溢出或异常，可以适当增大 `CONFIG_LVGL_TASK_STACK_SIZE`。

JS 后端不需要自己加锁：`bk_jerryscript` 组件在整段脚本/定时器/事件回调执行期间持 display mutex。

### 9.5 JS 初始化失败或页面空白

1. 日志出现 `js init failed: ...` 或 `context allocation failed`：JS 堆申请失败。确认 `CONFIG_PSRAM=y`，且 `ram_regions.csv` 中 `AP_PSRAM_HEAP` 不小于 `CONFIG_BK_JERRYSCRIPT_HEAP_SIZE_KB` + 16KB；也可以临时下调该 KB 值。
2. 日志出现 `js app failed: ...`：脚本语法或运行时错误，`bk_js_eval()` 会打印原因与大致行号。
3. 改了 `ap/js/*.js` 但屏幕没变化：忘记按 7.6 节的命令重新生成 `ap/js_bundle.c`。
4. 触摸有反应但页面不切换：确认事件名与绑定表一致（`click`/`pressed`/`pressing`/`released`/`press_lost`/`scroll`），并且手势逻辑用的是 `lv.touch()` 返回的事件坐标。
5. 想确认当前跑的是哪套 UI：看日志里是否出现 `js app running, bundle ... bytes`。

## 10. 与 widgets_v9 的关系

| 对比项 | `widgets_v9` | `launcher_v9` |
| --- | --- | --- |
| 入口 | `lv_demo_widgets()` | JS bundle（`ap/js/main.js` → `launcher_start()`）或 C 版 `launcher_start()` |
| 界面实现 | C | JavaScript（默认）或 C，由 `LAUNCHER_UI_USE_JS` 切换 |
| 界面 | LVGL 官方 Widgets Demo | 轮播主页 + 2048 + Juice Rush |
| 图片资源 | 需要 `assets/*.c` | 全部用图元绘制，无图片 |
| `LV_USE_DEMO_WIDGETS` | `1` | `0` |
| `CONFIG_BK_JERRYSCRIPT` | 不启用 | 启用 |
| 构建目录 | `build/bk7259/widgets_v9` | `build/bk7259/launcher_v9` |
| 适合场景 | 验证 LVGL 显示链路 | 参考 JS 驱动 UI 与游戏实现 |

两个工程互不引用，可以随时切换编译。如果只是验证 LVGL 显示链路，用 `widgets_v9` 更纯粹；如果要看自定义 UI 的完整实现，用 `launcher_v9`。
