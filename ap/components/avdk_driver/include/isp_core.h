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

#define ISP_SP_MB_LINE_STATE    (1u << 26)
#define ISP_MP_MB_LINE_STATE    (1u << 2)
#define ISP_SP_FRAME_END_STATE  (1u << 1)
#define ISP_MP_FRAME_END_STATE  (1)
#define BK_ERR_ISP_NOT_INIT             (BK_OK - 1) /**< ISP driver not init */
#define BK_ERR_ISP_CHNL_ID_INVALID      (BK_OK - 2) /**< ISP chne id invalid */
#define BK_ERR_ISP_CHNL_ID_INITED       (BK_OK - 3) /**< ISP chne id already init */

#define ISP_ISR_MODULE_MAX (8)

typedef void (*isp_isr_t)(uint32_t seqence, uint32_t line, uint8_t chnl, uint8_t error, void *param);

typedef enum
{
    ISP_MB_LINE_DONE = 0, /*isp mp & sp marco block line int*/
    ISP_FRAME_END_DONE, /*isp mp & sp frame end int*/
    ISP_ISR_MAX,
} isp_isr_type_t;

