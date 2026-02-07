# 音频播放器示例工程

* [English](./README.md)

## 1. 项目概述
本项目是一个音频播放器测试模块，用于测试Beken平台上的bk_audio_player组件功能。该模块提供了完整的音频播放功能，包括播放列表管理、播放控制、音量调节、播放模式设置等功能，并通过命令行接口(CLI)进行配置和控制。支持从SD卡读取和播放多种格式的音频文件。

* 有关音频播放器服务的API参考，请参阅：

  - [音频播放器服务API参考](../../../api-reference/audio/audio_player/index.html)

* 有关音频播放器服务的开发指南，请参阅：

  - [音频播放器服务开发指南](../../../developer-guide/audio/audio_player/index.html)


### 1.1 测试环境

   * 硬件配置：
      * 核心板，**BK7258_QFN88_9X9_V3.2**
      * 存有mp3音乐的sdcard
      * Speaker V1小板和喇叭


## 2. 目录结构
项目采用AP-CP双核架构，主要源代码位于AP目录下。项目结构如下：
```
audio_player_example/
├── .ci                             # CI配置目录
├── .gitignore                      # Git忽略文件配置
├── .it.csv                         # 集成测试用例配置文件
├── CMakeLists.txt                  # 项目级CMake构建文件
├── Makefile                        # Make构建文件
├── README.md                       # 英文README
├── README_CN.md                    # 中文README
├── app.rst                         # 应用描述文件
├── ap/                             # AP核代码目录
│   ├── CMakeLists.txt              # AP CMake构建文件
│   ├── ap_main.c                   # 主入口文件，实现初始化和CLI命令注册
│   ├── config/                     # AP配置目录
│   │   └── bk7258_ap/              # BK7258 AP配置
│   └── audio_player_test/          # 音频播放器测试模块目录
│       └── cli_audio_player.c      # 音频播放器CLI命令实现
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
   - **音频格式支持**：支持MP3、WAV、M4A、AMR、TS、AAC等多种音频格式
   - **播放控制**：支持播放、暂停、恢复、停止等基本播放控制
   - **播放列表管理**：支持添加、删除、清空播放列表
   - **播放模式**：支持顺序播放、单曲循环、列表循环等多种播放模式
   - **音量控制**：支持音量调节和查询
   - **播放导航**：支持上一首、下一首、跳转到指定歌曲
   - **SD卡支持**：支持SD卡挂载、卸载和自动扫描音频文件
   - **文件系统**：基于VFS和FATFS文件系统，支持FAT格式SD卡

### 3.2 音频播放器组件架构
   bk_audio_player组件提供了完整的音频播放框架：
      - **播放器核心**：管理整个音频播放生命周期
      - **解码器**：支持多种音频格式的解码
      - **播放列表管理**：维护和管理播放队列
      - **音频输出**：负责音频数据输出到扬声器
      - **配置管理**：支持灵活的播放参数配置

### 3.3 播放模式说明

音频播放器支持多种播放模式：

- **顺序播放模式**：按照播放列表顺序依次播放，播放完最后一首后停止
- **单曲循环模式**：重复播放当前歌曲
- **列表循环模式**：循环播放整个播放列表，播放完最后一首后从第一首继续
- **随机播放模式**：随机选择播放列表中的歌曲播放


## 4. 编译与运行
### 4.1 编译方法
使用以下命令编译项目：
```
make bk7258 PROJECT=audio_player_example
```

### 4.2 运行方法
编译完成后，将生成的固件烧录到开发板上，然后通过串口终端使用以下命令测试音频播放器功能：

#### 4.2.1 挂载SD卡
**命令格式**：
```
audio_player sd_mount
```

**功能说明**：挂载SD卡到/sd0目录，使得音频播放器能够访问SD卡中的音频文件。

**示例**：
```
ap_cmd audio_player sd_mount
```

#### 4.2.2 初始化音频播放器
**命令格式**：
```
audio_player init
```

**功能说明**：使用默认配置初始化音频播放器，准备播放环境。

**示例**：
```
ap_cmd audio_player init
```

#### 4.2.3 添加音频文件到播放列表
**命令格式**：
```
audio_player add [id] [file_path]
```

**参数说明**：
- ``id`` ：音频文件的唯一标识符
- ``file_path`` ：音频文件的完整路径（如：/sd0/test.mp3）

**示例**：
```
ap_cmd audio_player add 1 /sd0/test.mp3
```

#### 4.2.4 扫描SD卡音频文件
**命令格式**：
```
audio_player sd_scan
```

**功能说明**：自动扫描SD卡根目录及一级子目录中的音频文件，并添加到播放列表。

**示例**：
```
ap_cmd audio_player sd_scan
```

#### 4.2.5 播放控制

##### 开始播放
**命令格式**：
```
audio_player start
```

**功能说明**：开始播放播放列表中的第一首歌曲。

**示例**：
```
ap_cmd audio_player start
```

##### 暂停播放
**命令格式**：
```
audio_player pause
```

**功能说明**：暂停当前播放的歌曲。

**示例**：
```
ap_cmd audio_player pause
```

##### 恢复播放
**命令格式**：
```
audio_player resume
```

**功能说明**：恢复之前暂停的歌曲播放。

**示例**：
```
ap_cmd audio_player resume
```

##### 停止播放
**命令格式**：
```
audio_player stop
```

**功能说明**：停止当前播放的歌曲。

**示例**：
```
ap_cmd audio_player stop
```

#### 4.2.6 播放导航

##### 上一首
**命令格式**：
```
audio_player prev
```

**功能说明**：播放播放列表中的上一首歌曲。

**示例**：
```
ap_cmd audio_player prev
```

##### 下一首
**命令格式**：
```
audio_player next
```

**功能说明**：播放播放列表中的下一首歌曲。

**示例**：
```
ap_cmd audio_player next
```

##### 跳转到指定歌曲
**命令格式**：
```
audio_player jump [id]
```

**参数说明**：
- ``id`` ：要跳转到的歌曲的标识符

**示例**：
```
ap_cmd audio_player jump 3
```

#### 4.2.7 音量控制

##### 设置音量
**命令格式**：
```
audio_player volume [level]
```

**参数说明**：
- ``level`` ：音量等级（通常范围0-100）

**示例**：
```
ap_cmd audio_player volume 50
```

##### 查询音量
**命令格式**：
```
audio_player volume
```

**功能说明**：查询当前音量等级。

**示例**：
```
ap_cmd audio_player volume
```

#### 4.2.8 播放模式设置
**命令格式**：
```
audio_player mode [mode_value]
```

**参数说明**：

- ``mode_value`` ：播放模式值

   - ``0`` ：顺序播放
   - ``1`` ：单曲循环
   - ``2`` ：列表循环
   - ``3`` ：随机播放

**示例**：
```
ap_cmd audio_player mode 1
```

#### 4.2.9 播放列表管理

##### 删除指定音频文件
**命令格式**：
```
audio_player rm [file_path]
```

**参数说明**：
- ``file_path`` ：要删除的音频文件路径

**示例**：
```
ap_cmd audio_player rm /sd0/test.mp3
```

##### 清空播放列表
**命令格式**：
```
audio_player clear
```

**功能说明**：清空播放列表中的所有歌曲。

**示例**：
```
ap_cmd audio_player clear
```

##### 显示播放列表
**命令格式**：
```
audio_player dump
```

**功能说明**：显示当前播放列表中的所有歌曲信息。

**示例**：
```
ap_cmd audio_player dump
```

#### 4.2.10 去初始化音频播放器
**命令格式**：
```
audio_player deinit
```

**功能说明**：释放音频播放器资源，清理播放环境。

**示例**：
```
ap_cmd audio_player deinit
```

#### 4.2.11 卸载SD卡
**命令格式**：
```
audio_player sd_unmount
```

**功能说明**：卸载SD卡，释放文件系统资源。

**示例**：
```
ap_cmd audio_player sd_unmount
```


## 5. 测试方案
使用SD卡存储音频文件进行播放测试，测试流程如下：
1. 准备SD卡，在根目录或子目录中放置测试音频文件（如test.mp3）
2. 将SD卡插入开发板的SD卡槽
3. 挂载SD卡到文件系统
4. 初始化音频播放器
5. 添加音频文件到播放列表
6. 开始播放并测试各种播放控制功能
7. 停止播放并释放资源
8. 卸载SD卡

## 6. 测试示例

### 6.1 基本播放测试

测试基本的音频播放功能，包括初始化、添加文件、播放、暂停、恢复、停止、去初始化等完整流程。

```
顺序发送下述测试命令：
# 挂载SD卡
ap_cmd audio_player sd_mount

# 初始化音频播放器
ap_cmd audio_player init

# 添加音频文件到播放列表
ap_cmd audio_player add 1 /sd0/test.mp3

# 开始播放
ap_cmd audio_player start

# 暂停播放
ap_cmd audio_player pause

# 恢复播放
ap_cmd audio_player resume

# 停止播放
ap_cmd audio_player stop

# 去初始化音频播放器
ap_cmd audio_player deinit

# 卸载SD卡
ap_cmd audio_player sd_unmount
```

命令执行成功返回：**CMDRSP:OK**

命令执行失败返回：**CMDRSP:ERROR**


### 6.2 自动扫描播放测试

测试自动扫描SD卡中的音频文件并播放的功能。

```
顺序发送下述测试命令：
# 挂载SD卡
ap_cmd audio_player sd_mount

# 初始化音频播放器
ap_cmd audio_player init

# 自动扫描SD卡音频文件
ap_cmd audio_player sd_scan

# 显示播放列表
ap_cmd audio_player dump

# 开始播放
ap_cmd audio_player start

# 停止播放
ap_cmd audio_player stop

# 去初始化音频播放器
ap_cmd audio_player deinit

# 卸载SD卡
ap_cmd audio_player sd_unmount
```


### 6.3 播放列表导航测试

测试播放列表中的歌曲切换功能。

```
顺序发送下述测试命令：
# 挂载SD卡
ap_cmd audio_player sd_mount

# 初始化音频播放器
ap_cmd audio_player init

# 添加多个音频文件
ap_cmd audio_player add 1 /sd0/song1.mp3
ap_cmd audio_player add 2 /sd0/song2.mp3
ap_cmd audio_player add 3 /sd0/song3.mp3

# 开始播放第一首
ap_cmd audio_player start

# 切换到下一首
ap_cmd audio_player next

# 切换到上一首
ap_cmd audio_player prev

# 跳转到第三首
ap_cmd audio_player jump 3

# 停止播放
ap_cmd audio_player stop

# 去初始化音频播放器
ap_cmd audio_player deinit

# 卸载SD卡
ap_cmd audio_player sd_unmount
```


### 6.4 播放模式测试

测试不同播放模式的功能。

```
顺序发送下述测试命令：
# 挂载SD卡
ap_cmd audio_player sd_mount

# 初始化音频播放器
ap_cmd audio_player init

# 添加音频文件
ap_cmd audio_player add 1 /sd0/test.mp3

# 设置单曲循环模式
ap_cmd audio_player mode 1

# 开始播放
ap_cmd audio_player start

# （观察歌曲播放完后自动重复播放）

# 停止播放
ap_cmd audio_player stop

# 去初始化音频播放器
ap_cmd audio_player deinit

# 卸载SD卡
ap_cmd audio_player sd_unmount
```


### 6.5 音量控制测试

测试音量调节功能。

```
顺序发送下述测试命令：
# 挂载SD卡
ap_cmd audio_player sd_mount

# 初始化音频播放器
ap_cmd audio_player init

# 添加音频文件
ap_cmd audio_player add 1 /sd0/test.mp3

# 开始播放
ap_cmd audio_player start

# 设置音量为50
ap_cmd audio_player volume 50

# 查询当前音量
ap_cmd audio_player volume

# 设置音量为80
ap_cmd audio_player volume 80

# 停止播放
ap_cmd audio_player stop

# 去初始化音频播放器
ap_cmd audio_player deinit

# 卸载SD卡
ap_cmd audio_player sd_unmount
```


### 6.6 播放列表管理测试

测试播放列表的添加、删除、清空功能。

```
顺序发送下述测试命令：
# 挂载SD卡
ap_cmd audio_player sd_mount

# 初始化音频播放器
ap_cmd audio_player init

# 添加多个音频文件
ap_cmd audio_player add 1 /sd0/song1.mp3
ap_cmd audio_player add 2 /sd0/song2.mp3
ap_cmd audio_player add 3 /sd0/song3.mp3

# 显示播放列表
ap_cmd audio_player dump

# 删除一个音频文件
ap_cmd audio_player rm /sd0/song2.mp3

# 再次显示播放列表
ap_cmd audio_player dump

# 清空播放列表
ap_cmd audio_player clear

# 显示播放列表（应为空）
ap_cmd audio_player dump

# 去初始化音频播放器
ap_cmd audio_player deinit

# 卸载SD卡
ap_cmd audio_player sd_unmount
```


## 7. 注意事项

1. **SD卡格式**：SD卡必须格式化为FAT32或FAT16文件系统，否则无法正常挂载
2. **音频文件格式**：确保音频文件为支持的格式（MP3、WAV、M4A、AMR、TS、AAC）
3. **文件路径**：添加音频文件时必须使用完整路径，如 `/sd0/test.mp3`
4. **操作顺序**：必须先挂载SD卡并初始化播放器，才能进行播放操作
5. **资源释放**：测试完成后应该依次停止播放、去初始化播放器、卸载SD卡，以正确释放资源
6. **播放列表**：开始播放前必须确保播放列表中至少有一个音频文件
7. **错误处理**：如果命令执行失败，系统会自动尝试停止播放器、去初始化并卸载SD卡


