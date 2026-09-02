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
#include <stdint.h>
#include <stdbool.h>
#include <driver/int.h>
#include <os/mem.h>
#include "clock_driver.h"
#include "sys_driver.h"
#include "psram_hal.h"
#include "../hspl/hspl_res_lock.h"
#include "driver/psram_types.h"
#include "psram_driver.h"
#include <driver/psram.h>
#include "ram_regions.h"
#include <modules/pm.h>
#if (CONFIG_PSRAM_AUTO_DETECT)
#include "bk_ef.h"
#endif

#define PSRAM_4M_SIZE  (0x00400000)
#define PSRAM_8M_SIZE  (0x00800000)
#define PSRAM_16M_SIZE (0x01000000)

#define PSRAM_THREAD_STACK_SIZE    3072
#define PSRAM_THREAD_PRIORITY      4
#define PSRAM_SEMAPHORE_COUNT      1
#define PSRAM_ADDRESS_ALIGNMENT    4
#define PSRAM_BYTES_PER_WORD       4
#define PSRAM_MAX_STRING_LEN       1024

#define TAG "psram"

#define MEM_STATIC_LOGD( format, ... ) bk_printf_static_block(BK_LOG_DEBUG, TAG, format, ##__VA_ARGS__)
#define MEM_STATIC_LOGI( format, ... ) bk_printf_static_block(BK_LOG_INFO, TAG, format, ##__VA_ARGS__)
#define MEM_STATIC_LOGE( format, ... ) bk_printf_static_block(BK_LOG_ERROR, TAG, format, ##__VA_ARGS__)
#define MEM_STATIC_LOGW( format, ... ) bk_printf_static_block(BK_LOG_WARN, TAG, format, ##__VA_ARGS__)


#define PSRAM_CHECK_FLAG   0x3CA5C3A5
typedef struct {
	uint32_t psram_id;
	uint32_t magic_code;
} psram_flash_t;

#if (CONFIG_PSRAM_AUTO_DETECT)
static bool s_psram_id_need_write = false;
static beken_semaphore_t s_psram_sem = NULL;
static beken_thread_t psram_task = NULL;
#endif

extern void bk_delay_us(uint32_t us);
static bool s_psram_heap_is_init = false;
static beken_mutex_t s_psram_mutex = NULL;
static volatile bool s_psram_init_done[PSRAM_ID_MAX] = {false};
static uint8_t s_psram_channelmap[PSRAM_ID_MAX] = {0};
#define PSRAM_INIT_WAIT_TIMEOUT_MS   100

static psram_flash_t s_psram_id = {0};

static inline uint32_t bk_psram_get_data_base(psram_id_t psram_id)
{
#if defined(SOC_PSRAM0_DATA_BASE) && defined(SOC_PSRAM1_DATA_BASE)
	return (psram_id == PSRAM_ID_1) ? SOC_PSRAM1_DATA_BASE : SOC_PSRAM0_DATA_BASE;
#else
	(void)psram_id;
	return SOC_PSRAM_DATA_BASE;
#endif
}

static inline psram_id_t bk_psram_addr_to_id(uint32_t addr)
{
#if defined(SOC_PSRAM1_DATA_BASE)
	if ((addr >= bk_psram_get_data_base(PSRAM_ID_1)) && (addr < (bk_psram_get_data_base(PSRAM_ID_1) + SOC_PSRAM_DATA_SIZE))) {
		return PSRAM_ID_1;
	}
#endif
	return PSRAM_ID_0;
}

static inline bk_err_t bk_psram_return_on_not_init(psram_id_t psram_id)
{
	if ((psram_id >= PSRAM_ID_MAX) || (!s_psram_init_done[psram_id])) {
		return BK_ERR_PSRAM_SERVER_NOT_INIT;
	}
	return BK_OK;
}

static inline bool bk_psram_any_init_done(void)
{
	for (int i = 0; i < (int)PSRAM_ID_MAX; i++) {
		if (s_psram_init_done[i]) {
			return true;
		}
	}
	return false;
}

static bk_err_t bk_psram_write_through_lock(uint32_t *int_level)
{
	bk_err_t ret;

	if (!int_level) {
		return BK_ERR_PARAM;
	}

	*int_level = rtos_disable_int();
	ret = bk_hspl_res_try_lock(BK_HSPL_RES_PSRAM);
	if (ret != BK_OK) {
		rtos_enable_int(*int_level);
	}

	return ret;
}

static void bk_psram_write_through_unlock(uint32_t int_level)
{
	bk_hspl_res_unlock(BK_HSPL_RES_PSRAM);
	rtos_enable_int(int_level);
}

bk_err_t bk_psram_set_clk_with_id(psram_id_t psram_id, psram_clk_t clk)
{
	bk_err_t ret = BK_OK;

	psram_hal_set_clk_with_id(psram_id, clk);

	return ret;
}

bk_err_t bk_psram_init_with_para(uint32_t psram_clk, uint32_t psram_vol)
{
	psram_clk_t clk = PSRAM_240M;
	psram_voltage_t vol = PSRAM_OUT_1_95V;

	/* clk: accept MHz (80/120/160/240) */
	switch (psram_clk) {
		case 80:  clk = PSRAM_80M;  break;
		case 120: clk = PSRAM_120M; break;
		case 160: clk = PSRAM_160M; break;
		case 240: clk = PSRAM_240M; break;
		default:
			/* allow passing enum value directly */
			if (psram_clk <= (uint32_t)PSRAM_80M + 10) {
				clk = (psram_clk_t)psram_clk;
			}
			break;
	}

	/* vol: accept mV (1950/1900/...) or centi-volt (195/190/...) */
	uint32_t mv = psram_vol;
	if (psram_vol < 1000) {
		mv = psram_vol * 10;
	}
	switch (mv) {
		case 3200: vol = PSRAM_OUT_3_20V; break;
		case 3000: vol = PSRAM_OUT_3_0V; break;
		case 2000: vol = PSRAM_OUT_2_0V; break;
		case 1950: vol = PSRAM_OUT_1_95V; break;
		case 1900: vol = PSRAM_OUT_1_90V; break;
		case 1850: vol = PSRAM_OUT_1_85V; break;
		case 1800: vol = PSRAM_OUT_1_80V; break;
		case 1500: vol = PSRAM_OUT_1_50V; break;
		case 1200: vol = PSRAM_OUT_1_20V; break;
		default:
			break;
	}

	bk_psram_set_voltage(vol);
	bk_psram_init();
	bk_psram_set_clk_with_id(PSRAM_ID_0, clk);
	return BK_OK;
}

uint32_t bk_psram_get_psram_id(void)
{
	return s_psram_id.psram_id;
}

uint32_t bk_psram_get_psram_data_length(uint32_t id)
{
	switch (id) {
		case PSRAM_W955D8MKY_5J_ID:
			return PSRAM_4M_SIZE;
		case PSRAM_APS6408L_ID:
			return PSRAM_8M_SIZE;
		case PSRAM_APS128XXO_OB9_ID:
			return PSRAM_16M_SIZE;
		default:
			return CONFIG_PSRAM_CAPACITY;
	}
}
bk_err_t bk_psram_heap_init_flag_set(bool init)
{
	bk_err_t ret = BK_OK;
	s_psram_heap_is_init = init;
	return ret;
}
bool bk_psram_heap_init_flag_get()
{
	return s_psram_heap_is_init;
}

bk_err_t bk_psram_set_voltage(psram_voltage_t voltage)
{
	bk_err_t ret = BK_OK;

	psram_hal_set_voltage(voltage);

	return ret;
}

bk_err_t bk_psram_set_transfer_mode(psram_tansfer_mode_t transfer_mode)
{
	bk_err_t ret = BK_OK;

	psram_hal_set_transfer_mode(transfer_mode);

	return ret;
}

psram_write_through_area_t bk_psram_alloc_write_through_channel(void)
{
	return bk_psram_alloc_write_through_channel_with_id(PSRAM_ID_0);
}

psram_write_through_area_t bk_psram_alloc_write_through_channel_with_id(psram_id_t psram_id)
{
	uint8_t channel = PSRAM_WRITE_THROUGH_AREA_COUNT;
	uint32_t int_level;

	if (psram_id >= PSRAM_ID_MAX) {
		return PSRAM_WRITE_THROUGH_AREA_COUNT;
	}

	if (bk_psram_write_through_lock(&int_level) != BK_OK) {
		return PSRAM_WRITE_THROUGH_AREA_COUNT;
	}

	for (channel = 0; channel < PSRAM_WRITE_THROUGH_AREA_COUNT; channel++)
	{
		if ((s_psram_channelmap[psram_id] & (0x1 << channel)) == 0)
		{
			s_psram_channelmap[psram_id] |= (0x1 << channel);
			break;
		}
	}

	bk_psram_write_through_unlock(int_level);

	return channel;
}

bk_err_t bk_psram_free_write_through_channel(psram_write_through_area_t area)
{
	return bk_psram_free_write_through_channel_with_id(PSRAM_ID_0, area);
}

bk_err_t bk_psram_free_write_through_channel_with_id(psram_id_t psram_id, psram_write_through_area_t area)
{
	uint32_t int_level;
	bk_err_t ret;

	if (psram_id >= PSRAM_ID_MAX) {
		return BK_ERR_PARAM;
	}
	if (area >= PSRAM_WRITE_THROUGH_AREA_COUNT)
	{
		MEM_STATIC_LOGE("%s over range failed\r\n", __func__);
		return BK_ERR_PARAM;
	}

	ret = bk_psram_write_through_lock(&int_level);
	if (ret != BK_OK) {
		return ret;
	}

	if (s_psram_channelmap[psram_id] & (0x1 << area)) {
		s_psram_channelmap[psram_id] &= ~(0x1 << area);
	}

	bk_psram_write_through_unlock(int_level);

	return BK_OK;
}

bk_err_t  bk_psram_enable_write_through(psram_write_through_area_t area, uint32_t start, uint32_t end)
{
	return bk_psram_enable_write_through_with_id(PSRAM_ID_0, area, start, end);
}

bk_err_t bk_psram_disable_write_through(psram_write_through_area_t area)
{
	return bk_psram_disable_write_through_with_id(PSRAM_ID_0, area);
}

bk_err_t  bk_psram_enable_write_through_with_id(psram_id_t psram_id, psram_write_through_area_t area, uint32_t start, uint32_t end)
{
	if (psram_id >= PSRAM_ID_MAX) {
		return BK_ERR_PARAM;
	}
	bk_err_t init_ret = bk_psram_return_on_not_init(psram_id);
	if (init_ret != BK_OK) {
		return init_ret;
	}
	return psram_hal_set_write_through_with_id(psram_id, area, 1, start, end);
}

bk_err_t bk_psram_disable_write_through_with_id(psram_id_t psram_id, psram_write_through_area_t area)
{
	if (psram_id >= PSRAM_ID_MAX) {
		return BK_ERR_PARAM;
	}
	bk_err_t init_ret = bk_psram_return_on_not_init(psram_id);
	if (init_ret != BK_OK) {
		return init_ret;
	}
	return psram_hal_set_write_through_with_id(psram_id, area, 0, 0, 0);
}

bk_err_t bk_psram_calibrate(void)
{
#if CONFIG_PSRAM_CALIBRATE
	//TODO add calibrate strategy after get it from digital team
	return BK_OK;
#else
	return BK_OK;
#endif
}

#if (CONFIG_PSRAM_AUTO_DETECT)
static void psram_id_write(beken_thread_arg_t data)
{
	rtos_get_semaphore(&s_psram_sem, BEKEN_WAIT_FOREVER);

	if (s_psram_id_need_write)
	{
		bk_set_env_enhance(PSRAM_CHIP_ID, (const void *)&s_psram_id, sizeof(psram_flash_t));
	}

	MEM_STATIC_LOGD("psram id write to flash success\r\n");

	s_psram_id_need_write = false;

	rtos_deinit_semaphore(&s_psram_sem);
	s_psram_sem = NULL;

	psram_task = NULL;
	rtos_delete_thread(NULL);
}
#endif

bk_err_t bk_psram_id_auto_detect(void)
{
#if (CONFIG_PSRAM_AUTO_DETECT)
	int ret = bk_get_env_enhance(PSRAM_CHIP_ID, (void *)&s_psram_id, sizeof(psram_flash_t));

	if (ret != 8)
	{
		MEM_STATIC_LOGD("Auto detect:No PSRAM_CHIP_ID INFO, ret:%d\r\n", ret);
	}

	if (s_psram_id.magic_code == PSRAM_CHECK_FLAG)
	{
		return BK_OK;
	}

	if (s_psram_sem == NULL)
	{
		ret = rtos_init_semaphore(&s_psram_sem, PSRAM_SEMAPHORE_COUNT);
		if (ret != BK_OK)
		{
			MEM_STATIC_LOGE("%s, init s_psram_sem error\r\n", __func__);
			return ret;
		}
	}

	ret = rtos_create_thread(&psram_task,
						 PSRAM_THREAD_PRIORITY,
						 "psram_task",
						 (beken_thread_function_t)psram_id_write,
						 PSRAM_THREAD_STACK_SIZE,
						 NULL);

	if (BK_OK != ret)
	{
		MEM_STATIC_LOGE("%s psram_task init failed, ret:%d\n", __func__, ret);
		rtos_deinit_semaphore(&s_psram_sem);
		s_psram_sem = NULL;
		return ret;
	}
#endif

	return BK_OK;
}

bk_err_t bk_psram_init(void)
{
	bk_psram_init_with_id(PSRAM_ID_0);
	bk_psram_init_with_id(PSRAM_ID_1);
#if (CONFIG_PSRAM_INTERLEAVE)
	/* Set PSRAM interleave step after both PSRAM0 and PSRAM1 are inited so that
	 * 0x80.../0x81... address space maps to physical PSRAM (e.g. before CP starts AP).
	 * Step: CONFIG_PSRAM_INTERLEAVE_STEP 0=256B 1=128B 2=64B 3=32B. */
	psram_hal_set_interleave_config(CONFIG_PSRAM_INTERLEAVE_STEP);
#endif
	return BK_OK;
}

bk_err_t bk_psram_init_with_id(psram_id_t psram_id)
{
	bk_err_t ret = BK_OK;
	int is_skip_mutex = 0;

	if ((psram_id >= PSRAM_ID_MAX) || s_psram_init_done[psram_id])
	{
		return BK_OK;
	}

	if (!rtos_is_scheduler_started())
	{
		MEM_STATIC_LOGD("Scheduler not running, skip mutex\n");
		is_skip_mutex = 1;
		goto start_init;
	}

	if (rtos_is_in_interrupt_context())
	{
		MEM_STATIC_LOGW("Cannot init PSRAM in interrupt context!\n");
		return BK_ERR_BUSY;
	}

	if (s_psram_mutex == NULL)
	{
		ret = rtos_init_mutex(&s_psram_mutex);
		if (ret != BK_OK) {
			MEM_STATIC_LOGE("Failed to create psram mutex\n");
			return ret;
		}
	}

	rtos_lock_mutex(&s_psram_mutex);

	if (s_psram_init_done[psram_id]) {
		rtos_unlock_mutex(&s_psram_mutex);
		return BK_OK;
	}

start_init:
	MEM_STATIC_LOGD("Starting PSRAM %d init...\n", psram_id);

	uint32_t chip_id = 0, actual_id = 0;

	// psram voltage/ldo/clk are common resources, enable once
	if (!bk_psram_any_init_done()) {
		bk_psram_set_voltage(PSRAM_OUT_1_95V);
		psram_hal_power_clk_enable(1);
	}

	if (s_psram_id.magic_code == PSRAM_CHECK_FLAG) {
		chip_id = s_psram_id.psram_id;
	}

	MEM_STATIC_LOGD("%s, chip_id:%x\r\n", __func__, chip_id);

	actual_id = psram_hal_config_init_with_id(psram_id, chip_id);
	if (actual_id == 0) {
		MEM_STATIC_LOGE("%s, fail!\r\n", __func__);
		if (!is_skip_mutex) {
			rtos_unlock_mutex(&s_psram_mutex);
		}
		return BK_FAIL;
	}

	bk_delay_us(1000);
	/* SCB18X128XX_OAF uses 480M source; actual PSRAM bus clock is half (divided by 2). */
	if (actual_id == PSRAM_SCB18X128XX_OAF_ID)
		psram_hal_set_clk_with_id(psram_id, PSRAM_480M);
	else
		psram_hal_set_default_clk_with_id(psram_id);

	MEM_STATIC_LOGD("%s, %x-%x\r\n", __func__, actual_id, chip_id);

	switch (actual_id) {
		case PSRAM_W955D8MKY_5J_ID:
			if (CONFIG_PSRAM_CAPACITY != PSRAM_4M_SIZE)
				MEM_STATIC_LOGW("psram type(4MB) not match CONFIG_PSRAM_CAPACITY 0X%08X\r\n", CONFIG_PSRAM_CAPACITY);
			break;
		case PSRAM_APS6408L_ID:
			if (CONFIG_PSRAM_CAPACITY != PSRAM_8M_SIZE)
				MEM_STATIC_LOGW("psram type(8MB) not match CONFIG_PSRAM_CAPACITY 0X%08X\r\n", CONFIG_PSRAM_CAPACITY);
			break;
		case PSRAM_APS128XXO_OB9_ID:
			if (CONFIG_PSRAM_CAPACITY != PSRAM_16M_SIZE)
				MEM_STATIC_LOGW("psram type(16MB) not match CONFIG_PSRAM_CAPACITY 0X%08X\r\n", CONFIG_PSRAM_CAPACITY);
			break;
		case PSRAM_SCB18X128XX_OAF_ID:
			if (CONFIG_PSRAM_CAPACITY != PSRAM_16M_SIZE)
				MEM_STATIC_LOGW("psram type(16MB) not match CONFIG_PSRAM_CAPACITY 0X%08X\r\n", CONFIG_PSRAM_CAPACITY);
			break;
		default:
			MEM_STATIC_LOGW("Unknown PSRAM type, please check!\r\n");
			break;
	}

	if (actual_id != chip_id) {
		s_psram_id.psram_id = actual_id;
		s_psram_id.magic_code = PSRAM_CHECK_FLAG;
#if (CONFIG_PSRAM_AUTO_DETECT)
		if (s_psram_sem) {
			s_psram_id_need_write = true;
			rtos_set_semaphore(&s_psram_sem);
		}
#endif
	} else {
#if (CONFIG_PSRAM_AUTO_DETECT)
		if (s_psram_sem) {
			rtos_set_semaphore(&s_psram_sem);
		}
#endif
	}

	s_psram_init_done[psram_id] = true;

	if (!is_skip_mutex) {
		rtos_unlock_mutex(&s_psram_mutex);
	}
	MEM_STATIC_LOGD("PSRAM %d init success\n", psram_id);
	return BK_OK;
}

bk_err_t bk_psram_deinit(void)
{
	s_psram_init_done[PSRAM_ID_0] = false;
	s_psram_init_done[PSRAM_ID_1] = false;
	bk_psram_deinit_with_id(PSRAM_ID_0);
	//bk_psram_deinit_with_id(PSRAM_ID_1);
	return BK_OK;
}

bk_err_t bk_psram_deinit_with_id(psram_id_t psram_id)
{
	if ((psram_id >= PSRAM_ID_MAX) || (!s_psram_init_done[psram_id])) {
		return BK_OK;
	}

	if (s_psram_mutex) {
		rtos_lock_mutex(&s_psram_mutex);
	}

	//s_psram_init_done[psram_id] = false;

	// power down common resources only when all instances are de-inited
	if (!bk_psram_any_init_done()) {
		psram_hal_power_clk_enable(0);
	}

	if (s_psram_mutex) {
		rtos_unlock_mutex(&s_psram_mutex);
	}

	MEM_STATIC_LOGD("PSRAM deinit done\n");
	return BK_OK;
}

#if CONFIG_PSRAM_DATA_RETENTION_ENABLE

/* ============================================================
 * PSRAM data-retention helpers (used when AP / M55 subsystem
 * is powered down while CP keeps PSRAM contents alive).
 *
 * Retention path (replaces bk_psram_deinit() before AP power-off):
 *   1. flush PSRAM controller write buffer for each instance, so
 *      no dirty data is left in the controller when its clock is
 *      gated off together with the AHBP/M55 sub-system.
 *   2. latch PSRAM I/O pads at 3V (ana_reg5.gpio_latch=1) so the
 *      pad state stays valid while the M55 LDO is off.
 *   3. DO NOT call psram_hal_power_clk_enable(0); the PSRAM
 *      voltage LDO must stay on to keep external PSRAM cell data.
 *   4. mark instances as "in retention" so the recovery path knows
 *      to run; the SW init-done flag is kept untouched (cells &
 *      pads are still alive, no full re-init is required).
 *
 * Recover path (replaces bk_psram_init() after AP power-on):
 *   1. release GPIO/PSRAM pad latch.
 *   2. for each previously-active instance, redo the minimum
 *      controller sequence equivalent to the reference
 *      psram_recovery():
 *        - clock-gating bypass (REG2 bit1)
 *        - reselect 320M source, /2 divider, enable controller clock
 *        - soft-reset controller (REG2 bit0)
 *        - re-load mode register with PSRAM_MODE9 (the chip-mode
 *          value matching the current PSRAM die / clock).
 *
 * NOTE: this path assumes the PSRAM voltage rail was NOT cut. It
 * is the caller's responsibility to ensure that. With the standard
 * pm_module_shutdown_cpu1() path on bk7259, only the AP_CPU power
 * domain is gated; the PSRAM voltage and AHBP_PSRAM stay alive.
 * ============================================================ */

#define PSRAM_RETENTION_CKG_BYPASS_BIT (0x1U << 1)
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
#define PSRAM_RETENTION_MR0_ADDR       (0x00000000U)
#else
#define PSRAM_RETENTION_FLUSH_BIT      (0x1U << 3)
#endif
/* Fallback mode register value used only when no snapshot was taken
 * (e.g. retention helper called before any real PSRAM access). The
 * normal path always restores the snapshotted REG4 read live from
 * the controller right before pad-latch. PSRAM_MODE9 = 0xBC0F4049
 * matches the SCB18X128XX 240MHz setting used by bk7259 default
 * init flow (see psram_hal.c). */
#define PSRAM_RETENTION_MODE_REG_FALLBACK   (PSRAM_MODE9)

/* TEMP / LOCAL VERIFICATION SWITCH:
 *
 * The PSRAM bus-clock muxer (sys_ahbp REG8/9 cksel/ckdiv + REGA
 * pramX_cken) lives in the AHBP_PSRAM sub-domain. Under the default
 * pm_module_shutdown_cpu1() policy, only POWER_SUB_DOMAIN_NAME_AP_CPU
 * is gated; AHBP_PSRAM stays powered, so those registers should be
 * preserved across the AP power cycle. In that case re-issuing the
 * cksel / ckdiv / cken writes inside recovery is redundant.
 *
 * Set this to 0 to skip the (presumably redundant) clock re-setup
 * and verify locally that PSRAM still recovers correctly. Keep it
 * as 1 for production / when AHBP_PSRAM may also be cycled, since
 * the writes are then required for a clean controller comeback.
 * Recovery restores the live cksel / ckdiv snapshot rather than a
 * fixed reference frequency, so fast boot matches cold boot.
 *
 * After verification, either:
 *   - leave it 1 (defensive, matches reference, ~no cost), or
 *   - keep 0 if profiling shows the writes really are no-ops AND a
 *     comment makes the dependency on AHBP_PSRAM-stays-on explicit.
 */
#ifndef PM_PSRAM_RECOVER_RESET_CLOCK
#define PM_PSRAM_RECOVER_RESET_CLOCK 1
#endif

static volatile bool     s_psram_retention_active[PSRAM_ID_MAX]    = {false};
static uint32_t          s_psram_retention_saved_mode[PSRAM_ID_MAX] = {0};
static bool              s_psram_retention_mode_valid[PSRAM_ID_MAX] = {false};
static uint32_t          s_psram_retention_saved_clk_sel[PSRAM_ID_MAX] = {0};
static uint32_t          s_psram_retention_saved_clk_div[PSRAM_ID_MAX] = {0};
static bool              s_psram_retention_clock_valid[PSRAM_ID_MAX] = {false};

static bk_err_t psram_retention_drain(psram_id_t psram_id)
{
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	uint32_t mr0;

	/*
	 * AP has already stopped new work, waited for DMA idle and cleaned
	 * L1/L2 before publishing pm_ap0_sleep_state.  Serialize one real
	 * PSRAM command here so all previous controller traffic is complete
	 * before the pads are latched.
	 *
	 * Do not use REG8 bit3: it is not described by the BK7259 HAL and on
	 * current MP silicon it remains set forever instead of self-clearing.
	 */
	__asm volatile("dsb sy" ::: "memory");
	MEM_STATIC_LOGI("retention_drain begin: id=%d reg2=0x%08x reg4=0x%08x reg8=0x%08x\r\n",
		psram_id,
		psram_hal_get_reg2_value_with_id(psram_id),
		psram_hal_get_mode_value_with_id(psram_id),
		psram_hal_get_reg8_value_with_id(psram_id));
	mr0 = psram_hal_cmd_read_with_id(psram_id, PSRAM_RETENTION_MR0_ADDR);
	__asm volatile("dsb sy" ::: "memory");
	if (mr0 == 0U) {
		MEM_STATIC_LOGE("retention_drain failed: id=%d reg8=0x%08x\r\n",
			psram_id, psram_hal_get_reg8_value_with_id(psram_id));
		return BK_ERR_TIMEOUT;
	}
	MEM_STATIC_LOGI("retention_drain done: id=%d mr0=0x%08x reg8=0x%08x\r\n",
		psram_id, mr0, psram_hal_get_reg8_value_with_id(psram_id));
#else
	uint32_t reg8 = psram_hal_get_reg8_value_with_id(psram_id);

	psram_hal_set_reg8_value_with_id(psram_id,
		reg8 | PSRAM_RETENTION_FLUSH_BIT);
	while ((psram_hal_get_reg8_value_with_id(psram_id) &
		PSRAM_RETENTION_FLUSH_BIT) != 0U) {
	}
#endif
	return BK_OK;
}

static void psram_retention_save_mode(psram_id_t psram_id)
{
	s_psram_retention_saved_mode[psram_id] = psram_hal_get_mode_value_with_id(psram_id);
	s_psram_retention_mode_valid[psram_id] = true;
}

static void psram_retention_save_clock(psram_id_t psram_id)
{
	sys_drv_psram_get_clk_config_with_id((uint32_t)psram_id,
		&s_psram_retention_saved_clk_sel[psram_id],
		&s_psram_retention_saved_clk_div[psram_id]);
	s_psram_retention_clock_valid[psram_id] = true;
}

static uint32_t psram_retention_get_restore_mode(psram_id_t psram_id)
{
	if (s_psram_retention_mode_valid[psram_id]) {
		return s_psram_retention_saved_mode[psram_id];
	}
	return PSRAM_RETENTION_MODE_REG_FALLBACK;
}

static void psram_retention_recovery_one(psram_id_t psram_id)
{
	uint32_t v;
	uint32_t mode = psram_retention_get_restore_mode(psram_id);

	/* PSRAM REG2 bit1 = 1 : clock-gating bypass before clk re-select. */
	v = psram_hal_get_reg2_value_with_id(psram_id);
	v |= PSRAM_RETENTION_CKG_BYPASS_BIT;
	psram_hal_set_reg2_value_with_id(psram_id, v);

#if PM_PSRAM_RECOVER_RESET_CLOCK
	/* Restore the exact source/divider used before retention. This keeps
	 * fast-boot frequency aligned with cold boot even if the normal init
	 * clock policy changes later. An active retention instance always has
	 * a snapshot; use the HAL default only as a defensive fallback. */
	if (s_psram_retention_clock_valid[psram_id]) {
		sys_drv_psram_clk_sel_with_id((uint32_t)psram_id,
			s_psram_retention_saved_clk_sel[psram_id]);
		sys_drv_psram_set_clkdiv_with_id((uint32_t)psram_id,
			s_psram_retention_saved_clk_div[psram_id]);
	} else {
		psram_hal_set_default_clk_with_id(psram_id);
	}
	sys_drv_psram_disckg_with_id((uint32_t)psram_id, 1);     /* bus clk enable */
#endif

	/* PSRAM REG2 bit0 = 1 : soft-reset controller. */
	psram_hal_set_sf_reset_with_id(psram_id, 1);

	/* Re-load the snapshotted REG4 so the controller comes back with
	 * the exact same mode/latency setting it had before retention,
	 * regardless of which PSRAM die / clock was in use. */
	psram_hal_set_mode_value_with_id(psram_id, mode);
}

bk_err_t bk_psram_data_retention(void)
{
	bk_err_t ret;

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	MEM_STATIC_LOGI("psram_data_retention begin: init0=%d init1=%d\r\n",
		s_psram_init_done[PSRAM_ID_0], s_psram_init_done[PSRAM_ID_1]);
#endif

	for (int i = 0; i < (int)PSRAM_ID_MAX; i++) {
		if (!s_psram_init_done[i]) {
			s_psram_retention_active[i] = false;
			s_psram_retention_mode_valid[i] = false;
			s_psram_retention_clock_valid[i] = false;
			continue;
		}

		/* Snapshot the live controller mode and clock configuration BEFORE
		 * draining traffic / latching pads, so recovery exactly matches
		 * the cold-boot configuration. */
		psram_retention_save_mode((psram_id_t)i);
		psram_retention_save_clock((psram_id_t)i);

		ret = psram_retention_drain((psram_id_t)i);
		if (ret != BK_OK) {
			for (int j = 0; j <= i; j++) {
				s_psram_retention_active[j] = false;
			}
			return ret;
		}
		s_psram_retention_active[i] = true;
	}

	/* Latch PSRAM I/O pads at 3V. Covers PSRAM0 + PSRAM1. */
	sys_drv_set_psram_pad_latch(1);

	MEM_STATIC_LOGI("psram_data_retention: pads latched, p0=%d p1=%d, mode0=0x%08x mode1=0x%08x\r\n",
				   s_psram_retention_active[PSRAM_ID_0],
				   s_psram_retention_active[PSRAM_ID_1],
				   s_psram_retention_saved_mode[PSRAM_ID_0],
				   s_psram_retention_saved_mode[PSRAM_ID_1]);
	return BK_OK;
}

bk_err_t bk_psram_data_retention_recover(void)
{
	bool any_active = false;
	for (int i = 0; i < (int)PSRAM_ID_MAX; i++) {
		if (s_psram_retention_active[i]) {
			any_active = true;
			break;
		}
	}

	if (!any_active) {
		MEM_STATIC_LOGW("psram_data_retention_recover: no active retention, fall back to bk_psram_init\r\n");
		return bk_psram_init();
	}

	/* Release PSRAM pad latch before the controller drives them again. */
	sys_drv_set_psram_pad_latch(0);
	bk_delay_us(50);

	for (int i = 0; i < (int)PSRAM_ID_MAX; i++) {
		if (!s_psram_retention_active[i]) {
			continue;
		}
		psram_retention_recovery_one((psram_id_t)i);
		s_psram_init_done[i] = true;
		s_psram_retention_active[i] = false;
		s_psram_retention_mode_valid[i] = false;
		s_psram_retention_clock_valid[i] = false;
	}

	MEM_STATIC_LOGI("psram_data_retention_recover: done\r\n");
	return BK_OK;
}

#endif /* CONFIG_PSRAM_DATA_RETENTION_ENABLE */

bk_err_t bk_psram_memcpy(uint8_t *start_addr, uint8_t *data_buf, uint32_t len)
{
	int i;
	uint32_t val;
	uint8_t *pb = NULL, *pd = NULL;

	psram_id_t psram_id = bk_psram_addr_to_id((uint32_t)start_addr);
	bk_err_t init_ret = bk_psram_return_on_not_init(psram_id);
	if (init_ret != BK_OK) {
		return init_ret;
	}

	if (((uint32_t)start_addr & (PSRAM_ADDRESS_ALIGNMENT - 1)) != 0 ||
	    ((uint32_t)data_buf & (PSRAM_ADDRESS_ALIGNMENT - 1)) != 0)
	{
		MEM_STATIC_LOGE("address not aligned to %d bytes\r\n", PSRAM_ADDRESS_ALIGNMENT);
		return BK_FAIL;
	}

	while (len) {
		if (len < PSRAM_BYTES_PER_WORD) {
			val = *((uint32_t *)(start_addr));
			pb = (uint8_t *)&val;
			pd = (uint8_t *)data_buf;
			for (i = 0; i < len; i++) {
				*pb++ = *pd++;
			}
			*(uint32_t *)(start_addr) = val;
			len = 0;
		} else {
			val = *((uint32_t *)data_buf);
			*(uint32_t *)(start_addr) = val;
			data_buf += PSRAM_BYTES_PER_WORD;
			start_addr += PSRAM_BYTES_PER_WORD;
			len -= PSRAM_BYTES_PER_WORD;
		}
	}

	return BK_OK;
}

bk_err_t bk_psram_memread(uint8_t *start_addr, uint8_t *data_buf, uint32_t len)
{
	int i;
	uint32_t val;
	uint8_t *pb, *pd;

	psram_id_t psram_id = bk_psram_addr_to_id((uint32_t)start_addr);
	bk_err_t init_ret = bk_psram_return_on_not_init(psram_id);
	if (init_ret != BK_OK) {
		return init_ret;
	}

	if (((uint32_t)start_addr & (PSRAM_ADDRESS_ALIGNMENT - 1)) != 0 ||
	    ((uint32_t)data_buf & (PSRAM_ADDRESS_ALIGNMENT - 1)) != 0)
	{
		MEM_STATIC_LOGE("address not aligned to %d bytes\r\n", PSRAM_ADDRESS_ALIGNMENT);
		return BK_FAIL;
	}

	while (len) {
		if (len < PSRAM_BYTES_PER_WORD) {
			val = *((uint32_t *)(start_addr));
			pb = (uint8_t *)&val;
			pd = (uint8_t *)data_buf;
			for (i = 0; i < len; i++) {
				*pd++ = *pb++;
			}
			len = 0;
		} else {
			val = *((uint32_t *)(start_addr));
			*((uint32_t *)data_buf) = val;
			data_buf += PSRAM_BYTES_PER_WORD;
			start_addr += PSRAM_BYTES_PER_WORD;
			len -= PSRAM_BYTES_PER_WORD;
		}
	}

	return BK_OK;
}

char *bk_psram_strcat(char *start_addr, const char *data_buf)
{
	int i, j;
	uint32_t val;
	uint8_t *pb;
	uint8_t *pd = (uint8_t *)data_buf;
	uint32_t max_iterations = PSRAM_MAX_STRING_LEN / PSRAM_BYTES_PER_WORD;
	uint32_t iteration_count = 0;

	psram_id_t psram_id = bk_psram_addr_to_id((uint32_t)start_addr);
	if (bk_psram_return_on_not_init(psram_id) != BK_OK) {
		return NULL;
	}

	if (start_addr == NULL || data_buf == NULL) {
		return NULL;
	}

	if (*pd == '\0') {
		return start_addr;
	}

	do {
		if (iteration_count++ > max_iterations) {
			MEM_STATIC_LOGE("String too long or no null terminator found\r\n");
			return NULL;
		}

		val = *((uint32_t *)(start_addr));
		pb = (uint8_t *)&val;

		// Find the end of the existing string
		for (i = 0; i < PSRAM_BYTES_PER_WORD; i++) {
			if (*(pb + i) == '\0') {
				// Found end of existing string, append new data
				for (j = i; j < PSRAM_BYTES_PER_WORD && *pd != '\0'; j++) {
					*(pb + j) = *pd++;
				}

				// If we've reached the end of the input string, add null terminator
				if (*pd == '\0' && j < PSRAM_BYTES_PER_WORD) {
					*(pb + j) = '\0';
				}

				*((uint32_t *)(start_addr)) = val;

				if (*pd == '\0') {
					return start_addr;
				}
				break;
			}
		}

		// If no null terminator found in this word, move to next word
		if (i == PSRAM_BYTES_PER_WORD) {
			start_addr += PSRAM_BYTES_PER_WORD;
		}

	} while (*pd != '\0');

	return start_addr;
}



