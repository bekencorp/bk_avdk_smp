# LVGL Widgets 工程中文说明

本工程运行 LVGL 官方 widgets 演示，用于在 BK7258 平台上展示基础控件、主题样式、列表、图表和交互组件。

## 1. 目录结构
```
widgets/
├── CMakeLists.txt                 # 顶层 CMake 构建入口
├── Makefile                       # Make 构建入口
├── ap/                            # AP 核业务代码
│   ├── ap_main.c                  # LVGL、LCD、触摸和 widgets demo 初始化
│   ├── assets/                    # widgets demo 图片资源
│   └── config/                    # BK7258 AP 侧配置
├── cp/                            # CP 核配置
│   └── config/
└── partitions/                    # 分区与 RAM 区域配置
```

## 2. 功能特点
- 调用 `lv_demo_widgets()` 创建 LVGL widgets 演示界面。
- 展示按钮、滑块、列表、图表、图片等常用 LVGL 控件。
- 默认适配 ST7701S RGB LCD，使用 `lv_vendor` 局部刷新模式。
- 支持 `CONFIG_TP` 触摸输入，便于验证控件交互。
- 启动时打开 LCD LDO、背光和 PSRAM 供电。

## 3. 硬件与配置
- 硬件：BK7258 开发板、ST7701S RGB LCD、触摸屏（可选）。
- LCD 控制：默认 GPIO0/GPIO12/GPIO1/GPIO6，背光 GPIO7，LCD LDO GPIO13。
- 软件配置：确认 `CONFIG_LVGL` 和 `CONFIG_LV_USE_DEMO_WIDGETS` 已开启；如需触摸，开启 `CONFIG_TP`。

## 4. 编译与烧录
```
make bk7258 PROJECT=lvgl/widgets
```

烧录固件后复位开发板，LVGL 初始化完成后会自动进入 widgets demo。

## 5. 运行流程
1. 连接 LCD、触摸屏和开发板。
2. 编译并烧录工程固件。
3. 复位后观察串口日志，确认 framebuffer、LCD 和触摸初始化成功。
4. 在界面中滑动、点击控件，验证 LVGL 控件展示与触摸响应。

## 6. 常见问题
- **控件资源显示异常**：确认 `ap/assets` 中的资源已参与编译且未被裁剪。
- **触摸坐标不准**：检查触摸分辨率和镜像参数是否与 LCD 方向一致。
- **界面刷新卡顿**：检查 PSRAM、framebuffer 数量和系统后台任务负载。
