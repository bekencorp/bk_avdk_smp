#include "audio.h"
#include "le_audio_user_config.h"

#include <components/bluetooth/bk_assigned_numbers.h>
#include <components/log.h>
#include <os/str.h>

#define TAG "lea_codec"

static const le_audio_preset_t s_presets[] =
{
	{ "8_2_1",  {  8000, 10000,  30, 1, BK_BT_CODEC_CFG_FREQ_8KHZ,  BK_BT_CODEC_FRAME_DURATION_10000US } },
	{ "16_2_1", { 16000, 10000,  40, 1, BK_BT_CODEC_CFG_FREQ_16KHZ, BK_BT_CODEC_FRAME_DURATION_10000US } },
	{ "24_2_1", { 24000, 10000,  60, 1, BK_BT_CODEC_CFG_FREQ_24KHZ, BK_BT_CODEC_FRAME_DURATION_10000US } },
	{ "32_2_1", { 32000, 10000,  80, 1, BK_BT_CODEC_CFG_FREQ_32KHZ, BK_BT_CODEC_FRAME_DURATION_10000US } },
	{ "48_2_1", { 48000, 10000, 100, 1, BK_BT_CODEC_CFG_FREQ_48KHZ, BK_BT_CODEC_FRAME_DURATION_10000US } },
	{ "48_4_1", { 48000, 10000, 120, 1, BK_BT_CODEC_CFG_FREQ_48KHZ, BK_BT_CODEC_FRAME_DURATION_10000US } },
};

static le_audio_codec_cfg_t s_current;
static const char *s_current_name = "default";
static uint8_t s_inited;

static uint8_t le_audio_codec_freq_enum(uint32_t sr)
{
	switch (sr)
	{
	case 8000:  return BK_BT_CODEC_CFG_FREQ_8KHZ;
	case 16000: return BK_BT_CODEC_CFG_FREQ_16KHZ;
	case 24000: return BK_BT_CODEC_CFG_FREQ_24KHZ;
	case 32000: return BK_BT_CODEC_CFG_FREQ_32KHZ;
	case 44100: return BK_BT_CODEC_CFG_FREQ_44KHZ;
	default:    return BK_BT_CODEC_CFG_FREQ_48KHZ;
	}
}

static void le_audio_codec_init(void)
{
	if (s_inited)
	{
		return;
	}

	s_current.sample_rate = CONFIG_LE_AUDIO_SAMPLE_RATE;
	s_current.frame_us = CONFIG_LE_AUDIO_FRAME_US;
	s_current.frame_bytes = CONFIG_LE_AUDIO_FRAME_BYTES;
	s_current.channels = 1;
	s_current.sf = le_audio_codec_freq_enum(CONFIG_LE_AUDIO_SAMPLE_RATE);
	s_current.fd = (CONFIG_LE_AUDIO_FRAME_US == 7500) ?
	               BK_BT_CODEC_FRAME_DURATION_7500US : BK_BT_CODEC_FRAME_DURATION_10000US;
	s_inited = 1;
}

void le_audio_codec_cfg_default(le_audio_codec_cfg_t *cfg)
{
	if (!cfg)
	{
		return;
	}

	le_audio_codec_init();
	*cfg = s_current;
}

const le_audio_preset_t *le_audio_preset_table(uint8_t *count)
{
	if (count)
	{
		*count = (uint8_t)(sizeof(s_presets) / sizeof(s_presets[0]));
	}
	return s_presets;
}

int le_audio_codec_set_preset(const char *name)
{
	uint8_t i;

	if (!name)
	{
		return -1;
	}

	le_audio_codec_init();
	for (i = 0; i < sizeof(s_presets) / sizeof(s_presets[0]); i++)
	{
		if (os_strcmp(name, s_presets[i].name) == 0)
		{
			s_current = s_presets[i].cfg;
			s_current_name = s_presets[i].name;
			BK_LOGI(TAG, "preset=%s sr=%lu dt=%luus bytes=%u\n",
			        s_current_name, s_current.sample_rate, s_current.frame_us, s_current.frame_bytes);
			return 0;
		}
	}

	BK_LOGW(TAG, "unknown preset '%s'\n", name);
	return -1;
}

const char *le_audio_codec_current_name(void)
{
	return s_current_name;
}
