# LVGL 86box 工程中文说明

本工程是基于 LVGL 的 86 盒子界面演示，用于在 BK7258 平台上展示横向页面切换、家居控制类 UI 和触摸交互效果。

## 1. 目录结构
```
86box/
├── CMakeLists.txt                 # 顶层 CMake 构建入口
├── Makefile                       # Make 构建入口
├── ap/                            # AP 核业务代码
│   ├── ap_main.c                  # LVGL、LCD、触摸和页面入口初始化
│   ├── images/                    # 页面图片资源
│   └── config/                    # BK7258 AP 侧配置
├── cp/                            # CP 核配置
│   └── config/
└── partitions/                    # 分区与 RAM 区域配置
```

## 2. 功能特点
- 使用 `lv_vendor` 初始化 LVGL 显示框架，采用局部刷新模式和多帧缓存。
- 默认适配 ST7701S RGB LCD，控制引脚在 `ap/ap_main.c` 的 `rgb_ctlr_config` 中配置。
- 通过 `hor_page_load_main()` 创建 86 盒子主界面，展示多页面 UI 与图片资源。
- 可通过 `CONFIG_TP` 打开触摸屏支持。
- 上电时打开 LCD LDO、背光，并为 LVGL 代码运行打开 PSRAM 供电。

## 3. 硬件与配置
- 硬件：BK7258 开发板、ST7701S RGB LCD、触摸屏（可选）。
- LCD 控制：默认使用 GPIO0/GPIO12/GPIO1/GPIO6 作为 RGB 屏控制信号，GPIO7 控制背光，GPIO13 控制 LCD LDO。
- 软件配置：确认 `CONFIG_LVGL` 已开启；如需触摸，开启 `CONFIG_TP` 并确认触摸驱动匹配硬件。

## 4. 编译与烧录
```
make bk7258 PROJECT=lvgl/86box
```

编译完成后烧录生成固件，上电后工程会自动初始化 LVGL、打开 LCD 并进入 86 盒子演示界面。

## 5. 运行流程
1. 连接 LCD、触摸屏和开发板，确认背光及 LCD LDO 引脚与代码配置一致。
2. 烧录固件并复位开发板。
3. 观察串口日志，确认 LVGL framebuffer 分配、LCD 打开和触摸初始化没有报错。
4. 进入界面后，可通过触摸滑动或点击验证页面交互。

## 6. 常见问题
- **屏幕无显示**：检查 ST7701S 屏型号、RGB 控制引脚、背光 GPIO7 和 LCD LDO GPIO13。
- **触摸无响应**：确认 `CONFIG_TP` 已开启，并检查触摸屏 I2C 接线和坐标镜像配置。
- **启动失败或内存不足**：确认 PSRAM 可用，并根据分辨率调整 `CONFIG_LVGL_FRAME_BUFFER_NUM` 或 framebuffer 分配策略。
