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

#include "hpdma_hal.h"
#include "hpdma_ll.h"


bk_err_t hpdma_hal_init(hpdma_hal_t *hal)
{
    hal->hw = (hpdma_hw_t *)HPDMA_LL_REG_BASE(hal->id);
    hpdma_ll_init(hal->hw);
    hpdma_ll_set_prio_mode_round_robin(hal->hw);
    return BK_OK;
}

void hpdma_hal_init_without_channels(hpdma_hal_t *hal)
{
	hal->hw = (hpdma_hw_t *)HPDMA_LL_REG_BASE(hal->id);
	//soft reset for all CPUs
	hpdma_ll_init_without_channels(hal->hw);
}

bk_err_t hpdma_hal_init_dma(hpdma_hal_t *hal, hpdma_id_t id, const hpdma_config_t *config)
{
    hpdma_ll_set_work_mode(hal->hw, id, config->mode);
    hpdma_ll_set_chan_prio(hal->hw, id, config->chan_prio);

    hpdma_ll_set_dest_data_width(hal->hw, id, config->dst.width);
    hpdma_ll_set_src_data_width(hal->hw, id, config->src.width);

    hpdma_ll_set_dest_req_mux(hal->hw, id, config->dst.dev);
    hpdma_ll_set_src_req_mux(hal->hw, id, config->src.dev);
	hpdma_ll_set_pixel_trans_type(hal->hw, id, config->trans_type);

    hpdma_ll_set_src_start_addr(hal->hw, id, config->src.start_addr);
    hpdma_ll_set_dest_start_addr(hal->hw, id, config->dst.start_addr);

    if (config->src.addr_inc_en) {
        hpdma_ll_enable_src_addr_inc(hal->hw, id);
    }

    if (config->src.addr_loop_en) {
        // When src addr loop is enabled, ysize must be 0
        if (config->src.ysize != 0) {
            return BK_ERR_HPDMA_HAL_INVALID_YSIZE;
        }
        // Calculate loop end address = start address + xsize
        uint32_t src_loop_end_addr = config->src.start_addr + config->src.xsize;
        // Check if the end address is 128-bit aligned
        if ((src_loop_end_addr & 0xF) != 0) {
            return BK_ERR_HPDMA_HAL_INVALID_ALIGN;
        }
        hpdma_ll_set_src_loop_end_addr(hal->hw, id, src_loop_end_addr);
        hpdma_ll_enable_src_addr_loop(hal->hw, id);
    }

    if (config->dst.addr_inc_en) {
        hpdma_ll_enable_dest_addr_inc(hal->hw, id);
    }

    if (config->dst.addr_loop_en) {
        // When dst addr loop is enabled, ysize must be 0
        if (config->dst.ysize != 0) {
            return BK_ERR_HPDMA_HAL_INVALID_YSIZE;
        }
        // Calculate loop end address = start address + xsize
        uint32_t dst_loop_end_addr = config->dst.start_addr + config->dst.xsize;
        // Check if the end address is 128-bit aligned
        if ((dst_loop_end_addr & 0xF) != 0) {
            return BK_ERR_HPDMA_HAL_INVALID_ALIGN;
        }
        hpdma_ll_set_dest_loop_end_addr(hal->hw, id, dst_loop_end_addr);
        hpdma_ll_enable_dest_addr_loop(hal->hw, id);
    }
    // Configure X-direction and Y-direction sizes (Reg20 and Reg22)
    hpdma_ll_set_xsize(hal->hw, id, config->src.xsize, config->dst.xsize);
    hpdma_ll_set_ysize(hal->hw, id, config->src.ysize, config->dst.ysize);

    // Configure source and destination step (Reg29)
    hpdma_ll_set_step(hal->hw, id, config->src.step, config->dst.step);


    return BK_OK;
}

bk_err_t hpdma_hal_start_common(hpdma_hal_t *hal, hpdma_id_t id)
{
    hpdma_ll_enable(hal->hw, id);
    return BK_OK;
}

bk_err_t hpdma_hal_stop_common(hpdma_hal_t *hal, hpdma_id_t id)
{
    hpdma_ll_clear_interrupt_status(hal->hw, id);
    hpdma_ll_disable(hal->hw, id);
    return BK_OK;
}

