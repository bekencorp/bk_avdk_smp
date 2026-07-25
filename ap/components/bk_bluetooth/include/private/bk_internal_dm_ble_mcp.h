#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Private bridge between the EtherMind GA MCP reference (appl_mcp_ce.c /
 * appl_mcp_se.c) and the public bk_dm_mcp adapter (service/dm/ble/le_audio/mcp/bk_dm_mcp.c).
 *
 * appl_le_audio_mcp_*  : down-calls implemented in the appl layer (call GA_mcp_*).
 * bk_dm_mcp_internal_* : up-calls implemented in the adapter shim and invoked
 *                        from the appl_mcp_ce.c / appl_mcp_se.c GA callbacks.
 *
 * appl_le_audio_mcp_server_init() registers GMCS, invoked on demand via
 * bk_dm_mcp_server_init(). The dynamic GATT DB is committed automatically when a
 * server role first advertises.
 */

/* ---- down-calls (appl_mcp_ce.c, client) ---- */
uint32_t appl_le_audio_mcp_client_init(void);
uint32_t appl_le_audio_mcp_discover(uint8_t *addr, uint8_t addr_type);
uint32_t appl_le_audio_mcp_config_notify(uint8_t enable);
uint32_t appl_le_audio_mcp_control(uint8_t opcode);
uint32_t appl_le_audio_mcp_read_track_title(void);
uint32_t appl_le_audio_mcp_read_media_state(void);
uint32_t appl_le_audio_mcp_read_track_position(void);

/* ---- down-calls (appl_mcp_se.c, server) ---- */
void     appl_le_audio_mcp_server_init(void);
uint32_t appl_le_audio_mcp_server_set_media_state(uint8_t state);
uint32_t appl_le_audio_mcp_server_set_track_title(const char *title);
uint32_t appl_le_audio_mcp_server_set_track_position(int32_t position);

/* ---- up-calls (bk_dm_mcp.c shim) ---- */
void bk_dm_mcp_internal_setup(uint16_t acl_handle, uint8_t status);
void bk_dm_mcp_internal_track_title(uint16_t acl_handle, const char *title, uint16_t len);
void bk_dm_mcp_internal_media_state(uint16_t acl_handle, uint8_t state);
void bk_dm_mcp_internal_track_position(uint16_t acl_handle, int32_t position);
void bk_dm_mcp_internal_cp_done(uint16_t acl_handle, uint8_t status);
void bk_dm_mcp_internal_notify_cfg(uint16_t acl_handle, uint8_t status);
void bk_dm_mcp_internal_server_control(uint16_t acl_handle, uint8_t opcode);

#ifdef __cplusplus
}
#endif
