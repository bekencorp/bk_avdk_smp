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
#include <modules/pm.h>

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

static uint32_t hspl_get_bus_clock_hz(pm_cpu_freq_e cpu_freq)
{
	switch (cpu_freq) {
	case PM_CPU_FRQ_XTAL:
		return CONFIG_XTAL_FREQ;
	case PM_CPU_FRQ_60M:
		return 60000000U;
	case PM_CPU_FRQ_80M:
		return 80000000U;
	case PM_CPU_FRQ_120M:
		return 120000000U;
	case PM_CPU_FRQ_160M:
		return 160000000U;
	case PM_CPU_FRQ_240M:
		return 120000000U;
	case PM_CPU_FRQ_320M:
		return 160000000U;
	case PM_CPU_FRQ_480M:
		return 240000000U;
	case PM_CPU_FRQ_HIGHEST:
	case PM_CPU_FRQ_DEFAULT:
	default:
		/* Fall back to the maximum documented bus rate. */
		return 240000000U;
	}
}

static uint32_t hspl_get_timeout_threshold_cycles(void)
{
	uint32_t bus_hz;
	uint64_t cycles;

	bus_hz = hspl_get_bus_clock_hz(bk_pm_current_max_cpu_freq_get());
	cycles = ((uint64_t)bus_hz * CONFIG_HSPL_TIMEOUT_MONITOR_MS) / 1000U;

	if (cycles == 0U) {
		cycles = 1U;
	} else if (cycles > HSPL_TIMEOUT_TH_MASK) {
		cycles = HSPL_TIMEOUT_TH_MASK;
	}

	return (uint32_t)cycles;
}

static void hspl_timeout_monitor_init(bk_hspl_id_t hspl_id)
{
	uintptr_t base = hspl_get_base(hspl_id);
	uint32_t threshold_cycles;
	uint32_t cfg;

	if (!base) {
		return;
	}

	threshold_cycles = hspl_get_timeout_threshold_cycles();
	s_timeout_ctx[hspl_id].sel_channel =
		(uint8_t)CONFIG_HSPL_TIMEOUT_MONITOR_CHANNEL;

	cfg = (((uint32_t)s_timeout_ctx[hspl_id].sel_channel
		<< HSPL_TIMEOUT_SEL_SHIFT) & HSPL_TIMEOUT_SEL_MASK) |
	      (threshold_cycles & HSPL_TIMEOUT_TH_MASK) |
	      HSPL_TIMEOUT_EN_BIT;

	HSPL_REG_WR32(base, HSPL_REG_TIMEOUT_CFG, cfg);
	HSPL_REG_WR32(base, HSPL_REG_TIMEOUT_CTL, HSPL_TIMEOUT_IRQ_CLR_BIT);
	HSPL_REG_WR32(base, HSPL_REG_TIMEOUT_CTL, HSPL_TIMEOUT_IRQ_EN_BIT);

	HSPL_LOGI("timeout monitor: hspl_id=%u ch=%u timeout_ms=%u cycles=%u\r\n",
	          hspl_id,
	          s_timeout_ctx[hspl_id].sel_channel,
	          CONFIG_HSPL_TIMEOUT_MONITOR_MS,
	          threshold_cycles);
}

static void hspl_dump_timeout_state(bk_hspl_id_t hspl_id)
{
	uintptr_t base = hspl_get_base(hspl_id);
	uint32_t timeout_cfg;
	uint32_t timeout_sta;
	uint32_t timeout_ctl;
	uint32_t state;
	uint8_t ch;

	if (!base) {
		return;
	}

	timeout_cfg = HSPL_REG_RD32(base, HSPL_REG_TIMEOUT_CFG);
	timeout_sta = HSPL_REG_RD32(base, HSPL_REG_TIMEOUT_STA);
	timeout_ctl = HSPL_REG_RD32(base, HSPL_REG_TIMEOUT_CTL);
	state = HSPL_REG_RD32(base, HSPL_REG_STATE);

	HSPL_LOGE("timeout irq: hspl_id=%u mon_ch=%u cfg=0x%08X sta=0x%08X ctl=0x%08X state=0x%08X\r\n",
	          hspl_id,
	          s_timeout_ctx[hspl_id].sel_channel,
	          timeout_cfg,
	          timeout_sta,
	          timeout_ctl,
	          state);

	for (ch = 0; ch < HSPL_CHANNEL_MAX; ch++) {
		hspl_state_t lock_state = {0};

		if (bk_hspl_get_state(hspl_id, ch, &lock_state) != BK_OK) {
			continue;
		}

		if (lock_state.locked) {
			HSPL_LOGE("locked ch=%u owner_valid=%u owner_id=%u\r\n",
			          ch,
			          lock_state.owner_valid,
			          lock_state.owner_id);
		}
	}
}

/*
 * Local HSPL interrupt handling:
 * - CP(M52) handles BK_HSPL_ID_0
 * - AP(M55) handles BK_HSPL_ID_1
 */
/* AP side: handle BK_HSPL_ID_1 */
#define HSPL_LOCAL_HSPL_ID  BK_HSPL_ID_1
#define HSPL_LOCAL_INT_SRC  INT_SRC_HSPL


bk_err_t bk_hspl_driver_init(void)
{
	uintptr_t base0 = hspl_get_base(BK_HSPL_ID_0);
	uintptr_t base1 = hspl_get_base(BK_HSPL_ID_1);

	if (s_hspl_driver_init) {
		return BK_OK;
	}

	/* Enable clock / deassert reset (best-effort for both instances) */
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

	hspl_timeout_monitor_init(HSPL_LOCAL_HSPL_ID);

	/* Register HSPL interrupt for local domain */
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
	hspl_dump_timeout_state(HSPL_LOCAL_HSPL_ID);

	if (s_timeout_ctx[HSPL_LOCAL_HSPL_ID].cb) {
		s_timeout_ctx[HSPL_LOCAL_HSPL_ID].cb(s_timeout_ctx[HSPL_LOCAL_HSPL_ID].sel_channel,
		                                     s_timeout_ctx[HSPL_LOCAL_HSPL_ID].cb_param);
	}

	BK_ASSERT(0);
}

