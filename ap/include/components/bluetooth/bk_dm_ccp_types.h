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

/* Call State values (TBS/CCP spec). */
#define BK_CCP_CALL_STATE_INCOMING          0x00U
#define BK_CCP_CALL_STATE_DIALING           0x01U
#define BK_CCP_CALL_STATE_ALERTING          0x02U
#define BK_CCP_CALL_STATE_ACTIVE            0x03U
#define BK_CCP_CALL_STATE_LOCALLY_HELD      0x04U
#define BK_CCP_CALL_STATE_REMOTELY_HELD     0x05U
#define BK_CCP_CALL_STATE_LOC_REM_HELD      0x06U

/* Call Control Point opcodes (TBS/CCP spec) - reported to the server app on a
 * remote control-point write. */
#define BK_CCP_OPC_ACCEPT                   0x00U
#define BK_CCP_OPC_TERMINATE                0x01U
#define BK_CCP_OPC_LOCAL_HOLD               0x02U
#define BK_CCP_OPC_LOCAL_RETRIEVE           0x03U
#define BK_CCP_OPC_ORIGINATE                0x04U
#define BK_CCP_OPC_JOIN                     0x05U

/* Call Control Profile client completion / error events. */
typedef enum
{
    BK_CCP_EVT_INVALID = 0,
    BK_CCP_EVT_SETUP,        /* GTBS context setup confirmed */
    BK_CCP_EVT_CP_DONE,      /* a call control-point write completed */
} bk_ccp_cb_evt_t;

/*
 * Client (Call Control Client / GATT client) callbacks.
 */
typedef struct
{
    void (* on_setup)(uint16_t acl_handle, uint8_t status);
    void (* on_call_state)(uint16_t acl_handle, uint8_t call_index, uint8_t state);
    void (* on_incoming_call)(uint16_t acl_handle, uint8_t call_index, const char *uri, uint16_t len);
    void (* event_cb)(uint16_t acl_handle, bk_ccp_cb_evt_t evt, uint8_t status);
} bk_ccp_client_callbacks_t;

/*
 * Server (Call Control Server / GTBS) callbacks.
 * on_control fires when a remote client writes the Call Control Point.
 */
typedef struct
{
    void (* on_control)(uint16_t acl_handle, uint8_t opcode, uint8_t call_index);
} bk_ccp_server_callbacks_t;

#ifdef __cplusplus
}
#endif
