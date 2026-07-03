#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int le_audio_playback_open(void);
void le_audio_playback_close(void);
void le_audio_playback_write_pcm(const int16_t *pcm, uint32_t sample_count);

#ifdef __cplusplus
}
#endif
