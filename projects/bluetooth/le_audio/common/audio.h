#pragma once

#include <stdint.h>

#include <common/bk_err.h>
#include <components/bluetooth/bk_dm_bap_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One LC3 stream carries one channel, so a stereo stream pair needs two of them */
#define LE_AUDIO_MAX_STREAMS 2

typedef struct
{
	uint32_t sample_rate;
	uint32_t frame_us;
	uint16_t frame_bytes;
	/* Channels of the whole stream group, one LC3 stream per channel */
	uint8_t  channels;
	uint8_t  sf;
	uint8_t  fd;
} le_audio_codec_cfg_t;

/* Audio location of stream index 0 / 1: front left, front right */
uint32_t le_audio_channel_location(uint8_t stream_index);

void le_audio_codec_cfg_default(le_audio_codec_cfg_t *cfg);

typedef struct
{
	const char          *name;
	le_audio_codec_cfg_t cfg;
} le_audio_preset_t;

const le_audio_preset_t *le_audio_preset_table(uint8_t *count);
int le_audio_codec_set_preset(const char *name);
const char *le_audio_codec_current_name(void);

/* PCM is channel interleaved, so samples always counts every channel */
typedef struct
{
	int  (*open)(const le_audio_codec_cfg_t *cfg);
	int  (*read)(int16_t *pcm, int samples);
	void (*close)(void);
} le_audio_pcm_source_t;

typedef struct
{
	int  (*open)(const le_audio_codec_cfg_t *cfg);
	void (*write)(const int16_t *pcm, int samples);
	void (*close)(void);
} le_audio_pcm_sink_t;

void le_audio_audio_set_source(const le_audio_pcm_source_t *src);
void le_audio_audio_set_sink(const le_audio_pcm_sink_t *snk);

const le_audio_pcm_source_t *le_audio_source_tone(void);

typedef bk_err_t (*le_audio_tx_send_fn)(uint16_t handle, uint16_t seq, uint8_t *data, uint16_t len);

/*
 * Send one LC3 stream per handle, handles[i] carrying channel i of the source.
 * Starting with a different handle set replaces the running group.
 */
int le_audio_audio_tx_start(const uint16_t *handles, uint8_t count,
                            le_audio_tx_send_fn send, const le_audio_codec_cfg_t *cfg);
int le_audio_audio_tx_stop(void);
/*
 * Feed the ISO transmit completions reported by the controller. They clock the
 * send thread, so pass every completion through, CIS or BIS. Runs in the host
 * context, so it only releases credits and never blocks.
 */
void le_audio_audio_tx_iso_complete(uint16_t handle, uint16_t num_completed);

int le_audio_audio_rx_init(void);
/*
 * Receive one LC3 stream per handle, handles[i] feeding channel i of the sink.
 * Starting with a different handle set replaces the running group.
 */
int le_audio_audio_rx_start(const uint16_t *handles, uint8_t count,
                            const le_audio_codec_cfg_t *cfg);
void le_audio_audio_rx_stop(void);
int le_audio_audio_rx_push_iso(bk_bap_iso_header_t *header, uint8_t *data, uint32_t length);

int le_audio_playback_open(const le_audio_codec_cfg_t *cfg);
void le_audio_playback_close(void);
void le_audio_playback_write_pcm(const int16_t *pcm, uint32_t sample_count);
/* Apply a VCS volume (0 - 255) and mute state to the speaker. */
void le_audio_playback_set_volume(uint8_t volume, uint8_t mute);
const le_audio_pcm_sink_t *le_audio_playback_sink(void);

#ifdef __cplusplus
}
#endif
