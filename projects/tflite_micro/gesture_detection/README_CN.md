# TensorFlow Lite Micro 石头剪刀布手势检测工程

- [English](./README.md)

## 1. 项目概述

本项目在 Beken 平台上演示基于摄像头的 **石头/布/剪刀手势检测交互应用**。工程使用 `GestureDetectionModel` 执行 TFLM 手势识别，通过 `app_event` 管理游戏状态、提示音播放与结果显示，并使用 LVGL 生成界面展示倒计时、手势图片和输赢结果。

### 1.1 测试环境

- 硬件：
  - BK7259 系列核心板
  - USB 摄像头或 MIPI 摄像头（当前 defconfig 开启 `CONFIG_USB_CAMERA=y`）
  - MIPI 显示输出，当前配置 LT8912B bridge，1280×720
  - 音频 DAC，用于播放提示音
- 软件：
  - `CONFIG_TFLITE_MICRO=y`
  - `CONFIG_TFLM_GESTURE_DETECTION_V1=y`
  - `CONFIG_LVGL=y`、`CONFIG_AUDIO=y`
  - 当前提示音语言为 `CONFIG_GESTURE_DET_PROMPT_TONE_LANG_ENGLISH=y`

.. warning::

```
本工程是完整交互 demo，依赖摄像头、显示、LVGL、音频与模型资源。若只验证模型本身，可优先使用 CLI 模拟事件排查 app_event 状态机。
```

## 2. 目录结构

```
gesture_detection/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── CMakeLists.txt
│   ├── Kconfig.projbuild                 # 提示音语言选择
│   ├── ap_main.cc                        # 摄像头/显示/UI/模型主流程
│   ├── app_event.c                       # 游戏状态机、提示音、图片缓存
│   ├── app_event.h
│   ├── app_event_test_cli.c              # rps_* 测试命令
│   ├── resource/prompt_tone/             # CN/EN 提示音 C 数组
│   └── ui/beken_generated/               # LVGL 生成界面与图片资源
├── cp/
└── partitions/bk7259/
```

公共模型组件：

```
common_components/avdk_nn_module/
├── include/GestureDetectionModel.h
├── include/tflm_gesture_detection.h
└── src/tflm_gesture_detection/
    ├── GestureDetectionModel.cc
    └── gesture_detection_model_data.cc
```

## 3. 功能说明

### 3.1 主要功能

- 使用 `GestureDetectionModel` 检测 `rock`、`paper`、`scissors`，无法识别时返回 `GESTURE_NONE`
- `app_event` 管理获取准备、检测窗口、提示用户停手、播放结果、回到空闲等状态
- 支持提示音资源：`cant_det`、`detecting`、`get_ready`、`rock`、`paper`、`scissors`、`you_win` 等
- LVGL UI 展示游戏画面、内置手势图片和检测截图
- 提供 `rps_*` CLI 命令，用于无按键或调试时模拟游戏事件

### 3.2 处理流程

1. `bk_init()` → `media_service_init()` → `bk_auxldo_enable()` → `bk_frame_buffer_init()`
2. 配置摄像头、显示、GPU 与设备管理
3. `app_event_init()` 初始化事件队列、播放器与状态机，并注册 CLI
4. 创建 `GestureDetectionModel`，注册手势结果与图像回调
5. 初始化 LVGL UI，启动显示链路
6. 首轮上电自动投递 `APP_EVENT_PROMPT_GET_READY`

## 4. 编译与运行

### 4.1 编译

在 SDK 根目录：

```
make bk7259 PROJECT=tflite_micro/gesture_detection
```

### 4.2 运行与交互

烧录后工程会自动进入首轮游戏流程。典型观察点：

```
M55 main running...
rps test CLI registered: rps_start, rps_end, rps_rule, rps_gesture, rps_prompt
======Paper is detected...
======Rock is detected...
======Scissors is detected...
```

### 4.3 CLI

本工程在终端中执行 CLI 时，须在命令最前面加上 `ap_cmd`（由 shell 转发至 AP）。可通过串口 CLI 触发或模拟流程：

```
ap_cmd rps_start
ap_cmd rps_end
ap_cmd rps_rule
ap_cmd rps_gesture rock|paper|scissors
ap_cmd rps_prompt <name>
```

`rps_prompt` 支持的名称包括：`cant_det`、`detecting`、`get_ready`、`my_show`、`paper`、`pls_stop_hand`、`rock`、`scissors`、`you_loss`、`you_win`、`your_show`、`draw`、`game_rule`。

## 5. 测试方案

- UI：确认 LVGL 界面正常显示，手势图片和结果页能切换。
- 摄像头：确认 USB/MIPI 摄像头画面链路正常。
- 语音：执行 `ap_cmd rps_prompt get_ready`、`ap_cmd rps_prompt rock` 检查提示音。
- 模型：手势进入检测窗口后，日志出现对应 `Paper/Rock/Scissors is detected`。
- 状态机：用 `ap_cmd rps_start`、`ap_cmd rps_end`、`ap_cmd rps_gesture` 验证各状态切换。

### 模型、UI 与提示音资源对应修改

.. list-table::
   :header-rows: 1

   * - 内容
     - 说明
   * - `gesture_detection_model_data.cc` / `.h`
     - 存放 Vela 模型数组 `gesture_detection_tflite[]` 与 `gesture_detection_tflite_size`；替换模型时需保持符号一致。
   * - `GestureDetectionModel::resourceLoad()`
     - 维护模型输入尺寸、像素格式、模型位置、fast memory、arena 位置与大小；当前输入为 **192×192**。
   * - `GestureDetectionModel::post_process()`
     - 解析输出中的 score、paper、rock、scissors，并把结果回调给 `app_event`；更换输出布局或类别顺序时必须同步。
   * - `ap/resource/prompt_tone/`
     - CN/EN 提示音通过 `CONFIG_GESTURE_DET_PROMPT_TONE_LANG_*` 选择；新增提示音时需同步 CMake 源文件列表和 `resource.h`。
   * - `ap/ui/beken_generated/`
     - LVGL 生成页面与图片资源；替换分辨率、图片或页面逻辑后需重新验证显示与触摸。
   * - `app_event.c`
     - 管理游戏状态机、提示音序列、截图缓存和结果判定；调试时优先用 `ap_cmd rps_*` 命令定位状态流。

回归时建议按顺序验证：提示音播放、UI 切页、摄像头输入、模型识别、`app_event` 状态切换、截图显示和长时间运行内存稳定性。

## 6. 配置说明

- 摄像头路径：`CONFIG_USB_CAMERA` 决定使用 `OpenUVCCameraWithDisplay()` 或 MIPI 路径。
- 提示音语言：`ap/Kconfig.projbuild` 中 `GESTURE_DET_PROMPT_TONE_LANG`。
- UI 资源：`ap/ui/beken_generated/`。
- 提示音资源：`ap/resource/prompt_tone/cn` 与 `ap/resource/prompt_tone/en`。
- 模型开关：`CONFIG_TFLM_GESTURE_DETECTION_V1`。

当前默认内存位置与大小（见 `GestureDetectionModel::resourceLoad()`）：

.. list-table::
   :header-rows: 1

   * - 类型
     - 位置
     - 大小
     - 来源
   * - fast memory / Ethos-U scratch
     - HSRAM
     - **128 KB**
     - `fast_ram_type` / `fast_ram_data_size`
   * - tensor arena
     - PSRAM_SLAB（`MEM_SLAB_HEAP_CODED`）
     - **600 KB**
     - `arena_ram_type` / `arena_data_size`
   * - Vela model copy
     - PSRAM_SLAB（`MEM_SLAB_HEAP_CODED`）
     - **823,456 bytes**
     - `model_ram_type` / `gesture_detection_tflite_size`

## 7. 注意事项

1. 当前 defconfig 使用英文提示音，如需中文需切换 Kconfig 语言选项。
2. `GestureDetectionModel` 输入尺寸当前为 **192×192**，更换模型时需检查后处理阈值与输出布局。
3. 摄像头、显示、音频同时运行时内存压力较大，移植时需检查 `partitions/bk7259`。
4. CLI 仅用于测试事件流，真实识别仍依赖摄像头图像和模型回调。
