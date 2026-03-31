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

#define HPDMA_V_WORK_MODE_SINGLE       0x0
#define HPDMA_V_WORK_MODE_REPEAT       0x1

#define HPDMA_V_PRIO_MODE_ROUND_ROBIN      0x0
#define HPDMA_V_PRIO_MODE_FIXED_PRIO       0x1

#define HPDMA_V_REQ_MUX_DTCM             (0x0)
#define HPDMA_V_REQ_MUX_UART5            (0x1)
#define HPDMA_V_REQ_MUX_UART5_RX         (0x2)

#define HPDMA_HALF_FINISH_INT_POS        (18)
#define HPDMA_FINISH_INT_POS             (19)
#define HPDMA_BUS_ERR_INT_POS            (20)
#ifdef __cplusplus
}
#endif

