# Network Transfer Module

## 概述

network_transfer模块提供了统一的音频网络传输接口，支持多种RTC后端（Volc RTC、Agora RTC等）。

## 主要特性

- 统一的音频发送接口
- 支持多种RTC后端（火山引擎RTC、声网RTC）
- 模块化的设计，易于扩展
- 完整的错误处理和日志
- 支持音频数据接收和编码器类型获取

## 接口说明

### 头文件
```c
#include "network_transfer.h"
```

### 主要函数

#### 初始化/反初始化
```c
int ntwk_trans_init(void);
int ntwk_trans_deinit(void);
```

#### 启动/停止
```c
int ntwk_trans_start(void *user_data);
int ntwk_trans_stop(void *user_data);
```

#### 音频数据发送
```c
int ntwk_trans_send_audio(const uint8_t *data, size_t size, audio_enc_type_t audio_type);
```

#### 音频数据接收
```c
int ntwk_trans_recv_audio(const uint8_t *data, size_t size);
```

#### 网络类型管理
```c
network_type_t ntwk_trans_get_network_type(void);
```

#### 编码器类型获取
```c
audio_enc_type_t ntwk_trans_get_audio_encoder_type(void);
```

## 使用示例

### 基本使用
```c
#include "network_transfer.h"

int main() {
    // 初始化网络传输模块
    if (ntwk_trans_init() != 0) {
        printf("Network transfer init failed\n");
        return -1;
    }
    
    // 启动网络传输
    if (ntwk_trans_start(NULL) != 0) {
        printf("Network transfer start failed\n");
        return -1;
    }
    
    // 获取网络类型
    network_type_t network_type = ntwk_trans_get_network_type();
    printf("Current network type: %d\n", network_type);
    
    // 获取音频编码器类型
    audio_enc_type_t encoder_type = ntwk_trans_get_audio_encoder_type();
    printf("Audio encoder type: %d\n", encoder_type);
    
    // 发送音频数据
    uint8_t audio_data[1024];
    // ... 填充音频数据
    if (ntwk_trans_send_audio(audio_data, sizeof(audio_data), encoder_type) != 0) {
        printf("Send audio failed\n");
    }
    
    // 停止网络传输
    if (ntwk_trans_stop(NULL) != 0) {
        printf("Network transfer stop failed\n");
    }
    
    // 反初始化
    if (ntwk_trans_deinit() != 0) {
        printf("Network transfer deinit failed\n");
    }
    
    return 0;
}
```

## 配置选项

在项目的Kconfig中需要配置：
- `CONFIG_VOLC_RTC_EN` - 启用火山引擎RTC支持
- `CONFIG_AGORA_IOT_SDK` - 启用声网RTC支持
- `CONFIG_BK_AUDIO_ENGINE` - 启用音频引擎支持（用于音频数据接收和编码器类型获取）

## 错误处理

所有函数返回0表示成功，负数表示错误。详细的错误信息会通过日志输出。

## 扩展说明

要添加新的RTC后端支持：
1. 在`network_transfer.h`中添加新的网络类型枚举
2. 在`network_transfer.c`的ntwk_trans_init函数中添加对应的配置和回调函数设置
3. 实现相应的适配器模块（如volc_rtc目录下的火山引擎适配器）
4. 更新配置检查和编译选项

## 文件说明

- `network_transfer.h` - 头文件，包含所有接口定义和数据结构
- `network_transfer.c` - 实现文件，包含网络传输模块的核心功能
- `volc_rtc/` - 火山引擎RTC适配器目录
  - `bk_volc_api.c` - 火山引擎适配器实现
  - `bk_volc_api.h` - 火山引擎适配器头文件
  - `volc_agent_engine.c` - 火山引擎代理引擎
  - `volc_rtc_engine.c` - 火山引擎RTC引擎实现

## 依赖关系

- 需要火山引擎RTC或声网RTC的SDK支持
- 依赖系统日志和内存管理模块
- 依赖音频引擎模块（用于音频数据接收功能）
- 依赖操作系统抽象层（OSAL）模块