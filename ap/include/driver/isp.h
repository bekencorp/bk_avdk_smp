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

#include <driver/isp_types.h>
#include "isp_core.h"

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_isp_dev_init(isp_handle_t *handle);

bk_err_t bk_isp_port_init(isp_handle_t *handle, void *sensor_attr);

bk_err_t bk_isp_port_change(isp_handle_t *handle, uint8_t chnl);

bk_err_t bk_isp_deinit(isp_handle_t *handle);

bk_err_t bk_isp_open(isp_handle_t *handle, isp_config_ext_t *config);

bk_err_t bk_isp_close(isp_handle_t *handle, uint8_t chnl_id);

bk_err_t bk_isp_register_isr_callback(isp_handle_t *handle, isp_isr_type_t type, isp_isr_t cb, void *arg);

bk_err_t bk_isp_deregister_isr_callback(isp_handle_t *handle, isp_isr_type_t type, void *arg);

/**
 * @brief Vote to enable/disable ISP (CISP) clock.
 *
 * Shared by ISP driver and camera bus. Clock is powered up on the first
 * enable vote and powered down only when all enable votes are released.
 *
 * @param enable 1 to take a vote, 0 to release a vote
 */
bk_err_t bk_isp_clock_enable(uint8_t enable);

bk_err_t bk_cis_auxs_clock_enable(uint32_t clk, uint32_t gpio, uint8_t enable);

bk_err_t bk_cis_mclk_clock_enable(uint32_t clk, uint8_t gpio, uint8_t enable);

bk_err_t bk_isp_flexa_sbi_config(isp_handle_t *handle, uint8_t chnl, uint8_t enable);

bk_err_t bk_isp_soft_reset(isp_handle_t *handle);

bk_err_t bk_isp_get_exposure_luminance(isp_handle_t *handle, uint32_t *luminance);

bk_err_t bk_isp_get_cproc_attr(isp_handle_t *handle, void *cproc_attr);

bk_err_t bk_isp_set_cproc_attr(isp_handle_t *handle, void *cproc_attr);

#ifdef __cplusplus
}
#endif
