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

#include "ipi_driver.h"
#include <components/log.h>
#include <common/bk_assert.h>
#include <driver/int.h>
#include "bk_arm_arch.h"
#include <os/os.h>
#include "sys_driver.h"
#include "ipi_hal.h"

#define IPI_TAG "ipi"
#define IPI_LOGI(...) BK_LOGI(IPI_TAG, ##__VA_ARGS__)
#define IPI_LOGW(...) BK_LOGW(IPI_TAG, ##__VA_ARGS__)
#define IPI_LOGE(...) BK_LOGE(IPI_TAG, ##__VA_ARGS__)
#define IPI_LOGD(...) BK_LOGD(IPI_TAG, ##__VA_ARGS__)
#define IPI_LOGV(...) BK_LOGV(IPI_TAG, ##__VA_ARGS__)

typedef struct {
	ipi_domain_callback_t callback;
	void *param;
} ipi_domain_callback_info_t;

/* Driver state */
static bool s_ipi_driver_init = false;
static ipi_domain_callback_info_t s_ipi_domain_callbacks[IPI_DOMAIN_MAX] = {0};
static ipi_hal_t s_ipi_hal;

static void bk_ipi_isr_dispatch(void);

/**
 * @brief Validate core ID
 */
static inline bool ipi_is_valid_core(ipi_core_id_t core_id)
{
	return (core_id < IPI_CORE_MAX);
}

bk_err_t bk_ipi_driver_init(void)
{
	if (s_ipi_driver_init) {
		return BK_OK;
	}

	BK_LOG_ON_ERR(ipi_hal_init(&s_ipi_hal));

	/* Initialize callbacks */
	for (int i = 0; i < IPI_DOMAIN_MAX; i++) {
		s_ipi_domain_callbacks[i].callback = NULL;
		s_ipi_domain_callbacks[i].param = NULL;
	}

	/* Clear all interrupts */
	for (int i = 0; i < IPI_CORE_MAX; i++) {
		ipi_hal_clear(&s_ipi_hal, i);
	}

	/* Disable all interrupts by default */
	{
		uint32_t int_en = ipi_ll_get_int_reg(s_ipi_hal.hw);
		int_en &= ~((1U << IPI_AP_CORE0) | (1U << IPI_AP_CORE1));
		ipi_ll_set_int_reg(s_ipi_hal.hw, int_en);
	}

	/* Register IPI interrupt service routine
	 * This registers bk_ipi_isr_dispatch as the ISR handler for IPI interrupts.
	 * When an IPI interrupt occurs, the interrupt controller will call this function
	 * to dispatch the interrupt to the appropriate core's callback.
	 */
	bk_int_isr_register(INT_SRC_IPI, bk_ipi_isr_dispatch, NULL);

	/*
	 * Keep AP-side IPI interrupt routing enabled by default.
	 * CPU2: AP_CORE0, CPU3: AP_CORE1.
	 */
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_IPI, 1);
	sys_drv_set_int_en(CPU3_CORE_ID, INT_SRC_IPI, 1);

	/*
	 * Default enable AP-side IPI channels (bit2/bit3) so CP->AP IPI can be received
	 * without requiring AP CLI init.
	 */
	{
		uint32_t int_en = ipi_ll_get_int_reg(s_ipi_hal.hw);
		int_en |= (1U << IPI_AP_CORE0);
		int_en |= (1U << IPI_AP_CORE1);
		ipi_ll_set_int_reg(s_ipi_hal.hw, int_en);
	}
    
#if CONFIG_CLI && CONFIG_IPI_TEST
	int bk_ipi_register_cli_test_feature(void);
	bk_ipi_register_cli_test_feature();
#endif

	s_ipi_driver_init = true;

	return BK_OK;
}

bk_err_t bk_ipi_driver_deinit(void)
{
	if (!s_ipi_driver_init) {
		return BK_OK;
	}

	/* Disable all interrupts */
	{
		uint32_t int_en = ipi_ll_get_int_reg(s_ipi_hal.hw);
		int_en &= ~((1U << IPI_AP_CORE0) | (1U << IPI_AP_CORE1));
		ipi_ll_set_int_reg(s_ipi_hal.hw, int_en);
	}

	/* Clear all callbacks */
	for (int i = 0; i < IPI_DOMAIN_MAX; i++) {
		s_ipi_domain_callbacks[i].callback = NULL;
		s_ipi_domain_callbacks[i].param = NULL;
	}

	s_ipi_driver_init = false;

	return BK_OK;
}

static bk_err_t ipi_send_value(ipi_core_id_t core_id, uint32_t value)
{
	if (!s_ipi_driver_init) {
		IPI_LOGE("IPI driver not initialized\r\n");
		return BK_FAIL;
	}

	if (!ipi_is_valid_core(core_id)) {
		IPI_LOGE("Invalid core ID: %d\r\n", core_id);
		return BK_FAIL;
	}

	IPI_LOGD("IPI send: from_cpu=%u -> core_id=%d, value=0x%08X\r\n",
	         (unsigned)rtos_get_core_id(), core_id, value);
	BK_LOG_ON_ERR(ipi_hal_send(&s_ipi_hal, core_id, value));

	return BK_OK;
}

bk_err_t bk_ipi_send_domain(ipi_core_id_t core_id, ipi_domain_t domain, uint8_t event, uint16_t payload)
{
	uint32_t src_cpu;
	uint32_t value;

	if ((domain <= IPI_DOMAIN_RESERVED) || (domain >= IPI_DOMAIN_MAX)) {
		IPI_LOGE("Invalid IPI domain: %d\r\n", domain);
		return BK_FAIL;
	}

	src_cpu = rtos_get_core_id() & 0xF;
	value = IPI_VALUE_MAKE(src_cpu, domain, event, payload);

	return ipi_send_value(core_id, value);
}

uint32_t bk_ipi_get_status(ipi_core_id_t core_id)
{
	if (!s_ipi_driver_init) {
		return 0;
	}

	if (!ipi_is_valid_core(core_id)) {
		return 0;
	}

	return ipi_hal_get_status(&s_ipi_hal, core_id);
}

uint32_t bk_ipi_get_all_status(void)
{
	if (!s_ipi_driver_init) {
		return 0;
	}

	return ipi_hal_get_all_status(&s_ipi_hal);
}

bk_err_t bk_ipi_clear(ipi_core_id_t core_id)
{
	if (!s_ipi_driver_init) {
		return BK_FAIL;
	}

	if (!ipi_is_valid_core(core_id)) {
		return BK_FAIL;
	}

	return ipi_hal_clear(&s_ipi_hal, core_id);
}

bk_err_t bk_ipi_enable(ipi_core_id_t core_id)
{
	if (!s_ipi_driver_init) {
		IPI_LOGE("IPI driver not initialized\r\n");
		return BK_FAIL;
	}

	if (!ipi_is_valid_core(core_id)) {
		IPI_LOGE("Invalid core ID: %d\r\n", core_id);
		return BK_FAIL;
	}

	return ipi_hal_enable_channel(&s_ipi_hal, core_id);
}

bk_err_t bk_ipi_disable(ipi_core_id_t core_id)
{
	if (!s_ipi_driver_init) {
		IPI_LOGE("IPI driver not initialized\r\n");
		return BK_FAIL;
	}

	if (!ipi_is_valid_core(core_id)) {
		IPI_LOGE("Invalid core ID: %d\r\n", core_id);
		return BK_FAIL;
	}

	return ipi_hal_disable_channel(&s_ipi_hal, core_id);
}

bk_err_t bk_ipi_register_domain_callback(ipi_domain_t domain, ipi_domain_callback_t callback, void *param)
{
	if (!s_ipi_driver_init) {
		IPI_LOGE("IPI driver not initialized\r\n");
		return BK_FAIL;
	}

	if ((domain <= IPI_DOMAIN_RESERVED) || (domain >= IPI_DOMAIN_MAX) || (callback == NULL)) {
		IPI_LOGE("Invalid IPI domain callback: domain=%d callback=0x%p\r\n", domain, callback);
		return BK_FAIL;
	}

	s_ipi_domain_callbacks[domain].callback = callback;
	s_ipi_domain_callbacks[domain].param = param;

	return BK_OK;
}

bk_err_t bk_ipi_unregister_domain_callback(ipi_domain_t domain)
{
	if (!s_ipi_driver_init) {
		IPI_LOGE("IPI driver not initialized\r\n");
		return BK_FAIL;
	}

	if ((domain <= IPI_DOMAIN_RESERVED) || (domain >= IPI_DOMAIN_MAX)) {
		IPI_LOGE("Invalid IPI domain: %d\r\n", domain);
		return BK_FAIL;
	}

	s_ipi_domain_callbacks[domain].callback = NULL;
	s_ipi_domain_callbacks[domain].param = NULL;

	return BK_OK;
}

uint32_t bk_ipi_get_device_status(void)
{
	if (!s_ipi_driver_init) {
		return 0;
	}

	return ipi_hal_get_device_status(&s_ipi_hal);
}

static void bk_ipi_isr_dispatch(void)
{
	uint32_t int_status;
	uint32_t ipig_val;
	ipi_core_id_t core_id;

	if (!s_ipi_driver_init) {
		return;
	}

	/* Read interrupt status */
	int_status = ipi_hal_get_all_status(&s_ipi_hal);

	/* Check each core for pending interrupts */
	for (core_id = 0; core_id < IPI_CORE_MAX; core_id++) {
		if ((int_status >> core_id) & 0x1) {
			/* Read IPIG value to get the value field */
			ipig_val = ipi_ll_unpack_ipig(ipi_ll_read_ipig(s_ipi_hal.hw, core_id));

			IPI_LOGV("IPI recv: on_cpu=%u < src_cpu=%u, value=0x%08X\r\n",
			         (unsigned)rtos_get_core_id(),
			         (unsigned)IPI_VALUE_GET_SRC(ipig_val),
			         (unsigned)ipig_val);

			/* All public IPI users are domain encoded. */
			uint8_t domain = IPI_VALUE_GET_DOMAIN(ipig_val);
			if ((domain > IPI_DOMAIN_RESERVED) &&
			    (domain < IPI_DOMAIN_MAX) &&
			    s_ipi_domain_callbacks[domain].callback) {
				s_ipi_domain_callbacks[domain].callback(core_id, ipig_val,
					IPI_VALUE_GET_SRC(ipig_val), IPI_VALUE_GET_EVENT(ipig_val),
					IPI_VALUE_GET_PAYLOAD(ipig_val), s_ipi_domain_callbacks[domain].param);
			}

			/* Clear the interrupt */
			ipi_hal_clear(&s_ipi_hal, core_id);
		}
	}
}

#if CONFIG_IPI_DUMP
void bk_ipi_dump_info(void)
{
	IPI_LOGI("=== IPI Domain Callbacks Dump ===\r\n");
	IPI_LOGI("Driver initialized: %s\r\n", s_ipi_driver_init ? "Yes" : "No");

	if (!s_ipi_driver_init) {
		IPI_LOGI("Driver not initialized, no callbacks to dump\r\n");
		return;
	}

	for (uint32_t domain = 1; domain < IPI_DOMAIN_MAX; domain++) {
		if (s_ipi_domain_callbacks[domain].callback) {
			IPI_LOGI("Domain[%u]: callback=0x%p, param=0x%p\r\n",
			         domain,
			         s_ipi_domain_callbacks[domain].callback,
			         s_ipi_domain_callbacks[domain].param);
		} else {
			IPI_LOGI("Domain[%u]: callback=NULL\r\n", domain);
		}
	}

	IPI_LOGI("=== End of IPI Domain Callbacks Dump ===\r\n");
}
#endif

