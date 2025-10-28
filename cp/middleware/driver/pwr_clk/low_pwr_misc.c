// Copyright 2025-2026 Beken
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

#include <common/bk_include.h>
#include <driver/pwr_clk.h>
#include <modules/pm.h>
#include "sys_driver.h"
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <os/os.h>
#include <components/system.h>
#include <common/bk_kernel_err.h>
#include "aon_pmu_hal.h"
#include <components/log.h>
#include <driver/gpio.h>
#include <driver/hal/hal_gpio_types.h>
#include "gpio_hal.h"
#include "gpio_driver_base.h"
#include "gpio_driver.h"
#include <os/mem.h>
/*=====================DEFINE  SECTION  START=====================*/

#define PM_TAG "pm"
#define LOGD(...) BK_LOGD(PM_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(PM_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(PM_TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(PM_TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(PM_TAG, ##__VA_ARGS__)

/*=====================DEFINE  SECTION  END=====================*/
typedef struct gpio_ldo_vote_node {
	gpio_id_t gpio_id;                    /* GPIO ID */
	uint32_t vote_state;                  /* Vote state bitmap (32 bits, supports 32 modules) */
	struct gpio_ldo_vote_node *next;      /* Next node */
} gpio_ldo_vote_node_t;

/*=====================VARIABLE  SECTION  START=================*/
uint64_t static s_startup_rtc_tick = 0;
static gpio_ldo_vote_node_t *s_gpio_ldo_vote_list = NULL;

/*=====================VARIABLE  SECTION  END=================*/

/*================FUNCTION DECLARATION  SECTION  START========*/


/*================FUNCTION DECLARATION  SECTION  END========*/
bk_err_t bk_low_pwr_misc_rtc_enter_deepsleep(uint32_t time_interval , aon_rtc_isr_t callback)
{
	#if CONFIG_AON_RTC || CONFIG_ANA_RTC
	bk_err_t ret = BK_FAIL;
	alarm_info_t deep_sleep_alarm = {
									"pwr_misc",
									time_interval*AON_RTC_MS_TICK_CNT,
									1,
									callback,
									NULL
									};
	bk_alarm_unregister(AON_RTC_ID_1, deep_sleep_alarm.name);
	ret = bk_alarm_register(AON_RTC_ID_1, &deep_sleep_alarm);
	if(ret != BK_OK)
    {
	 	return ret;
    }
	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
	#endif //CONFIG_AON_RTC
	bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);
	return BK_OK;
}
bk_err_t bk_low_pwr_misc_get_time_interval_from_startup(uint32_t* time_interval)
{
	#if CONFIG_AON_RTC
	uint32_t tick_count = 0.0;
	uint64_t entry_tick  =0;
	if(time_interval == NULL)
    {
      	return BK_FAIL;
    }
	entry_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);

	pm_lpo_src_e lpo_src = bk_pm_lpo_src_get();
	if(lpo_src == PM_LPO_SRC_X32K)
	{
		tick_count = AON_RTC_EXTERN_32K_CLOCK_FREQ;
	}
	else
	{
		tick_count = AON_RTC_DEFAULT_CLOCK_FREQ;
	}

	*time_interval = (uint32_t)(((entry_tick - s_startup_rtc_tick)*1000000)/tick_count);
	#endif
	return BK_OK;
}
bk_err_t bk_low_pwr_misc_startup_rtc_tick_set(uint64_t time_tick)
{
	s_startup_rtc_tick = time_tick;
	return BK_OK;
}
#if CONFIG_DEEPSLEEP_USING_WDT_PROTECT
bk_err_t bk_low_pwr_deepsleep_using_wdt_protect()
{
	if(aon_pmu_hal_get_reset_reason() == RESET_SOURCE_FORCE_DEEPSLEEP)
    {
        /*Get the deepsleep protect count*/
        sleep_count = aon_pmu_hal_get_wdt_deepsleep_pt_count();
        sleep_count = sleep_count -1;
        if (sleep_count > 0)
        {
            /*Clear RTC INT */
            uint32_t value = REG_READ(0x44000200);
            value  |= (0x1 >> 4);
            value  |= (0x1 >> 5);
            REG_WRITE(0x44000200,value);

            value = REG_READ(0x44000200+0x3*4);
            value = value+60000*32;
            REG_WRITE(0x44000200+0x2*4,value);
            aon_pmu_hal_set_wdt_deepsleep_pt_count(sleep_count,false);
            sys_drv_enter_deep_sleep(NULL);
        }
    }
	return BK_OK;
}
#endif

static gpio_ldo_vote_node_t* gpio_ldo_find_or_create_node(gpio_id_t gpio_id, bool create)
{
	gpio_ldo_vote_node_t *current = s_gpio_ldo_vote_list;
	gpio_ldo_vote_node_t *prev = NULL;

	/* Traverse the linked list to check if GPIO node already exists */
	while (current != NULL) {
		if (current->gpio_id == gpio_id) {
			/* Found matching GPIO */
			return current;
		}
		prev = current;
		current = current->next;
	}

	/* Not found, allocate new node if creation is requested */
	if (create) {
		gpio_ldo_vote_node_t *new_node = (gpio_ldo_vote_node_t *)os_malloc(sizeof(gpio_ldo_vote_node_t));
		if (new_node == NULL) {
			LOGE("Failed to allocate GPIO LDO vote node for GPIO %d\r\n", gpio_id);
			return NULL;
		}

		/* Initialize new node */
		new_node->gpio_id = gpio_id;
		new_node->vote_state = 0;
		new_node->next = NULL;

		/* Add to the tail of the list */
		if (prev == NULL) {
			/* List is empty, set as head node */
			s_gpio_ldo_vote_list = new_node;
		} else {
			/* Add to the end of the list */
			prev->next = new_node;
		}

		LOGV("GPIO %d LDO vote node created (%d bytes)\r\n", gpio_id, sizeof(gpio_ldo_vote_node_t));
		return new_node;
	}

	/* Not creating and not found */
	return NULL;
}

static bk_err_t gpio_ldo_remove_node(gpio_id_t gpio_id)
{
	gpio_ldo_vote_node_t *current = s_gpio_ldo_vote_list;
	gpio_ldo_vote_node_t *prev = NULL;

	/* Traverse the linked list to find the node */
	while (current != NULL) {
		if (current->gpio_id == gpio_id) {
			/* Found the node, remove it from the list */
			if (prev == NULL) {
				/* It's the head node */
				s_gpio_ldo_vote_list = current->next;
			} else {
				/* It's a middle or tail node */
				prev->next = current->next;
			}

			/* Free node memory */
			os_free(current);
			LOGV("GPIO %d LDO vote node removed\r\n", gpio_id);
			return BK_OK;
		}
		prev = current;
		current = current->next;
	}

	return BK_ERR_GPIO_CHAN_ID;
}

static bk_err_t gpio_ldo_configure_output(gpio_id_t gpio_id, bool output_level)
{
	bk_err_t ret = BK_OK;

	if (gpio_id >= GPIO_NUM_MAX || gpio_id < 0) {
		LOGE("Invalid gpio_id: %d\r\n", gpio_id);
		return BK_ERR_GPIO_CHAN_ID;
	}

	/* GPIO function unmap and initialization */
	ret |= gpio_dev_unmap(gpio_id);
	if(ret != BK_OK)
	{
		LOGE("Failed to unmap for GPIO %d ret:%d\r\n", gpio_id, ret);
		return ret;
	}
	/* Configure GPIO as output mode */
	ret |= bk_gpio_set_capacity(gpio_id, 0);
	if(ret != BK_OK)
	{
		LOGE("Failed to set capacity for GPIO %d ret:%d\r\n", gpio_id, ret);
		return ret;
	}
	ret |= bk_gpio_disable_input(gpio_id);
	if(ret != BK_OK)
	{
		LOGE("Failed to disable input for GPIO %d ret:%d\r\n", gpio_id, ret);
		return ret;
	}
	ret |= bk_gpio_enable_output(gpio_id);
	if(ret != BK_OK)
	{
		LOGE("Failed to enable output for GPIO %d ret:%d\r\n", gpio_id, ret);
		return ret;
	}

	/* Set output level */
	if (output_level) {
		ret |= bk_gpio_set_output_high(gpio_id);
		if(ret != BK_OK)
		{
			LOGE("Failed to set output high for GPIO %d ret:%d\r\n", gpio_id, ret);
			return ret;
		}
	} else {
		ret |= bk_gpio_set_output_low(gpio_id);
		if(ret != BK_OK)
		{
			LOGE("Failed to set output low for GPIO %d ret:%d\r\n", gpio_id, ret);
			return ret;
		}
	}

	return ret;
}

static bk_err_t bk_gpio_get_ldo_vote_state(gpio_id_t gpio_id, uint32_t *vote_state)
{
	gpio_ldo_vote_node_t *node = NULL;

	if (gpio_id >= GPIO_NUM_MAX || gpio_id < 0) {
		LOGE("Invalid gpio_id: %d\r\n", gpio_id);
		return BK_ERR_GPIO_CHAN_ID;
	}

	if (vote_state == NULL) {
		LOGE("vote_state pointer is NULL\r\n");
		return BK_ERR_GPIO_INVALID_MODE;
	}

	/* Find the vote node for this GPIO */
	node = gpio_ldo_find_or_create_node(gpio_id, false);
	if (node == NULL) {
		/* Node doesn't exist, meaning never used, vote state is 0 */
		*vote_state = 0;
	} else {
		*vote_state = node->vote_state;
	}

	return BK_OK;
}
bk_err_t bk_gpio_ctrl_external_ldo(uint32_t module, gpio_id_t gpio_id, gpio_output_state_e value)
{
	bk_err_t ret               = BK_OK;
	uint32_t module_mask       = 0;
	uint32_t old_vote_state    = 0;
	gpio_ldo_vote_node_t *node = NULL;

	if (gpio_id >= GPIO_NUM_MAX || gpio_id < 0) {
		LOGE("Invalid gpio_id: %d\r\n", gpio_id);
		return BK_ERR_GPIO_CHAN_ID;
	}

	if (module >= 32) {
		LOGE("Invalid module: %d (valid range: 0~31)\r\n", module);
		return BK_ERR_GPIO_INVALID_MODE;
	}

	if (value != GPIO_OUTPUT_STATE_HIGH && value != GPIO_OUTPUT_STATE_LOW) {
		LOGE("Invalid output state: %d\r\n", value);
		return BK_ERR_GPIO_INVALID_MODE;
	}

	module_mask = (0x1 << module);

	/* Handle vote enable (output high level) */
	if (value == GPIO_OUTPUT_STATE_HIGH) {
		/* Find or create the vote node for this GPIO */
		node = gpio_ldo_find_or_create_node(gpio_id, true);
		if (node == NULL) {
			LOGE("Failed to create vote node for GPIO %d\r\n", gpio_id);
			return BK_ERR_NO_MEM;
		}

		old_vote_state = node->vote_state;

		/* Set the vote bit for this module */
		node->vote_state |= module_mask;

		/* If this is the first module voting, initialize GPIO and output high level */
		if (old_vote_state == 0) {
			ret = gpio_ldo_configure_output(gpio_id, true);
			if (ret != BK_OK) {
				LOGE("Failed to configure GPIO %d output HIGH (module %d) ret:%d\r\n", gpio_id, module, ret);
				/* Rollback vote state on failure */
				node->vote_state = old_vote_state;
				/* If it's a newly created node and configuration failed, delete the node */
				gpio_ldo_remove_node(gpio_id);
			} else {
				LOGV("GPIO %d output HIGH (module %d vote, state=0x%x)\r\n",gpio_id, module, node->vote_state);
			}
		} else {
			LOGV("GPIO %d already HIGH, module %d vote added (state=0x%x)\r\n",gpio_id, module, node->vote_state);
		}
	}
	/* Handle vote release (output low level) */
	else {
		/* Find the vote node for this GPIO (don't create) */
		node = gpio_ldo_find_or_create_node(gpio_id, true);
		if (node == NULL) {
			/* Node doesn't exist, meaning never voted, return success directly */
			LOGV("GPIO %d vote node not exist, already released\r\n", gpio_id);
			return BK_OK;
		}

		old_vote_state = node->vote_state;

		/* Clear the vote bit for this module */
		node->vote_state &= ~module_mask;

		/* Only output low level when all modules have released their votes */
		if (node->vote_state == 0) {
			ret = gpio_ldo_configure_output(gpio_id, false);
			if (ret != BK_OK) {
				LOGE("Failed to configure GPIO %d output LOW (module %d)\r\n", gpio_id, module);
				/* Rollback vote state on failure */
				node->vote_state = old_vote_state;
			} else {
				LOGV("GPIO %d output LOW (all modules released)\r\n", gpio_id);
				/* All modules released, delete the node to free memory */
				gpio_ldo_remove_node(gpio_id);
			}
		} else {
			LOGV("GPIO %d keep HIGH, module %d vote removed (remaining state=0x%x)\r\n",gpio_id, module, node->vote_state);
		}
	}

	return ret;
}