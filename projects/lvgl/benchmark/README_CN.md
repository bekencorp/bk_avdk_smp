# LVGL Benchmark 工程中文说明

本工程运行 LVGL 官方 benchmark 演示，用于在 BK7258 平台上评估 LVGL 控件绘制、动画和屏幕刷新性能。

## 1. 目录结构
```
benchmark/
├── CMakeLists.txt                 # 顶层 CMake 构建入口
├── Makefile                       # Make 构建入口
├── ap/                            # AP 核业务代码
│   ├── ap_main.c                  # LVGL、LCD、触摸和 benchmark 入口初始化
│   └── config/                    # BK7258 AP 侧配置
├── cp/                            # CP 核配置
│   └── config/
└── partitions/                    # 分区与 RAM 区域配置
```

## 2. 功能特点
- 调用 `lv_demo_benchmark()` 启动 LVGL benchmark 测试界面。
- 默认使用 ST7701S RGB LCD，分辨率由 LCD 驱动描述决定。
- 通过 `lv_vendor` 使用局部刷新模式，并按 `CONFIG_LVGL_FRAME_BUFFER_NUM` 分配 framebuffer。
- 可选启用触摸屏，便于观察和控制 demo 交互。
- 启动时初始化 `media_service` 并打开 LVGL 代码运行所需 PSRAM。

## 3. 硬件与配置
- 硬件：BK7258 开发板、ST7701S RGB LCD、触摸屏（可选）。
- LCD 控制：默认 GPIO0/GPIO12/GPIO1/GPIO6，背光 GPIO7，LCD LDO GPIO13。
- 软件配置：确认 `CONFIG_LVGL` 与 `CONFIG_LV_USE_DEMO_BENCHMARK` 已开启；如需触摸，开启 `CONFIG_TP`。

## 4. 编译与烧录
```
make bk7258 PROJECT=lvgl/benchmark
```

烧录固件后复位开发板，benchmark demo 会在 LVGL 初始化完成后自动运行。

## 5. 运行流程
1. 连接 LCD 和可选触摸屏。
2. 编译并烧录工程固件。
3. 复位后观察屏幕和串口日志，等待 benchmark 测试页面启动。
4. 记录屏幕显示的 benchmark 结果，用于比较不同 LVGL 配置、buffer 数量或显示驱动优化效果。

## 6. 常见问题
- **benchmark 未显示**：确认 `CONFIG_LVGL` 和 `CONFIG_LV_USE_DEMO_BENCHMARK` 已启用。
- **显示异常**：检查 LCD 型号、RGB 时序和控制引脚是否与硬件一致。
- **性能波动较大**：保持相同编译优化、framebuffer 数量、屏幕分辨率和后台负载后再比较测试结果。
