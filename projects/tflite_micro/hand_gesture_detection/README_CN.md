# TensorFlow Lite Micro 手势检测工程

- [English](./README.md)

## 1. 项目概述

本项目在 Beken 平台上演示 **TensorFlow Lite Micro（TFLM）+ Ethos-U NPU** 的离线手势检测流程。工程使用 Vela 编译后的 YOLOv8 int8 模型，对 `ap/resource/hand_gesture_image_input_*.cc` 中固化的输入张量进行推理，并在 AP 日志中输出检测框、类别与耗时。

### 1.1 测试环境

- 硬件：BK7259 系列核心板，配置需满足模型、scratch 与 tensor arena 的内存需求。
- 软件：
  - `CONFIG_TFLITE_MICRO=y`
  - `CONFIG_TFLM_HAND_GESTURE_DETECTION_V1=y`
  - 输入张量形状由 `tflm_hand_gesture_detection_model.h` 定义，当前为 **320×320×3**

.. warning::

```
本工程不从摄像头取图，默认只跑内置测试张量。更换模型或测试图时，需要同步更新模型头文件、Vela 产物和 ap/resource 下的输入张量。
```

## 2. 目录结构

```
hand_gesture_detection/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.c                         # 创建 tflm_demo 任务
│   ├── app/
│   │   ├── tflm_hand_gesture_detection_demo.cpp
│   │   └── tflm_hand_gesture_detection_demo.h
│   ├── resource/
│   │   ├── hand_gesture_image_input.h
│   │   ├── hand_gesture_image_input_1.cc
│   │   └── hand_gesture_image_input_2.cc
│   └── config/bk7259_ap/defconfig
├── cp/
└── partitions/bk7259/
```

公共模型组件位于：

```
common_components/avdk_nn_module/
├── include/tflm_hand_gesture_detection_model.h
└── src/tflm_hand_gesture_detection/hand_gesture_detect_model_data.cc
```

## 3. 功能说明

### 3.1 主要功能

- 初始化 Ethos-U、TFLM `MicroInterpreter` 与 tensor arena
- 从 `avdk_nn_module` 加载 `hand_gesture_detection_vela_tflite`
- 对 `test_hands_1`、`test_hands_2` 两个内置输入依次推理
- 执行 YOLOv8 风格后处理：letterbox 逆映射、按类 NMS、输出原图坐标
- 使用 DWT 打印 `Invoke()` 耗时，并默认通过 **GPIO_55** 高低电平测量推理区间

### 3.2 处理流程

1. `bk_init()` → `media_service_init()` → `bk_frame_buffer_init()`
2. 创建 `tflm_demo` 任务
3. 分配 Ethos scratch、模型拷贝缓冲与 tensor arena
4. `AllocateTensors()` 后循环执行两张内置图的 `Invoke()`
5. 打印检测结果与单轮完成日志

## 4. 编译与运行

### 4.1 编译

在 SDK 根目录：

```
make bk7259 PROJECT=tflite_micro/hand_gesture_detection
```

### 4.2 运行与日志

烧录后无需输入 CLI，AP 侧会周期性运行 demo。典型日志关键字如下：

```
===========>start hand_gesture_detection demo
ethosu scratch -> HSRAM ...
model -> MEM_SLAB_UNCODED ...
arena -> MEM_SLAB_UNCODED ...
arena used ... / ... bytes
model load & init (scratch+ethosu+model+arena+AllocateTensors): ...
test_hands_1 Invoke time: ...
=== test_hands_1 (384x384) detections=... ===
hand_gesture_detection round done
```

## 5. 测试方案

- 功能：确认无 `bk_ethosu_init failed`、`AllocateTensors failed`、`Invoke failed`。
- 结果：检查 `detections`、`class_id`、`score` 与 `bbox_orig` 是否符合预期。
- 性能：查看 `{tag} Invoke time`，或用逻辑分析仪测量 GPIO_55 高电平宽度。

### 与 `ap/resource/hand_gesture_image_input.h` 的对应修改

该头文件把**后处理门限**、**每张嵌入样例的原图尺寸**、**类别名**与 `hand_gesture_image_input_*.cc` 中的 C 数组绑定；换图、增删样例或改类别时需同步修改。

| 内容 | 说明 |
|------|------|
| `k_conf_threshold` / `k_iou_threshold` | 与 PC 侧生成输入/参考输出时使用的置信度、NMS 门限保持一致。 |
| `k_test_hands_1_orig_w` / `k_test_hands_1_orig_h` 等 | 对应每张嵌入输入的原图宽高，用于把模型坐标逆映射回原图坐标。 |
| `k_class_names[]` | 类别显示名，数量与 `tflm_hand_gesture_detection_model.h` 中 `k_num_classes` 一致。 |
| `extern` 的 `*_model_input` / `*_model_input_len` | 必须与 `hand_gesture_image_input_*.cc` 中数组名、长度符号一致；新增样例时还需在 `tflm_hand_gesture_detection_run_demo()` 中调用。 |

### 更新模型与回归建议

1. 使用与板端 Ethos-U 配置匹配的 Vela 参数重新编译 int8 `.tflite`，并根据 Vela summary 核对 scratch、arena、模型大小。
2. 将新 Vela `.tflite` 转为 C 数组，替换 `common_components/avdk_nn_module/src/tflm_hand_gesture_detection/hand_gesture_detect_model_data.cc`，保持符号 `hand_gesture_detection_vela_tflite[]` 与 `hand_gesture_detection_vela_tflite_len` 不变。
3. 若输入尺寸、类别数、候选框数量或输出通道变化，同步更新 `tflm_hand_gesture_detection_model.h` 中的 `k_input_*`、`k_num_classes`、`k_num_candidates`、`k_out_channels`、`TFLM_ARENA_SIZE`、`ETHOSU_SCRATCH_SIZE`。
4. 重新生成 `hand_gesture_image_input_*.cc` 与 `hand_gesture_image_input.h` 元数据，编译后对比设备日志中的 `class_id`、bbox 与 PC 参考结果；同时观察 `Invoke time` / GPIO_55 高电平宽度。

## 6. 配置说明

- `TFLM_ARENA_SIZE`：TFLM tensor arena 大小，需大于运行日志中的 `arena used`。
- `ETHOSU_SCRATCH_SIZE`：Ethos-U scratch 大小，需与 Vela 报告和代码保持一致。
- `k_input_h` / `k_input_w` / `k_input_c`：模型输入尺寸。
- `k_num_classes` / `k_num_candidates` / `k_out_channels`：后处理输出布局。
- `hand_gesture_image_input.h`：绑定测试图原图尺寸、类别名和输入数组符号。

当前默认内存位置与大小：

| 类型 | 位置 | 大小 | 来源 |
|------|------|------|------|
| fast memory / Ethos-U scratch | HSRAM | **256 KB**（`ETHOSU_SCRATCH_SIZE`） | `tflm_hand_gesture_detection_model.h`、`g_ethosu0_scratch_src` |
| tensor arena | MEM_SLAB_UNCODED | **3 MB**（`TFLM_ARENA_SIZE`） | `tflm_hand_gesture_detection_model.h`、`g_tensor_arena_src` |
| Vela model copy | MEM_SLAB_UNCODED | **2,685,984 bytes**（`hand_gesture_detection_vela_tflite_len`） | `hand_gesture_detect_model_data.cc`、`g_model_data_src` |

## 7. 注意事项

1. `tflm_demo` 任务为循环运行，默认每轮间隔约 500 ms。
2. DWT 时间换算按 480 MHz CPU 频率处理，主频变化时需重新换算。
3. 更新模型时需同步检查 Vela 产物、模型头文件常量、输入张量与后处理参数。
4. GPIO_55 测时由 `TFLM_INVOKE_TIME_TEST` 控制，硬件复用冲突时需关闭或调整。
