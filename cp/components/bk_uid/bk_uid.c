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
#include "bk_uid_adaptor.h"
#include <modules/uidlib.h>
#include <components/bk_uid.h>
#include <sys_sw_regs.h>
#if CONFIG_SUPPORT_CACHEABLE_SRAM
#include "cache.h"
#endif

/* Chip UID snapshot kept in CP SRAM. OTP is read (and SHA256 computed) at most
 * once for the whole system: the magic gate makes bk_uid_get_data() idempotent
 * across the CP SMP cores and the RPC service thread. The snapshot address is
 * published to sys_sw_regs.cp_uid_ptr so AP can read the UID cross-core. */
static bk_uid_snapshot_t s_cp_uid_snapshot;
static beken_mutex_t s_cp_uid_lock;

bk_err_t bk_uid_driver_init(void)
{
    bk_err_t ret = bk_uid_adaptor_init();
    if (ret != BK_OK) {
        return ret;
    }

    if (s_cp_uid_lock == NULL) {
        ret = rtos_init_mutex(&s_cp_uid_lock);
        if (ret != BK_OK) {
            return ret;
        }
    }

    /* Prewarm: read OTP once at init so the UID is published before AP asks. */
    unsigned char uid[BK_UID_SIZE] = {0};
    (void)bk_uid_get_data(uid);

    return BK_OK;
}

bk_err_t bk_uid_get_data(unsigned char data[32])
{
    bk_err_t ret = BK_OK;

    if (data == NULL) {
        return BK_ERR_PARAM;
    }

    if (s_cp_uid_lock != NULL) {
        rtos_lock_mutex(&s_cp_uid_lock);
    }

    if (s_cp_uid_snapshot.magic != BK_UID_SNAPSHOT_MAGIC) {
        unsigned char uid[BK_UID_SIZE] = {0};

        ret = (get_uid(uid) == 0) ? BK_OK : BK_FAIL;
        if (ret == BK_OK) {
            memcpy((void *)s_cp_uid_snapshot.uid, uid, BK_UID_SIZE);
            /* Publish order: uid[] first, DSB, magic last, so a cross-core
             * reader that sees the magic always sees a fully written uid[]. */
            __asm volatile ("dsb" ::: "memory");
            s_cp_uid_snapshot.magic = BK_UID_SNAPSHOT_MAGIC;
            __asm volatile ("dsb" ::: "memory");
#if CONFIG_SUPPORT_CACHEABLE_SRAM
            flush_dcache((void *)&s_cp_uid_snapshot, sizeof(s_cp_uid_snapshot));
            __asm volatile ("dsb" ::: "memory");
#endif
            /* Expose the snapshot address to AP (non-zero = ready). */
            bk_sys_sw_regs_set_cp_uid_ptr((uint32_t)(uintptr_t)&s_cp_uid_snapshot);
        }
    }

    if (ret == BK_OK) {
        memcpy(data, (const void *)s_cp_uid_snapshot.uid, BK_UID_SIZE);
    }

    if (s_cp_uid_lock != NULL) {
        rtos_unlock_mutex(&s_cp_uid_lock);
    }

    return ret;
}
