#include <stdint.h>
#include <stdbool.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/str.h>
#include "cli.h"
#include "bk_private/bk_cli.h"

#include <driver/aud_adc_types.h>
#include <components/bk_audio_asr_service.h>
#include <components/bk_audio_asr_service_types.h>
#include <components/bk_asr_service.h>
#include <components/bk_asr_service_types.h>

#define CP_ASR_DEFAULT_SAMPLE_RATE    16000u
#define CP_ASR_READ_SIZE              1280u
#define CP_ASR_STUB_HIT_FRAMES        25u
#define CP_ASR_STUB_LOG_FRAMES        64u

typedef struct {
	asr_handle_t asr_handle;
	aud_asr_handle_t aud_asr_handle;
	bool enabled;
	bool hit_reported;
	uint32_t frame_count;
	uint32_t total_bytes;
	uint32_t sample_rate;
	uint32_t aec;
	uint8_t asr_en;
} cp_asr_ctx_t;

static cp_asr_ctx_t s_cp_asr;

static int cp_asr_stub_init(void)
{
	s_cp_asr.frame_count = 0;
	s_cp_asr.total_bytes = 0;
	s_cp_asr.hit_reported = false;
	os_printf("cp_asr kws_stub init: lib=not_linked\r\n");
	return BK_OK;
}

static int cp_asr_stub_recog(void *read_buf, uint32_t read_size, void *p1, void *p2)
{
	(void)p1;
	(void)p2;

	const int16_t *pcm = (const int16_t *)read_buf;
	uint32_t samples = read_size / sizeof(int16_t);
	int result = 0;

	s_cp_asr.frame_count++;
	s_cp_asr.total_bytes += read_size;
	if ((s_cp_asr.frame_count % CP_ASR_STUB_LOG_FRAMES) == 0) {
		uint32_t max_abs = 0;
		uint32_t nonzero = 0;
		for (uint32_t i = 0; i < samples; i++) {
			int32_t sample = pcm[i];
			uint32_t abs_sample = (uint32_t)(sample < 0 ? -sample : sample);
			if (abs_sample > max_abs) {
				max_abs = abs_sample;
			}
			if (sample != 0) {
				nonzero++;
			}
		}
		os_printf("cp_asr frame=%u bytes=%u total=%u max=%u nz=%u aec=%u sr=%u asr=%u kws=stub\r\n",
				  s_cp_asr.frame_count, read_size, s_cp_asr.total_bytes,
				  max_abs, nonzero, s_cp_asr.aec, s_cp_asr.sample_rate,
				  s_cp_asr.asr_en);
	}

	if (!s_cp_asr.hit_reported && s_cp_asr.frame_count >= CP_ASR_STUB_HIT_FRAMES) {
		s_cp_asr.hit_reported = true;
		os_printf("cp_asr kws_stub result=success frames=%u\r\n", s_cp_asr.frame_count);
		result = 1;
	}

	return result;
}

static void cp_asr_stub_deinit(void)
{
	os_printf("cp_asr kws_stub deinit\r\n");
}

static void cp_asr_result_handler(void *p1, void *p2)
{
	(void)p1;
	(void)p2;
	os_printf("cp_asr result handler: recognition success\r\n");
}

int doorbell_asr_turn_off(void)
{
	if (!s_cp_asr.enabled) {
		os_printf("cp_asr already off\r\n");
		return BK_OK;
	}

	os_printf("cp_asr turn_off start\r\n");

	if (s_cp_asr.aud_asr_handle) {
		(void)bk_aud_asr_stop(s_cp_asr.aud_asr_handle);
	}

	if (s_cp_asr.asr_handle) {
		(void)bk_asr_stop(s_cp_asr.asr_handle);
	}

	if (s_cp_asr.aud_asr_handle) {
		(void)bk_aud_asr_deinit(s_cp_asr.aud_asr_handle);
		s_cp_asr.aud_asr_handle = NULL;
	}

	if (s_cp_asr.asr_handle) {
		(void)bk_asr_deinit(s_cp_asr.asr_handle);
		s_cp_asr.asr_handle = NULL;
	}

	s_cp_asr.enabled = false;
	os_printf("cp_asr turn_off done\r\n");
	return BK_OK;
}

static int cp_asr_turn_on(uint32_t aec, uint32_t sample_rate, uint8_t asr_en)
{
	asr_cfg_t asr_cfg = ASR_BY_ONBOARD_MIC_CFG_DEFAULT();
	aud_asr_cfg_t aud_asr_cfg = AUDIO_ASR_CFG_DEFAULT();
	bk_err_t ret;

	if (s_cp_asr.enabled) {
		os_printf("cp_asr already on\r\n");
		return BK_OK;
	}

	if (sample_rate != 8000 && sample_rate != 16000) {
		os_printf("cp_asr unsupported sample_rate=%u\r\n", sample_rate);
		return BK_FAIL;
	}

	os_printf("cp_asr turn_on start: aec=%u sample_rate=%u asr_en=%u\r\n",
			  aec, sample_rate, asr_en);

	asr_cfg.asr_en = (asr_en != 0);
	asr_cfg.aec_en = (aec != 0);
	asr_cfg.asr_rsp_en = false;
	asr_cfg.asr_sample_rate = sample_rate;
	asr_cfg.read_pool_size = CP_ASR_READ_SIZE;
	s_cp_asr.aec = aec;
	s_cp_asr.sample_rate = sample_rate;
	s_cp_asr.asr_en = asr_en;
#if CONFIG_ADK_AEC_V3_ALGORITHM
#if !CONFIG_AUD_AI_NS_SUPPORT
	asr_cfg.aec_cfg.aec_alg_cfg.aec_cfg.ns_type = NS_TRADITION;
#endif
#endif
	asr_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.sample_rate = sample_rate;
	asr_cfg.mic_cfg.onboard_mic_cfg.frame_size = sample_rate * 2 * 20 / 1000;
	asr_cfg.mic_cfg.onboard_mic_cfg.out_block_size = asr_cfg.mic_cfg.onboard_mic_cfg.frame_size;
	asr_cfg.mic_cfg.onboard_mic_cfg.out_block_num = 4;
#if CONFIG_ADK_ONBOARD_MIC_STREAM_V2
	asr_cfg.mic_cfg.onboard_mic_cfg.ch_bitmap = (1 << AUD_ADC_CHL_0);
	asr_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.chl_num = 1;
	asr_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.aec_en = aec;
#endif

	s_cp_asr.asr_handle = bk_asr_create(&asr_cfg);
	if (!s_cp_asr.asr_handle) {
		os_printf("cp_asr bk_asr_create failed\r\n");
		return BK_FAIL;
	}

	ret = bk_asr_init_with_mic(&asr_cfg, s_cp_asr.asr_handle);
	if (ret != BK_OK) {
		os_printf("cp_asr bk_asr_init_with_mic failed: %d\r\n", ret);
		(void)doorbell_asr_turn_off();
		return BK_FAIL;
	}

	aud_asr_cfg.asr_handle = s_cp_asr.asr_handle;
	aud_asr_cfg.mem_type = AUDIO_MEM_TYPE_SRAM;
	aud_asr_cfg.max_read_size = CP_ASR_READ_SIZE;
	aud_asr_cfg.task_stack = 4096;
	aud_asr_cfg.aud_asr_init = cp_asr_stub_init;
	aud_asr_cfg.aud_asr_recog = cp_asr_stub_recog;
	aud_asr_cfg.aud_asr_deinit = cp_asr_stub_deinit;
	aud_asr_cfg.aud_asr_result_handle = cp_asr_result_handler;

	s_cp_asr.aud_asr_handle = bk_aud_asr_init(&aud_asr_cfg);
	if (!s_cp_asr.aud_asr_handle) {
		os_printf("cp_asr bk_aud_asr_init failed\r\n");
		(void)doorbell_asr_turn_off();
		return BK_FAIL;
	}

	ret = bk_asr_start(s_cp_asr.asr_handle);
	if (ret != BK_OK) {
		os_printf("cp_asr bk_asr_start failed: %d\r\n", ret);
		(void)doorbell_asr_turn_off();
		return BK_FAIL;
	}

	ret = bk_aud_asr_start(s_cp_asr.aud_asr_handle);
	if (ret != BK_OK) {
		os_printf("cp_asr bk_aud_asr_start failed: %d\r\n", ret);
		(void)doorbell_asr_turn_off();
		return BK_FAIL;
	}

	s_cp_asr.enabled = true;
	os_printf("cp_asr turn_on done: service pipeline running\r\n");
	return BK_OK;
}

void cli_doorbell_asr_turn_off(void)
{
	(void)doorbell_asr_turn_off();
}

void cli_doorbell_asr_turn_on(uint32_t aec, uint32_t uac, uint32_t sample_rate, uint8_t asr_en)
{
	if (uac != 0) {
		os_printf("cp_asr uac mic is not supported in CP example\r\n");
		return;
	}

	(void)cp_asr_turn_on(aec, sample_rate, asr_en);
}

static void cp_audio_help(void)
{
	os_printf("audio asr_turn_on <aec> <uac> <sample_rate> [asr_en]\r\n");
	os_printf("audio asr_turn_off\r\n");
}

static void cli_cp_audio_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2 || argv[1] == NULL) {
		cp_audio_help();
		return;
	}

	if (os_strcmp(argv[1], "asr_turn_on") == 0) {
		uint32_t aec = 0;
		uint32_t uac = 0;
		uint32_t sample_rate = CP_ASR_DEFAULT_SAMPLE_RATE;
		uint8_t asr_en = 1;

		if (argc > 4) {
			aec = os_strtoul(argv[2], NULL, 10);
			uac = os_strtoul(argv[3], NULL, 10);
			sample_rate = os_strtoul(argv[4], NULL, 10);
			if (argc > 5 && argv[5]) {
				asr_en = os_strtoul(argv[5], NULL, 10);
			}
		}
		cli_doorbell_asr_turn_on(aec, uac, sample_rate, asr_en);
	} else if (os_strcmp(argv[1], "asr_turn_off") == 0) {
		cli_doorbell_asr_turn_off();
	} else {
		cp_audio_help();
	}
}

static const struct cli_command s_cp_audio_commands[] = {
	{"audio", "audio {asr_turn_on|asr_turn_off}", cli_cp_audio_cmd},
};

int cli_cp_audio_init(void)
{
	return cli_register_commands(s_cp_audio_commands,
								 sizeof(s_cp_audio_commands) / sizeof(s_cp_audio_commands[0]));
}
