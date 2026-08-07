# TensorFlow Lite Micro 人形检测工程

* [English](./README.md)

## 1. 项目概述

本工程在 Beken 平台上通过 **TensorFlow Lite Micro（TFLM）** 与 **Ethos-U NPU** 跑通端侧**人形检测**流程。应用侧打开 MIPI camera，使用 ISP MP 输出作为模型输入，公共组件 `avdk_nn_module` 负责模型加载、输入预处理、推理和检测后处理。

当前默认配置使用 **Gray 模型 + NV12 MP 输入**；也可显式切换为 **RGB 模型 + BGRA8888 MP 输入**。

主要功能：

- 初始化 camera、frame buffer、media service 与设备管理
- 从 `avdk_nn_module` 加载 Vela 生成的 `person_detection_vela_tflite`
- 使用 Ethos-U NPU 运行 320x180 人形检测模型
- Gray 模式下只取 NV12 前面的有效 Y 数据作为模型输入，减少搬运
- RGB 模式下接收 BGRA8888 MP 帧，并在模型内部转换为 RGB 输入
- 输出检测框数量、最高分检测框坐标和分数
- 模型加载成功后打印 tensor arena 总大小、实际使用量和剩余量

## 2. 目录结构

```
person_detection/
├── CMakeLists.txt
├── Makefile
├── README.md
├── README_CN.md
├── app.rst
├── ap/
│   ├── CMakeLists.txt
│   ├── ap_main.cc                       # camera 配置、模型创建、推理启动
│   └── config/bk7259_ap/
│       ├── defconfig                    # 默认开启 Gray person detection
│       └── usr_gpio_cfg.h
├── cp/
└── partitions/bk7259/
    ├── ram_regions.csv
    └── auto_partitions.csv
```

公共模型组件位于：

```
common_components/avdk_nn_module/
├── Kconfig                              # TFLM_PERSON_DETECTION_GRAY/RGB
├── CMakeLists.txt                       # 按宏选择 gray 或 rgb 模型数据
├── include/tflm_person_detect.h         # PersonDetectModel 声明
└── src/tflm_person_detection/
    ├── tflm_person_detection.cc         # 预处理、推理、后处理
    ├── person_detect_gray_model_data.cc # Gray Vela 模型
    └── person_detect_rgb_model_data.cc  # RGB Vela 模型
```

## 3. 配置说明

### 3.1 Kconfig

默认配置：

```
CONFIG_TFLITE_MICRO=y
CONFIG_AVDK_VIDEO_REATOR=y
CONFIG_TFLM_PERSON_DETECTION_GRAY=y
# CONFIG_TFLM_PERSON_DETECTION_RGB is not set
CONFIG_NPU=y
```

输入源宏为显式配置：

- `CONFIG_TFLM_PERSON_DETECTION_GRAY=y`：编译 Gray 模型，camera MP 配置为 `BK_PIXEL_FORMAT_NV12`
- `CONFIG_TFLM_PERSON_DETECTION_RGB=y`：编译 RGB 模型，camera MP 配置为 `BK_PIXEL_FORMAT_BGRA8888`

两者不要同时开启。`avdk_nn_module` 的 CMake 会按宏只编译一个模型数据文件，避免 `person_detection_vela_tflite` 符号重复。

### 3.2 Camera 输入

`ap/ap_main.cc` 中配置 camera：

- `mp_enable = true`
- `mp_width = 320`
- `mp_height = 180`
- Gray：`mp_format = BK_PIXEL_FORMAT_NV12`
- RGB：`mp_format = BK_PIXEL_FORMAT_BGRA8888`

应用只调用 `OpenCameraWithoutDisplay()` 与 `start_infer()`，当前不主动开启 LCD/display 路径；LCD、DPU、GPU 配置保留，后续需要显示时可直接接入。

## 4. 编译与运行

在 SDK 根目录：

```bash
make bk7259 PROJECT=tflite_micro/person_detection
```

烧录后无需 CLI 输入，AP 侧会打开 camera 并启动推理线程。典型日志关键字：

```text
M55 main running...
PersonDetection resourceLoad model=... arena=2097152 scratch=131072 format=...
PersonDetectionModel arena total=2097152 used=... free=...
PersonDetection input prepare: ... us
PersonDetection Invoke: ... us
PersonDetection decode + NMS: ... us
PersonDetection count=... top score=... xywh=(...)
```

## 5. 测试方案

- 功能：camera 能正常打开，无 `PersonDetection camera open failed`。
- 初始化：无 `bk_ethosu_init failed`、`AllocateTensors failed`、`PersonDetection input shape mismatch`。
- 输入格式：Gray 模式日志中模型 `format` 对应 NV12；RGB 模式由 `AvdkVideoReator` 以 BGRA8888 送入 `run()`。
- 结果：观察 `PersonDetection count`、`score` 和 `xywh` 是否随画面变化。
- 内存：查看 `arena total/used/free`，确认 `used` 小于 `total` 并留有余量。

## 6. 更新模型注意事项

1. 新模型需经 Vela 编译，并替换对应的 `person_detect_gray_model_data.cc` 或 `person_detect_rgb_model_data.cc`。
2. 保持模型数据符号名为 `person_detection_vela_tflite[]` 与 `person_detection_vela_tflite_len`。
3. 若输入尺寸、通道数或输出 shape 变化，需要同步修改 `tflm_person_detection.cc` 中的输入尺寸、anchor 配置、输出解析与后处理逻辑。
4. 根据运行日志中的 arena 使用量调整 `kPersonArenaSize`，根据 Vela 报告调整 `kPersonScratchSize`。
5. 分区文件位于 `partitions/bk7259/`，需保证 AP app、模型数据、arena 和 PSRAM 区域满足实际模型大小。

## 7. 注意事项

1. 当前工程是实时 camera 推理，不使用内置测试图片。
2. Gray 模式只使用 NV12 Y plane，UV 数据不会搬入模型输入。
3. RGB 模式输入由 MP 输出 BGRA8888，再在模型内部转换为 RGB。
4. `AvdkVideoReator` 中 RGB 模型会以 BGRA8888 作为实际推理输入格式传给模型。
