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
//
#pragma once
#include <common/bk_include.h>
#include <driver/xdac_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */

/**
 * @brief AUXDAC API
 * @defgroup bk_api_aud AUXDAC API group
 * @{
 */


/**
 * @brief     Init the xdac
 *
 * This API init the xdac module:
 *  - Init xdac driver
 *
 * @param 
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_xdac_driver_init(void);


/**
 * @brief     Deinit xdac module
 *
 * This API deinit the xdac module:
 *   - Disable xdac
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_xdac_driver_deinit(void);


/**
 * @brief     allocate xdac channel
 *
 * @param pointer of xdac channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_xdac_alloc(xdac_ch_t *ch);

/**
 * @brief     free xdac channel
 *
 * @param pointer of xdac channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_xdac_free(xdac_ch_t ch);

/**
 * @brief     start xdac channel that is allocated 
 *
 * @param ch allocated xdac channel
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_XDAC_DRVER_NOT_INIT: xdac is not init
 *    - BK_ERR_XDAC_CH_INVALID: xdac channel is invalid
 *    - others: other errors.
 */
bk_err_t bk_xdac_start(uint8_t ch);

/**
 * @brief     stop xdac channel that is allocated
 *
 * @param ch allocated xdac channel
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_XDAC_DRVER_NOT_INIT: xdac is not init
 *    - BK_ERR_XDAC_CH_INVALID: xdac channel is invalid
 *    - others: other errors.
 */
bk_err_t bk_xdac_stop(uint8_t ch);

/**
 * @brief     deinit xdac channel that is allocated
 *
 * @param ch deinit xdac channel
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_XDAC_DRVER_NOT_INIT: xdac is not init
 *    - BK_ERR_XDAC_CH_INVALID: xdac channel is invalid
 *    - others: other errors.
 */
bk_err_t bk_xdac_deinit(uint8_t ch);


/**
 * @brief     get the xdac channel fifo address
 *
 * @param ch allocated xdac channel
 * @param fifo_addr point of var to save fifo address
 *
 * @return   xdac channel fifo address
 */
bk_err_t bk_xdac_get_fifo_addr(uint8_t ch, uint32_t *fifo_addr);

/**
 * @brief     Get xdac channel configuration
 *
 * @param ch allocated xdac channel
 * @param cfg save configration of allocated xdac channel
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - BK_ERR_XDAC_CH_INVALID: xdac channel is invalid
 *    - BK_ERR_XDAC_PTR_IS_INVALID:cfg is NULL
 *    - others: other errors.
 */
bk_err_t bk_xdac_get_cfg(uint8_t ch, xdac_config_t *cfg);

/**
 * @brief     Set xdac channel configuration
 *
 * @param ch allocated xdac channel
 * @param cfg configration of allocated xdac channel
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_XDAC_CH_INVALID: xdac channel is invalid
 *    - BK_ERR_XDAC_PTR_IS_INVALID:cfg is NULL
 *    - others: other errors.
 */
bk_err_t bk_xdac_set_cfg(xdac_config_t *cfg);


/**
 * @brief     Update configuration of xdac channel
 *
 *
 * @param cfg new configration of allocated xdac channel
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - BK_ERR_XDAC_CH_INVALID: xdac channel is invalid
 *    - BK_ERR_XDAC_PTR_IS_INVALID:cfg is NULL
 *    - others: other errors.
 */
bk_err_t bk_xdac_update_cfg(xdac_config_t *cfg);

/**
 * @brief     get xdac init status
 *
 *
 * @return    xdac init status
 */
bool bk_xdac_is_driver_inited(void);

/**
 * @brief     Get xdac channel configuration
 *
 * @param ch allocated xdac channel
 * @param int_type XDAC_EMPTY_INT/XDAC_FULL_INT/XDAC_NEAR_FULL_INT/XDAC_NEAR_EMPTY_INT
 * @param en 0:disabe 1:enable
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_XDAC_CH_INVALID: xdac channel is invalid
 *    - BK_ERR_XDAC_INT_TYPE_IS_INVALID:int type is invalid
 *    - others: other errors.
 */
bk_err_t bk_xdac_set_int_en(uint8_t ch, uint8_t int_type, uint8_t en);

/**
 * @brief     Clear xdac channel interrupt status
 *
 * @param ch allocated xdac channel
 *
 * @return
 */
void bk_xdac_clr_int_status(uint8_t ch);


/**
 * @}
 */
bk_err_t bk_xdac_set_sample_rate(uint8_t ch, uint32_t sample_rate);


#ifdef __cplusplus
}
#endif
