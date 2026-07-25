# TensorFlow Lite Micro 宠物检测（猫/狗 YOLOv8）工程

## 项目概述

本工程在 Beken 平台上通过 **TensorFlow Lite Micro（TFLM）** 与 **Ethos-U NPU** 跑通端侧**猫/狗检测**（YOLOv8 int8 量化 + 后处理）全链路。模型权重经 **Vela** 编译后放入公共组件 `avdk_nn_module`；应用侧以嵌入的预处理输入张量做离线样例推理。

主要完成内容：

- 初始化 TFLM 运行时（`tflite::MicroInterpreter`）
- 从 `avdk_nn_module` 加载 Vela 生成的 `pet_detection_vela_tflite` 数据
- 为 Ethos-U 配置并传入 **scratch** buffer
- 为 TFLM 配置 **tensor arena**（大小见 `tflm_pet_detection_model.h`）
- 对两张嵌入的输入（与 PC 上 `tflite_int8_inference.py` 同源的 letterbox+int8 张量）依次推理，输出检测框、类别、得分，并支持 **DWT 计时** 与 **GPIO_55 脉宽** 测量 `Invoke()` 区间

> 需开启 Kconfig：`TFLITE_MICRO` 与 `CONFIG_TFLM_PET_DETECTION_V1`，否则未编译 `pet_detect_model_data.cc`，链接会缺 `pet_detection_vela_tflite` 符号。

## 目录结构

项目为 AP/CP 双核布局，**推理与日志逻辑在 AP**。目录与 `tflite_micro_example` 风格一致，示例如下：

```
pet_detection/
├── .ci                             # CI 配置
├── CMakeLists.txt                  # 项目级 CMake（含 EXTRA_COMPONENTS_DIRS：avdk_nn_module 等）
├── Makefile                        # Make 包装
├── ap/
|   ├── config/bk7259_ap/           # AP 板级/默认 Kconfig
|   |   └── defconfig
|   ├── ap_main.c                   # AP 主入口：媒体/缓存初始化，创建 tflm_demo 任务
|   ├── CMakeLists.txt              # AP 组件：ap_main、demo、resource 下 *.cc
|   ├── app/                        # 宠物检测 demo
|   |   ├── tflm_pet_detection_demo.h
|   |   └── tflm_pet_detection_demo.cpp   # TFLM + YOLOv8 后处理、DWT、GPIO55
|   └── resource/
|       ├── pet_image_input.h              # 阈值、原图尺寸、extern 张量名
|       ├── pet_image_input_1.cc           # 嵌入张量 1（如 cat_200 对应输入）
|       ├── pet_image_input_2.cc           # 嵌入张量 2（如 dog_204 对应输入）
|       └── tflite_int8_inference.py        # PC：生成 *_model_input.cc 与 *_output.txt
├── cp/
|   ├── config/bk7259/defconfig    # CP 默认配置
|   ├── CMakeLists.txt
|   └── cp_main.c                    # CP 侧入口（与 AP 推理无直接绑定）
└── partitions/bk7259/
    ├── ram_regions.csv              # RAM 区域
    └── auto_partitions.csv          # 自动分区
```

**公共组件中的 Vela 模型与头文件**（本工程通过 `avdk_nn_module` 拉取）：

```
common_components/avdk_nn_module/
├── Kconfig                                 # CONFIG_TFLM_PET_DETECTION_V1
├── CMakeLists.txt                          # 条件编译 pet_detect_model_data.cc
├── include/tflm_pet_detection_model.h     # 张量形状、TFLM_ARENA_SIZE、vela 符号声明等
└── src/tflm_pet_detection/
    └── pet_detect_model_data.cc            # pet_detection_vela_tflite[] + _len（体积大，工具生成）
```

## 功能说明

### 主要功能

1. **端侧推理与后处理**

    - 使用 TFLM + Ethos-U 运行 int8 检测模型。
    - 后处理为 YOLOv8 风格：letterbox 逆映射、按类 NMS、输出原图坐标的 `x1,y1,x2,y2` 与 `class_id`（`k_class_names`：cat / dog）。

2. **输入**

    - 当前 demo 不读摄像头，仅使用 `pet_image_input_*.cc` 中**固化的模型输入张量**（与 Python 生成脚本中传入 Interpreter 的 buffer 一致）。

3. **输出信息**

    - 首次初始化打印 scratch / model / arena 来源、指针与大小。
    - 每张样例图打印 `detections` 条数及每条 bbox、score、class。

### 推理耗时与 GPIO（tflm_pet_detection_demo.cpp）

- **DWT**：`pet_log_dwt_interval` 在「模型 load+init+AllocateTensors」与每次 Invoke 后打印耗时；**按 480 MHz** 将 cycle 差换算为 `us`（<1 ms）或 `ms`（≥1 ms），与 `tflite_micro_example` 中 person 示例习惯一致。若芯片主频非 480 MHz，需自行修正换算。
- **GPIO_55**：每次 `Invoke()` 前拉高、结束后拉低，可用逻辑分析仪测量高电平宽度作为单次推理时间。

### 内存来源（与 tflite 示例同思路）

- 在 `tflm_pet_detection_demo.cpp` 中通过 `tflm_mem_src_t` 选择：`HSRAM`、`PSRAM`、`MEM_SLAB_UNCODED` 等，分别用于 **Ethos scratch**、**模型拷贝缓冲**、**tensor arena**（以源码当前枚举与分配为准）。
- 模型与 arena 在代码路径上要求 **16 字节对齐**（与 Ethos 命令流等约束一致），示例中已对分配地址做对齐。

## 编译与运行

### 编译方法

在 SDK 根目录使用（与 `.ci` 一致）：

```
make bk7259 PROJECT=tflite_micro/pet_detection
```

### 运行与串口日志

烧录后，AP 侧任务 TAG `tflite_demo` 会周期性调用 `tflm_pet_detection_run_demo()`。典型成功日志（关键字可能因版本略异）：

```
ap1:tflite_d:I(1):===========>start pet_detection demo
ap1:pet_tflm:I(2):ethosu scratch -> HSRAM 0x28100020 size=256000
ap1:ETHOSU:W(3):W: Initializing NPU: base_address=0x48200000, fast_memory=0x28100020, fast_memory_size=256000, secure=1, privileged=1

ap1:ethosu:I(3):Ethos-U driver initialized successfully, ret=0
ap1:pet_tflm:I(74):model -> MEM_SLAB_UNCODED 0x60d53e40 size=2802048
ap1:pet_tflm:I(74):arena -> MEM_SLAB_UNCODED 0x60a53dc0 size=3145728
ap1:pet_tflm:I(74):arena used 2734676 / 3145728 bytes
ap1:pet_tflm:I(74):model load & init (scratch+ethosu+model+arena+AllocateTensors): 72 ms
ap1:pet_tflm:I(178):cat_200 Invoke time: 95 ms
ap1:pet_tflm:I(181):=== cat_200 (500x374) detections=1 ===
ap1:pet_tflm:I(181):  [0] class_id=0 (class0) score=0.8338 bbox_orig: x1=138.55 y1=9.84 x2=347.00 y2=272.24
ap1:pet_tflm:I(284):dog_204 Invoke time: 95 ms
ap1:pet_tflm:I(286):=== dog_204 (499x375) detections=1 ===
ap1:pet_tflm:I(286):  [0] class_id=1 (class1) score=0.9515 bbox_orig: x1=133.38 y1=46.53 x2=419.72 y2=342.67
```

- **DWT/Invoke 行**：`{标签} Invoke time: ...` 对应单次推理。
- **检测行**：`=== <tag> (WxH) detections=n ===` 后若干条 `[i] class_id=...`；`n==0` 时可能打印 `(none above conf=...)`。
- **错误**：如 `alloc ethosu scratch failed`、`bk_ethosu_init failed`、`AllocateTensors failed`、`Invoke failed`、shape 不匹配等，需对照 Kconfig、模型版本与 arena/scratch 大小。

## 测试方案

### 与 ap/resource/pet_image_input.h 的对应修改

该头文件把**后处理参数**、**每张嵌入样例的原图尺寸**与 **C 符号名** 和 `pet_image_input_*.cc` 绑定；换图、换脚本参数或增删样例时都需同步改这里及 `tflm_pet_detection_demo.cpp` 中的调用。

.. list-table::
   :header-rows: 1

   * - 内容
     - 说明
   * - `k_conf_threshold` / `k_iou_threshold`
     - `--conf`、`--iou` 及 `*_output.txt` 里 `# conf_threshold` / `# iou_threshold` **一致**，否则 PC 参考与板端后处理门限不同。
   * - `k_cat_orig_w` / `k_cat_orig_h`、`k_dog_orig_w` / `k_dog_orig_h`
     - 对应各嵌入输入在**原图**上的宽高（像素），需与 `*_output.txt` 中 `# orig_image_size` 及 `letterbox_params` 使用一致；换用其它图片样例时改为新原图尺寸。
   * - `k_class_names[]`
     - 检测类别显示名；依赖 `tflm_pet_detection_model.h` 中的 `k_num_classes`，顺序与模型 `class_id` 一致（当前为 cat / dog）。类别数变化时需与模型头一致。
   * - `extern` 的 `*_model_input` / `*_model_input_len`
     - 须与 `pet_image_input_*.cc` 里数组名、长度符号**完全一致**；增删 `.cc` 文件或改名时，在此处增删声明，并在 `tflm_pet_detection_demo.cpp` 的 `tflm_pet_detection_run_demo()` / `run_one_embedded_image()` 中传入匹配的标签与指针。

该文件已 `#include "tflm_pet_detection_model.h"`，因此**输入分辨率、类别数**以模型头文件为准；若仅换测试图而不改模型，一般只调阈值、原图尺寸与 `extern` 符号。

### 用 Vela 更新模型并合入固件

**前提**：已安装与 Ethos-U 工具链配套的 `vela` 命令行；`vela.ini` 与 int8 模型放在同一工作目录（或 `--config` 指到实际路径）。输入模型需与训练/量化产物一致，例如 `yolov8n_full_integer_quant.tflite`。

#### 步骤 1：用 Vela 编译模型

在含 `vela.ini` 的目录下执行（参数需与**片上** Ethos 加速器、**PSRAM/ SRAM** 与优化目标一致，可按实车环境微调）：

```bash
vela yolov8n_full_integer_quant.tflite \
  --output-dir ./vela_out \
  --accelerator-config ethos-u65-256 \
  --config vela.ini \
  --system-config Ethos_U65_PSRAM \
  --memory-mode Dedicated_Sram_256KB \
  --optimise Performance
```

- 查看 `--output-dir` 下 Vela 生成的 `.tflite` **/ 报告 / 可能的** `.cc`（依工具版本而定），并阅读同目录下 **summary CSV** 中建议的 **arena、scratch、权重大小**，用于核对你工程里的 `TFLM_ARENA_SIZE`、`ETHOSU_SCRATCH_SIZE` 与分区是否仍够用。

#### 与 common_components/avdk_nn_module/include/tflm_pet_detection_model.h 的对应修改

更新 Vela 模型或更换网络结构后，除替换 `pet_detect_model_data.cc` 外，需按**新 Flatbuffer 与 Vela 报告**检查本头文件，避免 `AllocateTensors` 失败或 shape 校验报错（`tflm_pet_detection_demo.cpp` 会对输入/输出维做断言）。

.. list-table::
   :header-rows: 1

   * - 内容
     - 说明
   * - `pet_detection_vela_tflite` / `pet_detection_vela_tflite_len`
     - 与 `pet_detect_model_data.cc` 中数组名、长度常量**声明一致**；一般只改 `.cc` 实现，头文件保持 `extern`。
   * - `TFLM_ARENA_SIZE`
     - TFLM **tensor arena** 字节数，应 **≥** Vela summary / 运行日志里 `arena_used`，并留余量；过小会 `AllocateTensors failed`。
   * - `ETHOSU_SCRATCH_SIZE`
     - Ethos-U **scratch** 大小，须与 Vela 对该网络的要求及 `bk_ethosu_init` 传入长度一致。
   * - `k_input_h` / `k_input_w` / `k_input_c`
     - 模型**输入张量**高、宽、通道；与 `pet_image_input` 中 letterbox 后的张量 shape 一致（当前为 320×320×3）。
   * - `k_num_classes`
     - 检测类别数；与模型输出通道中分类部分一致（当前 2：猫/狗）。
   * - `k_num_candidates`
     - 特征图展平后的候选框数量（如 2100）；与 TFLite 输出 `out->dims` 中最后一维一致。
   * - `k_out_channels`
     - 定义为 `4 + k_num_classes`（cx,cy,w,h + 各类 score），与 YOLOv8 头输出布局一致；改类别数时会随之变化。

当前默认内存位置与大小：

.. list-table::
   :header-rows: 1

   * - 类型
     - 位置
     - 大小
     - 来源
   * - fast memory / Ethos-U scratch
     - HSRAM
     - **250 KB**（`ETHOSU_SCRATCH_SIZE`）
     - `tflm_pet_detection_model.h`、`g_ethosu0_scratch_src`
   * - tensor arena
     - MEM_SLAB_UNCODED
     - **3 MB**（`TFLM_ARENA_SIZE`）
     - `tflm_pet_detection_model.h`、`g_tensor_arena_src`
   * - Vela model copy
     - MEM_SLAB_UNCODED
     - **2,802,048 bytes**（`pet_detection_vela_tflite_len`）
     - `pet_detect_model_data.cc`、`g_model_data_src`

若仅重跑 Vela、网络结构未变，通常只需核对 **arena/scratch** 与 `pet_detection_vela_tflite_len`；若输入/输出形状或类别数变化，必须同步改本头文件，并重新用 `tflite_int8_inference.py` 生成嵌入输入、检查 `pet_image_input.h` 中 `k_class_names` 与 `extern` 样例。

#### 步骤 2：将二进制固化为 C 源文件

若 Vela 只产出扁平二进制、或你需把某 `*.tflite` 转成 C 数组再与现有 `pet_detect_model_data.cc` 对齐，可用 `xxd` 生成无符号名的原始数组，再 **手工** 改成工程约定符号：

```bash
xxd -i -c 12 xxxx.tflite > xxxx.cc
```

- `-c 12`：每行 12 个字节
- `xxd -i` 会按 **文件名** 生成 `unsigned char 文件名各节[]` 与长度宏/常量；**必须** 改为与头文件一致的名字，并加上本工程中的对齐与链接属性，例如：
  - 符号：`pet_detection_vela_tflite[]`、`pet_detection_vela_tflite_len`（与 `tflm_pet_detection_model.h` 中 `extern` 一致）；
  - 建议：`#include "avdk_nn_module.h"`，数组使用 `extern "C"` 与 `DATA_ALIGN_ATTRIBUTE`（见现有 `pet_detect_model_data.cc` 头几行）；
  - 正常 Ethos 路径应 **先 Vela，再**对 Vela 输出做 `xxd` 或直接使用 Vela 导出的 C。

#### 步骤 3：替换仓库中的模型源文件

1. 步骤 2 整理好的数组，**整体替换**
   `common_components/avdk_nn_module/src/tflm_pet_detection/pet_detect_model_data.cc`。
2. 确认 `pet_detection_vela_tflite_len` 与数组字节数一致。
3. 按上一节「与 tflm_pet_detection_model.h 的对应修改」更新该头文件中的宏与 `constexpr`，使之与**新** Vela 产物及 TFLite 张量形状一致。
4. Kconfig 中保持 `CONFIG_TFLM_PET_DETECTION_V1=y`。
5. 全量重编并跑 `pet_detection` 工程，用串口日志与 PC 侧 `tflite_int8_inference.py` 的 `*_output.txt` 做对比回归；若输入/类别有变，同步「与 pet_image_input.h 的对应修改」。

### 功能与回归建议

- 功能：串口能稳定打印两次样例的 `detections` 与 bbox，无 `Invoke failed` / `AllocateTensors failed`。
- 与 PC 对比：同一张图，`*_output.txt` 的 `class_id` 与 bbox 应与设备在数值上接近（量化与实现差异可有小偏差）。
- 性能：观察 `Invoke time` 或 `GPIO_55` 高电平宽度。

## 注意事项

1. 推理任务在 `ap_main` 中为 **死循环** 调用，不会自行退出。
2. Vela 模型与 tensor arena 占用大，**分区与 RAM 配置**（`partitions/bk7259`）需满足跑通要求。
3. 更新模型时务必同时更新：Vela 产物、头文件常量、嵌入的 `model_input` 与 Python 参考 `*_output.txt` 元数据。
4. 类别显示名为固件中 `k_class_names`；PC 端 `output.txt` 可能为 `class0` 等字符串，对照时请 **以 class_id 为准**。
5. 更通用的 TFLM 使用说明可对照：`tflite_micro/tflite_micro_example/README_CN_.md`。

## 参考工程

.. list-table::
   :header-rows: 1

   * - 说明
     - 路径
   * - 通用 TFLM + Vela 例程（人形检测）
     - `tflite_micro/tflite_micro_example/`
