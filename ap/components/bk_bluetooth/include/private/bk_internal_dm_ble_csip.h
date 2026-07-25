#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Private bridge between the EtherMind GA CSIP reference (appl_csip.c) and the
 * public bk_dm_csip adapter (service/dm/ble/le_audio/csip/bk_dm_csip.c).
 *
 * appl_le_audio_csip_* : down-calls implemented in appl_csip.c (call GA_cs_*).
 * bk_dm_csip_internal_*: up-calls implemented in the adapter shim and invoked
 *                        from the appl_csip.c GA callbacks.
 */

/* ---- down-calls (appl_csip.c) ---- */
uint32_t appl_le_audio_csip_discover(uint8_t *addr, uint8_t addr_type);
uint32_t appl_le_audio_csip_get_sirk(void);
uint32_t appl_le_audio_csip_get_setsize(void);
uint32_t appl_le_audio_csip_get_rank(void);
uint32_t appl_le_audio_csip_get_lock(void);
uint32_t appl_le_audio_csip_set_lock(uint8_t lock);
uint32_t appl_le_audio_csip_release(void);
uint32_t appl_le_audio_csip_member_configure(uint8_t sirk_type, const uint8_t *sirk_value,
                                             uint8_t size, uint8_t rank, uint8_t lock);

/* ---- up-calls (bk_dm_csip.c shim) ---- */
void bk_dm_csip_internal_setup(uint16_t acl_handle, uint8_t status);
void bk_dm_csip_internal_sirk(uint16_t acl_handle, uint8_t type, const uint8_t *value);
void bk_dm_csip_internal_setsize(uint16_t acl_handle, uint8_t size);
void bk_dm_csip_internal_rank(uint16_t acl_handle, uint8_t rank);
void bk_dm_csip_internal_lock_state(uint16_t acl_handle, uint8_t lock);
void bk_dm_csip_internal_cp_done(uint16_t acl_handle, uint8_t status);
void bk_dm_csip_internal_released(uint16_t acl_handle, uint8_t status);
void bk_dm_csip_internal_member_lock_set(uint16_t acl_handle, uint8_t lock);

#ifdef __cplusplus
}
#endif
