# 播放器服务示例工程

* [English](./README.md)

## 1. 项目概述
本项目是一个播放器服务测试模块，用于测试Beken平台上的bk_player_service组件功能。该模块提供了完整的音频播放链路测试，包括音频数据源、编解码、音频输出等功能，并通过命令行接口(CLI)进行配置和控制。

* 有关播放器服务的API参考，请参阅：

  - [播放器服务API参考](../../../api-reference/audio/player_service/index.html)

* 有关播放器服务的开发指南，请参阅：

  - [播放器服务开发指南](../../../developer-guide/audio/player_service/index.html)

## 2. 目录结构
项目采用AP-CP双核架构，主要源代码位于AP目录下。项目结构如下：
```
player_service_example/
├── .ci                             # CI配置目录
├── .gitignore                      # Git忽略文件配置
├── CMakeLists.txt                  # 项目级CMake构建文件
├── Makefile                        # Make构建文件
├── README.md                       # 英文README
├── README_CN.md                    # 中文README
├── ap/                             # AP核代码目录
│   ├── CMakeLists.txt              # AP CMake构建文件
│   ├── ap_main.c                   # 主入口文件，实现CLI命令和测试功能
│   ├── config/                     # AP配置目录
│   │   └── bk7258_ap/              # BK7258 AP配置
│   └── player_service_test/        # 播放器服务测试模块目录
│       ├── cli_player_service.c    # 播放器服务CLI命令实现
│       ├── cli_player_service.h    # 播放器服务CLI命令头文件
│       └── prompt_tone_test.h      # 提示音测试数据头文件
│   └── voice_service_test/         # 语音通话服务测试模块目录
│       ├── cli_voice_service.c     # 语音通话服务CLI命令实现
│       └── cli_voice_service.h     # 语音通话服务CLI命令头文件
├── cp/                             # CP核代码目录
│   ├── CMakeLists.txt              # CP CMake构建文件
│   ├── cp_main.c                   # CP端主文件
│   ├── config/                     # CP配置目录
│   │   └── bk7258/                 # BK7258 CP配置
│   └── customer_msg/               # 客户消息处理模块
│       ├── customer_msg.c          # 客户消息处理实现
│       └── customer_msg.h          # 客户消息处理头文件
├── partitions/                     # 分区配置目录
│   └── bk7258/                     # BK7258分区配置
│       ├── auto_partitions.csv     # 自动分区配置
│       └── ram_regions.csv         # RAM区域配置
└── pj_config.mk                    # 项目配置文件
```

## 3. 功能说明
### 3.1 主要功能
   - **音频播放**：默认支持板载扬声器(onboard SPK)和UAC扬声器
   - **音频源支持**：支持数组(array)和文件系统(vfs)两种音频数据源
   - **编解码支持**：MP3、WAV等多种音频格式

### 3.3 配置参数说明

播放器服务支持两种类型的命令：
   - ``playback`` （音频播放）: 测试独立播放音频功能
   - ``prompt_tone`` （提示音播放）: 测试多音频播放时插播提示音功能

#### 3.3.1 Playback命令参数

1. **命令格式**：
   ```
   player_service playback [cmd] [source_type] [info]
   ```

2. **参数说明**：

   - ``[cmd]`` ：命令类型
      - ``start`` ：开始播放
      - ``stop`` ：停止播放

   - ``[source_type]`` ：音频源类型
      - ``array`` ：数组音频源，使用内置提示音数据
      - ``vfs`` ：文件系统音频源，从文件系统读取音频文件

   - ``[info]`` ：音频信息
      - 当 ``source_type`` 为 ``array`` 时， ``info`` 为数组ID：
         - ``0`` ：ASR唤醒提示音（pcm格式）
         - ``1`` ：网络配置提示音（mp3格式）
         - ``2`` ：低电压提示音（wav格式）
      - 当 ``source_type`` 为 ``vfs`` 时， ``info`` 为文件路径，例如 ``/data/test.wav``

#### 3.3.2 Prompt Tone命令参数

1. **命令格式**：
   ```
   player_service prompt_tone [cmd] [tone_id] [source_type] [info]
   ```

2. **参数说明**：

   - ``[cmd]`` ：命令类型
      - ``start`` ：开始播放
      - ``stop`` ：停止播放

   - ``[tone_id]`` ：提示音ID
      - ``0`` ：第一个提示音
      - ``1`` ：第二个提示音
      - ``2`` ：第三个提示音

   - ``[source_type]`` ：音频源类型
      - ``array`` ：数组音频源，使用内置提示音数据
      - ``vfs`` ：文件系统音频源，从文件系统读取音频文件

   - ``[info]`` ：音频信息
      - 当 ``source_type`` 为 ``array`` 时， ``info`` 为数组ID：
         - ``0`` ：ASR唤醒提示音（pcm格式）
         - ``1`` ：网络配置提示音（mp3格式）
         - ``2`` ：低电压提示音（wav格式）
      - 当 ``source_type`` 为 ``vfs`` 时， ``info`` 为文件路径，例如 ``/data/test.wav`` 

## 4. 编译与运行
### 4.1 编译方法
使用以下命令编译项目：

.. code-block:: bash

   make bk7258 PROJECT=player_service_example

### 4.2 运行方法
编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试播放器服务功能：

#### 4.2.1 Playback命令（音频播放）
**基本格式**：

.. code-block:: bash

   player_service playback [cmd] [source_type] [info]

**示例**：

.. code-block:: bash

   # 开始播放ASR唤醒提示音
   player_service playback start array 0

   # 开始播放文件系统中的音频文件
   player_service playback start vfs /sd0/test.wav

   # 停止播放
   player_service playback stop

#### 4.2.2 Prompt Tone命令（提示音播放）
**基本格式**：

.. code-block:: bash

   player_service prompt_tone [cmd] [tone_id] [source_type] [info]

**示例**：

.. code-block:: bash

   # 开始播放第一个提示音（ASR唤醒提示音）
   player_service prompt_tone start 1 array 0

   # 开始播放第二个提示音（网络配置提示音）
   player_service prompt_tone start 2 array 1

   # 开始播放文件系统中的音频文件作为提示音
   player_service prompt_tone start 1 vfs /sd0/test.wav

   # 停止提示音播放
   player_service prompt_tone stop 1

## 5. 测试方案
使用扬声器进行音频播放测试，测试流程如下：

1. 选择合适的播放命令（playback或prompt_tone）
2. 配置音频源类型和信息
3. 监听扬声器，检查是否能听到播放的音频
4. 发送停止命令结束播放

## 6. 测试示例
1. Playback命令测试 - 数组音频播放

.. code-block:: bash

   顺序发送下述测试命令：
   # 开始播放ASR唤醒提示音
   player_service playback start array 0

   # 停止播放
   player_service playback stop

2. Playback命令测试 - 文件音频播放

.. code-block:: bash

   顺序发送下述测试命令：
   # 开始播放文件系统中的音频文件
   player_service playback start vfs /data/test.wav

   # 停止播放
   player_service playback stop

3. Prompt Tone命令测试 - 插播数组提示音

.. code-block:: bash

   顺序发送下述测试命令：
   # 启动语音通话功能
   ap_cmd voice_service start onboard 8000 1 g711a g711a onboard 8000 0

   # 开始插播提示音
   player_service prompt_tone start 1 array 0

   # 停止提示音播放
   player_service prompt_tone stop

   # 关闭语音通话功能
   ap_cmd voice_service stop onboard 8000 1 g711a g711a onboard 8000 0

4. Prompt Tone命令测试 - 插播文件提示音

.. code-block:: bash

   顺序发送下述测试命令：
   # 启动语音通话功能
   ap_cmd voice_service start onboard 8000 1 g711a g711a onboard 8000 0

   # 开始播放文件系统中的音频文件作为提示音
   player_service prompt_tone start 1 vfs /sd0/test.wav

   # 停止播放
   player_service prompt_tone stop 1

   # 关闭语音通话功能
   ap_cmd voice_service stop onboard 8000 1 g711a g711a onboard 8000 0

命令执行成功返回： ``CMDRSP:OK``
命令执行失败返回： ``CMDRSP:ERROR``

.. important::

   有关语音通话功能命令的使用请参阅： `语音通话服务示例工程 <../../../examples/projects/voice_service_example/index.html>`_
