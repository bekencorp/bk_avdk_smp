#include "audio.h"

#include <components/bluetooth/bk_assigned_numbers.h>

#include <components/log.h>
#include <modules/lc3_codec.h>
#include <os/mem.h>
#include <os/os.h>

#define TAG "lea_tx"

#define LE_AUDIO_TX_ENCODE_PRIORITY 4
#define LE_AUDIO_TX_SEND_PRIORITY   5
#define LE_AUDIO_TX_ENCODE_STACK    6144
#define LE_AUDIO_TX_SEND_STACK      6144
#define LE_AUDIO_TX_LC3_WORK_BYTES  9064
#define LE_AUDIO_TX_FRAME_MAX_BYTES 160
#define LE_AUDIO_TX_FRAME_QUEUE     3
#define LE_AUDIO_TX_SEND_FAIL_LIMIT 10

extern int lc3_frame_samples(int dt_us, int sr_hz);

typedef struct
{
	uint16_t seq;
	uint16_t len;
	uint8_t  data[LE_AUDIO_TX_FRAME_MAX_BYTES];
} le_audio_tx_frame_t;

static const le_audio_pcm_source_t *s_source;

static lc3_enc_info_t  s_enc_info;
static uint8_t        *s_enc_mem;
static int16_t        *s_pcm_in;
static beken_queue_t   s_frame_queue;
static beken_thread_t  s_encode_thread;
static beken_thread_t  s_send_thread;
static beken_semaphore_t s_encode_exit_sem;
static beken_semaphore_t s_send_exit_sem;
static volatile uint8_t s_run;
static uint16_t        s_handle;
static le_audio_tx_send_fn s_send_fn;
static le_audio_codec_cfg_t s_cfg;

void le_audio_audio_set_source(const le_audio_pcm_source_t *src)
{
	s_source = src;
}

static void le_audio_tx_deinit(void)
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

	if (s_source && s_source->close)
	{
		s_source->close();
	}

	os_memset(&s_enc_info, 0, sizeof(s_enc_info));
}

static void le_audio_tx_wait_exit(beken_thread_t *thread, beken_semaphore_t *sem, uint32_t timeout_ms)
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

static int le_audio_tx_init(void)
{
	int dt = (int)s_cfg.frame_us;
	int sr = (int)s_cfg.sample_rate;
	int frame_samples = lc3_frame_samples(dt, sr);

	if (frame_samples <= 0)
	{
		BK_LOGE(TAG, "bad lc3 params dt=%d sr=%d\n", dt, sr);
		return -1;
	}

	if (s_cfg.frame_bytes > LE_AUDIO_TX_FRAME_MAX_BYTES)
	{
		BK_LOGE(TAG, "frame_bytes %u > max %u\n", s_cfg.frame_bytes, LE_AUDIO_TX_FRAME_MAX_BYTES);
		return -1;
	}

	os_memset(&s_enc_info, 0, sizeof(s_enc_info));
	s_enc_info.channel = 1;
	s_enc_info.frame_us = dt;
	s_enc_info.enc_srate_hz = sr;
	s_enc_info.pcm_sbytes = sizeof(int16_t);
	s_enc_info.bitrate = (uint32_t)s_cfg.frame_bytes * 8 * (1000000 / dt);

	s_enc_mem = (uint8_t *)os_malloc(LE_AUDIO_TX_LC3_WORK_BYTES);
	s_pcm_in = (int16_t *)os_malloc(frame_samples * sizeof(int16_t));
	if (!s_enc_mem || !s_pcm_in)
	{
		BK_LOGE(TAG, "encoder alloc failed mem=%p pcm=%p\n", s_enc_mem, s_pcm_in);
		le_audio_tx_deinit();
		return -1;
	}

	if (rtos_init_queue(&s_frame_queue, "lea_tx_q",
	                    sizeof(le_audio_tx_frame_t), LE_AUDIO_TX_FRAME_QUEUE) != BK_OK)
	{
		BK_LOGE(TAG, "frame queue init failed\n");
		le_audio_tx_deinit();
		return -1;
	}

	if (rtos_init_semaphore_ex(&s_encode_exit_sem, 1, 0) != BK_OK ||
	    rtos_init_semaphore_ex(&s_send_exit_sem, 1, 0) != BK_OK)
	{
		BK_LOGE(TAG, "exit sem init failed\n");
		le_audio_tx_deinit();
		return -1;
	}

	lc3_encoder_init(&s_enc_info, s_enc_mem);

	if (s_source && s_source->open && s_source->open(&s_cfg) != 0)
	{
		BK_LOGE(TAG, "pcm source open failed\n");
		le_audio_tx_deinit();
		return -1;
	}

	BK_LOGI(TAG, "tx ready dt=%dus sr=%dHz samples=%d frame_bytes=%d\n",
	        dt, sr, s_enc_info.frame_samples, s_enc_info.frame_bytes);
	return 0;
}

static void le_audio_tx_encode_thread(void *arg)
{
	uint16_t seq = 0;

	(void)arg;

	while (s_run)
	{
		le_audio_tx_frame_t frame;

		os_memset(&frame, 0, sizeof(frame));

		if (s_source && s_source->read)
		{
			s_source->read(s_pcm_in, s_enc_info.frame_samples);
		}
		else
		{
			os_memset(s_pcm_in, 0, s_enc_info.frame_samples * sizeof(int16_t));
		}

		lc3_encoder_proc(&s_enc_info, (uint8_t *)s_pcm_in, frame.data);
		frame.seq = seq;
		frame.len = (uint16_t)s_enc_info.frame_bytes;
		if (rtos_push_to_queue(&s_frame_queue, &frame, 20) == BK_OK)
		{
			seq++;
		}
	}

	BK_LOGI(TAG, "tx encode exit seq=%u\n", seq);
	if (s_encode_exit_sem)
	{
		rtos_set_semaphore(&s_encode_exit_sem);
	}
	s_encode_thread = NULL;
	rtos_delete_thread(NULL);
}

static void le_audio_tx_send_thread(void *arg)
{
	uint32_t delay_ms = s_cfg.frame_us / 1000;
	uint32_t next_send_ms;
	uint32_t send_fail_count = 0;
	uint32_t underrun_count = 0;
	uint32_t late_count = 0;

	(void)arg;

	if (delay_ms == 0)
	{
		delay_ms = 10;
	}

	BK_LOGI(TAG, "tx send start handle=0x%04x\n", s_handle);
	rtos_delay_milliseconds(delay_ms * (LE_AUDIO_TX_FRAME_QUEUE - 1));
	next_send_ms = rtos_get_time();

	while (s_run)
	{
		bk_err_t ret;
		le_audio_tx_frame_t frame;

		if (rtos_pop_from_queue(&s_frame_queue, &frame, delay_ms) != BK_OK)
		{
			underrun_count++;
			if ((underrun_count == 1U) || ((underrun_count % 100U) == 0U))
			{
				BK_LOGW(TAG, "tx queue underrun #%lu\n", underrun_count);
			}
			continue;
		}

		if (!s_run)
		{
			break;
		}

		ret = s_send_fn ? s_send_fn(s_handle, frame.seq, frame.data, frame.len) : BK_FAIL;
		if (ret != BK_OK)
		{
			send_fail_count++;
			if ((send_fail_count == 1U) || (send_fail_count >= LE_AUDIO_TX_SEND_FAIL_LIMIT))
			{
				BK_LOGW(TAG, "tx send failed ret=0x%x count=%lu handle=0x%04x seq=%u\n",
				        ret, send_fail_count, s_handle, frame.seq);
			}

			if (send_fail_count >= LE_AUDIO_TX_SEND_FAIL_LIMIT)
			{
				BK_LOGW(TAG, "stop tx after consecutive send failures\n");
				s_run = 0;
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
					BK_LOGW(TAG, "tx send late #%lu lag=%ldms\n",
					        late_count, (int32_t)(now_ms - next_send_ms));
				}
				next_send_ms = now_ms;
			}
		}
	}

	le_audio_tx_wait_exit(&s_encode_thread, &s_encode_exit_sem, 1000);
	BK_LOGI(TAG, "tx send exit\n");
	if (s_send_exit_sem)
	{
		rtos_set_semaphore(&s_send_exit_sem);
	}
	s_send_thread = NULL;
	rtos_delete_thread(NULL);
}

int le_audio_audio_tx_start(uint16_t handle, le_audio_tx_send_fn send, const le_audio_codec_cfg_t *cfg)
{
	if (s_run)
	{
		return 0;
	}

	if (!send || !cfg)
	{
		return -1;
	}

	le_audio_tx_wait_exit(&s_send_thread, &s_send_exit_sem, 2000);
	le_audio_tx_wait_exit(&s_encode_thread, &s_encode_exit_sem, 1000);
	if (s_enc_mem || s_pcm_in || s_frame_queue || s_encode_exit_sem || s_send_exit_sem)
	{
		le_audio_tx_deinit();
	}

	s_cfg = *cfg;
	s_handle = handle;
	s_send_fn = send;

	if (le_audio_tx_init() != 0)
	{
		return -1;
	}

	s_run = 1;

	if (rtos_create_thread(&s_encode_thread, LE_AUDIO_TX_ENCODE_PRIORITY, "lea_tx_enc",
	                       le_audio_tx_encode_thread, LE_AUDIO_TX_ENCODE_STACK, NULL) != 0)
	{
		BK_LOGE(TAG, "create tx encode thread failed\n");
		s_run = 0;
		le_audio_tx_deinit();
		return -1;
	}

	if (rtos_create_thread(&s_send_thread, LE_AUDIO_TX_SEND_PRIORITY, "lea_tx_send",
	                       le_audio_tx_send_thread, LE_AUDIO_TX_SEND_STACK, NULL) != 0)
	{
		BK_LOGE(TAG, "create tx send thread failed\n");
		s_run = 0;
		le_audio_tx_wait_exit(&s_encode_thread, &s_encode_exit_sem, 1000);
		le_audio_tx_deinit();
		return -1;
	}

	return 0;
}

int le_audio_audio_tx_stop(void)
{
	if (!s_run && !s_encode_thread && !s_send_thread &&
	    !s_enc_mem && !s_pcm_in && !s_frame_queue && !s_encode_exit_sem && !s_send_exit_sem)
	{
		return 0;
	}

	s_run = 0;
	le_audio_tx_wait_exit(&s_send_thread, &s_send_exit_sem, 2000);
	le_audio_tx_wait_exit(&s_encode_thread, &s_encode_exit_sem, 1000);
	le_audio_tx_deinit();
	return 0;
}
