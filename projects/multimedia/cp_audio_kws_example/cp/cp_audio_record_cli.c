#include <stdint.h>
#include <stdbool.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "cli.h"
#include "bk_private/bk_cli.h"

#include "audio_record.h"
#include "spk_service.h"
#include "cp_audio_res.h"

#define CP_AUD_REC_DEFAULT_RATE     16000u
#define CP_AUD_REC_DEFAULT_CHANS    1u
#define CP_AUD_REC_FRAME_MS         20u
#define CP_AUD_REC_DEFAULT_GAIN_DB  16
#define CP_AUD_REC_MAX_MS           30000u
#define CP_AUD_REC_LOOP_VOL_DB      (-7)
#define CP_AUD_REC_LOOP_WRITE_MS    100u

static audio_record_t *s_record;
static uint32_t s_record_rate;
static uint8_t s_record_chans;

static void cp_audio_record_help(void)
{
	os_printf("cp_audio_record open [rate] [chans] [gain_db]  (default 16000 1 16)\r\n");
	os_printf("cp_audio_record read [ms]                      (report bytes + peak, default 1000ms)\r\n");
	os_printf("cp_audio_record gain <db>\r\n");
	os_printf("cp_audio_record pause|resume\r\n");
	os_printf("cp_audio_record close\r\n");
	os_printf("cp_audio_record info\r\n");
	os_printf("cp_audio_record loop [ms] [rate]               (mic -> spk_service CALL, default 3000ms 16000)\r\n");
	os_printf("note: loop needs its own speaker, close cp_audio_play first\r\n");
}

static uint32_t cp_audio_record_frame_bytes(uint32_t rate, uint8_t chans)
{
	return rate * chans * sizeof(int16_t) * CP_AUD_REC_FRAME_MS / 1000u;
}

static bk_err_t cp_audio_record_do_open(uint32_t rate, uint8_t chans, int gain_db)
{
	audio_record_cfg_t cfg = DEFAULT_AUDIO_RECORD_CONFIG();

	if (s_record) {
		os_printf("cp_audio_record already open (%u Hz %u ch)\r\n", s_record_rate, s_record_chans);
		return BK_OK;
	}

	if (BK_OK != cp_aud_mic_acquire(CP_AUD_OWNER_AUDIO_RECORD)) {
		return BK_FAIL;
	}

	cfg.sampRate     = rate;
	cfg.nChans       = chans;
	cfg.adc_gain     = (float)gain_db;
	cfg.encoder_type = AUDIO_RECORD_ENCODER_PCM;
	cfg.frame_size   = cp_audio_record_frame_bytes(rate, chans);
	cfg.pool_size    = cfg.frame_size * 2;

	s_record = audio_record_create(AUDIO_RECORD_ONBOARD_MIC, &cfg);
	if (!s_record) {
		os_printf("cp_audio_record create failed\r\n");
		cp_aud_mic_release(CP_AUD_OWNER_AUDIO_RECORD);
		return BK_FAIL;
	}

	if (BK_OK != audio_record_open(s_record)) {
		os_printf("cp_audio_record open failed\r\n");
		(void)audio_record_destroy(s_record);
		s_record = NULL;
		cp_aud_mic_release(CP_AUD_OWNER_AUDIO_RECORD);
		return BK_FAIL;
	}

	s_record_rate  = rate;
	s_record_chans = chans;
	os_printf("cp_audio_record open ok: %u Hz %u ch, frame %u B, gain %d dB\r\n",
			  rate, chans, cfg.frame_size, gain_db);
	return BK_OK;
}

static bk_err_t cp_audio_record_do_close(void)
{
	if (!s_record) {
		os_printf("cp_audio_record not open\r\n");
		return BK_FAIL;
	}

	(void)audio_record_close(s_record);
	(void)audio_record_destroy(s_record);
	s_record = NULL;
	s_record_rate = 0;
	s_record_chans = 0;
	cp_aud_mic_release(CP_AUD_OWNER_AUDIO_RECORD);
	os_printf("cp_audio_record closed\r\n");
	return BK_OK;
}

static bk_err_t cp_audio_record_do_read(uint32_t duration_ms)
{
	uint32_t frame_bytes;
	uint32_t frames;
	uint32_t i;
	uint32_t total = 0;
	int16_t peak = 0;
	int16_t *frame;
	bk_err_t ret = BK_OK;

	if (!s_record && BK_OK != cp_audio_record_do_open(CP_AUD_REC_DEFAULT_RATE,
													  CP_AUD_REC_DEFAULT_CHANS,
													  CP_AUD_REC_DEFAULT_GAIN_DB)) {
		return BK_FAIL;
	}

	if (duration_ms == 0 || duration_ms > CP_AUD_REC_MAX_MS) {
		os_printf("cp_audio_record invalid duration: %u\r\n", duration_ms);
		return BK_FAIL;
	}

	frame_bytes = cp_audio_record_frame_bytes(s_record_rate, s_record_chans);
	frames = duration_ms / CP_AUD_REC_FRAME_MS;
	frame = (int16_t *)os_malloc(frame_bytes);
	if (!frame) {
		os_printf("cp_audio_record read malloc %u failed\r\n", frame_bytes);
		return BK_FAIL;
	}

	os_printf("cp_audio_record read start: %u ms\r\n", duration_ms);

	for (i = 0; i < frames; i++) {
		uint32_t s;

		int rd = audio_record_read_data(s_record, (char *)frame, frame_bytes);

		if (rd < (int)frame_bytes) {
			os_printf("cp_audio_record read short at frame %u: %d/%u\r\n", i, rd, frame_bytes);
			ret = BK_FAIL;
			break;
		}
		total += frame_bytes;

		for (s = 0; s < frame_bytes / sizeof(int16_t); s++) {
			int16_t v = frame[s] < 0 ? -frame[s] : frame[s];

			if (v > peak) {
				peak = v;
			}
		}
	}

	os_free(frame);
	os_printf("cp_audio_record read done: %u bytes, peak %d, ret=%d\r\n", total, peak, ret);
	return ret;
}

/* Mic -> speaker, shaped like the HFP call path on AP: the mic uplink is an
 * audio_record instance and the downlink goes through spk_service's CALL source
 * rather than a second speaker of its own (see hfp_hf_audio.c). Routing through
 * spk_service is what lets this coexist with 'cp_spk a2dp'/'hint'. */
static bk_err_t cp_audio_record_do_loop(uint32_t duration_ms, uint32_t rate)
{
	audio_record_cfg_t rec_cfg = DEFAULT_AUDIO_RECORD_CONFIG();
	audio_record_t *record = NULL;
	uint32_t frame_bytes = cp_audio_record_frame_bytes(rate, 1);
	uint32_t frames = duration_ms / CP_AUD_REC_FRAME_MS;
	uint32_t i = 0;
	char *frame = NULL;
	bool mic_held = false;
	bool attached = false;
	bk_err_t ret = BK_FAIL;
	spk_source_cfg_t scfg = {
		.src           = SPK_SERVICE_SRC_CALL,
		.nChans        = 1,
		.sampRate      = rate,
		.bitsPerSample = 16,
		.frame_size    = frame_bytes,
		.volume        = (float)CP_AUD_REC_LOOP_VOL_DB,
	};

	if (duration_ms == 0 || duration_ms > CP_AUD_REC_MAX_MS) {
		os_printf("cp_audio_record invalid duration: %u\r\n", duration_ms);
		return BK_FAIL;
	}

	if (s_record) {
		os_printf("cp_audio_record loop needs the mic, close it first\r\n");
		return BK_FAIL;
	}

	if (BK_OK != cp_aud_mic_acquire(CP_AUD_OWNER_AUDIO_RECORD)) {
		return BK_FAIL;
	}
	mic_held = true;

	/* cp_spk owns the spk_service lifecycle and keeps the speaker token until
	 * "cp_spk deinit", so this path must not release it on the way out. */
	if (BK_OK != cp_spk_service_ensure()) {
		goto out;
	}

	frame = (char *)os_malloc(frame_bytes);
	if (!frame) {
		os_printf("cp_audio_record loop malloc %u failed\r\n", frame_bytes);
		goto out;
	}

	rec_cfg.sampRate     = rate;
	rec_cfg.nChans       = 1;
	rec_cfg.adc_gain     = (float)CP_AUD_REC_DEFAULT_GAIN_DB;
	rec_cfg.encoder_type = AUDIO_RECORD_ENCODER_PCM;
	rec_cfg.frame_size   = frame_bytes;
	rec_cfg.pool_size    = frame_bytes * 2;

	record = audio_record_create(AUDIO_RECORD_ONBOARD_MIC, &rec_cfg);
	if (!record || BK_OK != audio_record_open(record)) {
		os_printf("cp_audio_record loop mic open failed\r\n");
		goto out;
	}

	if (BK_OK != spk_service_attach(&scfg)) {
		os_printf("cp_audio_record loop attach call failed\r\n");
		goto out;
	}
	attached = true;

	os_printf("cp_audio_record loop start: %u ms, %u Hz (mic -> spk_service CALL)\r\n",
			  duration_ms, rate);

	ret = BK_OK;
	for (i = 0; i < frames; i++) {
		int rd = audio_record_read_data(record, frame, frame_bytes);
		int off = 0;

		if (rd < (int)frame_bytes) {
			os_printf("cp_audio_record loop read short at %u: %d/%u\r\n", i, rd, frame_bytes);
			ret = BK_FAIL;
			break;
		}

		while (off < rd) {
			int wr = spk_service_write_ex(SPK_SERVICE_SRC_CALL, frame + off,
										 (uint32_t)(rd - off), CP_AUD_REC_LOOP_WRITE_MS);
			if (wr < 0) {
				os_printf("cp_audio_record loop write failed at %u: %d\r\n", i, wr);
				ret = BK_FAIL;
				break;
			}
			off += wr;
		}
		if (ret != BK_OK) {
			break;
		}
	}

	os_printf("cp_audio_record loop done: %u/%u frames, ret=%d\r\n", i, frames, ret);

out:
	if (attached) {
		spk_service_detach(SPK_SERVICE_SRC_CALL);
	}
	if (record) {
		(void)audio_record_close(record);
		(void)audio_record_destroy(record);
	}
	if (mic_held) {
		cp_aud_mic_release(CP_AUD_OWNER_AUDIO_RECORD);
	}
	os_free(frame);
	return ret;
}

static void cp_audio_record_do_info(void)
{
	if (!s_record) {
		os_printf("cp_audio_record: closed\r\n");
		return;
	}

	os_printf("cp_audio_record: open %u Hz %u ch, frame %u B\r\n",
			  s_record_rate, s_record_chans,
			  cp_audio_record_frame_bytes(s_record_rate, s_record_chans));
}

static void cli_cp_audio_record_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2 || argv[1] == NULL) {
		cp_audio_record_help();
		return;
	}

	if (os_strcmp(argv[1], "open") == 0) {
		uint32_t rate = CP_AUD_REC_DEFAULT_RATE;
		uint32_t chans = CP_AUD_REC_DEFAULT_CHANS;
		int gain_db = CP_AUD_REC_DEFAULT_GAIN_DB;

		if (argc > 2 && argv[2]) {
			rate = os_strtoul(argv[2], NULL, 10);
		}
		if (argc > 3 && argv[3]) {
			chans = os_strtoul(argv[3], NULL, 10);
		}
		if (argc > 4 && argv[4]) {
			gain_db = (int)os_strtoul(argv[4], NULL, 10);
		}
		if (chans < 1 || chans > 2) {
			os_printf("cp_audio_record invalid chans: %u\r\n", chans);
			return;
		}
		(void)cp_audio_record_do_open(rate, (uint8_t)chans, gain_db);
	} else if (os_strcmp(argv[1], "read") == 0) {
		uint32_t duration_ms = 1000;

		if (argc > 2 && argv[2]) {
			duration_ms = os_strtoul(argv[2], NULL, 10);
		}
		(void)cp_audio_record_do_read(duration_ms);
	} else if (os_strcmp(argv[1], "gain") == 0) {
		if (argc < 3 || !argv[2] || !s_record) {
			os_printf("cp_audio_record gain <db>, record must be open\r\n");
			return;
		}
		os_printf("cp_audio_record set gain %s dB: ret=%d\r\n", argv[2],
				  audio_record_set_adc_gain(s_record, (float)(int)os_strtoul(argv[2], NULL, 10)));
	} else if (os_strcmp(argv[1], "pause") == 0 || os_strcmp(argv[1], "resume") == 0) {
		audio_record_ctl_t ctl;

		if (!s_record) {
			os_printf("cp_audio_record not open\r\n");
			return;
		}
		ctl = (os_strcmp(argv[1], "pause") == 0) ? AUDIO_RECORD_PAUSE : AUDIO_RECORD_RESUME;
		os_printf("cp_audio_record %s: ret=%d\r\n", argv[1], audio_record_control(s_record, ctl));
	} else if (os_strcmp(argv[1], "close") == 0) {
		(void)cp_audio_record_do_close();
	} else if (os_strcmp(argv[1], "info") == 0) {
		cp_audio_record_do_info();
	} else if (os_strcmp(argv[1], "loop") == 0) {
		uint32_t duration_ms = 3000;
		uint32_t rate = CP_AUD_REC_DEFAULT_RATE;

		if (argc > 2 && argv[2]) {
			duration_ms = os_strtoul(argv[2], NULL, 10);
		}
		if (argc > 3 && argv[3]) {
			rate = os_strtoul(argv[3], NULL, 10);
		}
		(void)cp_audio_record_do_loop(duration_ms, rate);
	} else {
		cp_audio_record_help();
	}
}

static const struct cli_command s_cp_audio_record_commands[] = {
	{"cp_audio_record", "cp_audio_record {open|read|gain|pause|resume|close|info|loop}", cli_cp_audio_record_cmd},
};

int cli_cp_audio_record_init(void)
{
	return cli_register_commands(s_cp_audio_record_commands,
								 sizeof(s_cp_audio_record_commands) / sizeof(s_cp_audio_record_commands[0]));
}
