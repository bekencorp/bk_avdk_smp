#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Private bridge between the EtherMind GA CCP reference (appl_ccp_ce.c /
 * appl_ccp_se.c) and the public bk_dm_ccp adapter (service/dm/ble/le_audio/ccp/bk_dm_ccp.c).
 *
 * appl_le_audio_ccp_*  : down-calls implemented in the appl layer (call GA_ccp_*).
 * bk_dm_ccp_internal_* : up-calls implemented in the adapter shim and invoked
 *                        from the appl_ccp_ce.c / appl_ccp_se.c GA callbacks.
 *
 * appl_le_audio_ccp_server_init() registers GTBS, invoked on demand via
 * bk_dm_ccp_server_init(). The dynamic GATT DB is committed automatically when a
 * server role first advertises.
 */

/* ---- down-calls (appl_ccp_ce.c, client) ---- */
uint32_t appl_le_audio_ccp_client_init(void);
uint32_t appl_le_audio_ccp_discover(uint8_t *addr, uint8_t addr_type);
uint32_t appl_le_audio_ccp_accept(uint8_t call_index);
uint32_t appl_le_audio_ccp_terminate(uint8_t call_index);
uint32_t appl_le_audio_ccp_local_hold(uint8_t call_index);
uint32_t appl_le_audio_ccp_local_retrieve(uint8_t call_index);
uint32_t appl_le_audio_ccp_originate(const char *uri);
uint32_t appl_le_audio_ccp_read_call_state(void);

/* ---- down-calls (appl_ccp_se.c, server) ---- */
void     appl_le_audio_ccp_server_init(void);
uint32_t appl_le_audio_ccp_server_incoming_call(void);

/* ---- up-calls (bk_dm_ccp.c shim) ---- */
void bk_dm_ccp_internal_setup(uint16_t acl_handle, uint8_t status);
void bk_dm_ccp_internal_call_state(uint16_t acl_handle, uint8_t call_index, uint8_t state);
void bk_dm_ccp_internal_incoming_call(uint16_t acl_handle, uint8_t call_index, const char *uri, uint16_t len);
void bk_dm_ccp_internal_cp_done(uint16_t acl_handle, uint8_t status);
void bk_dm_ccp_internal_server_control(uint16_t acl_handle, uint8_t opcode, uint8_t call_index);

#ifdef __cplusplus
}
#endif
