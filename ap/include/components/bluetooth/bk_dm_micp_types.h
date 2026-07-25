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

/* MICS Mute characteristic states (Microphone Control Service). */
#define BK_MICP_UNMUTE                  0x00U
#define BK_MICP_MUTE                    0x01U
#define BK_MICP_MUTE_DISABLED           0x02U

/* Microphone Control Profile controller (client) completion / error events. */
typedef enum
{
    BK_MICP_EVT_INVALID = 0,
    BK_MICP_EVT_SETUP,        /* GA_mc_setup confirmed (profile discovered) */
    BK_MICP_EVT_CAPABILITIES, /* optional AICS capability discovery done */
    BK_MICP_EVT_CP_DONE,      /* a control-point write completed */
    BK_MICP_EVT_RELEASED,     /* context released */
} bk_micp_cb_evt_t;

/*
 * Controller (Microphone Controller / GATT client) callbacks.
 * Data events use typed callbacks; pure completion/error uses event_cb.
 */
typedef struct
{
    void (* on_setup)(uint16_t acl_handle, uint8_t status);
    void (* on_mute_state)(uint16_t acl_handle, uint8_t mute); /* GET_MUTE_CNF + MUTE_NTF */
    void (* on_aics_state)(uint16_t acl_handle, int8_t gain, uint8_t mute, uint8_t gain_mode);
    void (* event_cb)(uint16_t acl_handle, bk_micp_cb_evt_t evt, uint8_t status);
} bk_micp_controller_callbacks_t;

/*
 * Device (Microphone Device / GATT server) callbacks.
 * Fired when a remote controller changes local state; the GA layer has already
 * updated the local MICS/AICS value and sent the GATT response/notification.
 */
typedef struct
{
    void (* on_mute_set)(uint16_t acl_handle, uint8_t mute);
    void (* on_aics_gain_set)(uint16_t acl_handle, int8_t gain);
} bk_micp_device_callbacks_t;

#ifdef __cplusplus
}
#endif
