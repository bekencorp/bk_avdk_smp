# 移动侦测示例工程

* [English](./README.md)

## 1. 项目概述

本项目演示 Beken 平台上基于摄像头图像的移动侦测流程。工程从 ISP 获取 NV12 图像，使用灰度帧差算法判断画面处于 `moving` 或 `still` 状态，并可同时通过 GPU 和 MIPI 屏预览摄像头画面。

当前工程提供：

- SP 检测、MP 预览的完整演示模式
- 不启动 GPU 和显示的 MP-only 检测模式
- `320x180` NV12 灰度移动侦测
- 移动区域计数、3x3 区域统计和处理耗时日志
- 下一帧检测图像的十六进制 dump 功能

### 1.1 测试环境

- 核心板：`BK7259_QF128_12.3X12.3_V4.0`
- PSRAM：32 MB
- 摄像头：CSI GC2053，默认 `1280x720@20fps`
- 显示屏：MIPI DSI ER68576B，`720x1280`
- 检测输入：`320x180` NV12，算法使用 Y 分量
- 图形模块：VG-Lite GPU、DPU/MIPI

> 请优先使用参考外设。更换 sensor、屏幕或 GPIO 后，需要同步修改 `ap_main.c` 中的板级配置和工程 Kconfig。

## 2. 目录结构

```text
motion_detect/
├── .ci                         # CI 编译命令
├── CMakeLists.txt              # 项目级 CMake 配置
├── Makefile                    # Make 构建入口
├── README.md                   # 英文说明
├── README_CN.md                # 中文说明
├── ap/
│   ├── ap_main.c               # AP 初始化和板级配置
│   ├── CMakeLists.txt
│   ├── config/                 # AP 默认配置
│   ├── include/
│   └── src/motion_detect_demo.c# 移动侦测和 CLI 实现
├── cp/                         # CP 启动及校准代码
└── partitions/                 # 分区和 RAM 区域配置
```

## 3. 功能说明

### 3.1 `start` 完整模式

1. 启动 MIPI sensor 和 ISP MP 通道
2. 新建 `320x180` NV12 SP 通道用于检测
3. 启动 MIPI 显示、GPU 和 ISP-GPU bond，显示 MP 预览
4. 检测任务每秒读取一帧 SP 图像
5. 首帧保存为参考帧，后续帧与参考帧执行灰度差分
6. 检测到运动时更新参考帧，并打印运动状态和区域统计

### 3.2 `start_mp` MP-only 模式

该模式临时将 ISP MP 配置为 `320x180` NV12，直接读取 MP 图像进行移动侦测，不启动 SP、GPU 和显示。执行 `stop` 后恢复原有摄像头配置。

### 3.3 默认算法参数

- 图像尺寸：`320x180`
- 单像素差异阈值：`64`
- 运动计数阈值：`128`
- 处理周期：约 1 秒一帧
- 参考帧策略：首帧作为参考帧，检测到运动后更新参考帧

日志中的 `total` 为运动计数，`max_3x3`、`rows` 和 `cols` 用于描述运动区域分布。

## 4. 编译与运行

### 4.1 编译

在 SDK 根目录执行：

```bash
make bk7259 PROJECT=multimedia/motion_detect
```

### 4.2 CLI 命令

固件烧录并启动后，通过串口终端执行：

```text
ap_cmd motion_detect help
ap_cmd motion_detect start
ap_cmd motion_detect start_mp
ap_cmd motion_detect stop
ap_cmd motion_detect dump
```

- `start`：启动 SP 检测和 MP 屏幕预览
- `start_mp`：启动 MP-only 检测，不开启显示
- `stop`：停止任务并释放 camera、GPU、display 和 frame buffer 资源
- `dump`：请求以十六进制打印下一帧检测图像
- `CMDRSP:OK` 表示命令执行成功，`CMDRSP:ERROR` 表示参数错误或资源启动失败

## 5. 测试示例

### 5.1 带预览的移动侦测

```text
ap_cmd motion_detect start
```

保持画面静止或在镜头前移动物体，预期日志类似：

```text
motion_detect frame=2 state=still total=... max_3x3[...]=... rows=... cols=...
motion_detect frame=3 state=moving total=... max_3x3[...]=... rows=... cols=...
```

测试结束后执行：

```text
ap_cmd motion_detect stop
```

### 5.2 MP-only 检测

```text
ap_cmd motion_detect start_mp
ap_cmd motion_detect stop
```

### 5.3 Dump 检测输入

先启动任一检测模式，再执行：

```text
ap_cmd motion_detect dump
```

下一帧的 `320x180` Y 分量会通过串口输出。数据量较大，输出期间会影响实时性。

## 6. 配置说明

工程默认使能 ISP、MIPI CSI、frame buffer、VG-Lite GPU、DPU、MIPI DSI、GC2053、ER68576B 和 media service。主要板级参数位于 `ap/ap_main.c`：

- sensor GPIO、I2C、分辨率和帧率
- ISP MP 尺寸与格式
- MIPI 屏型号及 GPIO
- GPU 输入/输出格式和 90 度旋转

算法尺寸及阈值定义在 `ap/src/motion_detect_demo.c` 的 `MOTION_DETECT_*` 宏中。

## 7. 注意事项

1. `start` 与 `start_mp` 不能同时运行；重复启动会返回 busy。
2. `start` 占用 camera、ISP、GPU、DPU、显示和帧缓冲资源，切换模式前应先执行 `stop`。
3. 移动侦测只使用 NV12 的 Y 分量；修改尺寸时需同步调整 ISP 输出和算法配置。
4. 默认阈值适用于演示环境，光照闪烁、自动曝光和摄像头抖动可能被判断为运动。
5. `dump` 会输出 57,600 字节数据，仅建议用于调试。
