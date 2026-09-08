# LVGL Benchmark v9 工程中文说明

本工程运行 LVGL v9 版本的官方 benchmark 演示，用于在 BK7258 平台上验证 LVGL v9 控件绘制、动画和刷新性能。

## 1. 目录结构
```
benchmark_v9/
├── CMakeLists.txt                 # 顶层 CMake 构建入口
├── Makefile                       # Make 构建入口
├── ap/                            # AP 核业务代码
│   ├── ap_main.c                  # LVGL v9、LCD、触摸和 benchmark 入口初始化
│   └── config/                    # BK7258 AP 侧配置
├── cp/                            # CP 核配置
│   └── config/
└── partitions/                    # 分区与 RAM 区域配置
```

## 2. 功能特点
- 调用 `lv_demo_benchmark()` 启动 LVGL v9 benchmark 测试界面。
- 默认适配 ST7701S RGB LCD，LCD 时序和引脚在 `ap/ap_main.c` 中定义。
- 使用 `lv_vendor` 局部刷新模式，并检查 `lv_vendor_init()` 返回值，便于定位初始化失败。
- 可通过 `CONFIG_TP` 启用触摸输入。
- 上电后初始化媒体服务，并为 LVGL 代码运行打开 PSRAM 供电。

## 3. 硬件与配置
- 硬件：BK7258 开发板、ST7701S RGB LCD、触摸屏（可选）。
- LCD 控制：默认 GPIO0/GPIO12/GPIO1/GPIO6，背光 GPIO7，LCD LDO GPIO13。
- 软件配置：确认启用 LVGL v9 相关配置、`CONFIG_LVGL` 和 benchmark demo；如需触摸，开启 `CONFIG_TP`。

## 4. 编译与烧录
```
make bk7258 PROJECT=lvgl/benchmark_v9
```

烧录后复位开发板，工程会自动打开 LCD 并进入 LVGL v9 benchmark 界面。

## 5. 运行流程
1. 连接 ST7701S LCD 和可选触摸屏。
2. 编译并烧录固件。
3. 复位后查看串口日志，确认 `lv_vendor_init`、LCD 打开和 framebuffer 分配成功。
4. 等待 benchmark 跑完后记录结果，用于对比 LVGL v8/v9 或不同显示配置。

## 6. 常见问题
- **初始化失败**：优先查看串口中 `lv_vendor_init fail` 和 framebuffer 分配错误。
- **无显示或花屏**：确认屏型号、RGB 时序、GPIO 引脚和背光控制与硬件一致。
- **结果不可比**：比较 v8/v9 时应保持分辨率、buffer 数量、编译优化和后台任务一致。
