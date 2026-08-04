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

#define BT_AUDIO_SINK_DEMO_MSG_COUNT          (60)

#define LOCAL_NAME "bk_hfp_ag"


#define PAGE_SCAN_INTV   0x0800  //1.28s
#define PAGE_SCAN_WIN    0x00B4  //11.25ms

#define CONFIG_PAGE_TIMEOUT  16000   //unit of 0.625ms
#define CONFIG_RECONN_INTERVAL  2000   //unit of 1ms

#define CONFIG_WIFI_COEX_SCHEME  0

#define CONFIG_USE_AUDIO_LEGACY_INTERFACE  0
#define CONFIG_BOARD_AUDIO_CHANNLE_NUM     1
#define HFP_RESET_AUDIO_DATA_WHEN_ERR 0
