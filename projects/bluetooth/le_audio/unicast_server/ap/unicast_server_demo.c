#include "unicast_server_demo.h"
#include "../../common/audio.h"
#include "le_audio_user_config.h"

#include <components/bluetooth/bk_assigned_numbers.h>
#include <components/bluetooth/bk_dm_bap.h>
#include <components/bluetooth/bk_dm_gap_ble.h>
#if CONFIG_LE_AUDIO_MCP
#include <components/bluetooth/bk_dm_mcp.h>
#endif
#if CONFIG_LE_AUDIO_VCP
#include <components/bluetooth/bk_dm_vcp.h>
#endif
#include <components/log.h>
#include <os/mem.h>
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
	.audio_location = BK_BT_AUDIO_LOCATION_FRONT_LEFT | BK_BT_AUDIO_LOCATION_FRONT_RIGHT,
	.supported_sampling_frequencies = BK_BAP_LC3_CAP_FREQ_48KHZ,
	.supported_frame_durations = BK_BAP_LC3_CAP_DURATION_10MS,
	.supported_channel_counts = BK_BAP_LC3_CAP_CHANNEL_COUNT_1,
	.frame_octets_min = 100,
	.frame_octets_max = 120,
	.max_codec_frames_per_sdu = 1,
};

static const bk_bap_ascs_cfg_t s_sink_ascs_cfg =
{
	.ase_count = CONFIG_LE_AUDIO_STREAM_COUNT,
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
/* Streaming ASEs, in the order their ISO path came up: index i plays channel i */
static uint8_t s_rx_ase[LE_AUDIO_MAX_STREAMS];
static uint16_t s_rx_handle[LE_AUDIO_MAX_STREAMS];
static uint8_t s_rx_count;

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

static const char *usrv_ase_state_name(uint8_t state)
{
	switch (state)
	{
	case BK_BAP_UNICAST_STATE_IDLE:             return "idle";
	case BK_BAP_UNICAST_STATE_CODEC_CONFIGURED: return "codec_configured";
	case BK_BAP_UNICAST_STATE_QOS_CONFIGURED:   return "qos_configured";
	case BK_BAP_UNICAST_STATE_ENABLING:         return "enabling";
	case BK_BAP_UNICAST_STATE_STREAMING:        return "streaming";
	case BK_BAP_UNICAST_STATE_DISABLING:        return "disabling";
	case BK_BAP_UNICAST_STATE_RELEASING:        return "releasing";
	default:                                    return "unknown";
	}
}

static void usrv_on_ase_state(bk_bap_unicast_state_t *info)
{
	if (!info)
	{
		return;
	}
	BK_LOGI(TAG, "ase_state ase=0x%02x role=%u state=0x%02x(%s) acl=0x%04x\n",
	        info->ase_id, info->ase_role, info->state,
	        usrv_ase_state_name(info->state), info->acl_handle);
}

#if CONFIG_LE_AUDIO_VCP
/* The GA layer has already applied the value to VCS and answered the peer, so
 * this only has to follow it on the speaker. */
static void usrv_on_volume_set(uint16_t acl_handle, uint8_t volume, uint8_t mute)
{
	BK_LOGI(TAG, "vcp volume acl=0x%04x volume=%u mute=%u\n", acl_handle, volume, mute);
	le_audio_playback_set_volume(volume, mute);
}
#endif /* CONFIG_LE_AUDIO_VCP */

#if CONFIG_LE_AUDIO_MCP
/* Media control (play / pause / track change) is MCP. The media player lives on
 * the phone, so the headset is the MCP client. */
static const char *usrv_media_state_name(uint8_t state)
{
	switch (state)
	{
	case BK_MCP_MEDIA_STATE_INACTIVE: return "inactive";
	case BK_MCP_MEDIA_STATE_PLAYING:  return "playing";
	case BK_MCP_MEDIA_STATE_PAUSED:   return "paused";
	case BK_MCP_MEDIA_STATE_SEEKING:  return "seeking";
	default:                          return "unknown";
	}
}

static void usrv_mcp_on_setup(uint16_t acl_handle, uint8_t status)
{
	BK_LOGI(TAG, "mcp setup acl=0x%04x status=0x%02x\n", acl_handle, status);
	if (status == 0)
	{
		/* Subscribe so the peer's own play / pause / track changes reach us. */
		int ret = bk_dm_mcp_config_notify(acl_handle, 1);
		BK_LOGI(TAG, "mcp notify on ret=%d\n", ret);
	}
}

static void usrv_mcp_on_track_title(uint16_t acl_handle, const char *title, uint16_t len)
{
	BK_LOGI(TAG, "mcp track acl=0x%04x len=%u title=%.*s\n",
	        acl_handle, len, (int)len, title ? title : "");
}

static void usrv_mcp_on_media_state(uint16_t acl_handle, uint8_t state)
{
	BK_LOGI(TAG, "mcp media state acl=0x%04x state=0x%02x(%s)\n",
	        acl_handle, state, usrv_media_state_name(state));
}

static void usrv_mcp_on_track_position(uint16_t acl_handle, int32_t position)
{
	BK_LOGI(TAG, "mcp track position acl=0x%04x pos=%d\n", acl_handle, position);
}

static void usrv_mcp_event(uint16_t acl_handle, bk_mcp_cb_evt_t evt, uint8_t status)
{
	BK_LOGI(TAG, "mcp evt acl=0x%04x evt=%d status=0x%02x\n", acl_handle, evt, status);
}
#endif /* CONFIG_LE_AUDIO_MCP */

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

static void usrv_rx_reset(void)
{
	s_rx_count = 0;
	os_memset(s_rx_ase, 0, sizeof(s_rx_ase));
	os_memset(s_rx_handle, 0, sizeof(s_rx_handle));
}

/* Play every streaming ASE as its own channel, so two CISes give stereo */
static void usrv_rx_restart(void)
{
	le_audio_codec_cfg_t cfg;

	if (s_rx_count == 0U)
	{
		le_audio_audio_rx_stop();
		return;
	}

	le_audio_codec_cfg_default(&cfg);
	cfg.channels = s_rx_count;
	le_audio_audio_rx_start(s_rx_handle, s_rx_count, &cfg);
}

static void usrv_rx_add(uint8_t ase_id, uint16_t cis_handle)
{
	uint8_t i;

	if (cis_handle == 0U)
	{
		BK_LOGW(TAG, "ase=%u has no CIS handle yet\n", ase_id);
		return;
	}

	for (i = 0; i < s_rx_count; i++)
	{
		if (s_rx_ase[i] == ase_id)
		{
			s_rx_handle[i] = cis_handle;
			usrv_rx_restart();
			return;
		}
	}

	if (s_rx_count >= LE_AUDIO_MAX_STREAMS)
	{
		BK_LOGW(TAG, "ignoring ase=%u, already playing %u streams\n", ase_id, s_rx_count);
		return;
	}

	s_rx_ase[s_rx_count] = ase_id;
	s_rx_handle[s_rx_count] = cis_handle;
	s_rx_count++;
	usrv_rx_restart();
}

static void usrv_rx_remove(uint8_t ase_id)
{
	uint8_t i;

	for (i = 0; i < s_rx_count; i++)
	{
		if (s_rx_ase[i] != ase_id)
		{
			continue;
		}

		for (; (i + 1U) < s_rx_count; i++)
		{
			s_rx_ase[i] = s_rx_ase[i + 1U];
			s_rx_handle[i] = s_rx_handle[i + 1U];
		}
		s_rx_count--;
		usrv_rx_restart();
		return;
	}
}

static int usrv_do_rx_ready(uint8_t ase_id, uint16_t cis_handle)
{
	int ret;

	ret = bk_dm_bap_unicast_receiver_start_ready(ase_id);
	BK_LOGI(TAG, "receiver start ready ase=%u cis_handle=0x%04x ret=%d\n",
	        ase_id, cis_handle, ret);

	/*
	 * The stack already answers Receiver Start Ready when the ISO path is set
	 * up, so this request is often rejected as a duplicate. Play the stream
	 * anyway, the ISO data path is what actually matters here.
	 */
	usrv_rx_add(ase_id, cis_handle);
	return ret;
}

static int usrv_do_release(uint8_t ase_id)
{
	int ret;

	BK_LOGI(TAG, "release ase=%u\n", ase_id);
	ret = bk_dm_bap_unicast_release(ase_id);
	if (ret == BK_OK)
	{
		usrv_rx_remove(ase_id);
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
			ret = usrv_do_rx_ready(evt.ase_id, s_last_cis_handle);
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
			(void)usrv_do_rx_ready(evt.u.iso.ase_id, evt.u.iso.local_cis_handle);
			break;
		case USRV_EVT_READY:
			if (evt.u.ready.phase == BK_BAP_UNICAST_READY_ACL_CONNECTED)
			{
				/* Controller auto-terminates connectable adv on connect. */
				s_adv_on = 0;
			}
			else if (evt.u.ready.phase == BK_BAP_UNICAST_READY_ENCRYPTED)
			{
#if CONFIG_LE_AUDIO_MCP
				/* GMCS needs an authenticated link, so only look for it once
				 * encryption is up. Doing it on connect gets every read and
				 * CCCD write rejected with Insufficient Authentication. */
				ret = bk_dm_mcp_discover(evt.u.ready.addr, evt.u.ready.addr_type);
				BK_LOGI(TAG, "mcp discover ret=%d\n", ret);
#endif /* CONFIG_LE_AUDIO_MCP */
			}
			else if (evt.u.ready.phase == BK_BAP_UNICAST_READY_ACL_DISCONNECTED)
			{
				BK_LOGI(TAG, "peer disconnected, stopping playback\n");
				usrv_rx_reset();
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
		.unicast_state_cb = usrv_on_ase_state,
		.unicast_ready_cb = usrv_on_ready,
	};
#if CONFIG_LE_AUDIO_VCP
	static const bk_vcp_renderer_callbacks_t vcp_cbs =
	{
		.on_volume_set = usrv_on_volume_set,
	};
#endif
#if CONFIG_LE_AUDIO_MCP
	static const bk_mcp_client_callbacks_t mcp_cbs =
	{
		.on_setup = usrv_mcp_on_setup,
		.on_track_title = usrv_mcp_on_track_title,
		.on_media_state = usrv_mcp_on_media_state,
		.on_track_position = usrv_mcp_on_track_position,
		.event_cb = usrv_mcp_event,
	};
#endif
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

	/* The GA init done by bk_dm_bap_init already brings up VCS and registers
	 * VOCS/AICS, so the renderer role only has to hook the volume callback. */
#if CONFIG_LE_AUDIO_VCP
	ret = bk_dm_vcp_renderer_register(&vcp_cbs);
	BK_LOGI(TAG, "bk_dm_vcp_renderer_register ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}
#endif /* CONFIG_LE_AUDIO_VCP */

#if CONFIG_LE_AUDIO_MCP
	/* Unlike VCS, the MCP client entity is not brought up by GA_init, so it has
	 * to be initialised here or every setup fails with CNTX_ALLOC_FAILED. */
	ret = bk_dm_mcp_init();
	BK_LOGI(TAG, "bk_dm_mcp_init ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_mcp_client_register(&mcp_cbs);
	BK_LOGI(TAG, "bk_dm_mcp_client_register ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}
#endif /* CONFIG_LE_AUDIO_MCP */

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

#if CONFIG_LE_AUDIO_VCP
int unicast_server_demo_set_volume(uint8_t volume)
{
	int ret;

	/* Updates VCS and notifies the peer; the renderer callback then applies it
	 * to the speaker, so the local path stays identical to a remote change. */
	ret = bk_dm_vcp_renderer_set_volume(volume);
	BK_LOGI(TAG, "set volume %u ret=%d\n", volume, ret);
	return ret;
}

int unicast_server_demo_set_mute(uint8_t mute)
{
	int ret;

	ret = bk_dm_vcp_renderer_set_mute(mute ? BK_VCP_MUTE : BK_VCP_UNMUTE);
	BK_LOGI(TAG, "set mute %u ret=%d\n", mute, ret);
	return ret;
}
#endif /* CONFIG_LE_AUDIO_VCP */

#if CONFIG_LE_AUDIO_MCP
int unicast_server_demo_media_control(uint8_t opcode)
{
	int ret;

	/* acl_handle is unused by the MCP adapter, it tracks a single link. */
	ret = bk_dm_mcp_control(0U, opcode);
	BK_LOGI(TAG, "mcp control opcode=0x%02x ret=%d\n", opcode, ret);
	return ret;
}

int unicast_server_demo_media_notify(uint8_t enable)
{
	int ret;

	ret = bk_dm_mcp_config_notify(0U, enable);
	BK_LOGI(TAG, "mcp notify %u ret=%d\n", enable, ret);
	return ret;
}

int unicast_server_demo_media_read(void)
{
	int ret;

	/* Only one read may be outstanding; a second one returns IN_BUSY_STATE. */
	ret = bk_dm_mcp_read_media_state(0U);
	BK_LOGI(TAG, "mcp read media_state ret=%d\n", ret);
	return ret;
}
#endif /* CONFIG_LE_AUDIO_MCP */
