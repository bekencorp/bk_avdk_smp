# TensorFlow Lite Micro 手掌识别工程

- [English](./README.md)

## 1. 项目概述

本项目在 Beken 平台上演示基于摄像头的 **手掌检测、屏幕 OSD 显示与舵机跟踪**。工程使用 `avdk_nn_module` 中的 `PalmDetectionModel`，通过 `AvdkVideoReatorOSD` 打开 MIPI 摄像头与 MIPI 屏，并将检测框绘制到显示画面上；检测到手掌后，选择面积最大的框作为目标，驱动 PWM 舵机跟随手掌中心。

### 1.1 测试环境

- 硬件：
  - BK7259 系列核心板
  - MIPI 摄像头，当前配置为 **1088×1088 @ 15 fps**
  - MIPI LCD，当前使用 `lcd_device_hx8399c_mipi_1080x1920`
  - 可选 PWM 舵机，默认使用 `SERVO_CHAN=0`
- 软件：
  - `CONFIG_TFLITE_MICRO=y`
  - `CONFIG_TFLM_PALM_DETECTION_V1=y`
  - `CONFIG_BK_CAMERA=y`、`CONFIG_CSI_CAMERA=y`、`CONFIG_LCD_DSI=y`

.. warning::

```
摄像头、LCD、PWM 舵机引脚均与当前板级配置绑定。更换模组或屏幕时，请同步检查 ap_main.cc、usr_gpio_cfg.h 与 servo.h。
```

## 2. 目录结构

```
palm_recognition/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.cc                 # 摄像头/显示/GPU/模型主流程
│   ├── servo.c                    # PWM 舵机驱动与跟踪控制
│   ├── servo.h
│   └── config/bk7259_ap/defconfig
├── cp/
└── partitions/bk7259/
```

公共模型组件：

```
common_components/avdk_nn_module/
├── include/PalmDetectionModel.h
└── src/tflm_palm_detection/
    ├── PalmDetectionModel.cc
    └── palm_detect_model_data.cc
```

## 3. 功能说明

### 3.1 主要功能

- 初始化媒体服务、frame buffer、摄像头、显示与 GPU 配置
- 使用 `PalmDetectionModel` 执行 256×256 手掌检测
- 使用 `box_detection_path_build()` 将检测框映射到 1088×1088 OSD 画布
- 多手掌场景下选择面积最大的检测框作为舵机跟踪目标
- 通过 `palm_track_servo()` 根据手掌中心位置增量调整舵机角度

### 3.2 处理流程

1. `bk_init()` → `media_service_init()` → `bk_auxldo_enable()` → `bk_frame_buffer_init()`
2. 配置摄像头、显示、GPU，并调用 `devices_mgmt_init()`
3. `servo_init()` 并将舵机置中
4. 创建 `PalmDetectionModel`，注册检测框回调
5. `AvdkVideoReatorOSD` 打开摄像头和显示，启动检测与显示链路

## 4. 编译与运行

### 4.1 编译

在 SDK 根目录：

```
make bk7259 PROJECT=tflite_micro/palm_recognition
```

### 4.2 运行与日志

烧录后无需 CLI。正常启动后可在日志中看到：

```
M55 main running...
[servo] self-test start, chan=0
PalmDetection: ...
detection_box_cb: count=... target=... score=... xywh=(...)
[track] axis=cx ...
```

屏幕上应显示摄像头画面与手掌检测框；连接舵机时，舵机会随目标手掌位置缓慢调整。

## 5. 测试方案

- 摄像头：画面稳定，无花屏或黑屏。
- 检测：手掌进入画面后出现检测框，日志打印 `PalmDetection` 与 `detection_box_cb`。
- 舵机：目标偏离中心时日志打印 `[track]`，舵机角度变化平滑。

### 模型资源与外设联调建议

.. list-table::
   :header-rows: 1

   * - 内容
     - 说明
   * - `palm_detect_model_data.cc` / `.h`
     - 存放 Vela 模型数组 `palm_detection_builtin_256_integer_quant_vela_tflite[]` 与长度；替换模型时需保持符号与头文件声明一致。
   * - `PalmDetectionModel::resourceLoad()`
     - 维护模型输入宽高、像素格式、模型位置、fast memory、arena 位置与大小；更换模型后需同步 `width` / `height` 与 `arena_data_size`。
   * - `PalmDetectionModel::resolverLoad()`
     - 模型新增算子时需补充 resolver，否则 `AllocateTensors()` 可能失败。
   * - `detection_box_cb()`
     - OSD 映射当前按模型输入 **256×256** 到显示画布 **1088×1088**；更换模型输入或显示画布时需同步修改。
   * - `servo.h` / `servo.c`
     - 舵机方向、增益、死区与单步最大角度需按机械结构调试。

回归时建议同时确认：摄像头预览、检测框位置、最大面积目标选择、舵机方向、长时间运行内存稳定性。

## 6. 配置说明

- 摄像头输入：`ap_main.cc` 中 `camera_board.mipi` 与 `camera_board.isp`。
- 显示输出：`display_board.mipi.panel` 与 `display_board.dpu_video`。
- GPU 旋转/缩放：`gpu_board.flexa`。
- 舵机参数：`servo.h` 中 `SERVO_CHAN`、`SERVO_TRACK_GAIN`、`SERVO_TRACK_MAX_STEP`、`SERVO_TRACK_DIR`。
- 模型开关：`CONFIG_TFLM_PALM_DETECTION_V1`。

当前默认内存位置与大小（见 `PalmDetectionModel::resourceLoad()`）：

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
     - **2 MB**
     - `arena_ram_type` / `arena_data_size`
   * - Vela model copy
     - PSRAM_SLAB（`MEM_SLAB_HEAP_CODED`）
     - **2,207,168 bytes**
     - `model_ram_type` / `palm_detection_builtin_256_integer_quant_vela_tflite_size`

## 7. 注意事项

1. 当前检测输入与 OSD 映射按 256×256 → 1088×1088 处理，修改模型尺寸时需同步调整映射。
2. 舵机方向不符合实物安装时，可调整 `SERVO_TRACK_DIR`。
3. 若舵机 GPIO 复用冲突，可启用并修改 `SERVO_USE_CUSTOM_GPIO` 相关配置。
4. 该工程依赖摄像头、显示和模型资源，移植到其它板卡时应优先检查内存与外设配置。
