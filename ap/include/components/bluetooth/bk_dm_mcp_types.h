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

/* Media Control Point opcodes (MCP spec) - used by the controller to drive the
 * remote media player and reported to the server app on a remote write. */
#define BK_MCP_OPC_PLAY                 0x01U
#define BK_MCP_OPC_PAUSE                0x02U
#define BK_MCP_OPC_FAST_REWIND          0x03U
#define BK_MCP_OPC_FAST_FORWARD         0x04U
#define BK_MCP_OPC_STOP                 0x05U
#define BK_MCP_OPC_PREV_TRACK           0x30U
#define BK_MCP_OPC_NEXT_TRACK           0x31U
#define BK_MCP_OPC_FIRST_TRACK          0x32U
#define BK_MCP_OPC_LAST_TRACK           0x33U
#define BK_MCP_OPC_PREV_GROUP           0x40U
#define BK_MCP_OPC_NEXT_GROUP           0x41U

/* Media State values (MCP spec). */
#define BK_MCP_MEDIA_STATE_INACTIVE     0x00U
#define BK_MCP_MEDIA_STATE_PLAYING      0x01U
#define BK_MCP_MEDIA_STATE_PAUSED       0x02U
#define BK_MCP_MEDIA_STATE_SEEKING      0x03U

/* Media Control Profile client completion / error events. */
typedef enum
{
    BK_MCP_EVT_INVALID = 0,
    BK_MCP_EVT_SETUP,        /* GMCS context setup confirmed */
    BK_MCP_EVT_CP_DONE,      /* a media control-point write completed */
    BK_MCP_EVT_NOTIFY_CFG,   /* notification configuration completed */
} bk_mcp_cb_evt_t;

/*
 * Client (Media Control Client / GATT client) callbacks.
 * Data events use typed callbacks; pure completion/error uses event_cb.
 */
typedef struct
{
    void (* on_setup)(uint16_t acl_handle, uint8_t status);
    void (* on_track_title)(uint16_t acl_handle, const char *title, uint16_t len);
    void (* on_media_state)(uint16_t acl_handle, uint8_t state);
    void (* on_track_position)(uint16_t acl_handle, int32_t position);
    void (* event_cb)(uint16_t acl_handle, bk_mcp_cb_evt_t evt, uint8_t status);
} bk_mcp_client_callbacks_t;

/*
 * Server (Media Control Server / GATT server) callbacks.
 * on_control fires when a remote controller writes the Media Control Point;
 * the GA layer applies its default state machine and sends the response.
 */
typedef struct
{
    void (* on_control)(uint16_t acl_handle, uint8_t opcode);
} bk_mcp_server_callbacks_t;

#ifdef __cplusplus
}
#endif
