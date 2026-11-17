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
#include <driver/otp_types.h>
#include <os/mem.h>
#include <os/os.h>
#include "hspl/hspl_res_lock.h"
#if CONFIG_SYS_SW_REGS
#include "sys_sw_regs.h"
#endif
#include "otp_driver.h"
#include "otp_hal.h"
#include "_otp.c"
typedef struct {
	otp_hal_t hal;
} otp_driver_t;

/*the smallest unit length for configuration*/

/*              permission       secure_range             mask         */
/*    otp1        4 Byte            256 Byte             32 Byte       */
/*    otp2        128 Byte           NA                  128 Byte      */
/*    puf         4   Byte           32 Byte             32 Byte       */

static otp_driver_t s_otp = {0};
static bool s_otp_driver_is_init = false;

static void bk_otp_init(otp_hal_t *hal)
{
	if(s_otp_driver_is_init) {
		return ;
	}
	hal->hw = (otp_hw_t *)OTP_LL_REG_BASE();
	hal->hw2 = (otp2_hw_t *)OTP2_LL_REG_BASE();

	s_otp_driver_is_init = true;

	return ;
}

static void otp_sleep()
{
#if CONFIG_ATE_TEST
	return ;
#endif
	/* Register level teardown, runs under the FUSE HSPL lock. The power vote
	 * is released by otp_driver_unlock() after the lock is dropped. */
#if CONFIG_AON_ENCP
	/* Keep the shared OTP clock gate on: the AP gates it off at the end of
	 * every OTP transaction, so this side must re-enable it on entry and must
	 * not clear it here. */
	otp_ll_sleep(s_otp.hal.hw);
#else
	otp_ll_hw_sleep(s_otp.hal.hw);
#endif
}

static int otp_active()
{
#if CONFIG_ATE_TEST
	return 0;
#endif
	bk_otp_init(&s_otp.hal);
	/* The OTP clock gate belongs to the SYS block shared with the AP, which
	 * clears it when its own transaction ends, so the AON_ENCP path has to
	 * enable it too instead of only toggling pdstb/clkosc_en. */
	return otp_ll_hw_active(s_otp.hal.hw);
}

static int switch_map(uint8_t map_id);

/*
 * The SSPL lock keeps a per-core recursion count, so lock and unlock must run
 * on the same core. Interrupts stay disabled across the section to prevent the
 * task from migrating; the saved state is kept per local core.
 */
#define OTP_LOCAL_CORE_NUM 2
static uint32_t s_otp_irq_flags[OTP_LOCAL_CORE_NUM];

/*
 * OTP APB power holder count.
 *
 * bk_pm records the vote as a per-domain sub-module bit, not as a per-user
 * reference count, so an OFF vote from AP and an OFF vote from CP would power
 * the domain off under each other's feet. This keeps a holder count for both
 * sides in the shared window: every holder votes ON (set-only, idempotent)
 * before it enters a transaction and only the holder that drops the count to
 * zero votes OFF. TURNING_OFF closes the window between "count reached zero"
 * and "power-off completed".
 *
 * The vote waits for a CPU0 response (up to 100ms), so it must run before the
 * FUSE spin lock is taken and interrupts are disabled.
 */
#if CONFIG_SYS_SW_REGS
#define OTP_PWR_WORD()        (&bk_sys_sw_regs_ptr()->otp_apb_pwr_hold)
#else
static volatile uint32_t s_otp_pwr_word_local;
#define OTP_PWR_WORD()        (&s_otp_pwr_word_local)
/* Keep in sync with sys_sw_regs_layout.h */
#define BK_SWR_OTP_PWR_CNT_CP_MASK   (0x000000FFU)
#define BK_SWR_OTP_PWR_CNT_AP_MASK   (0x0000FF00U)
#define BK_SWR_OTP_PWR_CNT_ALL_MASK  (BK_SWR_OTP_PWR_CNT_CP_MASK | BK_SWR_OTP_PWR_CNT_AP_MASK)
#define BK_SWR_OTP_PWR_TURNING_OFF   (0x00010000U)
#define BK_SWR_OTP_PWR_TE200_HOLD    (0x00020000U)
#define BK_SWR_OTP_PWR_TE200_CP_HOLD (0x00040000U)
#endif

#define OTP_PWR_MY_ONE        (1U << 0)                     /* CP count lives in [7:0] */
#define OTP_PWR_MY_MASK       (BK_SWR_OTP_PWR_CNT_CP_MASK)
/* TE200 holds its own permanent reference on the AP side. This side must never
 * power the domain off while that bit is set, and must never clear it. */
#define OTP_PWR_TE200_HOLD    (BK_SWR_OTP_PWR_TE200_HOLD)
/* The CP's TE200 keeps its own, separate permanent reference (bit18); only this
 * side sets or clears it, and the AP never touches it. */
#define OTP_PWR_TE200_OWN     (BK_SWR_OTP_PWR_TE200_CP_HOLD)
#define OTP_PWR_TE200_ANY     (BK_SWR_OTP_PWR_TE200_HOLD | BK_SWR_OTP_PWR_TE200_CP_HOLD)
#define OTP_PWR_WAIT_US       (20U)
#define OTP_PWR_WAIT_MAX_US   (300U * 1000U)                /* > 100ms vote timeout, only for reset leftovers */
/*
 * The peer may legitimately hold FUSE for one long ATE/PUF command, whose busy
 * bound is OTP_LONG_BUSY_TIMEOUT_US (100 ms). A 20 ms budget made concurrent
 * OTP access on the other core fail with "FUSE lock timeout" under load, so
 * allow the whole long-command window plus margin before giving up.
 */
#define OTP_FUSE_LOCK_TIMEOUT_MS  (150U)

/*
 * The shared window sits in a NOLOAD section that no boot clears, so drop the
 * stale count this side may hold from the previous image/boot before the first
 * increment. The first OTP access on CP happens before the scheduler starts,
 * so no other user can race with it.
 *
 * The CP TE200 bit belongs to this side: clear it only while dubhe has not
 * claimed it yet (TE200 is not built, or its init has not run in this image).
 * Once dubhe is up it is a live reference and must be kept, otherwise the AP
 * could power the OTP APB off under a running CP TE200 OTP access.
 */
static void otp_power_side_reset_once(void)
{
	static volatile int s_done = 0;
	uint32_t clear;

	if (__atomic_exchange_n(&s_done, 1, __ATOMIC_ACQ_REL) != 0)
		return;

	clear = OTP_PWR_MY_MASK;

#if CONFIG_MBEDTLS_ACCELERATOR || CONFIG_TRUSTENGINE
	if (!dubhe_inited)
#endif
		clear |= OTP_PWR_TE200_OWN;

	__atomic_and_fetch(OTP_PWR_WORD(), ~clear, __ATOMIC_ACQ_REL);
}

#if !CONFIG_AON_ENCP
static int otp_power_hold(void)
{
	volatile uint32_t *word = OTP_PWR_WORD();
	uint32_t w, waited = 0;

	otp_power_side_reset_once();

	for (;;) {
		w = __atomic_load_n(word, __ATOMIC_ACQUIRE);
		if ((w & BK_SWR_OTP_PWR_TURNING_OFF) == 0) {
			if ((w & OTP_PWR_MY_MASK) == OTP_PWR_MY_MASK) {
				OTP_LOGE("otp pwr: local hold overflow\r\n");
				return -1;
			}
			if (__atomic_compare_exchange_n(word, &w, w + OTP_PWR_MY_ONE, false,
			                                __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
				break;
			continue;
		}
		if (waited >= OTP_PWR_WAIT_MAX_US) {
			/* The previous holder reset in the middle of its OFF vote,
			 * clear the leftover state and retry. */
			OTP_LOGE("otp pwr: stale off state 0x%x, recover\r\n", w);
			__atomic_and_fetch(word, ~BK_SWR_OTP_PWR_TURNING_OFF, __ATOMIC_ACQ_REL);
			waited = 0;
			continue;
		}
		bk_delay_us(OTP_PWR_WAIT_US);
		waited += OTP_PWR_WAIT_US;
	}

	/*
	 * Always vote ON: ON only sets the bit and does not count, so a repeated
	 * vote is harmless. This way a count polluted by an abnormal reset can
	 * never lead to "touching unpowered hardware without having voted".
	 */
	if (otp_ll_power_on(s_otp.hal.hw) != 0) {
		(void)__atomic_sub_fetch(word, OTP_PWR_MY_ONE, __ATOMIC_ACQ_REL);
		return -1;
	}
	return 0;
}

static void otp_power_release(void)
{
	volatile uint32_t *word = OTP_PWR_WORD();
	uint32_t w, waited = 0;

	for (;;) {
		w = __atomic_load_n(word, __ATOMIC_ACQUIRE);
		if ((w & OTP_PWR_MY_MASK) == 0) {
			OTP_LOGE("otp pwr: unbalanced release\r\n");
			return;
		}
		if (w & BK_SWR_OTP_PWR_TURNING_OFF) {
			/* Can only come from a reset leftover */
			if (waited >= OTP_PWR_WAIT_MAX_US) {
				__atomic_and_fetch(word, ~BK_SWR_OTP_PWR_TURNING_OFF, __ATOMIC_ACQ_REL);
				waited = 0;
				continue;
			}
			bk_delay_us(OTP_PWR_WAIT_US);
			waited += OTP_PWR_WAIT_US;
			continue;
		}
		if (((w & BK_SWR_OTP_PWR_CNT_ALL_MASK) != OTP_PWR_MY_ONE) ||
		    ((w & OTP_PWR_TE200_ANY) != 0)) {
			/* This side still has nested holders, the other side holds, or
			 * a TE200 (AP or CP) keeps its permanent reference. */
			if (__atomic_compare_exchange_n(word, &w, w - OTP_PWR_MY_ONE, false,
			                                __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
				return;
			continue;
		}
		/* Last holder in the system: set TURNING_OFF to block new holders,
		 * then power off. */
		if (__atomic_compare_exchange_n(word, &w, BK_SWR_OTP_PWR_TURNING_OFF, false,
		                                __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
			break;
	}

	otp_ll_power_off(s_otp.hal.hw);
	/* Drop the word only if it is still the TURNING_OFF state set above. A
	 * holder that timed out on it clears the bit and increments its own count;
	 * an unconditional store would wipe that reference, letting the next
	 * holder power the domain off under it. */
	w = BK_SWR_OTP_PWR_TURNING_OFF;
	if (!__atomic_compare_exchange_n(word, &w, 0, false,
	                                 __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
		/* Someone took a reference while we were powering off: vote the
		 * domain back on for it. On failure it may touch an unpowered block,
		 * so this must not stay silent. */
		if (otp_ll_power_on(s_otp.hal.hw) != 0) {
			OTP_LOGE("otp pwr: re-vote on failed, pwr=0x%x\r\n", w);
		}
	}
}
#endif /* !CONFIG_AON_ENCP */

/*
 * CONFIG_AON_ENCP: the ENCP power domain is always on and is shared with TE200
 * and SHANHAI, so the OTP driver must not vote it on or off. An OFF vote here
 * ends in sys_drv_module_power_ctrl(PM_POWER_DOMAIN_2, OFF), which cuts TE200
 * inside the same domain and hangs the next TE200 command; the domain is
 * powered down only by pm_low_voltage_enter().
 * The shared holder word is still cleaned once per boot so a stale CP count
 * from a previous image cannot keep the AP side from powering the domain off.
 * Non-AON builds keep the per-transaction reference counting.
 */
static int otp_power_acquire(void)
{
#if CONFIG_AON_ENCP
	otp_power_side_reset_once();
	return 0;
#else
	return otp_power_hold();
#endif
}

static void otp_power_drop(void)
{
#if !CONFIG_AON_ENCP
	otp_power_release();
#endif
}

/*
 * Take the shared FUSE lock and mask local interrupts in one step, with a
 * bounded wait. The blocking variant of the same API must not be used here:
 * the lock and the saved interrupt state have to be taken atomically on the
 * same core, otherwise a preemption in between lets the unlock run on another
 * core. The wait runs with interrupts enabled so a stuck peer delays this
 * access instead of stopping the tick with interrupts masked.
 */
static int otp_fuse_lock(void)
{
	uint32_t flags = 0;
	uint32_t waited_ms = 0;
	bk_err_t ret;

	for (;;) {
		ret = bk_hspl_res_lock_irqsave(BK_HSPL_RES_FUSE, &flags);
		if (ret == BK_OK) {
			s_otp_irq_flags[portGET_CORE_ID()] = flags;
			return 0;
		}
		if (ret != BK_ERR_TIMEOUT || waited_ms >= OTP_FUSE_LOCK_TIMEOUT_MS) {
			return -1;
		}
		bk_delay_us(1000);
		waited_ms++;
	}
}

static int otp_driver_lock(uint8_t map_id)
{
	/* Blocking part first: the holder count and the PM vote must stay outside
	 * the FUSE lock and the interrupt-disabled section, otherwise waiting
	 * for the CPU0 response can deadlock with the FUSE holder. */
	if (otp_power_acquire() != 0) {
		return -1;
	}

	/* Short register-level critical section, shared with the AP. */
	if (otp_fuse_lock() != 0) {
		OTP_LOGE("otp: FUSE lock timeout\r\n");
		otp_power_drop();
		return -1;
	}
	if (switch_map(map_id) != 0) {
		bk_hspl_res_unlock(BK_HSPL_RES_FUSE);
		rtos_enable_int(s_otp_irq_flags[portGET_CORE_ID()]);
		otp_power_drop();
		return -1;
	}
	return 0;
}

static void otp_driver_unlock(void)
{
	uint32_t core = portGET_CORE_ID();

	otp_map = NULL;
	/* Release FUSE before re-enabling interrupts: the unlock must run on the
	 * core that took the lock. */
	bk_hspl_res_unlock(BK_HSPL_RES_FUSE);
	rtos_enable_int(s_otp_irq_flags[core]);
	otp_power_drop();
}
static int switch_map(uint8_t map_id)
{
	if(otp_map == NULL){
		if (map_id == 1) {
			otp_map = otp_map_1;
		} else if (map_id == 2) {
			otp_map = otp_map_2;
		} else {
			return -1;
		}
	} else {
		return -1;
	}
	return 0;
}

#define OTP_ACTIVE(map_id) \
	do { \
		int ret = otp_driver_lock(map_id); \
		if(ret != 0) { \
			return ret;\
		} \
		ret = otp_active(); \
		if(ret != 0) { \
			otp_sleep(); \
			otp_driver_unlock(); \
			return -1;\
		} \
	} while(0);

#define OTP_SLEEP() \
	do { \
		otp_sleep(); \
		otp_driver_unlock(); \
	} while(0);

static uint32_t otp_read_otp(uint32_t location)
{
	return otp_hal_read_otp(&s_otp.hal,location);
}
static uint32_t otp2_read_otp(uint32_t location)
{
	return otp2_hal_read_otp(&s_otp.hal,location);
}

static void otp2_write_otp(uint32_t location,uint32_t value)
{
	otp2_hal_write_otp(&s_otp.hal, location, value);
}

static void otp_write_otp(uint32_t location,uint32_t value)
{
	otp_hal_write_otp(&s_otp.hal,location,value);
}

static uint32_t otp_read_status()
{
	return otp_hal_read_status(&s_otp.hal);
}

static uint32_t otp_get_failcnt()
{
	return otp_hal_get_failcnt(&s_otp.hal);
}

uint32_t otp_read_otp2_permission(uint32_t location)
{
	return otp_hal_read_otp2_permission(&s_otp.hal,location);
}

static void otp_set_lck_mask(uint32_t location)
{
	otp_hal_enable_mask_lck(&s_otp.hal);
}
static uint32_t otp_read_otp_mask(uint32_t location)
{
	return otp_hal_read_otp_mask(&s_otp.hal,location);
}
static void otp_zeroize_otp(uint32_t location)
{
	otp_hal_zeroize_otp(&s_otp.hal,location);
}
static void otp_zeroize_puf(uint32_t location)
{
	otp_hal_zeroize_puf(&s_otp.hal,location);
}
static uint32_t otp_get_otp_zeroized_flag(uint32_t location)
{
	return otp_hal_read_otp_zeroized_flag(&s_otp.hal,location);
}

otp_privilege_t bk_otp_apb_read_permission(otp1_id_t item)
{
	OTP_ACTIVE(1)
	uint32_t location = otp_map[item].offset / 4;
	otp_privilege_t permission = otp_hal_read_otp_permission(&s_otp.hal,location);
	otp_privilege_t mask = otp_hal_read_otp_mask(&s_otp.hal,location);
	otp_privilege_t ret_permission = max(permission, mask);

	OTP_SLEEP()
	return ret_permission;
}

otp_privilege_t bk_otp_ahb_read_permission(otp2_id_t item)
{
	OTP_ACTIVE(2)
	uint32_t location = otp_map[item].offset / 4;
	otp_privilege_t permission = otp_hal_read_otp2_permission(&s_otp.hal,location);
	otp_privilege_t mask = otp_hal_read_otp2_mask(&s_otp.hal,location);
	otp_privilege_t ret_permission = max(permission, mask);

	OTP_SLEEP()
	return ret_permission;
}


bk_err_t bk_otp_ahb_write_permission(otp2_id_t item, otp_privilege_t permission)
{
	if (item >= OTP2_MAX_ID) {
		return BK_ERR_OTP_INDEX_WRONG;
	}
	if(permission != OTP_READ_ONLY) {
		return BK_ERR_OTP_PERMISSION_WRONG;
	}
	OTP_ACTIVE(2)
	uint32_t location = otp_map[item].offset / 4;
	int32_t size = otp_map[item].allocated_size;
	while(size > 0) {
		otp_hal_write_otp2_permission(&s_otp.hal, location, permission);
		location++;
		size-=4;
	}
	OTP_SLEEP()
	return BK_OK;
}

bk_err_t bk_otp_apb_write_permission(otp1_id_t item, otp_privilege_t permission)
{
	if (item >= OTP1_MAX_ID) {
		return BK_ERR_OTP_INDEX_WRONG;
	}
	if(permission != OTP_READ_ONLY && permission != OTP_NO_ACCESS){
		return BK_ERR_OTP_PERMISSION_WRONG;
	}
	OTP_ACTIVE(1)
	uint32_t location = otp_map[item].offset / 4;
	int32_t size = otp_map[item].allocated_size;
	while(size > 0) {
		otp_hal_write_otp_permission(&s_otp.hal, location, permission);
		location++;
		size-=4;
	}
	OTP_SLEEP()
	return BK_OK;
}

otp_privilege_t bk_otp_apb_read_mask(otp1_id_t item)
{
	OTP_ACTIVE(1)
	uint32_t location = otp_map[item].offset / 4;
	otp_privilege_t mask = otp_hal_read_otp_mask(&s_otp.hal,location);
	OTP_SLEEP()
	return mask;
}

bk_err_t bk_otp_apb_write_mask(otp1_id_t item, uint32_t mask)
{
	if (item >= OTP1_MAX_ID) {
		return BK_ERR_OTP_INDEX_WRONG;
	}
	OTP_ACTIVE(1)
	uint32_t location = otp_map[item].offset / 4;
	int32_t size = otp_map[item].allocated_size;
	while(size > 0) {
		otp_hal_write_otp_mask(&s_otp.hal, location, mask);
		location++;
		size-=4;
	}
	OTP_SLEEP()
	return BK_OK;
}

otp_privilege_t bk_otp_ahb_read_mask(otp2_id_t item)
{
	OTP_ACTIVE(2)
	uint32_t location = otp_map[item].offset / 4;
	otp_privilege_t mask = otp_hal_read_otp2_mask(&s_otp.hal,location);
	OTP_SLEEP()
	return mask;
}

bk_err_t bk_otp_ahb_write_mask(otp2_id_t item, uint32_t mask)
{
	if (item >= OTP2_MAX_ID) {
		return BK_ERR_OTP_INDEX_WRONG;
	}
	OTP_ACTIVE(2)
	uint32_t location = otp_map[item].offset / 4;
	int32_t size = otp_map[item].allocated_size;
	while(size > 0) {
		otp_hal_write_otp2_mask(&s_otp.hal, location, mask);
		location++;
		size-=4;
	}
	OTP_SLEEP()
	return BK_OK;
}

/**
 * obtain APB OTP value in little endian with item ID:
 * 1. allowed start address of item not aligned
 * 2. allowed end address of item not aligned
 */
bk_err_t bk_otp_apb_read(otp1_id_t item, uint8_t* buf, uint32_t size)
{
	if (item >= OTP1_MAX_ID) {
		return BK_ERR_OTP_INDEX_WRONG;
	}

	if( bk_otp_apb_read_permission(item) > OTP_READ_ONLY ) {
		return BK_ERR_NO_READ_PERMISSION;
	}
	OTP_ACTIVE(1)
	if(size > otp_map[item].allocated_size){
		OTP_SLEEP()
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}
	uint32_t location = otp_map[item].offset / 4;
	uint32_t start = otp_map[item].offset % 4;
	uint32_t value;

	while(size > 0) {
		value = otp_read_otp(location);

		uint8_t * src_data = (uint8_t *)&value;
		int       cpy_cnt;

		src_data += start;

		cpy_cnt = (size >= (4 - start)) ? (4 - start) : size;

		switch( cpy_cnt ) {
			case 4:
				*buf++ = *src_data++;
			case 3:
				*buf++ = *src_data++;
			case 2:
				*buf++ = *src_data++;
			case 1:
				*buf++ = *src_data++;
		}

		size -= cpy_cnt;
		location++;
		start = 0;
	}
	OTP_SLEEP()
	return BK_OK;
}

/**
 * @brief     OTP read with item real offset
 *
 * @param item_offset the real item offset to read from otp
 * @param buf point to the buffer that reads the data
 * @param size length of item to read
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_OTP_ADDR_OUT_OF_RANGE: param size not match item real size
 *    - others: other errors.
 */
bk_err_t bk_otp_apb_read_by_offset(uint32_t item_offset, uint8_t* buf, uint32_t size)
{
	OTP_ACTIVE(1)

	uint32_t location = item_offset / 4;
	uint32_t start = item_offset % 4;
	uint32_t value;

	while(size > 0) {
		value = otp_read_otp(location);

		uint8_t * src_data = (uint8_t *)&value;
		int cpy_cnt;

		src_data += start;

		cpy_cnt = (size >= (4 - start)) ? (4 - start) : size;

		switch( cpy_cnt ) {
			case 4:
				*buf++ = *src_data++;
			case 3:
				*buf++ = *src_data++;
			case 2:
				*buf++ = *src_data++;
			case 1:
				*buf++ = *src_data++;
		}

		size -= cpy_cnt;
		location++;
		start = 0;
	}
	OTP_SLEEP()
	return BK_OK;
}

/**
 * @brief     OTP2 read with item real offset
 *
 * @param item_offset the real item offset to read from otp2
 * @param buf point to the buffer that reads the data
 * @param size length of item to read
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_OTP_ADDR_OUT_OF_RANGE: buf is NULL, size is 0, or item_offset+size exceeds the OTP2 bank
 *    - others: other errors.
 */
/* OTP2 (AHB) bank size in bytes; a raw offset read must stay within this range. */
#define OTP2_AHB_BANK_SIZE  (0xC00)

bk_err_t bk_otp_ahb_read_by_offset(uint32_t item_offset, uint8_t* buf, uint32_t size)
{
	if ((buf == NULL) || (size == 0) ||
	    (size > OTP2_AHB_BANK_SIZE) ||
	    (item_offset >= OTP2_AHB_BANK_SIZE) ||
	    ((item_offset + size) > OTP2_AHB_BANK_SIZE)) {
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}

	OTP_ACTIVE(2)

	uint32_t location = item_offset / 4;
	uint32_t start = item_offset % 4;
	uint32_t value;

	while(size > 0) {
		value = otp2_read_otp(location);

		uint8_t * src_data = (uint8_t *)&value;
		int cpy_cnt;

		src_data += start;

		cpy_cnt = (size >= (4 - start)) ? (4 - start) : size;

		switch( cpy_cnt ) {
			case 4:
				*buf++ = *src_data++;
			case 3:
				*buf++ = *src_data++;
			case 2:
				*buf++ = *src_data++;
			case 1:
				*buf++ = *src_data++;
		}

		size -= cpy_cnt;
		location++;
		start = 0;
	}
	OTP_SLEEP()
	return BK_OK;
}

static uint32_t bk_otp2_check_clean_range(uint32_t start_offset, uint32_t end_offset)
{
	uint32_t start_location = start_offset >> 2;
	uint32_t end_location = (end_offset + 3) >> 2;

	for (uint32_t location = start_location; location < end_location; location++) {
		uint32_t value = otp2_read_otp(location);
		if (value != 0) {
			uint8_t *value_byte = (uint8_t *)&value;
			for (uint32_t byte = 0; byte < 4; byte++) {
				uint32_t offset = (location << 2) + byte;
				if ((offset >= start_offset) && (offset < end_offset) && (value_byte[byte] != 0)) {
					return SOC_OTP_AHB_BASE + offset;
				}
			}
		}
	}

	return 0;
}

uint32_t bk_otp_partical_clean_check_customer(void)
{
	uint32_t result;

	OTP_ACTIVE(2)

	/* BK7259 channel-binding IDs live in OTP2, aligned with BK7239. */
	result = bk_otp2_check_clean_range(0x588, 0x600);
	OTP_SLEEP()
	return result;
}

/**
 * update APB OTP value in little endian with item ID:
 * 1. allowed start address of item not aligned
 * 2. allowed end address of item not aligned
 * 3. check overwritable before write, return BK_ERR_OTP_CANNOT_WRTIE if failed
 * 4. verify value after write, return BK_ERR_OTP_UPDATE_NOT_EQUAL if failed
 */
bk_err_t bk_otp_apb_update(otp1_id_t item, uint8_t* buf, uint32_t size)
{
	if (item >= OTP1_MAX_ID) {
		return BK_ERR_OTP_INDEX_WRONG;
	}
	if( bk_otp_apb_read_permission(item) != OTP_READ_WRITE ) {
		return BK_ERR_NO_WRITE_PERMISSION;
	}
	OTP_ACTIVE(1)
	if(size > otp_map[item].allocated_size){
		OTP_SLEEP()
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}
	uint32_t location = otp_map[item].offset / 4;
	uint32_t start = otp_map[item].offset % 4;
	uint32_t value = 0;
	uint32_t check_value = 0;

	uint8_t * dst_data;
	uint8_t * back_buf = buf;
	uint32_t  back_size = size;

	while(size > 0) {
		value = otp_read_otp(location);

		int       cmp_cnt = (size >= (4 - start)) ? (4 - start) : size;
		uint32_t  cmp_result = 0;

		dst_data = (uint8_t *)&value;
		dst_data += start;

		/* the initial data of every OTP memory bit is 0. */
		/* OTP memory bit can be changed to 1, */
		/* once it is changed to 1, it can't be reset to 0. */
		/* so check the target bit value before write to OTP memory. */
		/* if try to change OTP bit from 1 to 0, return fail. */
		switch( cmp_cnt ) {
			case 4:
				cmp_result |= (~(*buf) & (*dst_data));
				buf++; dst_data++;
			case 3:
				cmp_result |= (~(*buf) & (*dst_data));
				buf++; dst_data++;
			case 2:
				cmp_result |= (~(*buf) & (*dst_data));
				buf++; dst_data++;
			case 1:
				cmp_result |= (~(*buf) & (*dst_data));
				buf++; dst_data++;
		}

		if(cmp_result != 0)
			break;

		size -= cmp_cnt;
		location++;
		start = 0;
	}

	if(size > 0) {
		OTP_SLEEP()
		return BK_ERR_OTP_UPDATE_NOT_EQUAL;  // BK_ERR_OTP_CANNOT_WRTIE; ???
	}

	/* restore all variables. */
	location = otp_map[item].offset / 4;
	start = otp_map[item].offset % 4;
	size = back_size;
	buf = back_buf;

	while(size > 0) {
		int cpy_cnt = (size >= (4 - start)) ? (4 - start) : size;

		value = 0;
		dst_data = (uint8_t *)&value;
		dst_data += start;

		switch( cpy_cnt ) {
			case 4:
				*dst_data++ = *buf++;
			case 3:
				*dst_data++ = *buf++;
			case 2:
				*dst_data++ = *buf++;
			case 1:
				*dst_data++ = *buf++;
		}

		otp_write_otp(location, value);
		check_value = otp_read_otp(location);
		if (check_value != value) {
			OTP_SLEEP()
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}

		size -= cpy_cnt;
		location++;
		start = 0;
	}

	OTP_SLEEP()
	return BK_OK;
}

/**
 * obtain AHB OTP value with item ID:
 * 1. allowed start address of item not aligned
 * 2. allowed end address of item not aligned
 */
bk_err_t bk_otp_ahb_read(otp2_id_t item, uint8_t* buf, uint32_t size)
{
	if (item >= OTP2_MAX_ID) {
		return BK_ERR_OTP_INDEX_WRONG;
	}
	if( bk_otp_ahb_read_permission(item) > OTP_READ_ONLY ) {
		return BK_ERR_NO_READ_PERMISSION;
	}
	OTP_ACTIVE(2)
	if(size > otp_map[item].allocated_size){
		OTP_SLEEP()
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}
	uint32_t location = otp_map[item].offset / 4;
	uint32_t start = otp_map[item].offset % 4;
	uint32_t value;

	while(size > 0) {
		value = otp2_read_otp(location);

		uint8_t * src_data = (uint8_t *)&value;
		int       cpy_cnt;

		src_data += start;

		cpy_cnt = (size >= (4 - start)) ? (4 - start) : size;

		switch( cpy_cnt ) {
			case 4:
				*buf++ = *src_data++;
			case 3:
				*buf++ = *src_data++;
			case 2:
				*buf++ = *src_data++;
			case 1:
				*buf++ = *src_data++;
		}

		size -= cpy_cnt;
		location++;
		start = 0;
	}
	OTP_SLEEP()
	return BK_OK;
}

bk_err_t bk_otp_ahb_update(otp2_id_t item, uint8_t* buf, uint32_t size)
{
	if (item >= OTP2_MAX_ID) {
		return BK_ERR_OTP_INDEX_WRONG;
	}
	if( bk_otp_ahb_read_permission(item) != OTP_READ_WRITE ) {
		return BK_ERR_NO_WRITE_PERMISSION;
	}
	OTP_ACTIVE(2)
	if(size > otp_map[item].allocated_size){
		OTP_SLEEP()
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}
	uint32_t location = otp_map[item].offset / 4;
	uint32_t start = otp_map[item].offset % 4;
	uint32_t value = 0;
	uint32_t check_value = 0;

	uint8_t * dst_data;
	uint8_t * back_buf = buf;
	uint32_t  back_size = size;

	while(size > 0) {
		value = otp2_read_otp(location);

		int       cmp_cnt = (size >= (4 - start)) ? (4 - start) : size;
		uint32_t  cmp_result = 0;

		dst_data = (uint8_t *)&value;
		dst_data += start;

		/* the initial data of every OTP memory bit is 0. */
		/* OTP memory bit can be changed to 1, */
		/* once it is changed to 1, it can't be reset to 0. */
		/* so check the target bit value before write to OTP memory. */
		/* if try to change OTP bit from 1 to 0, return fail. */
		switch( cmp_cnt ) {
			case 4:
				cmp_result |= (~(*buf) & (*dst_data));
				buf++; dst_data++;
			case 3:
				cmp_result |= (~(*buf) & (*dst_data));
				buf++; dst_data++;
			case 2:
				cmp_result |= (~(*buf) & (*dst_data));
				buf++; dst_data++;
			case 1:
				cmp_result |= (~(*buf) & (*dst_data));
				buf++; dst_data++;
		}

		if(cmp_result != 0)
			break;

		size -= cmp_cnt;
		location++;
		start = 0;
	}

	if(size > 0) {
		OTP_SLEEP()
		return BK_ERR_OTP_UPDATE_NOT_EQUAL;  // BK_ERR_OTP_CANNOT_WRTIE; ???
	}

	/* restore all variables. */
	location = otp_map[item].offset / 4;
	start = otp_map[item].offset % 4;
	size = back_size;
	buf = back_buf;

	while(size > 0) {
		int cpy_cnt = (size >= (4 - start)) ? (4 - start) : size;

		value = 0;
		dst_data = (uint8_t *)&value;
		dst_data += start;

		switch( cpy_cnt ) {
			case 4:
				*dst_data++ = *buf++;
			case 3:
				*dst_data++ = *buf++;
			case 2:
				*dst_data++ = *buf++;
			case 1:
				*dst_data++ = *buf++;
		}

		otp2_write_otp(location, value);
		check_value = otp2_read_otp(location);
		if (check_value != value) {
			OTP_SLEEP()
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}

		size -= cpy_cnt;
		location++;
		start = 0;
	}

	OTP_SLEEP()

	return BK_OK;
}

bk_err_t bk_otp_read_random_number(uint32_t* value, uint32_t size)
{
	OTP_ACTIVE(1)
	for(int i = 0; i < size; ++i){
		value[i] = otp_hal_read_random_number(&s_otp.hal);
	}
	OTP_SLEEP()
	return BK_OK;
}

/*Initial Phase,done at factory*/
#if CONFIG_ATE_TEST
static bk_err_t bk_otp_init_puf()
{
	if(otp_hal_read_enroll(&s_otp.hal) == 0xF){
		return BK_OK;
	} else if(otp_hal_read_enroll(&s_otp.hal) == 0x0){
		otp_hal_do_puf_enroll(&s_otp.hal);
	}
	uint32_t flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("puf enrollment fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("puf enrollment has been executed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMlck is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	if(otp_hal_read_enroll(&s_otp.hal) != 0xF){
		OTP_LOGE("Error code:0x%x\r\n",otp_hal_read_status(&s_otp.hal));
		return BK_ERR_OTP_OPERATION_FAIL;
	}
	return BK_OK;
}

bk_err_t bk_otp_read_raw_entropy_output(uint32_t* value)
{
	otp_hal_set_fre_cont(&s_otp.hal, 1);
	*value = otp_hal_read_random_number(&s_otp.hal);
	otp_hal_set_fre_cont(&s_otp.hal, 0);
	return BK_OK;
}

static bk_err_t bk_otp_check_puf_quality()
{
	uint32_t flag = otp_hal_do_puf_quality_check(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("quality check fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGE("please init puf before quality check.\r\n");
		return BK_ERR_OTP_OPERATION_WARNING;
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	}
	return BK_OK;
}

static bk_err_t bk_otp_check_puf_health()
{
	uint32_t flag;

	(void)otp_hal_do_puf_health_check(&s_otp.hal);
	if (otp_check_busy_timeout(s_otp.hal.hw, OTP_LONG_BUSY_TIMEOUT_US) != 0) {
		return BK_ERR_OTP_OPERATION_ERROR;
	}
	flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("health check fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGE("please init puf before quality check.\r\n");
		return BK_ERR_OTP_OPERATION_WARNING;
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	}
	return BK_OK;
}

static bk_err_t bk_otp_do_auto_repair()
{
	uint32_t status = otp_hal_do_auto_repair(&s_otp.hal);
	uint32_t flag = status & 0x1F;
	uint32_t failcnt = (status & 0xF00) >> 8;

	if(failcnt != 0){
		OTP_LOGE("The total number of failed address is 0x%x.\r\n",failcnt);
	}
	if(failcnt > 4){
		return failcnt;
	}
	if(flag & OTP_ERROR){
		OTP_LOGE("auto repair fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("auto repair has been previously executed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	return BK_OK;
}

static bk_err_t bk_otp_enable_shuffle_read()
{
	otp_hal_set_flag(&s_otp.hal,0x99);
	uint32_t flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("enable shuffle read fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("shuffle read has been re-programmed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	volatile int delay = 1000;
	while(otp_hal_read_shfren(&s_otp.hal) != 0xF && delay){
		--delay;
	}
	otp_hal_deinit(&s_otp.hal);
	if(otp_hal_init(&s_otp.hal) != BK_OK){
		return BK_ERR_OTP_INIT_FAIL;
	}
	if (otp_check_busy_timeout(s_otp.hal.hw, OTP_LONG_BUSY_TIMEOUT_US) != 0) {
		return BK_ERR_OTP_OPERATION_ERROR;
	}
	return BK_OK;
}

static bk_err_t bk_otp_enable_shuffle_write()
{
	otp_hal_set_flag(&s_otp.hal,0xc2);
	uint32_t flag;

	if (otp_check_busy_timeout(s_otp.hal.hw, OTP_LONG_BUSY_TIMEOUT_US) != 0) {
		return BK_ERR_OTP_OPERATION_ERROR;
	}
	flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("enable shuffle write fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("shuffle write has been re-programmed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	volatile int delay = 1000;
	while(otp_hal_read_shfwen(&s_otp.hal) != 0xF && delay){
		--delay;
	}
	otp_hal_deinit(&s_otp.hal);
	if(otp_hal_init(&s_otp.hal) != BK_OK){
		return BK_ERR_OTP_INIT_FAIL;
	}
	if (otp_check_busy_timeout(s_otp.hal.hw, OTP_LONG_BUSY_TIMEOUT_US) != 0) {
		return BK_ERR_OTP_OPERATION_ERROR;
	}
	return BK_OK;
}

static bk_err_t bk_otp_set_ptc_page(uint32_t page)
{
	otp_hal_set_ptc_page(&s_otp.hal,page);
	return BK_OK;
}

static bk_err_t bk_otp_test_page(uint32_t start, uint32_t end)
{
	uint32_t data = 0x5A5A5A5A;
	uint8_t bit = 0x1;
	for(uint32_t i = start;i < end;i++){
		otp_hal_write_test_row(&s_otp.hal,i,data);
		if(otp_hal_read_test_row(&s_otp.hal,i) != data){
			OTP_LOGE("test row %d = 0x%x,should be 0x%x.\r\n",i,otp_hal_read_test_row(&s_otp.hal,i),data);
			return BK_ERR_OTP_OPERATION_ERROR;
		}
		for(uint32_t j = 0; j < 4;j++){
			otp_hal_set_ptc_page(&s_otp.hal,j);
			otp_hal_write_test_column(&s_otp.hal,i,bit);
			if(otp_hal_read_test_column(&s_otp.hal,i) != bit){
				OTP_LOGE("test column %d  in page %d = 0x%x,should be 0x%x.\r\n",i,otp_hal_read_test_column(&s_otp.hal,i),bit);
				return BK_ERR_OTP_OPERATION_ERROR;
			}
		}
	}
	return BK_OK;
}

static bk_err_t bk_otp_off_margin_read()
{
	otp_hal_enable_off_margin_read(&s_otp.hal);
	if(bk_otp_test_page(8,12) == BK_OK){
		otp_hal_disable_off_margin_read(&s_otp.hal);
		return BK_OK;
	}
	OTP_LOGE("off margin read fail.\r\n");
	return BK_ERR_OTP_OPERATION_ERROR;
}

static bk_err_t bk_otp_on_margin_read()
{
	otp_hal_enable_on_margin_read(&s_otp.hal);
	if(bk_otp_test_page(12,16) == BK_OK){
		otp_hal_disable_on_margin_read(&s_otp.hal);
		return BK_OK;
	}
	OTP_LOGE("on margin read fail.\r\n");
	return BK_ERR_OTP_OPERATION_ERROR;
}

static bk_err_t bk_otp_initial_off_check()
{
	uint32_t flag = otp_hal_init_off_check(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("initial_off_check fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	return BK_OK;
}

static bk_err_t bk_otp_enable_program_protect_function()
{
	if(otp_hal_read_pgmprt(&s_otp.hal) != 0xF){
		return BK_OK;
	}
	otp_hal_set_flag(&s_otp.hal,0xb6);
	uint32_t flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("program_protect fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("program_protect has been previously executed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGW("PTM data is not correctly set.\r\n");
		return BK_OK;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	if(otp_hal_read_pgmprt(&s_otp.hal) != 0xF){
		OTP_LOGE("program_protect fail.\r\n");
		return BK_ERR_OTP_OPERATION_FAIL;
	}
	return BK_OK;
}

static bk_err_t bk_otp_enable_test_mode_lock_function()
{
	if(otp_hal_read_tmlck(&s_otp.hal) == 0xF){
		return BK_OK;
	}
	otp_hal_set_flag(&s_otp.hal,0x71);
	uint32_t flag = otp_hal_read_status(&s_otp.hal);
	if(flag & OTP_ERROR){
		OTP_LOGE("test_mode_lock fail.\r\n");
		return BK_ERR_OTP_OPERATION_ERROR;
	} else if(flag & OTP_WARNING) {
		OTP_LOGW("test_mode_lock has been previously executed.\r\n");
	} else if(flag & OTP_WRONG) {
		OTP_LOGE("PTM data is not correctly set.\r\n");
		return BK_ERR_OTP_OPERATION_WRONG;
	} else if(flag & OTP_FORBID) {
		OTP_LOGE("TMLCK is locked.\r\n");
		return BK_ERR_OTP_OPERATION_FORBID;
	}
	if(otp_hal_read_tmlck(&s_otp.hal) != 0xF){
		OTP_LOGE("test_mode_lock fail.\r\n");
		return BK_ERR_OTP_OPERATION_FAIL;
	}
	return BK_OK;
}

uint32_t bk_otp_fully_flow_test()
{
	OTP_ACTIVE(1)
	uint32_t ret;
	uint32_t retry_count = 10;
	uint32_t fail_code = 0;
	/*test mode lck means test has been success*/
	if(otp_hal_read_tmlck(&s_otp.hal) == 0xF){
		ret = BK_OK;
		goto exit;
	}

	ret = bk_otp_init_puf();
	if(ret != BK_OK){
		fail_code |= 1 << 0;
		goto exit;
	}
	ret = bk_otp_check_puf_quality();
	if(ret != BK_OK){
		fail_code |= 1 << 1;
		goto exit;
	}
	ret = bk_otp_check_puf_health();
	if(ret != BK_OK){
		fail_code |= 1 << 2;
		goto exit;
	}
	ret = bk_otp_do_auto_repair();
	if(ret != BK_OK){
		fail_code |= 1 << 3;
		if (ret > 0)
			fail_code |= ret << 12;
		goto exit;
	}

	while(otp_hal_read_shfwen(&s_otp.hal) != 0xF && retry_count){
		ret = bk_otp_enable_shuffle_write();
		--retry_count;
		if(ret != BK_OK){
			fail_code |= 1 << 4;
			goto exit;
		}
	}
	if(otp_hal_read_shfwen(&s_otp.hal) != 0xF){
		OTP_LOGE("enable shuffle write fail.\r\n");
		ret = BK_ERR_OTP_OPERATION_FAIL;
		goto exit;
	}

	retry_count = 10;
	while(otp_hal_read_shfren(&s_otp.hal) != 0xF && retry_count){
		ret = bk_otp_enable_shuffle_read();
		--retry_count;
		if(ret != BK_OK){
			fail_code |= 1 << 5;
			goto exit;
		}
	}

	if(otp_hal_read_shfren(&s_otp.hal) != 0xF){
		OTP_LOGE("enable shuffle read fail.\r\n");
		ret = BK_ERR_OTP_OPERATION_FAIL;
		goto exit;
	}

	ret = bk_otp_test_page(0,8);
	if(ret != BK_OK){
		fail_code |= 1 << 6;
		goto exit;
	}
	ret = bk_otp_off_margin_read();
	if(ret != BK_OK){
		fail_code |= 1 << 7;
		goto exit;
	}
	ret = bk_otp_on_margin_read();
	if(ret != BK_OK){
		fail_code |= 1 << 8;
		goto exit;
	}
	ret = bk_otp_initial_off_check();
	if(ret != BK_OK){
		fail_code |= 1 << 9;
		goto exit;
	}
	ret = bk_otp_enable_program_protect_function();
	if(ret != BK_OK){
		fail_code |= 1 << 10;
		goto exit;
	}
	ret = bk_otp_enable_test_mode_lock_function();
	if(ret != BK_OK){
		fail_code |= 1 << 11;
		goto exit;
	}

exit:
	OTP_SLEEP()
	switch (ret)
	{
		case BK_ERR_OTP_OPERATION_ERROR:
			fail_code |= 1 << 16;
			break;

		case BK_ERR_OTP_OPERATION_WRONG:
			fail_code |= 1 << 17;
			break;

		case BK_ERR_OTP_OPERATION_FORBID:
			fail_code |= 1 << 18;
			break;

		case BK_ERR_OTP_OPERATION_FAIL:
			fail_code |= 1 << 19;
			break;

		case BK_ERR_OTP_INIT_FAIL:
			fail_code |= 1 << 20;
			break;

		default:
			break;
	}
	return fail_code;
}
#endif /*CONFIG_ATE_TEST*/

typedef struct {
    otp2_id_t main_item;
    otp2_id_t backup_item;
    uint32_t alloc_len;
    uint32_t actual_len;
} otp_backup_cfg_t;

static bool otp_get_backup_cfg(otp2_id_t item, otp_backup_cfg_t *cfg)
{
    switch (item) {
    case OTP_DIA:
    case OTP_DIA_BACKUP:
        cfg->main_item = OTP_DIA;
        cfg->backup_item = OTP_DIA_BACKUP;
        cfg->alloc_len = 4;
        cfg->actual_len = 1;
        break;
    case OTP_VDDDIG_BANDGAP:
    case OTP_VDDDIG_BANDGAP_BACKUP:
        cfg->main_item = OTP_VDDDIG_BANDGAP;
        cfg->backup_item = OTP_VDDDIG_BANDGAP_BACKUP;
        cfg->alloc_len = 4;
        cfg->actual_len = 1;
        break;
    case OTP_GADC_TEMPERATURE:
    case OTP_GADC_TEMPERATURE_BACKUP:
        cfg->main_item = OTP_GADC_TEMPERATURE;
        cfg->backup_item = OTP_GADC_TEMPERATURE_BACKUP;
        cfg->alloc_len = 8;
        cfg->actual_len = 4;
        break;
    case OTP_GADC_CALIBRATION:
    case OTP_GADC_CALIBRATION_BACKUP:
        cfg->main_item = OTP_GADC_CALIBRATION;
        cfg->backup_item = OTP_GADC_CALIBRATION_BACKUP;
        cfg->alloc_len = 72;
        cfg->actual_len = 70;
        break;
    default:
        return false;
    }

    return true;
}

bk_err_t bk_otp_ahb_read_backup(otp2_id_t item, uint8_t *value, int *both_valid)
{
    bk_err_t ret = BK_OK;
    otp_backup_cfg_t cfg;

    if (!otp_get_backup_cfg(item, &cfg)) {
        return BK_FAIL;
    }

    uint8_t *buf_main = (uint8_t *)os_malloc(cfg.alloc_len);
    uint8_t *buf_backup = (uint8_t *)os_malloc(cfg.alloc_len);
    if (!buf_main || !buf_backup) {
        ret = BK_FAIL;
        goto exit;
    }

    os_memset(buf_main, 0, cfg.alloc_len);
    os_memset(buf_backup, 0, cfg.alloc_len);
    bk_err_t ret_main = bk_otp_ahb_read(cfg.main_item, buf_main, cfg.actual_len);
    bk_err_t ret_backup = bk_otp_ahb_read(cfg.backup_item, buf_backup, cfg.actual_len);

    bool backup_not_empty = false;
    bool main_not_empty = false;

    for (uint32_t i = 0; i < cfg.actual_len; i++) {
        if (buf_backup[i] != 0x00) {
            backup_not_empty = true;
            break;
        }
    }
    for (uint32_t i = 0; i < cfg.actual_len; i++) {
        if (buf_main[i] != 0x00) {
            main_not_empty = true;
            break;
        }
    }

    if ((ret_backup == BK_OK) && backup_not_empty) {
        os_memcpy(value, buf_backup, cfg.actual_len);
        if (both_valid) {
            *both_valid = (ret_main == BK_OK) && main_not_empty;
        }
    } else if ((ret_main == BK_OK) && main_not_empty) {
        os_memcpy(value, buf_main, cfg.actual_len);
        if (both_valid) {
            *both_valid = 0;
        }
    } else if ((ret_main == BK_OK) || (ret_backup == BK_OK)) {
        os_memset(value, 0, cfg.actual_len);
        if (both_valid) {
            *both_valid = 0;
        }
    } else {
        ret = BK_FAIL;
        goto exit;
    }

exit:
    if (buf_main) {
        os_free(buf_main);
    }
    if (buf_backup) {
        os_free(buf_backup);
    }
    return ret;
}

bk_err_t bk_otp_ahb_update_backup(otp2_id_t item, uint8_t *value, bool write_backup, uint8_t *readback)
{
    bk_err_t ret = BK_FAIL;
    otp_backup_cfg_t cfg;
    otp2_id_t target_item;

    if (!otp_get_backup_cfg(item, &cfg)) {
        return BK_FAIL;
    }

    uint8_t *buf = (uint8_t *)os_malloc(cfg.alloc_len);
    if (!buf) {
        return BK_FAIL;
    }

    if (write_backup) {
        uint8_t *main_buf = buf;
        os_memset(main_buf, 0, cfg.alloc_len);
        ret = bk_otp_ahb_read(cfg.main_item, main_buf, cfg.actual_len);

        bool main_not_empty = false;
        for (uint32_t i = 0; i < cfg.actual_len; i++) {
            if (main_buf[i] != 0x00) {
                main_not_empty = true;
                break;
            }
        }

        if ((ret == BK_OK) && main_not_empty) {
            target_item = cfg.backup_item;
        } else {
            target_item = cfg.main_item;
        }
    } else {
        target_item = cfg.main_item;
    }

    uint8_t *read_buf = buf;
    os_memset(read_buf, 0, cfg.alloc_len);
    ret = bk_otp_ahb_read(target_item, read_buf, cfg.actual_len);
    if (ret != BK_OK) {
        os_free(buf);
        return BK_FAIL;
    }

    bool read_not_empty = false;
    for (uint32_t i = 0; i < cfg.actual_len; i++) {
        if (read_buf[i] != 0x00) {
            read_not_empty = true;
            break;
        }
    }

    bool write_different = false;
    if (os_memcmp(read_buf, value, cfg.actual_len) != 0) {
        write_different = true;
    }

    if (read_not_empty && write_different) {
        os_free(buf);
        return BK_FAIL;
    }

    uint8_t *write_buf = buf;
    os_memset(write_buf, 0, cfg.alloc_len);
    os_memcpy(write_buf, value, cfg.actual_len);
    ret = bk_otp_ahb_update(target_item, write_buf, cfg.alloc_len);
    if (ret != BK_OK) {
        os_free(buf);
        return BK_FAIL;
    }

    os_memset(read_buf, 0, cfg.alloc_len);
    ret = bk_otp_ahb_read(target_item, read_buf, cfg.actual_len);
    if (ret != BK_OK) {
        os_free(buf);
        return BK_FAIL;
    }

    if (readback) {
        os_memcpy(readback, read_buf, cfg.actual_len);
    }

    os_free(buf);
    return BK_OK;
}

