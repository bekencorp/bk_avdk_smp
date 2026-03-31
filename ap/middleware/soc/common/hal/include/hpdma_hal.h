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

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_config.h"
#include "hpdma_hw.h"
#include "hpdma_ll.h"
#include <driver/hal/hal_hpdma_types.h>

typedef struct {
    hpdma_hw_t *hw;
    hpdma_unit_t id;
} hpdma_hal_t;

#define hpdma_hal_get_soft_reset_value(hal) hpdma_ll_get_soft_reset_value((hal)->hw)
#define hpdma_hal_get_work_mode(hal, id) hpdma_ll_get_work_mode((hal)->hw, id)
#define hpdma_hal_repeat_wr_pause(hal, id) hpdma_ll_repeat_wr_pause((hal)->hw, (id))
#define hpdma_hal_repeat_rd_pause(hal, id) hpdma_ll_repeat_rd_pause((hal)->hw, (id))
#define hpdma_hal_finish_interrupt_cnt(hal, id) hpdma_ll_finish_interrupt_cnt((hal)->hw, (id))
#define hpdma_hal_half_finish_interrupt_cnt(hal, id) hpdma_ll_half_finish_interrupt_cnt((hal)->hw, (id))
#define hpdma_hal_clear_half_finish_interrupt_status(hal, id) hpdma_ll_clear_half_finish_interrupt_status((hal)->hw, (id))
#define hpdma_hal_clear_finish_interrupt_status(hal, id) hpdma_ll_clear_finish_interrupt_status((hal)->hw, (id))
#define hpdma_hal_is_half_finish_interrupt_triggered(hal, id) hpdma_ll_is_half_finish_interrupt_triggered((hal)->hw, id)
#define hpdma_hal_is_finish_interrupt_triggered(hal, id) hpdma_ll_is_finish_interrupt_triggered((hal)->hw, id)
#define hpdma_hal_enable_finish_interrupt(hal, id) hpdma_ll_enable_finish_interrupt((hal)->hw, id)
#define hpdma_hal_disable_finish_interrupt(hal, id) hpdma_ll_disable_finish_interrupt((hal)->hw, id)
#define hpdma_hal_enable_half_finish_interrupt(hal, id) hpdma_ll_enable_half_finish_interrupt((hal)->hw, id)
#define hpdma_hal_disable_half_finish_interrupt(hal, id) hpdma_ll_disable_half_finish_interrupt((hal)->hw, id)
#define hpdma_hal_clear_bus_err_interrupt_status(hal, id) hpdma_ll_clear_bus_err_interrupt_status((hal)->hw, (id))
#define hpdma_hal_is_bus_err_interrupt_triggered(hal, id) hpdma_ll_is_bus_err_interrupt_triggered((hal)->hw, id)
#define hpdma_hal_enable_bus_err_interrupt(hal, id) hpdma_ll_enable_bus_err_interrupt((hal)->hw, id)
#define hpdma_hal_disable_bus_err_interrupt(hal, id) hpdma_ll_disable_bus_err_interrupt((hal)->hw, id)

#define hpdma_hal_reset_config_to_default(hal, id) hpdma_ll_reset_config_to_default((hal)->hw, (id))
#define hpdma_hal_is_id_started(hal, id) hpdma_ll_is_id_started((hal)->hw, (id))
#define hpdma_hal_get_transfer_len_max(hal) hpdma_ll_get_transfer_len_max((hal)->hw)
#define hpdma_hal_get_remain_len(hal, id) hpdma_ll_get_remain_len((hal)->hw, (id))
#define hpdma_hal_set_prio_mode(hal, prio_mode) hpdma_ll_set_prio_mode((hal)->hw, prio_mode)
#define hpdma_hal_get_enable_status(hal, id) hpdma_ll_get_enable_status((hal)->hw, id)

#define hpdma_hal_set_cfg_cache(hal, id, cfg_cache) hpdma_ll_set_cfg_cache((hal)->hw, id, cfg_cache)
#define hpdma_hal_get_cfg_cache(hal, id) hpdma_ll_get_cfg_cache((hal)->hw, id)

#define hpdma_hal_set_src_start_addr(hal, id, addr) hpdma_ll_set_src_start_addr((hal)->hw, id, addr)
#define hpdma_hal_get_src_start_addr(hal, id) hpdma_ll_get_src_start_addr((hal)->hw, id)
#define hpdma_hal_set_dest_start_addr(hal, id, addr) hpdma_ll_set_dest_start_addr((hal)->hw, id, addr)
#define hpdma_hal_get_dest_start_addr(hal, id) hpdma_ll_get_dest_start_addr((hal)->hw, id)
#define hpdma_hal_set_src_loop_end_addr(hal, id, end_addr) hpdma_ll_set_src_loop_end_addr((hal)->hw, id, end_addr)
#define hpdma_hal_set_dest_loop_end_addr(hal, id, end_addr) hpdma_ll_set_dest_loop_end_addr((hal)->hw, id, end_addr)

#define hpdma_hal_enable_src_addr_inc(hal, id) hpdma_ll_enable_src_addr_inc((hal)->hw, id)
#define hpdma_hal_disable_src_addr_inc(hal, id) hpdma_ll_disable_src_addr_inc((hal)->hw, id)
#define hpdma_hal_enable_dest_addr_inc(hal, id) hpdma_ll_enable_dest_addr_inc((hal)->hw, id)
#define hpdma_hal_disable_dest_addr_inc(hal, id) hpdma_ll_disable_dest_addr_inc((hal)->hw, id)
#define hpdma_hal_enable_src_addr_loop(hal, id) hpdma_ll_enable_src_addr_loop((hal)->hw, id)
#define hpdma_hal_disable_src_addr_loop(hal, id) hpdma_ll_disable_src_addr_loop((hal)->hw, id)
#define hpdma_hal_enable_dest_addr_loop(hal, id) hpdma_ll_enable_dest_addr_loop((hal)->hw, id)
#define hpdma_hal_disable_dest_addr_loop(hal, id) hpdma_ll_disable_dest_addr_loop((hal)->hw, id)

#define hpdma_hal_set_src_pause_addr(hal, id, addr) hpdma_ll_set_src_pause_addr((hal)->hw, id, addr)
#define hpdma_hal_set_dest_pause_addr(hal, id, addr) hpdma_ll_set_dest_pause_addr((hal)->hw, id, addr)
#define hpdma_hal_get_src_read_addr(hal, id) hpdma_ll_get_src_read_addr((hal)->hw, id)
#define hpdma_hal_get_dest_write_addr(hal, id) hpdma_ll_get_dest_write_addr((hal)->hw, id)

#define hpdma_hal_set_src_data_width(hal, id, data_width) hpdma_ll_set_src_data_width((hal)->hw, id, data_width)
#define hpdma_hal_set_dest_data_width(hal, id, data_width) hpdma_ll_set_dest_data_width((hal)->hw, id, data_width)

#define hpdma_hal_set_dest_sec_attr(hal, id, attr) hpdma_ll_set_dest_sec_attr((hal)->hw, id, attr)
#define hpdma_hal_set_src_sec_attr(hal, id, attr) hpdma_ll_set_src_sec_attr((hal)->hw, id, attr)
#define hpdma_hal_bus_err_int_enable(hal, id) hpdma_ll_bus_err_int_enable((hal)->hw, id)
#define hpdma_hal_bus_err_int_disable(hal, id) hpdma_ll_bus_err_int_disable((hal)->hw, id)
#define hpdma_hal_set_pixel_trans_type(hal, id, type) hpdma_ll_set_pixel_trans_type((hal)->hw, id, type)
#define hpdma_hal_get_pixel_trans_type(hal, id) hpdma_ll_get_pixel_trans_type((hal)->hw, id)
#define hpdma_hal_set_dest_burst_len(hal, id, len) hpdma_ll_set_dest_burst_len((hal)->hw, id, len)
#define hpdma_hal_get_dest_burst_len(hal, id) hpdma_ll_get_dest_burst_len((hal)->hw, id)

#define hpdma_hal_set_src_burst_len(hal, id, len) hpdma_ll_set_src_burst_len((hal)->hw, id, len)
#define hpdma_hal_get_src_burst_len(hal, id) hpdma_ll_get_src_burst_len((hal)->hw, id)

#define hpdma_hal_set_sec_attr(hal, id, attr) hpdma_ll_set_secure_attr((hal)->hw, id, attr)
#define hpdma_hal_set_privileged_attr(hal, id, attr) hpdma_ll_set_privileged_attr((hal)->hw, id, attr);
#define hpdma_hal_set_int_allocate(hal, id, int_id) hpdma_ll_set_int_allocate((hal)->hw, id, int_id)
#define hpdma_hal_get_int_allocate(hal, id) hpdma_ll_get_int_allocate((hal)->hw, id)

#define hpdma_hal_set_next_ll_addr(hal, id, ll_addr) hpdma_ll_set_next_ll_addr((hal)->hw, id, ll_addr)
#define hpdma_hal_get_next_ll_addr(hal, id) hpdma_ll_get_next_ll_addr((hal)->hw, id)
#define hpdma_hal_set_xsize(hal, id, src_xsize, dest_xsize) hpdma_ll_set_xsize((hal)->hw, id, src_xsize, dest_xsize)
#define hpdma_hal_set_ysize(hal, id, src_ysize, dest_ysize) hpdma_ll_set_ysize((hal)->hw, id, src_ysize, dest_ysize)
#define hpdma_hal_get_xsize(hal, id, src_xsize, dest_xsize) hpdma_ll_get_xsize((hal)->hw, id, src_xsize, dest_xsize)
#define hpdma_hal_get_ysize(hal, id, src_ysize, dest_ysize) hpdma_ll_get_ysize((hal)->hw, id, src_ysize, dest_ysize)
#define hpdma_hal_set_step(hal, id, src_step, dest_step) hpdma_ll_set_step((hal)->hw, id, src_step, dest_step)
#define hpdma_hal_get_step(hal, id, src_step, dest_step) hpdma_ll_get_step((hal)->hw, id, src_step, dest_step)


bk_err_t hpdma_hal_init(hpdma_hal_t *hal);
void hpdma_hal_init_without_channels(hpdma_hal_t *hal);
bk_err_t hpdma_hal_init_dma(hpdma_hal_t *hal, hpdma_id_t id, const hpdma_config_t *config);
bk_err_t hpdma_hal_start_common(hpdma_hal_t *hal, hpdma_id_t id);
bk_err_t hpdma_hal_stop_common(hpdma_hal_t *hal, hpdma_id_t id);



#if 0
void hpdma_struct_dump(hpdma_id_t id);
#else
#define dma_struct_dump(id)
#endif

#ifdef __cplusplus
}
#endif


