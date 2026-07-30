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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "hal_sw_fih.h"
#include "fih.h"
#include "components/log.h"
#include "partitions_gen.h"

extern void tfm_core_panic(void);
#define TAG "FIH"

#if defined(CONFIG_BL2_SW_FIH) || defined(CONFIG_TFM_SW_FIH)

static uint8_t sw_fih_data_target[SW_FIH_INDEX_MAX];

/* Runtime FIH data */
static uint8_t sw_fih_data[SW_FIH_INDEX_MAX] = {0};
static int sw_fih_index = SW_FIH_INVALID_INDEX;
static bool fih_initialized = false;

bool bk_sw_fih_is_valid_index(fih_sw_index_t id)
{
	return (id >= 0 && id < FIH_SW_INVALID);
}

fih_result_t bk_sw_fih_init(void)
{
	if (fih_initialized) {
		return SW_FIH_SUCCESS;
	}

	for (int i = 0; i < SW_FIH_INDEX_MAX; i++) {
		sw_fih_data_target[i] = rand() % 256;
	}
	/* Clear all FIH data */
	memset(sw_fih_data, 0, sizeof(sw_fih_data));
	sw_fih_index = SW_FIH_INVALID_INDEX;
	fih_initialized = true;
	
	BK_LOGI(TAG, "FIH system initialized\r\n");
	return SW_FIH_SUCCESS;
}

fih_result_t bk_sw_fih_set_data(fih_sw_index_t id)
{
	if (!bk_sw_fih_is_valid_index(id)) {
		BK_LOGE(TAG, "Invalid FIH index: %d\r\n", id);
		return FIH_ERROR_INVALID_PARAM;
	}
	
	if (!fih_initialized) {
		bk_sw_fih_init();
	}
	
	sw_fih_data[id] = sw_fih_data_target[id];
	sw_fih_index = id;
	
	BK_LOGD(TAG, "FIH data set for index %d\r\n", id);
	return SW_FIH_SUCCESS;
}

fih_result_t bk_sw_cmp_data(void)
{
	if (!fih_initialized) {
		BK_LOGE(TAG, "FIH system not initialized\r\n");
		return FIH_ERROR_INVALID_PARAM;
	}
	
	if (sw_fih_index == SW_FIH_INVALID_INDEX) {
		/* No data to compare */
		return SW_FIH_SUCCESS;
	}
	
	if (!bk_sw_fih_is_valid_index(sw_fih_index)) {
		BK_LOGE(TAG, "Invalid FIH index: %d\r\n", sw_fih_index);
		return FIH_ERROR_INVALID_PARAM;
	}
	
	/* Compare only the data up to the current index */
	if (memcmp(sw_fih_data, sw_fih_data_target, sw_fih_index + 1) != 0) {
		dump_sw_fih_regs();
		BK_LOGE(TAG, "FIH data mismatch detected!\r\n");
		
#if CONFIG_BL2_SW_FIH
		FIH_PANIC;
#else 
		tfm_core_panic();
#endif
		return SW_FIH_FAILURE;
	}
	
	return SW_FIH_SUCCESS;
}

void dump_sw_fih_regs(void)
{
	BK_LOGI(TAG, "=== FIH Debug Information ===\r\n");
	BK_LOGI(TAG, "FIH initialized: %s\r\n", fih_initialized ? "Yes" : "No");
	BK_LOGI(TAG, "Current FIH index: %d\r\n", sw_fih_index);
	
	if (sw_fih_index != SW_FIH_INVALID_INDEX && bk_sw_fih_is_valid_index(sw_fih_index)) {
		BK_LOGI(TAG, "FIH Data Comparison:\r\n");
		for (int i = 0; i <= sw_fih_index; i++) {
			BK_LOGI(TAG, "[%2d] Target: 0x%02x, Current: 0x%02x %s\r\n", 
					i, sw_fih_data_target[i], sw_fih_data[i],
					(sw_fih_data[i] == sw_fih_data_target[i]) ? "✓" : "✗");
		}
	} else {
		BK_LOGI(TAG, "No valid FIH data to display\r\n");
	}
	BK_LOGI(TAG, "=============================\r\n");
}
#endif
