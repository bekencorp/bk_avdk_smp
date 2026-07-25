#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Private bridge between the EtherMind GA VCP reference (appl_vcp.c) and the
 * public bk_dm_vcp adapter (service/dm/ble/le_audio/vcp/bk_dm_vcp.c).
 *
 * appl_le_audio_vcp_* : down-calls implemented in appl_vcp.c (call GA_vc_*).
 * bk_dm_vcp_internal_*: up-calls implemented in the adapter shim and invoked
 *                       from the appl_vcp.c GA callbacks.
 */

/* Internal volume state mirrored across the bridge. */
typedef struct
{
    uint8_t volume;
    uint8_t mute;
    uint8_t change_counter;
} bki_vcp_volume_state_t;

/* ---- down-calls (appl_vcp.c) ---- */
uint16_t appl_vcp_rd_reg_opt_service(uint8_t srvc_type);
uint32_t appl_le_audio_vcp_discover(uint8_t *addr, uint8_t addr_type);
uint32_t appl_le_audio_vcp_get_capabilities(void);
uint32_t appl_le_audio_vcp_read_volume_state(void);
uint32_t appl_le_audio_vcp_set_abs_volume(uint8_t volume);
uint32_t appl_le_audio_vcp_volume_up(uint8_t unmute);
uint32_t appl_le_audio_vcp_volume_down(uint8_t unmute);
uint32_t appl_le_audio_vcp_set_mute(uint8_t mute);
uint32_t appl_le_audio_vcp_vocs_set_offset(int16_t offset);
uint32_t appl_le_audio_vcp_aics_set_gain(int8_t gain);
uint32_t appl_le_audio_vcp_release(void);
uint32_t appl_le_audio_vcp_renderer_set_volume(uint8_t volume);
uint32_t appl_le_audio_vcp_renderer_set_mute(uint8_t mute);

/* ---- up-calls (bk_dm_vcp.c shim) ---- */
void bk_dm_vcp_internal_setup(uint16_t acl_handle, uint8_t status);
void bk_dm_vcp_internal_volume_state(uint16_t acl_handle, const bki_vcp_volume_state_t *state);
void bk_dm_vcp_internal_vocs_offset(uint16_t acl_handle, int16_t offset);
void bk_dm_vcp_internal_aics_state(uint16_t acl_handle, int8_t gain, uint8_t mute, uint8_t gain_mode);
void bk_dm_vcp_internal_cp_done(uint16_t acl_handle, uint8_t status);
void bk_dm_vcp_internal_renderer_volume_set(uint16_t acl_handle, uint8_t volume, uint8_t mute);

#ifdef __cplusplus
}
#endif
