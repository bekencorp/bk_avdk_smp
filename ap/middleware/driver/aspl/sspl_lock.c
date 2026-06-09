#include <stdint.h>
#include <os/os.h>
#include "szymanski_lock/src/szymanski_lock.h"
#include "sys_sw_regs.h"
#include "hspl/hspl_res_lock.h"
#include "arch_interrupt.h"

static bk_err_t sspl_res_lock_impl(bk_hspl_res_t res)
{
    sspl_data_t *sspl_list = (sspl_data_t *)bk_sys_sw_regs_get_sspl_list();
    uint32_t core_id = rtos_get_core_id();
    szymanski_lock((szymanski_lock_t *)&sspl_list[res], core_id);
    return BK_OK;
}

static bk_err_t sspl_res_unlock_impl(bk_hspl_res_t res)
{
    sspl_data_t *sspl_list = (sspl_data_t *)bk_sys_sw_regs_get_sspl_list();
    uint32_t core_id = rtos_get_core_id();
    szymanski_unlock((szymanski_lock_t *)&sspl_list[res], core_id);
    return BK_OK;
}

#define HSPL_MAX_CORES 2
#define HSPL_REC_COUNT_MAX 255

static inline uint8_t hspl_core_index(void)
{
    uint32_t id = portGET_CORE_ID();
    BK_ASSERT(id < HSPL_MAX_CORES);
    return (uint8_t)id;
}

/* Recursive lock count: [res][core_id]. Same core locking again only increments count. */
static volatile uint8_t s_rec_count[BK_HSPL_RES_MAX][HSPL_MAX_CORES];
bk_err_t bk_sspl_res_unlock(bk_hspl_res_t res)
{
    uint8_t core_id;
    uint32_t flags;
    bk_err_t ret = BK_OK;

    if (res >= BK_HSPL_RES_MAX) {
        BK_ASSERT(0);
        return BK_ERR_PARAM;
    }

    core_id = hspl_core_index();
    flags = rtos_disable_int();
    if (s_rec_count[res][core_id] == 0) {
        rtos_enable_int(flags);
        BK_ASSERT(0);
        return BK_ERR_PARAM;
    }
    s_rec_count[res][core_id]--;
    if (s_rec_count[res][core_id] == 0) {
        ret = sspl_res_unlock_impl(res);
    }
    rtos_enable_int(flags);
    return ret;
}

bk_err_t bk_sspl_res_lock(bk_hspl_res_t res)
{
    uint8_t core_id;
    uint32_t flags;

    if (res >= BK_HSPL_RES_MAX) {
        BK_ASSERT(0);
        return BK_ERR_PARAM;
    }

    core_id = hspl_core_index();
    flags = rtos_disable_int();
    if (s_rec_count[res][core_id] > 0) {
        if (s_rec_count[res][core_id] >= HSPL_REC_COUNT_MAX) {
            rtos_enable_int(flags);
            return BK_ERR_NOT_SUPPORT;
        }
        s_rec_count[res][core_id]++;
        rtos_enable_int(flags);
        return BK_OK;
    }
    rtos_enable_int(flags);

    /* Busy-wait until lock is acquired.
     * In exception/coredump context skip the (un-timed) busy-wait: a stopped
     * peer core may hold this lock forever, and there is no real concurrency to
     * guard (interrupts disabled, other cores stopped). Avoid a hard hang that
     * would lead to a second watchdog timeout. */
    if (!arch_is_enter_exception()) {
        sspl_res_lock_impl(res);
    }

    flags = rtos_disable_int();
    s_rec_count[res][core_id] = 1;
    rtos_enable_int(flags);
    return BK_OK;
}
