#include "broadcast_sink_demo.h"
#include "../../common/audio.h"

#include <components/bluetooth/bk_assigned_numbers.h>
#include <components/bluetooth/bk_dm_bap.h>
#include <components/bluetooth/bk_dm_bass.h>
#include <components/bluetooth/bk_dm_gap_ble.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/os.h>

#define TAG "lea_bsink"
#define DEMO_MAX_SOURCES       8U
#define DEMO_PA_STATE_SYNCED   0x02U
#define BSINK_EVT_QUEUE_LEN    16U
#define BSINK_THREAD_PRIO      5U
#define BSINK_THREAD_STACK     4096U

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

enum
{
	BSINK_IDLE = 0,
	BSINK_SCANNING,
	BSINK_ASSOCIATED,
	BSINK_ENABLING,
	BSINK_STREAMING,
	BSINK_DISSOCIATING,
};

static uint8_t s_state = BSINK_IDLE;
static bk_bap_source_announce_data_t s_sources[DEMO_MAX_SOURCES];
static uint8_t s_source_count;
static uint8_t s_selected_bis = 1;
static uint8_t s_scan_enabled;
static uint16_t s_sync_handle = 0xFFFF;

static bk_bass_source_info_t s_bass_src;
static uint8_t s_bass_active;
static uint8_t s_bass_code[16];
static uint8_t s_bass_code_set;
static uint8_t s_delegator_adv;

static uint8_t s_bcode[16];
static uint8_t s_bcode_set;

typedef enum
{
	BSINK_EVT_ANNOUNCEMENT = 1,
	BSINK_EVT_ASSOCIATED,
	BSINK_EVT_BASE_CONFIG,
	BSINK_EVT_BIG_INFO,
	BSINK_EVT_BROADCAST_EVENT,
	BSINK_EVT_BASS_ADD_SOURCE,
	BSINK_EVT_BASS_SET_CODE,
	BSINK_EVT_BASS_REMOVE_SOURCE,
} bsink_evt_type_t;

typedef struct
{
	bsink_evt_type_t type;
	uint8_t source_id;
	uint32_t status;
	uint32_t event;
	uint8_t code[16];
	union
	{
		bk_bap_source_announce_data_t announce;
		bk_bap_source_associate_data_t associate;
		bk_bap_source_big_info_t big_info;
		bk_bass_source_info_t bass_src;
		struct
		{
			uint8_t data[128];
			uint16_t length;
		} base;
	} u;
} bsink_evt_t;

static beken_queue_t s_evt_queue;
static beken_thread_t s_evt_thread;

static void bsink_reset_sources(void)
{
	os_memset(s_sources, 0, sizeof(s_sources));
	s_source_count = 0;
	s_selected_bis = 1;
}

static uint8_t bsink_make_bis_list(uint32_t bis_sync, uint8_t *bis_list, uint8_t bis_list_size)
{
	uint8_t count = 0;
	uint8_t i;

	if (!bis_list || bis_list_size == 0U)
	{
		return 0;
	}

	if (bis_sync == 0U)
	{
		bis_list[0] = 0x01;
		return 1;
	}

	for (i = 0; i < 31U && count < bis_list_size; i++)
	{
		if (bis_sync & (1UL << i))
		{
			bis_list[count++] = (uint8_t)(i + 1U);
		}
	}

	if (count == 0U)
	{
		bis_list[0] = 0x01;
		count = 1U;
	}

	return count;
}

static const uint8_t *bsink_effective_broadcast_code(void)
{
	if (s_bass_code_set)
	{
		return s_bass_code;
	}
	return broadcast_sink_demo_broadcast_code();
}

static int bsink_source_index(const bk_bap_source_announce_data_t *source)
{
	uint8_t i;

	for (i = 0; i < s_source_count; i++)
	{
		if (s_sources[i].advertising_sid == source->advertising_sid &&
		    s_sources[i].address_type == source->address_type &&
		    os_memcmp(s_sources[i].address, source->address, sizeof(source->address)) == 0)
		{
			return i;
		}
	}

	return -1;
}

static void bsink_print_source(uint8_t index, const bk_bap_source_announce_data_t *source)
{
	BK_LOGI(TAG, "[%u] sid=%u addr=%02x:%02x:%02x:%02x:%02x:%02x type=%u rssi=%d bcast_id=0x%06lx\n",
	        index, source->advertising_sid,
	        source->address[5], source->address[4], source->address[3],
	        source->address[2], source->address[1], source->address[0],
	        source->address_type, source->rssi, (unsigned long)source->broadcast_id);
}

static int bsink_post_event(const bsink_evt_t *evt, uint32_t timeout_ms)
{
	if (!s_evt_queue || !evt)
	{
		return BK_FAIL;
	}

	return rtos_push_to_queue(&s_evt_queue, (void *)evt, timeout_ms);
}

static void bsink_on_lc3_data(bk_bap_iso_header_t *header, uint8_t *data, uint32_t length)
{
	le_audio_audio_rx_push_iso(header, data, length);
}

static void bsink_on_announcement(bk_bap_source_announce_data_t *source)
{
	bsink_evt_t evt = {0};

	if (!source)
	{
		return;
	}

	evt.type = BSINK_EVT_ANNOUNCEMENT;
	evt.u.announce = *source;
	(void)bsink_post_event(&evt, 0);
}

static void bsink_on_associate(bk_bap_source_associate_data_t *data)
{
	bsink_evt_t evt = {0};

	if (!data)
	{
		return;
	}

	evt.type = BSINK_EVT_ASSOCIATED;
	evt.u.associate = *data;
	(void)bsink_post_event(&evt, 0);
}

static void bsink_on_config(uint8_t *data, uint16_t length)
{
	bsink_evt_t evt = {0};

	evt.type = BSINK_EVT_BASE_CONFIG;
	if (data && length)
	{
		evt.u.base.length = (length > sizeof(evt.u.base.data)) ? sizeof(evt.u.base.data) : length;
		os_memcpy(evt.u.base.data, data, evt.u.base.length);
	}
	(void)bsink_post_event(&evt, 0);
}

static int bsink_associate_source(uint8_t source_index)
{
	int ret;

	if (source_index >= s_source_count)
	{
		BK_LOGW(TAG, "bad source index=%u count=%u\n", source_index, s_source_count);
		return -1;
	}

	ret = bk_dm_bap_broadcast_associate(&s_sources[source_index]);
	BK_LOGI(TAG, "associate source=%u ret=%d\n", source_index, ret);
	return ret;
}

static void bsink_enable(uint16_t sync_handle, uint32_t bis_sync)
{
	uint8_t bis_list[4];
	uint8_t bis_count;
	const uint8_t *code;
	int ret;

	if (s_state != BSINK_ASSOCIATED)
	{
		BK_LOGD(TAG, "enable ignored state=%u\n", s_state);
		return;
	}

	bis_count = bsink_make_bis_list(bis_sync, bis_list, sizeof(bis_list));
	code = bsink_effective_broadcast_code();
	s_sync_handle = sync_handle;
	ret = bk_dm_bap_broadcast_enable(sync_handle, (uint8_t *)code, bis_count, bis_list);
	BK_LOGI(TAG, "broadcast_enable sync=0x%04x bis=%u ret=%d\n", sync_handle, bis_list[0], ret);
	if (ret == 0)
	{
		s_state = BSINK_ENABLING;
	}
}

static void bsink_on_big_info(bk_bap_source_big_info_t *info)
{
	bsink_evt_t evt = {0};

	if (!info)
	{
		return;
	}

	evt.type = BSINK_EVT_BIG_INFO;
	evt.u.big_info = *info;
	(void)bsink_post_event(&evt, 0);
}

static void bsink_on_event(bk_bap_sink_cb_evt_t event, uint32_t status)
{
	bsink_evt_t evt = {0};

	evt.type = BSINK_EVT_BROADCAST_EVENT;
	evt.event = event;
	evt.status = status;
	(void)bsink_post_event(&evt, 0);
}

static void bsink_on_bass_control(uint16_t acl_handle, uint8_t opcode)
{
	BK_LOGI(TAG, "BASS control handle=0x%04x opcode=0x%02x\n", acl_handle, opcode);
}

static void bsink_on_add_source(uint16_t acl_handle, const bk_bass_source_info_t *info)
{
	bsink_evt_t evt = {0};

	(void)acl_handle;

	if (!info)
	{
		return;
	}
	evt.type = BSINK_EVT_BASS_ADD_SOURCE;
	evt.u.bass_src = *info;
	(void)bsink_post_event(&evt, 0);
}

static void bsink_on_set_broadcast_code(uint16_t acl_handle, uint8_t source_id, const uint8_t code[16])
{
	bsink_evt_t evt = {0};

	(void)acl_handle;

	if (!code)
	{
		return;
	}
	evt.type = BSINK_EVT_BASS_SET_CODE;
	evt.source_id = source_id;
	os_memcpy(evt.code, code, sizeof(evt.code));
	(void)bsink_post_event(&evt, 0);
}

static void bsink_on_remove_source(uint16_t acl_handle, uint8_t source_id)
{
	bsink_evt_t evt = {0};

	(void)acl_handle;
	evt.type = BSINK_EVT_BASS_REMOVE_SOURCE;
	evt.source_id = source_id;
	(void)bsink_post_event(&evt, 0);
}

static void bsink_handle_announcement(const bk_bap_source_announce_data_t *source)
{
	int index;

	if (!source || s_state != BSINK_SCANNING)
	{
		return;
	}

	index = bsink_source_index(source);
	if (index >= 0)
	{
		s_sources[index] = *source;
		return;
	}

	if (s_source_count >= DEMO_MAX_SOURCES)
	{
		return;
	}

	index = s_source_count++;
	s_sources[index] = *source;
	bsink_print_source((uint8_t)index, source);
	BK_LOGI(TAG, "next: ap_cmd le_audio sync %d 1\n", index);
}

static void bsink_handle_base_config(bsink_evt_t *evt)
{
	bk_bap_basic_audio_config_t config;

	BK_LOGI(TAG, "BASE len=%u\n", evt->u.base.length);
	if (evt->u.base.length &&
	    bk_dm_bap_decode_basic_audio_config(&config, evt->u.base.data, evt->u.base.length) == BK_OK)
	{
		BK_LOGI(TAG, "BASE parsed; BIS index is 1-based, e.g. ap_cmd le_audio sync 0 1\n");
	}
}

static void bsink_handle_big_info(const bk_bap_source_big_info_t *info)
{
	uint32_t bis_sync;

	if (!info || s_state != BSINK_ASSOCIATED)
	{
		return;
	}

	BK_LOGI(TAG, "BIGInfo sync=0x%04x num_bis=%u max_sdu=%u interval=%lu enc=%u\n",
	        info->sync_handle, info->num_bis, info->max_sdu,
	        (unsigned long)info->sdu_interval, info->encryption);
	for (uint8_t i = 1; i <= info->num_bis; i++)
	{
		BK_LOGI(TAG, "  BIS[%u]%s\n", i, i == s_selected_bis ? " selected" : "");
	}
	if (!s_bass_active && s_selected_bis > info->num_bis)
	{
		BK_LOGW(TAG, "selected BIS %u out of range, num_bis=%u\n", s_selected_bis, info->num_bis);
		return;
	}

	if (info->encryption && !bsink_effective_broadcast_code())
	{
		BK_LOGW(TAG, "encrypted broadcast; set code with ap_cmd le_audio broadcast_code <hex32>\n");
	}

	bis_sync = s_bass_active ? s_bass_src.bis_sync : (1UL << (s_selected_bis - 1U));
	bsink_enable(info->sync_handle, bis_sync);
}

static void bsink_handle_broadcast_event(bk_bap_sink_cb_evt_t event, uint32_t status)
{
	BK_LOGI(TAG, "broadcast event=%d status=%lu\n", event, (unsigned long)status);

	if (event == BK_BAP_SINK_ENABLE_CNF && status == 0)
	{
		s_scan_enabled = 0;
		s_state = BSINK_STREAMING;
		le_audio_audio_rx_start();
		if (s_bass_active)
		{
			(void)bk_dm_bass_delegator_set_pa_state(DEMO_PA_STATE_SYNCED);
		}
		BK_LOGI(TAG, "streaming; stop with ap_cmd le_audio stop\n");
	}
	else if (event == BK_BAP_SINK_SCAN_END && status == 0)
	{
		s_scan_enabled = 0;
		if (s_state == BSINK_SCANNING)
		{
			s_state = BSINK_IDLE;
		}
	}
	else if (event == BK_BAP_SINK_DISABLE_IND || event == BK_BAP_SINK_DISABLE_CNF)
	{
		le_audio_audio_rx_stop();
		if (s_sync_handle != 0xFFFF)
		{
			uint16_t handle = s_sync_handle;
			s_sync_handle = 0xFFFF;
			s_state = BSINK_DISSOCIATING;
			bk_dm_bap_broadcast_dissociate(handle);
		}
		else
		{
			s_state = BSINK_IDLE;
		}
	}
	else if (event == BK_BAP_SINK_DISSOCIATE_CNF)
	{
		s_state = BSINK_IDLE;
		s_bass_active = 0;
		s_bass_code_set = 0;
		bsink_reset_sources();
	}
}

static int bsink_do_scan(uint8_t on)
{
	int ret;

	if (!on)
	{
		if (s_scan_enabled)
		{
			ret = bk_dm_bap_broadcast_scan_stop();
			if (ret != BK_OK)
			{
				return ret;
			}
			s_scan_enabled = 0;
		}
		if (s_state == BSINK_SCANNING)
		{
			s_state = BSINK_IDLE;
		}
		return BK_OK;
	}

	if (s_state != BSINK_IDLE && s_state != BSINK_SCANNING)
	{
		BK_LOGW(TAG, "scan rejected, state=%u; stop first\n", s_state);
		return BK_FAIL;
	}

	bsink_reset_sources();
	ret = bk_dm_bap_broadcast_scan_start();
	if (ret == BK_OK)
	{
		s_scan_enabled = 1;
		s_state = BSINK_SCANNING;
	}
	BK_LOGI(TAG, "scan ret=%d\n", ret);
	return ret;
}

static void bsink_do_list(void)
{
	if (s_source_count == 0)
	{
		BK_LOGI(TAG, "no broadcast sources; run ap_cmd le_audio scan on\n");
		return;
	}

	for (uint8_t i = 0; i < s_source_count; i++)
	{
		bsink_print_source(i, &s_sources[i]);
	}
}

static int bsink_do_sync(uint8_t source_index, uint8_t bis_index)
{
	if (source_index >= s_source_count)
	{
		BK_LOGW(TAG, "bad source index=%u count=%u\n", source_index, s_source_count);
		return BK_FAIL;
	}
	if (bis_index == 0)
	{
		BK_LOGW(TAG, "BIS index is 1-based\n");
		return BK_FAIL;
	}

	s_selected_bis = bis_index;
	s_bass_active = 0;
	BK_LOGI(TAG, "sync source=%u bis=%u\n", source_index, bis_index);
	return bsink_associate_source(source_index);
}

static int bsink_do_delegator_adv(uint8_t on)
{
	int ret = bk_dm_bass_delegator_adv(on);

	BK_LOGI(TAG, "delegator adv %s ret=%d\n", on ? "on" : "off", ret);
	if (ret == BK_OK)
	{
		s_delegator_adv = on ? 1 : 0;
	}
	return ret;
}

static void bsink_do_stop(void)
{
	if (s_scan_enabled)
	{
		bk_dm_bap_broadcast_scan_stop();
		s_scan_enabled = 0;
	}
	if (s_state == BSINK_STREAMING || s_state == BSINK_ENABLING)
	{
		bk_dm_bap_broadcast_disable(s_sync_handle);
		return;
	}
	if (s_sync_handle != 0xFFFF)
	{
		bk_dm_bap_broadcast_dissociate(s_sync_handle);
		s_sync_handle = 0xFFFF;
	}
	if (s_delegator_adv)
	{
		bk_dm_bass_delegator_adv(0);
		s_delegator_adv = 0;
	}
	le_audio_audio_rx_stop();
	s_bass_active = 0;
	s_bass_code_set = 0;
	bsink_reset_sources();
	s_state = BSINK_IDLE;
}

static void bsink_handle_bass_add_source(const bk_bass_source_info_t *info)
{
	if (!info)
	{
		return;
	}
	if (s_state != BSINK_IDLE)
	{
		BK_LOGW(TAG, "Add Source ignored, busy state=%u\n", s_state);
		return;
	}

	s_bass_src = *info;
	s_bass_active = 1;
	s_state = BSINK_ASSOCIATED;
	BK_LOGI(TAG, "Assistant Add Source id=%u sid=%u bcast_id=0x%06lx bis_sync=0x%08lx pa_sync=%u\n",
	        info->source_id, info->adv_sid, (unsigned long)info->broadcast_id,
	        (unsigned long)info->bis_sync, info->pa_sync);

	if (info->pa_sync == 0)
	{
		BK_LOGI(TAG, "pa_sync=0; waiting for Assistant update\n");
		s_state = BSINK_IDLE;
		s_bass_active = 0;
	}
	else
	{
		BK_LOGI(TAG, "awaiting PAST from Assistant\n");
	}
}

static void bsink_handle_bass_set_code(uint8_t source_id, const uint8_t code[16])
{
	if (s_bass_active && source_id != s_bass_src.source_id)
	{
		BK_LOGW(TAG, "broadcast code ignored source_id=%u current=%u\n", source_id, s_bass_src.source_id);
		return;
	}

	os_memcpy(s_bass_code, code, sizeof(s_bass_code));
	s_bass_code_set = 1;
	BK_LOGI(TAG, "broadcast code received via BASS source_id=%u\n", source_id);
}

static void bsink_handle_bass_remove_source(uint8_t source_id)
{
	BK_LOGI(TAG, "Assistant Remove Source id=%u state=%u\n", source_id, s_state);
	s_bass_active = 0;
	if (s_state == BSINK_STREAMING || s_state == BSINK_ENABLING)
	{
		bk_dm_bap_broadcast_disable(s_sync_handle);
	}
	else if (s_sync_handle != 0xFFFF)
	{
		bk_dm_bap_broadcast_dissociate(s_sync_handle);
		s_sync_handle = 0xFFFF;
		s_state = BSINK_IDLE;
	}
}

static void bsink_evt_thread(void *arg)
{
	(void)arg;

	while (1)
	{
		bsink_evt_t evt = {0};

		if (rtos_pop_from_queue(&s_evt_queue, &evt, BEKEN_WAIT_FOREVER) != BK_OK)
		{
			continue;
		}

		switch (evt.type)
		{
		case BSINK_EVT_ANNOUNCEMENT:
			bsink_handle_announcement(&evt.u.announce);
			break;
		case BSINK_EVT_ASSOCIATED:
			s_sync_handle = evt.u.associate.handle;
			s_state = BSINK_ASSOCIATED;
			BK_LOGI(TAG, "PA associated handle=0x%04x\n", evt.u.associate.handle);
			if (s_bass_active)
			{
				/* Scan Delegator / PAST path: the PA sync arrived via PAST
				 * from the Assistant. Here the controller only attaches
				 * BIGInfo as ACAD while BIG create-sync is pending, so no
				 * separate BIGInfo event follows - drive broadcast_enable
				 * directly using the bis_sync the Assistant asked for. */
				bsink_enable(evt.u.associate.handle, s_bass_src.bis_sync);
			}
			else
			{
				/* Normal scan path: keep scanning and wait for the BIGInfo
				 * event (it carries num_bis) before enabling. Stopping scan
				 * here would leave the scan-disable HCI command in flight and
				 * BIG create-sync (same GA context) would be rejected. */
				BK_LOGI(TAG, "waiting BIG info\n");
			}
			break;
		case BSINK_EVT_BASE_CONFIG:
			bsink_handle_base_config(&evt);
			break;
		case BSINK_EVT_BIG_INFO:
			bsink_handle_big_info(&evt.u.big_info);
			break;
		case BSINK_EVT_BROADCAST_EVENT:
			bsink_handle_broadcast_event((bk_bap_sink_cb_evt_t)evt.event, evt.status);
			break;
		case BSINK_EVT_BASS_ADD_SOURCE:
			bsink_handle_bass_add_source(&evt.u.bass_src);
			break;
		case BSINK_EVT_BASS_SET_CODE:
			bsink_handle_bass_set_code(evt.source_id, evt.code);
			break;
		case BSINK_EVT_BASS_REMOVE_SOURCE:
			bsink_handle_bass_remove_source(evt.source_id);
			break;
		default:
			break;
		}
	}
}

static int bsink_fsm_init(void)
{
	int ret;

	if (s_evt_queue)
	{
		return BK_OK;
	}

	ret = rtos_init_queue(&s_evt_queue, "bsink_evt", sizeof(bsink_evt_t), BSINK_EVT_QUEUE_LEN);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = rtos_create_thread(&s_evt_thread, BSINK_THREAD_PRIO, "bsink_fsm",
	                         (beken_thread_function_t)bsink_evt_thread,
	                         BSINK_THREAD_STACK, NULL);
	if (ret != BK_OK)
	{
		rtos_deinit_queue(&s_evt_queue);
		s_evt_queue = NULL;
		return ret;
	}

	return BK_OK;
}

int broadcast_sink_demo_init(void)
{
	static const bk_bap_sink_callbacks_t sink_cbs =
	{
		.announcement_cb = bsink_on_announcement,
		.associate_cb = bsink_on_associate,
		.config_cb = bsink_on_config,
		.big_info_cb = bsink_on_big_info,
		.lc3_data_cb = bsink_on_lc3_data,
		.broadcast_event_cb = bsink_on_event,
	};
	static const bk_bass_server_callbacks_t server_cbs =
	{
		.on_control = bsink_on_bass_control,
		.on_add_source = bsink_on_add_source,
		.on_set_broadcast_code = bsink_on_set_broadcast_code,
		.on_remove_source = bsink_on_remove_source,
	};
	int ret;

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

	ret = bsink_fsm_init();
	BK_LOGI(TAG, "bsink_fsm_init ret=%d\n", ret);
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

	ret = bk_dm_bap_sink_register(&sink_cbs);
	BK_LOGI(TAG, "bk_dm_bap_sink_register ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bass_init();
	BK_LOGI(TAG, "bk_dm_bass_init ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bass_server_init();
	BK_LOGI(TAG, "bk_dm_bass_server_init ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_dm_bass_server_register(&server_cbs);
	BK_LOGI(TAG, "bk_dm_bass_server_register ret=%d\n", ret);
	if (ret != BK_OK)
	{
		return ret;
	}

	BK_LOGI(TAG, "BASS Scan Delegator registered\n");
	return BK_OK;
}

int broadcast_sink_demo_scan(uint8_t on)
{
	return bsink_do_scan(on ? 1 : 0);
}

void broadcast_sink_demo_list_sources(void)
{
	bsink_do_list();
}

int broadcast_sink_demo_sync(uint8_t source_index, uint8_t bis_index)
{
	return bsink_do_sync(source_index, bis_index);
}

int broadcast_sink_demo_delegator_adv(uint8_t on)
{
	return bsink_do_delegator_adv(on ? 1 : 0);
}

void broadcast_sink_demo_stop(void)
{
	bsink_do_stop();
}

int broadcast_sink_demo_set_broadcast_code(const uint8_t *code16)
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

const uint8_t *broadcast_sink_demo_broadcast_code(void)
{
	return s_bcode_set ? s_bcode : NULL;
}
