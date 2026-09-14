// Copyright 2025-2026 Beken
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

/*
 * AP → CP customer IPC (bk_wdrv_customer_transfer / cif customer handler).
 * Mirrors doorbell/ap_powerdown_keepalive: AP requests, CP votes AP OFF.
 */
enum {
	AUD_FB_IPC_CMD_AP_OFF = 0xA001,
};

/* Payload required by bk_wdrv_customer_transfer (len must be > 0). */
typedef struct {
	uint32_t magic;
} aud_fb_ipc_ap_off_t;

#define AUD_FB_IPC_AP_OFF_MAGIC 0xA11DFB0FU

#ifdef __cplusplus
}
#endif
