#include "audio.h"

#include <components/bluetooth/bk_assigned_numbers.h>

#include <components/log.h>
#include <modules/lc3_codec.h>
#include <os/mem.h>
#include <os/os.h>

#define TAG "lea_rx"

#define LE_AUDIO_RX_MSG_COUNT     16U
#define LE_AUDIO_RX_TASK_PRIO     6U
#define LE_AUDIO_RX_STACK         4096U
#define LE_AUDIO_RX_LC3_WORK_BYTES 9064U
#define LE_AUDIO_RX_FRAME_MAX_BYTES 160U

enum
{
	LE_AUDIO_RX_MSG_NULL = 0,
	LE_AUDIO_RX_MSG_PLAY_START,
	LE_AUDIO_RX_MSG_PLAY_STOP,
	LE_AUDIO_RX_MSG_ISO_DATA,
};

typedef struct
{
	uint8_t  type;
	uint16_t len;
	uint8_t *data;
} le_audio_rx_msg_t;

typedef struct
{
	bk_bap_iso_header_t header;
	uint16_t lc3_len;
	uint8_t  lc3[LE_AUDIO_RX_FRAME_MAX_BYTES];
} le_audio_rx_iso_pkt_t;

typedef struct
{
	uint16_t       handle;
	lc3_dec_info_t dec;
	uint8_t       *dec_mem;
} le_audio_rx_stream_t;

typedef struct
{
	uint16_t handles[LE_AUDIO_MAX_STREAMS];
	uint8_t  count;
	le_audio_codec_cfg_t cfg;
} le_audio_rx_group_t;

static const le_audio_pcm_sink_t *s_sink;
static le_audio_codec_cfg_t       s_cfg;

static beken_queue_t   s_rx_queue;
static beken_thread_t  s_rx_thread;
static beken_semaphore_t s_rx_stop_sema;
static uint8_t         s_rx_inited;

static le_audio_rx_stream_t s_streams[LE_AUDIO_MAX_STREAMS];
static uint8_t         s_stream_count;
static int16_t        *s_pcm_ch;
static int16_t        *s_pcm_out;
static uint8_t         s_dec_ready;
static uint8_t         s_sink_opened;
static uint32_t        s_lc3_rx_count;
static uint32_t        s_unknown_count;

/* Streams already collected for the SDU period being assembled */
static uint8_t         s_group_mask;

void le_audio_audio_set_sink(const le_audio_pcm_sink_t *snk)
{
	s_sink = snk;
}

static void le_audio_rx_msg_release(le_audio_rx_msg_t *msg)
{
	if (msg && msg->data)
	{
		os_free(msg->data);
		msg->data = NULL;
	}
}

static int le_audio_rx_queue_push(uint8_t type, const void *data, uint16_t len, uint32_t timeout_ms)
{
	le_audio_rx_msg_t msg = {0};
	int rc;

	if (!s_rx_queue)
	{
		return BK_FAIL;
	}

	if (len)
	{
		if (!data)
		{
			return BK_FAIL;
		}

		msg.data = (uint8_t *)os_malloc(len);
		if (!msg.data)
		{
			return BK_FAIL;
		}
		os_memcpy(msg.data, data, len);
		msg.len = len;
	}

	msg.type = type;
	rc = rtos_push_to_queue(&s_rx_queue, &msg, timeout_ms);
	if (rc != BK_OK)
	{
		le_audio_rx_msg_release(&msg);
	}

	return rc;
}

static void le_audio_rx_decoder_free(void)
{
	uint8_t i;

	for (i = 0; i < LE_AUDIO_MAX_STREAMS; i++)
	{
		if (s_streams[i].dec_mem)
		{
			os_free(s_streams[i].dec_mem);
		}
	}
	os_memset(s_streams, 0, sizeof(s_streams));

	if (s_pcm_ch)
	{
		os_free(s_pcm_ch);
		s_pcm_ch = NULL;
	}

	if (s_pcm_out)
	{
		os_free(s_pcm_out);
		s_pcm_out = NULL;
	}

	s_dec_ready = 0;
	s_group_mask = 0;
}

static int le_audio_rx_decoder_init(void)
{
	int dt = (int)s_cfg.frame_us;
	int sr = (int)s_cfg.sample_rate;
	int frame_samples;
	uint8_t i;

	if (s_dec_ready)
	{
		return 0;
	}

	frame_samples = (sr * dt) / 1000000;
	if ((frame_samples <= 0) || (s_stream_count == 0))
	{
		BK_LOGE(TAG, "bad lc3 params dt=%d sr=%d streams=%u\n", dt, sr, s_stream_count);
		return -1;
	}

	/* One decoder per stream: each one carries a single channel */
	for (i = 0; i < s_stream_count; i++)
	{
		s_streams[i].dec.channel = 1;
		s_streams[i].dec.frame_us = dt;
		s_streams[i].dec.frame_samples = frame_samples;
		s_streams[i].dec.frame_bytes = s_cfg.frame_bytes;
		s_streams[i].dec.dec_srate_hz = sr;
		s_streams[i].dec.pcm_sbytes = sizeof(int16_t);
		s_streams[i].dec.frame_cnt = 0;
		s_streams[i].dec_mem = (uint8_t *)os_malloc(LE_AUDIO_RX_LC3_WORK_BYTES);
		if (!s_streams[i].dec_mem)
		{
			BK_LOGE(TAG, "lc3 decoder alloc failed stream=%u\n", i);
			le_audio_rx_decoder_free();
			return -1;
		}
		lc3_decoder_init(&s_streams[i].dec, s_streams[i].dec_mem);
	}

	s_pcm_ch = (int16_t *)os_malloc(frame_samples * sizeof(int16_t));
	s_pcm_out = (int16_t *)os_malloc(frame_samples * s_stream_count * sizeof(int16_t));
	if (!s_pcm_ch || !s_pcm_out)
	{
		BK_LOGE(TAG, "pcm alloc failed ch=%p out=%p\n", s_pcm_ch, s_pcm_out);
		le_audio_rx_decoder_free();
		return -1;
	}

	os_memset(s_pcm_out, 0, frame_samples * s_stream_count * sizeof(int16_t));
	s_group_mask = 0;
	s_dec_ready = 1;

	BK_LOGI(TAG, "lc3 decoder ready streams=%u dt=%dus sr=%dHz samples=%d frame_bytes=%d\n",
	        s_stream_count, dt, sr, frame_samples, s_cfg.frame_bytes);
	return 0;
}

static int le_audio_rx_sink_open(void)
{
	if (s_sink_opened)
	{
		return 0;
	}

	if (!s_sink || !s_sink->open)
	{
		return -1;
	}

	if (s_sink->open(&s_cfg) != 0)
	{
		return -1;
	}

	s_sink_opened = 1;
	return 0;
}

static void le_audio_rx_sink_close(void)
{
	if (s_sink_opened && s_sink && s_sink->close)
	{
		s_sink->close();
	}
	s_sink_opened = 0;
}

static void le_audio_rx_teardown(void)
{
	le_audio_rx_sink_close();
	le_audio_rx_decoder_free();
	s_stream_count = 0;
	s_lc3_rx_count = 0;
	s_unknown_count = 0;
}

static int le_audio_rx_stream_index(uint16_t handle)
{
	uint8_t i;

	for (i = 0; i < s_stream_count; i++)
	{
		if (s_streams[i].handle == handle)
		{
			return (int)i;
		}
	}

	return -1;
}

/* Hand one SDU period to the sink, silent on channels that did not arrive */
static void le_audio_rx_flush_group(void)
{
	int samples = s_streams[0].dec.frame_samples * s_stream_count;

	if (s_group_mask == 0)
	{
		return;
	}

	if (s_sink && s_sink->write)
	{
		s_sink->write(s_pcm_out, samples);
	}

	os_memset(s_pcm_out, 0, samples * sizeof(int16_t));
	s_group_mask = 0;
}

static void le_audio_rx_handle_iso(const le_audio_rx_iso_pkt_t *pkt)
{
	uint16_t frame_bytes;
	int index;
	int frame_samples;
	int i;

	if (!pkt || pkt->lc3_len == 0U)
	{
		return;
	}

	index = le_audio_rx_stream_index(pkt->header.connection_handle);
	if (index < 0)
	{
		s_unknown_count++;
		if ((s_unknown_count == 1U) || ((s_unknown_count % 1000U) == 0U))
		{
			BK_LOGW(TAG, "iso for unknown handle=0x%03x #%lu, streams=%u\n",
			        pkt->header.connection_handle,
			        (unsigned long)s_unknown_count, s_stream_count);
		}
		return;
	}

	if (!s_dec_ready && le_audio_rx_decoder_init() != 0)
	{
		BK_LOGW(TAG, "lc3 data dropped, decoder not ready len=%u\n", pkt->lc3_len);
		return;
	}

	if (le_audio_rx_sink_open() != 0)
	{
		BK_LOGW(TAG, "lc3 data dropped, sink not ready len=%u\n", pkt->lc3_len);
		return;
	}

	s_lc3_rx_count++;
	if (s_lc3_rx_count == 1U)
	{
		BK_LOGI(TAG, "first iso payload handle=0x%03x seq=%lu len=%u status=%u\n",
		        pkt->header.connection_handle,
		        pkt->header.packet_sequence_number,
		        pkt->lc3_len,
		        pkt->header.packet_status_flag);
	}

	frame_bytes = pkt->lc3_len;
	if (frame_bytes > s_streams[index].dec.frame_bytes)
	{
		frame_bytes = s_streams[index].dec.frame_bytes;
	}
	s_streams[index].dec.frame_bytes = frame_bytes;

	lc3_decoder_proc(&s_streams[index].dec, (uint8_t *)pkt->lc3, (uint8_t *)s_pcm_ch);

	/*
	 * This stream already delivered the period being assembled, so it is the
	 * next one: release what was collected, silent on the channels that were
	 * lost. Each stream produces exactly one frame per SDU interval.
	 */
	if (s_group_mask & (uint8_t)(1U << index))
	{
		le_audio_rx_flush_group();
	}

	frame_samples = s_streams[index].dec.frame_samples;
	for (i = 0; i < frame_samples; i++)
	{
		s_pcm_out[i * s_stream_count + index] = s_pcm_ch[i];
	}
	s_group_mask |= (uint8_t)(1U << index);

	if (s_group_mask == (uint8_t)((1U << s_stream_count) - 1U))
	{
		le_audio_rx_flush_group();
	}
}

static void le_audio_rx_task(void *arg)
{
	(void)arg;

	while (1)
	{
		le_audio_rx_msg_t msg = {0};

		if (rtos_pop_from_queue(&s_rx_queue, &msg, BEKEN_WAIT_FOREVER) != BK_OK)
		{
			continue;
		}

		switch (msg.type)
		{
		case LE_AUDIO_RX_MSG_PLAY_START:
			if (msg.data && msg.len >= sizeof(le_audio_rx_group_t))
			{
				const le_audio_rx_group_t *group = (const le_audio_rx_group_t *)msg.data;
				uint8_t i;

				le_audio_rx_teardown();
				s_cfg = group->cfg;
				s_stream_count = group->count;
				for (i = 0; i < group->count; i++)
				{
					s_streams[i].handle = group->handles[i];
				}
				BK_LOGI(TAG, "PLAY_START streams=%u handle=0x%04x/0x%04x\n",
				        group->count, group->handles[0],
				        (group->count > 1) ? group->handles[1] : 0);
				le_audio_rx_decoder_init();
				le_audio_rx_sink_open();
			}
			break;

		case LE_AUDIO_RX_MSG_PLAY_STOP:
			BK_LOGI(TAG, "PLAY_STOP\n");
			le_audio_rx_teardown();
			if (s_rx_stop_sema)
			{
				rtos_set_semaphore(&s_rx_stop_sema);
			}
			break;

		case LE_AUDIO_RX_MSG_ISO_DATA:
			if (msg.data && msg.len >= sizeof(le_audio_rx_iso_pkt_t))
			{
				le_audio_rx_handle_iso((const le_audio_rx_iso_pkt_t *)msg.data);
			}
			break;

		default:
			break;
		}

		le_audio_rx_msg_release(&msg);
	}
}

int le_audio_audio_rx_init(void)
{
	bk_err_t ret;

	if (s_rx_inited)
	{
		return BK_OK;
	}

	le_audio_codec_cfg_default(&s_cfg);

	ret = rtos_init_semaphore(&s_rx_stop_sema, 1);
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "stop sema init failed\n");
		return ret;
	}

	ret = rtos_init_queue(&s_rx_queue, "lea_rx_q",
	                      sizeof(le_audio_rx_msg_t), LE_AUDIO_RX_MSG_COUNT);
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "rx queue init failed\n");
		rtos_deinit_semaphore(&s_rx_stop_sema);
		s_rx_stop_sema = NULL;
		return ret;
	}

	ret = rtos_create_thread(&s_rx_thread, LE_AUDIO_RX_TASK_PRIO, "lea_rx",
	                         (beken_thread_function_t)le_audio_rx_task,
	                         LE_AUDIO_RX_STACK, NULL);
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "rx thread create failed\n");
		rtos_deinit_queue(&s_rx_queue);
		s_rx_queue = NULL;
		rtos_deinit_semaphore(&s_rx_stop_sema);
		s_rx_stop_sema = NULL;
		s_rx_thread = NULL;
		return ret;
	}

	s_rx_inited = 1;
	BK_LOGI(TAG, "rx thread started\n");
	return BK_OK;
}

int le_audio_audio_rx_start(const uint16_t *handles, uint8_t count,
                            const le_audio_codec_cfg_t *cfg)
{
	le_audio_rx_group_t group = {0};
	uint8_t i;

	if (!handles || !cfg || count == 0 || count > LE_AUDIO_MAX_STREAMS)
	{
		return BK_ERR_PARAM;
	}

	/* Already playing this group; a new group replaces the old one */
	if (s_stream_count == count)
	{
		for (i = 0; i < count; i++)
		{
			if (s_streams[i].handle != handles[i])
			{
				break;
			}
		}
		if (i == count)
		{
			return BK_OK;
		}
	}

	group.count = count;
	group.cfg = *cfg;
	group.cfg.channels = count;
	for (i = 0; i < count; i++)
	{
		group.handles[i] = handles[i];
	}

	return le_audio_rx_queue_push(LE_AUDIO_RX_MSG_PLAY_START, &group, sizeof(group),
	                              BEKEN_WAIT_FOREVER);
}

void le_audio_audio_rx_stop(void)
{
	if (!s_rx_inited)
	{
		le_audio_rx_teardown();
		return;
	}

	if (le_audio_rx_queue_push(LE_AUDIO_RX_MSG_PLAY_STOP, NULL, 0, BEKEN_WAIT_FOREVER) != BK_OK)
	{
		BK_LOGW(TAG, "PLAY_STOP queue push failed\n");
		return;
	}

	if (s_rx_stop_sema)
	{
		rtos_get_semaphore(&s_rx_stop_sema, 2000);
	}
}

int le_audio_audio_rx_push_iso(bk_bap_iso_header_t *header, uint8_t *data, uint32_t length)
{
	le_audio_rx_iso_pkt_t pkt;
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
	pkt.lc3_len = (uint16_t)((length > LE_AUDIO_RX_FRAME_MAX_BYTES) ?
	                         LE_AUDIO_RX_FRAME_MAX_BYTES : length);
	os_memcpy(pkt.lc3, data, pkt.lc3_len);

	if (le_audio_rx_queue_push(LE_AUDIO_RX_MSG_ISO_DATA, &pkt, sizeof(pkt), 2) != BK_OK)
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
