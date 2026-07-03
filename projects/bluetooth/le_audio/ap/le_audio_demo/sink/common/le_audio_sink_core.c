#include "le_audio_protocol_demo.h"
#include "le_audio_sink_media.h"

#include <components/bluetooth/bk_dm_bap.h>
#include <components/log.h>

#define TAG "lea_sink_core"

void le_audio_broadcast_sink_announcement_cb(bk_bap_source_announce_data_t *source);
void le_audio_broadcast_sink_associate_cb(bk_bap_source_associate_data_t *data);
void le_audio_broadcast_sink_config_cb(uint8_t *data, uint16_t length);
void le_audio_broadcast_sink_big_info_cb(bk_bap_source_big_info_t *info);
void le_audio_broadcast_sink_event_cb(bk_bap_sink_cb_evt_t event, uint32_t status);

static uint8_t s_sink_core_registered;

static void le_audio_sink_lc3_data_cb(bk_bap_iso_header_t *header, uint8_t *data, uint32_t length)
{
	le_audio_sink_media_push_iso(header, data, length);
}

int le_audio_sink_core_init(void)
{
	static const bk_bap_sink_callbacks_t sink_cbs =
	{
		.announcement_cb = le_audio_broadcast_sink_announcement_cb,
		.associate_cb = le_audio_broadcast_sink_associate_cb,
		.config_cb = le_audio_broadcast_sink_config_cb,
		.big_info_cb = le_audio_broadcast_sink_big_info_cb,
		.lc3_data_cb = le_audio_sink_lc3_data_cb,
		.broadcast_event_cb = le_audio_broadcast_sink_event_cb,
		.unicast_ase_discovered_cb = le_audio_demo_unicast_ase_discovered_cb,
		.unicast_cis_request_cb = le_audio_demo_unicast_cis_request_cb,
		.unicast_cis_established_cb = le_audio_demo_unicast_cis_established_cb,
		.unicast_iso_path_ready_cb = le_audio_demo_unicast_iso_path_ready_cb,
	};
	int ret;

	ret = le_audio_sink_media_init();
	if (ret != BK_OK)
	{
		BK_LOGE(TAG, "sink media init failed ret=%d\n", ret);
		return ret;
	}

	if (s_sink_core_registered)
	{
		return BK_OK;
	}

	ret = bk_dm_bap_sink_register(&sink_cbs);
	BK_LOGI(TAG, "bk_dm_bap_sink_register ret=%d\n", ret);
	if (ret == BK_OK)
	{
		s_sink_core_registered = 1;
	}

	return ret;
}
