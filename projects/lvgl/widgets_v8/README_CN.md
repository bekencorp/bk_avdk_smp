# LVGL Widgets V8 示例工程说明

* [English](./README.md)

## 1. 工程简介

`widgets_v8` 是 BK7259 SDK 中的 LVGL 官方 Widgets Demo 参考工程，使用 **LVGL V8.3.9**，用于演示按钮、滑条、列表、图表、图片、字体等常见 UI 控件。

简单理解：

1. **它不是一个完整产品应用**，而是一个“LVGL 控件展示台”。
2. **它帮你跑通 LVGL + MIPI LCD + 触摸 + 显示刷新链路**。
3. **它适合作为新 UI 项目的起点**：先确认屏幕能亮、触摸能用，再把 Demo UI 换成自己的界面。

### 1.1 默认硬件与显示参数

| 项目 | 默认配置 |
| --- | --- |
| 芯片平台 | BK7259 |
| LVGL 版本 | LVGL V8.3.9 |
| 屏幕接口 | MIPI DSI |
| 默认屏驱动 | `lcd_device_hx8399c_mipi_1080x1920` |
| 默认分辨率 | `1080 x 1920` |
| 默认像素格式 | `RGB565` |
| 渲染模式 | `RENDER_PARTIAL_MODE` |
| 背光 GPIO | `GPIO_7` |
| LCD Reset GPIO | `GPIO_60` |
| 触摸 | 默认开启 `CONFIG_TP=y` |

> 如果你的开发板不是这块屏，先不要急着改 LVGL。请先适配 LCD 驱动、分辨率和 GPIO，否则很容易出现黑屏或花屏。

## 2. 目录结构

工程采用 AP + CP 双核结构。新手主要看 `ap/` 目录。

```text
widgets_v8/
├── CMakeLists.txt                 # 工程级 CMake 文件
├── Makefile                       # make 构建入口
├── README_CN.md                   # 中文工程说明文档
├── ap/                            # AP 侧：LVGL、显示、触摸、UI 主逻辑
│   ├── ap_main.c                  # AP 主入口，初始化显示和启动 LVGL Widgets Demo
│   ├── lv_demo_widgets.c          # LVGL V8 Widgets Demo UI 代码
│   ├── lv_demo_widgets.h
│   ├── lv_conf_custom.h           # 工程级 LVGL 配置覆盖
│   ├── assets/                    # Demo 用到的图片资源
│   └── config/bk7259_ap/
│       ├── defconfig              # AP 侧功能开关
│       └── usr_gpio_cfg.h         # AP 侧 GPIO 默认配置
├── cp/                            # CP 侧：启动 AP 系统、基础系统逻辑
│   ├── cp_main.c
│   └── config/bk7259/
└── partitions/                    # Flash/RAM 分区配置
```

建议阅读顺序：

1. `README_CN.md`：先了解工程怎么用。
2. `ap/ap_main.c`：看屏幕、LVGL、触摸如何启动。
3. `ap/lv_demo_widgets.c`：看官方 Widgets Demo UI 如何创建。
4. `ap/config/bk7259_ap/defconfig`：看功能开关。
5. `ap/lv_conf_custom.h`：看本工程额外打开了哪些 LVGL 功能。

## 3. 运行效果

固件启动后，AP 侧会自动初始化显示链路并调用 `lv_demo_widgets()`，屏幕上会出现 LVGL 官方 Widgets Demo 界面。

你可以通过界面看到：

| 模块 | 说明 |
| --- | --- |
| 按钮、开关、滑条 | 基础交互控件 |
| 列表、菜单、Tab 页面 | 常见页面组织方式 |
| 图片资源 | 如何在工程中编译并使用 C 数组图片 |
| 字体 | 工程额外启用了 Montserrat 12/16/18/20/24 字号 |
| 性能监视 | `CONFIG_LV_USE_PERF_MONITOR=y` 时可查看 FPS/CPU 等信息 |

## 4. 快速上手

### 4.1 准备环境

请先确认：

1. 已进入 SDK 根目录：`bk_avdk_smp_release_4.0.1/`
2. 编译环境已经配置好。
3. 开发板连接了与工程匹配的 MIPI 屏和触摸屏。

### 4.2 编译

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=lvgl/widgets_v8
```

也可以按 CI 使用的方式执行：

```bash
./dbuild.sh make bk7259 PROJECT=lvgl/widgets_v8
```

编译成功后，固件会生成在 `build/` 目录下。

> 注意：当前 `widgets_v8/CMakeLists.txt` 中的 `project()` 名称写的是 `widgets_v9`，因此部分构建输出目录或日志里可能出现 `widgets_v9` 字样。实际编译入口仍然是 `PROJECT=lvgl/widgets_v8`。

### 4.3 烧录

使用 Beken 烧录工具或 SDK 对应烧录流程，将编译出的 BK7259 固件烧录到开发板。

如果你使用项目内已有烧录脚本或自动化工具，请选择 `lvgl/widgets_v8` 这个工程。

### 4.4 运行

烧录完成后上电或复位开发板：

1. 串口日志应能看到系统启动、media service 初始化、LVGL 初始化等信息。
2. 屏幕背光会被拉高，默认使用 `GPIO_7`。
3. LCD 打开后显示 LVGL Widgets Demo。
4. 如果触摸屏与默认配置匹配，可以直接点击、滑动 Demo 界面。

## 5. 启动流程

`ap/ap_main.c` 是理解本工程的核心入口。

启动流程如下：

1. `main()` 调用 `bk_init()` 初始化系统。
2. 调用 `media_service_init()` 初始化多媒体服务。
3. 调用 `bk_lodoen_enable()` 配置相关电源寄存器。
4. 调用 `bk_frame_buffer_init()` 初始化帧缓冲。
5. 注册 `widgets` CLI 命令。
6. 调用 `lvgl_app_widgets_init()` 初始化显示、触摸和 LVGL。

`lvgl_app_widgets_init()` 内部主要做这些事：

1. 创建 DSI bus：`bk_display_dsi_bus_new()`
2. 创建 MIPI LCD panel：`bk_lcd_mipi_panel_new()`
3. 创建 DPU 显示控制器：`bk_display_dpu_ctlr_new()`
4. 初始化并打开显示：`bk_display_init()`、`bk_display_open()`
5. 打开背光：`GPIO_7` 输出高电平
6. 配置 `lv_vnd_config_t`
7. 申请 LVGL frame buffer
8. 调用 `lv_vendor_init()` 初始化 LVGL 适配层
9. 如开启触摸，调用 `drv_tp_open()`
10. 加锁后调用 `lv_demo_widgets()` 创建 UI
11. 调用 `lv_vendor_start()` 启动 LVGL 线程

## 6. 关键配置说明

### 6.1 功能开关

关键配置在 `ap/config/bk7259_ap/defconfig`：

| 配置项 | 作用 |
| --- | --- |
| `CONFIG_LVGL=y` | 启用 LVGL |
| `CONFIG_LVGL_V8=y` | 选择 LVGL V8 |
| `CONFIG_BK_DISPLAY=y` | 启用显示框架 |
| `CONFIG_MIPI_DSI=y` | 启用 MIPI DSI |
| `CONFIG_DPU_DRIVER=y` | 启用 DPU 显示控制器 |
| `CONFIG_FRAME_BUFFER=y` | 启用帧缓冲 |
| `CONFIG_LCD_HX8399C_MIPI_1080x1920=y` | 启用默认 MIPI 屏驱动 |
| `CONFIG_TP=y` | 启用触摸 |
| `CONFIG_LVGL_FRAME_BUFFER_NUM=2` | LVGL 使用 2 个 frame buffer |
| `CONFIG_LVGL_TASK_STACK_SIZE=8192` | LVGL 线程栈大小 |

### 6.2 LVGL 工程级配置

`ap/lv_conf_custom.h` 会先包含 SDK 默认的 `lv_conf.h`，再覆盖本工程需要的选项：

| 配置项 | 作用 |
| --- | --- |
| `LV_USE_DEMO_WIDGETS=1` | 打开 LVGL 官方 Widgets Demo |
| `LV_FONT_MONTSERRAT_24=1` | 启用 24 号字体 |
| `LV_FONT_MONTSERRAT_20=1` | 启用 20 号字体 |
| `LV_FONT_MONTSERRAT_18=1` | 启用 18 号字体 |
| `LV_FONT_MONTSERRAT_16=1` | 启用 16 号字体 |
| `LV_FONT_MONTSERRAT_12=1` | 启用 12 号字体 |

如果你想启用更多 LVGL 功能，优先在这个文件里做工程级覆盖，不建议直接修改 LVGL 组件的公共配置。

## 7. 常见修改

### 7.1 修改屏幕型号

需要同时改 3 类位置：

1. `ap/ap_main.c` 中的 LCD 头文件，例如当前为 `lcd_mipi_hx8399c_1080x1920.h`。
2. `ap/ap_main.c` 中 `bk_lcd_mipi_panel_new()` 使用的 LCD 设备对象。
3. `ap/config/bk7259_ap/defconfig` 中对应的 `CONFIG_LCD_xxx` 驱动开关。

如果分辨率变化，还要同步修改：

```c
#define WIDTH (1080)
#define HEIGHT (1920)
```

### 7.2 修改背光和复位 GPIO

| 功能 | 默认值 | 修改位置 |
| --- | --- | --- |
| 背光 | `GPIO_7` | `ap/ap_main.c` 中 `bk_gpio_set_output_high(GPIO_7)` 附近 |
| LCD Reset | `GPIO_60` | `ap/ap_main.c` 中 `panel_config.reset_pin` |
| TP Reset/INT/I2C | `GPIO_5/6/1/0` | `ap/config/bk7259_ap/defconfig` |

### 7.3 替换成自己的 UI

最简单的做法：

1. 保留 `lv_vendor_init()`、`drv_tp_open()`、`lv_vendor_start()` 等启动流程。
2. 将 `lv_demo_widgets()` 替换成你自己的页面创建函数，例如 `my_ui_create()`。
3. 如果 UI 创建在其他 task 中执行，必须使用 `lv_vendor_disp_lock()` 和 `lv_vendor_disp_unlock()` 保护 LVGL 调用。

示例：

```c
lv_vendor_disp_lock();
my_ui_create();
lv_vendor_disp_unlock();
```

### 7.4 添加图片资源

参考 `ap/assets/` 中已有的 `.c` 图片文件：

1. 将图片转换为 LVGL 可用的 C 数组资源。
2. 放到 `ap/assets/` 目录。
3. 在 `ap/CMakeLists.txt` 的 `srcs` 列表中加入新的资源 `.c` 文件。
4. 在 UI 代码中声明并使用该图片资源。

## 8. 常见问题

### 8.1 编译失败，提示找不到 LVGL Demo 或字体

检查：

1. `ap/lv_conf_custom.h` 是否开启了 `LV_USE_DEMO_WIDGETS`。
2. 使用到的字体是否在 `lv_conf_custom.h` 中开启。
3. 新增源码或资源是否加入 `ap/CMakeLists.txt`。

### 8.2 屏幕黑屏

优先检查：

1. 屏幕型号是否与 `lcd_device_hx8399c_mipi_1080x1920` 匹配。
2. MIPI 屏线、供电、背光是否正确。
3. `GPIO_7` 是否真的是当前硬件的背光控制脚。
4. `GPIO_60` 是否真的是当前硬件的 LCD reset 脚。
5. `defconfig` 中是否打开了对应 LCD 驱动。

### 8.3 触摸无反应

优先检查：

1. `CONFIG_TP=y` 是否开启。
2. `CONFIG_TP_RST_PIN`、`CONFIG_TP_INT_PIN`、`CONFIG_TP_I2C_SDA_PIN`、`CONFIG_TP_I2C_SCL_PIN` 是否匹配硬件。
3. 当前触摸 IC 是否在 `defconfig` 中开启。
4. `drv_tp_open()` 是否被执行。

### 8.4 UI 偶发异常或死机

LVGL 默认不是线程安全的。凡是在 LVGL 线程以外创建或修改 UI，都要加锁：

```c
lv_vendor_disp_lock();
/* LVGL API calls */
lv_vendor_disp_unlock();
```

如果 UI 很复杂，出现栈溢出或异常，可以适当增大 `CONFIG_LVGL_TASK_STACK_SIZE`。

## 9. widgets_v8 与 widgets_v9 的区别

| 对比项 | `widgets_v8` | `widgets_v9` |
| --- | --- | --- |
| LVGL 版本 | V8.3.9 | V9.3.0 |
| 功能开关 | `CONFIG_LVGL_V8=y` | `CONFIG_LVGL_V9=y` |
| Demo 代码 | LVGL V8 Widgets Demo | LVGL V9 Widgets Demo |
| 像素格式 | `BK_PIXEL_FORMAT_RGB565` | `BK_PIXEL_FORMAT_ARGB8888` |
| DPU 解压 | 默认关闭 | 默认开启 |
| 适合场景 | 维护或验证基于 LVGL V8 的 UI | 新项目优先参考 LVGL V9 |

如果是新项目且没有历史包袱，建议优先参考 `widgets_v9`；如果已有 UI 基于 LVGL V8 开发，则参考本工程更合适。

