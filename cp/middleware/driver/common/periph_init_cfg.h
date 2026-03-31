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

#define DEV_ID_MAX_VAL    (0xFFFFU)
#define DEV_ID_MASK       (DEV_ID_MAX_VAL)
#define AFFINITY_INVULNERABILITY ((DEV_ID_MAX_VAL << 16))
#define DEV_ID_POSI            (16)
#define IS_INITED_FLAG_BIT_CNT (4)
#define IS_INITED_FLAG_MASK    (0xF)

/* csv auto-generated
 * the following is the format, and the core numbere is varible, and the maximum core number is 16
 * dev_id, coren, core1, core0, is_inited
 * dev_id: 8bit, device index
 * core0: 1bit, non-zero value, the device will be inited or used at the current core
 * core1: 1bit, non-zero value, the device will be inited or used at the current core
 *  ...       , non-zero value, the device will be inited or used at the current core
 * coren: 1bit, non-zero value, the device will be inited or used at the current core
 * is_inited: 8bit, the init flag
 * */
#define CORE_CNT_IN_USE (4)

/* csv auto-generated
 */
#define DEV_AFFINITY_MAP {\
    ((DEV_SYS_ID << DEV_ID_POSI) + (0 << 7) + (0 << 6) + (0 << 5) + (1 << IS_INITED_FLAG_BIT_CNT) + (0 << 0)),\
    ((DEV_DMA_ID << DEV_ID_POSI) + (1 << 7) + (1 << 6) + (1 << 5) + (1 << IS_INITED_FLAG_BIT_CNT) + (0 << 0)),\
    ((DEV_ID_MAX_VAL << DEV_ID_POSI) + (1 << 7) + (1 << 6) + (1 << 5) + (1 << IS_INITED_FLAG_BIT_CNT) + (0 << 0)),\
}

#ifndef CORE_CNT_IN_USE
#define CORE_CNT_IN_USE   (1)
#endif
//eof
