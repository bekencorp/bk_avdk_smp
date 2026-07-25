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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BASS Control Point opcodes (reported to the Scan Delegator app on a remote
 * Broadcast Audio Scan Control Point write by a Broadcast Assistant). */
#define BK_BASS_OPC_REMOTE_SCAN_STOPPED     0x00U
#define BK_BASS_OPC_REMOTE_SCAN_STARTED     0x01U
#define BK_BASS_OPC_ADD_SOURCE              0x02U
#define BK_BASS_OPC_MODIFY_SOURCE           0x03U
#define BK_BASS_OPC_SET_BC_CODE             0x04U
#define BK_BASS_OPC_REMOVE_SOURCE           0x05U

/* Broadcast source address types. */
#define BK_BASS_ADDR_TYPE_PUBLIC            0x00U
#define BK_BASS_ADDR_TYPE_RANDOM            0x01U

/* Broadcast Audio Scan Service client completion / error events. */
typedef enum
{
    BK_BASS_EVT_INVALID = 0,
    BK_BASS_EVT_SETUP,      /* BASS context setup confirmed */
    BK_BASS_EVT_CP_DONE,    /* a BAS control-point write completed */
} bk_bass_cb_evt_t;

/*
 * Client (Broadcast Assistant / GATT client) callbacks.
 * on_rx_state delivers the raw Broadcast Receive State characteristic value.
 */
typedef struct
{
    void (* on_setup)(uint16_t acl_handle, uint8_t status);
    void (* on_rx_state)(uint16_t acl_handle, const uint8_t *data, uint16_t len);
    void (* event_cb)(uint16_t acl_handle, bk_bass_cb_evt_t evt, uint8_t status);
} bk_bass_client_callbacks_t;

/*
 * Parsed Add/Modify Source parameters handed to the Scan Delegator app so it can
 * drive the actual PA sync + BIG sync (the BASS control point only carries the
 * source description; the delegator turns it into a periodic-adv/BIG sync).
 */
typedef struct
{
    uint8_t  source_id;      /* delegator-assigned Source_ID                 */
    uint8_t  addr_type;      /* advertiser address type (0 public/1 random)  */
    uint8_t  addr[6];        /* advertiser address                           */
    uint8_t  adv_sid;        /* advertising SID                              */
    uint32_t broadcast_id;   /* 24-bit Broadcast_ID                          */
    uint8_t  pa_sync;        /* requested PA_Sync (0 no / 1 PAST / 2 no-PAST) */
    uint16_t pa_interval;    /* periodic advertising interval                */
    uint8_t  num_subgroups;  /* number of subgroups                          */
    uint32_t bis_sync;       /* subgroup[0] requested BIS_Sync bitmap         */
} bk_bass_source_info_t;

/*
 * Server (Scan Delegator / GATT server) callbacks.
 *  - on_control    : any BAS Control Point write (opcode only, back-compat).
 *  - on_add_source : Add Source, with parsed source description to sync to.
 *  - on_remove_source : Remove Source, with the Source_ID to drop.
 */
typedef struct
{
    void (* on_control)(uint16_t acl_handle, uint8_t opcode);
    void (* on_add_source)(uint16_t acl_handle, const bk_bass_source_info_t *info);
    void (* on_set_broadcast_code)(uint16_t acl_handle, uint8_t source_id, const uint8_t code[16]);
    void (* on_remove_source)(uint16_t acl_handle, uint8_t source_id);
} bk_bass_server_callbacks_t;

#ifdef __cplusplus
}
#endif
