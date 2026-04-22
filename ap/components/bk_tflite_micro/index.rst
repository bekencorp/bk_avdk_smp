**bk_tflite_micro 开发指南**
=================================

**1. 简介**
---------------------------------

    ``bk_tflite_micro`` 组件是在官方 `TensorFlow Lite for Microcontrollers (TFLite-Micro)`_ 组件基础上完成的平台化适配，
    结合 Beken 芯片的软件栈，增加了对芯片内置 NPU 的支持能力。

    当前适配的 NPU 型号为 **Arm Ethos-U65**，芯片侧最高提供 **0.24 TOPS** 推理算力。
    组件通过构建系统将官方 TFLite-Micro、CMSIS-NN 与 Ethos-U 驱动整合到 Armino 工程中，使开发者可以在 MCU/RTOS 环境下运行
    经过离线编译优化后的 `.tflite` 模型。

    对于支持 NPU 的模型，推荐工作流如下：

    1. 在训练框架中导出量化后的 `.tflite` 模型。
    2. 使用 **MLIA** 对模型进行兼容性和性能分析，提前识别不支持算子、CPU 回退和内存瓶颈，并据此优化模型结构。
    3. 使用 **Vela** 对优化后的模型进行离线编译，生成适合 Ethos-U65 执行的 `_vela.tflite` 模型。
    4. 将编译后的模型集成到工程中，通过 ``bk_tflite_micro`` 执行推理。


* 有关bk_tflite_micro的示例工程，请参阅：

  - `bk_tflite_micro示例工程 <../../../projects/tflite_micro/tflite_micro_example/index.html>`_


**2. TFLite-Micro 概述**
---------------------------------

    TFLite-Micro 是 TensorFlow 面向微控制器场景的轻量级推理运行时，主要特点包括：

    - 不依赖动态文件系统和复杂运行时环境，适合裸机或 RTOS 场景。
    - 通过 ``tflite::MicroInterpreter`` 管理模型、张量和算子执行。
    - 通过 ``MicroMutableOpResolver`` 注册模型所需的算子，减小代码体积。
    - 使用开发者提供的 ``tensor arena`` 作为张量与中间缓冲区内存池。

    在官方 TFLite-Micro 运行时中，算子默认运行在 CPU 上；而在 ``bk_tflite_micro`` 中，当工程启用 NPU 能力后，
    可通过 ``resolver.AddEthosU()`` 将支持的算子下发到 Ethos-U65 执行，不支持的算子仍由 TFLite-Micro/CMSIS-NN 在 CPU 上执行。

    这意味着：

    - **模型不需要全部算子都能跑在 NPU 上**。
    - **是否真正使用到 NPU，取决于模型量化情况、算子类型和 Vela 编译结果**。

    TFLite Micro 使用参考指南：
    `TFLite Micro 使用参考指南 <https://www.tensorflow.org/lite/microcontrollers?hl=zh-cn>`_
    其中包含 ``person_detection`` 等基础示例，可用于参考和学习。


**3. 芯片 NPU 能力概述**
---------------------------------

    本芯片集成的 NPU 为 **Arm Ethos-U65 256MAC**。
    在 ``bk_tflite_micro`` 组件中，当工程启用 NPU 能力并且模型经过 Vela 编译后，运行时可将支持的算子下发到 Ethos-U65 执行，
    不支持的算子则继续由 TFLite-Micro/CMSIS-NN 在 CPU 上执行。

    芯片侧最高提供 **0.24 TOPS** 推理算力。实际可获得的加速效果取决于以下因素：

    - 模型是否为 Ethos-U 友好的量化模型
    - 模型中的算子是否属于 NPU 支持范围
    - 张量形状、通道数和内存布局是否适合 NPU 执行
    - Vela 编译后是否存在较多 CPU fallback


    NPU支持的算子如下：

    - ``ABS``
    - ``ADD``
    - ``ARG_MAX``
    - ``AVERAGE_POOL_2D``
    - ``CONCATENATION``
    - ``CONV_2D``
    - ``DEPTHWISE_CONV_2D``
    - ``EXP``
    - ``EXPAND_DIMS``
    - ``FULLY_CONNECTED``
    - ``HARD_SWISH``
    - ``LEAKY_RELU``
    - ``LOGISTIC``
    - ``MAXIMUM``
    - ``MAX_POOL_2D``
    - ``MEAN``
    - ``MINIMUM``
    - ``MIRROR_PAD``
    - ``MUL``
    - ``PACK``
    - ``PAD``
    - ``PRELU``
    - ``QUANTIZE``
    - ``RELU``
    - ``RELU6``
    - ``RELU_0_TO_1``
    - ``RELU_N1_TO_1``
    - ``RESHAPE``
    - ``RESIZE_BILINEAR``
    - ``RESIZE_NEAREST_NEIGHBOR``
    - ``RSQRT``
    - ``SHAPE``
    - ``SLICE``
    - ``SOFTMAX``
    - ``SPLIT``
    - ``SPLIT_V``
    - ``SQUARED_DIFFERENCE``
    - ``SQUEEZE``
    - ``STRIDED_SLICE``
    - ``SUB``
    - ``TANH``
    - ``TRANSPOSE``
    - ``TRANSPOSE_CONV``
    - ``UNIDIRECTIONAL_SEQUENCE_LSTM``
    - ``UNPACK``

    说明：

    - 上述列表表示 NPU 侧支持的算子类型，并不等同于“任意参数组合都必然完全下发”。
    - 某些算子即使类型受支持，也可能因张量维度、量化形式、参数约束或子图切分方式而回退到 CPU。
    - 建议始终结合 **MLIA 分析结果** 和 **Vela 编译结果** 来判断模型的实际 NPU 覆盖率。

**4. MLIA 工具分析模型**
---------------------------------

**4.1 工具简介**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    MLIA 的全称是 **Arm ML Inference Advisor**。
    它用于在部署前分析模型在 Arm 目标硬件上的兼容性、性能和优化空间，适合作为 Vela 编译模型前的辅助分析工具。

    对 ``bk_tflite_micro`` 开发者来说，MLIA 主要用于回答以下问题：

    - 模型中哪些算子能映射到 Ethos-U65，哪些会回退到 CPU。
    - 模型推理可能的性能瓶颈在哪里。
    - 是否存在可通过改网络结构、改通道数、改算子形式来提升 NPU 利用率的空间。

**4.2 安装说明**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    python3 -m venv .venv
    source .venv/bin/activate
    pip install --upgrade pip
    pip install mlia

    安装完成后可执行：

::

    mlia --help
    mlia-target list

**4.3 常用分析命令**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    性能分析示例：

::

    mlia check model.tflite \
      --target-profile ethos-u65-256 \
      --performance

    建议：

    - 在导出原始量化模型后，先做一次 MLIA 分析，确认算子兼容性。
    - 在 Vela 编译后结合编译日志一起分析，确认是否存在大量 CPU fallback。

**4.4 分析结果如何使用**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    当 MLIA 报告存在如下情况时，通常需要重新优化模型：

    - 大量算子无法映射到 NPU。
    - 某些卷积层通道数、张量形状与 NPU 并行度不匹配，导致利用率偏低。
    - 峰值 SRAM 占用过高，超出系统为 NPU / tensor arena 预留的空间。

    在实际项目中，建议把 **MLIA 分析 + Vela 编译结果 + 实板日志** 结合起来看，而不要只依赖单一工具结论。

**4.5 官方说明链接**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    - `Arm MLIA GitHub <https://github.com/arm/mlia>`_
    - `ML Platform: Arm MLIA <https://www.mlplatform.org/mlia/>`_


**5. Vela 工具处理 TFLite 模型**
---------------------------------

**5.1 工具简介**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Vela 是 Arm 为 Ethos-U 系列 NPU 提供的离线编译优化工具。
    它会读取量化后的 `.tflite` 模型，对其中可由 Ethos-U 执行的子图进行编译与替换，生成包含 NPU command stream 的优化模型。

    对 ``bk_tflite_micro`` 而言，**Vela 是模型定型后的离线编译步骤**。通常建议先用 MLIA 分析模型兼容性与性能瓶颈，
    在模型结构和量化策略基本稳定后，再使用 Vela 生成最终部署到 Ethos-U65 的模型。
    未经 Vela 处理的普通 `.tflite` 模型虽然仍可在 TFLite-Micro 上运行，但通常无法发挥 Ethos-U65 的硬件加速能力。

**5.2 安装说明**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    推荐在独立 Python 虚拟环境中安装：

::

    python3 -m venv .venv
    source .venv/bin/activate
    pip install --upgrade pip
    pip install ethos-u-vela

    安装完成后，可使用以下命令确认版本：

::

    vela --version



**5.3 使用说明**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Vela 的输入通常是 **量化后的** `.tflite` 模型，输出为用于 Ethos-U 的优化模型。
    示例命令如下：

::

    vela model.tflite \
      --output-dir ./vela_out \
      --accelerator-config ethos-u65-256 \
      --config Arm/vela.ini \
      --system-config Ethos_U65_High_End \
      --memory-mode Dedicated_Sram \
      --optimise Performance

    说明：

    - ``--accelerator-config``：指定 Ethos-U 目标配置。示例中使用 ``ethos-u65-256`` 作为参考配置，实际项目请以芯片集成配置为准。
    - ``--system-config`` / ``--memory-mode``：描述目标系统的 SRAM/DRAM 组织方式。
    - ``--optimise Performance``：优先优化推理速度；若更关注 SRAM 峰值，可考虑 ``Size``。
    - ``--config``：指定使用 ``bk_tflite_micro`` 组件根目录下的 ``vela.ini`` 配置文件。
    - ``model.tflite``：输入模型文件。

    运行完成后，通常会得到类似 ``model_vela.tflite`` 的输出模型。建议将该模型作为后续固件集成对象，而不是原始 `.tflite` 文件。

**5.4 集成建议**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    在工程集成阶段，常见做法如下：

    1. 先使用 MLIA 分析原始量化模型，并根据报告优化模型结构或参数。
    2. 对优化后的模型运行 Vela，生成 ``*_vela.tflite``。
    3. 将 ``*_vela.tflite`` 转换为 C 数组或二进制资源，随固件一起编译。
    4. 运行时通过 ``tflite::GetModel()`` 加载 Vela 产物。

    本仓库示例工程中，模型数据文件 ``tflm_person_detection_model_vela_data.cc`` 就属于该类集成方式。
    如果项目支持外部存储器如sdcard，也可以从sdcard中加载模型数据。

**5.5 官方说明链接**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    - `Arm Developer: Ethos-U Vela compiler <https://developer.arm.com/documentation/109267/latest/Tool-support-for-the-Arm-Ethos-U-NPU/Ethos-U-Vela-compiler>`_
    - `PyPI: ethos-u-vela <https://pypi.org/project/ethos-u-vela/>`_


**6. bk_tflite_micro 组件使用示例**
---------------------------------

**6.1 组件使能**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    工程中至少需要打开以下配置：

    - ``CONFIG_TFLITE_MICRO=y``
    - ``CONFIG_NPU=y`` 

**6.2 基本接入流程**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    一个典型的接入流程如下：

    1. 准备经过 Vela 处理后的 ``*_vela.tflite`` 模型数据。
    2. 为 Ethos-U scratch buffer 分配高速内存。
    3. 调用 ``bk_ethosu_init()`` 初始化 NPU 驱动。
    4. 为 ``tensor arena`` 和模型数据分配运行时内存。
    5. 使用 ``tflite::GetModel()`` 和 ``tflite::MicroInterpreter`` 构建解释器。
    6. 在 ``MicroMutableOpResolver`` 中注册需要的算子，并调用 ``AddEthosU()``。
    7. 调用 ``AllocateTensors()`` 完成张量分配。
    8. 填充输入张量并执行 ``Invoke()``。

**6.3 代码示例**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: cpp

    #include "ethosu_driver.h"
    #include "bk_ethosu.h"
    #include "tensorflow/lite/micro/micro_interpreter.h"
    #include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
    #include "tensorflow/lite/schema/schema_generated.h"

    #define TFLM_ARENA_SIZE       (100 * 1024)
    #define ETHOSU0_SCRATCH_SIZE  (250 * 1024)

    uint8_t *tensor_arena = ...;          // runtime allocated
    uint8_t *model_data   = ...;          // Vela output model
    void *ethosu_scratch  = ...;          // 16-byte aligned fast memory

    /* 申请16字节对齐的高速的sram作为NPU的scratch buffer */
    ethosu_scratch = hsram_malloc(ETHOSU0_SCRATCH_SIZE);
    /* 申请psram存储模型 */
    model_data = psram_malloc(model_data_size);
    /* 申请psram作为tensor arena */
    tensor_arena = psram_malloc(TFLM_ARENA_SIZE);

    /* 初始化 NPU 驱动 */
    bk_ethosu_init(ethosu_scratch, ETHOSU0_SCRATCH_SIZE);

    /* 加载模型 */
    const tflite::Model *model = tflite::GetModel(model_data);

    /* 注册算子 */
    static tflite::MicroMutableOpResolver<13> resolver;
    /* 注册 Ethos-U 算子（如果模型中存在NPU不支持的算子，还需要单独注册对应的cpu算子） */
    resolver.AddEthosU();

    /* 创建解释器 */
    static tflite::MicroInterpreter interpreter(model,
                                                resolver,
                                                tensor_arena,
                                                TFLM_ARENA_SIZE);

    /* 分配张量 */
    if (interpreter.AllocateTensors() != kTfLiteOk) {
        // handle error
    }

    /* 执行推理 */
    if (interpreter.Invoke() != kTfLiteOk) {
        // handle error
    }


**7. NPU 使用注意事项**
---------------------------------

**7.1 模型必须优先考虑量化**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Ethos-U65 主要面向量化模型。
    在实际项目中，应使用 int8 量化模型，并通过 Vela 检查模型是否能够被 NPU 有效加速。

**7.2 不要忽略 CPU fallback**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    即使工程使能了 NPU，也不代表整个模型都会在 NPU 上执行。
    若模型中包含 Ethos-U 不支持的算子或张量形式，这部分仍会退回 CPU 执行，可能导致：

    - 推理时间明显增加
    - tensor arena 占用升高
    - 开发者误以为“已经启用 NPU，但性能没有提升”

    因此建议在部署前同时查看：

    - MLIA 兼容性分析结果
    - Vela 编译日志
    - 实板推理耗时日志

**7.3 重点关注三类内存**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    ``bk_tflite_micro`` 场景下至少要规划以下三类内存：

    - **Ethos-U scratch**：NPU 运行时高速工作区，建议放在 HSRAM 等高速内存中，需要保证16字节对齐
    - **tensor arena**：TFLite-Micro 运行时张量池
    - **model data**：Vela 输出模型数据，也需要单独规划存储位置，需要保证16字节对齐

    本仓库的 ``person_detection`` 示例中，分别使用了：

    - ``ETHOSU0_SCRATCH_SIZE = 250 * 1024``
    - ``TFLM_ARENA_SIZE = 100 * 1024``

    这些数值仅供参考，实际大小取决于模型结构、算子种类和 Vela 编译结果。

**7.4 注意 16 字节对齐**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Ethos-U 对 command stream 和相关缓冲区有对齐要求。
    示例代码中对 model data 和 scratch buffer 都做了 **16 字节对齐** 处理，实际开发中不应破坏这一约束。

**7.5 高速内存优先**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    对性能敏感场景，建议优先保证：

    - Ethos-U scratch 放在 HSRAM
    - tensor arena 放在高速或带宽较高的内存区域
    - 若系统允许，再考虑将模型数据放在 PSRAM/外部内存

    若高速内存不足，应评估是否需要：

    - 调整模型规模
    - 调整 Vela 优化策略
    - 调整 arena 预算
    - 接受部分算子回退 CPU


**8. 参考资料**
---------------------------------

    - `TensorFlow Lite for Microcontrollers <https://github.com/tensorflow/tflite-micro>`_
    - `Arm Ethos-U Vela compiler documentation <https://developer.arm.com/documentation/109267/latest/Tool-support-for-the-Arm-Ethos-U-NPU/Ethos-U-Vela-compiler>`_
    - `PyPI: ethos-u-vela <https://pypi.org/project/ethos-u-vela/>`_
    - `Arm MLIA GitHub <https://github.com/arm/mlia>`_
    - `ML Platform: Arm MLIA <https://www.mlplatform.org/mlia/>`_
    - `Arm Ethos-U65 product page <https://www.arm.com/products/silicon-ip-cpu/ethos/ethos-u65>`_
    - 示例工程源码路径：``projects/tflite_micro/tflite_micro_example``


.. _TensorFlow Lite for Microcontrollers: https://github.com/tensorflow/tflite-micro
