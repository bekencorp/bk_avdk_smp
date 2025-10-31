# 语音识别服务示例工程

* [English](./README.md)

## 1. 项目概述
本项目是一个语音识别服务测试模块，用于测试Beken平台上的bk_asr_service组件功能。该模块提供了完整的语音识别处理链路测试，包括音频采集、重采样、语音识别(ASR)，并通过命令行接口(CLI)进行配置和控制。

* 有关语音识别服务的API参考，请参阅：

  - [语音识别服务API参考](../../../api-reference/audio/asr_service/index.html)

* 有关语音识别服务的开发指南，请参阅：

  - [语音识别服务开发指南](../../../developer-guide/audio/asr_service/index.html)


## 2. 目录结构
项目采用AP-CP双核架构，主要源代码位于AP目录下。项目结构如下：
```
asr_service_example/
├── .ci                             # CI配置目录
├── .gitignore                      # Git忽略文件配置
├── CMakeLists.txt                  # 项目级CMake构建文件
├── Makefile                        # Make构建文件
├── README.md                       # 英文README
├── README_CN.md                    # 中文README
├── app.rst                         # 应用描述文件
├── ap/                             # AP核代码目录
│   ├── CMakeLists.txt              # AP CMake构建文件
│   ├── ap_main.c                   # 主入口文件，实现CLI命令和测试功能
│   ├── config/                     # AP配置目录
│   │   └── bk7258_ap/              # BK7258 AP配置
│   └── asr_service_test/           # 语音识别服务测试模块目录
│       ├── cli_asr_service.c       # 语音识别服务CLI命令实现
│       └── cli_asr_service.h       # 语音识别服务CLI命令头文件
├── cp/                             # CP核代码目录
│   ├── CMakeLists.txt              # CP CMake构建文件
│   ├── cp_main.c                   # CP端主文件
│   ├── config/                     # CP配置目录
│   │   └── bk7258/                 # BK7258 CP配置
│   ├── customer_msg/               # 客户消息处理模块
│   │   ├── customer_msg.c          # 客户消息处理实现
│   │   └── customer_msg.h          # 客户消息处理头文件
│   └── doorbell_service/           # 门铃服务模块
│       ├── doorbell_comm.h         # 门铃通信头文件
│       └── doorbell_core.c         # 门铃核心实现
├── partitions/                     # 分区配置目录
│   └── bk7258/                     # BK7258分区配置
│       ├── auto_partitions.csv     # 自动分区配置
│       └── ram_regions.csv         # RAM区域配置
└── pj_config.mk                    # 项目配置文件
```

## 3. 功能说明
### 3.1 主要功能
   - **音频采集**：支持板载麦克风(onboard MIC)和UAC麦克风
   - **语音识别**：支持Wanson ASR引擎(用户可以根据需要替换为其他ASR引擎)
   - **重采样支持**：当输入采样率与ASR要求采样率不匹配时，需进行重采样(目前使用的Wanson ASR 要求的采样率为16K).
   - **灵活集成**：支持直接使用麦克风或通过语音服务集成等多种工作模式
   - **采样率支持**：8KHz和16KHz两种采样率
   - **双核通信**：AP-CP双核架构，通过消息机制进行通信

### 3.2 语音识别服务组件架构
   bk_asr_service组件提供了完整的语音识别处理框架：
      - **语音识别服务核心**：管理整个语音识别处理生命周期
      - **语音读取服务**：负责音频数据采集和读取
      - **配置管理**：支持灵活的音频参数配置

### 3.3 配置参数说明

1. **麦克风类型**：

   - `onboard`：板载麦克风
   - `uac`：UAC麦克风

2. **采样率**：

   - `8000`：8KHz采样率
   - `16000`：16KHz采样率

## 4. 编译与运行
### 4.1 编译方法
使用以下命令编译项目：

```
make bk7258 PROJECT=asr_service_example
```

### 4.2 运行方法
编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试语音识别服务功能：

#### 4.2.1 直接使用麦克风启动语音识别服务
**基本格式**：

```
asr_service startwithmic [mic_type] [mic_samp_rate]
```

**示例**：

```
asr_service startwithmic onboard 8000
```

#### 4.2.2 通过语音服务启动语音识别服务
**基本格式**：

```
asr_service startnomic [mic_type] [mic_samp_rate] [aec_en]
```

**参数说明**：

- `mic_type`：麦克风类型（onboard/uac）

- `mic_samp_rate`：麦克风采样率（8000/16000）

- `aec_en`：是否启用回声消除（aec/空）

**示例**：

```
asr_service startnomic onboard 8000 aec
```

#### 4.2.3 停止语音识别服务
**基本格式**：

```
asr_service stop [mic_type] [mic_samp_rate]
```

**示例**：

```
asr_service stop onboard 8000
```

## 5. 测试方案
### 5.1 直接麦克风测试流程
1. 启动语音识别服务，配置麦克风类型和采样率
2. 观察串口输出，检查是否成功识别到唤醒词
3. 停止语音识别服务

### 5.2 通过语音服务测试流程
1. 启动语音服务+语音识别服务，配置麦克风类型、采样率和是否启用回声消除
2. 观察串口输出，检查是否成功识别到唤醒词
3. 停止语音识别服务和语音服务

## 6. 测试示例
1. UAC语音 16KHz ASR测试（直接使用麦克风）

```
顺序发送下述测试命令：
# 开启语音识别服务
ap_cmd asr_service startwithmic uac 16000

# 停止语音识别服务
ap_cmd asr_service stop uac 16000
```

命令执行成功返回：CMDRSP:OK

命令执行失败返回：CMDRSP:ERROR

.. important::

    1. 后续其他测试的执行结果请参考此case的说明，就不再一一赘述。

2. 板载语音 16KHz ASR测试（直接使用麦克风）

```
顺序发送下述测试命令：
# 开启语音识别服务
ap_cmd asr_service startwithmic onboard 16000

# 停止语音识别服务
ap_cmd asr_service stop onboard 16000
```

3. UAC语音 8KHz 重采样 ASR测试（直接使用麦克风）

```
顺序发送下述测试命令：
# 开启语音识别服务
ap_cmd asr_service startwithmic uac 8000

# 停止语音识别服务
ap_cmd asr_service stop uac 8000
```

4. 板载语音 8KHz 重采样 ASR测试（直接使用麦克风）

```
顺序发送下述测试命令：
# 开启语音识别服务
ap_cmd asr_service startwithmic onboard 8000

# 停止语音识别服务
ap_cmd asr_service stop onboard 8000
```

5. 板载语音 8KHz ASR测试（带AEC，通过语音服务）

```
顺序发送下述测试命令：
# 开启语音识别服务（通过语音服务）
ap_cmd asr_service startnomic onboard 8000 aec

# 停止语音识别服务
ap_cmd asr_service stop onboard 8000
```

6. UAC语音 16KHz ASR测试（通过语音服务）

```
顺序发送下述测试命令：
# 开启语音识别服务（通过语音服务）
ap_cmd asr_service startnomic uac 16000

# 停止语音识别服务
ap_cmd asr_service stop uac 16000
```
