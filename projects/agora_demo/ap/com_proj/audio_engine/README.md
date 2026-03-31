# Audio Engine Module

A high-level audio engine module that provides simplified APIs for audio start/stop operations, built on top of the bk_voice_service infrastructure.

## Features

- **Simplified API**: Easy-to-use `audio_engine_start()` and `audio_engine_stop()` functions
- **Comprehensive Error Handling**: Detailed error codes for easy debugging
- **Flexible Configuration**: Support for various microphone/speaker types and audio processing features
- **Thread Safety**: Proper resource management and state tracking
- **Callback Support**: Event and data read callbacks for real-time audio processing

## API Overview

### Core Functions

- `int audio_engine_start(audio_engine_cfg_t *cfg)` - Start audio engine with configuration
- `int audio_engine_stop(void)` - Stop audio engine and cleanup resources
- `bool audio_engine_is_running(void)` - Check if audio engine is running
- `int audio_engine_write_data(const uint8_t *data, uint32_t size, uint32_t timeout_ms)` - Write audio data to speaker
- `const char *audio_engine_err_to_str(int err)` - Convert error code to string

### Configuration Structure

```c
typedef struct {
    /* Microphone configuration */
    mic_type_t mic_type;           // MIC_TYPE_ONBOARD, MIC_TYPE_UAC, MIC_TYPE_ONBOARD_DUAL_DMIC_MIC
    uint32_t mic_sample_rate;      // 8000 or 16000
    
    /* Speaker configuration */
    spk_type_t spk_type;           // SPK_TYPE_ONBOARD or SPK_TYPE_UAC
    uint32_t spk_sample_rate;      // 8000 or 16000
    
    /* Audio processing */
    uint8_t aec_enable;            // 0: disable, 1: enable AEC
    uint8_t eq_enable;             // 0: disable, 1: mono EQ, 2: stereo EQ
    
    /* Encoding/Decoding */
    audio_enc_type_t enc_type;     // AUDIO_ENC_TYPE_PCM, AUDIO_ENC_TYPE_G711A, AUDIO_ENC_TYPE_G711U
    audio_dec_type_t dec_type;     // AUDIO_DEC_TYPE_PCM, AUDIO_DEC_TYPE_G711A, AUDIO_DEC_TYPE_G711U
    
    /* Callback functions */
    voice_event_handle event_cb;   // Voice event callback function
    audio_engine_read_callback_t read_cb; // Audio engine read callback function
    void *user_data;               // User data for callbacks
} audio_engine_cfg_t;
```

### Error Codes

| Error Code | Description |
|------------|-------------|
| AUDIO_ENGINE_SUCCESS | Success |
| AUDIO_ENGINE_ERR_INVALID_PARAM | Invalid parameters |
| AUDIO_ENGINE_ERR_NOT_STARTED | Audio engine not started |
| AUDIO_ENGINE_ERR_VOICE_INIT | Voice init failed |
| AUDIO_ENGINE_ERR_READ_INIT | Voice read init failed |
| AUDIO_ENGINE_ERR_WRITE_INIT | Voice write init failed |
| AUDIO_ENGINE_ERR_VOICE_START | Voice start failed |
| AUDIO_ENGINE_ERR_READ_START | Voice read start failed |
| AUDIO_ENGINE_ERR_WRITE_START | Voice write start failed |
| AUDIO_ENGINE_ERR_VOICE_STOP | Voice stop failed |
| AUDIO_ENGINE_ERR_READ_STOP | Voice read stop failed |
| AUDIO_ENGINE_ERR_WRITE_STOP | Voice write stop failed |

## Quick Start

### Basic Usage

```c
#include "audio_engine.h"

/* Callback functions */
static void voice_read_callback(const uint8_t *data, uint32_t size, void *args)
{
    /* Process received audio data */
}

static bk_err_t voice_event_callback(voice_evt_t event, void *data, void *args)
{
    /* Handle voice events */
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

    /* Start audio engine */
    int ret = audio_engine_start(&cfg);
    if (ret != AUDIO_ENGINE_SUCCESS) {
        printf("Failed to start: %s\n", audio_engine_err_to_str(ret));
        return ret;
    }

    /* Audio engine is now running */
    
    /* Stop audio engine when done */
    ret = audio_engine_stop();
    if (ret != AUDIO_ENGINE_SUCCESS) {
        printf("Failed to stop: %s\n", audio_engine_err_to_str(ret));
        return ret;
    }

    return AUDIO_ENGINE_SUCCESS;
}
```

### Error Handling

```c
int ret = audio_engine_start(&cfg);
if (ret != AUDIO_ENGINE_SUCCESS) {
    const char *error_msg = audio_engine_err_to_str(ret);
    printf("Error %d: %s\n", ret, error_msg);
    
    switch (ret) {
        case AUDIO_ENGINE_ERR_INVALID_PARAM:
            /* Handle invalid parameters */
            break;
        case AUDIO_ENGINE_ERR_VOICE_INIT:
            /* Handle voice init failure */
            break;
        /* ... other error cases */
    }
}
```

## Examples

The module includes comprehensive examples in `audio_engine_example.c`:

1. **Basic voice call** - Simple onboard mic/speaker configuration
2. **High quality voice call** - Dual DMIC with EQ processing
3. **UAC audio configuration** - USB Audio Class device support
4. **Error handling demonstration** - Proper error checking and reporting
5. **Status checking** - Runtime status monitoring

## Build Instructions

Add the following to your component.mk:

```makefile
COMPONENT_ADD_INCLUDEDIRS := audio_engine
COMPONENT_SRCDIRS := audio_engine
```

## Dependencies

- bk_voice_service
- bk_voice_read_service  
- bk_voice_write_service
- os layer
- audio driver kit components

## License

Apache License 2.0

## Support

For issues and questions, please refer to the audio service documentation or contact the development team.
