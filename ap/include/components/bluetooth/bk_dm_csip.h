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
#include "bk_dm_csip_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LE Audio Coordinated Set Identification Profile (CSIP) adapter.
 *
 * Follows the bk_dm_bap adapter style (init + role register + typed callbacks).
 *
 * Coordinator operations require an existing ACL/GATT connection to the peer.
 * bk_dm_csip_discover() performs CSIS discovery and automatically completes the
 * GA setup step, so the context becomes usable once on_setup() fires. The
 * current adapter tracks a single peer context; acl_handle is carried in the
 * public API for forward compatibility.
 */

bk_err_t bk_dm_csip_init(void);
bk_err_t bk_dm_csip_coordinator_register(const bk_csip_coordinator_callbacks_t *callbacks);
bk_err_t bk_dm_csip_member_register(const bk_csip_member_callbacks_t *callbacks);

/* ---- Coordinator (CSIP Set Coordinator / GATT client) ---- */
bk_err_t bk_dm_csip_discover(uint8_t *addr, uint8_t addr_type);
bk_err_t bk_dm_csip_get_sirk(uint16_t acl_handle);
bk_err_t bk_dm_csip_get_setsize(uint16_t acl_handle);
bk_err_t bk_dm_csip_get_rank(uint16_t acl_handle);
bk_err_t bk_dm_csip_get_lock(uint16_t acl_handle);
bk_err_t bk_dm_csip_set_lock(uint16_t acl_handle, uint8_t lock); /* BK_CSIP_LOCK_UNLOCKED/LOCKED */
bk_err_t bk_dm_csip_release(uint16_t acl_handle);

/* ---- Set Member (CSIP Set Member / GATT server) ---- */
bk_err_t bk_dm_csip_member_configure(const bk_csip_sirk_t *sirk, uint8_t size, uint8_t rank, uint8_t lock);

#ifdef __cplusplus
}
#endif
