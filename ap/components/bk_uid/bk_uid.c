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

#include <string.h>
#include "os/os.h"
#include <components/bk_uid.h>
#include <sys_sw_regs.h>
#include "bk_api_rpc.h"

bk_err_t bk_uid_driver_init(void)
{
    /* AP does not read OTP; the UID is produced and published by CP. Nothing to
     * initialize here, kept for API symmetry with the CP build. */
    return BK_OK;
}

bk_err_t bk_uid_get_data(unsigned char data[32])
{
    if (data == NULL) {
        return BK_ERR_PARAM;
    }

    /* Fast path: CP has published the UID snapshot into CP SRAM and its address
     * into sys_sw_regs.cp_uid_ptr. In the shipping config the shared window and
     * CP SRAM are non-cacheable on AP, so a plain read is coherent. */
    uint32_t ptr = bk_sys_sw_regs_ptr()->cp_uid_ptr;
    if (ptr != 0U) {
        const bk_uid_snapshot_t *snap = (const bk_uid_snapshot_t *)(uintptr_t)ptr;
        if (snap->magic == BK_UID_SNAPSHOT_MAGIC) {
            memcpy(data, (const void *)snap->uid, BK_UID_SIZE);
            return BK_OK;
        }
    }

    /* Cold path: CP has not published yet. Trigger CP to compute+publish and
     * return the value directly (CP still reads OTP only once). */
    return bk_api_rpc_get_uid(data);
}
