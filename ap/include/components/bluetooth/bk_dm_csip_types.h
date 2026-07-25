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

/* Set Identity Resolving Key length (CSIS). */
#define BK_CSIP_SIRK_LEN                16U

/* SIRK type. */
#define BK_CSIP_SIRK_TYPE_ENCRYPTED     0x00U
#define BK_CSIP_SIRK_TYPE_PLAIN         0x01U

/* Set Member Lock characteristic states. */
#define BK_CSIP_LOCK_UNLOCKED           0x01U
#define BK_CSIP_LOCK_LOCKED             0x02U

/* Coordinated Set Identification Profile coordinator completion / error events. */
typedef enum
{
    BK_CSIP_EVT_INVALID = 0,
    BK_CSIP_EVT_SETUP,      /* discover + setup confirmed (context ready) */
    BK_CSIP_EVT_LOCK_DONE,  /* set-lock write completed */
    BK_CSIP_EVT_RELEASED,   /* context released */
} bk_csip_cb_evt_t;

/* CSIS Set Identity Resolving Key value. */
typedef struct
{
    uint8_t type;                       /* BK_CSIP_SIRK_TYPE_* */
    uint8_t value[BK_CSIP_SIRK_LEN];
} bk_csip_sirk_t;

/*
 * Coordinator (CSIP Set Coordinator / GATT client) callbacks.
 * Data events use typed callbacks; pure completion/error uses event_cb.
 */
typedef struct
{
    void (* on_setup)(uint16_t acl_handle, uint8_t status);
    void (* on_sirk)(uint16_t acl_handle, const bk_csip_sirk_t *sirk);
    void (* on_setsize)(uint16_t acl_handle, uint8_t size);
    void (* on_rank)(uint16_t acl_handle, uint8_t rank);
    void (* on_lock_state)(uint16_t acl_handle, uint8_t lock); /* GET_CNF + NTF */
    void (* event_cb)(uint16_t acl_handle, bk_csip_cb_evt_t evt, uint8_t status);
} bk_csip_coordinator_callbacks_t;

/*
 * Set Member (CSIP Set Member / GATT server) callbacks.
 * Fired when a remote coordinator changes the local Set Member Lock; the GA
 * layer has already applied the value and sent the GATT response/notification.
 */
typedef struct
{
    void (* on_lock_set)(uint16_t acl_handle, uint8_t lock);
} bk_csip_member_callbacks_t;

#ifdef __cplusplus
}
#endif
