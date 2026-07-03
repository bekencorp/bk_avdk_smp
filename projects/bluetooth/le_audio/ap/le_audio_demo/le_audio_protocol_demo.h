#pragma once

#include <stdint.h>
#include <components/bluetooth/bk_dm_bap_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	LE_AUDIO_DEMO_ROLE_SINK = 0,
	LE_AUDIO_DEMO_ROLE_SOURCE = 1,
} le_audio_demo_role_t;

int le_audio_demo_init(void);
int le_audio_broadcast_init(void);
int le_audio_sink_core_init(void);
int le_audio_demo_set_role(le_audio_demo_role_t role);
le_audio_demo_role_t le_audio_demo_get_role(void);
int le_audio_demo_broadcast_start(void);
int le_audio_demo_broadcast_stop(void);
int le_audio_broadcast_sink_scan(void);
int le_audio_broadcast_sink_sync(uint32_t broadcast_id);
void le_audio_broadcast_sink_stop(void);
int le_audio_demo_stop(void);
int le_audio_unicast_sink_receiver_start_ready(uint8_t ase_id);
int le_audio_unicast_sink_release(uint8_t ase_id);
int le_audio_unicast_tone_start(uint16_t connection_handle);
int le_audio_unicast_tone_stop(void);
void le_audio_demo_unicast_ase_discovered_cb(bk_bap_unicast_ase_discovered_t *info);
void le_audio_demo_unicast_cis_handle_assigned_cb(bk_bap_unicast_cis_info_t *info);
void le_audio_demo_unicast_cis_request_cb(bk_bap_unicast_cis_info_t *info);
void le_audio_demo_unicast_cis_established_cb(bk_bap_unicast_cis_info_t *info);
void le_audio_demo_unicast_iso_path_ready_cb(bk_bap_unicast_iso_path_t *info);

#ifdef __cplusplus
}
#endif
