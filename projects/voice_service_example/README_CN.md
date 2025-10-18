# 语音服务示例工程

* [English](./README.md)

## 1. 项目概述
本项目是一个语音服务测试模块，用于测试Beken平台上的bk_voice_service组件功能。该模块提供了完整的语音处理链路测试，包括音频采集、编解码、回声消除(AEC)、音频传输等功能，并通过命令行接口(CLI)进行配置和控制。

* 有关语音通话服务的API参考，请参阅：

  - [语音通话服务API参考](../../../api-reference/audio/voice_service/index.html)

* 有关语音通话服务的开发指南，请参阅：

  - [语音通话服务开发指南](../../../developer-guide/audio/voice_service/index.html)


## 2. 目录结构
项目采用AP-CP双核架构，主要源代码位于AP目录下。项目结构如下：
```
voice_service_example/
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
│   └── voice_service_test/         # 语音服务测试模块目录
│       ├── cli_voice_service.c     # 语音服务CLI命令实现
│       └── cli_voice_service.h     # 语音服务CLI命令头文件
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
   - **音频播放**：支持板载扬声器(onboard SPK)和UAC扬声器
   - **编解码支持**：PCM、G.711A、G.711U、AAC、G.722等多种音频编解码格式
   - **回声消除**：支持AEC回声消除算法，可配置不同版本
   - **音频均衡**：支持单声道和立体声音频均衡调节
   - **采样率支持**：8KHz和16KHz两种采样率
   - **双核通信**：AP-CP双核架构，通过消息机制进行通信

### 3.2 语音服务组件架构
   bk_voice_service组件提供了完整的语音处理框架：
      - **语音服务核心**：管理整个语音处理生命周期
      - **语音读取服务**：负责音频数据采集和读取
      - **语音写入服务**：负责音频数据播放和输出
      - **配置管理**：支持灵活的音频参数配置

### 3.3 配置参数说明

1. **麦克风类型**：

   - `onboard`：板载麦克风
   - `uac`：UAC麦克风
   - `onboard_dual_dmic_mic`：板载双数字麦克风

2. **采样率**：

   - `8000`：8KHz采样率
   - `16000`：16KHz采样率

3. **回声消除**：

   - bit0:0 aec 关闭/1 aec 使能
   - bit1:0 参考信号软回采模式/1 参考信号硬回采模式
   - bit2:0 数字双麦方向角0°/1 数字双麦方向角90°
   - bit3:0 数字双麦主辅麦交换关闭/1 数字双麦主辅麦交换使能
   - bit4:0 不输出仅做回声消除的数据/1 输出仅做回声消除的数据

4. **编解码格式**：

   - `pcm`：原始PCM数据
   - `g711a`：G.711 A-law
   - `g711u`：G.711 μ-law
   - `aac`：AAC编解码
   - `g722`：G.722编解码

5. **扬声器类型**：

   - `onboard`：板载扬声器
   - `uac`：UAC扬声器

6. **音频均衡**：

   - `0`：禁用均衡
   - `1`：单声道均衡
   - `2`：立体声均衡


## 4. 编译与运行
### 4.1 编译方法
使用以下命令编译项目：
```
make bk7258 PROJECT=voice_service_example
```

### 4.2 运行方法
编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试语音服务功能：

#### 4.2.1 启动语音服务
**基本格式**：
```
voice_service start [mic_type] [mic_samp_rate] [aec_en] [enc_type] [dec_type] [spk_type] [spk_samp_rate] [eq_type]
```


**示例**：
```
voice_service stop onboard 8000 1 g711a g711a onboard 8000 0
```

#### 4.2.2 停止语音服务
**基本格式**：
```
voice_service stop  [mic_type] [mic_samp_rate] [aec_en] [enc_type] [dec_type] [spk_type] [spk_samp_rate] [eq_type]
```

**示例**：
```
voice_service stop onboard 8000 1 g711a g711a onboard 8000 0
```

## 5. 测试方案
使用麦克风和扬声器进行本地音频自回环测试，测试流程如下：
1. 启动语音服务，配置麦克风和扬声器为板载麦克风和板载扬声器
2. 播放音频文件到麦克风
3. 监听扬声器，检查是否能听到播放的音频
4. 停止语音服务

## 6. 测试示例
1. 板载语音 G711A 8KHz AEC 测试
```
顺序发送下述测试命令：
# 开启语音通话服务
ap_cmd voice_service start onboard 8000 1 g711a g711a onboard 8000 0

# 停止语音通话服务
ap_cmd voice_service stop onboard 8000 1 g711a g711a onboard 8000 0
```

命令执行成功返回：CMDRSP:OK
命令执行失败返回：CMDRSP:ERROR

```
启动语音通话服务成功后会在固定时间间隔打印类似下述信息的内部组件的工作时的数据统计信息：
ap0:count_ut:D(36128):[ONBOARD_MIC] data_size: 63680(Bytes), 15KB/s
ap0:count_ut:D(36128):[RAW_READ] data_size: 30720(Bytes), 7KB/s 
ap0:count_ut:D(36130):[RAW_WRITE] data_size: 30400(Bytes), 7KB/s 
ap0:count_ut:D(36132):[ONBOARD_SPK] data_size: 59840(Bytes), 14KB/s 
ap0:count_ut:D(36134):[WIFI_TX] data_size: 30720(Bytes), 7KB/s 
ap0:count_ut:D(36134):[WIFI_RX] data_size: 30720(Bytes), 7KB/s 
```

.. important::

    1. 后续其他测试的执行结果请参考此cas的说明，就不再一一赘述。


2. 板载语音 G711A 16KHz AEC 测试

```
顺序发送下述测试命令：
# 开启语音通话服务
ap_cmd voice_service start onboard 16000 1 g711a g711a onboard 16000 0

# 停止语音通话服务
ap_cmd voice_service stop onboard 16000 1 g711a g711a onboard 16000 0
```

3. uac语音 G711A 8KHz AEC 测试

```
顺序发送下述测试命令：
# 开启语音通话服务
ap_cmd voice_service start uac 8000 1 g711a g711a uac 8000 0

# 停止语音通话服务
ap_cmd voice_service stop uac 8000 1 g711a g711a uac 8000 0
```


4. 板载语音 aac 8KHz AEC 测试

```
顺序发送下述测试命令：
# 开启语音通话服务
ap_cmd voice_service start onboard 8000 1 aac aac onboard 8000 0

# 停止语音通话服务
ap_cmd voice_service stop onboard 8000 1 aac aac onboard 8000 0
```

5. 板载语音 g722 16KHz AEC 测试

```
顺序发送下述测试命令：
# 开启语音通话服务
ap_cmd voice_service start onboard 16000 1 g722 g722 onboard 16000 0

# 停止语音通话服务
ap_cmd voice_service stop onboard 16000 1 g722 g722 onboard 16000 0
```
