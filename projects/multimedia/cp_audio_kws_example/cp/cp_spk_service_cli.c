#include <stdint.h>
#include <stdbool.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "cli.h"
#include "bk_private/bk_cli.h"

#include "spk_service.h"
#include "audio_play.h"
#include "hint_prompt_clips.h"
#include "cp_audio_res.h"

/*
 * spk_service multi-source tests, built to mirror the headset topology on AP:
 * spk_service is the single speaker/DAC owner and every playback path is an
 * audio_play instance in external-sink mode whose pcm_sink hands frames to
 * spk_service_write_ex(). See a2dp_sink_audio.c (SRC_A2DP), hfp_hf_audio.c
 * (SRC_CALL) and hint_service.c (SRC_HINT) for the originals.
 *
 * Each of the three DAC sources gets its own context and worker task, so all
 * three can run at once and be hardware-mixed by the DAC. 'cp_spk mix' starts
 * three concurrent decode pipelines, which is the worst-case memory footprint
 * a headset ever reaches (music + call + prompt overlapping).
 *
 *   tone   : attach <src> + spk_service_write_ex        (raw PCM, no decoder)
 *   decode : audio_play(decoder + pcm_sink) -> spk_service_write_ex(<src>)
 */

#define CP_SPK_BITS                 (16)
#define CP_SPK_FRAME_MS             (20)
#define CP_SPK_TASK_PRI             (BEKEN_DEFAULT_WORKER_PRIORITY)
#define CP_SPK_TASK_STACK           (4096)
#define CP_SPK_WRITE_TIMEOUT_MS     (100)
#define CP_SPK_DRAIN_IDLE_MS        (300)
#define CP_SPK_DEFAULT_RATE         (16000u)
#define CP_SPK_MAX_MS               (60000u)
#define CP_SPK_STOP_POLL_MS         (10)

/* One period of a 16-point sine; at rate R this yields an R/16 Hz tone. */
static const int16_t s_sine16[16] = {
	0,  12539,  23170,  30273,  32767,  30273,  23170,  12539,
	0, -12539, -23170, -30273, -32767, -30273, -23170, -12539,
};

typedef struct {
	spk_service_src_t   src;
	uint32_t            rate;
	uint8_t             chans;
	uint32_t            duration_ms;
	const char         *enc;
	uint32_t            enc_len;
	audio_play_decoder_t decoder;

	audio_play_t       *play;
	bool                attached;
	volatile bool       stop_req;
	volatile bool       running;
	volatile uint32_t   last_pcm_tick;
	beken_thread_t      thread;
} cp_spk_ctx_t;

static cp_spk_ctx_t s_ctx[AUD_DAC_SOURCE_MAX];
static bool s_svc_held;

static const char *cp_spk_src_name(spk_service_src_t src)
{
	switch (src) {
	case SPK_SERVICE_SRC_A2DP: return "a2dp";
	case SPK_SERVICE_SRC_CALL: return "call";
	case SPK_SERVICE_SRC_HINT: return "hint";
	default: return "?";
	}
}

static void cp_spk_help(void)
{
	os_printf("cp_spk init | deinit\r\n");
	os_printf("cp_spk <a2dp|call|hint> tone [ms] [rate]  (raw PCM into the source FIFO)\r\n");
	os_printf("cp_spk <a2dp|call|hint> mp3 | wav         (decode path, = BT music/call path)\r\n");
	os_printf("cp_spk mix [ms]                           (all 3 sources at once, peak memory)\r\n");
	os_printf("cp_spk stop [a2dp|call|hint]              (no arg = stop all)\r\n");
	os_printf("cp_spk volume <db> | mute <0|1> | srcmute <a2dp|call|hint> <0|1>\r\n");
	os_printf("cp_spk info\r\n");
}

static uint32_t cp_spk_frame_bytes(uint32_t rate, uint8_t chans)
{
	return rate * chans * (CP_SPK_BITS / 8) * CP_SPK_FRAME_MS / 1000u;
}

static uint32_t cp_spk_running_count(void)
{
	uint32_t n = 0;

	for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++) {
		if (s_ctx[i].running) {
			n++;
		}
	}
	return n;
}

/* Push one frame through back-pressure so nothing is dropped: write_ex returns
 * 0 when the source port stays full, which paces us to the DAC drain rate. */
static bool cp_spk_write_all(cp_spk_ctx_t *ctx, const void *buf, uint32_t len)
{
	int off = 0;

	while (off < (int)len && !ctx->stop_req) {
		int w = spk_service_write_ex(ctx->src, (const uint8_t *)buf + off,
									 len - (uint32_t)off, CP_SPK_WRITE_TIMEOUT_MS);
		if (w < 0) {
			os_printf("cp_spk %s write failed: %d\r\n", cp_spk_src_name(ctx->src), w);
			return false;
		}
		off += w;
	}
	return true;
}

static void cp_spk_job_done(cp_spk_ctx_t *ctx)
{
	if (ctx->play) {
		audio_play_close(ctx->play);
		audio_play_destroy(ctx->play);
		ctx->play = NULL;
	}
	if (ctx->attached) {
		spk_service_detach(ctx->src);
		ctx->attached = false;
	}
	ctx->thread = NULL;
	ctx->running = false;
	rtos_delete_thread(NULL);
}

/* Feed synthesised PCM straight into the source FIFO, no decoder involved. */
static void cp_spk_tone_task(void *arg)
{
	cp_spk_ctx_t *ctx = (cp_spk_ctx_t *)arg;
	uint32_t frame_bytes = cp_spk_frame_bytes(ctx->rate, ctx->chans);
	uint32_t frames = ctx->duration_ms / CP_SPK_FRAME_MS;
	uint32_t phase = 0;
	uint32_t i;
	int16_t *frame;
	spk_source_cfg_t scfg = {
		.src           = ctx->src,
		.nChans        = ctx->chans,
		.sampRate      = ctx->rate,
		.bitsPerSample = CP_SPK_BITS,
		.frame_size    = frame_bytes,
		.volume        = 0.0f,
	};

	frame = (int16_t *)os_malloc(frame_bytes);
	if (!frame) {
		os_printf("cp_spk tone malloc %u failed\r\n", frame_bytes);
		cp_spk_job_done(ctx);
		return;
	}

	if (BK_OK != spk_service_attach(&scfg)) {
		os_printf("cp_spk attach %s failed\r\n", cp_spk_src_name(ctx->src));
		os_free(frame);
		cp_spk_job_done(ctx);
		return;
	}
	ctx->attached = true;
	os_printf("cp_spk %s tone start: %u ms, %u Hz\r\n",
			  cp_spk_src_name(ctx->src), ctx->duration_ms, ctx->rate / 16u);

	for (i = 0; i < frames && !ctx->stop_req; i++) {
		uint32_t samples = frame_bytes / sizeof(int16_t);
		uint32_t s;

		for (s = 0; s < samples; s += ctx->chans) {
			for (uint8_t c = 0; c < ctx->chans; c++) {
				frame[s + c] = s_sine16[phase & 0x0F];
			}
			phase++;
		}

		if (!cp_spk_write_all(ctx, frame, frame_bytes)) {
			break;
		}
	}

	os_free(frame);
	os_printf("cp_spk %s tone done: %u/%u frames\r\n", cp_spk_src_name(ctx->src), i, frames);
	cp_spk_job_done(ctx);
}

/* Decoded PCM lands here; attach lazily once the decoder reports its format,
 * exactly like a2dp_sink_pcm_to_spk() / hfp_hf_pcm_to_spk() do on AP. */
static int cp_spk_pcm_sink(void *user, void *pcm, uint32_t len)
{
	cp_spk_ctx_t *ctx = (cp_spk_ctx_t *)user;

	if (!ctx || ctx->stop_req) {
		return (int)len;
	}

	if (!ctx->attached) {
		uint32_t rate = 0;
		uint8_t ch = 0, bits = 0;
		spk_source_cfg_t scfg;

		if (audio_play_get_pcm_info(ctx->play, &rate, &ch, &bits) != BK_OK) {
			return (int)len;
		}
		if (ch != 1 && ch != 2) {
			ch = 1;
		}

		scfg.src           = ctx->src;
		scfg.nChans        = ch;
		scfg.sampRate      = rate;
		scfg.bitsPerSample = CP_SPK_BITS;
		scfg.frame_size    = cp_spk_frame_bytes(rate, ch);
		scfg.volume        = 0.0f;

		if (spk_service_attach(&scfg) != BK_OK) {
			os_printf("cp_spk attach %s failed\r\n", cp_spk_src_name(ctx->src));
			ctx->stop_req = true;
			return (int)len;
		}
		ctx->attached = true;
		os_printf("cp_spk %s attached: %u Hz %u ch\r\n", cp_spk_src_name(ctx->src), rate, ch);
	}

	ctx->last_pcm_tick = rtos_get_time();
	(void)cp_spk_write_all(ctx, pcm, len);
	return (int)len;
}

/* raw -> decoder -> pump -> cp_spk_pcm_sink -> spk_service, i.e. the exact
 * topology a2dp_sink_audio.c builds for BT music. */
static void cp_spk_decode_task(void *arg)
{
	cp_spk_ctx_t *ctx = (cp_spk_ctx_t *)arg;
	audio_play_cfg_t cfg = DEFAULT_AUDIO_PLAY_CONFIG();
	uint32_t n = 0;

	cfg.decoder_type   = ctx->decoder;
	cfg.pcm_sink       = cp_spk_pcm_sink;
	cfg.pcm_sink_user  = ctx;
	cfg.frame_size     = cp_spk_frame_bytes(CP_SPK_DEFAULT_RATE, 1);
	cfg.pool_size      = cfg.frame_size * 2;

	ctx->play = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &cfg);
	if (!ctx->play) {
		os_printf("cp_spk %s decode create failed\r\n", cp_spk_src_name(ctx->src));
		cp_spk_job_done(ctx);
		return;
	}
	if (BK_OK != audio_play_open(ctx->play)) {
		os_printf("cp_spk %s decode open failed\r\n", cp_spk_src_name(ctx->src));
		cp_spk_job_done(ctx);
		return;
	}

	ctx->last_pcm_tick = rtos_get_time();
	os_printf("cp_spk %s decode start: %u bytes\r\n", cp_spk_src_name(ctx->src), ctx->enc_len);

	while (n < ctx->enc_len && !ctx->stop_req) {
		uint32_t chunk = ctx->enc_len - n;

		if (chunk > cfg.frame_size) {
			chunk = cfg.frame_size;
		}
		if (audio_play_write_data(ctx->play, (char *)(ctx->enc + n), chunk) < (int)chunk) {
			break;
		}
		n += chunk;
	}

	if (!ctx->stop_req) {
		audio_play_write_eos(ctx->play);
	}

	/* Idle window, not a hard cap: the clip keeps decoding long after the encoded
	 * bytes are fed, so only give up once the pump has handed us no PCM for a while. */
	while (!ctx->stop_req) {
		if (audio_play_pcm_ended(ctx->play)) {
			break;
		}
		if ((rtos_get_time() - ctx->last_pcm_tick) > CP_SPK_DRAIN_IDLE_MS) {
			break;
		}
		rtos_delay_milliseconds(20);
	}

	os_printf("cp_spk %s decode done: fed %u bytes\r\n", cp_spk_src_name(ctx->src), n);
	cp_spk_job_done(ctx);
}

/* spk_service is a singleton with one lifecycle owner and up to three concurrent
 * users attaching A2DP/CALL/HINT. This module is that single owner: everyone who
 * needs the service calls here, and only "cp_spk deinit" gives the speaker back.
 * Releasing the arbitration token per job would hand it back while the idle
 * linger still keeps the DAC alive. */
bk_err_t cp_spk_service_ensure(void)
{
	bool acquired = false;

	/* s_svc_held only says the speaker token is ours: the idle linger can auto-deinit
	 * the service behind our back while we still hold it. So always re-assert the
	 * service; spk_service_init() is idempotent and cancels a pending linger timer. */
	if (!s_svc_held) {
		if (BK_OK != cp_aud_spk_acquire(CP_AUD_OWNER_SPK_SERVICE)) {
			return BK_FAIL;
		}
		acquired = true;
	}

	if (BK_OK != spk_service_init()) {
		os_printf("cp_spk init failed\r\n");
		if (acquired) {
			cp_aud_spk_release(CP_AUD_OWNER_SPK_SERVICE);
		}
		return BK_FAIL;
	}

	s_svc_held = true;
	return BK_OK;
}

static bk_err_t cp_spk_start(const cp_spk_ctx_t *tmpl, const char *name, void (*entry)(void *))
{
	cp_spk_ctx_t *ctx = &s_ctx[tmpl->src];

	if (ctx->running) {
		os_printf("cp_spk %s busy, use 'cp_spk stop %s'\r\n",
				  cp_spk_src_name(ctx->src), cp_spk_src_name(ctx->src));
		return BK_FAIL;
	}

	if (BK_OK != cp_spk_service_ensure()) {
		return BK_FAIL;
	}

	os_memcpy(ctx, tmpl, sizeof(cp_spk_ctx_t));
	ctx->running = true;

	if (BK_OK != rtos_create_thread(&ctx->thread, CP_SPK_TASK_PRI, name,
									(beken_thread_function_t)entry, CP_SPK_TASK_STACK, ctx)) {
		os_printf("cp_spk %s create task failed\r\n", cp_spk_src_name(ctx->src));
		ctx->running = false;
		ctx->thread = NULL;
		return BK_FAIL;
	}

	return BK_OK;
}

static void cp_spk_stop_src(spk_service_src_t src)
{
	cp_spk_ctx_t *ctx = &s_ctx[src];

	if (!ctx->running) {
		return;
	}

	ctx->stop_req = true;
	while (ctx->running) {
		rtos_delay_milliseconds(CP_SPK_STOP_POLL_MS);
	}
	os_printf("cp_spk %s stopped\r\n", cp_spk_src_name(src));
}

static void cp_spk_stop_all(void)
{
	if (cp_spk_running_count() == 0) {
		os_printf("cp_spk not playing\r\n");
		return;
	}

	for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++) {
		s_ctx[i].stop_req = true;
	}
	for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++) {
		cp_spk_stop_src((spk_service_src_t)i);
	}
}

static bk_err_t cp_spk_play_tone(spk_service_src_t src, uint32_t duration_ms, uint32_t rate)
{
	cp_spk_ctx_t tmpl = {0};

	if (duration_ms == 0 || duration_ms > CP_SPK_MAX_MS) {
		os_printf("cp_spk invalid duration: %u\r\n", duration_ms);
		return BK_FAIL;
	}

	tmpl.src         = src;
	tmpl.rate        = rate;
	tmpl.chans       = 1;
	tmpl.duration_ms = duration_ms;

	return cp_spk_start(&tmpl, "cp_spk_tone", cp_spk_tone_task);
}

static bk_err_t cp_spk_play_decode(spk_service_src_t src, audio_play_decoder_t decoder)
{
	cp_spk_ctx_t tmpl = {0};

	tmpl.src     = src;
	tmpl.decoder = decoder;

	if (decoder == AUDIO_PLAY_DECODER_MP3) {
		tmpl.enc     = network_provision_prompt_tone_array;
		tmpl.enc_len = network_provision_prompt_tone_array_len;
	} else {
		tmpl.enc     = low_voltage_prompt_tone_array;
		tmpl.enc_len = low_voltage_prompt_tone_array_len;
	}

	return cp_spk_start(&tmpl, "cp_spk_dec", cp_spk_decode_task);
}

/* Worst case a headset reaches: music decoding, call voice and a prompt clip
 * all mixed at the same time. Run 'memshow' while this is playing. */
static void cp_spk_mix(uint32_t duration_ms)
{
	os_printf("cp_spk mix: a2dp mp3 + call tone(%u ms) + hint wav\r\n", duration_ms);

	if (BK_OK != cp_spk_play_decode(SPK_SERVICE_SRC_A2DP, AUDIO_PLAY_DECODER_MP3)) {
		return;
	}
	if (BK_OK != cp_spk_play_tone(SPK_SERVICE_SRC_CALL, duration_ms, CP_SPK_DEFAULT_RATE)) {
		return;
	}
	(void)cp_spk_play_decode(SPK_SERVICE_SRC_HINT, AUDIO_PLAY_DECODER_WAV);
}

static void cp_spk_info(void)
{
	aud_dac_a2dp_rate_policy_t policy = 0;

	os_printf("cp_spk: service %s, %u source(s) running\r\n",
			  spk_service_is_running() ? "running" : "stopped", cp_spk_running_count());
	for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++) {
		os_printf("cp_spk:   %-4s %s%s\r\n", cp_spk_src_name((spk_service_src_t)i),
				  s_ctx[i].running ? "running" : "idle",
				  s_ctx[i].attached ? " (attached)" : "");
	}
	if (BK_OK == spk_service_get_a2dp_rate_policy(&policy)) {
		os_printf("cp_spk: a2dp rate policy %d\r\n", policy);
	}
	cp_aud_res_dump();
}

static bool cp_spk_parse_src(const char *s, spk_service_src_t *src)
{
	if (os_strcmp(s, "a2dp") == 0) {
		*src = SPK_SERVICE_SRC_A2DP;
	} else if (os_strcmp(s, "call") == 0) {
		*src = SPK_SERVICE_SRC_CALL;
	} else if (os_strcmp(s, "hint") == 0) {
		*src = SPK_SERVICE_SRC_HINT;
	} else {
		return false;
	}
	return true;
}

static void cp_spk_do_init(void)
{
	if (s_svc_held && spk_service_is_running()) {
		os_printf("cp_spk already inited\r\n");
		return;
	}

	if (BK_OK == cp_spk_service_ensure()) {
		os_printf("cp_spk init ok\r\n");
	}
}

static void cp_spk_do_deinit(void)
{
	cp_spk_stop_all();

	os_printf("cp_spk deinit: ret=%d\r\n", spk_service_deinit());
	if (s_svc_held) {
		cp_aud_spk_release(CP_AUD_OWNER_SPK_SERVICE);
		s_svc_held = false;
	}
}

static void cli_cp_spk_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	spk_service_src_t src;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2 || argv[1] == NULL) {
		cp_spk_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		cp_spk_do_init();
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		cp_spk_do_deinit();
	} else if (os_strcmp(argv[1], "mix") == 0) {
		uint32_t duration_ms = 5000;

		if (argc > 2 && argv[2]) {
			duration_ms = os_strtoul(argv[2], NULL, 10);
		}
		cp_spk_mix(duration_ms);
	} else if (cp_spk_parse_src(argv[1], &src)) {
		if (argc < 3 || !argv[2]) {
			cp_spk_help();
			return;
		}

		if (os_strcmp(argv[2], "tone") == 0) {
			uint32_t duration_ms = 3000;
			uint32_t rate = CP_SPK_DEFAULT_RATE;

			if (argc > 3 && argv[3]) {
				duration_ms = os_strtoul(argv[3], NULL, 10);
			}
			if (argc > 4 && argv[4]) {
				rate = os_strtoul(argv[4], NULL, 10);
			}
			(void)cp_spk_play_tone(src, duration_ms, rate);
		} else if (os_strcmp(argv[2], "mp3") == 0) {
			(void)cp_spk_play_decode(src, AUDIO_PLAY_DECODER_MP3);
		} else if (os_strcmp(argv[2], "wav") == 0) {
			(void)cp_spk_play_decode(src, AUDIO_PLAY_DECODER_WAV);
		} else {
			cp_spk_help();
		}
	} else if (os_strcmp(argv[1], "stop") == 0) {
		if (argc > 2 && argv[2] && cp_spk_parse_src(argv[2], &src)) {
			cp_spk_stop_src(src);
		} else {
			cp_spk_stop_all();
		}
	} else if (os_strcmp(argv[1], "volume") == 0) {
		if (argc < 3 || !argv[2]) {
			os_printf("cp_spk volume <db>\r\n");
			return;
		}
		os_printf("cp_spk volume %s dB: ret=%d\r\n", argv[2],
				  spk_service_set_volume((float)(int)os_strtoul(argv[2], NULL, 10)));
	} else if (os_strcmp(argv[1], "mute") == 0) {
		if (argc < 3 || !argv[2]) {
			os_printf("cp_spk mute <0|1>\r\n");
			return;
		}
		os_printf("cp_spk mute %s: ret=%d\r\n", argv[2],
				  spk_service_set_mute((uint8_t)os_strtoul(argv[2], NULL, 10)));
	} else if (os_strcmp(argv[1], "srcmute") == 0) {
		if (argc < 4 || !argv[2] || !argv[3] || !cp_spk_parse_src(argv[2], &src)) {
			os_printf("cp_spk srcmute <a2dp|call|hint> <0|1>\r\n");
			return;
		}
		os_printf("cp_spk srcmute %s %s: ret=%d\r\n", argv[2], argv[3],
				  spk_service_set_src_mute(src, (uint8_t)os_strtoul(argv[3], NULL, 10)));
	} else if (os_strcmp(argv[1], "info") == 0) {
		cp_spk_info();
	} else {
		cp_spk_help();
	}
}

static const struct cli_command s_cp_spk_commands[] = {
	{"cp_spk", "cp_spk {init|deinit|a2dp|call|hint|mix|stop|volume|mute|srcmute|info}", cli_cp_spk_cmd},
};

int cli_cp_spk_init(void)
{
	return cli_register_commands(s_cp_spk_commands,
								 sizeof(s_cp_spk_commands) / sizeof(s_cp_spk_commands[0]));
}
