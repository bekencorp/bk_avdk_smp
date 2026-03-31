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

#include <driver/int_types.h>
#include <common/bk_include.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BK_ERR_XDAC_DRVER_NOT_INIT         (BK_ERR_XDAC_HAL_BASE - 1) /**< xdac driver is not initialized */
#define BK_ERR_XDAC_CH_INVALID             (BK_ERR_XDAC_HAL_BASE - 2) /**< xdac driver channel is invalid */
#define BK_ERR_XDAC_LACK_OF_MEM            (BK_ERR_XDAC_HAL_BASE - 3) /**< xdac driver lack of memory */
#define BK_ERR_XDAC_PTR_IS_INVALID         (BK_ERR_XDAC_HAL_BASE - 4) /**< xdac drvier ptr is invalid */
#define BK_ERR_XDAC_INT_TYPE_IS_INVALID    (BK_ERR_XDAC_HAL_BASE - 5) /**< xdac drvier int type invalid */



/**
 * @brief AUXDAC enum defines
 * @defgroup bk_api_xdac_enum AUXDAC enums
 * @ingroup bk_api_xdac
 * @{
 */

typedef enum {
    XDAC_0 = 0,
    XDAC_1,
    XDAC_CH_CNT
} xdac_ch_t;
    
typedef enum {
    XDAC_EMPTY_INT = 0,
    XDAC_FULL_INT,
    XDAC_NEAR_FULL_INT,
    XDAC_NEAR_EMPTY_INT,
    XDAC_INT_TYPE_MAX
} xdac_int_type_t;

/**
 * @}
 */

/**
 * @brief AUXDAC struct defines
 * @defgroup bk_api_xdac_structs structs in AUXDAC
 * @ingroup bk_api_xdac
 * @{
 */
 typedef struct {
    uint8_t  ch;              /**< AUXDAC channel*/
    uint8_t  dac_wthrd;       /**< AUXDAC fifo near full threshold*/
    uint8_t  dac_rthrd;       /**< AUXDAC fifo near empty threshold*/
    uint8_t  reserved;        
    uint16_t dac_clk_div;     /**< AUXDAC dac clock divder*/
    uint16_t reserverd_1;
    void   (*xdac_isr)(void); /**< AUXDAC interrupt isr*/
} xdac_config_t;

/**
 * @}
 */


#ifdef __cplusplus
}
#endif
