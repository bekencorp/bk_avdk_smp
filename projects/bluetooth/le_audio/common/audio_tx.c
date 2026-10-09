#include "audio.h"

#include <components/bluetooth/bk_assigned_numbers.h>

#include <components/log.h>
#include <modules/lc3_codec.h>
#include <os/mem.h>
#include <os/os.h>

#define TAG "lea_tx"

#define LE_AUDIO_TX_PRIORITY        4
#define LE_AUDIO_TX_STACK           6144
#define LE_AUDIO_TX_LC3_WORK_BYTES  9064
#define LE_AUDIO_TX_FRAME_MAX_BYTES 160
#define LE_AUDIO_TX_SEND_FAIL_LIMIT 10
/*
 * SDUs allowed in flight per stream. A credit is spent per SDU and returned by
 * the ISO transmit completion, so the controller sets the rate.
 */
#define LE_AUDIO_TX_CREDITS         3
/* Progress report period, in SDUs */
#define LE_AUDIO_TX_REPORT_SDUS     1000UL

extern int lc3_frame_samples(int dt_us, int sr_hz);

typedef struct
{
	uint16_t       handle;
	lc3_enc_info_t enc;
	uint8_t       *enc_mem;
} le_audio_tx_stream_t;

static const le_audio_pcm_source_t *s_source;

static le_audio_tx_stream_t s_streams[LE_AUDIO_MAX_STREAMS];
static uint8_t         s_stream_count;
static int16_t        *s_pcm_in;
static int16_t        *s_pcm_ch;
static beken_thread_t  s_thread;
static beken_semaphore_t s_exit_sem;
static beken_semaphore_t s_credit_sem;
static volatile uint8_t s_run;
static le_audio_tx_send_fn s_send_fn;
static le_audio_codec_cfg_t s_cfg;

void le_audio_audio_set_source(const le_audio_pcm_source_t *src)
{
	s_source = src;
}

static int le_audio_tx_frame_samples(void)
{
	return s_streams[0].enc.frame_samples;
}

static int le_audio_tx_handles_match(const uint16_t *handles, uint8_t count)
{
	uint8_t i;

	if (count != s_stream_count)
	{
		return 0;
	}

	for (i = 0; i < count; i++)
	{
		if (handles[i] != s_streams[i].handle)
		{
			return 0;
		}
	}

	return 1;
}

static void le_audio_tx_deinit(void)
{
	uint8_t i;

	for (i = 0; i < LE_AUDIO_MAX_STREAMS; i++)
	{
		if (s_streams[i].enc_mem)
		{
			os_free(s_streams[i].enc_mem);
		}
	}
	os_memset(s_streams, 0, sizeof(s_streams));
	s_stream_count = 0;

	if (s_pcm_in)
	{
		os_free(s_pcm_in);
		s_pcm_in = NULL;
	}

	if (s_pcm_ch)
	{
		os_free(s_pcm_ch);
		s_pcm_ch = NULL;
	}

	if (s_exit_sem)
	{
		rtos_deinit_semaphore(&s_exit_sem);
		s_exit_sem = NULL;
	}

	if (s_credit_sem)
	{
		rtos_deinit_semaphore(&s_credit_sem);
		s_credit_sem = NULL;
	}

	if (s_source && s_source->close)
	{
		s_source->close();
	}
}

static void le_audio_tx_wait_exit(void)
{
	if (!s_thread)
	{
		return;
	}

	if (s_exit_sem)
	{
		rtos_get_semaphore(&s_exit_sem, 2000);
	}
	else
	{
		while (s_thread)
		{
			rtos_delay_milliseconds(5);
		}
	}
}

static int le_audio_tx_init(const uint16_t *handles, uint8_t count)
{
	int dt = (int)s_cfg.frame_us;
	int sr = (int)s_cfg.sample_rate;
	int frame_samples = lc3_frame_samples(dt, sr);
	int credits = LE_AUDIO_TX_CREDITS * count;
	uint8_t i;

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

	/* One encoder per stream: each one codes a single channel */
	for (i = 0; i < count; i++)
	{
		s_streams[i].handle = handles[i];
		s_streams[i].enc.channel = 1;
		s_streams[i].enc.frame_us = dt;
		s_streams[i].enc.enc_srate_hz = sr;
		s_streams[i].enc.pcm_sbytes = sizeof(int16_t);
		s_streams[i].enc.bitrate = (uint32_t)s_cfg.frame_bytes * 8 * (1000000 / dt);
		s_streams[i].enc_mem = (uint8_t *)os_malloc(LE_AUDIO_TX_LC3_WORK_BYTES);
		if (!s_streams[i].enc_mem)
		{
			BK_LOGE(TAG, "encoder alloc failed stream=%u\n", i);
			le_audio_tx_deinit();
			return -1;
		}
	}
	s_stream_count = count;

	s_pcm_in = (int16_t *)os_malloc(frame_samples * count * sizeof(int16_t));
	s_pcm_ch = (int16_t *)os_malloc(frame_samples * sizeof(int16_t));
	if (!s_pcm_in || !s_pcm_ch)
	{
		BK_LOGE(TAG, "pcm alloc failed in=%p ch=%p\n", s_pcm_in, s_pcm_ch);
		le_audio_tx_deinit();
		return -1;
	}

	if (rtos_init_semaphore_ex(&s_exit_sem, 1, 0) != BK_OK)
	{
		BK_LOGE(TAG, "exit sem init failed\n");
		le_audio_tx_deinit();
		return -1;
	}

	/* Prime the window so the controller has SDUs to work through */
	if (rtos_init_semaphore_ex(&s_credit_sem, credits, credits) != BK_OK)
	{
		BK_LOGE(TAG, "credit sem init failed\n");
		le_audio_tx_deinit();
		return -1;
	}

	for (i = 0; i < count; i++)
	{
		lc3_encoder_init(&s_streams[i].enc, s_streams[i].enc_mem);
	}

	if (s_source && s_source->open && s_source->open(&s_cfg) != 0)
	{
		BK_LOGE(TAG, "pcm source open failed\n");
		le_audio_tx_deinit();
		return -1;
	}

	BK_LOGI(TAG, "tx ready streams=%u dt=%dus sr=%dHz samples=%d frame_bytes=%d\n",
	        count, dt, sr, s_streams[0].enc.frame_samples, s_streams[0].enc.frame_bytes);
	return 0;
}

static void le_audio_tx_read_pcm(void)
{
	int samples = le_audio_tx_frame_samples() * s_stream_count;

	if (s_source && s_source->read)
	{
		s_source->read(s_pcm_in, samples);
	}
	else
	{
		os_memset(s_pcm_in, 0, samples * sizeof(int16_t));
	}
}

/* Pick channel index out of the interleaved source frame */
static const int16_t *le_audio_tx_channel_pcm(uint8_t index)
{
	int frame_samples = le_audio_tx_frame_samples();
	int i;

	if (s_stream_count == 1)
	{
		return s_pcm_in;
	}

	for (i = 0; i < frame_samples; i++)
	{
		s_pcm_ch[i] = s_pcm_in[i * s_stream_count + index];
	}

	return s_pcm_ch;
}

static void le_audio_tx_thread(void *arg)
{
	uint8_t sdu[LE_AUDIO_TX_FRAME_MAX_BYTES];
	uint32_t interval_ms = s_cfg.frame_us / 1000;
	uint32_t sent_count = 0;
	uint32_t starve_count = 0;
	uint16_t seq = 0;
	uint32_t fail_count = 0;

	(void)arg;

	if (interval_ms == 0)
	{
		interval_ms = 10;
	}

	BK_LOGI(TAG, "tx start streams=%u handle=0x%04x/0x%04x interval=%lums\n",
	        s_stream_count, s_streams[0].handle,
	        (s_stream_count > 1) ? s_streams[1].handle : 0, (unsigned long)interval_ms);

	while (s_run)
	{
		uint8_t i;

		/*
		 * One credit per stream, so the whole group is paced by the controller.
		 * If it reports nothing within one SDU interval, send anyway and pace on
		 * the interval instead - a controller that never reports completions
		 * must not silence the stream.
		 */
		for (i = 0; i < s_stream_count; i++)
		{
			if (rtos_get_semaphore(&s_credit_sem, interval_ms) != BK_OK)
			{
				starve_count++;
			}
		}

		if (!s_run)
		{
			break;
		}

		le_audio_tx_read_pcm();

		for (i = 0; i < s_stream_count; i++)
		{
			bk_err_t ret;

			lc3_encoder_proc(&s_streams[i].enc,
			                 (uint8_t *)le_audio_tx_channel_pcm(i), sdu);

			ret = s_send_fn(s_streams[i].handle, seq, sdu,
			                (uint16_t)s_streams[i].enc.frame_bytes);
			if (ret == BK_OK)
			{
				fail_count = 0;
				continue;
			}

			fail_count++;
			BK_LOGW(TAG, "tx send failed ret=0x%x count=%lu handle=0x%04x seq=%u\n",
			        ret, (unsigned long)fail_count, s_streams[i].handle, seq);
			if (fail_count >= LE_AUDIO_TX_SEND_FAIL_LIMIT)
			{
				BK_LOGW(TAG, "stop tx after consecutive send failures\n");
				s_run = 0;
				break;
			}
		}

		seq++;
		if ((++sent_count % LE_AUDIO_TX_REPORT_SDUS) == 0UL)
		{
			BK_LOGI(TAG, "tx progress frames=%lu starved=%lu\n",
			        (unsigned long)sent_count, (unsigned long)starve_count);
		}
	}

	BK_LOGI(TAG, "tx exit frames=%lu starved=%lu\n",
	        (unsigned long)sent_count, (unsigned long)starve_count);
	if (s_exit_sem)
	{
		rtos_set_semaphore(&s_exit_sem);
	}
	s_thread = NULL;
	rtos_delete_thread(NULL);
}

void le_audio_audio_tx_iso_complete(uint16_t handle, uint16_t num_completed)
{
	uint8_t i;

	if (!s_run || !s_credit_sem)
	{
		return;
	}

	for (i = 0; i < s_stream_count; i++)
	{
		if (handle != s_streams[i].handle)
		{
			continue;
		}

		while (num_completed--)
		{
			rtos_set_semaphore(&s_credit_sem);
		}
		return;
	}
}

int le_audio_audio_tx_start(const uint16_t *handles, uint8_t count,
                            le_audio_tx_send_fn send, const le_audio_codec_cfg_t *cfg)
{
	if (!handles || !send || !cfg || count == 0 || count > LE_AUDIO_MAX_STREAMS)
	{
		return -1;
	}

	if (s_run)
	{
		/* Already sending this group; a new group replaces the old one */
		if (le_audio_tx_handles_match(handles, count))
		{
			return 0;
		}
		le_audio_audio_tx_stop();
	}

	le_audio_tx_wait_exit();
	le_audio_tx_deinit();

	s_cfg = *cfg;
	s_cfg.channels = count;
	s_send_fn = send;

	if (le_audio_tx_init(handles, count) != 0)
	{
		return -1;
	}

	s_run = 1;

	if (rtos_create_thread(&s_thread, LE_AUDIO_TX_PRIORITY, "lea_tx",
	                       le_audio_tx_thread, LE_AUDIO_TX_STACK, NULL) != 0)
	{
		BK_LOGE(TAG, "create tx thread failed\n");
		s_run = 0;
		le_audio_tx_deinit();
		return -1;
	}

	return 0;
}

int le_audio_audio_tx_stop(void)
{
	if (!s_run && !s_thread && !s_stream_count)
	{
		return 0;
	}

	s_run = 0;
	le_audio_tx_wait_exit();
	le_audio_tx_deinit();
	return 0;
}
