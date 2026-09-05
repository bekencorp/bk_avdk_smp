// Copyright 2020-2026 Beken
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

#include "hspl_driver.h"
#include <components/log.h>
#include <driver/int.h>

#define HSPL_TAG "hspl"
#define HSPL_LOGI(...) BK_LOGI(HSPL_TAG, ##__VA_ARGS__)
#define HSPL_LOGE(...) BK_LOGE(HSPL_TAG, ##__VA_ARGS__)


/* Register offsets (word offsets, multiplied by 4 when accessing) */
#define HSPL_REG_DEVID       (0x0)
#define HSPL_REG_VERID       (0x1)
#define HSPL_REG_CLKRST      (0x2)
#define HSPL_REG_STATE       (0x3)
#define HSPL_REG_TIMEOUT_CFG (0x4)
#define HSPL_REG_TIMEOUT_STA (0x5)
#define HSPL_REG_TIMEOUT_CTL (0x6)

#define HSPL_REG_LOCK0       (0x10) /* 0x10..0x1F: LOCK registers for channel 0..15 */
#define HSPL_REG_STA0        (0x20) /* 0x20..0x2F: STA registers for channel 0..15 (read-only helper) */

/* HSPL lock register encoding (based on BK7259 verification tests) */
#define HSPL_LOCK_SUCCESS_BIT     (1U << 0)  /* read returns 1 when lock succeeds */
#define HSPL_LOCK_OWNER_SHIFT     (1)
#define HSPL_LOCK_OWNER_MASK      (0xFU << HSPL_LOCK_OWNER_SHIFT)
#define HSPL_LOCK_OWNER_VALID_BIT (1U << 5)  /* read returns 6'b1xxxx0 when already locked */

/* HSPL STA register encoding (based on BK7259 verification tests) */
#define HSPL_STA_OWNER_SHIFT      (1)
#define HSPL_STA_OWNER_MASK       (0xFU << HSPL_STA_OWNER_SHIFT)
#define HSPL_STA_OWNER_VALID_BIT  (1U << 5)

/* Timeout config (based on BK7259 verification tests) */
#define HSPL_TIMEOUT_EN_BIT       (1U << 24)
#define HSPL_TIMEOUT_SEL_SHIFT    (20)
#define HSPL_TIMEOUT_SEL_MASK     (0xFU << HSPL_TIMEOUT_SEL_SHIFT)
#define HSPL_TIMEOUT_TH_MASK      (0xFFFFFU) /* bits[19:0] */

/* Timeout control (based on BK7259 verification tests) */
#define HSPL_TIMEOUT_IRQ_CLR_BIT  (1U << 0)
#define HSPL_TIMEOUT_IRQ_EN_BIT   (1U << 1)

static inline uintptr_t hspl_get_base(bk_hspl_id_t hspl_id)
{
	switch (hspl_id) {
	case BK_HSPL_ID_0:
		return (uintptr_t)SOC_HSPL0_REG_BASE;
	case BK_HSPL_ID_1:
		return (uintptr_t)SOC_HSPL1_REG_BASE;
	default:
		return 0;
	}
}

/* Register access macros */
#define HSPL_REG_RD32(base, offset)      (*((volatile uint32_t *)((base) + (offset) * 4)))
#define HSPL_REG_WR32(base, offset, val) (*((volatile uint32_t *)((base) + (offset) * 4)) = (val))

static bool s_hspl_driver_init = false;

typedef struct {
	hspl_timeout_callback_t cb;
	void *cb_param;
	uint8_t sel_channel;
} hspl_timeout_ctx_t;

static hspl_timeout_ctx_t s_timeout_ctx[BK_HSPL_ID_MAX] = {0};

static inline bool hspl_is_valid_channel(uint8_t channel)
{
	return channel < HSPL_CHANNEL_MAX;
}

static inline void hspl_lazy_init(void)
{
	if (s_hspl_driver_init) {
		return;
	}

	(void)bk_hspl_driver_init();
}

/*
 * Local HSPL interrupt handling:
 * - CP(M52) handles BK_HSPL_ID_0
 * - AP(M55) handles BK_HSPL_ID_1
 *
 * 
 * so CP always uses BK_HSPL_ID_0 and INT_SRC_HSPL (69).
 */

/* CP side: handle BK_HSPL_ID_0 */
#define HSPL_LOCAL_HSPL_ID  BK_HSPL_ID_0
#define HSPL_LOCAL_INT_SRC  INT_SRC_HSPL

bk_err_t bk_hspl_driver_init(void)
{
	uintptr_t base0 = hspl_get_base(BK_HSPL_ID_0);
	uintptr_t base1 = hspl_get_base(BK_HSPL_ID_1);

	if (s_hspl_driver_init) {
		return BK_OK;
	}

	if (base0) {
		HSPL_REG_WR32(base0, HSPL_REG_CLKRST, 0x1);
		HSPL_REG_WR32(base0, HSPL_REG_TIMEOUT_CFG, 0x0);
		HSPL_REG_WR32(base0, HSPL_REG_TIMEOUT_CTL, 0x0);
	}

	if (base1) {
		HSPL_REG_WR32(base1, HSPL_REG_CLKRST, 0x1);
		HSPL_REG_WR32(base1, HSPL_REG_TIMEOUT_CFG, 0x0);
		HSPL_REG_WR32(base1, HSPL_REG_TIMEOUT_CTL, 0x0);
	}

	bk_int_isr_register(HSPL_LOCAL_INT_SRC, bk_hspl_isr_dispatch, NULL);

	s_hspl_driver_init = true;
	// HSPL_LOGI("init ok, hspl0_base=0x%08X hspl1_base=0x%08X\r\n",
	//           (unsigned int)base0, (unsigned int)base1);
	return BK_OK;
}

bk_err_t bk_hspl_driver_deinit(void)
{
	uintptr_t base0 = hspl_get_base(BK_HSPL_ID_0);
	uintptr_t base1 = hspl_get_base(BK_HSPL_ID_1);

	if (!s_hspl_driver_init) {
		return BK_OK;
	}

	if (base0) {
		HSPL_REG_WR32(base0, HSPL_REG_TIMEOUT_CFG, 0x0);
		HSPL_REG_WR32(base0, HSPL_REG_TIMEOUT_CTL, 0x0);
	}
	if (base1) {
		HSPL_REG_WR32(base1, HSPL_REG_TIMEOUT_CFG, 0x0);
		HSPL_REG_WR32(base1, HSPL_REG_TIMEOUT_CTL, 0x0);
	}

	for (int i = 0; i < BK_HSPL_ID_MAX; i++) {
		s_timeout_ctx[i].cb = NULL;
		s_timeout_ctx[i].cb_param = NULL;
		s_timeout_ctx[i].sel_channel = 0;
	}

	s_hspl_driver_init = false;
	return BK_OK;
}

#if CONFIG_DEEP_LV
bk_err_t bk_hspl_deep_lv_resume_reinit(void)
{
	uintptr_t base = hspl_get_base(BK_HSPL_ID_0);

	if (!base) {
		return BK_FAIL;
	}

	HSPL_REG_WR32(base, HSPL_REG_CLKRST, 0x1);
	HSPL_REG_WR32(base, HSPL_REG_TIMEOUT_CFG, 0x0);
	HSPL_REG_WR32(base, HSPL_REG_TIMEOUT_CTL, 0x0);
	__asm volatile("dsb sy" ::: "memory");
	__asm volatile("isb sy" ::: "memory");

	if ((HSPL_REG_RD32(base, HSPL_REG_CLKRST) & 0x1U) == 0U) {
		return BK_FAIL;
	}

	return BK_OK;
}
#endif

uint32_t bk_hspl_read_lock_raw(bk_hspl_id_t hspl_id, uint8_t channel)
{
	uintptr_t base = hspl_get_base(hspl_id);
	if (!hspl_is_valid_channel(channel) || !base) {
		return 0;
	}
	return HSPL_REG_RD32(base, HSPL_REG_LOCK0 + channel);
}

uint32_t bk_hspl_read_sta_raw(bk_hspl_id_t hspl_id, uint8_t channel)
{
	uintptr_t base = hspl_get_base(hspl_id);
	if (!hspl_is_valid_channel(channel) || !base) {
		return 0;
	}
	return HSPL_REG_RD32(base, HSPL_REG_STA0 + channel);
}

bk_err_t bk_hspl_get_state(bk_hspl_id_t hspl_id, uint8_t channel, hspl_state_t *state)
{
	uint32_t sta;
	if (!state || !hspl_is_valid_channel(channel)) {
		return BK_FAIL;
	}

	sta = bk_hspl_read_sta_raw(hspl_id, channel);
	state->owner_valid = (sta & HSPL_STA_OWNER_VALID_BIT) ? 1 : 0;
	state->locked = state->owner_valid ? 1 : 0;
	state->owner_id = (uint8_t)((sta & HSPL_STA_OWNER_MASK) >> HSPL_STA_OWNER_SHIFT);
	return BK_OK;
}

bk_err_t bk_hspl_try_lock(bk_hspl_id_t hspl_id, uint8_t channel, uint8_t *owner_id)
{
	uint32_t val;
	uintptr_t base = hspl_get_base(hspl_id);

	hspl_lazy_init();

	if (!hspl_is_valid_channel(channel) || !base) {
		return BK_FAIL;
	}

	val = bk_hspl_read_lock_raw(hspl_id, channel);

	if (val & HSPL_LOCK_SUCCESS_BIT) {
		if (owner_id) {
			*owner_id = 0;
		}
		return BK_OK;
	}

	if (owner_id) {
		if (val & HSPL_LOCK_OWNER_VALID_BIT) {
			*owner_id = (uint8_t)((val & HSPL_LOCK_OWNER_MASK) >> HSPL_LOCK_OWNER_SHIFT);
		} else {
			*owner_id = 0xFF;
		}
	}

	return BK_FAIL;
}

bk_err_t bk_hspl_unlock(bk_hspl_id_t hspl_id, uint8_t channel)
{
	uintptr_t base = hspl_get_base(hspl_id);
	hspl_lazy_init();

	if (!hspl_is_valid_channel(channel) || !base) {
		return BK_FAIL;
	}

	HSPL_REG_WR32(base, HSPL_REG_LOCK0 + channel, HSPL_UNLOCK_MAGIC);
	return BK_OK;
}

bk_err_t bk_hspl_timeout_config(bk_hspl_id_t hspl_id, uint8_t channel, uint32_t threshold_cycles, bool enable)
{
	uint32_t cfg;
	uintptr_t base = hspl_get_base(hspl_id);

	hspl_lazy_init();

	if (!hspl_is_valid_channel(channel) || !base) {
		return BK_FAIL;
	}

	s_timeout_ctx[hspl_id].sel_channel = channel;

	cfg = ((channel << HSPL_TIMEOUT_SEL_SHIFT) & HSPL_TIMEOUT_SEL_MASK) |
	      (threshold_cycles & HSPL_TIMEOUT_TH_MASK);

	if (enable) {
		cfg |= HSPL_TIMEOUT_EN_BIT;
	}

	HSPL_REG_WR32(base, HSPL_REG_TIMEOUT_CFG, cfg);
	return BK_OK;
}

bk_err_t bk_hspl_timeout_irq_enable(bk_hspl_id_t hspl_id, bool enable)
{
	uintptr_t base = hspl_get_base(hspl_id);
	uint32_t ctl;

	hspl_lazy_init();

	if (!base) {
		return BK_FAIL;
	}

	ctl = HSPL_REG_RD32(base, HSPL_REG_TIMEOUT_CTL);
	if (enable) {
		ctl |= HSPL_TIMEOUT_IRQ_EN_BIT;
	} else {
		ctl &= ~HSPL_TIMEOUT_IRQ_EN_BIT;
	}
	HSPL_REG_WR32(base, HSPL_REG_TIMEOUT_CTL, ctl);
	return BK_OK;
}

void bk_hspl_timeout_irq_clear(bk_hspl_id_t hspl_id)
{
	uintptr_t base = hspl_get_base(hspl_id);
	uint32_t ctl;

	hspl_lazy_init();

	if (!base) {
		return;
	}

	/*
	 * Per verification test: write 1 clears the interrupt.
	 * Use RMW to preserve IRQ enable bit.
	 */
	ctl = HSPL_REG_RD32(base, HSPL_REG_TIMEOUT_CTL);
	HSPL_REG_WR32(base, HSPL_REG_TIMEOUT_CTL, ctl | HSPL_TIMEOUT_IRQ_CLR_BIT);
}

bk_err_t bk_hspl_register_timeout_callback(bk_hspl_id_t hspl_id, hspl_timeout_callback_t cb, void *param)
{
	if (hspl_id >= BK_HSPL_ID_MAX) {
		return BK_ERR_PARAM;
	}

	s_timeout_ctx[hspl_id].cb = cb;
	s_timeout_ctx[hspl_id].cb_param = param;
	return BK_OK;
}

void bk_hspl_isr_dispatch(void)
{
	/* Clear and callback for local HSPL instance */
	bk_hspl_timeout_irq_clear(HSPL_LOCAL_HSPL_ID);

	if (s_timeout_ctx[HSPL_LOCAL_HSPL_ID].cb) {
		s_timeout_ctx[HSPL_LOCAL_HSPL_ID].cb(s_timeout_ctx[HSPL_LOCAL_HSPL_ID].sel_channel,
		                                     s_timeout_ctx[HSPL_LOCAL_HSPL_ID].cb_param);
	}
}

