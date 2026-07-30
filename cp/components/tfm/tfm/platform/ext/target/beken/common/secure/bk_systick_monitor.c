/*
 * Copyright (c)     2023-2028 Wind River Systems, Inc.
 * Copyright (c)     2023-2028 Arm Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include "tfm_hal_device_header.h"
#include "boot_hal.h"
#include "uart_stdout.h"
#include "tfm_plat_otp.h"
#include "tfm_plat_provisioning.h"
#include "efuse.h"

#include "hal_sw_fih.h"
#include "bk_sca_defense.h"
#include "partitions_gen.h"

bool is_validate_image = false;

#if defined(CONFIG_BL2_SW_FIH) || defined(CONFIG_TFM_SW_FIH)
#define portNVIC_SYSTICK_CTRL_REG             ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG             ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SHPR3_REG                    ( *( ( volatile uint32_t * ) 0xe000ed20 ) )
#define portNVIC_SYSTICK_ENABLE_BIT           ( 1UL << 0UL )
#define portNVIC_SYSTICK_INT_BIT              ( 1UL << 1UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT       ( 1UL << 16UL )
#define portMIN_INTERRUPT_PRIORITY            ( 255UL )
#define portNVIC_PENDSV_PRI                   ( portMIN_INTERRUPT_PRIORITY << 16UL )
#define portNVIC_SYSTICK_PRI                  ( portMIN_INTERRUPT_PRIORITY << 24UL )
#define configCPU_TFM_CLOCK_HZ                ( 240000000 ) 
#define configSYSTICK_CLOCK_HZ			      ( configCPU_TFM_CLOCK_HZ )
#define portNVIC_SYSTICK_CLK_BIT		      ( 1UL << 2UL )
#define configTICK_TFM_RATE_HZ                ( 1200 )

static uint32_t s_bl2_tick_cnt = 0;

void systick_init(void)
{
    portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;

    /* Stop and reset the SysTick. */
    portNVIC_SYSTICK_CTRL_REG = 0UL;
    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

    /* Configure SysTick to interrupt at the requested rate. */
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_TFM_RATE_HZ ) - 1UL;
    portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;

	/*__enable_irq();*/
	__asm volatile ("cpsie i" : : : "memory");
}

void systick_uninit(void)
{
	portNVIC_SYSTICK_CTRL_REG = 0;
	portNVIC_SYSTICK_LOAD_REG = 0;
}

#if CONFIG_BL2_SW_FIH
void SysTick_Handler(void)
{
	s_bl2_tick_cnt ++;
	if (efuse_is_secureboot_enabled()) {
		bk_sw_cmp_data();
		extern void volt_temp_detect(void);
		volt_temp_detect();
#if CONFIG_SOC_BK7236N || CHIP_BK7236N ||CONFIG_SOC_BK7239N || CHIP_BK7239N
		bk_sca_buck_switch();
#endif
	}
	if (is_validate_image) {
		bk_sca_power_switch();
		bk_sca_read_flash(CONFIG_BL2_VIRTUAL_CODE_START);
	}
}

 #else
 #include "tfm_spm_log.h"

#define MAX_CALLBACKS 1

typedef struct {
	int32_t (*func)(void *,size_t, void *, size_t);
	void *a0;
	size_t a1;
	void *a2;
	size_t a3;
	bool active; 
} systick_cb_t;

static systick_cb_t callback_list[MAX_CALLBACKS] = {0};

bool register_systick_callback(int32_t (*func)(void *,size_t, void *, size_t),
							   void *a0, size_t a1, void *a2, size_t a3)
{
	if (!func) {
		return false;
	}

	__disable_irq();

	for (int i = 0; i < MAX_CALLBACKS; i++) {
		if (!callback_list[i].active) {
			callback_list[i].func = func;
			callback_list[i].a0 = a0;
			callback_list[i].a1 = a1;
			callback_list[i].a2 = a2;
			callback_list[i].a3 = a3;
			callback_list[i].active = true;
			__enable_irq();
			return true;
		}
	}
	__enable_irq();
	return false;
}

void unregister_systick_callback(int32_t (*func)(void *,size_t, void *, size_t))
{
	__disable_irq();

	for (int i = 0; i < MAX_CALLBACKS; i++) {
		if (callback_list[i].func == func && callback_list[i].active) {
			callback_list[i].active = false;
			break;
		}
	}

	__enable_irq();
}

 void SysTick_Handler(void)
{
	s_bl2_tick_cnt++;
	bk_sw_cmp_data();
	extern void volt_temp_detect(void);
	volt_temp_detect();

	for (int i = 0; i < MAX_CALLBACKS; i++) {
		if (!callback_list[i].active) {
			continue;
		}
		callback_list[i].func(callback_list[i].a0,
							  callback_list[i].a1,
							  callback_list[i].a2,
							  callback_list[i].a3);
		bk_sca_power_switch();
		bk_sca_read_flash(CONFIG_PRIMARY_TFM_S_VIRTUAL_CODE_START);
	}
	
	/* Only call bk_sca_random_freq if it's initialized */
	bk_sca_random_freq();
}
#endif /* CONFIG_BL2_SW_FIH */

#else
void systick_init(void)
{
}

void systick_uninit(void)
{
}

void SysTick_Handler(void)
{
}
#endif
// eof

