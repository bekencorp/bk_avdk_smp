
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
#include <os/os.h>

typedef struct
{
    uint32_t event;
    uint32_t param;
    uint32_t extra;
} bta_msg_t;

enum
{
    BTA_MOD_CORE = 1,
    BTA_MOD_MANAGER,
    BTA_MOD_AURACAST,
    BTA_MOD_AUDIO,
};

#define MODULE_BIT       (24)

enum
{
    BTA_EVT_CORE_WAIT4START = (BTA_MOD_CORE << MODULE_BIT),

    BTA_EVT_MAN_PAIRING = (BTA_MOD_MANAGER << MODULE_BIT),
    BTA_EVT_MAN_BROADCAST,
    BTA_EVT_MAN_PLAY,
    BTA_EVT_MAN_TURN_ON,
    BTA_EVT_MAN_TURN_OFF,
    BTA_EVT_MAN_VOLUME_UP,
    BTA_EVT_MAN_VOLUME_DOWN,
    BTA_EVT_MAN_SINK_BROADCAST_SCAN_COMPLETE_IND,

    BTA_EVT_AURACAST_INIT = (BTA_MOD_AURACAST << MODULE_BIT),
    BTA_EVT_AURACAST_SINK_BROADCAST_CALLBACK_IND,
    BTA_EVT_AURACAST_SINK_BROADCAST_SCAN_START,
    BTA_EVT_AURACAST_SINK_BROADCAST_SCAN_STOP,
    BTA_EVT_AURACAST_SINK_BROADCAST_SCAN_RESULT_IND,
    BTA_EVT_AURACAST_SINK_DISABLE_REQ,
    BTA_EVT_AURACAST_SOURCE_DISABLE_REQ,
    BTA_EVT_AURACAST_SROUCE_START_CNF,
    BTA_EVT_AURACAST_SROUCE_SUSPEND_CNF,
    BTA_EVT_AURACAST_SROUCE_SETUP_ANNOUNCEMENT_CNF,
    BTA_EVT_AURACAST_SROUCE_END_ANNOUNCEMENT_CNF,

    BTA_EVT_AUDIO_INIT = (BTA_MOD_AUDIO << MODULE_BIT),
    BTA_EVT_AUDIO_VOLUME_ABS,
    BTA_EVT_AUDIO_VOLUME_UP,
    BTA_EVT_AUDIO_VOLUME_DOWN

};



#define BTA_EVT_MSG_COUNT          (60)

int bta_event_init(void);
bk_err_t bta_event_send(uint32_t event, uint32_t param, uint32_t extra);

void bta_auracast_event_dispather(uint32_t event, uint32_t param, uint32_t extra);
void bta_audio_event_dispather(uint32_t event, uint32_t param, uint32_t extra);

