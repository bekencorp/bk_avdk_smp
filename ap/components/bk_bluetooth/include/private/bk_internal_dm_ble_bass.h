#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Private bridge between the EtherMind GA BASS reference (appl_bass.c) and the
 * public bk_dm_bass adapter (service/dm/ble/le_audio/bass/bk_dm_bass.c).
 *
 * appl_le_audio_bass_* : down-calls implemented in the appl layer (call GA_bass_*).
 * bk_dm_bass_internal_*: up-calls implemented in the adapter shim and invoked
 *                        from the appl_bass.c GA callbacks.
 *
 * appl_le_audio_bass_server_init() registers the BASS service (Scan Delegator),
 * invoked on demand via bk_dm_bass_server_init(). The dynamic GATT DB is
 * committed automatically when a server role first advertises.
 */

/* ---- down-calls (appl_bass.c, client / Broadcast Assistant) ---- */
uint32_t appl_le_audio_bass_client_init(void);
uint32_t appl_le_audio_bass_discover(uint8_t *addr, uint8_t addr_type);
uint32_t appl_le_audio_bass_read_rx_state(void);
uint32_t appl_le_audio_bass_scan(uint8_t started);
uint32_t appl_le_audio_bass_add_source(uint8_t *src_addr, uint8_t src_addr_type, uint8_t adv_sid);
uint32_t appl_le_audio_bass_add_source_ex(uint8_t *src_addr, uint8_t src_addr_type,
                                          uint8_t adv_sid, uint32_t broadcast_id,
                                          uint32_t bis_sync);
uint32_t appl_le_audio_bass_set_broadcast_code(uint8_t source_id, const uint8_t *code);
uint32_t appl_le_audio_bass_remove_source(uint8_t source_id);

/* ---- down-calls (appl_bass.c, server / Scan Delegator) ---- */
void     appl_le_audio_bass_server_init(void);
uint32_t appl_le_audio_bass_server_notify(void);

/* ---- down-calls (appl_bass.c, PAST) ---- */
/* Scan Delegator: accept incoming PAST on any connection (set default params). */
uint32_t appl_le_audio_bass_sd_setup_default_past(void);
/* Broadcast Assistant: transfer our source periodic-adv sync to the delegator. */
uint32_t appl_le_audio_bass_ba_send_past(uint8_t *deleg_addr, uint8_t deleg_addr_type, uint16_t sync_handle);

/* ---- up-calls (bk_dm_bass.c shim) ---- */
void bk_dm_bass_internal_setup(uint16_t acl_handle, uint8_t status);
void bk_dm_bass_internal_rx_state(uint16_t acl_handle, const uint8_t *data, uint16_t len);
void bk_dm_bass_internal_cp_done(uint16_t acl_handle, uint8_t status);
void bk_dm_bass_internal_server_control(uint16_t acl_handle, uint8_t opcode);
/* Scan Delegator: parsed Add/Remove Source so the app can drive PA/BIG sync.
 * Fields passed individually to keep the host-lib appl unit free of the public
 * bk_dm_bass_types.h struct. */
void bk_dm_bass_internal_server_add_source(uint16_t acl_handle, uint8_t source_id,
        uint8_t addr_type, const uint8_t *addr, uint8_t adv_sid, uint32_t broadcast_id,
        uint8_t pa_sync, uint16_t pa_interval, uint8_t num_subgroups, uint32_t bis_sync);
void bk_dm_bass_internal_server_set_broadcast_code(uint16_t acl_handle, uint8_t source_id,
        const uint8_t *code);
void bk_dm_bass_internal_server_remove_source(uint16_t acl_handle, uint8_t source_id);

/* Scan Delegator Broadcast Receive State (owned by bk_dm_bass adapter). */
void     bk_dm_bass_internal_se_reset(void);
uint16_t bk_dm_bass_internal_se_get_rx_state(const uint8_t **data);
void     bk_dm_bass_internal_se_handle_cp(uint16_t acl_handle, uint8_t opcode,
                                            const uint8_t *cp, uint16_t cp_len);
uint32_t bk_dm_bass_internal_se_set_pa_state(uint8_t pa_sync_state);

#ifdef __cplusplus
}
#endif
