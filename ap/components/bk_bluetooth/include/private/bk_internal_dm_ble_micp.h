#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Private bridge between the EtherMind GA MICP reference (appl_micp.c) and the
 * public bk_dm_micp adapter (service/dm/ble/le_audio/micp/bk_dm_micp.c).
 *
 * appl_le_audio_micp_* : down-calls implemented in appl_micp.c (call GA_mc_*).
 * bk_dm_micp_internal_*: up-calls implemented in the adapter shim and invoked
 *                        from the appl_micp.c GA callbacks.
 */

/* ---- down-calls (appl_micp.c) ---- */
uint16_t appl_micp_dev_reg_opt_service(void);
uint32_t appl_le_audio_micp_discover(uint8_t *addr, uint8_t addr_type);
uint32_t appl_le_audio_micp_get_capabilities(void);
uint32_t appl_le_audio_micp_read_mute(void);
uint32_t appl_le_audio_micp_set_mute(uint8_t mute);
uint32_t appl_le_audio_micp_aics_set_gain(int8_t gain);
uint32_t appl_le_audio_micp_release(void);
uint32_t appl_le_audio_micp_device_set_mute(uint8_t mute);

/* ---- up-calls (bk_dm_micp.c shim) ---- */
void bk_dm_micp_internal_setup(uint16_t acl_handle, uint8_t status);
void bk_dm_micp_internal_mute_state(uint16_t acl_handle, uint8_t mute);
void bk_dm_micp_internal_aics_state(uint16_t acl_handle, int8_t gain, uint8_t mute, uint8_t gain_mode);
void bk_dm_micp_internal_cp_done(uint16_t acl_handle, uint8_t status);
void bk_dm_micp_internal_device_mute_set(uint16_t acl_handle, uint8_t mute);
void bk_dm_micp_internal_device_aics_gain_set(uint16_t acl_handle, int8_t gain);

#ifdef __cplusplus
}
#endif
