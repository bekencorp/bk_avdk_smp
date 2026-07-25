#include "unicast_server_demo.h"
#include "../../common/audio.h"

#include <components/bluetooth/bk_assigned_numbers.h>
#include <components/bluetooth/bk_dm_bap.h>
#include <components/bluetooth/bk_dm_gap_ble.h>
#include <components/log.h>
#include <os/os.h>

#define TAG "lea_usrv"
#define USRV_EVT_QUEUE_LEN      12U
#define USRV_THREAD_PRIO        5U
#define USRV_THREAD_STACK       4096U
#define USRV_CMD_TIMEOUT_MS     5000U

static const bk_bap_pacs_cfg_t s_sink_pacs_cfg =
{
	.supported_contexts = BK_GAP_DEFAULT_CONTEXTS,
	.available_contexts = BK_GAP_DEFAULT_CONTEXTS,
	.audio_location = BK_BT_AUDIO_LOCATION_FRONT_LEFT,
	.supported_sampling_frequencies = BK_BAP_LC3_CAP_FREQ_48KHZ,
	.supported_frame_durations = BK_BAP_LC3_CAP_DURATION_10MS,
	.supported_channel_counts = BK_BAP_LC3_CAP_CHANNEL_COUNT_1,
	.frame_octets_min = 100,
	.frame_octets_max = 120,
	.max_codec_frames_per_sdu = 1,
};

static const bk_bap_ascs_cfg_t s_sink_ascs_cfg =
{
	.ase_count = 1,
	.pref_framing = BK_BAP_QOS_FRAMING_UNFRAMED,
	.pref_phy = BK_BAP_QOS_PHY_1M,
	.pref_max_transport_latency = BK_BAP_QOS_LATENCY_10MS,
	.pref_presentation_delay_min = BK_BAP_QOS_PRESENTATION_DELAY_0US,
	.pref_presentation_delay_max = BK_BAP_QOS_PRESENTATION_DELAY_40MS,
	.pref_retransmission_number = BK_BAP_QOS_RETRANSMISSION_2,
	.supported_presentation_delay_min = BK_BAP_QOS_PRESENTATION_DELAY_0US,
	.supported_presentation_delay_max = BK_BAP_QOS_PRESENTATION_DELAY_40MS,
};

static uint8_t s_last_ase;
static uint16_t s_last_cis_handle;
static uint8_t s_adv_on;

typedef enum
{
	USRV_EVT_CMD_ADV = 1,
	USRV_EVT_CMD_RX_READY,
	USRV_EVT_CMD_RELEASE,
	USRV_EVT_CIS_REQUEST,
	USRV_EVT_CIS_ESTABLISHED,
	USRV_EVT_ISO_PATH_READY,
	USRV_EVT_READY,
} usrv_evt_type_t;

typedef struct
{
	usrv_evt_type_t type;
	uint8_t sync;
	uint8_t on;
	uint8_t ase_id;
	union
	{
		bk_bap_unicast_cis_info_t cis;
		bk_bap_unicast_iso_path_t iso;
		bk_bap_unicast_ready_t ready;
	} u;
} usrv_evt_t;

static beken_queue_t s_evt_queue;
static beken_thread_t s_evt_thread;
static beken_semaphore_t s_cmd_sem;
static int s_cmd_result;

static int usrv_post_event(const usrv_evt_t *evt, uint32_t timeout_ms)
{
	if (!s_evt_queue || !evt)
	{
		return BK_FAIL;
	}
	return rtos_push_to_queue(&s_evt_queue, (void *)evt, timeout_ms);
}

static int usrv_call_event(usrv_evt_t *evt)
{
	int ret;

	if (!evt)
	{
		return BK_FAIL;
	}
	evt->sync = 1;
	s_cmd_result = BK_FAIL;
	ret = usrv_post_event(evt, BEKEN_WAIT_FOREVER);
	if (ret != BK_OK)
	{
		return ret;
	}
	if (rtos_get_semaphore(&s_cmd_sem, USRV_CMD_TIMEOUT_MS) != BK_OK)
	{
		return BK_FAIL;
	}
	return s_cmd_result;
}

static void usrv_finish_cmd(const usrv_evt_t *evt, int ret)
{
	if (evt && evt->sync)
	{
		s_cmd_result = ret;
		(void)rtos_set_semaphore(&s_cmd_sem);
	}
}

static void usrv_set_security(void)
{
	uint8_t iocap = BK_IO_CAP_NONE;
	uint8_t authreq = BK_LE_AUTH_REQ_SC_BOND;

	(void)bk_ble_gap_set_security_param(BK_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
	(void)bk_ble_gap_set_security_param(BK_BLE_SM_AUTHEN_REQ_MODE, &authreq, sizeof(authreq));
	BK_LOGI(TAG, "security: iocap=NONE authreq=SC|BOND\n");
}

static void usrv_on_lc3_data(bk_bap_iso_header_t *header, uint8_t *data, uint32_t length)
{
	le_audio_audio_rx_push_iso(header, data, length);
}

static void usrv_on_ase_discovered(bk_bap_unicast_ase_discovered_t *info)
{
	if (!info)
	{
		return;
	}
	BK_LOGI(TAG, "ase_discovered ase=0x%02x role=%u state=0x%02x acl=0x%04x\n",
	        info->ase_id, info->ase_role, info->ase_state, info->acl_handle);
}

static void usrv_on_cis_request(bk_bap_unicast_cis_info_t *info)
{
	usrv_evt_t evt = {0};

	if (!info)
	{
		return;
	}
	evt.type = USRV_EVT_CIS_REQUEST;
	evt.u.cis = *info;
	(void)usrv_post_event(&evt, 0);
}

static void usrv_on_cis_established(bk_bap_unicast_cis_info_t *info)
{
	usrv_evt_t evt = {0};

	if (!info)
	{
		return;
	}
	evt.type = USRV_EVT_CIS_ESTABLISHED;
	evt.u.cis = *info;
	(void)usrv_post_event(&evt, 0);
}

static void usrv_on_iso_path_ready(bk_bap_unicast_iso_path_t *info)
{
	usrv_evt_t evt = {0};

	if (!info)
	{
		return;
	}
	evt.type = USRV_EVT_ISO_PATH_READY;
	evt.u.iso = *info;
	(void)usrv_post_event(&evt, 0);
}

static void usrv_on_ready(bk_bap_unicast_ready_t *info)
{
	usrv_evt_t evt = {0};

	if (!info)
	{
		return;
	}
	evt.type = USRV_EVT_READY;
	evt.u.ready = *info;
	(void)usrv_post_event(&evt, 0);
}

/* Idempotent advertising control. Re-enabling an already-active adv set makes
 * the host remove the adv handle (Command Disallowed 0x0C), which silently
 * kills advertising. The server auto-advertises at init, so guard here. */
static int usrv_do_adv(uint8_t on)
{
	int ret;

	if (on && s_adv_on)
	{
		BK_LOGI(TAG, "adv already on\n");
		return BK_OK;
	}
	if (!on && !s_adv_on)
	{
		BK_LOGI(TAG, "adv already off\n");
		return BK_OK;
	}

	ret = bk_dm_bap_unicast_adv(on ? 1 : 0);
	BK_LOGI(TAG, "adv %s ret=%d\n", on ? "on" : "off", ret);
	if (ret == BK_OK)
	{
		s_adv_on = on ? 1 : 0;
	}
	return ret;
}

static int usrv_do_rx_ready(uint8_t ase_id)
{
	int ret;

	BK_LOGI(TAG, "receiver start ready ase=%u\n", ase_id);
	ret = bk_dm_bap_unicast_receiver_start_ready(ase_id);
	if (ret == BK_OK)
	{
		le_audio_audio_rx_start();
	}
	return ret;
}

static int usrv_do_release(uint8_t ase_id)
{
	int ret;

	BK_LOGI(TAG, "release ase=%u\n", ase_id);
	ret = bk_dm_bap_unicast_release(ase_id);
	if (ret == BK_OK)
	{
		le_audio_audio_rx_stop();
	}
	return ret;
}

static void usrv_evt_thread(void *arg)
{
	(void)arg;

	while (1)
	{
		usrv_evt_t evt = {0};
		int ret = BK_OK;

		if (rtos_pop_from_queue(&s_evt_queue, &evt, BEKEN_WAIT_FOREVER) != BK_OK)
		{
			continue;
		}

		switch (evt.type)
		{
		case USRV_EVT_CMD_ADV:
			ret = usrv_do_adv(evt.on);
			break;
		case USRV_EVT_CMD_RX_READY:
			ret = usrv_do_rx_ready(evt.ase_id);
			break;
		case USRV_EVT_CMD_RELEASE:
			ret = usrv_do_release(evt.ase_id);
			break;
		case USRV_EVT_CIS_REQUEST:
			s_last_ase = evt.u.cis.ase_id;
			s_last_cis_handle = evt.u.cis.local_cis_handle;
			BK_LOGI(TAG, "cis_request ase=0x%02x cig=0x%02x cis=0x%02x cis_handle=0x%04x\n",
			        evt.u.cis.ase_id, evt.u.cis.cig_id, evt.u.cis.cis_id, evt.u.cis.local_cis_handle);
			break;
		case USRV_EVT_CIS_ESTABLISHED:
			s_last_ase = evt.u.cis.ase_id;
			s_last_cis_handle = evt.u.cis.local_cis_handle;
			BK_LOGI(TAG, "cis_established ase=0x%02x cis_handle=0x%04x\n",
			        evt.u.cis.ase_id, evt.u.cis.local_cis_handle);
			break;
		case USRV_EVT_ISO_PATH_READY:
			s_last_ase = evt.u.iso.ase_id;
			s_last_cis_handle = evt.u.iso.local_cis_handle;
			BK_LOGI(TAG, "iso_path_ready ase=0x%02x cis_handle=0x%04x -> auto rx_ready\n",
			        evt.u.iso.ase_id, evt.u.iso.local_cis_handle);
			(void)usrv_do_rx_ready(evt.u.iso.ase_id);
			break;
		case USRV_EVT_READY:
			if (evt.u.ready.phase == BK_BAP_UNICAST_READY_ACL_CONNECTED)
			{
				/* Controller auto-terminates connectable adv on connect. */
				s_adv_on = 0;
			}
			else if (evt.u.ready.phase == BK_BAP_UNICAST_READY_ACL_DISCONNECTED)
			{
				BK_LOGI(TAG, "peer disconnected, stopping playback\n");
				le_audio_audio_rx_stop();
				s_adv_on = 0;
				BK_LOGI(TAG, "re-enable advertising\n");
				(void)usrv_do_adv(1);
			}
			break;
		default:
			break;
		}

		usrv_finish_cmd(&evt, ret);
	}
}

static int usrv_fsm_init(void)
{
	int ret;

	if (s_evt_queue)
	{
		return BK_OK;
	}

	ret = rtos_init_queue(&s_evt_queue, "usrv_evt", sizeof(usrv_evt_t), USRV_EVT_QUEUE_LEN);
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

	ret = rtos_create_thread(&s_evt_thread, USRV_THREAD_PRIO, "usrv_fsm",
	                         (beken_thread_function_t)usrv_evt_thread,
	                         USRV_THREAD_STACK, NULL);
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

int unicast_server_demo_init(void)
{
	static const bk_bap_sink_callbacks_t sink_cbs =
	{
		.lc3_data_cb = usrv_on_lc3_data,
		.unicast_ase_discovered_cb = usrv_on_ase_discovered,
		.unicast_cis_request_cb = usrv_on_cis_request,
		.unicast_cis_established_cb = usrv_on_cis_established,
		.unicast_iso_path_ready_cb = usrv_on_iso_path_ready,
		.unicast_ready_cb = usrv_on_ready,
	};
	int ret;

	(void)s_last_ase;
	(void)s_last_cis_handle;
	usrv_set_security();
	ret = bk_dm_bap_init();
	BK_LOGI(TAG, "bk_dm_bap_init ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	le_audio_audio_set_sink(le_audio_playback_sink());
	ret = le_audio_audio_rx_init();
	BK_LOGI(TAG, "le_audio_audio_rx_init ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = usrv_fsm_init();
	BK_LOGI(TAG, "usrv_fsm_init ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bap_pacs_register(BK_GAP_ROLE_SINK, &s_sink_pacs_cfg);
	BK_LOGI(TAG, "bk_dm_bap_pacs_register sink ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bap_ascs_register(BK_GAP_ROLE_SINK, &s_sink_ascs_cfg);
	BK_LOGI(TAG, "bk_dm_bap_ascs_register sink ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bap_sink_register(&sink_cbs);
	BK_LOGI(TAG, "bk_dm_bap_sink_register ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = usrv_do_adv(1);
	BK_LOGI(TAG, "initial adv on ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	return BK_OK;
}

int unicast_server_demo_adv(uint8_t on)
{
	usrv_evt_t evt = {0};

	evt.type = USRV_EVT_CMD_ADV;
	evt.on = on ? 1 : 0;
	return usrv_call_event(&evt);
}

int unicast_server_demo_rx_ready(uint8_t ase_id)
{
	usrv_evt_t evt = {0};

	evt.type = USRV_EVT_CMD_RX_READY;
	evt.ase_id = ase_id;
	return usrv_call_event(&evt);
}

int unicast_server_demo_release(uint8_t ase_id)
{
	usrv_evt_t evt = {0};

	evt.type = USRV_EVT_CMD_RELEASE;
	evt.ase_id = ase_id;
	return usrv_call_event(&evt);
}
