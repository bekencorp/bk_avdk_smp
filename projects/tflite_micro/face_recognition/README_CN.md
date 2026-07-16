# TensorFlow Lite Micro 人脸识别工程

- [English](./README.md)

## 1. 项目概述

本项目在 Beken 平台上演示基于摄像头的 **YOLOFace 人脸检测与屏幕 OSD 显示**。工程使用 `avdk_nn_module` 中的 `YolofaceDetectionModel`，通过 `AvdkVideoReatorOSD` 打开 MIPI 摄像头与 MIPI 屏，并将检测框绘制到显示画面上。

### 1.1 测试环境

- 硬件：
  - BK7259 系列核心板
  - MIPI 摄像头，当前配置为 **1088×1088 @ 15 fps**
  - MIPI LCD，当前使用 `lcd_device_hx8399c_mipi_1080x1920`
- 软件：
  - `CONFIG_TFLITE_MICRO=y`
  - `CONFIG_TFLM_YOLOFACE_V1=y`
  - `CONFIG_BK_CAMERA=y`、`CONFIG_CSI_CAMERA=y`、`CONFIG_LCD_DSI=y`

.. warning::

```
本工程名称为 face_recognition，但当前应用侧实现是摄像头人脸检测与 OSD 框显示，不包含人脸特征比对或身份注册流程。
```

## 2. 目录结构

```
face_recognition/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.cc                 # 摄像头/显示/GPU/模型主流程
│   └── config/bk7259_ap/defconfig
├── cp/
└── partitions/bk7259/
```

公共模型组件：

```
common_components/avdk_nn_module/
├── include/YolofaceDetectionModel.h
└── src/tflm_yoloface_detection/
    ├── YolofaceDetectionModel.cc
    └── yoloface_detect_model_data.cc
```

## 3. 功能说明

### 3.1 主要功能

- 初始化媒体服务、frame buffer、摄像头、显示与 GPU 配置
- 使用 `YolofaceDetectionModel` 执行人脸检测
- 检测输入尺寸当前为 **56×56**，检测框映射到 1088×1088 显示画布
- 使用 `box_detection_path_build()` 绘制全部保留的人脸框
- 在日志中打印最高分框的 score 与 xywh

### 3.2 处理流程

1. `bk_init()` → `media_service_init()` → `bk_auxldo_enable()` → `bk_frame_buffer_init()`
2. 配置摄像头、显示、GPU，并调用 `devices_mgmt_init()`
3. 创建 `YolofaceDetectionModel`，注册检测框回调
4. `AvdkVideoReatorOSD` 打开摄像头和显示
5. 启动检测与 OSD 显示链路

## 4. 编译与运行

### 4.1 编译

在 SDK 根目录：

```
make bk7259 PROJECT=tflite_micro/face_recognition
```

### 4.2 运行与日志

烧录后无需 CLI。典型日志关键字如下：

```
AP main running...
detection_box_cb: score=..., x=..., y=..., w=..., h=...
```

屏幕上应显示摄像头预览，人脸进入画面后出现检测框。

## 5. 测试方案

- 摄像头：确认预览稳定。
- 检测：人脸进入画面后，日志打印 `detection_box_cb`，屏幕显示检测框。
- 稳定性：长时间运行无 `Invoke failed`、内存分配失败或显示异常。

### 模型资源与检测链路对应修改

.. list-table::
   :header-rows: 1

   * - 内容
     - 说明
   * - `yoloface_detect_model_data.cc` / `.h`
     - 存放 Vela 模型数组 `g_yoloface_int8_vela_tflite[]` 与 `YOLOFACE_MODEL_DATA_SIZE`；替换模型时需保持符号与长度一致。
   * - `YolofaceDetectionModel::resourceLoad()`
     - 维护模型输入尺寸、像素格式、模型位置、fast memory、arena 位置与大小；当前模型输入为 **56×56**。
   * - `YolofaceDetectionModel::resolverLoad()`
     - 新模型若使用额外算子，需补充 resolver，否则 `AllocateTensors()` 可能失败。
   * - `YolofaceDetectionModel::run()`
     - 包含输入格式检查、预处理和后处理；更换输出布局、anchor 或阈值时需同步修改。
   * - `detection_box_cb()`
     - OSD 映射当前按模型输入 **56×56** 到显示画布 **1088×1088**；更换模型或画布尺寸时需同步修改。

回归时建议确认：摄像头预览、人脸框位置、多人脸场景、长时间运行稳定性，以及无 `AllocateTensors failed` / `Invoke failed`。

## 6. 配置说明

- 摄像头输入：`ap_main.cc` 中 `camera_board.mipi` 与 `camera_board.isp`。
- 显示输出：`display_board.mipi.panel` 与 `display_board.dpu_video`。
- GPU 旋转/缩放：`gpu_board.flexa`。
- 模型开关：`CONFIG_TFLM_YOLOFACE_V1`。
- 模型资源：`yoloface_detect_model_data.cc`。

当前默认内存位置与大小（见 `YolofaceDetectionModel::resourceLoad()`）：

.. list-table::
   :header-rows: 1

   * - 类型
     - 位置
     - 大小
     - 来源
   * - fast memory / Ethos-U scratch
     - HSRAM
     - **40 KB**
     - `fast_ram_type` / `fast_ram_data_size`
   * - tensor arena
     - HSRAM
     - **80 KB**
     - `arena_ram_type` / `arena_data_size`
   * - Vela model
     - Flash（不拷贝到 RAM）
     - **34,416 bytes**
     - `YOLOFACE_MODEL_DATA_SIZE` / `g_yoloface_int8_vela_tflite_size`

## 7. 注意事项

1. 当前工程仅做人脸检测，不做人脸库注册、比对或识别身份。
2. 检测框映射按模型输入与 1088×1088 画布处理，更换模型尺寸时需检查回调中的映射参数。
3. 更换摄像头或 LCD 时需同步检查引脚、分辨率和 panel 配置。
4. 模型资源由 `avdk_nn_module` 条件编译，需保持 `CONFIG_TFLM_YOLOFACE_V1=y`。
