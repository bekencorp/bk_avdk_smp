#include <stdint.h>
#include <stdbool.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/str.h>
#include "cli.h"
#include "bk_private/bk_cli.h"

#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_decoders/g711_decoder.h>
#include <components/bk_audio/audio_streams/raw_stream.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream_v2.h>

#define CP_PROMPT_SAMPLE_RATE       16000u
#define CP_PROMPT_FRAME_MS          20u
#define CP_PROMPT_FRAME_BYTES       (CP_PROMPT_SAMPLE_RATE * 2u * CP_PROMPT_FRAME_MS / 1000u)
#define CP_PROMPT_DEFAULT_MS        500u
#define CP_PROMPT_MAX_MS            3000u

/* One short 16 kHz mono G711A cycle; repeated by the array source during smoke playback. */
static const uint8_t s_prompt_g711a_cycle[] = {
	0xd5, 0xed, 0x9d, 0x87, 0x82, 0x88, 0xb4, 0xb6,
	0xb3, 0xbd, 0xbc, 0xbe, 0xb9, 0xb8, 0xbb, 0xba,
	0xba, 0xba, 0xbb, 0xb8, 0xb9, 0xbe, 0xbc, 0xbd,
	0xb3, 0xb6, 0xb4, 0x88, 0x82, 0x87, 0x9d, 0xed,
	0xd5, 0x6d, 0x1d, 0x07, 0x02, 0x08, 0x34, 0x36,
	0x33, 0x3d, 0x3c, 0x3e, 0x39, 0x38, 0x3b, 0x3a,
	0x3a, 0x3a, 0x3b, 0x38, 0x39, 0x3e, 0x3c, 0x3d,
	0x33, 0x36, 0x34, 0x08, 0x02, 0x07, 0x1d, 0x6d,
};

typedef struct {
	audio_pipeline_handle_t pipeline;
	audio_element_handle_t raw_write;
	audio_element_handle_t decoder;
	audio_element_handle_t speaker;
} cp_prompt_player_t;

static void cp_prompt_help(void)
{
	os_printf("cp_prompt play [ms]\r\n");
}

static void cp_prompt_player_cleanup(cp_prompt_player_t *player)
{
	if (!player) {
		return;
	}

	if (player->pipeline) {
		(void)audio_pipeline_stop(player->pipeline);
		(void)audio_pipeline_wait_for_stop(player->pipeline);
		(void)audio_pipeline_terminate(player->pipeline);
	}

	if (player->pipeline && player->raw_write) {
		(void)audio_pipeline_unregister(player->pipeline, player->raw_write);
	}

	if (player->pipeline && player->speaker) {
		(void)audio_pipeline_unregister(player->pipeline, player->speaker);
	}

	if (player->pipeline && player->decoder) {
		(void)audio_pipeline_unregister(player->pipeline, player->decoder);
	}

	if (player->raw_write) {
		(void)audio_element_deinit(player->raw_write);
		player->raw_write = NULL;
	}

	if (player->decoder) {
		(void)audio_element_deinit(player->decoder);
		player->decoder = NULL;
	}

	if (player->speaker) {
		(void)audio_element_deinit(player->speaker);
		player->speaker = NULL;
	}

	if (player->pipeline) {
		(void)audio_pipeline_deinit(player->pipeline);
		player->pipeline = NULL;
	}
}

static bk_err_t cp_prompt_player_init(cp_prompt_player_t *player)
{
	audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
	g711_decoder_cfg_t decoder_cfg = DEFAULT_G711_DECODER_CONFIG();
	onboard_speaker_stream_cfg_t speaker_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
	bk_err_t ret;

	if (!player) {
		return BK_FAIL;
	}

	pipeline_cfg.rb_size = CP_PROMPT_FRAME_BYTES * 2;
	player->pipeline = audio_pipeline_init(&pipeline_cfg);
	if (!player->pipeline) {
		os_printf("cp_prompt pipeline init failed\r\n");
		return BK_FAIL;
	}

	raw_cfg.type = AUDIO_STREAM_WRITER;
	raw_cfg.out_block_size = CP_PROMPT_FRAME_BYTES / 2;
	raw_cfg.out_block_num = 2;
	player->raw_write = raw_stream_init(&raw_cfg);
	if (!player->raw_write) {
		os_printf("cp_prompt raw stream init failed\r\n");
		goto fail;
	}

	decoder_cfg.buf_sz = CP_PROMPT_FRAME_BYTES / 2;
	decoder_cfg.out_block_size = CP_PROMPT_FRAME_BYTES;
	decoder_cfg.out_block_num = 2;
	decoder_cfg.dec_mode = G711_DEC_MODE_A_LOW;
	player->decoder = g711_decoder_init(&decoder_cfg);
	if (!player->decoder) {
		os_printf("cp_prompt g711 decoder init failed\r\n");
		goto fail;
	}

	speaker_cfg.chl_num = 1;
	speaker_cfg.bits = 16;
	speaker_cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_HINT_BIT;
	speaker_cfg.main_dac_source = AUD_DAC_SOURCE_HINT;
	speaker_cfg.sample_rate[AUD_DAC_SOURCE_HINT] = CP_PROMPT_SAMPLE_RATE;
	speaker_cfg.frame_size[AUD_DAC_SOURCE_HINT] = CP_PROMPT_FRAME_BYTES;
	speaker_cfg.pool_length = CP_PROMPT_FRAME_BYTES * 6;
	speaker_cfg.pool_play_thold = CP_PROMPT_FRAME_BYTES;
	speaker_cfg.pool_pause_thold = CP_PROMPT_FRAME_BYTES / 2;
	player->speaker = onboard_speaker_stream_init(&speaker_cfg);
	if (!player->speaker) {
		os_printf("cp_prompt speaker init failed\r\n");
		goto fail;
	}

	ret = audio_pipeline_register(player->pipeline, player->raw_write, "raw_write");
	if (ret != BK_OK) {
		os_printf("cp_prompt register raw failed: %d\r\n", ret);
		goto fail;
	}

	ret = audio_pipeline_register(player->pipeline, player->decoder, "decode");
	if (ret != BK_OK) {
		os_printf("cp_prompt register decoder failed: %d\r\n", ret);
		goto fail;
	}

	ret = audio_pipeline_register(player->pipeline, player->speaker, "speaker");
	if (ret != BK_OK) {
		os_printf("cp_prompt register speaker failed: %d\r\n", ret);
		goto fail;
	}

	ret = audio_pipeline_link(player->pipeline, (const char *[]){"raw_write", "decode", "speaker"}, 3);
	if (ret != BK_OK) {
		os_printf("cp_prompt pipeline link failed: %d\r\n", ret);
		goto fail;
	}

	ret = audio_pipeline_run(player->pipeline);
	if (ret != BK_OK) {
		os_printf("cp_prompt pipeline run failed: %d\r\n", ret);
		goto fail;
	}

	return BK_OK;

fail:
	cp_prompt_player_cleanup(player);
	return BK_FAIL;
}

static bk_err_t cp_prompt_write_array_g711a(audio_element_handle_t raw_write, uint32_t duration_ms)
{
	uint32_t total_samples = CP_PROMPT_SAMPLE_RATE * duration_ms / 1000u;
	uint32_t cycle_samples = sizeof(s_prompt_g711a_cycle) / sizeof(s_prompt_g711a_cycle[0]);
	uint32_t sample_idx = 0;
	int ret;

	while (sample_idx < total_samples) {
		ret = raw_stream_write(raw_write, (char *)s_prompt_g711a_cycle,
							   sizeof(s_prompt_g711a_cycle));
		if (ret <= 0) {
			os_printf("cp_prompt write failed: %d\r\n", ret);
			return BK_FAIL;
		}
		sample_idx += cycle_samples;
	}

	return BK_OK;
}

static bk_err_t cp_prompt_play_array(uint32_t duration_ms)
{
	cp_prompt_player_t player = {0};
	bk_err_t ret;

	if (duration_ms == 0 || duration_ms > CP_PROMPT_MAX_MS) {
		os_printf("cp_prompt invalid duration: %u\r\n", duration_ms);
		return BK_FAIL;
	}

	ret = cp_prompt_player_init(&player);
	if (ret != BK_OK) {
		return ret;
	}

	os_printf("cp_prompt array g711a playback start: %u ms\r\n", duration_ms);
	ret = cp_prompt_write_array_g711a(player.raw_write, duration_ms);
	rtos_delay_milliseconds(duration_ms + 100);
	cp_prompt_player_cleanup(&player);
	os_printf("cp_prompt array g711a playback done: ret=%d\r\n", ret);
	return ret;
}

int cp_prompt_play_success(void)
{
	return cp_prompt_play_array(CP_PROMPT_DEFAULT_MS);
}

static void cli_cp_prompt_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2 || argv[1] == NULL) {
		cp_prompt_help();
		return;
	}

	if (os_strcmp(argv[1], "play") == 0) {
		uint32_t duration_ms = CP_PROMPT_DEFAULT_MS;
		if (argc > 2 && argv[2]) {
			duration_ms = os_strtoul(argv[2], NULL, 10);
		}
		(void)cp_prompt_play_array(duration_ms);
	} else {
		cp_prompt_help();
	}
}

static const struct cli_command s_cp_prompt_commands[] = {
	{"cp_prompt", "cp_prompt {play [ms]}", cli_cp_prompt_cmd},
};

int cli_cp_prompt_init(void)
{
	return cli_register_commands(s_cp_prompt_commands,
								 sizeof(s_cp_prompt_commands) / sizeof(s_cp_prompt_commands[0]));
}
