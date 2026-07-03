#include "le_audio_protocol_demo.h"
#include "le_audio_user_config.h"

#include <components/bluetooth/bk_assigned_numbers.h>
#include <components/bluetooth/bk_dm_bap.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/os.h>
#include <modules/lc3_codec.h>

#define TAG "lea_broadcast"
#define LE_AUDIO_BROADCAST_ID 0x725900
#define LE_AUDIO_PRESENTATION_DELAY_US 40000
#define LE_AUDIO_FEED_PRIORITY 5
#define LE_AUDIO_ENCODE_PRIORITY 4
#define LE_AUDIO_FEED_STACK 6144
#define LE_AUDIO_ENCODE_STACK 6144
#define LE_AUDIO_LC3_WORK_BUFFER_SIZE 9064
#define LE_AUDIO_FRAME_QUEUE_COUNT 3
#define LE_AUDIO_SEND_FAIL_LIMIT 10

static uint8_t s_broadcast_session;
static uint8_t s_broadcast_sep;
static uint16_t s_bis_handle[BK_GAP_MAX_BIS];
static volatile uint8_t s_broadcast_start_pending;

/* Source-side audio feeder. The PCM source is intentionally a locally generated
 * test tone (no microphone / file): it is encoded with LC3 and pushed into the
 * BIS ISO data path, so the full source data path (PCM -> LC3 -> ISO send) can
 * be exercised end-to-end without external audio hardware. */
static lc3_enc_info_t s_enc_info;
static uint8_t *s_enc_mem;
static int16_t *s_pcm_in;
static beken_queue_t s_frame_queue;
static beken_thread_t s_encode_thread;
static beken_thread_t s_send_thread;
static beken_semaphore_t s_encode_exit_sem;
static beken_semaphore_t s_send_exit_sem;
static volatile uint8_t s_feed_run;
static uint16_t s_feed_handle;
static uint32_t s_tone_phase;

typedef struct
{
	uint16_t seq;
	uint16_t len;
	uint8_t data[CONFIG_LE_AUDIO_FRAME_BYTES];
} le_audio_broadcast_frame_t;

static void le_audio_fill_fake_pcm(int16_t *pcm, int samples)
{
	/* Integer triangle wave (~480 Hz at 48 kHz), no libm dependency. */
	const uint32_t period = 100;
	int i;

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
}

static void le_audio_source_feed_deinit(void)
{
	if (s_enc_mem)
	{
		os_free(s_enc_mem);
		s_enc_mem = NULL;
	}

	if (s_pcm_in)
	{
		os_free(s_pcm_in);
		s_pcm_in = NULL;
	}

	if (s_frame_queue)
	{
		rtos_deinit_queue(&s_frame_queue);
		s_frame_queue = NULL;
	}

	if (s_encode_exit_sem)
	{
		rtos_deinit_semaphore(&s_encode_exit_sem);
		s_encode_exit_sem = NULL;
	}

	if (s_send_exit_sem)
	{
		rtos_deinit_semaphore(&s_send_exit_sem);
		s_send_exit_sem = NULL;
	}

	os_memset(&s_enc_info, 0, sizeof(s_enc_info));
}

static void le_audio_source_wait_thread_exit(beken_thread_t *thread, beken_semaphore_t *sem, uint32_t timeout_ms)
{
	if (!thread || !(*thread))
	{
		return;
	}

	if (sem && *sem)
	{
		rtos_get_semaphore(sem, timeout_ms);
	}
	else
	{
		while (*thread)
		{
			rtos_delay_milliseconds(5);
		}
	}
}

static int le_audio_source_feed_init(void)
{
	int dt = CONFIG_LE_AUDIO_FRAME_US;
	int sr = CONFIG_LE_AUDIO_SAMPLE_RATE;
	int frame_samples;

	frame_samples = (sr * dt) / 1000000;
	if (frame_samples <= 0)
	{
		BK_LOGE(TAG, "bad lc3 params dt=%d sr=%d\n", dt, sr);
		return -1;
	}

	os_memset(&s_enc_info, 0, sizeof(s_enc_info));
	s_enc_info.channel = 1;
	s_enc_info.frame_us = dt;
	s_enc_info.enc_srate_hz = sr;
	s_enc_info.pcm_sbytes = sizeof(int16_t);
	/* Mono bitrate that resolves back to CONFIG_LE_AUDIO_FRAME_BYTES per frame. */
	s_enc_info.bitrate = CONFIG_LE_AUDIO_FRAME_BYTES * 8 * (1000000 / dt);

	s_enc_mem = (uint8_t *)os_malloc(LE_AUDIO_LC3_WORK_BUFFER_SIZE);
	s_pcm_in = (int16_t *)os_malloc(frame_samples * sizeof(int16_t));

	if (!s_enc_mem || !s_pcm_in)
	{
		BK_LOGE(TAG, "encoder alloc failed mem=%p pcm=%p\n", s_enc_mem, s_pcm_in);
		le_audio_source_feed_deinit();
		return -1;
	}

	if (rtos_init_queue(&s_frame_queue,
	                    "lea_bis_frame_q",
	                    sizeof(le_audio_broadcast_frame_t),
	                    LE_AUDIO_FRAME_QUEUE_COUNT) != BK_OK)
	{
		BK_LOGE(TAG, "frame queue init failed\n");
		le_audio_source_feed_deinit();
		return -1;
	}

	if (rtos_init_semaphore_ex(&s_encode_exit_sem, 1, 0) != BK_OK)
	{
		BK_LOGE(TAG, "encode exit sem init failed\n");
		le_audio_source_feed_deinit();
		return -1;
	}

	if (rtos_init_semaphore_ex(&s_send_exit_sem, 1, 0) != BK_OK)
	{
		BK_LOGE(TAG, "send exit sem init failed\n");
		le_audio_source_feed_deinit();
		return -1;
	}

	lc3_encoder_init(&s_enc_info, s_enc_mem);
	return 0;
}

static void le_audio_source_encode_thread(void *arg)
{
	uint16_t seq = 0;

	(void)arg;

	while (s_feed_run)
	{
		le_audio_broadcast_frame_t frame;

		os_memset(&frame, 0, sizeof(frame));
		le_audio_fill_fake_pcm(s_pcm_in, s_enc_info.frame_samples);
		lc3_encoder_proc(&s_enc_info, (uint8_t *)s_pcm_in, frame.data);

		frame.seq = seq;
		frame.len = (uint16_t)s_enc_info.frame_bytes;
		if (rtos_push_to_queue(&s_frame_queue, &frame, 20) == BK_OK)
		{
			seq++;
		}
	}

	BK_LOGI(TAG, "broadcast encode exit seq=%u\n", seq);
	if (s_encode_exit_sem)
	{
		rtos_set_semaphore(&s_encode_exit_sem);
	}
	s_encode_thread = NULL;
	rtos_delete_thread(NULL);
}

static void le_audio_source_send_thread(void *arg)
{
	uint32_t delay_ms = CONFIG_LE_AUDIO_FRAME_US / 1000;
	uint32_t next_send_ms;
	uint32_t send_fail_count = 0;
	uint32_t underrun_count = 0;
	uint32_t late_count = 0;

	if (delay_ms == 0)
	{
		delay_ms = 10;
	}

	(void)arg;

	BK_LOGI(TAG, "broadcast feed start handle=0x%04x\n", s_feed_handle);
	rtos_delay_milliseconds(delay_ms * (LE_AUDIO_FRAME_QUEUE_COUNT - 1));
	next_send_ms = rtos_get_time();

	while (s_feed_run)
	{
		bk_err_t ret;
		le_audio_broadcast_frame_t frame;

		if (rtos_pop_from_queue(&s_frame_queue, &frame, delay_ms) != BK_OK)
		{
			underrun_count++;
			if ((underrun_count == 1U) || ((underrun_count % 100U) == 0U))
			{
				BK_LOGW(TAG, "broadcast frame queue underrun #%lu\n", underrun_count);
			}
			continue;
		}

		if (!s_feed_run)
		{
			break;
		}

		ret = bk_dm_bap_broadcast_data_send(s_feed_handle, 0, 0, frame.seq, frame.data, frame.len);
		if (ret != BK_OK)
		{
			send_fail_count++;
			if ((send_fail_count == 1U) || (send_fail_count >= LE_AUDIO_SEND_FAIL_LIMIT))
			{
				BK_LOGW(TAG, "broadcast send failed ret=0x%x count=%lu handle=0x%04x seq=%u\n",
				        ret, send_fail_count, s_feed_handle, frame.seq);
			}

			if (send_fail_count >= LE_AUDIO_SEND_FAIL_LIMIT)
			{
				BK_LOGW(TAG, "stop broadcast feed after consecutive send failures\n");
				s_feed_run = 0;
				break;
			}
		}
		else
		{
			send_fail_count = 0;
		}

		next_send_ms += delay_ms;
		{
			uint32_t now_ms = rtos_get_time();

			if ((int32_t)(next_send_ms - now_ms) > 0)
			{
				rtos_delay_milliseconds(next_send_ms - now_ms);
			}
			else
			{
				late_count++;
				if ((late_count == 1U) || ((late_count % 100U) == 0U))
				{
					BK_LOGW(TAG, "broadcast send late #%lu lag=%ldms\n",
					        late_count, (int32_t)(now_ms - next_send_ms));
				}
				next_send_ms = now_ms;
			}
		}
	}

	le_audio_source_wait_thread_exit(&s_encode_thread, &s_encode_exit_sem, 1000);
	BK_LOGI(TAG, "broadcast feed exit\n");
	if (s_send_exit_sem)
	{
		rtos_set_semaphore(&s_send_exit_sem);
	}
	s_send_thread = NULL;
	rtos_delete_thread(NULL);
}

static void le_audio_source_feed_start(uint16_t handle)
{
	if (s_feed_run)
	{
		return;
	}

	le_audio_source_wait_thread_exit(&s_send_thread, &s_send_exit_sem, 2000);
	le_audio_source_wait_thread_exit(&s_encode_thread, &s_encode_exit_sem, 1000);
	if (s_enc_mem || s_pcm_in || s_frame_queue || s_encode_exit_sem || s_send_exit_sem)
	{
		le_audio_source_feed_deinit();
	}

	if (le_audio_source_feed_init() != 0)
	{
		return;
	}

	s_feed_handle = handle;
	s_feed_run = 1;

	if (rtos_create_thread(&s_encode_thread, LE_AUDIO_ENCODE_PRIORITY, "lea_bis_enc",
	                       le_audio_source_encode_thread, LE_AUDIO_ENCODE_STACK, NULL) != 0)
	{
		BK_LOGE(TAG, "create broadcast encode thread failed\n");
		s_feed_run = 0;
		le_audio_source_feed_deinit();
		return;
	}

	if (rtos_create_thread(&s_send_thread, LE_AUDIO_FEED_PRIORITY, "lea_bis_send",
	                       le_audio_source_send_thread, LE_AUDIO_FEED_STACK, NULL) != 0)
	{
		BK_LOGE(TAG, "create broadcast send thread failed\n");
		s_feed_run = 0;
		le_audio_source_wait_thread_exit(&s_encode_thread, &s_encode_exit_sem, 1000);
		le_audio_source_feed_deinit();
	}
}

static void le_audio_source_feed_stop(void)
{
	if (!s_feed_run && !s_encode_thread && !s_send_thread &&
	    !s_enc_mem && !s_pcm_in && !s_frame_queue && !s_encode_exit_sem && !s_send_exit_sem)
	{
		return;
	}

	s_feed_run = 0;

	le_audio_source_wait_thread_exit(&s_send_thread, &s_send_exit_sem, 2000);
	le_audio_source_wait_thread_exit(&s_encode_thread, &s_encode_exit_sem, 1000);
	le_audio_source_feed_deinit();
}

static int le_audio_broadcast_create_big(void)
{
	int ret;

	ret = bk_dm_bap_broadcast_start(s_broadcast_session,
	                                CONFIG_LE_AUDIO_FRAME_US,
	                                CONFIG_LE_AUDIO_FRAME_BYTES,
	                                10,
	                                2,
	                                0);
	return ret;
}

static void le_audio_setup_announcement_cb(uint32_t status, void *paramters)
{
	(void)paramters;

	if (!s_broadcast_start_pending)
	{
		return;
	}

	s_broadcast_start_pending = 0;

	if (status != 0)
	{
		BK_LOGE(TAG, "setup announcement failed status=%lu\n", status);
		return;
	}

	/* setup_announcement is asynchronous. Create BIG only after EA/PA/BASE
	 * configuration completes and the BAP session reaches CONFIGURED state. */
	le_audio_broadcast_create_big();
}

static void le_audio_end_announcement_cb(uint32_t status, void *paramters)
{
	BK_LOGI(TAG, "end announcement status=%lu param=%p\n", status, paramters);
}

static void le_audio_broadcast_start_cb(bk_bap_boradcast_paramters_t *param)
{
	uint8_t i;

	if (!param)
	{
		return;
	}

	for (i = 0; i < param->num_bis && i < BK_GAP_MAX_BIS; i++)
	{
		s_bis_handle[i] = param->connection_handle[i];
	}

	/* Once the BIS handle is known, start streaming the locally generated tone. */
	if (param->error_code == 0 && param->num_bis > 0)
	{
		le_audio_source_feed_start(s_bis_handle[0]);
	}
}

static void le_audio_broadcast_suspend_cb(uint8_t handle, uint8_t reason)
{
	BK_LOGI(TAG, "broadcast suspend handle=%u reason=%u\n", handle, reason);
	le_audio_source_feed_stop();
}

int le_audio_broadcast_init(void)
{
	static const bk_bap_source_callbacks_t source_cbs =
	{
		.setup_announcement_cb = le_audio_setup_announcement_cb,
		.end_announcement_cb = le_audio_end_announcement_cb,
		.broadcast_start_cb = le_audio_broadcast_start_cb,
		.broadcast_suspend_cb = le_audio_broadcast_suspend_cb,
	};
	int ret;

	ret = bk_dm_bap_source_register(&source_cbs);
	BK_LOGI(TAG, "bk_dm_bap_source_register ret=%d\n", ret);

	return ret;
}

int le_audio_demo_broadcast_start(void)
{
	bk_bap_codec_info_t codec;
	bk_bap_medadata_t meta;
	bk_bap_codec_ie_t stream;
	bk_bap_lc3_codec_specific_conf_t lc3_conf;
	int ret;

	if (le_audio_demo_get_role() != LE_AUDIO_DEMO_ROLE_SOURCE)
	{
		BK_LOGW(TAG, "broadcast start requested while not in source role\n");
		return -1;
	}

	os_memset(&codec, 0, sizeof(codec));
	os_memset(&meta, 0, sizeof(meta));
	os_memset(&stream, 0, sizeof(stream));
	os_memset(&lc3_conf, 0, sizeof(lc3_conf));

	lc3_conf.sf = BK_BT_CODEC_CFG_FREQ_48KHZ;
	lc3_conf.fd = BK_BT_CODEC_FRAME_DURATION_10000US;
	lc3_conf.aca = BK_BT_AUDIO_LOCATION_FRONT_LEFT;
	lc3_conf.opcf = CONFIG_LE_AUDIO_FRAME_BYTES;
	lc3_conf.mcfpSDU = 1;

	codec.coding_format = BK_BT_CODEC_ID_LC3;
	codec.company_id = 0;
	codec.vendor_codec_id = 0;
	bk_dm_bap_create_codec_spec_conf_ltv(&lc3_conf, codec.ie, &codec.ie_len);

	bk_dm_bap_create_codec_spec_conf_ltv(&lc3_conf, stream.value, &stream.length);
	bk_dm_bap_create_metadata_ltv(meta.data, &meta.length);

	ret = bk_dm_bap_broadcast_alloc_session(&s_broadcast_session);
	if (ret)
	{
		BK_LOGE(TAG, "alloc session failed %d\n", ret);
		return ret;
	}

	ret = bk_dm_bap_broadcast_configure_session(s_broadcast_session, 0x02, 0x00, NULL);
	if (ret)
	{
		BK_LOGE(TAG, "configure session failed %d\n", ret);
		return ret;
	}

	ret = bk_dm_bap_broadcast_sep_register(s_broadcast_session, &codec, &meta, 1, &stream, &s_broadcast_sep);
	if (ret)
	{
		BK_LOGE(TAG, "sep register failed %d\n", ret);
		return ret;
	}

	s_broadcast_start_pending = 1;
	ret = bk_dm_bap_setup_announcement(s_broadcast_session, LE_AUDIO_BROADCAST_ID, 0, LE_AUDIO_PRESENTATION_DELAY_US);
	if (ret)
	{
		s_broadcast_start_pending = 0;
		BK_LOGE(TAG, "setup announcement failed %d\n", ret);
		return ret;
	}

	BK_LOGI(TAG, "broadcast source setup pending session=%u sep=%u\n", s_broadcast_session, s_broadcast_sep);
	return 0;
}

int le_audio_demo_broadcast_stop(void)
{
	int ret;

	s_broadcast_start_pending = 0;
	le_audio_source_feed_stop();

	ret = bk_dm_bap_broadcast_suspend(s_broadcast_session);
	bk_dm_bap_end_announcement(s_broadcast_session);
	bk_dm_bap_broadcast_free_session(s_broadcast_session);
	BK_LOGI(TAG, "broadcast source stop ret=%d\n", ret);
	return ret;
}
