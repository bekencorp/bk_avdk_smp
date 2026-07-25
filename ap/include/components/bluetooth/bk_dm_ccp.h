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
#include "bk_dm_ccp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LE Audio Call Control Profile (CCP) adapter.
 *
 * Follows the bk_dm_bap adapter style (init + role register + typed callbacks).
 *
 * Call bk_dm_ccp_server_init() to bring up the GTBS server; the client side is
 * initialized on bk_dm_ccp_init(). The current adapter tracks a single
 * peer/GTBS context; acl_handle is carried in the public API for forward
 * compatibility.
 */

bk_err_t bk_dm_ccp_init(void);
bk_err_t bk_dm_ccp_client_register(const bk_ccp_client_callbacks_t *callbacks);
bk_err_t bk_dm_ccp_server_register(const bk_ccp_server_callbacks_t *callbacks);
bk_err_t bk_dm_ccp_server_init(void);

/* ---- Client (Call Control Client / GATT client) ---- */
bk_err_t bk_dm_ccp_discover(uint8_t *addr, uint8_t addr_type);
bk_err_t bk_dm_ccp_accept(uint16_t acl_handle, uint8_t call_index);
bk_err_t bk_dm_ccp_terminate(uint16_t acl_handle, uint8_t call_index);
bk_err_t bk_dm_ccp_hold(uint16_t acl_handle, uint8_t call_index);
bk_err_t bk_dm_ccp_retrieve(uint16_t acl_handle, uint8_t call_index);
bk_err_t bk_dm_ccp_originate(uint16_t acl_handle, const char *uri);
bk_err_t bk_dm_ccp_read_call_state(uint16_t acl_handle);

/* ---- Server (Call Control Server / GTBS) ---- */
bk_err_t bk_dm_ccp_server_incoming_call(void);

#ifdef __cplusplus
}
#endif
