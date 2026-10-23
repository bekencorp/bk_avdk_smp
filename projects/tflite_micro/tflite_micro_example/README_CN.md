# TensorFlow Lite Micro 示例工程

* [English](./README.md)

## 1. 项目概述
本工程是一个 TensorFlow Lite Micro（TFLM）与 Ethos-U NPU 的组合示例，用于在 Beken 平台上验证端侧推理链路是否可用。

工程提供两种编译期可选模式（由 `ap/ap_main.c` 中的 `TFLM_MULTI_INTERPRETER_DEMO` 宏切换）：
- **双解释器并行模式（默认，`=1`）**：两个不同模型在两个绑核任务中并发推理，模拟真实"单 NPU 承载多路 AI 业务"的产品形态：
  - `tflm_person` 任务运行在 AP core0（CPU2）：person_detection 模型（96x96x1 灰度，轻量分类）
  - `tflm_pet` 任务运行在 AP core1（CPU3）：pet_detection 模型（YOLOv8n，320x320x3，检测 + NMS 后处理）
- **单解释器模式（`=0`）**：原始 person_detection 示例，保留作为性能基线与回退路径（文件零改动）。

两种模式共享同一套 NPU 运行时：Ethos-U65 driver 的 reserve/release 机制在两个任务间串行化 NPU 访问，CPU 侧工作（输入准备、后处理）则在两颗 AP 核上真正并行。

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
│   ├── ap_main.c                   # AP主入口：TFLM_MULTI_INTERPRETER_DEMO 切换双/单解释器模式
│   ├── CMakeLists.txt              # AP CMake构建文件
│   ├── person_detection/           # person_detection 模型示例（单解释器基线，未改动）
│   │   ├── tflm_person_detection_demo.cpp           # 主要推理逻辑（TFLM + Ethos-U + 计时/内存策略）
│   │   ├── tflm_person_detection_image_provider.cc  # 输入图像提供器
│   │   └── tflm_person_detection_model_vela_data.cc # 模型数据（Vela输出）
│   └── multi_model/                # 双解释器并行示例（person + pet）
│       ├── tflm_multi_model_demo.h/.cpp              # 核心：模型描述表、每实例上下文、
│       │                                              # 单线程初始化阶段、绑核推理任务
│       ├── tflm_pet_postprocess.h/.cpp               # Pet YOLOv8 后处理（letterbox + 反量化 + 按类 NMS）
│       ├── pet_detect_model_data.cc                  # Pet 模型数据（自 avdk_nn_module 自足拷贝）
│       └── pet_image_input_1.cc / pet_image_input_2.cc # 内置 cat/dog int8 输入张量
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

### 3.1 双解释器并行示例（默认模式）
`ap/multi_model/tflm_multi_model_demo.cpp` 实现双解释器示例：

**初始化（单线程阶段，任务创建前在 `main` 中完成）**
1. 从 HSRAM 分配共享的 Ethos-U fast scratch（250KB，两模型共用一份，使用期由 driver 互斥保护）
2. 全局仅调用一次 `bk_ethosu_init()`（组件内已加幂等保护）
3. 对两个模型逐一执行：拷贝模型数据（16 字节对齐）、分配 tensor arena、构造 op resolver 与解释器、执行 `AllocateTensors()`

**运行期**
- 每个解释器的资源完全独立（模型副本 / arena / resolver / 解释器 / 后处理缓冲），两实例之间无任何可写共享
- 两个任务绑核运行（`rtos_core0/1_create_psram_thread`）：`tflm_person` 在 core0、周期 100ms；`tflm_pet` 在 core1、周期 500ms
- NPU 访问由 Ethos-U driver 的 reserve/release 机制串行化；完成中断固定路由到 CPU2，跨核唤醒等待任务
- DWT 周期计数器在各自核上独立测量 `Invoke()` 耗时

**内存布局**

| 资源 | 内存区域 | 大小 | 归属 |
|---|---|---|---|
| person 模型 + arena | PSRAM heap | 262KB + 100KB | 仅 T0 |
| pet 模型 + arena | MEM_SLAB_UNCODED | 2.68MB + 3MB | 仅 T1 |
| pet 后处理缓冲 | PSRAM heap | ~145KB | 仅 T1 |
| NPU fast scratch | HSRAM | 250KB（两模型所需最大值） | 共享，driver 互斥 |
| 任务栈 | PSRAM | 8KB x 2 | 每任务 |

**预期串口日志**
```
===========>start multi-interpreter (person + pet) demo init
ethosu scratch -> HSRAM 0x2c100020 size=256000
[T0:person] ready: model 0x64e60050 (267104 B, PSRAM) arena 0x64ea13e8 (102400 B, used 74924, PSRAM)
[T1:pet] ready: model 0x60d52d80 (2806272 B, MEM_SLAB_UNCODED) arena 0x60a52d00 (3145728 B, used 2734676, MEM_SLAB_UNCODED)
multi-interpreter init OK (shared NPU, 2 interpreters)
dual-interpreter tasks started (core0: person, core1: pet)
[T1:pet] started on core 1 (period 500 ms)
[T0:person] started on core 0 (period 100 ms)
[T0:person] Invoke time: 6 ms
[T0:person] input=person person score:76%
[T1:pet] Invoke time: 170 ms
[T1:pet] input=cat_200 (500x374) detections=1
  [0] class_id=0 (cat) score=0.8338 bbox_orig: x1=138.55 y1=9.84 x2=347.00 y2=272.24
```
person 任务在 person 图（分数约 76%）与 no-person 图（分数约 28%）间交替；pet 任务在内置 cat / dog 两张输入间交替。

**预期并行行为**
- pet 推理占用 NPU 期间（每次约 170ms），person 任务的 `Invoke()` 由约 6ms 升至约 157ms（NPU 排队）
- pet 的 NMS 后处理（CPU，core1）与 person 推理（core0）在时间上重叠
- NPU 总吞吐维持在单颗 NPU 上限之内：两路业务是"共享"而非"倍增"NPU 算力
- 推理结果稳定且与各自单独运行基线一致（无相互干扰）

### 3.2 单解释器 person_detection 示例（回退模式）
将 `ap/ap_main.c` 中 `TFLM_MULTI_INTERPRETER_DEMO` 置 `0` 即恢复原示例：
- 初始化 TFLM 运行时（`tflite::MicroInterpreter`）并加载 person_detection 模型（Vela 输出）
- 为 Ethos-U 配置 scratch buffer，为 TFLM 配置 tensor arena
- 获取输入图像并执行推理，输出 person / no person 分数

### 3.3 推理耗时自动计算（DWT）
两种示例均使用 DWT 周期计数器测量 `Invoke()` 耗时，并按固定 CPU 主频 `480MHz` 换算；小于 1ms 以 `us` 打印，大于等于 1ms 以 `ms` 打印。
- 单解释器示例通过 `CONFIG_PERSON_DETECTION_INFER_TIME_AUTO` 控制（编译期需定义该宏）
- 双解释器示例每轮固定打印，带 `[T0:person]` / `[T1:pet]` 前缀；任务绑核后各自使用本核 DWT，互不干扰

### 3.4 内存/性能策略（HSRAM/PSRAM/SRAM）
单解释器示例中，`tflm_person_detection_demo.cpp` 的 `PERSON_DETECTION_PERF_TEST` 控制内存策略：
- `PERSON_DETECTION_PERF_TEST=1`（性能模式）：scratch / arena / 模型数据优先申请 HSRAM，不足时自动回退（低速）SRAM
- `PERSON_DETECTION_PERF_TEST=0`：scratch 仍用 HSRAM，arena / 模型数据用 PSRAM

双解释器示例采用固定策略（见 3.1 表格）：person → PSRAM heap，pet → MEM_SLAB_UNCODED，共享 scratch → HSRAM。

两种示例均对模型数据做 16 字节对齐（Ethos-U 命令流约束）。

### 3.5 GPIO 推理耗时观测（逻辑分析仪）
在单解释器示例 `tflm_person_detection_demo.cpp` 中定义 `PERSON_DETECTION_DEBUG` 后，推理各阶段会拉取指定 GPIO：
- `GPIO_34`：推理开始/结束切换（测量 `Invoke()` 耗时区间）
- `GPIO_32/GPIO_33/GPIO_35`：区分取图、推理、结果处理等阶段

## 4. 编译与运行
### 4.1 编译方法
使用以下命令编译项目：
```
make bk7259 PROJECT=tflite_micro/tflite_micro_example
```

### 4.2 运行方法
编译完成并烧录固件到开发板后：
1. （默认）双解释器示例在单线程阶段完成两模型初始化，随后启动 core0 的 `tflm_person` 与 core1 的 `tflm_pet`
2. person 任务每 100ms 一轮，对 person / no-person 图交替打印分数；pet 任务每 500ms 一轮，对 cat / dog 图打印检测框
3. 观察两任务的 `Invoke time` 日志及 3.1 所述的 NPU 排队行为

如需切回单解释器示例，将 `ap/ap_main.c` 中 `TFLM_MULTI_INTERPRETER_DEMO` 置 `0` 后重新编译。

## 5. 测试方案
1. 功能验证：两任务输出稳定且与各自单独运行基线一致（person 约 76% / 28%，cat 约 0.83，dog 约 0.95），无 `Invoke failed` / `AllocateTensors failed` / 内存分配失败日志
2. 并发验证：pet 推理窗口内 person 的 `Invoke time` 上升（NPU 排队）但两任务持续出结果；长时间（>=10 分钟）运行无 hardfault / 死锁 / 结果漂移
3. 性能验证（可选）：统计两任务典型 `Invoke time` 区间；对比 pet 推理窗口内外的 person 耗时，量化 NPU 共享开销
4. 内存验证（可选）：核对初始化阶段内存布局日志（地址与区域）与 3.1 表格；单解释器模式下仍可验证 `PERSON_DETECTION_PERF_TEST` 策略

## 6. 注意事项
1. 本示例推理任务为持续运行，进入循环后不会自动退出
2. Ethos-U scratch、tensor arena 以及（双解释器模式下）大体积 pet 模型的内存占用较大，请确保对应内存区域可用。双解释器模式下 AP flash 使用率达 8M 分区的约 96%（pet 模型 2.68MB 为主因），如需继续增加内容请更换更大 flash 变体
3. 模型数据对齐要求（16 字节）不能破坏，两种示例均已做对齐处理
4. DWT cycles 换算使用固定 `480MHz`，若平台实际主频不同，耗时换算可能会偏差
5. 两个模型均被 Vela 全图融合为单个 `ethos-u` custom op：每次 `Invoke()` 恰好触发一次 NPU 命令流执行，任务间 NPU 争用发生在整次推理粒度（reserve/release）
