#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <components/log.h>
#include <driver/aud_dac_types.h>
#include "bta_dac.h"

#define TAG "bta_dac"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#if CONFIG_AUDIO_PLAY
#include "audio_play.h"

#define BTA_DAC_UNMUTE_FRAMES   3U
#define BTA_DAC_DEFAULT_VOLUME  (-24.0f)

typedef struct
{
	uint8_t state;
	uint8_t ready;
	uint8_t unmuted;
	uint8_t frame_count;
	uint8_t gain;
	uint32_t sample_rate;
	uint16_t pcm_length;
	audio_play_t *play;
} bta_dac_t;

static bta_dac_t s_bta_dac;

static float bta_dac_gain_to_db(uint8_t gain)
{
	if (gain == 0)
	{
		return BK_AUD_DAC_DIG_GAIN_DB_SILENCE;
	}

	return BTA_DAC_DEFAULT_VOLUME + ((float)gain / 63.0f) * 8.0f;
}

static void bta_dac_try_unmute(void)
{
	if (!s_bta_dac.ready || s_bta_dac.unmuted)
	{
		return;
	}

	s_bta_dac.frame_count++;
	if (s_bta_dac.frame_count >= BTA_DAC_UNMUTE_FRAMES)
	{
		s_bta_dac.unmuted = 1;
		audio_play_set_volume(s_bta_dac.play, bta_dac_gain_to_db(s_bta_dac.gain));
		audio_play_control(s_bta_dac.play, AUDIO_PLAY_UNMUTE);
	}
}

uint32_t bta_dac_get_fill_size(void)
{
	return 0;
}

uint8_t bta_dac_get_state(void)
{
	return s_bta_dac.state;
}

uint16_t bta_dac_get_pcm_length(void)
{
	return s_bta_dac.pcm_length;
}

void bta_dac_set_gain_value(uint8_t gain)
{
	s_bta_dac.gain = gain;
	if (s_bta_dac.ready && s_bta_dac.unmuted)
	{
		audio_play_set_volume(s_bta_dac.play, bta_dac_gain_to_db(gain));
	}
}

bk_err_t bta_dac_set_gain(uint8_t gain)
{
	bta_dac_set_gain_value(gain);
	return BK_OK;
}

int bta_dac_init(bta_dac_config_t *cfg)
{
	audio_play_cfg_t play_cfg = DEFAULT_AUDIO_PLAY_CONFIG();
	bk_err_t ret;

	if (!cfg)
	{
		return BK_FAIL;
	}

	if (s_bta_dac.play)
	{
		bta_dac_deinit();
	}

	s_bta_dac.sample_rate = cfg->sample_rate ? cfg->sample_rate : 48000U;
	s_bta_dac.pcm_length = (uint16_t)(s_bta_dac.sample_rate * cfg->duration / 1000000U);
	if (s_bta_dac.pcm_length == 0)
	{
		s_bta_dac.pcm_length = 480;
	}

	play_cfg.nChans = 1;
	play_cfg.sampRate = s_bta_dac.sample_rate;
	play_cfg.decoder_type = AUDIO_PLAY_DECODER_PCM;
	play_cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_A2DP_BIT;
	play_cfg.main_dac_source = AUD_DAC_SOURCE_A2DP;
	play_cfg.volume = BK_AUD_DAC_DIG_GAIN_DB_SILENCE;
	play_cfg.frame_size = s_bta_dac.pcm_length * sizeof(int16_t);
	play_cfg.pool_size = play_cfg.frame_size * 8U;

	s_bta_dac.play = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &play_cfg);
	if (!s_bta_dac.play)
	{
		LOGE("audio_play_create failed\n");
		return BK_FAIL;
	}

	ret = audio_play_open(s_bta_dac.play);
	if (ret != BK_OK)
	{
		LOGE("audio_play_open failed ret=%d\n", ret);
		audio_play_destroy(s_bta_dac.play);
		s_bta_dac.play = NULL;
		return BK_FAIL;
	}

	audio_play_control(s_bta_dac.play, AUDIO_PLAY_MUTE);
	s_bta_dac.ready = 1;
	s_bta_dac.unmuted = 0;
	s_bta_dac.frame_count = 0;
	s_bta_dac.state = DAC_STATE_STOP;

	LOGI("init sr=%u frame=%uB\n", s_bta_dac.sample_rate, play_cfg.frame_size);
	return BK_OK;
}

int bta_dac_write(uint8_t *data, uint32_t length)
{
	int written;

	if (!s_bta_dac.ready || !data || length == 0U)
	{
		return -1;
	}

	written = audio_play_write_data(s_bta_dac.play, (char *)data, length);
	if (written <= 0)
	{
		return -1;
	}

	bta_dac_try_unmute();
	if (s_bta_dac.state == DAC_STATE_STOP)
	{
		s_bta_dac.state = DAC_STATE_START;
	}

	return written;
}

int bta_dac_start(void)
{
	s_bta_dac.state = DAC_STATE_START;
	return BK_OK;
}

int bta_dac_stop(void)
{
	s_bta_dac.state = DAC_STATE_STOP;
	return BK_OK;
}

int bta_dac_deinit(void)
{
	if (s_bta_dac.play)
	{
		audio_play_close(s_bta_dac.play);
		audio_play_destroy(s_bta_dac.play);
		s_bta_dac.play = NULL;
	}

	s_bta_dac.ready = 0;
	s_bta_dac.unmuted = 0;
	s_bta_dac.frame_count = 0;
	s_bta_dac.state = DAC_STATE_INVALID;
	return BK_OK;
}

#else

static uint8_t s_bta_dac_gain;

uint32_t bta_dac_get_fill_size(void)
{
	return 0;
}

uint8_t bta_dac_get_state(void)
{
	return DAC_STATE_INVALID;
}

uint16_t bta_dac_get_pcm_length(void)
{
	return 0;
}

void bta_dac_set_gain_value(uint8_t gain)
{
	s_bta_dac_gain = gain;
}

bk_err_t bta_dac_set_gain(uint8_t gain)
{
	bta_dac_set_gain_value(gain);
	return BK_OK;
}

int bta_dac_init(bta_dac_config_t *cfg)
{
	(void)cfg;
	LOGI("CONFIG_AUDIO_PLAY disabled, DAC output is muted\n");
	return BK_OK;
}

int bta_dac_write(uint8_t *data, uint32_t length)
{
	(void)data;
	(void)length;
	return -1;
}

int bta_dac_start(void)
{
	return BK_OK;
}

int bta_dac_stop(void)
{
	return BK_OK;
}

int bta_dac_deinit(void)
{
	return BK_OK;
}

#endif /* CONFIG_AUDIO_PLAY */
