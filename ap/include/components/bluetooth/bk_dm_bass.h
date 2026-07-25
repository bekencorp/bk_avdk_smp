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
#include "bk_dm_bass_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LE Audio Broadcast Audio Scan Service (BASS) adapter.
 *
 * Follows the bk_dm_bap adapter style (init + role register + typed callbacks).
 * Covers the BAP "Broadcast Assistant" (client) and "Scan Delegator" (server)
 * roles.
 *
 * Call bk_dm_bass_server_init() to bring up the Scan Delegator (BASS server);
 * the client side is initialized on bk_dm_bass_init(). The current adapter
 * tracks a single peer/BASS context; acl_handle is carried in the public API
 * for forward compatibility.
 */

bk_err_t bk_dm_bass_init(void);
bk_err_t bk_dm_bass_client_register(const bk_bass_client_callbacks_t *callbacks);
bk_err_t bk_dm_bass_server_register(const bk_bass_server_callbacks_t *callbacks);
bk_err_t bk_dm_bass_server_init(void);

/* ---- Client (Broadcast Assistant / GATT client) ---- */
bk_err_t bk_dm_bass_discover(uint8_t *addr, uint8_t addr_type);
bk_err_t bk_dm_bass_read_rx_state(uint16_t acl_handle);
bk_err_t bk_dm_bass_scan_start(uint16_t acl_handle);
bk_err_t bk_dm_bass_scan_stop(uint16_t acl_handle);
bk_err_t bk_dm_bass_add_source(uint16_t acl_handle, uint8_t *src_addr, uint8_t src_addr_type, uint8_t adv_sid);
bk_err_t bk_dm_bass_add_source_ex(uint16_t acl_handle, uint8_t *src_addr, uint8_t src_addr_type,
                                  uint8_t adv_sid, uint32_t broadcast_id, uint32_t bis_sync);
bk_err_t bk_dm_bass_set_broadcast_code(uint16_t acl_handle, uint8_t source_id, const uint8_t *code);
bk_err_t bk_dm_bass_remove_source(uint16_t acl_handle, uint8_t source_id);

/* ---- Server (Scan Delegator / GATT server) ---- */
bk_err_t bk_dm_bass_delegator_adv(uint8_t enable);
bk_err_t bk_dm_bass_server_notify_rx_state(void);

/* ---- PAST (Periodic Advertising Sync Transfer) ----
 * On this platform the Scan Delegator advertises legacy connectable, so it gets
 * the broadcast source's periodic-adv sync from the Broadcast Assistant via PAST
 * rather than syncing itself. */
/* Scan Delegator: accept incoming PAST on any connection (call once when up). */
bk_err_t bk_dm_bass_delegator_setup_past(void);
/* Scan Delegator: report Broadcast Receive State PA_Sync_State (e.g. synced). */
bk_err_t bk_dm_bass_delegator_set_pa_state(uint8_t pa_sync_state);
/* Broadcast Assistant: transfer our source periodic-adv sync (sync_handle) to
 * the connected Scan Delegator. */
bk_err_t bk_dm_bass_assistant_send_past(uint8_t *deleg_addr, uint8_t deleg_addr_type, uint16_t sync_handle);

#ifdef __cplusplus
}
#endif
