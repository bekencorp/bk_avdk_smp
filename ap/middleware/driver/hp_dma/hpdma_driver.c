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

#include <common/bk_include.h>
#include <common/bk_compiler.h>
#include <os/mem.h>
#include <os/os.h>
#include "icu_driver.h"
#include "hpdma_driver.h"
#include "hpdma_hal.h"
#include <driver/hpdma.h>
#include <driver/hal/hal_hpdma_types.h>
#include <driver/int.h>
#include "sys_driver.h"
#include "cmsis_gcc.h"
#include <soc/soc.h>
#include <soc/reg_base.h>

#if CONFIG_SUPPORT_CACHEABLE_SRAM
#include "cache.h"
#endif

#include "bk_misc.h"

#include <os/mem.h>

/*
 * S2 (HPDMA review):
 *   Pull PM API in so bk_hpdma_driver_init() can register an
 *   exit-low-voltage callback for the controller's global registers
 *   (soft_reset / secure_attr / privileged_attr / prio_mode), which are
 *   powered down across low-voltage sleep. See
 *   bk_hpdma_recover_after_low_voltage().
 */
#if CONFIG_PM_ENABLE
#include <modules/pm.h>
#endif

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static SPINLOCK_SECTION volatile spinlock_t hpdma_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP


static inline uint32_t hpdma_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
	spin_lock(&hpdma_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	return flags;
}

static inline void hpdma_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
	spin_unlock(&hpdma_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	rtos_enable_int(flags);
}

static void hpdma_isr(void) __BK_SECTION(".itcm");	//dma-hw-0


typedef struct {
    hpdma_hal_t hal;
    uint32_t id_init_bits;
} hpdma_driver_t;

static hpdma_driver_t s_hpdma;
static hpdma_isr_info_t s_hpdma_finish_isr[SOC_HPDMA_CHAN_NUM_PER_UNIT] = {{NULL, NULL}};
static hpdma_isr_info_t s_hpdma_half_finish_isr[SOC_HPDMA_CHAN_NUM_PER_UNIT] = {{NULL, NULL}};
static hpdma_isr_info_t s_hpdma_bus_err_isr[SOC_HPDMA_CHAN_NUM_PER_UNIT] = {{NULL, NULL}};
/*
 * P0 (HPDMA review): fifo_err is now a first-class interrupt class. Each
 *   channel can register its own callback, paralleling bus_err.
 */
static hpdma_isr_info_t s_hpdma_fifo_err_isr[SOC_HPDMA_CHAN_NUM_PER_UNIT] = {{NULL, NULL}};

static bool s_hpdma_driver_is_init = false;
static hpdma_chnl_pool_t s_hpdma_chnl_pool = {0};

#define HPDMA_RETURN_ON_NOT_INIT() do {\
        if (!s_hpdma_driver_is_init) {\
            return BK_ERR_HPDMA_NOT_INIT;\
        }\
    } while(0)

#define HPDMA_RETURN_ON_INVALID_ID(channel) do {\
        if (((channel) < CONFIG_HPDMA_LOGIC_CHAN_ID_MIN) || ((channel) >= CONFIG_HPDMA_LOGIC_CHAN_ID_MIN + CONFIG_HPDMA_LOGIC_CHAN_CNT)) {\
            return BK_ERR_HPDMA_ID;\
        }\
    } while(0)

#define HPDMA_RETURN_ON_ID_NOT_INIT(hpdma_num,id) do {\
        if (!(s_hpdma.id_init_bits & BIT((id)))) {\
            return BK_ERR_HPDMA_ID_NOT_INIT;\
        }\
    } while(0)

#define HPDMA_LOG_ON_ID_IS_STARTED(hpdma_num,channel) do {\
        if (hpdma_hal_is_id_started(&s_hpdma.hal, (channel))) {\
        }\
    } while(0)

#define HPDMA_RETURN_ON_INVALID_ADDR(start_addr, end_addr) do {\
        if ((0 < (end_addr)) && ((end_addr) < (start_addr))) {\
            return BK_ERR_HPDMA_INVALID_ADDR;\
        }\
    } while(0)

/*
 * New P0 (HPDMA review):
 *   Forward declaration so bk_hpdma_start() / hpdma_memcpy_by_chnl()
 *   can defer SMEM-same-block burst evaluation to the moment the
 *   channel actually has real src/dst addresses. The definition lives
 *   alongside the rest of the SMEM helpers further down in this file.
 */
static void hpdma_apply_smem_burst_policy_at_start(hpdma_id_t id);

/*
 * Forward declaration: hpdma_wait_to_idle() is defined further down in
 * this file but bk_hpdma_free() (above) needs to call it on the
 * single-shot path to make sure the engine actually honoured dma_en=0
 * before the channel bitmap bit is released.
 */
bk_err_t hpdma_wait_to_idle(hpdma_id_t id);

#if CONFIG_PM_ENABLE
/*
 * S2 (HPDMA review):
 *   Adapter that matches the PM module's pm_cb signature
 *   (int (*)(uint64_t sleep_time, void *args)). Registered as the
 *   exit-low-voltage callback under PM_DEV_ID_HPDMA. The PM core calls
 *   this once per wakeup, before any peripheral driver runs, which is
 *   exactly the contract bk_hpdma_recover_after_low_voltage() needs.
 *
 *   Returning a non-zero on failure follows the convention used by
 *   other peripheral PM callbacks in this SDK.
 */
static int hpdma_pm_exit_low_voltage_cb(uint64_t sleep_time, void *args)
{
    (void)sleep_time;
    (void)args;
    bk_err_t ret = bk_hpdma_recover_after_low_voltage();
    if (ret != BK_OK) {
        HPDMA_LOGE("PM low-voltage exit: recover failed ret=%d\r\n", ret);
    }
    return (int)ret;
}

/*
 * pm_cb_conf_t must be passed by non-const pointer per PM API; the
 * struct is owned by this driver and lives for the lifetime of the
 * registration (until bk_hpdma_driver_deinit unregisters it).
 */
static pm_cb_conf_t s_hpdma_pm_exit_cfg = {
    .cb = hpdma_pm_exit_low_voltage_cb,
    .args = NULL,
};
#endif /* CONFIG_PM_ENABLE */

static void hpdma_id_init_common(hpdma_id_t id)
{
    // 2: cache enabled
    hpdma_hal_set_cfg_cache(&s_hpdma.hal, id, 2);
    s_hpdma.id_init_bits |= BIT(id);

}

static void hpdma_id_deinit_common(hpdma_id_t id)
{
    s_hpdma.id_init_bits &= ~BIT(id);
    hpdma_hal_stop_common(&s_hpdma.hal, id);
    hpdma_hal_reset_config_to_default(&s_hpdma.hal, id);
}

static void hpdma_id_enable_interrupt_common(hpdma_id_t id)
{
    //0:int route to M55 core 0
    hpdma_hal_set_int_allocate(&s_hpdma.hal, id, HPDMA_INT_0);
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_HPDMA, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_HPDMA, 1);
#endif
}

static void hpdma_clear_channel_interrupt_state(hpdma_id_t id)
{
    if (id >= HPDMA_ID_MAX) {
        return;
    }

    uint32_t flags = hpdma_enter_critical();

    hpdma_hal_disable_half_finish_interrupt(&s_hpdma.hal, id);
    hpdma_hal_disable_finish_interrupt(&s_hpdma.hal, id);
    hpdma_hal_disable_bus_err_interrupt(&s_hpdma.hal, id);
    hpdma_hal_disable_fifo_err_interrupt(&s_hpdma.hal, id);

    hpdma_hal_clear_half_finish_interrupt_status(&s_hpdma.hal, id);
    hpdma_hal_clear_finish_interrupt_status(&s_hpdma.hal, id);
    hpdma_hal_clear_bus_err_interrupt_status(&s_hpdma.hal, id);
    hpdma_hal_clear_fifo_err_interrupt_status(&s_hpdma.hal, id);

    s_hpdma_half_finish_isr[id].callback = NULL;
    s_hpdma_half_finish_isr[id].user_data = NULL;
    s_hpdma_finish_isr[id].callback = NULL;
    s_hpdma_finish_isr[id].user_data = NULL;
    s_hpdma_bus_err_isr[id].callback = NULL;
    s_hpdma_bus_err_isr[id].user_data = NULL;
    s_hpdma_fifo_err_isr[id].callback = NULL;
    s_hpdma_fifo_err_isr[id].user_data = NULL;

    hpdma_exit_critical(flags);
    __DSB();
    __ISB();
}

/* used internally, called in context of interrupt disabled. */
u8 hpdma_chnl_alloc(u32 user_id)
{
	u8 chnl_id;

    for(chnl_id = CONFIG_HPDMA_LOGIC_CHAN_ID_MIN; chnl_id < CONFIG_HPDMA_LOGIC_CHAN_ID_MIN + CONFIG_HPDMA_LOGIC_CHAN_CNT; chnl_id++)
	{
		if((s_hpdma_chnl_pool.chnl_bitmap & (0x01 << chnl_id)) == 0)
		{
			s_hpdma_chnl_pool.chnl_bitmap |= (0x01 << chnl_id);
			s_hpdma_chnl_pool.chnl_user[chnl_id] = user_id;
			return chnl_id;
		}
	}

	//alloc failed
	HPDMA_LOGE("%s:chnl_id=%d\r\n", __func__, chnl_id);
	return HPDMA_ID_MAX;
}

hpdma_id_t hpdma_fixed_chnl_alloc(u32 user_id, hpdma_id_t fixed_chnl_id)
{

    if((fixed_chnl_id >= CONFIG_HPDMA_LOGIC_CHAN_ID_MIN) && (fixed_chnl_id < CONFIG_HPDMA_LOGIC_CHAN_ID_MIN + CONFIG_HPDMA_LOGIC_CHAN_CNT))
	{
		if((s_hpdma_chnl_pool.chnl_bitmap & (0x01 << fixed_chnl_id)) == 0)
		{
			s_hpdma_chnl_pool.chnl_bitmap |= (0x01 << fixed_chnl_id);
			s_hpdma_chnl_pool.chnl_user[fixed_chnl_id] = user_id;
			return fixed_chnl_id;
		}
	}

	HPDMA_LOGE("chan=%d has been allocated\r\n", fixed_chnl_id);
	return HPDMA_ID_MAX;
}

/* used internally, called in context of interrupt disabled. */
bk_err_t hpdma_chnl_free(u32 user_id, hpdma_id_t chnl_id)
{
	if( chnl_id >= HPDMA_ID_MAX )
		return BK_ERR_HPDMA_ID;

	if( s_hpdma_chnl_pool.chnl_user[chnl_id] != user_id )
		return BK_ERR_PARAM;

	s_hpdma_chnl_pool.chnl_bitmap &= ~(0x01 << chnl_id);
	s_hpdma_chnl_pool.chnl_user[chnl_id] = -1;

	return BK_OK;
}

/* used internally. */
u32 hpdma_chnl_user(hpdma_id_t chnl_id)
{
	if( chnl_id >= HPDMA_ID_MAX )
		return -1;

	return s_hpdma_chnl_pool.chnl_user[chnl_id];
}

bk_err_t bk_hpdma_driver_init(void)
{
    if (s_hpdma_driver_is_init) {
        return BK_OK;
    }

	s_hpdma_chnl_pool.chnl_bitmap = 0;
	for(u8 i = CONFIG_HPDMA_LOGIC_CHAN_ID_MIN; i < CONFIG_HPDMA_LOGIC_CHAN_ID_MIN + CONFIG_HPDMA_LOGIC_CHAN_CNT; i++)
	{
		s_hpdma_chnl_pool.chnl_user[i] = -1;
	}

    // workaround: must uncomment it after mailbox problem fixed
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA0, PM_POWER_MODULE_STATE_ON);
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA1, PM_POWER_MODULE_STATE_ON);

    /* 1)intc_service_register
     * 2)init hpdma_finish_int handler, hpdma_half_finish_int handler
     * 3)disable hpdma_en (0~6), clear int status
     * 4)init hpdma_config
     */
    os_memset(&s_hpdma, 0, sizeof(s_hpdma));
    os_memset(&s_hpdma_finish_isr, 0, sizeof(s_hpdma_finish_isr));
    os_memset(&s_hpdma_half_finish_isr, 0, sizeof(s_hpdma_half_finish_isr));
    os_memset(&s_hpdma_bus_err_isr, 0, sizeof(s_hpdma_bus_err_isr));
    os_memset(&s_hpdma_fifo_err_isr, 0, sizeof(s_hpdma_fifo_err_isr));  /* P0 (HPDMA review) */

    bk_int_isr_register(INT_SRC_HPDMA, hpdma_isr, NULL);


    for (uint32_t uint_id = 0; uint_id < SOC_HPDMA_UNIT_NUM; uint_id++) {
	    s_hpdma.hal.id = uint_id;
		hpdma_hal_init(&s_hpdma.hal);
	}

    s_hpdma_driver_is_init = true;

#if CONFIG_PM_ENABLE
    /*
     * S2 (HPDMA review):
     *   Wire bk_hpdma_recover_after_low_voltage() into the PM
     *   exit-low-voltage path. Without this, the first HPDMA transfer
     *   after wakeup would silently fail because soft_reset /
     *   secure_attr / privileged_attr come back as zero from sleep.
     *
     *   Registered here (driver_init), unregistered in driver_deinit.
     *   No enter-low-voltage callback is needed: HPDMA has no in-RAM
     *   state worth saving - the channel pool is software-only and the
     *   controller registers are always reprogrammed on the next init.
     */
    bk_err_t pm_ret = bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE,
                                              PM_DEV_ID_HPDMA,
                                              NULL,
                                              &s_hpdma_pm_exit_cfg);
    if (pm_ret != BK_OK) {
        HPDMA_LOGE("PM low-voltage exit cb register failed ret=%d\r\n", pm_ret);
        /* Non-fatal: HPDMA still works without low-voltage support. */
    }
#endif

    return BK_OK;
}

bk_err_t bk_hpdma_driver_deinit(void)
{
    if (!s_hpdma_driver_is_init) {
        return BK_OK;
    }

    /*
     * S0 (HPDMA SMP review):
     *   hpdma_id_deinit_common touches each channel's ctrl / req_mux /
     *   status (W1C) and the shared id_init_bits bitmap. Defensive lock
     *   here protects against any peer-core task or routed-core ISR
     *   that is still in flight when the driver is being torn down.
     */
    for (int id = 0; id < (SOC_HPDMA_CHAN_NUM_PER_UNIT*SOC_HPDMA_UNIT_NUM); id++) {
        uint32_t int_mask = hpdma_enter_critical();
        hpdma_id_deinit_common(id);
        hpdma_exit_critical(int_mask);
    }

#if CONFIG_PM_ENABLE
    /*
     * S2 (HPDMA review):
     *   Unregister the exit-low-voltage callback paired with the one
     *   installed in bk_hpdma_driver_init(). Both enter and exit cb
     *   flags are set to true to mirror the bk_pm_sleep_unregister_cb
     *   pattern used by other drivers (e.g. i2c_driver.c) even though
     *   we never registered an enter cb.
     */
    bk_pm_sleep_unregister_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_HPDMA, true, true);
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA0, PM_POWER_MODULE_STATE_OFF);
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA1, PM_POWER_MODULE_STATE_OFF);
#endif


#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_HPDMA, 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_HPDMA, 0);
#endif

    s_hpdma_driver_is_init = false;

    return BK_OK;
}

hpdma_id_t bk_hpdma_alloc(u16 user_id)
{
    if (!s_hpdma_driver_is_init)
    {
        return HPDMA_ID_MAX;
    }

    u32 int_mask = hpdma_enter_critical();

    u8 chnl_id = hpdma_chnl_alloc(user_id);

    hpdma_exit_critical(int_mask);

    return chnl_id;
}

hpdma_id_t bk_fixed_hpdma_alloc(u16 user_id, hpdma_id_t chnl_id)
{
    if (!s_hpdma_driver_is_init)
    {
        return HPDMA_ID_MAX;
    }

    u32 int_mask = hpdma_enter_critical();

    u8 chnl_id_ret = hpdma_fixed_chnl_alloc(user_id, chnl_id);

    hpdma_exit_critical(int_mask);

    return chnl_id_ret;
}

bk_err_t bk_hpdma_free(u16 user_id, hpdma_id_t chnl_id)
{
    if (!s_hpdma_driver_is_init)
    {
        return BK_ERR_HPDMA_NOT_INIT;
    }

    /*
     * S0 (HPDMA stability review):
     *   Previously bk_hpdma_free only cleared the software bitmap, leaving
     *   the hardware channel possibly still transferring. The next allocator
     *   of the same chnl_id would race with the previous owner's DMA (seen
     *   in GPU thread_exit, psram_dma_stress cleanup, CLI hpdma chnl free).
     *
     *   Defensive teardown contract for this API is now:
     *     1) stop the hardware channel,
     *     2) wait until dma_en clears (bounded by HPDMA_MAX_BUSY_TIME us),
     *     3) only then return the channel to the pool.
     *
     *   On timeout we deliberately KEEP the channel reserved (the bitmap is
     *   not cleared) and return BK_ERR_HPDMA_TIMEOUT, so a still-running DMA
     *   cannot be handed out as a fresh channel. Callers should then either
     *   retry or treat it as a fatal teardown error.
     */
    if (chnl_id < HPDMA_ID_MAX)
    {
        /*
         * S1/D (HPDMA review):
         *   Use stop_disable_only here instead of hpdma_hal_stop_common.
         *   The latter W1C-clears half/finish/bus/fifo status as a side
         *   effect, which previously corrupted any caller's interrupt
         *   counter / status snapshot taken right after a free attempt.
         *   wait_to_idle below also uses stop_disable_only for the same
         *   reason.
         * S0 (HPDMA SMP review):
         *   stop_disable_only is a ctrl RMW; serialize it. Do NOT
         *   extend the lock across hpdma_wait_to_idle - that polls for
         *   up to HPDMA_MAX_BUSY_TIME us (10 ms) and would be a hard
         *   real-time violation under SMP.
         */
        {
            uint32_t int_mask = hpdma_enter_critical();
            hpdma_hal_stop_disable_only(&s_hpdma.hal, chnl_id);
            hpdma_exit_critical(int_mask);
        }
        bk_err_t wait_ret = hpdma_wait_to_idle(chnl_id);
        if (wait_ret != BK_OK)
        {
            /*
             * S0/C (HPDMA review):
             *   wait_to_idle timeout means the DMA engine itself did not
             *   honour `dma_en=0` within HPDMA_MAX_BUSY_TIME. We deliber-
             *   ately leave the bitmap bit set so the same chnl_id is not
             *   handed out to a fresh allocator while the engine may
             *   still touch memory. Callers can recover via
             *   bk_hpdma_force_reclaim() (per-channel reset, no global
             *   soft_reset) when they accept the data-loss risk.
             */
            HPDMA_LOGE("hpdma_free: ch%d still busy, keep channel reserved; call bk_hpdma_force_reclaim() to recover\r\n",
                       chnl_id);
            return BK_ERR_HPDMA_TIMEOUT;
        }

        hpdma_clear_channel_interrupt_state(chnl_id);
    }

    u32  int_mask = hpdma_enter_critical();

    bk_err_t ret_val = hpdma_chnl_free(user_id, chnl_id);

    hpdma_exit_critical(int_mask);

    return ret_val;
}

uint32_t bk_hpdma_user(hpdma_id_t chnl_id)
{
    if (!s_hpdma_driver_is_init)
    {
        return -1;
    }

    return hpdma_chnl_user(chnl_id);
}

bk_err_t bk_hpdma_init(hpdma_id_t id, const hpdma_config_t *config)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    if (config == NULL) {
        return BK_ERR_NULL_PARAM;
    }

    // Validate ysize: user input should be >= 1 (1 = 1 row, 2 = 2 rows, etc.)
    if (config->src.ysize == 0) {
        HPDMA_LOGE("Source ysize must be >= 1 (1 = 1 row, 2 = 2 rows, etc.)\r\n");
        return BK_ERR_PARAM;
    }
    if (config->dst.ysize == 0) {
        HPDMA_LOGE("Destination ysize must be >= 1 (1 = 1 row, 2 = 2 rows, etc.)\r\n");
        return BK_ERR_PARAM;
    }

    /*
     * P1 (HPDMA review):
     *   Reg20/Reg22 require src_xsize*src_ysize == dst_xsize*dst_ysize.
     *   The hardware silently produces shifted/garbled output when the
     *   two byte counts differ; previously nothing validated this. Link
     *   mode (xsize == 0 by convention because the real sizes live in
     *   the descriptors) is exempted.
     */
    if (config->src.xsize != 0 && config->dst.xsize != 0) {
        uint32_t src_bytes = (uint32_t)config->src.xsize * config->src.ysize;
        uint32_t dst_bytes = (uint32_t)config->dst.xsize * config->dst.ysize;
        if (src_bytes != dst_bytes) {
            HPDMA_LOGE("src bytes (%u) != dst bytes (%u)\r\n", src_bytes, dst_bytes);
            return BK_ERR_HPDMA_TRANS_LEN;
        }
    }

    if (config->src.addr_loop_en) {
        uint32_t src_loop_end_addr = config->src.start_addr + config->src.xsize;
        if (((config->src.start_addr | src_loop_end_addr) & 0xFU) != 0) {
            HPDMA_LOGE("src loop window must be 128-bit aligned, start=0x%x end=0x%x\r\n",
                       config->src.start_addr, src_loop_end_addr);
            return BK_ERR_HPDMA_HAL_INVALID_ALIGN;
        }
    }
    if (config->dst.addr_loop_en) {
        uint32_t dst_loop_end_addr = config->dst.start_addr + config->dst.xsize;
        if (((config->dst.start_addr | dst_loop_end_addr) & 0xFU) != 0) {
            HPDMA_LOGE("dst loop window must be 128-bit aligned, start=0x%x end=0x%x\r\n",
                       config->dst.start_addr, dst_loop_end_addr);
            return BK_ERR_HPDMA_HAL_INVALID_ALIGN;
        }
    }

    /*
     * S2 (HPDMA review):
     *   The previous code called hpdma_hal_init_without_channels() on
     *   every bk_hpdma_init(), which on a fresh boot is a NOP (after the
     *   first soft_reset) but after a low-voltage wakeup performs a full
     *   controller soft_reset including overwriting secure_attr /
     *   privileged_attr. If another channel was mid-transfer at the
     *   moment of wakeup recovery, that channel would be silently
     *   aborted. Recovery now has its own explicit entry point
     *   bk_hpdma_recover_after_low_voltage() that the PM module should
     *   call exactly once on wakeup; bk_hpdma_init() no longer touches
     *   global controller state.
     */

#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Calculate total transfer size for cache flush
    // User input: ysize = 1 means 1 row, ysize = 2 means 2 rows, etc.
    // Total size = xsize * ysize (user input)
    uint32_t src_total_size = config->src.xsize * config->src.ysize;
    uint32_t dst_total_size = config->dst.xsize * config->dst.ysize;

    /*
     * Address-alias note: the cache is indexed by the CPU-side 0x2Cxxxxxx
     * alias, so this flush MUST use the original start_addr (0x2C) passed by
     * the caller, never the 0x28 alias written to the hardware - otherwise it
     * would flush the wrong cache lines and DMA would read stale data. Writing
     * 0x28 to the register while flushing 0x2C for the cache is intentional;
     * do not "unify" them to a single alias in later maintenance.
     */
    // Flush source cache to ensure DMA reads latest CPU-written data
    if (src_total_size > 0) {
        flush_dcache((void *)config->src.start_addr, src_total_size);
    }

    // Flush destination cache to prepare for DMA write
    // This ensures any dirty cache lines are written back before DMA overwrites memory
    if (dst_total_size > 0) {
        flush_dcache((void *)config->dst.start_addr, dst_total_size);
    }
#endif
    __DSB();

    // Create a modified config with ysize decremented by 1 for hardware
    // Hardware expects: ysize = 0 means 1 row, ysize = 1 means 2 rows, etc.
    // User input: ysize = 1 means 1 row, ysize = 2 means 2 rows, etc.
    hpdma_config_t hw_config = *config;
    hw_config.src.ysize = config->src.ysize - 1;
    hw_config.dst.ysize = config->dst.ysize - 1;

    /*
     * S0 (HPDMA SMP review):
     *   hpdma_id_init_common() and hpdma_hal_init_dma() between them
     *   issue ~14 RMW writes that touch ctrl/req_mux bitfields,
     *   id_init_bits software state, and the channel's address /
     *   xsize / ysize / step registers. Without serialization, a peer
     *   core's task or the routed-core HPDMA ISR (which writes ctrl on
     *   bus_err / fifo_err) can interleave with this configuration and
     *   produce ctrl/req_mux corruption that surfaces only as silent
     *   data loss or random bus_err on the next start.
     */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_id_init_common(id);
    bk_err_t ret = hpdma_hal_init_dma(&s_hpdma.hal, id, &hw_config);
    hpdma_exit_critical(int_mask);
    return ret;
}

bk_err_t bk_hpdma_deinit(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);
    /*
     * S0 (HPDMA SMP review):
     *   hpdma_id_deinit_common writes ctrl / req_mux / status /
     *   id_init_bits. hpdma_clear_channel_interrupt_state already
     *   takes the same lock internally; the recursive spinlock makes
     *   this a no-op on the inner call.
     */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_id_deinit_common(id);
    hpdma_exit_critical(int_mask);

    hpdma_clear_channel_interrupt_state(id);
    return BK_OK;
}

bk_err_t bk_hpdma_start(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    /*
     * P1 (HPDMA review):
     *   Macro existed but was never used. Starting an un-initialized
     *   channel writes a still-zeroed ctrl reg and produces an immediate
     *   bus_err.
     */
    HPDMA_RETURN_ON_ID_NOT_INIT(0, id);

    /*
     * S0 (HPDMA SMP review):
     *   apply_smem_burst_policy_at_start may downgrade burst_len in
     *   req_mux (RMW), and hpdma_hal_start_common writes ctrl.enable
     *   (RMW). On bus_err / fifo_err the routed-core ISR also touches
     *   ctrl on the same channel via stop_disable_only, so this whole
     *   "apply policy + flag DSB + raise enable" sequence must run as
     *   one atomic block from the hardware's point of view.
     */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_apply_smem_burst_policy_at_start(id);
    __DSB();
    hpdma_hal_start_common(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_stop(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    /*
     * P1 (HPDMA review): see bk_hpdma_start. Stopping an un-init'd
     *   channel is harmless on hardware but masks API misuse.
     */
    HPDMA_RETURN_ON_ID_NOT_INIT(0, id);

    /* S0 (HPDMA SMP review): see bk_hpdma_start. stop_common writes
     * ctrl.enable (RMW) and clears status (W1C). */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_stop_common(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

uint32_t bk_hpdma_get_enable_status(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();

    uint32_t ret;
    ret = hpdma_hal_get_enable_status(&s_hpdma.hal, id);
    return ret;
}

#define HPDMA_MAX_BUSY_TIME (10000)  //us
/*
 * S1 (HPDMA stability review):
 *   Previously this returned uint32_t (the spin count) and treated timeout
 *   as "idle". Callers that change channel registers right after were left
 *   to touch a still-running DMA, producing bus_err / wrong addr writes.
 *
 *   Now it returns bk_err_t:
 *     - HPDMA_WORK_MODE_SINGLE : poll dma_en up to HPDMA_MAX_BUSY_TIME us;
 *                                 timeout -> BK_ERR_HPDMA_TIMEOUT.
 *     - HPDMA_WORK_MODE_REPEAT : hardware never auto-clears dma_en, so we
 *                                 must stop it first, then poll briefly.
 */
bk_err_t hpdma_wait_to_idle(hpdma_id_t id)
{
	hpdma_work_mode_t mode = hpdma_hal_get_work_mode(&s_hpdma.hal, id);

	if (mode == HPDMA_WORK_MODE_REPEAT)
	{
		/*
		 * S1: repeat mode keeps dma_en=1 by design; stop it before polling.
		 * S1/D: use stop_disable_only so we do not silently W1C-clear the
		 *       channel's half/finish/bus/fifo interrupt status (which the
		 *       caller may still want to consume).
		 * S0 (HPDMA SMP review):
		 *       stop_disable_only is a ctrl RMW; serialize against the
		 *       routed-core ISR (which writes ctrl on bus_err/fifo_err)
		 *       and any peer-core task. The polling loop below reads
		 *       ctrl.enable as a single 32-bit load, so it is safe to
		 *       observe outside the critical section - and must be,
		 *       because it can spin up to HPDMA_MAX_BUSY_TIME us.
		 */
		uint32_t int_mask = hpdma_enter_critical();
		hpdma_hal_stop_disable_only(&s_hpdma.hal, id);
		hpdma_exit_critical(int_mask);
	}

	uint32_t i = 0;
	while (hpdma_hal_get_enable_status(&s_hpdma.hal, id))
	{
		bk_delay_us(1);

		i++;
		if (i > HPDMA_MAX_BUSY_TIME)
		{
			HPDMA_LOGE("ch%d busy,remain len=%d,dst_addr=%x\r\n", id,
				   hpdma_hal_get_remain_len(&s_hpdma.hal, id),
				   hpdma_hal_get_dest_write_addr(&s_hpdma.hal, id));
			/* S1: do NOT pretend the channel is idle on timeout. */
			return BK_ERR_HPDMA_TIMEOUT;
		}
	}

	return BK_OK;
}

bk_err_t bk_hpdma_wait_to_idle(hpdma_id_t id)
{
	HPDMA_RETURN_ON_NOT_INIT();
	HPDMA_RETURN_ON_INVALID_ID(id);
	return hpdma_wait_to_idle(id);
}

/*
 * S0 (HPDMA SMP review):
 *   All enable/disable per-interrupt helpers below RMW the channel's
 *   req_mux register (one bitfield each). hpdma_id_enable_interrupt_common
 *   additionally RMWs the controller-wide int_allocate register and the
 *   system intc enable. The routed-core HPDMA ISR can preempt any of
 *   these on a peer core. Wrap the full sequence so the bitfield write
 *   and the W1C status clear are atomic against ISR, peer tasks, and
 *   peer-channel concurrent configuration.
 */
bk_err_t bk_hpdma_enable_finish_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_id_enable_interrupt_common(id);
    hpdma_hal_enable_finish_interrupt(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_finish_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);

    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_disable_finish_interrupt(&s_hpdma.hal, id);
    hpdma_hal_clear_finish_interrupt_status(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    __DSB();
    return BK_OK;
}

bk_err_t bk_hpdma_enable_half_finish_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_id_enable_interrupt_common(id);
    hpdma_hal_enable_half_finish_interrupt(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_half_finish_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);

    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_disable_half_finish_interrupt(&s_hpdma.hal, id);
    hpdma_hal_clear_half_finish_interrupt_status(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    __DSB();
    return BK_OK;
}

bk_err_t bk_hpdma_enable_bus_err_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_id_enable_interrupt_common(id);
    hpdma_hal_enable_bus_err_interrupt(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_bus_err_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);

    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_disable_bus_err_interrupt(&s_hpdma.hal, id);
    hpdma_hal_clear_bus_err_interrupt_status(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    __DSB();
    return BK_OK;
}

bk_err_t bk_hpdma_register_isr(hpdma_id_t id, hpdma_isr_t half_finish_isr, void *half_finish_data,
                                hpdma_isr_t finish_isr, void *finish_data)
{
    HPDMA_RETURN_ON_NOT_INIT();

    HPDMA_RETURN_ON_INVALID_ID(id);
    /*
     * S0 (HPDMA SMP review):
     *   GLOBAL_INT_DISABLE only masks IRQs on the calling core. The HPDMA
     *   ISR is routed to a fixed core (see hpdma_id_enable_interrupt_common),
     *   so on FreeRTOS-SMP a task running on a different core could observe
     *   a half-updated {callback, user_data} pair (e.g. new callback +
     *   stale user_data) and dereference into freed memory. Use the same
     *   driver-wide critical section the ISR uses so the update is atomic
     *   from every observer's point of view.
     */
    uint32_t int_mask = hpdma_enter_critical();
    s_hpdma_half_finish_isr[id].callback = half_finish_isr;
    s_hpdma_half_finish_isr[id].user_data = half_finish_data;
    s_hpdma_finish_isr[id].callback = finish_isr;
    s_hpdma_finish_isr[id].user_data = finish_data;
    hpdma_exit_critical(int_mask);

    return BK_OK;
}

bk_err_t bk_hpdma_register_bus_err_isr(hpdma_id_t id, hpdma_isr_t bus_err_isr, void *user_data)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    /* S0 (HPDMA SMP review): see bk_hpdma_register_isr. */
    uint32_t int_mask = hpdma_enter_critical();
    s_hpdma_bus_err_isr[id].callback = bus_err_isr;
    s_hpdma_bus_err_isr[id].user_data = user_data;
    hpdma_exit_critical(int_mask);

    return BK_OK;
}

/*
 * P0 (HPDMA review):
 *   Companion of bk_hpdma_register_bus_err_isr for the newly-exposed
 *   fifo_err class. Clients that opt-in via
 *   bk_hpdma_enable_fifo_err_interrupt() can use this to be notified
 *   from the ISR after the engine has been halted.
 */
bk_err_t bk_hpdma_register_fifo_err_isr(hpdma_id_t id, hpdma_isr_t fifo_err_isr, void *user_data)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    /* S0 (HPDMA SMP review): see bk_hpdma_register_isr. */
    uint32_t int_mask = hpdma_enter_critical();
    s_hpdma_fifo_err_isr[id].callback = fifo_err_isr;
    s_hpdma_fifo_err_isr[id].user_data = user_data;
    hpdma_exit_critical(int_mask);

    return BK_OK;
}

bk_err_t bk_hpdma_enable_fifo_err_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_id_enable_interrupt_common(id);
    hpdma_hal_enable_fifo_err_interrupt(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_fifo_err_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);

    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_disable_fifo_err_interrupt(&s_hpdma.hal, id);
    hpdma_hal_clear_fifo_err_interrupt_status(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    __DSB();
    return BK_OK;
}

bk_err_t bk_hpdma_set_src_start_addr(hpdma_id_t id, uint32_t start_addr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    /*
     * S1/E (HPDMA review):
     *   Asymmetric with set_dest_start_addr previously: set_src skipped
     *   wait_to_idle entirely. Changing src_start_addr mid-transfer is
     *   just as dangerous (immediate bus_err / wrong source read), so
     *   mirror the wait + error-return pattern.
     */
    bk_err_t wait_ret = hpdma_wait_to_idle(id);
    if (wait_ret != BK_OK) {
        HPDMA_LOGE("set_src_start_addr: ch%d busy, refuse to update\r\n", id);
        return wait_ret;
    }
    /*
     * S0 (HPDMA SMP review):
     *   start_addr is a full 32-bit register write so the per-store is
     *   atomic, but the lock here keeps the {wait_to_idle outcome ->
     *   write address} contract valid against a peer-core start that
     *   would otherwise re-arm the channel between our wait and our
     *   write.
     */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_src_start_addr(&s_hpdma.hal, id, start_addr);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_set_dest_start_addr(hpdma_id_t id, uint32_t start_addr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    /*
     * S1/E (HPDMA review):
     *   Previously the return value of hpdma_wait_to_idle was thrown
     *   away, so a busy / timed-out channel would still get its
     *   dest_start_addr re-written - exactly the bus_err / corruption
     *   hazard that wait_to_idle was changed to report. Now we refuse
     *   to touch the register and propagate the error up.
     */
    bk_err_t wait_ret = hpdma_wait_to_idle(id);
    if (wait_ret != BK_OK) {
        HPDMA_LOGE("set_dest_start_addr: ch%d busy, refuse to update\r\n", id);
        return wait_ret;
    }
    /* S0 (HPDMA SMP review): see bk_hpdma_set_src_start_addr. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_dest_start_addr(&s_hpdma.hal, id, start_addr);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

/*
 * S0 (HPDMA SMP review):
 *   The four addr_inc / addr_loop pairs and the data-width setters all
 *   touch ctrl bitfields (RMW). The routed-core ISR also writes ctrl
 *   on bus_err / fifo_err (via stop_disable_only) so each setter must
 *   serialize with both peer tasks and the ISR.
 */
bk_err_t bk_hpdma_enable_src_addr_increase(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_enable_src_addr_inc(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_src_addr_increase(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_disable_src_addr_inc(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_enable_src_addr_loop(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_enable_src_addr_loop(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_src_addr_loop(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_disable_src_addr_loop(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_enable_dest_addr_increase(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_enable_dest_addr_inc(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_dest_addr_increase(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_disable_dest_addr_inc(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_enable_dest_addr_loop(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_enable_dest_addr_loop(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_dest_addr_loop(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_disable_dest_addr_loop(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

uint32_t bk_hpdma_get_remain_len(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    return hpdma_hal_get_remain_len(&s_hpdma.hal, id);
}

bk_err_t hpdma_set_src_pause_addr(hpdma_id_t id, uint32_t addr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /* S0 (HPDMA SMP review): full 32-bit register write but lock to
     * keep configuration ordering against the routed-core ISR. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_src_pause_addr(&s_hpdma.hal, id, addr);
    hpdma_exit_critical(int_mask);

    return BK_OK;
}

bk_err_t hpdma_set_dst_pause_addr(hpdma_id_t id, uint32_t addr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /* S0 (HPDMA SMP review): see hpdma_set_src_pause_addr. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_dest_pause_addr(&s_hpdma.hal, id, addr);
    hpdma_exit_critical(int_mask);

    return BK_OK;
}

uint32_t hpdma_get_src_read_addr(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    return hpdma_hal_get_src_read_addr(&s_hpdma.hal, id);
}

uint32_t hpdma_get_dest_write_addr(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    return hpdma_hal_get_dest_write_addr(&s_hpdma.hal, id);
}

bk_err_t bk_hpdma_set_src_data_width(hpdma_id_t id, hpdma_data_width_t data_width)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /* S1/E (HPDMA review): see bk_hpdma_set_dest_start_addr. */
    bk_err_t wait_ret = hpdma_wait_to_idle(id);
    if (wait_ret != BK_OK) {
        HPDMA_LOGE("set_src_data_width: ch%d busy, refuse to update\r\n", id);
        return wait_ret;
    }
    /* S0 (HPDMA SMP review): ctrl.src_data_width is a 3-bit RMW. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_src_data_width(&s_hpdma.hal, id, data_width);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_set_dest_data_width(hpdma_id_t id, hpdma_data_width_t data_width)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /* S1/E (HPDMA review): see bk_hpdma_set_dest_start_addr. */
    bk_err_t wait_ret = hpdma_wait_to_idle(id);
    if (wait_ret != BK_OK) {
        HPDMA_LOGE("set_dest_data_width: ch%d busy, refuse to update\r\n", id);
        return wait_ret;
    }
    /* S0 (HPDMA SMP review): ctrl.dest_data_width is a 3-bit RMW. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_dest_data_width(&s_hpdma.hal, id, data_width);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}


bk_err_t bk_hpdma_set_pixel_trans_type(hpdma_id_t id, hpdma_pixel_trans_type_t type)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /* S0 (HPDMA SMP review): req_mux.pixel_trans_type is a 2-bit RMW. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_pixel_trans_type(&s_hpdma.hal, id, type);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

uint32_t bk_hpdma_get_pixel_trans_type(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);


    return hpdma_hal_get_pixel_trans_type(&s_hpdma.hal, id);
}

bk_err_t bk_hpdma_bus_err_int_enable(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /* S0 (HPDMA SMP review): req_mux.bus_err_int_en is a 1-bit RMW. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_bus_err_int_enable(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_bus_err_int_diable(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /* S0 (HPDMA SMP review): req_mux.bus_err_int_en is a 1-bit RMW. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_bus_err_int_disable(&s_hpdma.hal, id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_set_dest_sec_attr(hpdma_id_t id, hpdma_sec_attr_t attr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /* S0 (HPDMA SMP review): req_mux.dest_sec_attr is a 1-bit RMW. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_dest_sec_attr(&s_hpdma.hal, id, attr);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_set_src_sec_attr(hpdma_id_t id, hpdma_sec_attr_t attr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /* S0 (HPDMA SMP review): req_mux.src_sec_attr is a 1-bit RMW. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_src_sec_attr(&s_hpdma.hal, id, attr);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

/*
 * Same-physical-SMEM burst override (HPDMA review request):
 *
 *   Hardware constraint: when HPDMA's source and destination both live in the
 *   *same* physical SMEM block (smem3/4/5/6 on BK7259), bursts longer than
 *   INC8 expose a read/write contention inside that SMEM and can corrupt
 *   data. The contention exists for the 3 alias windows that map to the same
 *   physical block:
 *     - main window      : +0x00000000
 *     - NS / peer view   : +SOC_S_NS_ADDR_DIFF (0x10000000)
 *     - cacheable view   : +0x04000000
 *
 *   This helper maps an address to a physical SMEM block id (3..6, encoded
 *   as 0..3) regardless of which alias window it sits in. -1 means "not in
 *   smem3..smem6".
 *
 *   Callers must only consult the *start* address as the user requested -
 *   range overlap across blocks is not handled here.
 */
static int hpdma_resolve_smem_block(uint32_t addr)
{
    /* Reduce the 3 alias windows to one canonical secure / main view. */
    if (addr >= 0x38000000 && addr < 0x40000000) {
        addr -= SOC_S_NS_ADDR_DIFF;          /* NS  -> secure */
    } else if (addr >= 0x2C000000 && addr < 0x30000000) {
        addr -= 0x04000000;                  /* cacheable -> non-cacheable */
    }

    const struct {
        uint32_t base;
        uint32_t size;
    } smem_blocks[] = {
        { 0x28100000, SOC_SRAM3_DATA_SIZE },  /* smem3 */
        { 0x28140000, SOC_SRAM4_DATA_SIZE },  /* smem4 */
        { 0x28180000, SOC_SRAM5_DATA_SIZE },  /* smem5 */
        { 0x281C0000, SOC_SRAM6_DATA_SIZE },  /* smem6 */
    };

    for (int i = 0; i < (int)(sizeof(smem_blocks) / sizeof(smem_blocks[0])); i++) {
        if (addr >= smem_blocks[i].base &&
            addr < smem_blocks[i].base + smem_blocks[i].size) {
            return i;
        }
    }
    return -1;
}

/*
 * Return BK_OK and fill *src and *dst with the start addresses the channel
 * will actually use:
 *   - Plain (non-link) mode: read from channel registers.
 *   - Link mode: channel registers may still be 0 between set_burst() and
 *     link_transfer(); fall back to the first descriptor pointed to by
 *     next_ll_addr.
 */
static bk_err_t hpdma_get_effective_addrs(hpdma_id_t id, uint32_t *src, uint32_t *dst)
{
    uint32_t s = hpdma_hal_get_src_start_addr(&s_hpdma.hal, id);
    uint32_t d = hpdma_hal_get_dest_start_addr(&s_hpdma.hal, id);

    if (s == 0 || d == 0) {
        uint32_t ll = hpdma_hal_get_next_ll_addr(&s_hpdma.hal, id);
        if (ll != 0) {
            const hpdma_descriptor_t *desc = (const hpdma_descriptor_t *)ll;
            if (s == 0) s = desc->src_addr;
            if (d == 0) d = desc->dst_addr;
        }
    }

    *src = s;
    *dst = d;
    return BK_OK;
}

/*
 * New P0 (HPDMA review - SMEM burst policy timing fix):
 *
 *   The previous implementation hooked the "same physical SMEM block ->
 *   force INC8" policy inside bk_hpdma_set_src/dest_burst_len(). All
 *   real callers (LVGL, GPU) invoke set_burst_len BEFORE link_transfer
 *   / start, when the channel's src/dst registers are either zero (fresh
 *   alloc) or stale (previous transfer's addresses). The policy
 *   therefore either silently no-op'd (first transfer) or judged the
 *   wrong addresses (subsequent transfers), leaving INC16 active for
 *   transfers that genuinely needed INC8 - the exact SMEM contention
 *   the policy was supposed to prevent.
 *
 *   Fix:
 *     - set_burst_len() reverts to its plain meaning: write the
 *       hardware register to the requested value.
 *     - apply_smem_burst_policy_at_start() is invoked from
 *       bk_hpdma_start() and bk_hpdma_link_transfer() - i.e. the last
 *       moments before the engine actually starts. By then the channel
 *       has the real src/dst (plain mode: written by hpdma_hal_init_dma;
 *       link mode: derivable via next_ll_addr -> first descriptor),
 *       so the policy sees ground truth and may downgrade the already-
 *       programmed burst_len from INC16 to INC8 in-place.
 *
 *   The user-visible API surface is unchanged.
 */
static void hpdma_apply_smem_burst_policy_at_start(hpdma_id_t id)
{
    uint32_t src_addr = 0;
    uint32_t dst_addr = 0;

    if (hpdma_get_effective_addrs(id, &src_addr, &dst_addr) != BK_OK) {
        return;
    }
    if (src_addr == 0 || dst_addr == 0) {
        return;
    }

    int src_blk = hpdma_resolve_smem_block(src_addr);
    int dst_blk = hpdma_resolve_smem_block(dst_addr);
    if (src_blk < 0 || dst_blk < 0 || src_blk != dst_blk) {
        return;
    }

    uint32_t src_burst = hpdma_hal_get_src_burst_len(&s_hpdma.hal, id);
    uint32_t dst_burst = hpdma_hal_get_dest_burst_len(&s_hpdma.hal, id);

    if (src_burst > HPDMA_BURST_LEN_INC8) {
        HPDMA_LOGI("ch%d src in smem%d, downgrade src burst %u->INC8\r\n",
                   id, 3 + src_blk, src_burst);
        hpdma_hal_set_src_burst_len(&s_hpdma.hal, id, HPDMA_BURST_LEN_INC8);
    }
    if (dst_burst > HPDMA_BURST_LEN_INC8) {
        HPDMA_LOGI("ch%d dst in smem%d, downgrade dst burst %u->INC8\r\n",
                   id, 3 + dst_blk, dst_burst);
        hpdma_hal_set_dest_burst_len(&s_hpdma.hal, id, HPDMA_BURST_LEN_INC8);
    }
}

bk_err_t bk_hpdma_set_dest_burst_len(hpdma_id_t id, hpdma_burst_len_t len)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /*
     * New P0 (HPDMA review):
     *   The SMEM-same-block override moved out of this setter and into
     *   apply_smem_burst_policy_at_start(), which runs at real start
     *   time when the channel actually knows its src/dst addresses.
     *   Here we just write what the caller asked for.
     * S0 (HPDMA SMP review):
     *   req_mux.dtst_burst_len is a 2-bit RMW. apply_smem_burst_policy_at_start
     *   runs from bk_hpdma_start (already locked) and from
     *   hpdma_memcpy_by_chnl (also locked) - both already hold this
     *   lock, the recursive spinlock makes that safe.
     */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_dest_burst_len(&s_hpdma.hal, id, len);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

uint32_t bk_hpdma_get_dest_burst_len(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    return hpdma_hal_get_dest_burst_len(&s_hpdma.hal, id);
}

bk_err_t bk_hpdma_set_src_burst_len(hpdma_id_t id, hpdma_burst_len_t len)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /* New P0 (HPDMA review): see bk_hpdma_set_dest_burst_len. */
    /* S0 (HPDMA SMP review): req_mux.src_burst_len is a 2-bit RMW. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_src_burst_len(&s_hpdma.hal, id, len);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

uint32_t bk_hpdma_get_src_burst_len(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    return hpdma_hal_get_src_burst_len(&s_hpdma.hal, id);
}

bk_err_t bk_hpdma_set_sec_attr(hpdma_id_t id, hpdma_sec_attr_t attr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /*
     * S0 (HPDMA SMP review):
     *   secure_attr is a *controller-wide* register where each channel
     *   owns one bit out of 4. The setter is a per-bit RMW. Two cores
     *   configuring different channels on this controller concurrently
     *   would otherwise race and silently lose one of the writes.
     */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_sec_attr(&s_hpdma.hal, id, attr);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_set_privileged_attr(hpdma_id_t id, hpdma_sec_attr_t attr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    /* S0 (HPDMA SMP review): see bk_hpdma_set_sec_attr. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_privileged_attr(&s_hpdma.hal, id, attr);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

bk_err_t bk_hpdma_set_int_allocate(hpdma_id_t id, hpdma_int_id_t int_id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    /*
     * S0 (HPDMA SMP review):
     *   int_allocate is a controller-wide register where each channel
     *   owns 3 contiguous bits. The setter is a 3-bit RMW. Concurrent
     *   peer-channel configuration from different cores would clobber
     *   each other.
     */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_int_allocate(&s_hpdma.hal, id, int_id);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

hpdma_int_id_t bk_hpdma_get_int_allocate(hpdma_id_t id)
{
    return hpdma_hal_get_int_allocate(&s_hpdma.hal, id);
}


uint32_t bk_hpdma_get_repeat_wr_pause(hpdma_id_t id)
{
	return hpdma_hal_repeat_wr_pause(&s_hpdma.hal, id);
}

uint32_t bk_hpdma_get_repeat_rd_pause(hpdma_id_t id)
{
	return hpdma_hal_repeat_rd_pause(&s_hpdma.hal, id);
}

uint32_t bk_hpdma_get_finish_interrupt_cnt(hpdma_id_t id)
{
	return hpdma_hal_finish_interrupt_cnt(&s_hpdma.hal, id);
}

uint32_t bk_hpdma_get_half_finish_interrupt_cnt(hpdma_id_t id)
{
	return hpdma_hal_half_finish_interrupt_cnt(&s_hpdma.hal, id);
}

/* ------------------------------------------------------------------ *
 * memcpy completion helpers (link-mode split + interrupt/semaphore wait)
 * ------------------------------------------------------------------ */

/*
 * HPDMA single-shot xsize is a 16-bit field: one descriptor / one single-mode
 * transfer can move at most 0xFFFF bytes. Anything larger is split into
 * 0xF000-byte chunks (safely below the limit) and chained as a linked list.
 */
#define HPDMA_SINGLE_MAX_XFER   0xF000U   /* 61440, < 16-bit xsize limit 0xFFFF */
#define HPDMA_DONE_WAIT_MS      1000U     /* semaphore wait bound for one transfer */

/*
 * A linked-list descriptor is a 2D transfer: it moves xsize bytes per row for
 * ysize rows, advancing the row start by (xsize + step). With step == 0 the
 * rows are back-to-back, so one descriptor copies xsize*ysize *contiguous*
 * bytes. Both fields are 16-bit, so a single node can move up to
 * HPDMA_LINK_ROW_BYTES * HPDMA_LINK_MAX_ROWS (~4 GiB) instead of the old
 * 0xF000-per-node limit -- i.e. one or two descriptors cover any 32-bit length.
 * HPDMA_LINK_ROW_BYTES is kept 16-byte aligned so every row is a whole number
 * of 128-bit words (the width the link path configures).
 */
#define HPDMA_LINK_ROW_BYTES    0xFFF0U   /* per-row xsize, 16-byte aligned, <= 0xFFFF */
#define HPDMA_LINK_MAX_ROWS     0xFFFFU   /* 16-bit ysize limit */

/*
 * Per-channel completion semaphore. Lazily created the first time a channel is
 * used on the interrupt-wait path and then reused. Indexed by logic channel id
 * (== the channel value the ISR reports on unit 0).
 */
static beken_semaphore_t s_hpdma_done_sem[SOC_HPDMA_CHAN_NUM_PER_UNIT] = {NULL};

typedef struct {
    hpdma_isr_t saved_cb;
    void       *saved_data;
    bool        armed;
} hpdma_done_ctx_t;

/* Driver-owned finish callback: wake the task blocked in hpdma_wait_done(). */
static void hpdma_internal_finish_isr(hpdma_id_t id, void *user_data)
{
    (void)user_data;
    if ((id < SOC_HPDMA_CHAN_NUM_PER_UNIT) && (s_hpdma_done_sem[id] != NULL)) {
        rtos_set_semaphore(&s_hpdma_done_sem[id]);
    }
}

/*
 * True only when it is safe to block the caller on a semaphore: task context,
 * scheduler running and not suspended. Otherwise the caller must busy-wait.
 */
static bool hpdma_can_block(void)
{
    return (!rtos_is_in_interrupt_context()
            && rtos_is_scheduler_started()
            && !rtos_is_scheduler_suspended());
}

/*
 * Install the internal finish callback + enable the finish interrupt so the
 * transfer completion posts the per-channel semaphore. Saves any pre-existing
 * finish callback so an externally supplied channel is restored afterwards.
 * Must be called from task context (creates the semaphore on first use).
 *
 * finish_int_en, dest_req_mux and src_req_mux are independent bitfields of the
 * same req_mux register, so arming here before bk_hpdma_init() (which only
 * touches the req_mux dev fields) is preserved across init.
 */
static bk_err_t hpdma_arm_completion(hpdma_id_t id, hpdma_done_ctx_t *ctx)
{
    ctx->armed = false;
    if (id >= SOC_HPDMA_CHAN_NUM_PER_UNIT) {
        return BK_ERR_HPDMA_ID;
    }

    if (s_hpdma_done_sem[id] == NULL) {
        if (rtos_init_semaphore(&s_hpdma_done_sem[id], 1) != BK_OK) {
            return BK_FAIL;
        }
    }

    /* Drop any stale post left by a previous timed-out transfer. */
    while (rtos_get_semaphore(&s_hpdma_done_sem[id], BEKEN_NO_WAIT) == BK_OK) {
    }

    uint32_t int_mask = hpdma_enter_critical();
    ctx->saved_cb = s_hpdma_finish_isr[id].callback;
    ctx->saved_data = s_hpdma_finish_isr[id].user_data;
    s_hpdma_finish_isr[id].callback = hpdma_internal_finish_isr;
    s_hpdma_finish_isr[id].user_data = NULL;
    hpdma_exit_critical(int_mask);

    bk_hpdma_enable_finish_interrupt(id);
    ctx->armed = true;
    return BK_OK;
}

static void hpdma_disarm_completion(hpdma_id_t id, hpdma_done_ctx_t *ctx)
{
    if (!ctx->armed) {
        return;
    }
    bk_hpdma_disable_finish_interrupt(id);

    uint32_t int_mask = hpdma_enter_critical();
    s_hpdma_finish_isr[id].callback = ctx->saved_cb;
    s_hpdma_finish_isr[id].user_data = ctx->saved_data;
    hpdma_exit_critical(int_mask);
    ctx->armed = false;
}

/*
 * Wait for the channel to finish. When use_sem is true the caller has armed the
 * finish interrupt via hpdma_arm_completion() and we block on the semaphore
 * (CPU is yielded). Otherwise we fall back to the bounded busy-poll on dma_en.
 */
static bk_err_t hpdma_wait_done(hpdma_id_t id, uint32_t timeout_ms, bool use_sem)
{
    if (use_sem) {
        if (rtos_get_semaphore(&s_hpdma_done_sem[id], timeout_ms) != BK_OK) {
            HPDMA_LOGE("wait_done: ch%d sem timeout, remain=%u dst_wr=0x%x\r\n", id,
                       hpdma_hal_get_remain_len(&s_hpdma.hal, id),
                       hpdma_hal_get_dest_write_addr(&s_hpdma.hal, id));
            uint32_t int_mask = hpdma_enter_critical();
            hpdma_hal_stop_disable_only(&s_hpdma.hal, id);
            hpdma_exit_critical(int_mask);
            return BK_ERR_HPDMA_TIMEOUT;
        }
        return BK_OK;
    }

    uint32_t spun_us = 0;
    while (hpdma_hal_get_enable_status(&s_hpdma.hal, id)) {
        bk_delay_us(1);
        if (++spun_us > HPDMA_MAX_BUSY_TIME) {
            HPDMA_LOGE("wait_done: ch%d hung; remain=%u dst_wr=0x%x\r\n", id,
                       hpdma_hal_get_remain_len(&s_hpdma.hal, id),
                       hpdma_hal_get_dest_write_addr(&s_hpdma.hal, id));
            uint32_t int_mask = hpdma_enter_critical();
            hpdma_hal_stop_disable_only(&s_hpdma.hal, id);
            hpdma_exit_critical(int_mask);
            return BK_ERR_HPDMA_TIMEOUT;
        }
    }
    return BK_OK;
}

/*
 * Copy >0 bytes via an HPDMA linked list on the given (already allocated)
 * channel. Each descriptor is a step==0 2D transfer, so it moves
 * HPDMA_LINK_ROW_BYTES bytes per row over up to HPDMA_LINK_MAX_ROWS contiguous
 * rows (xsize*ysize bytes total). len is therefore covered by a handful of
 * descriptors: the bulk goes into full-width row descriptors and any trailing
 * (len % HPDMA_LINK_ROW_BYTES) bytes go into a final single-row node. The
 * finish interrupt is armed on the LAST descriptor only, the whole chain is
 * kicked once, and we block on the completion semaphore (task context) or
 * busy-wait (otherwise). This is how transfers larger than the 16-bit
 * single-shot limit are handled.
 */
static bk_err_t hpdma_memcpy_link_by_chnl(void *out, const void *in, uint32_t len, hpdma_id_t chnl)
{
    const uint8_t *s = (const uint8_t *)in;
    uint8_t *d = (uint8_t *)out;

    uint32_t full_rows = len / HPDMA_LINK_ROW_BYTES;
    uint32_t tail = len % HPDMA_LINK_ROW_BYTES;
    uint32_t row_descs = (full_rows + HPDMA_LINK_MAX_ROWS - 1U) / HPDMA_LINK_MAX_ROWS;
    uint32_t desc_cnt = row_descs + (tail ? 1U : 0U);

    void *table = bk_hpdma_link_init(desc_cnt);
    if (table == NULL) {
        return BK_ERR_NO_MEM;
    }

    uint32_t idx = 0;
    uint32_t off = 0;
    uint32_t rows_left = full_rows;
    while (rows_left > 0U) {
        uint32_t rows = (rows_left > HPDMA_LINK_MAX_ROWS) ? HPDMA_LINK_MAX_ROWS : rows_left;
        uint32_t bytes = rows * HPDMA_LINK_ROW_BYTES;
        hpdma_link_config_t cfg;

        os_memset(&cfg, 0, sizeof(cfg));
        cfg.src_addr = (uint32_t)(uintptr_t)(s + off);
        cfg.dst_addr = (uint32_t)(uintptr_t)(d + off);
        cfg.src_xsize = (uint16_t)HPDMA_LINK_ROW_BYTES;
        cfg.dst_xsize = (uint16_t)HPDMA_LINK_ROW_BYTES;
        cfg.src_ysize = (uint16_t)rows;
        cfg.dst_ysize = (uint16_t)rows;
        cfg.src_step = 0U;   /* step==0 => rows are contiguous */
        cfg.dst_step = 0U;
        /* Only the last node raises finish, so we wake up exactly once. */
        cfg.finish_int_en = (idx == desc_cnt - 1U) ? 1U : 0U;
        cfg.half_finish_int_en = 0U;

        bk_err_t set_ret = bk_hpdma_link_set_desc(table, idx, &cfg);
        if (set_ret != BK_OK) {
            bk_hpdma_link_deinit(table);
            return set_ret;
        }
        off += bytes;
        rows_left -= rows;
        idx++;
    }

    if (tail) {
        hpdma_link_config_t cfg;

        os_memset(&cfg, 0, sizeof(cfg));
        cfg.src_addr = (uint32_t)(uintptr_t)(s + off);
        cfg.dst_addr = (uint32_t)(uintptr_t)(d + off);
        cfg.src_xsize = (uint16_t)tail;
        cfg.dst_xsize = (uint16_t)tail;
        cfg.src_ysize = 1U;
        cfg.dst_ysize = 1U;
        cfg.src_step = 0U;
        cfg.dst_step = 0U;
        cfg.finish_int_en = (idx == desc_cnt - 1U) ? 1U : 0U;
        cfg.half_finish_int_en = 0U;

        bk_err_t set_ret = bk_hpdma_link_set_desc(table, idx, &cfg);
        if (set_ret != BK_OK) {
            bk_hpdma_link_deinit(table);
            return set_ret;
        }
        idx++;
    }

    hpdma_done_ctx_t ctx = {0};
    bool use_sem = hpdma_can_block();
    if (use_sem && (hpdma_arm_completion(chnl, &ctx) != BK_OK)) {
        use_sem = false;   /* fall back to busy-wait if arming failed */
    }

    bk_err_t ret = bk_hpdma_link_transfer(chnl, table);
    if (ret != BK_OK) {
        hpdma_disarm_completion(chnl, &ctx);
        bk_hpdma_link_deinit(table);
        return ret;
    }

    ret = hpdma_wait_done(chnl, HPDMA_DONE_WAIT_MS, use_sem);

    hpdma_disarm_completion(chnl, &ctx);

#if CONFIG_SUPPORT_CACHEABLE_SRAM
    if (ret == BK_OK) {
        /* Invalidate destination cache so the CPU sees DMA-written data. */
        flush_dcache((void *)out, len);
    }
#endif
    __DMB();

    bk_hpdma_link_deinit(table);
    return ret;
}

bk_err_t hpdma_memcpy_by_chnl(void *out, const void *in, uint32_t len, hpdma_id_t cpy_chnl)
{
    hpdma_config_t hpdma_config;

    if (out == NULL || in == NULL) {
        return BK_ERR_NULL_PARAM;
    }
    if (len == 0) {
        return BK_OK;
    }
    /*
     * Single-shot xsize is a 16-bit field. Refuse >0xFFFF here instead of
     * silently truncating len into xsize; callers with larger transfers must
     * use the linked-list path (hpdma_memcpy_link_by_chnl / bk_hpdma_memcpy).
     */
    if (len > 0xFFFFU) {
        HPDMA_LOGE("memcpy_by_chnl: len %u exceeds single-shot 0xFFFF; use link mode\r\n", len);
        return BK_ERR_HPDMA_TRANS_LEN;
    }

    os_memset(&hpdma_config, 0, sizeof(hpdma_config_t));

    hpdma_config.mode = HPDMA_WORK_MODE_SINGLE;
    hpdma_config.chan_prio = 0;

    hpdma_config.src.dev = HPDMA_DEV_DTCM;
    hpdma_config.src.width = HPDMA_DATA_WIDTH_32BITS;
    hpdma_config.src.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    hpdma_config.src.start_addr = (uintptr_t)in;
    hpdma_config.src.xsize = len;
    hpdma_config.src.ysize = 1;  // 1 row (user input: 1 = 1 row, will be decremented in bk_hpdma_init)
    hpdma_config.src.step = 0;

    hpdma_config.dst.dev = HPDMA_DEV_DTCM;
    hpdma_config.dst.width = HPDMA_DATA_WIDTH_32BITS;
    hpdma_config.dst.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    hpdma_config.dst.start_addr = (uintptr_t)out;
    hpdma_config.dst.xsize = len;
    hpdma_config.dst.ysize = 1;  // 1 row (user input: 1 = 1 row, will be decremented in bk_hpdma_init)
    hpdma_config.dst.step = 0;

    HPDMA_LOGV("hpdma_memcpy cpy_chnl: %d\r\n", cpy_chnl);

    /*
     * S0 (HPDMA SMP review):
     *   The previous version bracketed the whole "wait + init + start"
     *   sequence with GLOBAL_INT_DISABLE. That had two problems:
     *     1. GLOBAL_INT_DISABLE only masks IRQs on the calling core,
     *        so on FreeRTOS-SMP a peer core's task or the routed-core
     *        HPDMA ISR could still preempt the configuration sequence.
     *     2. hpdma_wait_to_idle can spin for up to HPDMA_MAX_BUSY_TIME
     *        us (10 ms). Holding IRQs off across that window stalls
     *        every higher-priority task on the local core; this was the
     *        kind of thing that produced the heartbeat / scheduler
     *        hangs we tracked down in the SMP MIPI logs.
     *
     *   The wait is now done out of any critical section. The actual
     *   configuration sequence (bk_hpdma_init -> sec_attr -> burst
     *   policy -> DSB -> hpdma_hal_start_common) is wrapped in
     *   hpdma_enter_critical so it is atomic against peer cores AND
     *   the routed-core ISR. Each inner bk_hpdma_* call also takes the
     *   same lock; the recursive spinlock turns those nested takes
     *   into cheap counter increments.
     */
    bk_err_t pre_wait = hpdma_wait_to_idle(cpy_chnl);
    if (pre_wait != BK_OK) {
        HPDMA_LOGE("memcpy: ch%d busy before reconfig, ret=%d\r\n", cpy_chnl, pre_wait);
        return pre_wait;
    }

    /*
     * S1 (HPDMA review): completion no longer busy-waits when we are in a
     *   context that can block. hpdma_arm_completion() installs a driver-owned
     *   finish callback + enables the finish interrupt so hpdma_wait_done()
     *   sleeps on a semaphore and yields the CPU. Armed BEFORE bk_hpdma_init /
     *   start so the completion can never be missed; finish_int_en is an
     *   independent req_mux bitfield that init preserves.
     */
    hpdma_done_ctx_t ctx = {0};
    bool use_sem = hpdma_can_block();
    if (use_sem && (hpdma_arm_completion(cpy_chnl, &ctx) != BK_OK)) {
        use_sem = false;
    }

    uint32_t int_mask = hpdma_enter_critical();

    // Note: Cache operations for source and destination are handled in bk_hpdma_init
    // which flushes both source and destination cache before DMA transfer starts
    bk_hpdma_init(cpy_chnl, &hpdma_config);
#if (CONFIG_SPE)
    bk_hpdma_set_src_sec_attr(cpy_chnl, HPDMA_ATTR_SEC);
    bk_hpdma_set_dest_sec_attr(cpy_chnl, HPDMA_ATTR_SEC);
#endif

    /*
     * New P0 (HPDMA review): apply SMEM-same-block burst override here
     *   too. hpdma_memcpy_by_chnl bypasses bk_hpdma_start() and calls
     *   hpdma_hal_start_common directly, so it would otherwise miss the
     *   policy that now lives in bk_hpdma_start().
     */
    hpdma_apply_smem_burst_policy_at_start(cpy_chnl);

    __DSB();
    hpdma_hal_start_common(&s_hpdma.hal, cpy_chnl);
    hpdma_exit_critical(int_mask);

    /*
     * S1 (HPDMA review): bounded wait (semaphore or busy-poll fallback). On
     *   timeout hpdma_wait_done() halts the channel via stop_disable_only so it
     *   reaches a known-dead state without disturbing interrupt status.
     */
    bk_err_t ret = hpdma_wait_done(cpy_chnl, HPDMA_DONE_WAIT_MS, use_sem);

    hpdma_disarm_completion(cpy_chnl, &ctx);

#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Invalidate destination cache to ensure CPU reads DMA-written data
    if (ret == BK_OK) {
        flush_dcache((void *)out, len);
    }
#endif
    __DMB();

    return ret;
}

/*
 * Copy len bytes on an already-allocated channel, choosing single-shot for
 * len <= HPDMA_SINGLE_MAX_XFER and the linked-list split for anything larger
 * (the single-shot xsize field is only 16 bits). Exposed so callers that
 * manage their own channel can copy arbitrary lengths.
 */
bk_err_t bk_hpdma_memcpy_link(void *out, const void *in, uint32_t len, hpdma_id_t cpy_chnl)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(cpy_chnl);

    if (out == NULL || in == NULL) {
        return BK_ERR_NULL_PARAM;
    }
    if (len == 0) {
        return BK_OK;
    }

    if (len > HPDMA_SINGLE_MAX_XFER) {
        return hpdma_memcpy_link_by_chnl(out, in, len, cpy_chnl);
    }
    return hpdma_memcpy_by_chnl(out, in, len, cpy_chnl);
}

bk_err_t bk_hpdma_memcpy(void *out, const void *in, uint32_t len)
{
    HPDMA_RETURN_ON_NOT_INIT();

    if (out == NULL || in == NULL) {
        return BK_ERR_NULL_PARAM;
    }
    if (len == 0) {
        return BK_OK;
    }

    bk_err_t ret;
    hpdma_id_t cpy_chnl = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    HPDMA_RETURN_ON_INVALID_ID(cpy_chnl);

    /*
     * len > 16-bit single-shot limit is split into a linked list of
     * <=HPDMA_SINGLE_MAX_XFER chunks; smaller copies keep the single-shot path.
     * Cache maintenance for both paths happens inside the callees:
     *   - Before transfer: bk_hpdma_init() / bk_hpdma_link_transfer() flush src.
     *   - After transfer: flush_dcache() invalidates the destination.
     */
    if (len > HPDMA_SINGLE_MAX_XFER) {
        ret = hpdma_memcpy_link_by_chnl(out, in, len, cpy_chnl);
    } else {
        ret = hpdma_memcpy_by_chnl(out, in, len, cpy_chnl);
    }

    bk_hpdma_free(HPDMA_DEV_DTCM, cpy_chnl);

    /*
     * Correctness over speed: if the DMA path failed (alloc/link/timeout), fall
     * back to a CPU copy so the caller always ends up with the right data.
     */
    if (ret != BK_OK) {
        HPDMA_LOGW("memcpy: DMA path failed ret=%d, falling back to CPU copy (len=%u)\r\n", ret, len);
        os_memcpy(out, in, len);
        ret = BK_OK;
    }

    return ret;
}

/*
 * P0 (HPDMA review):
 *   The header declared bk_hpdma_flush_src_buffer() for parity with the
 *   regular DMA's bk_dma_flush_src_buffer(), but the symbol was never
 *   defined - any link that referenced it (e.g. generic DMA wrappers
 *   built with HPDMA) would simply fail at link time, while builds that
 *   didn't reference it carried a phantom API.
 *
 *   On BK7259 HPDMA there is no dedicated "flush fifo" status bit
 *   exposed in the Reg23/Reg28 spec; the engine drains its internal
 *   buffers when the channel is disabled and dma_en reads back 0.
 *   The implementation therefore boils down to "park the channel and
 *   wait until the engine reports idle", with stop_disable_only to
 *   avoid clobbering interrupt status that the caller may consume.
 */
bk_err_t bk_hpdma_flush_src_buffer(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    HPDMA_RETURN_ON_ID_NOT_INIT(0, id);

    /*
     * S0 (HPDMA SMP review):
     *   stop_disable_only is a ctrl RMW. Lock JUST the stop; the wait
     *   below polls without the lock to keep the busy-wait window out
     *   of any critical section.
     */
    {
        uint32_t int_mask = hpdma_enter_critical();
        hpdma_hal_stop_disable_only(&s_hpdma.hal, id);
        hpdma_exit_critical(int_mask);
    }
    return hpdma_wait_to_idle(id);
}

/*
 * S0/C (HPDMA review - bk_hpdma_free timeout escape hatch):
 *
 *   When bk_hpdma_free()'s internal stop + wait_to_idle hits a
 *   HPDMA_MAX_BUSY_TIME timeout (engine wedged), we deliberately keep
 *   the bitmap bit set so the channel cannot be silently handed to a
 *   new owner. Without an escape hatch the chnl_id would leak forever
 *   and the 4-channel pool would eventually be exhausted.
 *
 *   bk_hpdma_force_reclaim:
 *     1) verifies the bitmap still tracks the original allocator
 *        (the same user_id contract enforced by bk_hpdma_free).
 *     2) resets *just this channel's* registers to their power-on
 *        defaults via hpdma_ll_reset_config_to_default - this is the
 *        per-channel equivalent of soft_reset and avoids the full
 *        controller reset that hpdma_hal_init_without_channels would
 *        do (which would disturb peer channels).
 *     3) clears the channel ISR callbacks and releases the bitmap.
 *
 *   Data already partially written to the destination is lost; that's
 *   the price of recovering from a wedged engine. Callers that hold
 *   buffers on behalf of the channel must NOT free them until they
 *   accept this loss.
 */
bk_err_t bk_hpdma_force_reclaim(u16 user_id, hpdma_id_t chnl_id)
{
    if (!s_hpdma_driver_is_init) {
        return BK_ERR_HPDMA_NOT_INIT;
    }
    if (chnl_id >= HPDMA_ID_MAX) {
        return BK_ERR_HPDMA_ID;
    }
    if (s_hpdma_chnl_pool.chnl_user[chnl_id] != user_id) {
        return BK_ERR_PARAM;
    }

    HPDMA_LOGW("force reclaim ch%d (user=0x%x): DMA may have left partial writes\r\n",
               chnl_id, user_id);

    /*
     * S0 (HPDMA SMP review):
     *   Extend the critical section to cover stop_disable_only +
     *   reset_config_to_default + id_init_bits clear. All three touch
     *   bitfields or shared software state and must serialize with
     *   peer cores and the routed-core ISR. Previously only the ISR
     *   table NULL-ing and the channel pool update were locked.
     */
    u32 int_mask = hpdma_enter_critical();
    hpdma_hal_stop_disable_only(&s_hpdma.hal, chnl_id);
    /* Per-channel reset: ctrl/req_mux/status/addresses all back to 0. */
    hpdma_hal_reset_config_to_default(&s_hpdma.hal, chnl_id);
    s_hpdma.id_init_bits &= ~BIT(chnl_id);

    s_hpdma_half_finish_isr[chnl_id].callback = NULL;
    s_hpdma_half_finish_isr[chnl_id].user_data = NULL;
    s_hpdma_finish_isr[chnl_id].callback = NULL;
    s_hpdma_finish_isr[chnl_id].user_data = NULL;
    s_hpdma_bus_err_isr[chnl_id].callback = NULL;
    s_hpdma_bus_err_isr[chnl_id].user_data = NULL;
    s_hpdma_fifo_err_isr[chnl_id].callback = NULL;
    s_hpdma_fifo_err_isr[chnl_id].user_data = NULL;
    s_hpdma_chnl_pool.chnl_bitmap &= ~(0x01 << chnl_id);
    s_hpdma_chnl_pool.chnl_user[chnl_id] = -1;
    hpdma_exit_critical(int_mask);

    return BK_OK;
}

/*
 * S2 (HPDMA review):
 *   The controller-wide registers (soft_reset / secure_attr /
 *   privileged_attr / prio_mode) are powered down with the rest of the
 *   chip during low-voltage sleep and read back as zeros on wakeup.
 *
 *   Previously bk_hpdma_init() called hpdma_hal_init_without_channels()
 *   on *every* invocation to paper over this; that turned every init
 *   into a potential global reset that silently aborted any peer
 *   channel that happened to be mid-transfer (e.g. GPU running while
 *   LVGL kicks a new memcpy after wakeup).
 *
 *   PM should now call bk_hpdma_recover_after_low_voltage() exactly
 *   once on wakeup, BEFORE any client calls bk_hpdma_init / start.
 *   This is the only place that may touch global controller state.
 */
bk_err_t bk_hpdma_recover_after_low_voltage(void)
{
    HPDMA_RETURN_ON_NOT_INIT();
    /*
     * S0 (HPDMA SMP review):
     *   hpdma_hal_init_without_channels writes the controller-wide
     *   prio_mode (with soft_reset bit), secure_attr, and
     *   privileged_attr registers. Even on PM wakeup these may race
     *   against tasks on the other core that already started running
     *   before this PM exit callback completes. Lock them.
     */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_init_without_channels(&s_hpdma.hal);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}


bk_err_t bk_hpdma_set_next_ll_addr(hpdma_id_t id, uint32_t ll_addr)
{
	HPDMA_RETURN_ON_NOT_INIT();
	HPDMA_RETURN_ON_INVALID_ID(id);
    /* S0 (HPDMA SMP review): full 32-bit register write but lock to
     * keep configuration ordering against peer cores and the ISR. */
    uint32_t int_mask = hpdma_enter_critical();
    hpdma_hal_set_next_ll_addr(&s_hpdma.hal, id, ll_addr);
    hpdma_exit_critical(int_mask);
    return BK_OK;
}

uint32_t bk_hpdma_get_next_ll_addr(hpdma_id_t id)
{
	HPDMA_RETURN_ON_NOT_INIT();
	HPDMA_RETURN_ON_INVALID_ID(id);
    HPDMA_LOGV("%s\r\n", __func__);
	return hpdma_hal_get_next_ll_addr(&s_hpdma.hal, id);
}

// ============================================================================
// New descriptor format implementation (24 bytes, 6 words)
// ============================================================================

void hpdma_link_wait_ready(hpdma_id_t hpdma_chn)
{
    HPDMA_LOGV("%s wait ready\r\n", __func__);
	while(bk_hpdma_get_next_ll_addr(hpdma_chn));
	while(bk_hpdma_get_enable_status(hpdma_chn));
    HPDMA_LOGV("%s\r\n", __func__);
}


static inline hpdma_descriptor_t *hpdma_get_desc_by_index(void *desc_table, uint32_t index)
{
    const uint32_t desc_aligned_size = (sizeof(hpdma_descriptor_t) + 15) & ~15;  // 32 bytes
    uintptr_t first_desc_addr = (uintptr_t)desc_table;
    uintptr_t desc_addr = first_desc_addr + index * desc_aligned_size;
    return (hpdma_descriptor_t *)desc_addr;
}


void *bk_hpdma_link_init(uint32_t link_cnt)
{
    // Calculate aligned size per descriptor (24 bytes -> 32 bytes for 16-byte alignment)
    const uint32_t desc_size = sizeof(hpdma_descriptor_t);  // 24 bytes
    const uint32_t desc_aligned_size = (desc_size + 15) & ~15;  // 32 bytes (aligned to 16)

    // Allocate memory: each descriptor needs aligned_size, plus space for alignment and raw pointer storage
    // Total: link_cnt * desc_aligned_size + 15 (for first descriptor alignment) + sizeof(void*) (for raw pointer)
    void *raw_ptr = os_malloc(link_cnt * desc_aligned_size + 15 + sizeof(void*));
    if (raw_ptr == NULL) {
        HPDMA_LOGE("Failed to allocate descriptor table\r\n");
        return NULL;
    }

    // Align first descriptor to 16-byte boundary
    uintptr_t first_desc_addr = ((uintptr_t)raw_ptr + 15 + sizeof(void*)) & ~15;

    // Store raw pointer before aligned address for deinit
    void **raw_ptr_storage = (void **)(first_desc_addr - sizeof(void*));
    *raw_ptr_storage = raw_ptr;

    HPDMA_LOGV("%s raw_ptr=0x%x first_desc=0x%x desc_size=%d aligned_size=%d\r\n",
               __func__, raw_ptr, first_desc_addr, desc_size, desc_aligned_size);

    // Clear all descriptors
    os_memset((void *)first_desc_addr, 0, link_cnt * desc_aligned_size);

    // Link descriptors together - each descriptor's next_desc_addr points to next 16-byte aligned address
    for (int i = 0; i < link_cnt - 1; i++) {
        uintptr_t curr_desc_addr = first_desc_addr + i * desc_aligned_size;
        uintptr_t next_desc_addr = first_desc_addr + (i + 1) * desc_aligned_size;
        hpdma_descriptor_t *curr_desc = (hpdma_descriptor_t *)curr_desc_addr;

        // Verify alignment
        if ((curr_desc_addr & 15) != 0 || (next_desc_addr & 15) != 0) {
            HPDMA_LOGE("Descriptor alignment error: curr=0x%x next=0x%x\r\n", curr_desc_addr, next_desc_addr);
        }

        /*
         * The HPDMA engine walks the chain via this DES5 pointer and can
         * only reach the 0x28xxxxxx SRAM alias. next_desc_addr here is the
         * CPU-side allocation (0x2Cxxxxxx when CONFIG_SRAM_DIRECT_ADDR is on),
         * which the engine cannot fetch - it would stall after the first node
         * with dma_en stuck at 1 and no finish interrupt. Mirror the same
         * SOC_SRAM_PERI_ADDR() conversion already applied to desc->src/dst_addr
         * (bk_hpdma_link_set_desc) and to the channel's next_ll_addr register
         * (hpdma_hal_set_next_ll_addr). The macro is range-guarded and leaves
         * 0x28/PSRAM/register addresses unchanged.
         */
        curr_desc->next_desc_addr = (uint32_t)SOC_SRAM_PERI_ADDR(next_desc_addr);
        HPDMA_LOGV("Desc[%d] addr=0x%x next_addr=0x%x\r\n", i, curr_desc_addr, curr_desc->next_desc_addr);
    }

    // Last descriptor: next_addr = 0 (end of list)
    uintptr_t last_desc_addr = first_desc_addr + (link_cnt - 1) * desc_aligned_size;
    hpdma_descriptor_t *last_desc = (hpdma_descriptor_t *)last_desc_addr;
    last_desc->next_desc_addr = 0;
    HPDMA_LOGV("Desc[%d] addr=0x%x next_addr=0 (end of list)\r\n", link_cnt - 1, last_desc_addr);

#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Flush descriptor table to ensure DMA sees latest data after initialization
    // This ensures next_desc_addr is visible to DMA
    flush_dcache((void *)first_desc_addr, link_cnt * desc_aligned_size);
#endif
    __DSB();

    return (void *)first_desc_addr;
}


void bk_hpdma_link_deinit(void *desc_table)
{
    if (desc_table == NULL) {
        return;
    }

    // Get raw pointer stored before aligned address
    void **raw_ptr_storage = (void **)((uint32_t)desc_table - sizeof(void*));
    void *raw_ptr = *raw_ptr_storage;

    // Validate: raw_ptr should be within reasonable range
    // (desc_table - 15 - sizeof(void*) to desc_table - sizeof(void*))
    if (raw_ptr != NULL &&
        (uint32_t)raw_ptr >= ((uint32_t)desc_table - 15 - sizeof(void*)) &&
        (uint32_t)raw_ptr < (uint32_t)desc_table) {
        // Free raw pointer (which was allocated with extra space)
        os_free(raw_ptr);
        HPDMA_LOGD("%s freed desc_table=0x%x raw_ptr=0x%x\r\n", __func__, desc_table, raw_ptr);
    } else {
        HPDMA_LOGE("%s invalid raw_ptr: desc_table=0x%x raw_ptr=0x%x\r\n", __func__, desc_table, raw_ptr);
    }
}


bk_err_t bk_hpdma_link_set_desc(void *desc_table, uint32_t index,
                                 const hpdma_link_config_t *config)
{
    if (desc_table == NULL || config == NULL) {
        HPDMA_LOGE("Invalid parameters\r\n");
        return BK_ERR_NULL_PARAM;
    }

    // Validate ysize: user input should be >= 1 (1 = 1 row, 2 = 2 rows, etc.)
    if (config->src_ysize == 0) {
        HPDMA_LOGE("Source ysize must be >= 1 (1 = 1 row, 2 = 2 rows, etc.)\r\n");
        return BK_ERR_PARAM;
    }
    if (config->dst_ysize == 0) {
        HPDMA_LOGE("Destination ysize must be >= 1 (1 = 1 row, 2 = 2 rows, etc.)\r\n");
        return BK_ERR_PARAM;
    }

    // Get descriptor by index (handles 16-byte alignment spacing)
    hpdma_descriptor_t *desc = hpdma_get_desc_by_index(desc_table, index);

    // Configure descriptor for 2D transfer
    // User input: ysize = 1 means 1 row, ysize = 2 means 2 rows, etc.
    // Hardware expects: ysize = 0 means 1 row, ysize = 1 means 2 rows, etc.
    // So we need to decrement by 1 for hardware
    /* The HPDMA engine can only access the 0x28xxxxxx SRAM alias. Plain mode
     * (hpdma_hal_init_dma) already remaps via SOC_SRAM_PERI_ADDR, but link mode
     * writes descriptors directly and previously kept the raw 0x2Cxxxxxx CPU
     * alias, which the engine cannot reach. Mirror the plain-mode conversion
     * here so link-mode transfers from cacheable SRAM read/write real data.
     * The macro is range-guarded: PSRAM / 0x28 / register addresses pass through
     * unchanged. */
    desc->src_addr = (uint32_t)SOC_SRAM_PERI_ADDR((uintptr_t)config->src_addr);
    desc->dst_addr = (uint32_t)SOC_SRAM_PERI_ADDR((uintptr_t)config->dst_addr);
    desc->xdim.bits.source_xsize = config->src_xsize;
    desc->xdim.bits.dest_xsize = config->dst_xsize;
    desc->ydim.bits.source_ysize = config->src_ysize - 1;
    desc->ydim.bits.dest_ysize = config->dst_ysize - 1;
    desc->ctrl.bits.source_step = config->src_step;
    desc->ctrl.bits.dest_step = config->dst_step;
    desc->ctrl.bits.int_finish_en = config->finish_int_en;
    desc->ctrl.bits.int_half_finish_en = config->half_finish_int_en;

    HPDMA_LOGV("Desc[%d] src=0x%x dst=0x%x src_x=%d src_y=%d dst_x=%d dst_y=%d step_src=%d step_dst=%d\r\n",
               index, config->src_addr, config->dst_addr,
               config->src_xsize, config->src_ysize,
               config->dst_xsize, config->dst_ysize,
               config->src_step, config->dst_step);

#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Flush descriptor to ensure DMA sees latest data
    // Note: Source and destination addresses cache will be flushed in bk_hpdma_link_transfer
    // using flush_all_dcache(), so no need to flush them here
    const uint32_t desc_aligned_size = (sizeof(hpdma_descriptor_t) + 15) & ~15;  // 32 bytes
    flush_dcache((void *)desc, desc_aligned_size);
#endif
    __DSB();
    return BK_OK;
}


bk_err_t bk_hpdma_link_set_descs(void *desc_table,
                                  const hpdma_link_config_t *configs,
                                  uint32_t link_cnt)
{
    if (desc_table == NULL || configs == NULL) {
        HPDMA_LOGE("Invalid parameters\r\n");
        return BK_ERR_NULL_PARAM;
    }

    for (uint32_t i = 0; i < link_cnt; i++) {
        bk_err_t ret = bk_hpdma_link_set_desc(desc_table, i, &configs[i]);
        if (ret != BK_OK) {
            HPDMA_LOGE("Failed to set desc[%d]\r\n", i);
            return ret;
        }
    }

    return BK_OK;
}


bk_err_t bk_hpdma_link_transfer(hpdma_id_t id, void *desc_table)
{
    if (desc_table == NULL) {
        HPDMA_LOGE("Invalid descriptor table\r\n");
        return BK_ERR_NULL_PARAM;
    }

    if (id >= HPDMA_ID_MAX) {
        HPDMA_LOGE("Invalid DMA channel ID: %d\r\n", id);
        return BK_ERR_HPDMA_ID;
    }

    HPDMA_LOGV("%s DMA channel = %d\r\n", __func__, id);

    hpdma_config_t hpdma_config;

    os_memset(&hpdma_config, 0, sizeof(hpdma_config_t));

    // Configure DMA for linked list mode (all info in descriptors)
    // Mode 1: Set register values to 0, hardware reads all info from descriptors
    hpdma_config.mode = HPDMA_WORK_MODE_SINGLE;
    hpdma_config.chan_prio = 0;
    hpdma_config.src.dev = HPDMA_DEV_DTCM;
    hpdma_config.src.width = HPDMA_DATA_WIDTH_128BITS;
    hpdma_config.src.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    hpdma_config.src.xsize = 0;
    hpdma_config.src.ysize = 1;  // 1 row (user input: 1 = 1 row, will be decremented in bk_hpdma_init)
    hpdma_config.src.step = 0;
    hpdma_config.src.addr_loop_en = HPDMA_ADDR_LOOP_DISABLE;

    hpdma_config.dst.dev = HPDMA_DEV_DTCM;
    hpdma_config.dst.width = HPDMA_DATA_WIDTH_128BITS;
    hpdma_config.dst.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    hpdma_config.dst.xsize = 0;
    hpdma_config.dst.ysize = 1;  // 1 row (user input: 1 = 1 row, will be decremented in bk_hpdma_init)
    hpdma_config.dst.step = 0;
    hpdma_config.dst.addr_loop_en = HPDMA_ADDR_LOOP_DISABLE;

    bk_hpdma_set_next_ll_addr(id, (uint32_t)desc_table);

    // Initialize and configure DMA
    bk_hpdma_init(id, &hpdma_config);

#if CONFIG_SPE
    bk_hpdma_set_src_sec_attr(id, HPDMA_ATTR_SEC);
    bk_hpdma_set_dest_sec_attr(id, HPDMA_ATTR_SEC);
#endif


//TODO: 12.30 need ASIC verify
#if 0
    bk_hpdma_set_src_burst_len(id, HPDMA_BURST_LEN_INC16);
    bk_hpdma_set_dest_burst_len(id, HPDMA_BURST_LEN_INC16);
#endif

#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Flush all cache to ensure DMA sees all descriptors and data
    // This is more efficient than traversing all descriptors when link_cnt is large
    // flush_all_dcache() will flush:
    // 1. Descriptor table (already flushed in bk_hpdma_link_init and bk_hpdma_link_set_desc, but flush again for safety)
    // 2. All source addresses (CPU-written data)
    // 3. All destination addresses (prepare for DMA write)
    flush_all_dcache();
#endif

    bk_hpdma_start(id);

    HPDMA_LOGV("%s DMA started\r\n", __func__);

    return BK_OK;
}

/*
 * S0 (HPDMA SMP review):
 *   The ISR coordinates with peer-core tasks via the same hpdma_spin_lock
 *   that wraps every register-touch API. For each event class on each
 *   channel we:
 *     1. Take the lock.
 *     2. Re-check the status bit (it may have been cleared by a peer
 *        already), perform the W1C clear and, for error classes, the
 *        ctrl.enable RMW that halts the channel.
 *     3. Snapshot the {callback, user_data} pair so we have a stable
 *        copy independent of any concurrent register/unregister.
 *     4. Release the lock.
 *     5. Run the user callback OUTSIDE the lock. This is mandatory:
 *        clients post semaphores / queue work / take their own mutexes,
 *        and they must not do that under our spinlock or we risk
 *        priority inversion / deadlock.
 *
 *   Re-checking the triggered bit after taking the lock is important:
 *   between "is_triggered() true" outside the lock and grabbing the lock,
 *   a peer could have already W1C-cleared the status (e.g. a CLI command
 *   probing status on the other core). Without the re-check we would
 *   call the callback for an event that is no longer pending and may
 *   dereference stale user_data.
 */
static void hpdma_isr_common(hpdma_unit_t hpdma_unit_id)
{
    hpdma_hal_t *hal = &s_hpdma.hal;

    for (int id = 0; id < SOC_HPDMA_CHAN_NUM_PER_UNIT; id++) {
        const uint32_t channel = id + hpdma_unit_id * SOC_HPDMA_CHAN_NUM_PER_UNIT;
        hpdma_isr_t cb;
        void *cb_data;
        bool fire;

        /* ---- half-finish ---- */
        fire = false;
        cb = NULL;
        cb_data = NULL;
        {
            uint32_t int_mask = hpdma_enter_critical();
            if (hpdma_hal_is_half_finish_interrupt_triggered(hal, id)) {
                hpdma_hal_clear_half_finish_interrupt_status(hal, id);
                cb = s_hpdma_half_finish_isr[id].callback;
                cb_data = s_hpdma_half_finish_isr[id].user_data;
                fire = true;
            }
            hpdma_exit_critical(int_mask);
        }
        if (fire) {
            __DSB();
            __ISB();
            HPDMA_LOGV("hpdma_isr HALF FINISH ch%u\r\n", channel);
            if (cb) {
                cb(channel, cb_data);
            }
        }

        /* ---- finish ---- */
        fire = false;
        cb = NULL;
        cb_data = NULL;
        {
            uint32_t int_mask = hpdma_enter_critical();
            if (hpdma_hal_is_finish_interrupt_triggered(hal, id)) {
                /*
                 * S1 (HPDMA review):
                 *   Old code called flush_all_dcache() on every finish
                 *   ISR. Whole-cache flush from interrupt context is a
                 *   millisecond-class stall. Per-callback cache
                 *   maintenance is the caller's responsibility (LVGL /
                 *   GPU already flush the specific buffer they read
                 *   after waiting on the completion semaphore).
                 */
                hpdma_hal_clear_finish_interrupt_status(hal, id);
                cb = s_hpdma_finish_isr[id].callback;
                cb_data = s_hpdma_finish_isr[id].user_data;
                fire = true;
            }
            hpdma_exit_critical(int_mask);
        }
        if (fire) {
            __DSB();
            __ISB();
            HPDMA_LOGV("hpdma_isr FINISH ch%u\r\n", channel);
            if (cb) {
                cb(channel, cb_data);
            }
        }

        /* ---- bus_err ---- */
        fire = false;
        cb = NULL;
        cb_data = NULL;
        {
            uint32_t int_mask = hpdma_enter_critical();
            if (hpdma_hal_is_bus_err_interrupt_triggered(hal, id)) {
                /*
                 * S1 (HPDMA review):
                 *   On bus_err the channel is in a known-bad state.
                 *   Disable the channel BEFORE running the callback so
                 *   the callback always observes a halted engine and
                 *   subsequent ISRs cannot re-fire for the same fault.
                 *   stop_disable_only() preserves half/finish/fifo
                 *   status bits for the callback to inspect.
                 */
                hpdma_hal_stop_disable_only(hal, id);
                hpdma_hal_clear_bus_err_interrupt_status(hal, id);
                cb = s_hpdma_bus_err_isr[id].callback;
                cb_data = s_hpdma_bus_err_isr[id].user_data;
                fire = true;
            }
            hpdma_exit_critical(int_mask);
        }
        if (fire) {
            __DSB();
            __ISB();
            HPDMA_LOGE("hpdma_isr BUS ERR ch%u\r\n", channel);
            if (cb) {
                cb(channel, cb_data);
            }
        }

        /* ---- fifo_err ---- */
        /*
         * P0 (HPDMA review): fifo_err handling was previously absent;
         *   bit17 in status would never get W1C'd, latching the line.
         *   Mirror bus_err: halt first, then clear, then notify.
         */
        fire = false;
        cb = NULL;
        cb_data = NULL;
        {
            uint32_t int_mask = hpdma_enter_critical();
            if (hpdma_hal_is_fifo_err_interrupt_triggered(hal, id)) {
                hpdma_hal_stop_disable_only(hal, id);
                hpdma_hal_clear_fifo_err_interrupt_status(hal, id);
                cb = s_hpdma_fifo_err_isr[id].callback;
                cb_data = s_hpdma_fifo_err_isr[id].user_data;
                fire = true;
            }
            hpdma_exit_critical(int_mask);
        }
        if (fire) {
            __DSB();
            __ISB();
            HPDMA_LOGE("hpdma_isr FIFO ERR ch%u\r\n", channel);
            if (cb) {
                cb(channel, cb_data);
            }
        }
    }
}

static void hpdma_isr(void)
{
	HPDMA_LOGV("hpdma_isr hw 0\r\n");
	hpdma_isr_common(0);
}