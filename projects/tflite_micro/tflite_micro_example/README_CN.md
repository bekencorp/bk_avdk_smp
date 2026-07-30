# TensorFlow Lite Micro 示例工程

* [English](./README.md)

## 1. 项目概述
本工程是一个 TensorFlow Lite Micro（TFLM）与 Ethos-U NPU 的组合示例，用于在 Beken 平台上验证端侧推理链路是否可用。

当前示例主要完成了 `person_detection` 模型的初始化与推理流程：
- 初始化 TFLM 运行时（`tflite::MicroInterpreter`）
- 加载模型数据（Vela 生成的模型输出）
- 为 Ethos-U 配置并传入 scratch buffer
- 为 TFLM 配置 tensor arena
- 获取输入图像并执行推理，然后输出 person/no person 的分数

同时此示例支持性能测试和推理耗时统计。

## 2. 目录结构
项目采用 AP/CP 双核架构，其中主要逻辑在 AP 端，目录结构如下：
```
tflite_micro_example/
├── .ci                             # CI配置目录
├── CMakeLists.txt                  # 项目级CMake构建文件
├── Makefile                        # Make构建文件
├── ap/
|   ├── config/
|   |   └── defconfig               # AP默认配置
│   ├── ap_main.c                   # AP主入口：初始化媒体服务并创建推理任务
│   ├── CMakeLists.txt              # AP CMake构建文件
│   └── person_detection/           # person_detection 模型示例
│       ├── tflm_person_detection_demo.cpp           # 主要推理逻辑（TFLM + Ethos-U + 计时/内存策略）
│       ├── tflm_person_detection_image_provider.cc  # 输入图像提供器
│       └── tflm_person_detection_model_vela_data.cc # 模型数据（Vela输出）
├── cp/
|   ├── config/
|   |   └── defconfig              # CP默认配置
│   ├── CMakeLists.txt             # CP CMake构建文件
│   └── cp_main.c                  # CP端入口
└── partitions/                    # 分区/ram 配置目录
    └── bk7259/
        ├── ram_regions.csv        # RAM区域配置
        └── auto_partitions.csv    # 自动分区配置
```

## 3. 功能说明
### 3.1 主要功能
1. 端侧推理链路验证：使用 TFLM 构建并运行 `person_detection` 模型，并通过 Ethos-U driver 完成 NPU 相关初始化与中断处理。
2. 输入输出处理：从图像提供器获取输入张量数据，读取输出张量并计算 `person_score/no_person_score` 的概率百分比。
3. 输出信息：初始化阶段会打印内存使用信息（scratch/tensor arena/model data），每次推理完成后会打印 person/no person 的分数。

### 3.2 推理耗时自动计算（DWT）
在 `tflm_person_detection_demo.cpp` 中通过 `CONFIG_PERSON_DETECTION_INFER_TIME_AUTO` 控制是否打开推理耗时统计：
- 打开后，使用 DWT cycles 记录 `Invoke()` 前后的 cycle 值
- 通过固定 CPU 主频 `480MHz` 换算耗时
- 打印格式：推理耗时小于 `1ms` 时使用 `us` 单位打印；推理耗时大于等于 `1ms` 时使用 `ms` 单位打印

提示：由于该功能是通过 `#ifdef CONFIG_PERSON_DETECTION_INFER_TIME_AUTO` 编译期开关实现，需在工程编译时让该宏处于已定义状态。

### 3.3 内存/性能策略（HSRAM/PSRAM/SRAM）
在 `tflm_person_detection_demo.cpp` 中通过 `PERSON_DETECTION_PERF_TEST` 控制内存策略：
- 当 `PERSON_DETECTION_PERF_TEST=1`（性能模式）时，`EthosU0 Scratch` / `Tensor Arena` / `Model Data` 优先申请 HSRAM；HSRAM 不足时自动回退到（低速）SRAM 分配。
- 当 `PERSON_DETECTION_PERF_TEST=0` 时，`EthosU0 Scratch` 仍使用 HSRAM，`Tensor Arena` / `Model Data` 使用 PSRAM。

此外，该示例对模型数据做了 16 字节对齐要求，以满足 Ethos-U 命令流相关约束。性能模式用于测试NPU推理的极限性能。

### 3.4 GPIO 推理耗时观测（逻辑分析仪）
在 `tflm_person_detection_demo.cpp` 中如果定义了 `PERSON_DETECTION_DEBUG`，推理过程中会按阶段拉取指定的 GPIO：
- `GPIO_34`：推理开始/结束阶段切换（便于用逻辑分析仪测量 `Invoke()` 的耗时区间）
- 其他阶段：`GPIO_32/GPIO_33/ GPIO_35` 用于区分取图、推理、结果处理等不同阶段

建议接入逻辑分析仪后，通过观察 `GPIO_34` 的高电平持续时间即可获得一次推理的耗时。

## 4. 编译与运行
### 4.1 编译方法
使用以下命令编译项目：
```
make bk7259 PROJECT=tflite_micro/tflite_micro_example
```

### 4.2 运行方法
编译完成并烧录固件到开发板后，串口日志中可以看到推理任务输出：
1. AP 上创建推理线程
2. 推理任务会进入循环，依次对两个输入样本执行推理（paper/no person 与 rock/person）
3. 每次推理完成后会打印输出分数

示例（日志关键字可能因版本略有差异）：
```
===========>start person_detection demo
Start detecting person
person score:xx%, no person score yy%
Start detecting no person
person score:xx%, no person score yy%
```

若开启了推理耗时统计（`CONFIG_PERSON_DETECTION_INFER_TIME_AUTO`），则在 `Invoke()` 完成后还会额外打印形如：
```
Invoke time: xxx us
```
或
```
Invoke time: xxx ms
```

## 5. 测试方案
1. 功能验证：直接运行示例，观察是否持续输出 person/no person 的分数，并确认没有出现 `Invoke failed` / `AllocateTensors failed` / 相关初始化失败日志。
2. 性能验证（可选）：打开 `CONFIG_PERSON_DETECTION_INFER_TIME_AUTO`，观察 `Invoke time` 日志，统计典型推理耗时范围（us 或 ms）。
3. 内存策略验证（可选）：修改 `PERSON_DETECTION_PERF_TEST`，观察初始化阶段打印的 scratch/tensor arena/model data 内存来源与是否触发回退。

## 6. 注意事项
1. 本示例推理任务为持续运行，进入循环后不会自动退出
2. Ethos-U scratch 与 tensor arena 的内存占用较大，请确保对应内存区域可用
3. 模型数据对齐要求（16 字节）不能破坏，示例中已对齐处理
4. DWT cycles 换算使用固定 `480MHz`，若平台实际主频不同，耗时换算可能会偏差

