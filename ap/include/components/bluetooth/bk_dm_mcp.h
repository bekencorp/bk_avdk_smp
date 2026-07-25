// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "bk_dm_bluetooth_types.h"
#include "bk_dm_mcp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LE Audio Media Control Profile (MCP) adapter.
 *
 * Follows the bk_dm_bap adapter style (init + role register + typed callbacks).
 *
 * Call bk_dm_mcp_server_init() to bring up the GMCS server; the client side is
 * initialized on bk_dm_mcp_init(). The current adapter tracks a single
 * peer/GMCS context; acl_handle is carried in the public API for forward
 * compatibility.
 */

bk_err_t bk_dm_mcp_init(void);
bk_err_t bk_dm_mcp_client_register(const bk_mcp_client_callbacks_t *callbacks);
bk_err_t bk_dm_mcp_server_register(const bk_mcp_server_callbacks_t *callbacks);
bk_err_t bk_dm_mcp_server_init(void);

/* ---- Client (Media Control Client / GATT client) ---- */
bk_err_t bk_dm_mcp_discover(uint8_t *addr, uint8_t addr_type);
bk_err_t bk_dm_mcp_config_notify(uint16_t acl_handle, uint8_t enable);
bk_err_t bk_dm_mcp_control(uint16_t acl_handle, uint8_t opcode); /* BK_MCP_OPC_* */
bk_err_t bk_dm_mcp_read_track_title(uint16_t acl_handle);
bk_err_t bk_dm_mcp_read_media_state(uint16_t acl_handle);
bk_err_t bk_dm_mcp_read_track_position(uint16_t acl_handle);

/* ---- Server (Media Control Server / GATT server) ---- */
bk_err_t bk_dm_mcp_server_set_media_state(uint8_t state); /* BK_MCP_MEDIA_STATE_* */
bk_err_t bk_dm_mcp_server_set_track_title(const char *title);
bk_err_t bk_dm_mcp_server_set_track_position(int32_t position);

#ifdef __cplusplus
}
#endif
