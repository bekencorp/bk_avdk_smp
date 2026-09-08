# LVGL Widgets v9 工程中文说明

本工程运行 LVGL v9 版本的官方 widgets 演示，用于在 BK7258 平台上展示 LVGL v9 控件、主题和触摸交互效果。

## 1. 目录结构
```
widgets_v9/
├── CMakeLists.txt                 # 顶层 CMake 构建入口
├── Makefile                       # Make 构建入口
├── ap/                            # AP 核业务代码
│   ├── ap_main.c                  # LVGL v9、LCD、触摸和 widgets demo 初始化
│   └── config/                    # BK7258 AP 侧配置
├── cp/                            # CP 核配置
│   └── config/
└── partitions/                    # 分区与 RAM 区域配置
```

## 2. 功能特点
- 调用 `lv_demo_widgets()` 创建 LVGL v9 widgets 演示界面。
- 展示 LVGL v9 控件、主题、布局和交互组件。
- 默认适配 `lcd_device_h050iwv` RGB LCD，控制引脚在 `ap/ap_main.c` 中配置。
- 通过 `lv_vendor` 使用局部刷新模式，并按 `CONFIG_LVGL_FRAME_BUFFER_NUM` 分配 framebuffer。
- 支持 `CONFIG_TP` 触摸输入。

## 3. 硬件与配置
- 硬件：BK7258 开发板、`lcd_device_h050iwv` 对应 RGB LCD、触摸屏（可选）。
- LCD 控制：默认 GPIO0/GPIO12/GPIO1/GPIO6，背光 GPIO7，LCD LDO GPIO13。
- 软件配置：确认启用 LVGL v9、`CONFIG_LVGL` 和 `CONFIG_LV_USE_DEMO_WIDGETS`；如需触摸，开启 `CONFIG_TP`。

## 4. 编译与烧录
```
make bk7258 PROJECT=lvgl/widgets_v9
```

烧录固件后复位开发板，工程会自动启动 LVGL v9 widgets demo。

## 5. 运行流程
1. 连接 LCD、触摸屏和开发板。
2. 编译并烧录固件。
3. 复位后观察串口日志，确认 framebuffer 分配、LCD 打开和触摸初始化成功。
4. 进入界面后点击或滑动控件，验证 LVGL v9 控件和触摸响应。

## 6. 常见问题
- **屏幕无显示或分辨率不匹配**：确认实际屏幕与 `lcd_device_h050iwv` 配置一致。
- **触摸方向错误**：检查 `drv_tp_open()` 中传入的分辨率和镜像参数。
- **控件显示或主题异常**：确认当前工程使用的是 LVGL v9 相关配置和 demo 资源。
