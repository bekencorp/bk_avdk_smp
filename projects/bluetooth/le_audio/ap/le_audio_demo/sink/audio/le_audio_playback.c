#include "le_audio_playback.h"
#include "le_audio_user_config.h"

#include <components/log.h>

#if CONFIG_AUDIO_PLAY
#include <driver/aud_dac_types.h>
#include "audio_play.h"
#endif

#define TAG "lea_playback"

#if CONFIG_AUDIO_PLAY
#define LE_AUDIO_PLAYBACK_VOLUME_DB     (-32.0f)
#define LE_AUDIO_PLAY_UNMUTE_FRAMES     3U

static audio_play_t *s_play;
static uint8_t s_play_ready;
static uint8_t s_play_unmuted;
static uint8_t s_play_frame_count;

void le_audio_playback_close(void)
{
	if (!s_play)
	{
		s_play_ready = 0;
		s_play_unmuted = 0;
		s_play_frame_count = 0;
		return;
	}

	audio_play_close(s_play);
	audio_play_destroy(s_play);
	s_play = NULL;
	s_play_ready = 0;
	s_play_unmuted = 0;
	s_play_frame_count = 0;
}

int le_audio_playback_open(void)
{
	audio_play_cfg_t cfg = DEFAULT_AUDIO_PLAY_CONFIG();
	uint32_t frame_pcm_bytes;
	bk_err_t ret;

	if (s_play_ready)
	{
		return 0;
	}

	if (s_play)
	{
		audio_play_close(s_play);
		audio_play_destroy(s_play);
		s_play = NULL;
	}

	cfg.nChans = 1;
	cfg.sampRate = CONFIG_LE_AUDIO_SAMPLE_RATE;
	cfg.decoder_type = AUDIO_PLAY_DECODER_PCM;
	cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_A2DP_BIT;
	cfg.main_dac_source = AUD_DAC_SOURCE_A2DP;
	cfg.volume = BK_AUD_DAC_DIG_GAIN_DB_SILENCE;
	cfg.frame_size = (uint32_t)cfg.sampRate * cfg.nChans / 1000U
	                 * (CONFIG_LE_AUDIO_FRAME_US / 1000U) * cfg.bitsPerSample / 8U;
	cfg.pool_size = cfg.frame_size * 4U;
	frame_pcm_bytes = cfg.frame_size;

	s_play = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &cfg);
	if (!s_play)
	{
		BK_LOGE(TAG, "audio_play_create failed\n");
		return -1;
	}

	ret = audio_play_open(s_play);
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "audio_play_open failed ret=%d\n", ret);
		audio_play_destroy(s_play);
		s_play = NULL;
		return -1;
	}

	audio_play_control(s_play, AUDIO_PLAY_MUTE);
	s_play_ready = 1;
	s_play_unmuted = 0;
	s_play_frame_count = 0;

	BK_LOGI(TAG, "speaker playback open sr=%u frame=%uB pool=%u\n",
	        cfg.sampRate, frame_pcm_bytes, cfg.pool_size);
	return 0;
}

static void le_audio_playback_try_unmute(void)
{
	if (!s_play_ready || s_play_unmuted)
	{
		return;
	}

	s_play_frame_count++;
	if (s_play_frame_count >= LE_AUDIO_PLAY_UNMUTE_FRAMES)
	{
		s_play_unmuted = 1;
		audio_play_set_volume(s_play, LE_AUDIO_PLAYBACK_VOLUME_DB);
		audio_play_control(s_play, AUDIO_PLAY_UNMUTE);
		BK_LOGI(TAG, "speaker playback unmuted, volume=%.1fdB\n", LE_AUDIO_PLAYBACK_VOLUME_DB);
	}
}

void le_audio_playback_write_pcm(const int16_t *pcm, uint32_t sample_count)
{
	uint32_t pcm_bytes;
	int written;

	if (!pcm || sample_count == 0U || !s_play_ready)
	{
		return;
	}

	pcm_bytes = sample_count * sizeof(int16_t);
	/* audio_play_write_data() returns bytes written (via raw_stream_write), not BK_OK. */
	written = (int)audio_play_write_data(s_play, (char *)pcm, pcm_bytes);
	if (written <= 0)
	{
		BK_LOGW(TAG, "audio_play_write_data failed written=%d bytes=%u\n", written, pcm_bytes);
		return;
	}

	le_audio_playback_try_unmute();
}
#else
int le_audio_playback_open(void)
{
	BK_LOGW(TAG, "CONFIG_AUDIO_PLAY is disabled\n");
	return -1;
}

void le_audio_playback_close(void)
{
}

void le_audio_playback_write_pcm(const int16_t *pcm, uint32_t sample_count)
{
	(void)pcm;
	(void)sample_count;
}
#endif /* CONFIG_AUDIO_PLAY */
