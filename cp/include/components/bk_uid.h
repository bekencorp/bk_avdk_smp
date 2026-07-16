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

/* Chip UID length in bytes. Single source of truth for the UID feature; the
 * UID RPC transport (bk_api_rpc.h) includes this header and reuses BK_UID_SIZE. */
#define BK_UID_SIZE           (32)
#define BK_UID_SNAPSHOT_MAGIC (0x55494430U) /* 'UID0' */

/* Chip UID snapshot placed in CP SRAM. CP fills uid[] then sets magic; the
 * snapshot address is published to sys_sw_regs.cp_uid_ptr so AP can read the
 * UID cross-core without re-reading OTP. */
typedef struct {
    volatile uint32_t magic;                /**< BK_UID_SNAPSHOT_MAGIC when uid[] is valid */
    volatile uint8_t  uid[BK_UID_SIZE];
} bk_uid_snapshot_t;

bk_err_t bk_uid_adaptor_init(void);

bk_err_t bk_uid_driver_init(void);

bk_err_t bk_uid_get_data(unsigned char data[32]);


#ifdef __cplusplus
}
#endif