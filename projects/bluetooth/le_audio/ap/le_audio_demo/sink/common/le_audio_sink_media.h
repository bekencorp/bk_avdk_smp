#pragma once

#include <stdint.h>

#include <components/bluetooth/bk_dm_bap.h>

#ifdef __cplusplus
extern "C" {
#endif

int le_audio_sink_media_init(void);
int le_audio_sink_media_push_iso(bk_bap_iso_header_t *header, uint8_t *data, uint32_t length);
int le_audio_sink_media_start(void);
void le_audio_sink_media_stop(void);

#ifdef __cplusplus
}
#endif
