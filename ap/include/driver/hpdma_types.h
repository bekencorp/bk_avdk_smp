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

#include <driver/hal/hal_hpdma_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DMA defines
 * @defgroup bk_api_hpdma_defs macos
 * @ingroup bk_api_dma
 * @{
 */
#define BK_ERR_HPDMA_ID             (BK_ERR_HPDMA_BASE - 1) /**< HPDMA id is invalid */
#define BK_ERR_HPDMA_NOT_INIT       (BK_ERR_HPDMA_BASE - 2) /**< HPDMA driver not init */
#define BK_ERR_HPDMA_ID_NOT_INIT    (BK_ERR_HPDMA_BASE - 3) /**< HPDMA id not init */
#define BK_ERR_HPDMA_ID_NOT_START   (BK_ERR_HPDMA_BASE - 4) /**< HPDMA id not start */
#define BK_ERR_HPDMA_INVALID_ADDR   (BK_ERR_HPDMA_BASE - 5) /**< HPDMA addr is invalid */
#define BK_ERR_HPDMA_ID_REINIT      (BK_ERR_HPDMA_BASE - 6) /**< HPDMA id has inited, if reinit,please de-init firstly */
#define BK_ERR_HPDMA_TRANS_LEN      (BK_ERR_HPDMA_BASE - 7) /**< HPDMA  trans len  is invalid */

/**
 * @brief DMA interrupt service routine
 */
typedef void (*hpdma_isr_t)(hpdma_id_t hpdma_id, void *user_data);

/**
 * @brief DMA interrupt service routine with user data
 */
typedef struct {
    hpdma_isr_t callback;
    void *user_data;
} hpdma_isr_info_t;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

