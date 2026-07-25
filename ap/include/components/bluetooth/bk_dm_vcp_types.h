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

/* VCS mute states (Volume Control Service). */
#define BK_VCP_UNMUTE                   0x00U
#define BK_VCP_MUTE                     0x01U

/* Volume Control Profile controller (client) completion / error events. */
typedef enum
{
    BK_VCP_EVT_INVALID = 0,
    BK_VCP_EVT_SETUP,        /* GA_vc_setup confirmed (profile discovered) */
    BK_VCP_EVT_CAPABILITIES, /* optional VOCS/AICS capability discovery done */
    BK_VCP_EVT_CP_DONE,      /* a control-point write completed */
    BK_VCP_EVT_RELEASED,     /* context released */
} bk_vcp_cb_evt_t;

/* VCS Volume State characteristic value. */
typedef struct
{
    uint8_t volume;          /* 0 - 255 */
    uint8_t mute;            /* BK_VCP_UNMUTE / BK_VCP_MUTE */
    uint8_t change_counter;
} bk_vcp_volume_state_t;

/*
 * Controller (VCP Controller / GATT client) callbacks.
 * Data events use typed callbacks; pure completion/error uses event_cb.
 */
typedef struct
{
    void (* on_setup)(uint16_t acl_handle, uint8_t status);
    void (* on_volume_state)(uint16_t acl_handle, const bk_vcp_volume_state_t *state); /* CNF + NTF */
    void (* on_vocs_offset)(uint16_t acl_handle, int16_t offset);
    void (* on_aics_state)(uint16_t acl_handle, int8_t gain, uint8_t mute, uint8_t gain_mode);
    void (* event_cb)(uint16_t acl_handle, bk_vcp_cb_evt_t evt, uint8_t status);
} bk_vcp_controller_callbacks_t;

/*
 * Renderer (VCP Renderer / GATT server) callbacks.
 * Fired when a remote controller changes local state; the GA layer has already
 * updated the local VCS value and sent the GATT response/notification.
 */
typedef struct
{
    void (* on_volume_set)(uint16_t acl_handle, uint8_t volume, uint8_t mute);
} bk_vcp_renderer_callbacks_t;

#ifdef __cplusplus
}
#endif
