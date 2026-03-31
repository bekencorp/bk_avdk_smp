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

	hw->prio_mode.v = 0;
	hw->prio_mode.soft_reset = 1;	//reset it before anyother operations

	hw->secure_attr.v = 0xF;
	hw->privileged_attr.v = 0xF;


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
		hw->config_group[hpdma_id].status.v = 0;
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
	if(0 == hpdma_ll_get_soft_reset_value(hw)) {
		hw->prio_mode.v = 0;
		hw->prio_mode.soft_reset = 1;	//reset it before anyother operations
		hw->secure_attr.v = 0xF;  // attr is 4-bit, so use 0xF instead of 0xFFF
		hw->privileged_attr.v = 0xF;  // attr is 4-bit, so use 0xF instead of 0xFFF
	}
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

static inline void hpdma_ll_enable_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hpdma_ll_enable_half_finish_interrupt(hw, id);
	hpdma_ll_enable_finish_interrupt(hw, id);
	hpdma_ll_enable_bus_err_interrupt(hw,id);
}

static inline void hpdma_ll_disable_interrupt(hpdma_hw_t *hw, hpdma_id_t id)
{
	hpdma_ll_disable_half_finish_interrupt(hw, id);
	hpdma_ll_disable_finish_interrupt(hw, id);
	hpdma_ll_disable_bus_err_interrupt(hw,id);
}

static inline void hpdma_ll_clear_finish_interrupt_status(hpdma_hw_t *hw, hpdma_id_t id)
{
	/*other interrupt bit also wirte 1 to clear, so should not effect other bits*/
	hw->config_group[id].status.v |= BIT(HPDMA_FINISH_INT_POS);
}

static inline void hpdma_ll_clear_half_finish_interrupt_status(hpdma_hw_t *hw, hpdma_id_t id)
{
	/*other interrupt bit also wirte 1 to clear, so should not effect other bits*/
	hw->config_group[id].status.v |= BIT(HPDMA_HALF_FINISH_INT_POS);
}

static inline void hpdma_ll_clear_bus_err_interrupt_status(hpdma_hw_t *hw, hpdma_id_t id)
{
	/*other interrupt bit also wirte 1 to clear, so should not effect other bits*/
	hw->config_group[id].status.v |= BIT(HPDMA_BUS_ERR_INT_POS);
}


static inline void hpdma_ll_clear_interrupt_status(hpdma_hw_t *hw, hpdma_id_t id)
{
	hpdma_ll_clear_half_finish_interrupt_status(hw, id);
	hpdma_ll_clear_finish_interrupt_status(hw, id);
	hpdma_ll_clear_bus_err_interrupt_status(hw, id);

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
	uint32_t mask = 0x7UL << (id * 3);  // 3-bit mask for this channel
	uint32_t value = ((uint32_t)int_id & 0x7UL) << (id * 3);
	
	// Clear the 3 bits for this channel and set new value
	hw->int_allocate.v = (hw->int_allocate.v & ~mask) | value;
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

