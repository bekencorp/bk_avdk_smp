#include "audio.h"

/*
 * Default PCM source: a locally generated integer triangle wave (~480 Hz at
 * 48 kHz, no libm dependency). It lets the full source data path
 * (PCM -> LC3 -> ISO send) run end-to-end without external audio hardware.
 * A real source (PCM clip / USB / line-in) can replace it by implementing
 * le_audio_pcm_source_t and calling le_audio_audio_set_source().
 */

static uint32_t s_tone_phase;

static int le_audio_tone_open(const le_audio_codec_cfg_t *cfg)
{
	(void)cfg;
	s_tone_phase = 0;
	return 0;
}

static int le_audio_tone_read(int16_t *pcm, int samples)
{
	const uint32_t period = 100;
	int i;

	if (!pcm)
	{
		return -1;
	}

	for (i = 0; i < samples; i++)
	{
		uint32_t ph = s_tone_phase % period;
		int32_t v;

		if (ph < period / 2)
		{
			v = (int32_t)ph - (int32_t)(period / 4);
		}
		else
		{
			v = (int32_t)(period - ph) - (int32_t)(period / 4);
		}

		pcm[i] = (int16_t)(v * 400);
		s_tone_phase++;
	}

	return samples;
}

static void le_audio_tone_close(void)
{
	s_tone_phase = 0;
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
