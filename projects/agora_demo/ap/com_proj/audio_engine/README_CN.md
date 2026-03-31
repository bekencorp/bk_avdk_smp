# Audio Engine 模块

基于 bk_voice_service 基础设施构建的高级音频引擎模块，提供简化的音频启动/停止操作 API。

## 特性

- **简化 API**：易于使用的 `audio_engine_start()` 和 `audio_engine_stop()` 函数
- **完整错误处理**：详细的错误代码，便于调试
- **灵活配置**：支持多种麦克风/扬声器类型和音频处理功能
- **线程安全**：正确的资源管理和状态跟踪
- **回调支持**：事件和数据读取回调，用于实时音频处理

## API 概述

### 核心函数

- `int audio_engine_start(audio_engine_cfg_t *cfg)` - 使用配置启动音频引擎
- `int audio_engine_stop(void)` - 停止音频引擎并清理资源
- `bool audio_engine_is_running(void)` - 检查音频引擎是否正在运行
- `int audio_engine_write_data(const uint8_t *data, uint32_t size, uint32_t timeout_ms)` - 向扬声器写入音频数据
- `const char *audio_engine_err_to_str(int err)` - 将错误代码转换为字符串

### 配置结构体

```c
typedef struct {
    /* 麦克风配置 */
    mic_type_t mic_type;           // MIC_TYPE_ONBOARD, MIC_TYPE_UAC, MIC_TYPE_ONBOARD_DUAL_DMIC_MIC
    uint32_t mic_sample_rate;      // 8000 或 16000
    
    /* 扬声器配置 */
    spk_type_t spk_type;           // SPK_TYPE_ONBOARD 或 SPK_TYPE_UAC
    uint32_t spk_sample_rate;      // 8000 或 16000
    
    /* 音频处理 */
    uint8_t aec_enable;            // 0: 禁用, 1: 启用 AEC
    uint8_t eq_enable;             // 0: 禁用, 1: 单声道 EQ, 2: 立体声 EQ
    
    /* 编码/解码 */
    audio_enc_type_t enc_type;     // AUDIO_ENC_TYPE_PCM, AUDIO_ENC_TYPE_G711A, AUDIO_ENC_TYPE_G711U
    audio_dec_type_t dec_type;     // AUDIO_DEC_TYPE_PCM, AUDIO_DEC_TYPE_G711A, AUDIO_DEC_TYPE_G711U
    
    /* 回调函数 */
    voice_event_handle event_cb;   // 语音事件回调函数
    audio_engine_read_callback_t read_cb; // 语音读取回调函数
    void *user_data;               // 用户数据，用于回调
} audio_engine_cfg_t;
```

### 错误代码

| 错误代码 | 描述 |
|------------|-------------|
| AUDIO_ENGINE_SUCCESS | 成功 |
| AUDIO_ENGINE_ERR_INVALID_PARAM | 无效参数 |
| AUDIO_ENGINE_ERR_NOT_STARTED | 音频引擎未启动 |
| AUDIO_ENGINE_ERR_VOICE_INIT | 语音初始化失败 |
| AUDIO_ENGINE_ERR_READ_INIT | 语音读取初始化失败 |
| AUDIO_ENGINE_ERR_WRITE_INIT | 语音写入初始化失败 |
| AUDIO_ENGINE_ERR_VOICE_START | 语音启动失败 |
| AUDIO_ENGINE_ERR_READ_START | 语音读取启动失败 |
| AUDIO_ENGINE_ERR_WRITE_START | 语音写入启动失败 |
| AUDIO_ENGINE_ERR_VOICE_STOP | 语音停止失败 |
| AUDIO_ENGINE_ERR_READ_STOP | 语音读取停止失败 |
| AUDIO_ENGINE_ERR_WRITE_STOP | 语音写入停止失败 |

## 快速开始

### 基本用法

```c
#include "audio_engine.h"

/* 回调函数 */
static void voice_read_callback(const uint8_t *data, uint32_t size, void *args)
{
    /* 处理接收到的音频数据 */
}

static bk_err_t voice_event_callback(voice_evt_t event, void *data, void *args)
{
    /* 处理语音事件 */
    return BK_OK;
}

int main(void)
{
    audio_engine_cfg_t cfg = {
        .mic_type = MIC_TYPE_ONBOARD,
        .mic_sample_rate = 16000,
        .spk_type = SPK_TYPE_ONBOARD,
        .spk_sample_rate = 16000,
        .aec_enable = 1,
        .eq_enable = 0,
        .enc_type = AUDIO_ENC_TYPE_G711A,
        .dec_type = AUDIO_DEC_TYPE_G711A,
        .event_cb = voice_event_callback,
        .read_cb = voice_read_callback,
        .user_data = NULL
    };

    /* 启动音频引擎 */
    int ret = audio_engine_start(&cfg);
    if (ret != AUDIO_ENGINE_SUCCESS) {
        printf("启动失败: %s\n", audio_engine_err_to_str(ret));
        return ret;
    }

    /* 音频引擎现在正在运行 */
    
    /* 完成后停止音频引擎 */
    ret = audio_engine_stop();
    if (ret != AUDIO_ENGINE_SUCCESS) {
        printf("停止失败: %s\n", audio_engine_err_to_str(ret));
        return ret;
    }

    return AUDIO_ENGINE_SUCCESS;
}
```

### 错误处理

```c
int ret = audio_engine_start(&cfg);
if (ret != AUDIO_ENGINE_SUCCESS) {
    const char *error_msg = audio_engine_err_to_str(ret);
    printf("错误 %d: %s\n", ret, error_msg);
    
    switch (ret) {
        case AUDIO_ENGINE_ERR_INVALID_PARAM:
            /* 处理无效参数 */
            break;
        case AUDIO_ENGINE_ERR_VOICE_INIT:
            /* 处理语音初始化失败 */
            break;
        /* ... 其他错误情况 */
    }
}
```

## 示例

模块在 `audio_engine_example.c` 中包含全面的示例：

1. **基本语音通话** - 简单的板载麦克风/扬声器配置
2. **高质量语音通话** - 双 DMIC 带 EQ 处理
3. **UAC 音频配置** - USB 音频类设备支持
4. **错误处理演示** - 正确的错误检查和报告
5. **状态检查** - 运行时状态监控

## 构建说明

在您的 component.mk 中添加以下内容：

```makefile
COMPONENT_ADD_INCLUDEDIRS := audio_engine
COMPONENT_SRCDIRS := audio_engine
```

## 依赖项

- bk_voice_service
- bk_voice_read_service  
- bk_voice_write_service
- os 层
- 音频驱动套件组件

## 许可证

Apache License 2.0

## 支持

如有问题和疑问，请参阅音频服务文档或联系开发团队。
