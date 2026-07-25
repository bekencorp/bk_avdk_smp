#include "broadcast_source_demo.h"
#include "../../common/audio.h"

#include <components/bluetooth/bk_assigned_numbers.h>
#include <components/bluetooth/bk_dm_bap.h>
#include <components/bluetooth/bk_dm_gap_ble.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/os.h>

#define TAG "lea_bsrc"
#define LE_AUDIO_BROADCAST_ID          0x725900
#define LE_AUDIO_PRESENTATION_DELAY_US 40000
#define LE_AUDIO_BSRC_SESSION_NONE     0xFF
#define BSRC_EVT_QUEUE_LEN             12U
#define BSRC_THREAD_PRIO               5U
#define BSRC_THREAD_STACK              4096U
#define BSRC_CMD_TIMEOUT_MS            5000U

enum
{
	BSRC_IDLE = 0,
	BSRC_ANNOUNCING,
	BSRC_STARTING,
	BSRC_STREAMING,
	BSRC_ENDING,
};

static uint8_t s_state = BSRC_IDLE;
static uint8_t s_session = LE_AUDIO_BSRC_SESSION_NONE;
static uint8_t s_sep;
static uint8_t s_stop_pending;
static uint16_t s_bis_handle;
static le_audio_codec_cfg_t s_cfg;
static uint8_t s_bcode[16];
static uint8_t s_bcode_set;

typedef enum
{
	BSRC_EVT_CMD_START = 1,
	BSRC_EVT_CMD_STOP,
	BSRC_EVT_ANNOUNCEMENT_DONE,
	BSRC_EVT_BROADCAST_STARTED,
	BSRC_EVT_BROADCAST_SUSPENDED,
	BSRC_EVT_ANNOUNCEMENT_ENDED,
} bsrc_evt_type_t;

typedef struct
{
	bsrc_evt_type_t type;
	uint8_t sync;
	uint32_t status;
	uint8_t handle;
	uint8_t reason;
	union
	{
		bk_bap_boradcast_paramters_t start;
	} u;
} bsrc_evt_t;

static beken_queue_t s_evt_queue;
static beken_thread_t s_evt_thread;
static beken_semaphore_t s_cmd_sem;
static int s_cmd_result;

static bk_err_t bsrc_send(uint16_t handle, uint16_t seq, uint8_t *data, uint16_t len)
{
	return bk_dm_bap_broadcast_data_send(handle, 0, 0, seq, data, len);
}

static int bsrc_post_event(const bsrc_evt_t *evt, uint32_t timeout_ms)
{
	if (!s_evt_queue || !evt)
	{
		return BK_FAIL;
	}
	return rtos_push_to_queue(&s_evt_queue, (void *)evt, timeout_ms);
}

static int bsrc_call_event(bsrc_evt_t *evt)
{
	int ret;

	if (!evt)
	{
		return BK_FAIL;
	}
	evt->sync = 1;
	s_cmd_result = BK_FAIL;
	ret = bsrc_post_event(evt, BEKEN_WAIT_FOREVER);
	if (ret != BK_OK)
	{
		return ret;
	}
	if (rtos_get_semaphore(&s_cmd_sem, BSRC_CMD_TIMEOUT_MS) != BK_OK)
	{
		return BK_FAIL;
	}
	return s_cmd_result;
}

static void bsrc_finish_cmd(const bsrc_evt_t *evt, int ret)
{
	if (evt && evt->sync)
	{
		s_cmd_result = ret;
		(void)rtos_set_semaphore(&s_cmd_sem);
	}
}

static void bsrc_reset_session(void)
{
	le_audio_audio_tx_stop();
	if (s_session != LE_AUDIO_BSRC_SESSION_NONE)
	{
		bk_dm_bap_broadcast_free_session(s_session);
	}
	s_session = LE_AUDIO_BSRC_SESSION_NONE;
	s_sep = 0;
	s_stop_pending = 0;
	s_bis_handle = 0;
	s_state = BSRC_IDLE;
}

static int bsrc_setup_announcement(void)
{
	bk_bap_codec_info_t codec;
	bk_bap_medadata_t meta;
	bk_bap_codec_ie_t stream;
	bk_bap_lc3_codec_specific_conf_t lc3_conf;
	uint8_t session = LE_AUDIO_BSRC_SESSION_NONE;
	int ret;

	if (s_session != LE_AUDIO_BSRC_SESSION_NONE)
	{
		BK_LOGW(TAG, "session %u already active\n", s_session);
		return -1;
	}

	le_audio_codec_cfg_default(&s_cfg);
	os_memset(&codec, 0, sizeof(codec));
	os_memset(&meta, 0, sizeof(meta));
	os_memset(&stream, 0, sizeof(stream));
	os_memset(&lc3_conf, 0, sizeof(lc3_conf));

	lc3_conf.sf = s_cfg.sf;
	lc3_conf.fd = s_cfg.fd;
	lc3_conf.aca = BK_BT_AUDIO_LOCATION_FRONT_LEFT;
	lc3_conf.opcf = s_cfg.frame_bytes;
	lc3_conf.mcfpSDU = 1;

	codec.coding_format = BK_BT_CODEC_ID_LC3;
	bk_dm_bap_create_codec_spec_conf_ltv(&lc3_conf, codec.ie, &codec.ie_len);
	bk_dm_bap_create_codec_spec_conf_ltv(&lc3_conf, stream.value, &stream.length);
	bk_dm_bap_create_metadata_ltv(meta.data, &meta.length);

	ret = bk_dm_bap_broadcast_alloc_session(&session);
	if (ret)
	{
		BK_LOGE(TAG, "alloc session failed ret=%d\n", ret);
		return ret;
	}
	s_session = session;

	ret = bk_dm_bap_broadcast_configure_session(s_session, 0x02, 0x00, (uint8_t *)broadcast_source_demo_broadcast_code());
	if (ret)
	{
		BK_LOGE(TAG, "configure session failed ret=%d\n", ret);
		goto fail;
	}

	ret = bk_dm_bap_broadcast_sep_register(s_session, &codec, &meta, 1, &stream, &s_sep);
	if (ret)
	{
		BK_LOGE(TAG, "sep register failed ret=%d\n", ret);
		goto fail;
	}

	ret = bk_dm_bap_setup_announcement(s_session, LE_AUDIO_BROADCAST_ID, 0, LE_AUDIO_PRESENTATION_DELAY_US);
	if (ret)
	{
		BK_LOGE(TAG, "setup announcement failed ret=%d\n", ret);
		goto fail;
	}

	s_state = BSRC_ANNOUNCING;
	BK_LOGI(TAG, "setup announcement pending session=%u sep=%u\n", s_session, s_sep);
	return 0;

fail:
	bsrc_reset_session();
	return ret;
}

static void bsrc_on_announcement(uint32_t status, void *param)
{
	bsrc_evt_t evt = {0};

	(void)param;
	evt.type = BSRC_EVT_ANNOUNCEMENT_DONE;
	evt.status = status;
	(void)bsrc_post_event(&evt, 0);
}

static void bsrc_on_start(bk_bap_boradcast_paramters_t *param)
{
	bsrc_evt_t evt = {0};

	if (!param)
	{
		return;
	}
	evt.type = BSRC_EVT_BROADCAST_STARTED;
	evt.u.start = *param;
	(void)bsrc_post_event(&evt, 0);
}

static void bsrc_on_suspend(uint8_t handle, uint8_t reason)
{
	bsrc_evt_t evt = {0};

	evt.type = BSRC_EVT_BROADCAST_SUSPENDED;
	evt.handle = handle;
	evt.reason = reason;
	(void)bsrc_post_event(&evt, 0);
}

static void bsrc_on_end(uint32_t status, void *param)
{
	bsrc_evt_t evt = {0};

	(void)param;
	evt.type = BSRC_EVT_ANNOUNCEMENT_ENDED;
	evt.status = status;
	(void)bsrc_post_event(&evt, 0);
}

static int bsrc_do_start(void)
{
	s_stop_pending = 0;
	return bsrc_setup_announcement();
}

static int bsrc_do_stop(void)
{
	if (s_session == LE_AUDIO_BSRC_SESSION_NONE)
	{
		le_audio_audio_tx_stop();
		s_state = BSRC_IDLE;
		return BK_OK;
	}

	if (s_state == BSRC_STREAMING || s_state == BSRC_STARTING)
	{
		le_audio_audio_tx_stop();
		s_stop_pending = 0;
		BK_LOGI(TAG, "suspend session=%u\n", s_session);
		return bk_dm_bap_broadcast_suspend(s_session);
	}
	if (s_state == BSRC_ANNOUNCING)
	{
		s_stop_pending = 1;
		BK_LOGI(TAG, "stop deferred until BIG starts\n");
		return BK_OK;
	}
	if (s_state == BSRC_ENDING)
	{
		return BK_OK;
	}

	bsrc_reset_session();
	return BK_OK;
}

static void bsrc_handle_announcement_done(uint32_t status)
{
	int ret;

	if (status != 0)
	{
		BK_LOGE(TAG, "setup announcement failed status=%lu\n", (unsigned long)status);
		bsrc_reset_session();
		return;
	}

	ret = bk_dm_bap_broadcast_start(s_session, s_cfg.frame_us, s_cfg.frame_bytes, 10, 2, 0);
	BK_LOGI(TAG, "create BIG session=%u ret=%d\n", s_session, ret);
	if (ret == BK_OK)
	{
		s_state = BSRC_STARTING;
	}
	else
	{
		(void)bsrc_do_stop();
	}
}

static void bsrc_handle_started(const bk_bap_boradcast_paramters_t *param)
{
	if (!param)
	{
		return;
	}
	if (param->error_code != 0 || param->num_bis == 0)
	{
		BK_LOGE(TAG, "broadcast start failed error=%u num_bis=%u\n", param->error_code, param->num_bis);
		(void)bsrc_do_stop();
		return;
	}

	s_bis_handle = param->connection_handle[0];
	BK_LOGI(TAG, "broadcast started big=%u bis_handle=0x%04x\n", param->big_handle, s_bis_handle);
	if (s_stop_pending)
	{
		s_stop_pending = 0;
		(void)bsrc_do_stop();
		return;
	}

	if (le_audio_audio_tx_start(s_bis_handle, bsrc_send, &s_cfg) == 0)
	{
		s_state = BSRC_STREAMING;
		BK_LOGI(TAG, "tx started; stop with ap_cmd le_audio stop\n");
	}
}

static void bsrc_handle_suspended(uint8_t handle, uint8_t reason)
{
	int ret;

	BK_LOGI(TAG, "broadcast suspend handle=%u reason=%u\n", handle, reason);
	le_audio_audio_tx_stop();
	ret = bk_dm_bap_end_announcement(s_session);
	BK_LOGI(TAG, "end announcement session=%u ret=%d\n", s_session, ret);
	if (ret == BK_OK)
	{
		s_state = BSRC_ENDING;
	}
	else
	{
		bsrc_reset_session();
	}
}

static void bsrc_evt_thread(void *arg)
{
	(void)arg;

	while (1)
	{
		bsrc_evt_t evt = {0};
		int ret = BK_OK;

		if (rtos_pop_from_queue(&s_evt_queue, &evt, BEKEN_WAIT_FOREVER) != BK_OK)
		{
			continue;
		}

		switch (evt.type)
		{
		case BSRC_EVT_CMD_START:
			ret = bsrc_do_start();
			break;
		case BSRC_EVT_CMD_STOP:
			ret = bsrc_do_stop();
			break;
		case BSRC_EVT_ANNOUNCEMENT_DONE:
			bsrc_handle_announcement_done(evt.status);
			break;
		case BSRC_EVT_BROADCAST_STARTED:
			bsrc_handle_started(&evt.u.start);
			break;
		case BSRC_EVT_BROADCAST_SUSPENDED:
			bsrc_handle_suspended(evt.handle, evt.reason);
			break;
		case BSRC_EVT_ANNOUNCEMENT_ENDED:
			BK_LOGI(TAG, "end announcement done status=%lu\n", (unsigned long)evt.status);
			bsrc_reset_session();
			break;
		default:
			break;
		}

		bsrc_finish_cmd(&evt, ret);
	}
}

static int bsrc_fsm_init(void)
{
	int ret;

	if (s_evt_queue)
	{
		return BK_OK;
	}

	ret = rtos_init_queue(&s_evt_queue, "bsrc_evt", sizeof(bsrc_evt_t), BSRC_EVT_QUEUE_LEN);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = rtos_init_semaphore(&s_cmd_sem, 1);
	if (ret != BK_OK)
	{
		rtos_deinit_queue(&s_evt_queue);
		s_evt_queue = NULL;
		return ret;
	}

	ret = rtos_create_thread(&s_evt_thread, BSRC_THREAD_PRIO, "bsrc_fsm",
	                         (beken_thread_function_t)bsrc_evt_thread,
	                         BSRC_THREAD_STACK, NULL);
	if (ret != BK_OK)
	{
		rtos_deinit_semaphore(&s_cmd_sem);
		s_cmd_sem = NULL;
		rtos_deinit_queue(&s_evt_queue);
		s_evt_queue = NULL;
		return ret;
	}

	return BK_OK;
}

int broadcast_source_demo_init(void)
{
	static const bk_bap_source_callbacks_t source_cbs =
	{
		.setup_announcement_cb = bsrc_on_announcement,
		.end_announcement_cb = bsrc_on_end,
		.broadcast_start_cb = bsrc_on_start,
		.broadcast_suspend_cb = bsrc_on_suspend,
	};
	int ret;

	ret = bk_dm_bap_init();
	BK_LOGI(TAG, "bk_dm_bap_init ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	le_audio_audio_set_source(le_audio_source_tone());
	ret = bsrc_fsm_init();
	BK_LOGI(TAG, "bsrc_fsm_init ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bap_source_register(&source_cbs);
	BK_LOGI(TAG, "bk_dm_bap_source_register ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	return BK_OK;
}

int broadcast_source_demo_start(void)
{
	bsrc_evt_t evt = {0};

	evt.type = BSRC_EVT_CMD_START;
	return bsrc_call_event(&evt);
}

int broadcast_source_demo_stop(void)
{
	bsrc_evt_t evt = {0};

	evt.type = BSRC_EVT_CMD_STOP;
	return bsrc_call_event(&evt);
}

int broadcast_source_demo_set_broadcast_code(const uint8_t *code16)
{
	if (!code16)
	{
		s_bcode_set = 0;
		os_memset(s_bcode, 0, sizeof(s_bcode));
		BK_LOGI(TAG, "broadcast code cleared\n");
		return 0;
	}

	os_memcpy(s_bcode, code16, sizeof(s_bcode));
	s_bcode_set = 1;
	BK_LOGI(TAG, "broadcast code set\n");
	return 0;
}

const uint8_t *broadcast_source_demo_broadcast_code(void)
{
	return s_bcode_set ? s_bcode : NULL;
}
