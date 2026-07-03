#include "le_audio_protocol_demo.h"
#include "../common/le_audio_sink_media.h"

#include <components/bluetooth/bk_dm_bap.h>
#include <components/log.h>

#define TAG "lea_unicast_sink"

int le_audio_unicast_sink_receiver_start_ready(uint8_t ase_id)
{
	BK_LOGI(TAG, "receiver start ready ase=%u\n", ase_id);
	return bk_dm_bap_unicast_receiver_start_ready(ase_id);
}

int le_audio_unicast_sink_release(uint8_t ase_id)
{
	int ret;

	BK_LOGI(TAG, "release ase=%u\n", ase_id);
	ret = bk_dm_bap_unicast_release(ase_id);
	if (ret == BK_OK)
	{
		le_audio_sink_media_stop();
	}

	return ret;
}
