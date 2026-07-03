#include "le_audio_protocol_demo.h"
#include "../common/le_audio_sink_media.h"

#include <components/bluetooth/bk_dm_bap.h>
#include <components/log.h>
#include <os/mem.h>

#define TAG "lea_broadcast_sink"

static bk_bap_source_announce_data_t s_last_source;
static uint8_t s_has_source;
static uint8_t s_scan_enabled;
static uint32_t s_announcement_count;

/* sync_handle of the BIG we have enabled (BIG_CREATE_SYNC), 0xFFFF = none. */
static uint16_t s_enabled_sync_handle = 0xFFFF;

void le_audio_broadcast_sink_announcement_cb(bk_bap_source_announce_data_t *source)
{
	uint8_t is_new_source;

	if (!source)
	{
		return;
	}

	is_new_source = (!s_has_source ||
	                 (s_last_source.advertising_sid != source->advertising_sid) ||
	                 (s_last_source.address_type != source->address_type) ||
	                 (0 != os_memcmp(s_last_source.address, source->address, sizeof(source->address))));

	os_memcpy(&s_last_source, source, sizeof(s_last_source));
	s_has_source = 1;
	s_announcement_count++;

	if (is_new_source || ((s_announcement_count % 50U) == 0U))
	{
		BK_LOGI(TAG, "announcement sid=%u addr=%02x:%02x:%02x:%02x:%02x:%02x rssi=%d%s\n",
		        source->advertising_sid,
		        source->address[5], source->address[4], source->address[3],
		        source->address[2], source->address[1], source->address[0],
		        source->rssi,
		        is_new_source ? "" : " repeat");
	}
}

void le_audio_broadcast_sink_associate_cb(bk_bap_source_associate_data_t *data)
{
	BK_LOGI(TAG, "associate handle=0x%04x\n", data ? data->handle : 0);
}

void le_audio_broadcast_sink_config_cb(uint8_t *data, uint16_t length)
{
	BK_LOGI(TAG, "basic audio config len=%u data=%p\n", length, data);
}

void le_audio_broadcast_sink_big_info_cb(bk_bap_source_big_info_t *info)
{
	if (!info)
	{
		return;
	}

	BK_LOGI(TAG, "big info sync=0x%04x num_bis=%u max_sdu=%u interval=%lu enc=%u\n",
	        info->sync_handle, info->num_bis, info->max_sdu, info->sdu_interval, info->encryption);

	/* Issue BIG_CREATE_SYNC so we actually start receiving BIS ISO data.
	 * Without this step the device only syncs to the periodic advertising and
	 * the shared sink LC3 callback never fires. We select BIS index 1 (1-based)
	 * which is enough for a single-stream broadcaster. */
	if (s_enabled_sync_handle != 0xFFFF)
	{
		/* Already enabled for a previous BIG-info report; ignore repeats. */
		return;
	}

	if (info->encryption)
	{
		/* Encrypted broadcast needs the 16-byte broadcast code. The demo joins
		 * open (unencrypted) broadcasts only; passing NULL here will not decrypt
		 * the stream. Wire a real code if the source advertises encryption. */
		BK_LOGW(TAG, "broadcast is encrypted; joining without a broadcast code may fail\n");
	}

	{
		uint8_t bis_count = 1;
		uint8_t bis_list[1] = { 0x01 };
		int en = bk_dm_bap_broadcast_enable(info->sync_handle, NULL, bis_count, bis_list);

		BK_LOGI(TAG, "broadcast_enable sync=0x%04x bis=%u ret=%d\n",
		        info->sync_handle, bis_count, en);

		if (en == 0)
		{
			s_enabled_sync_handle = info->sync_handle;
		}
	}
}

void le_audio_broadcast_sink_event_cb(bk_bap_sink_cb_evt_t event, uint32_t status)
{
	BK_LOGI(TAG, "broadcast event=%d status=%lu\n", event, status);

	if (event == BK_BAP_SINK_ENABLE_CNF && status == 0)
	{
		/* Heavy playback setup runs on lea_sink_media, not EtherMind WT. */
		le_audio_sink_media_start();
		s_scan_enabled = 0;
	}
	else if (event == BK_BAP_SINK_SCAN_END && status == 0)
	{
		s_scan_enabled = 0;
	}
}

int le_audio_broadcast_sink_scan(void)
{
	int ret;

	if (le_audio_demo_get_role() != LE_AUDIO_DEMO_ROLE_SINK)
	{
		BK_LOGW(TAG, "scan requested while not in sink role\n");
		return -1;
	}

	s_has_source = 0;
	s_scan_enabled = 0;
	s_announcement_count = 0;
	s_enabled_sync_handle = 0xFFFF;
	le_audio_sink_media_stop();
	ret = bk_dm_bap_broadcast_scan_start();
	if (ret == 0)
	{
		s_scan_enabled = 1;
	}

	return ret;
}

int le_audio_broadcast_sink_sync(uint32_t broadcast_id)
{
	if (le_audio_demo_get_role() != LE_AUDIO_DEMO_ROLE_SINK)
	{
		BK_LOGW(TAG, "sync requested while not in sink role\n");
		return -1;
	}

	if (!s_has_source)
	{
		BK_LOGW(TAG, "no source discovered yet, broadcast_id=0x%08lx\n", broadcast_id);
		return -1;
	}

	BK_LOGI(TAG, "associate last source, requested broadcast_id=0x%08lx\n", broadcast_id);
	return bk_dm_bap_broadcast_associate(&s_last_source);
}

void le_audio_broadcast_sink_stop(void)
{
	if (s_enabled_sync_handle != 0xFFFF)
	{
		bk_dm_bap_broadcast_disable(s_enabled_sync_handle);
		bk_dm_bap_broadcast_dissociate(s_enabled_sync_handle);
		s_enabled_sync_handle = 0xFFFF;
	}

	bk_dm_bap_broadcast_scan_stop();
	s_scan_enabled = 0;
	le_audio_sink_media_stop();
}
