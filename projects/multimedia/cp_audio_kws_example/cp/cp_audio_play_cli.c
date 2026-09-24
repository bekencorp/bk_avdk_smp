#include <stdint.h>
#include <stdbool.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "cli.h"
#include "bk_private/bk_cli.h"

#include "audio_play.h"
#include "hint_prompt_clips.h"
#include "cp_audio_res.h"

#define CP_AUD_PLAY_DEFAULT_RATE    16000u
#define CP_AUD_PLAY_DEFAULT_CHANS   1u
#define CP_AUD_PLAY_FRAME_MS        20u
#define CP_AUD_PLAY_DEFAULT_VOL_DB  (-7)
#define CP_AUD_PLAY_MAX_MS          10000u
#define CP_AUD_PLAY_CLIP_RATE       16000u

/* One period of a 16-point sine, 16-bit signed. Played back at rate R this
 * yields an R/16 Hz tone, i.e. 1 kHz at the default 16 kHz. */
static const int16_t s_sine16[16] = {
	0,  12539,  23170,  30273,  32767,  30273,  23170,  12539,
	0, -12539, -23170, -30273, -32767, -30273, -23170, -12539,
};

static audio_play_t *s_play;
static uint32_t s_play_rate;
static uint8_t s_play_chans;

static void cp_audio_play_help(void)
{
	os_printf("cp_audio_play open [rate] [chans] [vol_db]   (default 16000 1 -7)\r\n");
	os_printf("cp_audio_play tone [ms]                      (rate/16 Hz sine, default 1000ms)\r\n");
	os_printf("cp_audio_play clip                           (built-in 16k/mono PCM prompt)\r\n");
	os_printf("cp_audio_play volume <db>\r\n");
	os_printf("cp_audio_play pause|resume|mute|unmute\r\n");
	os_printf("cp_audio_play close\r\n");
	os_printf("cp_audio_play info\r\n");
	os_printf("note: direct-DAC mode, no spk_service. Use it to isolate audio_play\r\n");
	os_printf("      from the mixer; 'cp_spk' is the path a headset actually ships.\r\n");
}

static uint32_t cp_audio_play_frame_bytes(uint32_t rate, uint8_t chans)
{
	return rate * chans * sizeof(int16_t) * CP_AUD_PLAY_FRAME_MS / 1000u;
}

static bk_err_t cp_audio_play_do_open(uint32_t rate, uint8_t chans, int vol_db)
{
	audio_play_cfg_t cfg = DEFAULT_AUDIO_PLAY_CONFIG();

	if (s_play) {
		os_printf("cp_audio_play already open (%u Hz %u ch)\r\n", s_play_rate, s_play_chans);
		return BK_OK;
	}

	if (BK_OK != cp_aud_spk_acquire(CP_AUD_OWNER_AUDIO_PLAY)) {
		return BK_FAIL;
	}

	cfg.sampRate      = rate;
	cfg.nChans        = chans;
	cfg.bitsPerSample = 16;
	cfg.volume        = (float)vol_db;
	cfg.decoder_type  = AUDIO_PLAY_DECODER_PCM;
	cfg.frame_size    = cp_audio_play_frame_bytes(rate, chans);
	cfg.pool_size     = cfg.frame_size * 2;

	s_play = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &cfg);
	if (!s_play) {
		os_printf("cp_audio_play create failed\r\n");
		cp_aud_spk_release(CP_AUD_OWNER_AUDIO_PLAY);
		return BK_FAIL;
	}

	if (BK_OK != audio_play_open(s_play)) {
		os_printf("cp_audio_play open failed\r\n");
		(void)audio_play_destroy(s_play);
		s_play = NULL;
		cp_aud_spk_release(CP_AUD_OWNER_AUDIO_PLAY);
		return BK_FAIL;
	}

	s_play_rate  = rate;
	s_play_chans = chans;
	os_printf("cp_audio_play open ok: %u Hz %u ch, frame %u B, vol %d dB\r\n",
			  rate, chans, cfg.frame_size, vol_db);
	return BK_OK;
}

static bk_err_t cp_audio_play_do_close(void)
{
	if (!s_play) {
		os_printf("cp_audio_play not open\r\n");
		return BK_FAIL;
	}

	(void)audio_play_close(s_play);
	(void)audio_play_destroy(s_play);
	s_play = NULL;
	s_play_rate = 0;
	s_play_chans = 0;
	cp_aud_spk_release(CP_AUD_OWNER_AUDIO_PLAY);
	os_printf("cp_audio_play closed\r\n");
	return BK_OK;
}

static bk_err_t cp_audio_play_do_tone(uint32_t duration_ms)
{
	uint32_t frame_bytes;
	uint32_t frames;
	uint32_t i;
	uint32_t s;
	int16_t *frame;
	uint32_t phase = 0;
	bk_err_t ret = BK_OK;

	if (!s_play && BK_OK != cp_audio_play_do_open(CP_AUD_PLAY_DEFAULT_RATE,
												  CP_AUD_PLAY_DEFAULT_CHANS,
												  CP_AUD_PLAY_DEFAULT_VOL_DB)) {
		return BK_FAIL;
	}

	if (duration_ms == 0 || duration_ms > CP_AUD_PLAY_MAX_MS) {
		os_printf("cp_audio_play invalid duration: %u\r\n", duration_ms);
		return BK_FAIL;
	}

	frame_bytes = cp_audio_play_frame_bytes(s_play_rate, s_play_chans);
	frames = duration_ms / CP_AUD_PLAY_FRAME_MS;
	frame = (int16_t *)os_malloc(frame_bytes);
	if (!frame) {
		os_printf("cp_audio_play tone malloc %u failed\r\n", frame_bytes);
		return BK_FAIL;
	}

	os_printf("cp_audio_play tone start: %u ms, %u Hz\r\n", duration_ms, s_play_rate / 16u);

	for (i = 0; i < frames; i++) {
		uint32_t samples = frame_bytes / sizeof(int16_t);

		for (s = 0; s < samples; s += s_play_chans) {
			uint8_t c;

			for (c = 0; c < s_play_chans; c++) {
				frame[s + c] = s_sine16[phase & 0x0F];
			}
			phase++;
		}

		int wr = audio_play_write_data(s_play, (char *)frame, frame_bytes);

		if (wr < (int)frame_bytes) {
			os_printf("cp_audio_play write short at frame %u: %d/%u\r\n", i, wr, frame_bytes);
			ret = BK_FAIL;
			break;
		}
	}

	os_free(frame);
	os_printf("cp_audio_play tone done: %u/%u frames, ret=%d\r\n", i, frames, ret);
	return ret;
}

static bk_err_t cp_audio_play_do_clip(void)
{
	uint32_t frame_bytes;
	uint32_t offset = 0;
	bk_err_t ret = BK_OK;

	if (!s_play && BK_OK != cp_audio_play_do_open(CP_AUD_PLAY_CLIP_RATE, 1,
												  CP_AUD_PLAY_DEFAULT_VOL_DB)) {
		return BK_FAIL;
	}

	if (s_play_rate != CP_AUD_PLAY_CLIP_RATE || s_play_chans != 1) {
		os_printf("cp_audio_play clip needs 16000 Hz mono, current %u Hz %u ch\r\n",
				  s_play_rate, s_play_chans);
		return BK_FAIL;
	}

	frame_bytes = cp_audio_play_frame_bytes(s_play_rate, s_play_chans);
	os_printf("cp_audio_play clip start: %u bytes\r\n", asr_wakeup_prompt_tone_array_len);

	while (offset < asr_wakeup_prompt_tone_array_len) {
		uint32_t chunk = asr_wakeup_prompt_tone_array_len - offset;

		if (chunk > frame_bytes) {
			chunk = frame_bytes;
		}

		int wr = audio_play_write_data(s_play, (char *)&asr_wakeup_prompt_tone_array[offset], chunk);

		if (wr < (int)chunk) {
			os_printf("cp_audio_play clip write short at %u: %d/%u\r\n", offset, wr, chunk);
			ret = BK_FAIL;
			break;
		}
		offset += chunk;
	}

	os_printf("cp_audio_play clip done: %u bytes, ret=%d\r\n", offset, ret);
	return ret;
}

static void cp_audio_play_do_info(void)
{
	if (!s_play) {
		os_printf("cp_audio_play: closed\r\n");
		return;
	}

	os_printf("cp_audio_play: open %u Hz %u ch, frame %u B\r\n",
			  s_play_rate, s_play_chans, cp_audio_play_frame_bytes(s_play_rate, s_play_chans));
}

static void cli_cp_audio_play_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2 || argv[1] == NULL) {
		cp_audio_play_help();
		return;
	}

	if (os_strcmp(argv[1], "open") == 0) {
		uint32_t rate = CP_AUD_PLAY_DEFAULT_RATE;
		uint32_t chans = CP_AUD_PLAY_DEFAULT_CHANS;
		int vol_db = CP_AUD_PLAY_DEFAULT_VOL_DB;

		if (argc > 2 && argv[2]) {
			rate = os_strtoul(argv[2], NULL, 10);
		}
		if (argc > 3 && argv[3]) {
			chans = os_strtoul(argv[3], NULL, 10);
		}
		if (argc > 4 && argv[4]) {
			vol_db = (int)os_strtoul(argv[4], NULL, 10);
		}
		if (chans < 1 || chans > 2) {
			os_printf("cp_audio_play invalid chans: %u\r\n", chans);
			return;
		}
		(void)cp_audio_play_do_open(rate, (uint8_t)chans, vol_db);
	} else if (os_strcmp(argv[1], "tone") == 0) {
		uint32_t duration_ms = 1000;

		if (argc > 2 && argv[2]) {
			duration_ms = os_strtoul(argv[2], NULL, 10);
		}
		(void)cp_audio_play_do_tone(duration_ms);
	} else if (os_strcmp(argv[1], "clip") == 0) {
		(void)cp_audio_play_do_clip();
	} else if (os_strcmp(argv[1], "volume") == 0) {
		if (argc < 3 || !argv[2] || !s_play) {
			os_printf("cp_audio_play volume <db>, play must be open\r\n");
			return;
		}
		os_printf("cp_audio_play set volume %s dB: ret=%d\r\n", argv[2],
				  audio_play_set_volume(s_play, (float)(int)os_strtoul(argv[2], NULL, 10)));
	} else if (os_strcmp(argv[1], "pause") == 0 || os_strcmp(argv[1], "resume") == 0
			   || os_strcmp(argv[1], "mute") == 0 || os_strcmp(argv[1], "unmute") == 0) {
		audio_play_ctl_t ctl;

		if (!s_play) {
			os_printf("cp_audio_play not open\r\n");
			return;
		}
		if (os_strcmp(argv[1], "pause") == 0) {
			ctl = AUDIO_PLAY_PAUSE;
		} else if (os_strcmp(argv[1], "resume") == 0) {
			ctl = AUDIO_PLAY_RESUME;
		} else if (os_strcmp(argv[1], "mute") == 0) {
			ctl = AUDIO_PLAY_MUTE;
		} else {
			ctl = AUDIO_PLAY_UNMUTE;
		}
		os_printf("cp_audio_play %s: ret=%d\r\n", argv[1], audio_play_control(s_play, ctl));
	} else if (os_strcmp(argv[1], "close") == 0) {
		(void)cp_audio_play_do_close();
	} else if (os_strcmp(argv[1], "info") == 0) {
		cp_audio_play_do_info();
	} else {
		cp_audio_play_help();
	}
}

static const struct cli_command s_cp_audio_play_commands[] = {
	{"cp_audio_play", "cp_audio_play {open|tone|clip|volume|pause|resume|mute|unmute|close|info}", cli_cp_audio_play_cmd},
};

int cli_cp_audio_play_init(void)
{
	return cli_register_commands(s_cp_audio_play_commands,
								 sizeof(s_cp_audio_play_commands) / sizeof(s_cp_audio_play_commands[0]));
}
