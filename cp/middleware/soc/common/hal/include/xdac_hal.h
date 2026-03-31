// Copyright 2020-2025 Beken
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

#include <common/bk_err.h>
#include "hal_config.h"
#include <driver/xdac.h>
#include <driver/xdac_types.h>
#include "xdac0_hw.h"
#include "xdac0_ll.h"
#include "xdac1_hw.h"
#include "xdac1_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    union {
        xdac0_hw_t *xdac0_hw;
        xdac1_hw_t *xdac1_hw;
        void       *hw_ptr;
    };
} xdac_hal_t;

bk_err_t xdac_hal_init(uint8_t ch, xdac_hal_t *hal);
uint32_t xdac_hal_get_fifo_empty_int(uint8_t ch);
uint32_t xdac_hal_get_fifo_full_int(uint8_t ch);
uint32_t xdac_hal_get_fifo_near_full_int(uint8_t ch);
uint32_t xdac_hal_get_fifo_near_empty_int(uint8_t ch);
uint32_t xdac_hal_get_fifo_empty_int_en(uint8_t ch);
uint32_t xdac_hal_get_fifo_full_int_en(uint8_t ch);
uint32_t xdac_hal_get_fifo_near_full_int_en(uint8_t ch);
uint32_t xdac_hal_get_fifo_near_empty_int_en(uint8_t ch);
bk_err_t xdac_hal_set_fifo_empty_int_en(uint8_t ch, uint8_t en);
bk_err_t xdac_hal_set_fifo_full_int_en(uint8_t ch, uint8_t en);
bk_err_t xdac_hal_set_fifo_near_full_int_en(uint8_t ch, uint8_t en);
bk_err_t xdac_hal_set_fifo_near_empty_int_en(uint8_t ch, uint8_t en);
bk_err_t xdac_hal_set_soft_reset(uint8_t ch, uint8_t rst);
bk_err_t xdac_hal_set_dac_clk_div(uint8_t ch, uint16_t clk_div);
uint32_t xdac_hal_get_dac_clk_div(uint8_t ch);
bk_err_t xdac_hal_set_dac_clk_en(uint8_t ch, uint8_t dac_clk_en);
bk_err_t xdac_hal_set_fifo_enable(uint8_t ch, uint8_t fifo_en);
bk_err_t xdac_hal_set_dac_rthrd(uint8_t ch, uint8_t dac_rthrd);
uint32_t xdac_hal_get_dac_rthrd(uint8_t ch);
bk_err_t xdac_hal_set_dac_wthrd(uint8_t ch, uint8_t dac_wthrd);
uint32_t xdac_hal_get_dac_wthrd(uint8_t ch);
bk_err_t xdac_hal_set_dac_enable(uint8_t ch, uint8_t dac_en);
uint32_t xdac_hal_get_fifo_addr(uint8_t ch);
bk_err_t xdac_hal_set_int_en(uint8_t ch, uint8_t int_type, uint8_t en);


#ifdef __cplusplus
}
#endif


