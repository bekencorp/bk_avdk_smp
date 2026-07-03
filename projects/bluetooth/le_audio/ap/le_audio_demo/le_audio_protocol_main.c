#include "le_audio_protocol_demo.h"

#include <components/bluetooth/bk_dm_bap.h>
#include <components/log.h>
#include <os/os.h>

#define TAG "lea_demo"

static le_audio_demo_role_t s_role =
#if CONFIG_LE_AUDIO_ROLE_SOURCE
	LE_AUDIO_DEMO_ROLE_SOURCE;
#else
	LE_AUDIO_DEMO_ROLE_SINK;
#endif

static uint8_t s_source_inited;
static uint8_t s_sink_inited;

static const char *le_audio_unicast_role_name(uint8_t role)
{
	switch (role)
	{
	case BK_GAP_ROLE_SINK:
		return "sink";
	case BK_GAP_ROLE_SOURCE:
		return "source";
	default:
		return "unknown";
	}
}

void le_audio_demo_unicast_ase_discovered_cb(bk_bap_unicast_ase_discovered_t *info)
{
	if (!info)
	{
		return;
	}

	BK_LOGI(TAG,
	        "unicast ase_discovered: ase_id=0x%02x role=%s state=0x%02x acl=0x%04x\n",
	        info->ase_id,
	        le_audio_unicast_role_name(info->ase_role),
	        info->ase_state,
	        info->acl_handle);
}

void le_audio_demo_unicast_cis_handle_assigned_cb(bk_bap_unicast_cis_info_t *info)
{
	if (!info)
	{
		return;
	}

	BK_LOGI(TAG,
	        "unicast cis_handle_assigned: ase_id=0x%02x role=%s cig_id=0x%02x cis_id=0x%02x acl=0x%04x local_cis_handle=0x%04x\n",
	        info->ase_id,
	        le_audio_unicast_role_name(info->ase_role),
	        info->cig_id,
	        info->cis_id,
	        info->acl_handle,
	        info->local_cis_handle);
}

void le_audio_demo_unicast_cis_request_cb(bk_bap_unicast_cis_info_t *info)
{
	if (!info)
	{
		return;
	}

	BK_LOGI(TAG,
	        "unicast cis_request: ase_id=0x%02x role=%s cig_id=0x%02x cis_id=0x%02x acl=0x%04x local_cis_handle=0x%04x\n",
	        info->ase_id,
	        le_audio_unicast_role_name(info->ase_role),
	        info->cig_id,
	        info->cis_id,
	        info->acl_handle,
	        info->local_cis_handle);
}

void le_audio_demo_unicast_cis_established_cb(bk_bap_unicast_cis_info_t *info)
{
	if (!info)
	{
		return;
	}

	BK_LOGI(TAG,
	        "unicast cis_established: ase_id=0x%02x role=%s cig_id=0x%02x cis_id=0x%02x acl=0x%04x local_cis_handle=0x%04x\n",
	        info->ase_id,
	        le_audio_unicast_role_name(info->ase_role),
	        info->cig_id,
	        info->cis_id,
	        info->acl_handle,
	        info->local_cis_handle);
}

void le_audio_demo_unicast_iso_path_ready_cb(bk_bap_unicast_iso_path_t *info)
{
	if (!info)
	{
		return;
	}

	BK_LOGI(TAG,
	        "unicast iso_path_ready: ase_id=0x%02x role=%s local_cis_handle=0x%04x\n",
	        info->ase_id,
	        le_audio_unicast_role_name(info->ase_role),
	        info->local_cis_handle);
}

static void le_audio_demo_init_role(le_audio_demo_role_t role)
{
	int ret;

	if (role == LE_AUDIO_DEMO_ROLE_SOURCE)
	{
		if (!s_source_inited)
		{
			ret = le_audio_broadcast_init();
			BK_LOGI(TAG, "source init ret=%d\n", ret);
			if (ret == 0)
			{
				s_source_inited = 1;
			}
		}
	}
	else
	{
		if (!s_sink_inited)
		{
			ret = le_audio_sink_core_init();
			BK_LOGI(TAG, "sink init ret=%d\n", ret);
			if (ret == 0)
			{
				s_sink_inited = 1;
			}
		}
	}
}

int le_audio_demo_init(void)
{
	int ret;

	BK_LOGI(TAG, "init role=%s\n", s_role == LE_AUDIO_DEMO_ROLE_SOURCE ? "source" : "sink");
	ret = bk_dm_bap_init();
	BK_LOGI(TAG, "bk_dm_bap_init ret=%d\n", ret);

	/* Register only the side matching the configured role; the other side can
	 * be brought up later via le_audio_demo_set_role(). */
	le_audio_demo_init_role(s_role);
	return 0;
}

int le_audio_demo_set_role(le_audio_demo_role_t role)
{
	s_role = role;
	BK_LOGI(TAG, "set role=%s\n", s_role == LE_AUDIO_DEMO_ROLE_SOURCE ? "source" : "sink");
	le_audio_demo_init_role(s_role);
	return 0;
}

le_audio_demo_role_t le_audio_demo_get_role(void)
{
	return s_role;
}

int le_audio_demo_stop(void)
{
	BK_LOGI(TAG, "stop current LE Audio activity (role=%s)\n",
	        s_role == LE_AUDIO_DEMO_ROLE_SOURCE ? "source" : "sink");

	if (s_role == LE_AUDIO_DEMO_ROLE_SOURCE)
	{
		le_audio_demo_broadcast_stop();
	}
	else
	{
		le_audio_broadcast_sink_stop();
	}

	return 0;
}
