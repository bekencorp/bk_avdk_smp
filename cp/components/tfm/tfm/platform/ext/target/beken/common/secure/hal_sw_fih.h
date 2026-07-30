// Copyright     2023-2028 Beken
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

#include <stdint.h>
#include <stdbool.h>

#define SW_FIH_INDEX_MAX 32
#define SW_FIH_INVALID_INDEX (-1)

/* FIH Software Fault Injection Hardening indices */
typedef enum {
    FIH_SW_INDEX0 = 0,
    FIH_SW_INDEX1,
    FIH_SW_INDEX2,
    FIH_SW_INDEX3,
    FIH_SW_INDEX4,
    FIH_SW_INDEX5,
    FIH_SW_INDEX6,
    FIH_SW_INDEX7,
    FIH_SW_INDEX8,
    FIH_SW_INDEX9,
    FIH_SW_INDEX10,
    FIH_SW_INDEX11,
    FIH_SW_INDEX12,
    FIH_SW_INDEX13,
    FIH_SW_INDEX14,
    FIH_SW_INDEX15,
    FIH_SW_INDEX16,
    FIH_SW_INDEX17,
    FIH_SW_INDEX18,
    FIH_SW_INDEX19,
    FIH_SW_INDEX20,
    FIH_SW_INDEX21,
    FIH_SW_INDEX22,
    FIH_SW_INDEX23,
    FIH_SW_INDEX24,
    FIH_SW_INDEX25,
    FIH_SW_INDEX26,
    FIH_SW_INDEX27,
    FIH_SW_INDEX28,
    FIH_SW_INDEX29,
    FIH_SW_INDEX30,
    FIH_SW_INDEX31,
    FIH_SW_INVALID = SW_FIH_INDEX_MAX,
} fih_sw_index_t;

/* FIH result codes */
typedef enum {
    SW_FIH_SUCCESS = 0,
    SW_FIH_FAILURE = 1,
    FIH_ERROR_INVALID_PARAM = 2,
    FIH_ERROR_OUT_OF_RANGE = 3,
} fih_result_t;

#if defined(CONFIG_BL2_SW_FIH) || defined(CONFIG_TFM_SW_FIH)

/**
 * @brief Set FIH data for a specific index
 * @param id FIH index to set
 * @return FIH result code
 */
fih_result_t bk_sw_fih_set_data(fih_sw_index_t id);

/**
 * @brief Compare current FIH data with target data
 * @return FIH result code
 */
fih_result_t bk_sw_cmp_data(void);

/**
 * @brief Initialize FIH system
 * @return FIH result code
 */
fih_result_t bk_sw_fih_init(void);

/**
 * @brief Reset FIH data to initial state
 */
void bk_sw_fih_reset(void);

/**
 * @brief Check if FIH index is valid
 * @param id FIH index to check
 * @return true if valid, false otherwise
 */
bool bk_sw_fih_is_valid_index(fih_sw_index_t id);

#else
#define UNUSED_VAR(var) (void)(var)
#define bk_sw_fih_set_data(id) SW_FIH_SUCCESS
#define bk_sw_cmp_data() SW_FIH_SUCCESS
#define bk_sw_fih_init() SW_FIH_SUCCESS
#define bk_sw_fih_reset()
#define bk_sw_fih_is_valid_index(id) true
#endif

/**
 * @brief Dump FIH register values for debugging
 */
void dump_sw_fih_regs(void);

/**
 * @brief Initialize system tick timer
 */
void systick_init(void);

/**
 * @brief Uninitialize system tick timer
 */
void systick_uninit(void);
