// Copyright 2020-2024 Beken
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
#include <os/os.h>
#include <driver/int.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_hal_v2px.h"
#include "gpio_driver_base.h"
#include "sys_driver.h"
#if CONFIG_ANA_GPIO
#include "ana_gpio_driver.h"
#endif
#include "bk_misc.h"

#define GPIO_REG_DEFAULT_VALUE                    (0x0)
#define GPIO_WAKE_SOURCE_IDLE_ID                  (GPIO_NUM_MAX)
#define GPIO_LOWPOWER_KEEP_STATUS_IDLE_ID         (GPIO_NUM_MAX)

#define GPIO_RETURN_ON_INVALID_ID(id) do {\
	if ((id) >= SOC_GPIO_NUM) {\
		return BK_ERR_GPIO_INVALID_ID;\
	}\
} while(0)

typedef struct
{
	gpio_id_t id;
	gpio_int_type_t int_type;
	//gpio_isr_t isr;
} gpio_dynamic_wakeup_t;

typedef struct
{
	gpio_id_t gpio_id;
	gpio_config_t config;
} gpio_dynamic_keep_status_t;

static bool s_gpio_is_init = false;
static gpio_isr_t s_gpio_isr[SOC_GPIO_NUM] = {NULL};
static uint32_t s_gpio_baked_regs[SOC_GPIO_NUM] = {0};

#if CONFIG_GPIO_WAKEUP_SUPPORT
static uint64_t s_gpio_is_setted_wake_status;
static gpio_id_t s_gpio_wakeup_gpio_id = SOC_GPIO_NUM;
#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
static gpio_dynamic_wakeup_t s_gpio_dynamic_wakeup_source_map[CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT];
#endif
#endif

#if CONFIG_GPIO_KPSTAT_SUPPORT
static uint64_t s_gpio_is_lowpower_keep_status;
#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
static gpio_dynamic_keep_status_t s_gpio_lowpower_keep_config[CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT];
#endif
#endif


#if CONFIG_GPIO_DEFAULT_SET_SUPPORT
static void gpio_default_map_init(void);
#endif

#if CONFIG_GPIO_WAKEUP_SUPPORT
static void gpio_wakeup_source_config(void);
static void gpio_record_wakeup_pin_id(void);
#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
static void gpio_dynamic_wakeup_source_init(void);
#endif
#endif

#if CONFIG_GPIO_KPSTAT_SUPPORT
static void gpio_keep_status_init(void);
#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
static void gpio_keep_status_config(void);
#endif
#endif

static void gpio_isr(void);
bk_err_t bk_gpio_driver_init(void)
{
	//avoid re-init caused some info lost
	if (s_gpio_is_init) {
		GPIO_LOGD("%s:has inited \r\n", __func__);
		return BK_OK;
	}

	gpio_hal_init();

#if CONFIG_GPIO_KPSTAT_SUPPORT
	gpio_keep_status_init();
#endif

#if CONFIG_GPIO_WAKEUP_SUPPORT
	gpio_record_wakeup_pin_id();

#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
	gpio_dynamic_wakeup_source_init();
#endif

#if CONFIG_ANA_GPIO
	ana_gpio_wakeup_init();
#endif
#endif

#if CONFIG_GPIO_DEFAULT_SET_SUPPORT
	gpio_default_map_init();
#endif

	//Move ISR to last to avoid other resouce doesn't finish but isr has came.
	//F.E:GPIO wakeup deepsleep.
#if CONFIG_TZ && (!CONFIG_SPE)
	bk_int_isr_register(INT_SRC_GPIO_NS, gpio_isr, NULL);
#else
	bk_int_isr_register(INT_SRC_GPIO, gpio_isr, NULL);
#endif

	//interrupt to CPU enable
#if CONFIG_TZ && (!CONFIG_SPE)
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPIO_NS, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPIO_NS, 1);
#endif
#else
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPIO, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPIO, 1);
#endif
#endif

	s_gpio_is_init = true;

#if CONFIG_CLI && CONFIG_GPIO_TEST
	int bk_gpio_register_cli_test_feature(void);
	bk_gpio_register_cli_test_feature();
#endif

	return BK_OK;
}

bk_err_t bk_gpio_driver_deinit(void)
{
	if (!s_gpio_is_init)
	{
		GPIO_LOGD("%s:isn't init \r\n", __func__);
		return BK_OK;
	}

#if CONFIG_TZ && (!CONFIG_SPE)
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPIO_NS, 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPIO_NS, 0);
#endif
#else
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPIO, 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPIO, 0);
#endif
#endif

	s_gpio_is_init = false;

	return BK_OK;
}

uint32_t bk_gpio_get_gpio_func_code(uint32_t gpio_id)
{
	return gpio_hal_get_func_code(gpio_id);
}

bk_err_t bk_gpio_set_gpio_func(uint32_t gpio_id, IOMX_CODE_T func_code)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);

	return gpio_hal_set_func_code(gpio_id, func_code);
}

void bk_gpio_set_value(gpio_id_t id, uint32_t v)
{
	gpio_hal_set_value(id, v);
}

uint32_t bk_gpio_get_value(gpio_id_t id)
{
	return gpio_hal_get_value(id);
}

bk_err_t bk_gpio_enable_output(gpio_id_t gpio_id)
{
	return gpio_hal_set_func_code(gpio_id, FUNC_CODE_OUTPUT);
}

bk_err_t bk_gpio_disable_output(gpio_id_t gpio_id)
{
	// do nothing
	return BK_OK;
}

bk_err_t bk_gpio_enable_input(gpio_id_t gpio_id)
{
	return gpio_hal_set_func_code(gpio_id, FUNC_CODE_INPUT);
}

bk_err_t bk_gpio_disable_input(gpio_id_t gpio_id)
{
	// do nothing
	return BK_OK;
}

bk_err_t bk_gpio_enable_pull(gpio_id_t gpio_id )
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	return gpio_hal_pull_enable(gpio_id, 1);
}

bk_err_t bk_gpio_disable_pull(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	return gpio_hal_pull_enable(gpio_id, 0);
}

bk_err_t bk_gpio_pull_up(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	gpio_hal_pull_enable(gpio_id, 1);
	return gpio_hal_pull_up_enable(gpio_id, 1);
}

bk_err_t bk_gpio_pull_down(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	gpio_hal_pull_enable(gpio_id, 1);
	return gpio_hal_pull_up_enable(gpio_id, 0);
}

bk_err_t bk_gpio_set_output_high(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	return gpio_hal_set_output_value(gpio_id, 1);
}

bk_err_t bk_gpio_set_output_low(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	return gpio_hal_set_output_value(gpio_id, 0);
}

bool bk_gpio_get_output(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);

	return gpio_hal_get_output(gpio_id);
}

bool bk_gpio_get_input(gpio_id_t gpio_id)
{
	return gpio_hal_get_input(gpio_id);
}

//MAX capactiy:3
bool bk_gpio_set_capacity(gpio_id_t gpio_id, uint32 capacity)
{
	return gpio_hal_set_capacity(gpio_id, capacity);
}

bk_err_t bk_gpio_set_config(gpio_id_t gpio_id, const gpio_config_t *config)
{
	switch (config->io_mode) {
	case GPIO_OUTPUT_ENABLE:
		bk_gpio_set_gpio_func(gpio_id, FUNC_CODE_OUTPUT);
		break;

	case GPIO_INPUT_ENABLE:
		bk_gpio_set_gpio_func(gpio_id, FUNC_CODE_INPUT);
		break;

	case GPIO_IO_DISABLE:
		bk_gpio_set_gpio_func(gpio_id, FUNC_CODE_HIGH_Z);
		break;

	default:
		break;
	}

	switch (config->pull_mode) {
	case GPIO_PULL_DISABLE:
		bk_gpio_disable_pull(gpio_id);
		break;

	case GPIO_PULL_DOWN_EN:
		bk_gpio_pull_down(gpio_id);
		break;

	case GPIO_PULL_UP_EN:
		bk_gpio_pull_up(gpio_id);
		break;

	default:
		break;
	}

	return BK_OK;
}

/* Enable GPIO  interrupt.
*/
bk_err_t bk_gpio_register_isr(gpio_id_t gpio_id, gpio_isr_t isr)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);

	s_gpio_isr[gpio_id] = isr;

	return BK_OK;
}

//This function just enable the select GPIO can report IRQ to CPU
bk_err_t bk_gpio_enable_interrupt(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);

	//Before enable the interrupt,wait for the internal stability of the chip
	for (volatile int i = 0; i < 1000; i++);

	return gpio_hal_enable_interrupt(gpio_id);
}

bk_err_t bk_gpio_disable_interrupt(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	gpio_hal_disable_interrupt(gpio_id);
	return BK_OK;
}

bk_err_t bk_gpio_clear_interrupt(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	gpio_hal_clear_chan_interrupt_status(gpio_id);
	return BK_OK;
}

bk_err_t bk_gpio_set_interrupt_type(gpio_id_t gpio_id, gpio_int_type_t type)
{
	return gpio_hal_set_int_type(gpio_id, type);
}

static void gpio_isr(void)
{
	gpio_interrupt_status_t gpio_status;
	int gpio_id;

	gpio_hal_get_interrupt_status(&gpio_status);

	for (gpio_id = 0; gpio_id < SOC_GPIO_NUM; gpio_id++) {
		if (gpio_hal_is_interrupt_triggered(gpio_id, &gpio_status)) {
			if (s_gpio_isr[gpio_id]) {
				GPIO_LOGV("gpio int: index:%d \r\n",gpio_id);
				s_gpio_isr[gpio_id](gpio_id);
			}
			bk_gpio_clear_interrupt(gpio_id);
		}
	}
}

bk_err_t gpio_backup_gpio_configs(void)
{
	return gpio_hal_bakup_configs(s_gpio_baked_regs);
}

bk_err_t gpio_restore_gpio_configs(void)
{
	return gpio_hal_restore_configs(s_gpio_baked_regs);
}

void gpio_dump_regs(bool config, bool overview, bool atpg)
{
#if CONFIG_GPIO_WAKEUP_DEBUG
	gpio_id_t gpio_id;

	GPIO_LOGV("%s[+]\r\n", __func__);

	if (config)
	{
		for(gpio_id = GPIO_0; gpio_id < SOC_GPIO_NUM; gpio_id++)
		{
			///gpio_struct_dump(gpio_id);
			GPIO_LOGV("gpio[%d]=0x%x\r\n", gpio_id, *(volatile uint32_t*)(GPIO_LL_REG_BASE + 4*gpio_id));
		}
	}

	if (overview)
	{
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_INTSTA_ADDR, REG_READ(IOMX_GPIO_INTSTA_ADDR));
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_INPUT_ADDR,  REG_READ(IOMX_GPIO_INPUT_ADDR));
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_OUTPUT_ADDR, REG_READ(IOMX_GPIO_OUTPUT_ADDR));
	}

	if (atpg)
	{
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_FUNC_O_ATPG_ADDR,   REG_READ(IOMX_GPIO_FUNC_O_ATPG_ADDR));
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_FUNC_IE_ATPG_ADDR,  REG_READ(IOMX_GPIO_FUNC_IE_ATPG_ADDR));
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_FUNC_OEN_ATPG_ADDR, REG_READ(IOMX_GPIO_FUNC_OEN_ATPG_ADDR));
	}

	GPIO_LOGV("%s[-]\r\n", __func__);
#endif
}

static void gpio_low_power_config(void);
bk_err_t gpio_enter_low_power(void *param)
{
#if 0
	GPIO_LOGV("%s[+]\r\n", __func__);

	gpio_dump_regs(true, true, true);

	gpio_backup_gpio_configs();

	iomx_dump_baked_regs();

	//NOTES:force disable all int to avoid config gpio caused error isr
	iomx_disable_all_interrupts();

	// setup gpio configs for sleep
#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
	gpio_keep_status_config();
#endif

#if CONFIG_GPIO_WAKEUP_SUPPORT
	gpio_wakeup_source_config();
#endif

	gpio_low_power_config();

	iomx_dump_regs(true, true, true);

	GPIO_LOGV("%s[-]\r\n", __func__);
#endif
	return BK_OK;
}

bk_err_t gpio_exit_low_power(void *param)
{
#if 0
	GPIO_LOGV("%s[+]\r\n", __func__);

	iomx_dump_regs(true, true, true);
	iomx_dump_baked_regs();

	iomx_disable_all_interrupts();

#if CONFIG_GPIO_WAKEUP_SUPPORT
#if CONFIG_ANA_GPIO
	// another workaround fix for unexpected gpio interrupt
	ana_gpio_clear_wakeup_source();
#endif
#endif

	iomx_restore_gpio_configs();

	iomx_dump_regs(true, true, true);
	iomx_dump_baked_regs();

	GPIO_LOGV("%s[-]\r\n", __func__);
#endif
	return BK_OK;
}

bk_err_t gpio_hal_switch_to_low_power_status(uint64_t skip_io)
{
	for (gpio_id_t i = GPIO_0; i < GPIO_NUM_MAX; i++)
	{
		if (skip_io & (0x1ULL << i))
			continue;
		bk_gpio_set_value(i, GPIO_REG_DEFAULT_VALUE);
	}

	return BK_OK;
}

static void gpio_low_power_config(void)
{
	uint64_t skip_io = 0;

#if CONFIG_GPIO_WAKEUP_SUPPORT
	skip_io |= s_gpio_is_setted_wake_status;
#endif

#if CONFIG_GPIO_KPSTAT_SUPPORT
	skip_io |= s_gpio_is_lowpower_keep_status;
#endif

	gpio_hal_switch_to_low_power_status(skip_io);
}

#if CONFIG_GPIO_WAKEUP_SUPPORT
gpio_id_t bk_gpio_get_wakeup_gpio_id(void)
{
	return s_gpio_wakeup_gpio_id;
}

static void gpio_record_wakeup_pin_id(void)
{
#if CONFIG_ANA_GPIO
	s_gpio_wakeup_gpio_id = ana_gpio_get_wakeup_pin();
#endif
}

bk_err_t gpio_enable_interrupt_mult_for_wake(void)
{
#if CONFIG_ANA_GPIO
	ana_gpio_config_wakeup_source(s_gpio_is_setted_wake_status);
#endif

	return gpio_enable_multi_interrupts(s_gpio_is_setted_wake_status);
}

static void gpio_set_wakeup_config(gpio_id_t gpio_id, gpio_int_type_t int_type)
{
	// setup gpio as input mode
	bk_iomx_set_value(gpio_id, GPIO_REG_DEFAULT_VALUE);
	bk_iomx_set_gpio_func(gpio_id, FUNC_CODE_INPUT);

	switch(int_type)
	{
		case GPIO_INT_TYPE_LOW_LEVEL:
		case GPIO_INT_TYPE_FALLING_EDGE:
			bk_gpio_pull_up(gpio_id);
			GPIO_LOGV("%s GPIO %d Pull_up!\r\n", __func__, gpio_id);
			break;
		case GPIO_INT_TYPE_HIGH_LEVEL:
		case GPIO_INT_TYPE_RISING_EDGE:
			bk_gpio_pull_down(gpio_id);
			GPIO_LOGV("%s GPIO %d Pull_down!\r\n", __func__, gpio_id);
			break;
		default:
			GPIO_LOGD("%s Please set fill in the mode correctly!\r\n", __func__);
			break;
	}

	bk_gpio_set_interrupt_type(gpio_id, int_type);
}

static void gpio_wakeup_source_config(void)
{
	uint32_t i = 0;

	GPIO_LOGV("%s[+]\r\n", __func__);

	s_gpio_is_setted_wake_status = 0;

	gpio_wakeup_t gpio_wakeup_map[] = GPIO_STATIC_WAKEUP_SOURCE_MAP;
	for (i = 0; i < sizeof(gpio_wakeup_map)/sizeof(gpio_wakeup_t); i++)
	{
		gpio_set_wakeup_config(gpio_wakeup_map[i].id, gpio_wakeup_map[i].int_type);
		s_gpio_is_setted_wake_status |= ((uint64_t)1 << gpio_wakeup_map[i].id);
	}

#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		if (s_gpio_dynamic_wakeup_source_map[i].id != GPIO_WAKE_SOURCE_IDLE_ID) {
			//maybe the PIN is re-used as SECOND_FUNCTION and GPIO,F.E:UART RXD re-uses as wakeup PIN
			gpio_set_wakeup_config(s_gpio_dynamic_wakeup_source_map[i].id, s_gpio_dynamic_wakeup_source_map[i].int_type);
			s_gpio_is_setted_wake_status |= ((uint64_t)1 << s_gpio_dynamic_wakeup_source_map[i].id);
		}
	}
#endif

	GPIO_LOGV("%s[-]set wake src h=0x%0x, l=0x%0x\r\n", __func__, (uint32_t)(s_gpio_is_setted_wake_status>>32), (uint32_t)s_gpio_is_setted_wake_status);
}

#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
bk_err_t bk_gpio_register_wakeup_source(gpio_id_t gpio_id, gpio_int_type_t int_type)
{
	uint32_t i = 0;

	// GPIO_RETURN_ON_INVALID_ID(gpio_id);
	// GPIO_RETURN_ON_INVALID_INT_TYPE_MODE(int_type);

	//search the same id and replace it.
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		if (s_gpio_dynamic_wakeup_source_map[i].id == gpio_id)
		{
			s_gpio_dynamic_wakeup_source_map[i].int_type = int_type;

			//NOTES:If doesn't set int type, exit lowpower, if the ISR has reported by rising/falling type
			//Then restore to level type, entry GPIO ISR caused the int status lost.
			bk_gpio_set_interrupt_type(gpio_id, int_type);
			//s_gpio_dynamic_wakeup_source_map[i].isr = isr;

			GPIO_LOGV("gpio=%d,int_type=%d replace previous wake src\r\n", gpio_id, int_type);
			return BK_OK;
		}
	}

	//serach the first idle id
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		if (s_gpio_dynamic_wakeup_source_map[i].id == GPIO_WAKE_SOURCE_IDLE_ID)
		{
			s_gpio_dynamic_wakeup_source_map[i].id = gpio_id;
			s_gpio_dynamic_wakeup_source_map[i].int_type = int_type;

			//NOTES:If doesn't set int type, exit lowpower, if the ISR has reported by rising/falling type
			//Then restore to level type, entry GPIO ISR caused the int status lost.
			bk_gpio_set_interrupt_type(gpio_id, int_type);
			//s_gpio_dynamic_wakeup_source_map[i].isr = isr;
			s_gpio_is_setted_wake_status |= ((uint64_t)1 << s_gpio_dynamic_wakeup_source_map[i].id);

			GPIO_LOGV("gpio=%d,int_type=%d register wake src\r\n", gpio_id, int_type);

			return BK_OK;
		}
	}

	GPIO_LOGE("too much(%d) GPIO is setted wake src\r\n", CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT);
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		GPIO_LOGE("gpio id:%d is using \r\n", s_gpio_dynamic_wakeup_source_map[i].id);
	}
	return BK_FAIL;
}

bk_err_t bk_gpio_unregister_wakeup_source(gpio_id_t gpio_id)
{
	uint32_t i = 0;

	// GPIO_RETURN_ON_INVALID_ID(gpio_id);

	/* search the same id and replace it.*/
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		if (s_gpio_dynamic_wakeup_source_map[i].id == gpio_id)
		{
			s_gpio_is_setted_wake_status &= ~(((uint64_t)1 << s_gpio_dynamic_wakeup_source_map[i].id));
			s_gpio_dynamic_wakeup_source_map[i].id = GPIO_WAKE_SOURCE_IDLE_ID;
			s_gpio_dynamic_wakeup_source_map[i].int_type = GPIO_INT_TYPE_MAX;
			//s_gpio_dynamic_wakeup_source_map[i].isr = NULL;

			/* Clear the hardware status during deregister */
			bk_gpio_disable_input(gpio_id);
			bk_gpio_disable_interrupt(gpio_id);

			GPIO_LOGV("%s[-]gpioid=%d\r\n", __func__, gpio_id);

			return BK_OK;
		}
	}

	GPIO_LOGE("gpio id:%d is not using \r\n", gpio_id);
	return BK_FAIL;
}

static void gpio_dynamic_wakeup_source_init(void)
{
	uint32_t i = 0;

	GPIO_LOGV("%s[+]gpio wakecnt=%d\r\n", __func__, CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT);
	//search the same id and replace it.
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		s_gpio_dynamic_wakeup_source_map[i].id = GPIO_WAKE_SOURCE_IDLE_ID;
	}

	GPIO_LOGV("%s[-]\r\n", __func__);
}
#else
bk_err_t bk_gpio_register_wakeup_source(gpio_id_t gpio_id,
                                                 gpio_int_type_t int_type)
{
	return BK_OK;
}

bk_err_t bk_gpio_unregister_wakeup_source(gpio_id_t gpio_id)
{
	return BK_OK;
}
#endif
#endif

#if CONFIG_GPIO_KPSTAT_SUPPORT
static void gpio_keep_status_init(void)
{
	//has configured in default map with static mode
#if CONFIG_GPIO_DEFAULT_SET_SUPPORT
	const gpio_default_map_t default_map[] = GPIO_DEFAULT_DEV_CONFIG;

	for (uint32_t i = 0; i < sizeof(default_map)/sizeof(gpio_default_map_t); i++)
	{
		//uses equal to avoid some guy maybe write other value
		if ((default_map[i].low_power_io_ctrl == GPIO_LOW_POWER_KEEP_INPUT_STATUS) ||
			(default_map[i].low_power_io_ctrl == GPIO_LOW_POWER_KEEP_OUTPUT_STATUS))
		{
			s_gpio_is_lowpower_keep_status |= ((uint64_t)0x1 << default_map[i].gpio_id);
		}
	}
#endif

#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
	GPIO_LOGV("%s[+]gpio wakecnt=%d\r\n", __func__, CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT);
	for (uint32_t i = 0; i < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; i++)
	{
		s_gpio_lowpower_keep_config[i].gpio_id = GPIO_LOWPOWER_KEEP_STATUS_IDLE_ID;
	}
#endif

	GPIO_LOGV("%s[-]\r\n", __func__);
}

#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
static void gpio_keep_status_config(void)
{
	uint32_t index;
	gpio_id_t gpio_id;
	gpio_config_t config;

	config.io_mode = GPIO_IO_DISABLE;
	config.pull_mode = GPIO_PULL_DISABLE;
	config.func_mode = GPIO_SECOND_FUNC_DISABLE;

	for(index = 0; index < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; index++)
	{
		if(s_gpio_is_lowpower_keep_status & ((uint64_t)1 << s_gpio_lowpower_keep_config[index].gpio_id)) {
			gpio_id = s_gpio_lowpower_keep_config[index].gpio_id;
			config.io_mode = s_gpio_lowpower_keep_config[index].config.io_mode;
			config.pull_mode = s_gpio_lowpower_keep_config[index].config.pull_mode;
			config.func_mode = s_gpio_lowpower_keep_config[index].config.func_mode;
			bk_gpio_set_config(gpio_id, &config);
			BK_LOGD(NULL, "set config %d %d %d %x\r\n", config.io_mode, config.pull_mode, config.func_mode, bk_gpio_get_value(gpio_id));
		}
	}
}

bk_err_t bk_gpio_register_lowpower_keep_status(gpio_id_t gpio_id,
                                                 const gpio_config_t *config)
{
	uint32_t i = 0;
	GPIO_LOGD("[+]gpio=%d io_mode=%d pull_mode=%d func_mode=%d\r\n",
		gpio_id, config->io_mode, config->pull_mode, config->func_mode);

#if CONFIG_GPIO_RETENTION_SUPPORT
	if (config->io_mode == GPIO_OUTPUT_ENABLE && config->pull_mode == GPIO_PULL_UP_EN)
	{
		gpio_retention_map_set(gpio_id, GPIO_OUTPUT_STATE_HIGH);
	}
	else if (config->io_mode == GPIO_OUTPUT_ENABLE && config->pull_mode == GPIO_PULL_DOWN_EN)
	{
		gpio_retention_map_set(gpio_id, GPIO_OUTPUT_STATE_LOW);
	}
#endif

	//search the same id and replace it.
	for(i = 0; i < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; i++)
	{
		if(s_gpio_lowpower_keep_config[i].gpio_id == gpio_id) {
			s_gpio_lowpower_keep_config[i].config.io_mode = config->io_mode;
			s_gpio_lowpower_keep_config[i].config.pull_mode = config->pull_mode;
			s_gpio_lowpower_keep_config[i].config.func_mode = config->func_mode;
			s_gpio_is_lowpower_keep_status |= ((uint64_t)1 << gpio_id);

			GPIO_LOGV("gpio=%d io_mode=%d pull_mode=%d func_mode=%d\r\n",
				gpio_id, config->io_mode, config->pull_mode, config->func_mode);
			return BK_OK;
		}
	}

	//serach the first idle id
	for(i = 0; i < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; i++)
	{
		if(s_gpio_lowpower_keep_config[i].gpio_id == GPIO_LOWPOWER_KEEP_STATUS_IDLE_ID) {
			s_gpio_lowpower_keep_config[i].gpio_id = gpio_id;
			s_gpio_lowpower_keep_config[i].config.io_mode = config->io_mode;
			s_gpio_lowpower_keep_config[i].config.pull_mode = config->pull_mode;
			s_gpio_lowpower_keep_config[i].config.func_mode = config->func_mode;
			s_gpio_is_lowpower_keep_status |= ((uint64_t)1 << gpio_id);

			GPIO_LOGD("gpio=%d io_mode=%d pull_mode=%d func_mode=%d\r\n",
				gpio_id, config->io_mode, config->pull_mode, config->func_mode);

			return BK_OK;
		}
	}

	GPIO_LOGE("too much(%d) GPIO is setted keep status\r\n", CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT);
	for(i = 0; i < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; i++)
	{
		GPIO_LOGE("gpio id:%d is using \r\n", s_gpio_lowpower_keep_config[i].gpio_id);
	}
	return BK_FAIL;
}

bk_err_t bk_gpio_unregister_lowpower_keep_status(gpio_id_t gpio_id)
{
	gpio_config_t config;
	uint32_t i = 0;

	config.io_mode = GPIO_IO_DISABLE;
	config.pull_mode = GPIO_PULL_DISABLE;
	config.func_mode = GPIO_SECOND_FUNC_DISABLE;

	/* search the same id and replace it.*/
	for(i = 0; i < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; i++)
	{
		if(s_gpio_lowpower_keep_config[i].gpio_id == gpio_id) {
			s_gpio_lowpower_keep_config[i].gpio_id = GPIO_LOWPOWER_KEEP_STATUS_IDLE_ID;
			s_gpio_is_lowpower_keep_status &= ~(((uint64_t)1 << gpio_id));
			bk_gpio_set_config(gpio_id, &config);
			s_gpio_lowpower_keep_config[i].config.io_mode = config.io_mode;
			s_gpio_lowpower_keep_config[i].config.pull_mode = config.pull_mode;
			s_gpio_lowpower_keep_config[i].config.func_mode = config.func_mode;

			GPIO_LOGV("%s[-]gpioid=%d\r\n", __func__, gpio_id);

			return BK_OK;
		}
	}

	GPIO_LOGE("gpio id:%d is not using \r\n", gpio_id);
	return BK_FAIL;
}
#endif
#endif

#if CONFIG_GPIO_DEFAULT_SET_SUPPORT
static void gpio_default_map_init(void)
{
#if 0
	const gpio_default_map_t default_map[] = GPIO_DEFAULT_DEV_CONFIG;
	gpio_id_t gpio_id;
	IOMX_CODE_T code;

	for (int i = 0; i < sizeof(default_map)/sizeof(gpio_default_map_t); i++)
	{
		gpio_id = default_map[i].gpio_id;

		if (default_map[i].gpio_skip == GPIO_INIT_SKIP){
			GPIO_LOGV("skipping gpio_id : %d", gpio_id);
			continue;
		}

		bk_iomx_disable_pull(gpio_id);
		bk_iomx_disable_interrupt(gpio_id);

		GPIO_LOGV("gpio_id: %d, second_func_en:%d, second_func_dev %d, low_power_io_ctrl:%d", gpio_id,
					default_map[i].second_func_en, default_map[i].second_func_dev,
					default_map[i].low_power_io_ctrl);

		GPIO_LOGV("int_en: %d, int_type:%d \r\n",
				default_map[i].int_en, default_map[i].int_type);

		//function mode
		if (default_map[i].second_func_en) {
			code = contert_gpio_dev_to_iomx_code(default_map[i].second_func_dev);
			bk_iomx_set_gpio_func(gpio_id, code);
		}

		//low power
		if (default_map[i].low_power_io_ctrl == GPIO_LOW_POWER_KEEP_OUTPUT_STATUS)
			bk_iomx_set_gpio_func(gpio_id, FUNC_CODE_OUTPUT);
		else if (default_map[i].low_power_io_ctrl == GPIO_LOW_POWER_KEEP_INPUT_STATUS)
			bk_iomx_set_gpio_func(gpio_id, FUNC_CODE_INPUT);

		//io mode
		switch(default_map[i].io_mode)
		{
			case GPIO_IO_DISABLE:
				bk_iomx_set_gpio_func(gpio_id, FUNC_CODE_HIGH_Z);
				break;
			case GPIO_OUTPUT_ENABLE:
				//NOTES:special combine use it with pull mode
				if (default_map[i].pull_mode == GPIO_PULL_DOWN_EN)
					bk_iomx_output_low(gpio_id);
				if (default_map[i].pull_mode == GPIO_PULL_UP_EN)
					bk_iomx_output_high(gpio_id);
				break;
			case GPIO_INPUT_ENABLE:
				bk_iomx_set_gpio_func(gpio_id, FUNC_CODE_INPUT);
				break;
			default:
				break;
		}

		//pull mode
		switch(default_map[i].pull_mode)
		{
			case GPIO_PULL_DISABLE:
				bk_iomx_disable_pull(gpio_id);
				break;
			case GPIO_PULL_DOWN_EN:
				bk_iomx_pull_down(gpio_id);
				break;
			case GPIO_PULL_UP_EN:
				bk_iomx_pull_up(gpio_id);
				break;
			default:
				break;
		}

		//interrupt
		if (default_map[i].int_en) {
			bk_iomx_set_interrupt_type(gpio_id, default_map[i].int_type);

			for (volatile int i = 0; i < 1000; i++);    //Before enable the interrupt,wait for the internal stability of the chip
			bk_iomx_enable_interrupt(gpio_id);
		}

		bk_iomx_clear_interrupt(gpio_id);

		//driver_capacity
		bk_iomx_set_capacity(gpio_id, default_map[i].driver_capacity);
	}
#endif
}
#endif

#if CONFIG_GPIO_SIMULATE_UART_WRITE
/**
 * @brief	  Uses specifies GPIO to simulate UART write data
 *
 * This API Uses specifies GPIO to simulate UART write data:
 *	 - Uses CPU poll wait to do delay, so it blocks CPU.
 *	 - The caller should confirm the specifies GPIO is not used by other APP.
 *
 * @param *buff  Which buffers will be write with GPIO.
 * @param len    How many bytes data will be wrote.
 * @param gpio_id  Which GPIO will be simulated as UART write data.
 * @param div    Baud rate == 1Mbps/(1+div)
 *
 * @attention 1. As this function just simulate uart write, it blocks the CPU,
 *               so please don't write too much data.
 *
 * @return
 */
void gpio_simulate_uart_write(unsigned char *buff, uint32_t len, gpio_id_t gpio_id, uint32_t div)
{
	volatile unsigned char c, n;
	UINT32 param;
	uint32_t div_cnt = div+1;

	BK_LOG_ON_ERR(bk_gpio_disable_input(gpio_id));
	BK_LOG_ON_ERR(bk_gpio_enable_output(gpio_id));

	bk_gpio_set_output_high(gpio_id);
	bk_delay_us(div_cnt);

	while (len--) {
		//in while loop, to avoid disable IRQ too much time, release it if finish one byte.
		GLOBAL_INT_DECLARATION();
		GLOBAL_INT_DISABLE();

		//UART start bit
		bk_gpio_set_output_low(gpio_id);
		bk_delay_us(div_cnt);

		//char value
		c = *buff++;
		n = 8;
		while (n--) {
			param = c & 0x01;
			if (param) {
				bk_gpio_set_output_high(gpio_id);
			} else {
				bk_gpio_set_output_low(gpio_id);
			}

			bk_delay_us(div_cnt);
			c >>= 1;
		}

		//UART stop bit
		bk_gpio_set_output_high(gpio_id);
		bk_delay_us(div_cnt);

		GLOBAL_INT_RESTORE();
	}
}
#endif
