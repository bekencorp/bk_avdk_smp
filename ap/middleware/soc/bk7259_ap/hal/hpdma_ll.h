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

#include <soc/soc.h>
#include "hal_port.h"
#include "hpdma_hw.h"
#include <driver/hal/hal_hpdma_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HPDMA_LL_REG_BASE(_hpdma_unit_id) (SOC_HPDMA_REG_BASE)
#define CASE_DEV(dev) case HPDMA_DEV_##dev: return HPDMA_V_REQ_MUX_##dev
#define CASE_D() default: return 0

static inline void hpdma_ll_init(hpdma_hw_t *hw)
{
	int hpdma_id;

	/*
	 * CONFIG_SPE=0 (NS after TF-M/PPHS): TF-M already programmed
	 * controller-wide regs; NS writes BusFault — skip.
	 */
#if CONFIG_SPE
	hw->prio_mode.v = 0;
	hw->prio_mode.soft_reset = 1;	//reset it before anyother operations
	hw->secure_attr.v = 0xF;
	hw->privileged_attr.v = 0xF;
#endif

	for (hpdma_id = 0; hpdma_id < SOC_HPDMA_CHAN_NUM_PER_UNIT; hpdma_id++) {
		hw->config_group[hpdma_id].ctrl.v = 0;
		hw->config_group[hpdma_id].dest_start_addr = 0;
		hw->config_group[hpdma_id].src_start_addr = 0;
		hw->config_group[hpdma_id].dest_loop_end_addr = 0;
		hw->config_group[hpdma_id].xsize_reg.v = 0;
		hw->config_group[hpdma_id].src_loop_end_addr = 0;
		hw->config_group[hpdma_id].ysize_reg.v = 0;
		hw->config_group[hpdma_id].req_mux.v = 0;
		hw->config_group[hpdma_id].src_pause_addr = 0;
		hw->config_group[hpdma_id].dest_pause_addr = 0;
		/* W1C: writing 0 never clears latched interrupt bits */
		hw->config_group[hpdma_id].status.v = BIT(HPDMA_HALF_FINISH_INT_POS)
		                                    | BIT(HPDMA_FINISH_INT_POS)
		                                    | BIT(HPDMA_BUS_ERR_INT_POS)
		                                    | BIT(HPDMA_FIFO_ERR_INT_POS);
		hw->config_group[hpdma_id].step_reg.v = 0;
		hw->config_group[hpdma_id].remain_length.v = 0;
		hw->config_group[hpdma_id].next_ll_addr = 0;
	}
}

static inline uint32_t hpdma_ll_get_soft_reset_value(hpdma_hw_t *hw)
{
	return hw->prio_mode.soft_reset;
}

static inline void hpdma_ll_init_without_channels(hpdma_hw_t *hw)
{
#if CONFIG_SPE
	if(0 == hpdma_ll_get_soft_reset_value(hw)) {
		hw->prio_mode.v = 0;
		hw->prio_mode.soft_reset = 1;	//reset it before anyother operations
		hw->secure_attr.v = 0xF;  // attr is 4-bit, so use 0xF instead of 0xFFF
		hw->privileged_attr.v = 0xF;  // attr is 4-bit, so use 0xF instead of 0xFFF
	}
#else
	(void)hw; /* NS: TF-M owns soft_reset / attr (see hpdma_ll_init). */
#endif
}

//TODO: add other devices
static inline uint32_t hpdma_ll_dev_to_req_mux(uint32 req_mux)
{
	switch (req_mux) {
		CASE_DEV(DTCM);
		CASE_D();
	}
}

static inline void hpdma_ll_enable(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].ctrl.enable = 1;
}

static inline void hpdma_ll_disable(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].ctrl.enable = 0;
}

static inline uint32_t hpdma_ll_get_enable_status(hpdma_hw_t *hw, hpdma_id_t id)
{
	return hw->config_group[id].ctrl.enable;
}

static inline bool hpdma_ll_is_id_started(hpdma_hw_t *hw, hpdma_id_t id)
{
	return !!(hw->config_group[id].ctrl.enable == 1);
}

static inline void hpdma_ll_enable_finish_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].req_mux.finish_int_en = 1;
}

static inline void hpdma_ll_disable_finish_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].req_mux.finish_int_en = 0;
}

static inline void hpdma_ll_enable_half_finish_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].req_mux.half_finish_int_en = 1;
}

static inline void hpdma_ll_disable_half_finish_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].req_mux.half_finish_int_en = 0;
}

static inline void hpdma_ll_enable_bus_err_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].req_mux.bus_err_int_en = 1;
}

static inline void hpdma_ll_disable_bus_err_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].req_mux.bus_err_int_en = 0;
}

/*
 * P0 (HPDMA review): fifo_err is the 4th hardware error class on Reg23/Reg28.
 *   Previously there was no enable / disable / clear / triggered helper, so
 *   the driver could neither route fifo_err to a callback nor reliably W1C
 *   clear bit17 in status. The pair below mirrors bus_err.
 */
static inline void hpdma_ll_enable_fifo_err_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].req_mux.fifo_err_int_en = 1;
}

static inline void hpdma_ll_disable_fifo_err_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].req_mux.fifo_err_int_en = 0;
}

static inline void hpdma_ll_enable_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hpdma_ll_enable_half_finish_interrupt(hw, id);
	hpdma_ll_enable_finish_interrupt(hw, id);
	hpdma_ll_enable_bus_err_interrupt(hw,id);
	hpdma_ll_enable_fifo_err_interrupt(hw, id);
}

static inline void hpdma_ll_disable_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hpdma_ll_disable_half_finish_interrupt(hw, id);
	hpdma_ll_disable_finish_interrupt(hw, id);
	hpdma_ll_disable_bus_err_interrupt(hw,id);
	hpdma_ll_disable_fifo_err_interrupt(hw, id);
}

/*
 * S0 (HPDMA SMP review):
 *   The status register (REG_0x1C) holds four W1C event bits in the same
 *   word - fifo_err(17), half_finish(18), finish(19), bus_err(20). The
 *   previous "|= BIT(x)" implementation was a read-modify-write: it read
 *   the live status, ORed the requested bit, then wrote the result back.
 *   Because every other W1C bit that happened to be set at read time was
 *   also written back as 1, clearing one event silently cleared *all*
 *   pending events of the channel. e.g. clearing finish from the ISR while
 *   half_finish was also pending would lose the half_finish edge.
 *
 *   The standard W1C idiom is a direct assignment of just the bit being
 *   cleared. Writing 0 to the other W1C bits is a no-op (W1C only acts on
 *   write 1); writing 0 to the surrounding RO fields (desc_num,
 *   counters, repeat_*_pause) is ignored by the hardware. This also
 *   removes the RMW window that two cores (task + ISR) could both be in
 *   simultaneously and that was the root cause of lost interrupts under
 *   SMP load.
 */
static inline void hpdma_ll_clear_finish_interrupt_status(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].status.v = BIT(HPDMA_FINISH_INT_POS);
}

static inline void hpdma_ll_clear_half_finish_interrupt_status(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].status.v = BIT(HPDMA_HALF_FINISH_INT_POS);
}

static inline void hpdma_ll_clear_bus_err_interrupt_status(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].status.v = BIT(HPDMA_BUS_ERR_INT_POS);
}

/*
 * P0 (HPDMA review): added fifo_err W1C and a helper to clear *all* W1C
 *   status bits in one shot. The previous implementation skipped bit17
 *   (fifo_err) entirely, so once fifo_err was triggered the status bit
 *   stayed set forever.
 */
static inline void hpdma_ll_clear_fifo_err_interrupt_status(hpdma_hw_t *hw, hpdma_id_t id)
{
	/* See note above clear_finish: direct W1C write, no RMW. */
	hw->config_group[id].status.v = BIT(HPDMA_FIFO_ERR_INT_POS);
}

static inline bool hpdma_ll_is_fifo_err_interrupt_triggered(hpdma_hw_t *hw, hpdma_id_t id)
{
	return !!(hw->config_group[id].status.fifo_err_int & 0x1);
}

static inline void hpdma_ll_clear_interrupt_status(hpdma_hw_t *hw, hpdma_id_t id)
{
	/*
	 * Single write: assert 1 on every W1C event bit at once. Writing 0
	 * to other W1C bits is a no-op, so this is functionally equivalent
	 * to four sequential clears but avoids three extra register cycles
	 * and three extra RMW windows under SMP.
	 */
	hw->config_group[id].status.v = BIT(HPDMA_HALF_FINISH_INT_POS)
	                              | BIT(HPDMA_FINISH_INT_POS)
	                              | BIT(HPDMA_BUS_ERR_INT_POS)
	                              | BIT(HPDMA_FIFO_ERR_INT_POS);
}

static inline uint32_t hpdma_ll_repeat_wr_pause(hpdma_hw_t *hw, hpdma_id_t id)
{
	return (uint32_t)(hw->config_group[id].status.repeat_wr_pause);
}

static inline uint32_t hpdma_ll_repeat_rd_pause(hpdma_hw_t *hw, hpdma_id_t id)
{
	return (uint32_t)(hw->config_group[id].status.repeat_rd_pause);
}

static inline uint32_t hpdma_ll_finish_interrupt_cnt(hpdma_hw_t *hw, hpdma_id_t id)
{
	return (uint32_t)(hw->config_group[id].status.finish_int_counter);
}

static inline uint32_t hpdma_ll_half_finish_interrupt_cnt(hpdma_hw_t *hw, hpdma_id_t id)
{
	return (uint32_t)(hw->config_group[id].status.half_finish_int_counter);
}

static inline bool hpdma_ll_is_finish_interrupt_triggered(hpdma_hw_t *hw, hpdma_id_t id)
{
	return !!(hw->config_group[id].status.finish_int & 0x1);
}

static inline bool hpdma_ll_is_half_finish_interrupt_triggered(hpdma_hw_t *hw, hpdma_id_t id)
{
	return !!(hw->config_group[id].status.half_finish_int & 0x1);
}

static inline bool hpdma_ll_is_bus_err_interrupt_triggered(hpdma_hw_t *hw, hpdma_id_t id)
{
	return !!(hw->config_group[id].status.bus_err_int & 0x1);
}

static inline void hpdma_ll_reset_config_to_default(hpdma_hw_t *hw, volatile hpdma_id_t id)
{
	hw->config_group[id].ctrl.v = 0;
	hw->config_group[id].dest_start_addr = 0;
	hw->config_group[id].src_start_addr = 0;
	hw->config_group[id].dest_loop_end_addr = 0;
	hw->config_group[id].xsize_reg.v = 0;
	hw->config_group[id].src_loop_end_addr = 0;
	hw->config_group[id].ysize_reg.v = 0;
	hw->config_group[id].req_mux.v = 0;
	hw->config_group[id].src_pause_addr = 0;
	hw->config_group[id].dest_pause_addr = 0;
	hw->config_group[id].status.v = 0;
	hw->config_group[id].step_reg.v = 0;
	hw->config_group[id].remain_length.v = 0;
	hw->config_group[id].next_ll_addr = 0;
}

static inline void hpdma_ll_set_work_mode(hpdma_hw_t *hw, hpdma_id_t id, uint32_t mode)
{
	hw->config_group[id].ctrl.mode = mode & 0x01;
}

#define hpdma_ll_set_mode_single(hw, id) hpdma_ll_set_work_mode(hw, id, HPDMA_V_WORK_MODE_SINGLE)
#define hpdma_ll_set_mode_repeat(hw, id) hpdma_ll_set_work_mode(hw, id, HPDMA_V_WORK_MODE_REPEAT)

static uint32_t hpdma_ll_get_work_mode(hpdma_hw_t *hw, hpdma_id_t id)
{
    return hw->config_group[id].ctrl.mode;
}

static inline void hpdma_ll_set_chan_prio(hpdma_hw_t *hw, hpdma_id_t id, uint32_t chan_prio)
{
	hw->config_group[id].ctrl.chan_prio = chan_prio & 0x7;  // chan_prio is 3-bit
}

static inline void hpdma_ll_set_cfg_cache(hpdma_hw_t *hw, hpdma_id_t id, uint32_t cfg_cache)
{
	hw->config_group[id].ctrl.cfg_cache = cfg_cache & 0xF;
}

static inline uint32_t hpdma_ll_get_cfg_cache(hpdma_hw_t *hw, hpdma_id_t id)
{
	return hw->config_group[id].ctrl.cfg_cache;
}

static inline void hpdma_ll_set_dest_data_width(hpdma_hw_t *hw, hpdma_id_t id, uint32_t data_width)
{
	hw->config_group[id].ctrl.dest_data_width = data_width & 0x7;  // dest_data_width is 3-bit
}

static inline void hpdma_ll_set_src_data_width(hpdma_hw_t *hw, hpdma_id_t id, uint32_t data_width)
{
	hw->config_group[id].ctrl.src_data_width = data_width & 0x7;  // src_data_width is 3-bit
}

static inline void hpdma_ll_set_dest_req_mux(hpdma_hw_t *hw, hpdma_id_t id, uint32_t req_mux)
{
	hw->config_group[id].req_mux.dest_req_mux = (hpdma_ll_dev_to_req_mux(req_mux));
}

static inline void hpdma_ll_set_src_req_mux(hpdma_hw_t *hw, hpdma_id_t id, uint32_t req_mux)
{
	hw->config_group[id].req_mux.src_req_mux = (hpdma_ll_dev_to_req_mux(req_mux));
}

static inline void hpdma_ll_enable_src_addr_inc(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].ctrl.src_addr_inc_en = 1;
}

static inline void hpdma_ll_disable_src_addr_inc(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].ctrl.src_addr_inc_en = 0;
}

static inline void hpdma_ll_enable_dest_addr_inc(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].ctrl.dest_addr_inc_en = 1;
}

static inline void hpdma_ll_disable_dest_addr_inc(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].ctrl.dest_addr_inc_en = 0;
}

static inline void hpdma_ll_enable_src_addr_loop(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].ctrl.src_addr_loop_en = 1;
}

static inline void hpdma_ll_disable_src_addr_loop(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].ctrl.src_addr_loop_en = 0;
}

static inline void hpdma_ll_enable_dest_addr_loop(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].ctrl.dest_addr_loop_en = 1;
}

static inline void hpdma_ll_disable_dest_addr_loop(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].ctrl.dest_addr_loop_en = 0;
}

/*
 * SRAM address-alias warning (0x2C/0x28):
 *   The *_set_*_addr / set_next_ll_addr helpers below write the given value
 *   straight into the hardware register; this layer performs NO 0x2C->0x28
 *   conversion. The SRAM cacheable alias 0x2Cxxxxxx must first be mapped to the
 *   HPDMA-visible peripheral alias 0x28xxxxxx, otherwise the engine cannot
 *   reach the data.
 *
 *   Convention: these LL helpers should only be reached through the same-named
 *   hpdma_hal_set_*_addr macros in hpdma_hal.h, which already wrap
 *   SOC_SRAM_PERI_ADDR. If new code must call this layer directly, apply
 *   SOC_SRAM_PERI_ADDR() to the SRAM address yourself before passing it in;
 *   never write a raw 0x2C address into the register.
 */
static inline void hpdma_ll_set_dest_start_addr(hpdma_hw_t *hw, volatile hpdma_id_t id, uint32_t addr)
{
	hw->config_group[id].dest_start_addr = addr;
}

static inline uint32_t hpdma_ll_get_dest_start_addr(hpdma_hw_t *hw, hpdma_id_t id)
{
	return hw->config_group[id].dest_start_addr;
}

static inline void hpdma_ll_set_src_start_addr(hpdma_hw_t *hw, hpdma_id_t id, uint32_t addr)
{
	hw->config_group[id].src_start_addr = addr;
}

static inline uint32_t hpdma_ll_get_src_start_addr(hpdma_hw_t *hw, hpdma_id_t id)
{
	return hw->config_group[id].src_start_addr;
}

static inline void hpdma_ll_set_src_loop_end_addr(hpdma_hw_t *hw, hpdma_id_t id, uint32_t end_addr)
{
	hw->config_group[id].src_loop_end_addr = end_addr;
}

static inline void hpdma_ll_set_dest_loop_end_addr(hpdma_hw_t *hw, hpdma_id_t id, uint32_t end_addr)
{
	hw->config_group[id].dest_loop_end_addr = end_addr;
}

static inline uint32_t hpdma_ll_get_remain_len(hpdma_hw_t *hw, hpdma_id_t id)
{
	return (uint32_t)hw->config_group[id].remain_length.remain_len;
}

static inline void hpdma_ll_set_prio_mode(hpdma_hw_t *hw, uint32_t prio_mode)
{
	hw->prio_mode.prio_mode = prio_mode & 0x1;
}

#define hpdma_ll_set_prio_mode_round_robin(hw) hpdma_ll_set_prio_mode(hw, HPDMA_V_PRIO_MODE_ROUND_ROBIN)
#define hpdma_ll_set_prio_mode_fixed_prio(hw) hpdma_ll_set_prio_mode(hw, HPDMA_V_PRIO_MODE_FIXED_PRIO)

static inline void hpdma_ll_set_secure_attr(hpdma_hw_t *hw, hpdma_id_t id, hpdma_sec_attr_t attr)
{
	if (attr == HPDMA_ATTR_NON_SEC) {
		hw->secure_attr.attr &= ~(BIT(id) & 0xF);  // attr is 4-bit, so use 0xF instead of 0xFF
	} else if (attr == HPDMA_ATTR_SEC) {
		hw->secure_attr.attr |= (BIT(id) & 0xF);  // attr is 4-bit, so use 0xF instead of 0xFF
	}
}

static inline void hpdma_ll_set_privileged_attr(hpdma_hw_t *hw, hpdma_id_t id, hpdma_sec_attr_t attr)
{
	if (attr == HPDMA_ATTR_NON_SEC) {
		hw->privileged_attr.attr &= ~(BIT(id) & 0xF);  // attr is 4-bit, so use 0xF instead of 0xFF
	} else if (attr == HPDMA_ATTR_SEC) {
		hw->privileged_attr.attr |= (BIT(id) & 0xF);  // attr is 4-bit, so use 0xF instead of 0xFF
	}
}

/**
 * @brief Set interrupt allocation for a specific DMA channel
 * @param hw Pointer to HPDMA hardware registers
 * @param id DMA channel ID (0-3, only 4 channels supported)
 * @param int_id Interrupt ID (0-4, maps to int0-int4)
 * 
 * This function allocates the interrupt from channel 'id' to interrupt 'int_id'.
 * Each channel occupies 3 bits in the register:
 *   - channel0: bits [2:0]
 *   - channel1: bits [5:3]
 *   - channel2: bits [8:6]
 *   - channel3: bits [11:9]
 * 
 * Note: int_allocate.status is 12-bit, only supports channels 0-3.
 *       Configuration must be done in secure world.
 */
static inline void hpdma_ll_set_int_allocate(hpdma_hw_t *hw, hpdma_id_t id, hpdma_int_id_t int_id)
{
#if CONFIG_SPE
	uint32_t mask = 0x7UL << (id * 3);  // 3-bit mask for this channel
	uint32_t value = ((uint32_t)int_id & 0x7UL) << (id * 3);

	// Clear the 3 bits for this channel and set new value
	hw->int_allocate.v = (hw->int_allocate.v & ~mask) | value;
#else
	(void)hw;
	(void)id;
	(void)int_id;
#endif
}

/**
 * @brief Get interrupt allocation for a specific DMA channel
 * @param hw Pointer to HPDMA hardware registers
 * @param id DMA channel ID (0-3, only 4 channels supported)
 * @return Interrupt ID (0-4, maps to int0-int4)
 */
static inline hpdma_int_id_t hpdma_ll_get_int_allocate(hpdma_hw_t *hw, hpdma_id_t id)
{
	uint32_t value = (hw->int_allocate.v >> (id * 3)) & 0x7UL;
	return (hpdma_int_id_t)value;
}

static inline void hpdma_ll_set_src_pause_addr(hpdma_hw_t *hw, hpdma_id_t id, uint32_t addr)
{
	hw->config_group[id].src_pause_addr = addr;
}

static inline void hpdma_ll_set_dest_pause_addr(hpdma_hw_t *hw, hpdma_id_t id, uint32_t addr)
{
	hw->config_group[id].dest_pause_addr = addr;
}

static inline uint32_t hpdma_ll_get_src_read_addr(hpdma_hw_t *hw, hpdma_id_t id)
{
	return hw->config_group[id].src_rd_addr;
}

static inline uint32_t hpdma_ll_get_dest_write_addr(hpdma_hw_t *hw, hpdma_id_t id)
{
	return hw->config_group[id].dest_wr_addr;
}

static inline void hpdma_ll_set_dest_burst_len(hpdma_hw_t *hw, hpdma_id_t id, uint32_t len)
{
	hw->config_group[id].req_mux.dtst_burst_len = len;
}

static inline uint32_t hpdma_ll_get_dest_burst_len(hpdma_hw_t *hw, hpdma_id_t id)
{
	return (hw->config_group[id].req_mux.dtst_burst_len);
}

static inline void hpdma_ll_set_pixel_trans_type(hpdma_hw_t *hw, hpdma_id_t id, uint32_t type)
{
	hw->config_group[id].req_mux.pixel_trans_type = type;
}

static inline uint32_t hpdma_ll_get_pixel_trans_type(hpdma_hw_t *hw, hpdma_id_t id)
{
	return (hw->config_group[id].req_mux.pixel_trans_type);
}

static inline void hpdma_ll_set_src_burst_len(hpdma_hw_t *hw, hpdma_id_t id, uint32_t len)
{
	hw->config_group[id].req_mux.src_burst_len = len;
}

static inline uint32_t hpdma_ll_get_src_burst_len(hpdma_hw_t *hw, hpdma_id_t id)
{
	return (hw->config_group[id].req_mux.src_burst_len);
}

static inline void hpdma_ll_bus_err_int_enable(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].req_mux.bus_err_int_en = 0x1;
}

static inline void hpdma_ll_bus_err_int_disable(hpdma_hw_t *hw, hpdma_id_t id)
{
	hw->config_group[id].req_mux.bus_err_int_en = 0x0;
}

static inline void hpdma_ll_set_dest_sec_attr(hpdma_hw_t *hw, hpdma_id_t id, uint32_t attr)
{
	hw->config_group[id].req_mux.dest_sec_attr = attr ;
}

static inline void hpdma_ll_set_src_sec_attr(hpdma_hw_t *hw, hpdma_id_t id, uint32_t attr)
{
	hw->config_group[id].req_mux.src_sec_attr = attr ;
}

static inline void hpdma_ll_set_next_ll_addr(hpdma_hw_t *hw, hpdma_id_t id, uint32_t ll_addr)
{
	hw->config_group[id].next_ll_addr = ll_addr;
}

static inline uint32_t hpdma_ll_get_next_ll_addr(hpdma_hw_t *hw, hpdma_id_t id)
{
	return hw->config_group[id].next_ll_addr;
}

static inline void hpdma_ll_set_xsize(hpdma_hw_t *hw, hpdma_id_t id, uint32_t src_xsize, uint32_t dest_xsize)
{
	hw->config_group[id].xsize_reg.xsize.src_xsize = src_xsize & 0xFFFF;
	hw->config_group[id].xsize_reg.xsize.dest_xsize = dest_xsize & 0xFFFF;
}

static inline void hpdma_ll_set_ysize(hpdma_hw_t *hw, hpdma_id_t id, uint32_t src_ysize, uint32_t dest_ysize)
{
	hw->config_group[id].ysize_reg.ysize.src_ysize = src_ysize & 0xFFFF;
	hw->config_group[id].ysize_reg.ysize.dest_ysize = dest_ysize & 0xFFFF;
}

static inline void hpdma_ll_get_xsize(hpdma_hw_t *hw, hpdma_id_t id, uint32_t *src_xsize, uint32_t *dest_xsize)
{
	if (src_xsize) {
		*src_xsize = hw->config_group[id].xsize_reg.xsize.src_xsize;
	}
	if (dest_xsize) {
		*dest_xsize = hw->config_group[id].xsize_reg.xsize.dest_xsize;
	}
}

static inline void hpdma_ll_get_ysize(hpdma_hw_t *hw, hpdma_id_t id, uint32_t *src_ysize, uint32_t *dest_ysize)
{
	if (src_ysize) {
		*src_ysize = hw->config_group[id].ysize_reg.ysize.src_ysize;
	}
	if (dest_ysize) {
		*dest_ysize = hw->config_group[id].ysize_reg.ysize.dest_ysize;
	}
}

static inline void hpdma_ll_set_step(hpdma_hw_t *hw, hpdma_id_t id, uint32_t src_step, uint32_t dest_step)
{
	hw->config_group[id].step_reg.step.src_step = src_step & 0x7FFF;
	hw->config_group[id].step_reg.step.dest_step = dest_step & 0x7FFF;
}

static inline void hpdma_ll_get_step(hpdma_hw_t *hw, hpdma_id_t id, uint32_t *src_step, uint32_t *dest_step)
{
	if (src_step) {
		*src_step = hw->config_group[id].step_reg.step.src_step;
	}
	if (dest_step) {
		*dest_step = hw->config_group[id].step_reg.step.dest_step;
	}
}

#ifdef __cplusplus
}
#endif

