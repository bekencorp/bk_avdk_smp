#include "le_audio_sink_media.h"

#include "../audio/le_audio_playback.h"
#include "le_audio_user_config.h"

#include <components/log.h>
#include <modules/lc3_codec.h>
#include <os/mem.h>
#include <os/os.h>

#define TAG "lea_sink_media"

#define LE_AUDIO_SINK_MEDIA_MSG_COUNT    16U
#define LE_AUDIO_SINK_MEDIA_TASK_PRIO    6U
#define LE_AUDIO_SINK_MEDIA_STACK        4096U
#define LE_AUDIO_LC3_WORK_BUFFER_SIZE    9064U

enum
{
	LE_AUDIO_SINK_MEDIA_MSG_NULL = 0,
	LE_AUDIO_SINK_MEDIA_MSG_PLAY_START,
	LE_AUDIO_SINK_MEDIA_MSG_PLAY_STOP,
	LE_AUDIO_SINK_MEDIA_MSG_ISO_DATA,
};

typedef struct
{
	uint8_t type;
	uint16_t len;
	uint8_t *data;
} le_audio_sink_media_msg_t;

typedef struct
{
	bk_bap_iso_header_t header;
	uint16_t lc3_len;
	uint8_t lc3[CONFIG_LE_AUDIO_FRAME_BYTES];
} le_audio_sink_media_iso_pkt_t;

static beken_queue_t s_sink_media_queue;
static beken_thread_t s_sink_media_thread;
static beken_semaphore_t s_sink_media_stop_sema;
static uint8_t s_sink_media_task_inited;

static lc3_dec_info_t s_dec_info;
static uint8_t *s_dec_mem;
static int16_t *s_pcm;
static uint8_t s_dec_ready;
static uint32_t s_lc3_rx_count;

static void le_audio_sink_media_msg_release(le_audio_sink_media_msg_t *msg)
{
	if (!msg || !msg->data)
	{
		return;
	}

	os_free(msg->data);
	msg->data = NULL;
}

static int le_audio_sink_media_queue_push(uint8_t type, const void *data, uint16_t len, uint32_t timeout_ms)
{
	le_audio_sink_media_msg_t msg = {0};
	int rc;

	if (!s_sink_media_queue)
	{
		BK_LOGE(TAG, "media queue not ready\n");
		return BK_FAIL;
	}

	if (len)
	{
		if (!data)
		{
			BK_LOGE(TAG, "queue push null data type=%u len=%u\n", type, len);
			return BK_FAIL;
		}

		msg.data = (uint8_t *)os_malloc(len);
		if (!msg.data)
		{
			BK_LOGE(TAG, "queue push malloc failed type=%u len=%u\n", type, len);
			return BK_FAIL;
		}
		os_memcpy(msg.data, data, len);
		msg.len = len;
	}

	msg.type = type;
	rc = rtos_push_to_queue(&s_sink_media_queue, &msg, timeout_ms);
	if (rc != BK_OK)
	{
		le_audio_sink_media_msg_release(&msg);
	}

	return rc;
}

static void le_audio_sink_decoder_free(void)
{
	if (s_dec_mem)
	{
		os_free(s_dec_mem);
		s_dec_mem = NULL;
	}

	if (s_pcm)
	{
		os_free(s_pcm);
		s_pcm = NULL;
	}

	s_dec_ready = 0;
	os_memset(&s_dec_info, 0, sizeof(s_dec_info));
}

static int le_audio_sink_decoder_init(void)
{
	int dt = CONFIG_LE_AUDIO_FRAME_US;
	int sr = CONFIG_LE_AUDIO_SAMPLE_RATE;
	int frame_samples;

	if (s_dec_ready)
	{
		return 0;
	}

	frame_samples = (sr * dt) / 1000000;
	if (frame_samples <= 0)
	{
		BK_LOGE(TAG, "bad lc3 params dt=%d sr=%d samples=%d\n", dt, sr, frame_samples);
		return -1;
	}

	s_dec_mem = (uint8_t *)os_malloc(LE_AUDIO_LC3_WORK_BUFFER_SIZE);
	s_pcm = (int16_t *)os_malloc(frame_samples * sizeof(int16_t));
	if (!s_dec_mem || !s_pcm)
	{
		BK_LOGE(TAG, "lc3 decoder alloc failed (mem=%p pcm=%p)\n", s_dec_mem, s_pcm);
		le_audio_sink_decoder_free();
		return -1;
	}

	os_memset(&s_dec_info, 0, sizeof(s_dec_info));
	s_dec_info.channel = 1;
	s_dec_info.frame_us = dt;
	s_dec_info.frame_samples = frame_samples;
	s_dec_info.frame_bytes = CONFIG_LE_AUDIO_FRAME_BYTES;
	s_dec_info.dec_srate_hz = sr;
	s_dec_info.pcm_sbytes = sizeof(int16_t);
	s_dec_info.frame_cnt = 0;

	lc3_decoder_init(&s_dec_info, s_dec_mem);
	s_dec_ready = 1;

	BK_LOGI(TAG, "lc3 decoder ready dt=%dus sr=%dHz samples=%d frame_bytes=%d\n",
	        dt, sr, frame_samples, s_dec_info.frame_bytes);
	return 0;
}

static void le_audio_sink_media_teardown(void)
{
	le_audio_playback_close();
	le_audio_sink_decoder_free();
	s_lc3_rx_count = 0;
}

static void le_audio_sink_media_handle_iso_data(const le_audio_sink_media_iso_pkt_t *pkt)
{
	uint16_t frame_bytes;
	bk_bap_iso_header_t header;

	if (!pkt || pkt->lc3_len == 0U)
	{
		return;
	}

	if (!s_dec_ready && le_audio_sink_decoder_init() != 0)
	{
		BK_LOGW(TAG, "lc3 data dropped, decoder not ready len=%u\n", pkt->lc3_len);
		return;
	}

	if (le_audio_playback_open() != 0)
	{
		BK_LOGW(TAG, "lc3 data dropped, playback not ready len=%u\n", pkt->lc3_len);
		return;
	}

	s_lc3_rx_count++;
	header = pkt->header;

	if (s_lc3_rx_count == 1U)
	{
		BK_LOGI(TAG, "first iso payload handle=0x%03x seq=%lu len=%u status=%u\n",
		        header.connection_handle,
		        header.packet_sequence_number,
		        pkt->lc3_len,
		        header.packet_status_flag);
	}

	frame_bytes = pkt->lc3_len;
	if (frame_bytes > s_dec_info.frame_bytes)
	{
		frame_bytes = s_dec_info.frame_bytes;
	}
	s_dec_info.frame_bytes = frame_bytes;

	lc3_decoder_proc(&s_dec_info, (uint8_t *)pkt->lc3, (uint8_t *)s_pcm);
	le_audio_playback_write_pcm(s_pcm, (uint32_t)s_dec_info.frame_samples);

	if (s_lc3_rx_count == 1U)
	{
		BK_LOGI(TAG, "first lc3 decoded handle=0x%03x seq=%lu in=%uB samples=%d\n",
		        header.connection_handle,
		        header.packet_sequence_number,
		        frame_bytes,
		        s_dec_info.frame_samples);
	}
}

static void le_audio_sink_media_task(void *arg)
{
	(void)arg;

	while (1)
	{
		le_audio_sink_media_msg_t msg = {0};

		if (rtos_pop_from_queue(&s_sink_media_queue, &msg, BEKEN_WAIT_FOREVER) != BK_OK)
		{
			continue;
		}

		switch (msg.type)
		{
		case LE_AUDIO_SINK_MEDIA_MSG_PLAY_START:
			BK_LOGI(TAG, "PLAY_START\n");
			s_lc3_rx_count = 0;
			le_audio_sink_decoder_init();
#if CONFIG_AUDIO_PLAY
			le_audio_playback_open();
#endif
			break;

		case LE_AUDIO_SINK_MEDIA_MSG_PLAY_STOP:
			BK_LOGI(TAG, "PLAY_STOP\n");
			le_audio_sink_media_teardown();
			if (s_sink_media_stop_sema)
			{
				rtos_set_semaphore(&s_sink_media_stop_sema);
			}
			break;

		case LE_AUDIO_SINK_MEDIA_MSG_ISO_DATA:
			if (msg.data && msg.len >= sizeof(le_audio_sink_media_iso_pkt_t))
			{
				le_audio_sink_media_handle_iso_data((const le_audio_sink_media_iso_pkt_t *)msg.data);
			}
			break;

		default:
			break;
		}

		le_audio_sink_media_msg_release(&msg);
	}
}

int le_audio_sink_media_init(void)
{
	bk_err_t ret;

	if (s_sink_media_task_inited)
	{
		return BK_OK;
	}

	ret = rtos_init_semaphore(&s_sink_media_stop_sema, 1);
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "stop sema init failed\n");
		return ret;
	}

	ret = rtos_init_queue(&s_sink_media_queue,
	                      "lea_sink_media_q",
	                      sizeof(le_audio_sink_media_msg_t),
	                      LE_AUDIO_SINK_MEDIA_MSG_COUNT);
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "media queue init failed\n");
		rtos_deinit_semaphore(&s_sink_media_stop_sema);
		s_sink_media_stop_sema = NULL;
		return ret;
	}

	ret = rtos_create_thread(&s_sink_media_thread,
	                         LE_AUDIO_SINK_MEDIA_TASK_PRIO,
	                         "lea_sink_media",
	                         (beken_thread_function_t)le_audio_sink_media_task,
	                         LE_AUDIO_SINK_MEDIA_STACK,
	                         NULL);
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "media thread create failed\n");
		rtos_deinit_queue(&s_sink_media_queue);
		s_sink_media_queue = NULL;
		rtos_deinit_semaphore(&s_sink_media_stop_sema);
		s_sink_media_stop_sema = NULL;
		s_sink_media_thread = NULL;
		return ret;
	}

	s_sink_media_task_inited = 1;
	BK_LOGI(TAG, "media thread started\n");
	return BK_OK;
}

int le_audio_sink_media_start(void)
{
	return le_audio_sink_media_queue_push(LE_AUDIO_SINK_MEDIA_MSG_PLAY_START,
	                                      NULL,
	                                      0,
	                                      BEKEN_WAIT_FOREVER);
}

void le_audio_sink_media_stop(void)
{
	if (!s_sink_media_task_inited)
	{
		le_audio_sink_media_teardown();
		return;
	}

	if (le_audio_sink_media_queue_push(LE_AUDIO_SINK_MEDIA_MSG_PLAY_STOP, NULL, 0, BEKEN_WAIT_FOREVER) != BK_OK)
	{
		BK_LOGW(TAG, "PLAY_STOP queue push failed\n");
		return;
	}

	if (s_sink_media_stop_sema)
	{
		rtos_get_semaphore(&s_sink_media_stop_sema, 2000);
	}
}

int le_audio_sink_media_push_iso(bk_bap_iso_header_t *header, uint8_t *data, uint32_t length)
{
	le_audio_sink_media_iso_pkt_t pkt;
	static uint32_t s_empty_iso_count;
	static uint32_t s_drop_count;

	if (!data || length == 0)
	{
		s_empty_iso_count++;
		if ((s_empty_iso_count == 1U) || ((s_empty_iso_count % 1000U) == 0U))
		{
			BK_LOGW(TAG, "iso empty #%lu handle=0x%03x seq=%lu status=%u\n",
			        s_empty_iso_count,
			        header ? header->connection_handle : 0,
			        header ? header->packet_sequence_number : 0,
			        header ? header->packet_status_flag : 0);
		}
		return BK_OK;
	}

	if (!header)
	{
		return BK_FAIL;
	}

	os_memset(&pkt, 0, sizeof(pkt));
	os_memcpy(&pkt.header, header, sizeof(pkt.header));
	pkt.lc3_len = (uint16_t)((length > CONFIG_LE_AUDIO_FRAME_BYTES) ?
	                         CONFIG_LE_AUDIO_FRAME_BYTES : length);
	os_memcpy(pkt.lc3, data, pkt.lc3_len);

	if (le_audio_sink_media_queue_push(LE_AUDIO_SINK_MEDIA_MSG_ISO_DATA, &pkt, sizeof(pkt), 2) != BK_OK)
	{
		s_drop_count++;
		if ((s_drop_count == 1U) || ((s_drop_count % 100U) == 0U))
		{
			BK_LOGW(TAG, "iso queue full, dropped #%lu len=%u\n", s_drop_count, pkt.lc3_len);
		}
		return BK_FAIL;
	}

	return BK_OK;
}
