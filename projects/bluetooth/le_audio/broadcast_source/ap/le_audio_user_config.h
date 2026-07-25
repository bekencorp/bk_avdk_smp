#pragma once

/*
 * Per-project audio stream configuration for the LE Audio demos.
 * The shared code in ../../common (audio_codec.c / audio_playback.c) reads these
 * CONFIG_LE_AUDIO_* values to build the default LC3 stream config and to size the
 * PCM playback buffers. Override from Kconfig/defconfig if a different codec
 * setting is required.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_LE_AUDIO_SAMPLE_RATE
#define CONFIG_LE_AUDIO_SAMPLE_RATE 48000
#endif

#ifndef CONFIG_LE_AUDIO_FRAME_US
#define CONFIG_LE_AUDIO_FRAME_US 10000
#endif

#ifndef CONFIG_LE_AUDIO_FRAME_BYTES
#define CONFIG_LE_AUDIO_FRAME_BYTES 120
#endif

#ifdef __cplusplus
}
#endif
