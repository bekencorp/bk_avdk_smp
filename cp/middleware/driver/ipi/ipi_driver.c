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

#define IPI_TAG "ipi"
#define IPI_LOGI(...) BK_LOGI(IPI_TAG, ##__VA_ARGS__)
#define IPI_LOGW(...) BK_LOGW(IPI_TAG, ##__VA_ARGS__)
#define IPI_LOGE(...) BK_LOGE(IPI_TAG, ##__VA_ARGS__)
#define IPI_LOGD(...) BK_LOGD(IPI_TAG, ##__VA_ARGS__)

/* IPI register base address */
#define IPI_REG_BASE                (0x458F0000)

/* Register offsets */
#define IPI_REG_DEVID               (0x0)   /* Device ID */
#define IPI_REG_VERID               (0x1)   /* Version ID */
#define IPI_REG_CLKRST              (0x2)   /* Clock and Reset Control */
#define IPI_REG_STATE               (0x3)   /* Device Status */
#define IPI_REG_IPIG0               (0x4)   /* IPI Generate for CP_CORE0 */
#define IPI_REG_IPIG1               (0x5)   /* IPI Generate for CP_CORE1 */
#define IPI_REG_IPIG2               (0x6)   /* IPI Generate for AP_CORE0 */
#define IPI_REG_IPIG3               (0x7)   /* IPI Generate for AP_CORE1 */
#define IPI_REG_IPIG4               (0x8)   /* IPI Generate for DSP_CORE (DSP) */
#define IPI_REG_IPIC0               (0xC)   /* IPI Clear for CP_CORE0 */
#define IPI_REG_IPIC1               (0xD)   /* IPI Clear for CP_CORE1 */
#define IPI_REG_IPIC2               (0xE)   /* IPI Clear for AP_CORE0 */
#define IPI_REG_IPIC3               (0xF)   /* IPI Clear for AP_CORE1 */
#define IPI_REG_IPIC4               (0x10)  /* IPI Clear for DSP_CORE (DSP) */
#define IPI_REG_INT                 (0x14)  /* Interrupt Status and Enable */

/* Bit field definitions */
#define IPI_CLKRST_CLKG_BYPASS_BIT  (1)     /* Bit 1: Hclk gating bypass */
#define IPI_CLKRST_SOFT_RESET_BIT   (0)     /* Bit 0: Soft reset */

/* IPIG register bit fields */
#define IPI_IPIG_VAL_MASK           (0xFFFFFFFE)  /* Bits 31:1 - Value field */
#define IPI_IPIG_VAL_SHIFT          (1)
#define IPI_IPIG_SET_BIT            (0x1)         /* Bit 0 - Set interrupt */

/* IPI_INT register bit fields */
#define IPI_INT_STA_MASK            (0x1F00)      /* Bits 12:8 - Interrupt status */
#define IPI_INT_STA_SHIFT          (8)
#define IPI_INT_EN_MASK             (0x1F)        /* Bits 4:0 - Interrupt enable */
#define IPI_INT_EN_SHIFT            (0)


/* Callback structure */
typedef struct {
	ipi_callback_t callback;
	void *param;
} ipi_callback_info_t;

/* Driver state */
static bool s_ipi_driver_init = false;
static ipi_callback_info_t s_ipi_callbacks[IPI_CORE_MAX] = {0};

/* Register access macros */
#define IPI_REG_RD32(offset)        (*((volatile uint32_t *)(IPI_REG_BASE + (offset) * 4)))
#define IPI_REG_WR32(offset, val)   (*((volatile uint32_t *)(IPI_REG_BASE + (offset) * 4)) = (val))

/**
 * @brief Get IPIG register offset for core
 */
static inline uint32_t ipi_get_ipig_offset(ipi_core_id_t core_id)
{
	return IPI_REG_IPIG0 + core_id;
}

/**
 * @brief Get IPIC register offset for core
 */
static inline uint32_t ipi_get_ipic_offset(ipi_core_id_t core_id)
{
	return IPI_REG_IPIC0 + core_id;
}

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

	/* Initialize callbacks */
	for (int i = 0; i < IPI_CORE_MAX; i++) {
		s_ipi_callbacks[i].callback = NULL;
		s_ipi_callbacks[i].param = NULL;
	}

	/* Clear all interrupts */
	for (int i = 0; i < IPI_CORE_MAX; i++) {
		uint32_t ipig_val = IPI_REG_RD32(ipi_get_ipig_offset(i));
		IPI_REG_WR32(ipi_get_ipic_offset(i), ipig_val);
	}

	/* Disable all interrupts by default */
	IPI_REG_WR32(IPI_REG_INT, 0x0);

	/* Register IPI interrupt service routine
	 * This registers bk_ipi_isr_dispatch as the ISR handler for IPI interrupts.
	 * When an IPI interrupt occurs, the interrupt controller will call this function
	 * to dispatch the interrupt to the appropriate core's callback.
	 */
	bk_int_isr_register(INT_SRC_IPI, bk_ipi_isr_dispatch, NULL);

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
	IPI_REG_WR32(IPI_REG_INT, 0x0);

	/* Clear all callbacks */
	for (int i = 0; i < IPI_CORE_MAX; i++) {
		s_ipi_callbacks[i].callback = NULL;
		s_ipi_callbacks[i].param = NULL;
	}

	s_ipi_driver_init = false;

	return BK_OK;
}

bk_err_t bk_ipi_send(ipi_core_id_t core_id, uint32_t value)
{
	uint32_t ipig_offset;
	uint32_t reg_val;

	if (!s_ipi_driver_init) {
		IPI_LOGE("IPI driver not initialized\r\n");
		return BK_FAIL;
	}

	if (!ipi_is_valid_core(core_id)) {
		IPI_LOGE("Invalid core ID: %d\r\n", core_id);
		return BK_FAIL;
	}

	/* Prepare register value: value in bits 31:1, set bit 0 to 1 to generate interrupt */
	reg_val = ((value << IPI_IPIG_VAL_SHIFT) & IPI_IPIG_VAL_MASK) | IPI_IPIG_SET_BIT;

	ipig_offset = ipi_get_ipig_offset(core_id);
	IPI_REG_WR32(ipig_offset, reg_val);

	return BK_OK;
}

uint32_t bk_ipi_get_status(ipi_core_id_t core_id)
{
	uint32_t int_status;

	if (!s_ipi_driver_init) {
		return 0;
	}

	if (!ipi_is_valid_core(core_id)) {
		return 0;
	}

	int_status = IPI_REG_RD32(IPI_REG_INT);
	int_status = (int_status >> IPI_INT_STA_SHIFT) & IPI_INT_EN_MASK;

	return (int_status >> core_id) & 0x1;
}

uint32_t bk_ipi_get_all_status(void)
{
	uint32_t int_status;

	if (!s_ipi_driver_init) {
		return 0;
	}

	int_status = IPI_REG_RD32(IPI_REG_INT);
	return (int_status >> IPI_INT_STA_SHIFT) & IPI_INT_EN_MASK;
}

bk_err_t bk_ipi_clear(ipi_core_id_t core_id)
{
	uint32_t ipig_offset, ipic_offset;
	uint32_t ipig_val;

	if (!s_ipi_driver_init) {
		return BK_FAIL;
	}

	if (!ipi_is_valid_core(core_id)) {
		return BK_FAIL;
	}

	/* Read current IPIG value and write it to IPIC to clear interrupt */
	ipig_offset = ipi_get_ipig_offset(core_id);
	ipic_offset = ipi_get_ipic_offset(core_id);
	ipig_val = IPI_REG_RD32(ipig_offset);
	IPI_REG_WR32(ipic_offset, ipig_val);

	return BK_OK;
}

bk_err_t bk_ipi_enable(ipi_core_id_t core_id)
{
	uint32_t int_en;

	if (!s_ipi_driver_init) {
		IPI_LOGE("IPI driver not initialized\r\n");
		return BK_FAIL;
	}

	if (!ipi_is_valid_core(core_id)) {
		IPI_LOGE("Invalid core ID: %d\r\n", core_id);
		return BK_FAIL;
	}

	/* Read current interrupt enable register */
	int_en = IPI_REG_RD32(IPI_REG_INT);
	/* Set the corresponding bit in the enable field (bits 4:0) */
	int_en |= (1 << core_id);
	IPI_REG_WR32(IPI_REG_INT, int_en);

	return BK_OK;
}

bk_err_t bk_ipi_disable(ipi_core_id_t core_id)
{
	uint32_t int_en;

	if (!s_ipi_driver_init) {
		IPI_LOGE("IPI driver not initialized\r\n");
		return BK_FAIL;
	}

	if (!ipi_is_valid_core(core_id)) {
		IPI_LOGE("Invalid core ID: %d\r\n", core_id);
		return BK_FAIL;
	}

	/* Read current interrupt enable register */
	int_en = IPI_REG_RD32(IPI_REG_INT);
	/* Clear the corresponding bit in the enable field (bits 4:0) */
	int_en &= ~(1 << core_id);
	IPI_REG_WR32(IPI_REG_INT, int_en);

	return BK_OK;
}

bk_err_t bk_ipi_register_callback(ipi_core_id_t core_id, ipi_callback_t callback, void *param)
{
	if (!s_ipi_driver_init) {
		IPI_LOGE("IPI driver not initialized\r\n");
		return BK_FAIL;
	}

	if (!ipi_is_valid_core(core_id)) {
		IPI_LOGE("Invalid core ID: %d\r\n", core_id);
		return BK_FAIL;
	}

	s_ipi_callbacks[core_id].callback = callback;
	s_ipi_callbacks[core_id].param = param;

	return BK_OK;
}

bk_err_t bk_ipi_unregister_callback(ipi_core_id_t core_id)
{
	if (!s_ipi_driver_init) {
		IPI_LOGE("IPI driver not initialized\r\n");
		return BK_FAIL;
	}

	if (!ipi_is_valid_core(core_id)) {
		IPI_LOGE("Invalid core ID: %d\r\n", core_id);
		return BK_FAIL;
	}

	s_ipi_callbacks[core_id].callback = NULL;
	s_ipi_callbacks[core_id].param = NULL;

	return BK_OK;
}

uint32_t bk_ipi_get_device_status(void)
{
	if (!s_ipi_driver_init) {
		return 0;
	}

	return IPI_REG_RD32(IPI_REG_STATE);
}

void bk_ipi_isr_dispatch(void)
{
	uint32_t int_status;
	uint32_t ipig_val;
	ipi_core_id_t core_id;

	if (!s_ipi_driver_init) {
		return;
	}

	/* Read interrupt status */
	int_status = IPI_REG_RD32(IPI_REG_INT);
	int_status = (int_status >> IPI_INT_STA_SHIFT) & IPI_INT_EN_MASK;

	/* Check each core for pending interrupts */
	for (core_id = 0; core_id < IPI_CORE_MAX; core_id++) {
		if ((int_status >> core_id) & 0x1) {
			/* Read IPIG value to get the value field */
			ipig_val = IPI_REG_RD32(ipi_get_ipig_offset(core_id));
			ipig_val = (ipig_val & IPI_IPIG_VAL_MASK) >> IPI_IPIG_VAL_SHIFT;

			/* Call registered callback if available */
			if (s_ipi_callbacks[core_id].callback) {
				s_ipi_callbacks[core_id].callback(core_id, ipig_val, s_ipi_callbacks[core_id].param);
			}

		/* Clear the interrupt */
		bk_ipi_clear(core_id);
		}
	}
}

void bk_ipi_dump_info(void)
{
	const char *core_names[] = {
		"IPI_CP_CORE0",
		"IPI_CP_CORE1",
		"IPI_AP_CORE0",
		"IPI_AP_CORE1",
		"IPI_DSP_CORE"
	};

	ipi_core_id_t core_id;

	IPI_LOGI("=== IPI Callbacks Dump ===\r\n");
	IPI_LOGI("Driver initialized: %s\r\n", s_ipi_driver_init ? "Yes" : "No");

	if (!s_ipi_driver_init) {
		IPI_LOGI("Driver not initialized, no callbacks to dump\r\n");
		return;
	}

	for (core_id = 0; core_id < IPI_CORE_MAX; core_id++) {
		const char *core_name = (core_id < sizeof(core_names) / sizeof(core_names[0])) 
		                         ? core_names[core_id] : "UNKNOWN";
		
		if (s_ipi_callbacks[core_id].callback) {
			IPI_LOGI("Core[%d] %s: callback=0x%p, param=0x%p\r\n",
			         core_id,
			         core_name,
			         s_ipi_callbacks[core_id].callback,
			         s_ipi_callbacks[core_id].param);
		} else {
			IPI_LOGI("Core[%d] %s: callback=NULL\r\n", core_id, core_name);
		}
	}

	IPI_LOGI("=== End of IPI Callbacks Dump ===\r\n");
}

