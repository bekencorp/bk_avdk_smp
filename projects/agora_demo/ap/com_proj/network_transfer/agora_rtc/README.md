# Agora RTC Integration for AI SO Main Project

## 概述

本目录包含将 Agora RTC SDK 集成到 `ai_so_main` 工程的移植代码。此移植参考了 `volc_rtc` 的架构模式，提供统一的网络传输接口。

## 架构设计

```
agora_rtc/
├── agora_config.h           # Agora SDK 配置（App ID, Token等）
├── agora_rtc_engine.h       # RTC引擎接口定义
├── agora_rtc_engine.c       # RTC引擎核心实现
├── bk_agora_api.h           # 顶层公共API接口
├── bk_agora_api.c           # 顶层API实现（对接network_transfer）
└── README.md                # 本文档
```

### 架构层次

```
Application Layer (app_main.c)
        ↓
Network Transfer Layer (network_transfer.c)
        ↓
Agora API Layer (bk_agora_api.c)
        ↓
Agora Engine Layer (agora_rtc_engine.c)
        ↓
Agora RTC SDK (agora_rtc_api.h)
```

## 主要组件说明

### 1. agora_config.h
包含 Agora RTC SDK 的配置参数：
- **CONFIG_AGORA_APP_ID**: Agora应用ID（必须配置）
- **CONFIG_AGORA_TOKEN**: Token（生产环境必须启用）
- **CONFIG_CHANNEL_NAME**: 默认频道名称
- **CONFIG_MASTER_SERVER_URL**: Agora服务器地址
- **CONFIG_PRODUCT_KEY**: 设备管理平台产品密钥

### 2. agora_rtc_engine.c
RTC引擎核心实现：
- **__agora_rtc_create()**: 创建并初始化RTC实例
- **__agora_rtc_start()**: 启动RTC并加入频道
- **__agora_rtc_stop()**: 停止RTC并离开频道
- **__agora_rtc_destroy()**: 销毁RTC实例
- **事件回调**: 处理连接状态、用户加入/离开等事件

### 3. bk_agora_api.c
应用层API实现：
- **bk_agora_rtc_audio_data_send()**: 发送音频数据
- **bk_agora_rtc_video_data_send()**: 发送视频数据（支持H.264/H.265/JPEG）
- **bk_agora_start()**: 启动Agora服务
- **bk_agora_stop()**: 停止Agora服务
- **帧率控制**: 默认500ms发送一次视频I帧

## 配置方法

### 1. Kconfig 配置

在 `components/network_transfer/Kconfig` 中启用：

```kconfig
config BK_AGORA_RTC
    bool "Enable Agora RTC for network transfer"
    default n
```

使用 `menuconfig` 启用：
```bash
make menuconfig
# 导航到: net_transfer -> Enable Agora RTC for network transfer
```

### 2. 修改 agora_config.h

```c
// 必须配置您自己的 App ID
#define CONFIG_AGORA_APP_ID "your_agora_app_id_here"

// 生产环境建议启用 Token
#define CONFIG_AGORA_TOKEN "your_token_here"

// 可选：修改频道名称
#define CONFIG_CHANNEL_NAME "your_channel_name"
```

### 3. 配置 Room Info

在应用代码中设置房间信息：

```c
#include "bk_agora_api.h"

extern agora_rtc_room_info_t *agora_room_info;

// 初始化房间信息
agora_room_info = (agora_rtc_room_info_t *)psram_malloc(sizeof(agora_rtc_room_info_t));
os_strcpy(agora_room_info->app_id, CONFIG_AGORA_APP_ID);
os_strcpy(agora_room_info->channel_name, "test_channel");
agora_room_info->uid_int = 12345;
os_strcpy(agora_room_info->token, CONFIG_AGORA_TOKEN);
```

## 使用示例

### 1. 基本初始化流程

```c
#include "network_transfer.h"

// 1. 初始化网络传输模块
ntwk_trans_init();

// 2. 启动 RTC（Agora 会自动启动）
ntwk_trans_start(NULL);

// 3. 网络传输模块会自动调用 Agora RTC 的相关接口
```

### 2. 发送音频数据

```c
// 音频数据会通过 network_transfer 自动路由到 Agora
ntwk_trans_send_audio(audio_data, audio_len, AUDIO_ENC_TYPE_G711A);
```

### 3. 发送视频数据

```c
// 视频数据会通过 network_transfer 自动路由到 Agora
frame_buffer_t *frame = ...; // 从 video_engine 获取
ntwk_trans_send_video(frame);
```

### 4. 停止和清理

```c
// 停止网络传输
ntwk_trans_stop(NULL);

// 反初始化
ntwk_trans_deinit();
```

## 特性说明

### 视频传输特性

1. **I帧过滤**: 仅发送H.264 I帧以节省带宽
2. **帧率控制**: 默认500ms发送一次（2 FPS），可通过 `VIDEO_FRAME_INTERVAL` 调整
3. **格式支持**: 
   - H.264 (VIDEO_DATA_TYPE_H264)
   - H.265 (VIDEO_DATA_TYPE_H265)
   - JPEG (VIDEO_DATA_TYPE_GENERIC_JPEG)

### 音频传输特性

1. **编码格式支持**:
   - G.711A (PCMA)
   - G.711U (PCMU)
   - G.722
   - OPUS
   - PCM

2. **自动编码映射**: 从 `audio_enc_type_t` 到 Agora `audio_data_type_e` 的自动转换

### 日志控制

关键日志已默认禁用以减少屏幕刷屏：
- 连接失败日志（仅在未连接时）
- 视频/音频发送成功日志
- 用户静音/取消静音事件

## 与 volc_rtc 的对比

| 特性 | volc_rtc | agora_rtc |
|------|----------|-----------|
| 架构 | 火山引擎RTC | 声网Agora RTC |
| 配置方式 | volc_config.h | agora_config.h |
| API接口 | bk_byte_* | bk_agora_* |
| 视频I帧过滤 | ✓ | ✓ |
| 帧率控制 | ✓ (500ms) | ✓ (500ms) |
| 音频编码 | 多种 | 多种 |
| Agent管理 | ✓ | ✓ (可扩展) |

## 注意事项

1. **App ID 配置**: 必须在 `agora_config.h` 中配置有效的 Agora App ID
2. **Token 安全**: 生产环境必须启用 Token 认证
3. **内存管理**: Room info 使用 `psram_malloc` 分配，注意内存释放
4. **线程安全**: Agora 主线程优先级为 4，栈大小 6KB
5. **WiFi 连接**: 确保设备已连接WiFi后再启动 Agora RTC

## 调试建议

### 1. 启用详细日志

在 `agora_rtc_config` 中设置：
```c
agora_rtc_config.log_disable = false; // 启用 SDK 日志
```

### 2. 检查连接状态

```c
extern bool g_connected_flag;
extern bool g_agent_offline;

if (!g_connected_flag) {
    LOGE("Agora RTC not connected\n");
}
```

### 3. 查看事件消息

事件消息在 `bk_agora_user_notify_msg_handle()` 中处理，可添加日志跟踪：
- JOIN_CHANNEL_SUCCESS
- USER_JOINED
- CONNECTION_LOST
- etc.

## 未来扩展

1. **Agent Engine**: 参考 `volc_agent_engine.c` 添加设备管理功能
2. **数据流控制**: 添加视频流动态码率调整
3. **多路流支持**: 支持多用户视频会议
4. **屏幕共享**: 添加屏幕共享功能

## 参考文档

- [Agora RTC SDK 文档](https://docs.agora.io/cn)
- [Network Transfer 架构](../README_CN.md)
- [VolcEngineRTC 参考实现](../volc_rtc/)

## 版权声明

```
Copyright (C) 2025 Agora IO
All rights reserved.
```


