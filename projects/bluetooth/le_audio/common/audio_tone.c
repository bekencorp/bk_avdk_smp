#include "audio.h"

#include <os/mem.h>

/*
 * Default PCM source: a locally generated integer triangle wave (~480 Hz at
 * 48 kHz, no libm dependency). It lets the full source data path
 * (PCM -> LC3 -> ISO send) run end-to-end without external audio hardware.
 * A real source (PCM clip / USB / line-in) can replace it by implementing
 * le_audio_pcm_source_t and calling le_audio_audio_set_source().
 */

/* Channels get a different period so left and right are told apart by ear */
#define LE_AUDIO_TONE_PERIOD(ch) (100U + (uint32_t)(ch) * 50U)

static uint32_t s_tone_phase[LE_AUDIO_MAX_STREAMS];
static uint8_t s_tone_channels = 1;

static int le_audio_tone_open(const le_audio_codec_cfg_t *cfg)
{
	os_memset(s_tone_phase, 0, sizeof(s_tone_phase));
	s_tone_channels = (cfg && cfg->channels) ? cfg->channels : 1;
	if (s_tone_channels > LE_AUDIO_MAX_STREAMS)
	{
		s_tone_channels = LE_AUDIO_MAX_STREAMS;
	}
	return 0;
}

static int16_t le_audio_tone_sample(uint8_t ch)
{
	uint32_t period = LE_AUDIO_TONE_PERIOD(ch);
	uint32_t ph = s_tone_phase[ch] % period;
	int32_t v;

	if (ph < period / 2)
	{
		v = (int32_t)ph - (int32_t)(period / 4);
	}
	else
	{
		v = (int32_t)(period - ph) - (int32_t)(period / 4);
	}

	s_tone_phase[ch]++;
	return (int16_t)(v * 400);
}

static int le_audio_tone_read(int16_t *pcm, int samples)
{
	int i;

	if (!pcm)
	{
		return -1;
	}

	/* Interleaved, so consecutive samples belong to consecutive channels */
	for (i = 0; i < samples; i++)
	{
		pcm[i] = le_audio_tone_sample((uint8_t)(i % s_tone_channels));
	}

	return samples;
}

static void le_audio_tone_close(void)
{
	os_memset(s_tone_phase, 0, sizeof(s_tone_phase));
}

const le_audio_pcm_source_t *le_audio_source_tone(void)
{
	static const le_audio_pcm_source_t tone =
	{
		.open = le_audio_tone_open,
		.read = le_audio_tone_read,
		.close = le_audio_tone_close,
	};

	return &tone;
}
