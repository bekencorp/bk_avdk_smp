/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cmsis.h"
#include "target_cfg.h"
#include "tfm_hal_platform.h"
#include "tfm_plat_defs.h"
#include "uart_stdout.h"
#include "core_star.h"
#include "security.h"
#include "prro.h"
#include "tfm_flash_partition.h"
#include "tfm_hal_ppc.h"
#include "tfm_builtin_key_loader.h"
#include "hal_hw_fih.h"
#include "hal_sw_fih.h"
#include "driver/flash.h"
#include "os/mem.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "device_cfg.h"
#if CONFIG_SLEEP_RETENTION_NSC
#include "tfm_sleep_context.h"
#endif

/* Hardware register base addresses */
#define DMA0_BASE_ADDR             0x45020000
#define DMA1_BASE_ADDR             0x4C300000
#define PPRO_BASE_ADDR             0x41040000

/* DMA register offsets */
#define DMA_CTRL_REG_OFFSET        0x02
#define DMA_ENABLE_REG_OFFSET      0x04
#define DMA_MASK_REG_OFFSET        0x05
#define DMA0_INT_ALLOC_REG_OFFSET  0x0A
#define DMA1_INT_ALLOC_REG_OFFSET  0x0B
#define DMA0_INT_ALLOC_VALUE       0x0000AA00
#define DMA1_INT_ALLOC_VALUE       0x00000000

/* PPRO register offsets */
#define PPRO_RESET_REG_OFFSET      0x02
#define PPRO_GPIO_NONSEC0_OFFSET  0x04
#define PPRO_CONFIG_REG6_OFFSET   0x06
#define PPRO_CONFIG_REG7_OFFSET   0x07
#define PPRO_CONFIG_REG8_OFFSET   0x08
#define PPRO_CONFIG_REG12_OFFSET  0x0C
#define PPRO_CONFIG_REG13_OFFSET  0x0D
#define PPRO_CONFIG_REG14_OFFSET  0x0E
#define PPRO_CONFIG_REG15_OFFSET  0x0F

#define TAG "platform"

extern uint32_t sys_is_enable_fast_boot(void);
extern uint32_t sys_is_running_from_deep_sleep(void);
extern void tfm_deepsleep_fastboot_save_xip(void);
int bk_flash_set_dbus_security_region(uint32_t id, uint32_t start, uint32_t end, bool secure);
/* A/B trial confirm (boot_param_confirm.c): settle a TRIAL boot record to NORMAL
 * once the image has proven it can bring up the secure world. */
extern int boot_param_confirm(void);
extern uint32_t flash_get_excute_enable(void);

extern const struct memory_region_limits memory_regions;
/*
	MCLK:26MHz, delay(1): about 25us
				delay(10):about 125us
				delay(100):about 850us
 */
void delay(uint32_t num)
{
	volatile uint32_t i, j;

	for (i = 0; i < num; i ++) {
		for (j = 0; j < 100; j ++)
			;
	}
}

int bk_flash_dbus_isolation_init(void)
{
	uint32_t ps_offset = partition_get_phy_offset(PARTITION_SYS_PS);
	uint32_t ps_size = partition_get_phy_size(PARTITION_SYS_PS);
	uint32_t primary_offset = partition_get_phy_offset(PARTITION_PRIMARY_ALL);
	uint32_t secondary_offset = partition_get_phy_offset(PARTITION_SECONDARY_ALL);
	uint32_t secondary_size = partition_get_phy_size(PARTITION_SECONDARY_ALL);
	uint32_t region0_end;
	uint32_t region1_start;
	uint32_t region1_end;

	bk_fih_set_src(FIH_DATA_DBUS, 0xbb);
	bk_sw_fih_set_data(FIH_SW_INDEX11);

	/* Region 0: boot + ITS/PS (and anything before PS end) stays Secure on DBUS. */
	region0_end = ps_offset + ps_size - 1u;
	bk_flash_set_dbus_security_region(0, 0, region0_end, true);
	bk_sw_fih_set_data(FIH_SW_INDEX12);

	/*
	 * Region 1: protect the active Secure image window.
	 * Dual-slot A/B: slot size = secondary_offset - primary_offset.
	 * Single-slot (secondary_all absent): that formula underflows to
	 * 0xFFFFFFFF and marks nearly all flash Secure, so NS flash-controller
	 * reads (e.g. sys_rf) BusFault. Fall back to primary_tfm_s only.
	 */
	if (!CONFIG_DIRECT_XIP ||
#if defined(CONFIG_XIP_FORCE_SLOT_A)
	    true ||
#endif
	    (secondary_offset == 0u) || (secondary_size == 0u)) {
		region1_start = partition_get_phy_offset(PARTITION_PRIMARY_TFM_S);
		region1_end = region1_start + partition_get_phy_size(PARTITION_PRIMARY_TFM_S) - 1u;
	} else {
		region1_start = flash_get_excute_enable() ? secondary_offset : primary_offset;
		region1_end = region1_start + secondary_offset - primary_offset - 1u;
	}

	bk_flash_set_dbus_security_region(1, region1_start, region1_end, true);

	bk_sw_fih_set_data(FIH_SW_INDEX13);
	bk_fih_set_dst(FIH_DATA_DBUS, 0xbb);

	return BK_OK;
}

FIH_RET_TYPE(enum tfm_hal_status_t) tfm_hal_platform_init(void)
{
    enum tfm_plat_err_t plat_err = TFM_PLAT_ERR_SYSTEM_ERR;
#ifdef TFM_FIH_PROFILE_ON
    fih_int fih_rc = FIH_FAILURE;
#endif

    plat_err = enable_fault_handlers();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

    plat_err = system_reset_cfg();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

#ifdef TFM_FIH_PROFILE_ON
    FIH_CALL(init_debug, fih_rc);
    if (fih_not_eq(fih_rc, fih_int_encode(TFM_PLAT_ERR_SUCCESS))) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }
#else
    plat_err = init_debug();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        return TFM_HAL_ERROR_GENERIC;
    }
#endif

    __enable_irq(); //Never failed
    stdio_init(); //Assert the system if any failure
    plat_err = bk_prro_driver_init();
    if (plat_err != 0) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

    plat_err = nvic_interrupt_target_state_cfg();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

    plat_err = nvic_interrupt_enable();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

#if 1
    plat_err = bk_flash_driver_init();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }
#endif
    plat_err = bk_flash_dbus_isolation_init();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

#if CONFIG_SLEEP_RETENTION_NSC
    if (tfm_sleep_context_build_snapshot() != 0) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }
#endif

    /* Partition and flash map initialization - currently disabled
     * Enable when needed by uncommenting the code below
     */
#if 0
    plat_err = partition_init();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

    plat_err = flash_map_init();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }

    tfm_builtin_key_loader_init();
#endif

    tfm_deepsleep_fastboot_save_xip();

    /* A/B trial confirm: reaching here means MCUboot verified the image and the
     * secure world came up, so adopt a TRIAL slot as the new NORMAL exec_slot.
     * flash + partition table are ready (partition_init ran earlier in
     * tfm_core_init). Idempotent: no-op for a non-TRIAL / virgin record. */
#if !defined(CONFIG_XIP_FORCE_SLOT_A)
    (void)boot_param_confirm();
#endif

    FIH_RET(fih_int_encode(TFM_HAL_SUCCESS));
}

uint32_t tfm_hal_get_ns_VTOR(void)
{
    return memory_regions.non_secure_code_start;
}

uint32_t tfm_hal_get_ns_MSP(void)
{
    return *((uint32_t *)memory_regions.non_secure_code_start);
}

extern uint32_t piece_address(uint8_t *array, uint32_t index);

void tfm_hal_ppc_init_from_flash(uint32_t *reg)
{
	if (!reg) {
		return;
	}
	
	volatile uint32_t *ppro_base = (volatile uint32_t *)PPRO_BASE_ADDR;
	
	for (int i = 0; i < 12; i++) {
		ppro_base[4 + i] = reg[i];
	}
}

/* if fast boot and restart from deep sleep, bootrom will skip bl2/mcuboot
 * tfm will work fleetly. Generally, bl2/mcuboot will share some data with
 * tfm, so if skipping bl2, the data sharing will be ignored.
 */
uint32_t tfm_hal_is_ignore_data_shared(void)
{
    return (sys_is_running_from_deep_sleep() && sys_is_enable_fast_boot());
}

void tfm_hal_dma_init(void)
{
	/* DMA0 lives in the CP domain (always powered here) and is configured at the
	 * SPE->NSPE transition. DMA1 (AP HPDMA) lives in the AP power domain, which is
	 * off at this point; it is configured by tfm_hal_ap_dma_init() from the AP
	 * secure-prepare path after CP NS powers the AP up. */
	volatile uint32_t *dma0_ctrl = (volatile uint32_t *)(DMA0_BASE_ADDR + DMA_CTRL_REG_OFFSET * 4);
	volatile uint32_t *dma0_enable = (volatile uint32_t *)(DMA0_BASE_ADDR + DMA_ENABLE_REG_OFFSET * 4);
	volatile uint32_t *dma0_mask = (volatile uint32_t *)(DMA0_BASE_ADDR + DMA_MASK_REG_OFFSET * 4);
	volatile uint32_t *dma0_int_alloc = (volatile uint32_t *)(DMA0_BASE_ADDR + DMA0_INT_ALLOC_REG_OFFSET * 4);

	/* Soft reset DMA0 module */
	*dma0_ctrl = 0;
	*dma0_ctrl = 1;
	*dma0_mask = 0xFFF;
	*dma0_enable = 0;
	*dma0_int_alloc = DMA0_INT_ALLOC_VALUE;
}

void tfm_hal_ap_dma_init(void)
{
	/* AP HPDMA (DMA1) controller-wide setup. The AP power domain must already be
	 * up and clocked (driven by CP NS). The DMA1 control registers are Secure and
	 * are written through the secure alias; the soft reset clears secure_attr to 0
	 * so the NS AP can program the channels afterwards. */
	volatile uint32_t *dma1_ctrl = (volatile uint32_t *)(DMA1_BASE_ADDR + DMA_CTRL_REG_OFFSET * 4);
	volatile uint32_t *dma1_enable = (volatile uint32_t *)(DMA1_BASE_ADDR + DMA_ENABLE_REG_OFFSET * 4);
	volatile uint32_t *dma1_mask = (volatile uint32_t *)(DMA1_BASE_ADDR + DMA_MASK_REG_OFFSET * 4);
	volatile uint32_t *dma1_int_alloc = (volatile uint32_t *)(DMA1_BASE_ADDR + DMA1_INT_ALLOC_REG_OFFSET * 4);

	*dma1_ctrl = 0;
	*dma1_ctrl = 1;
	*dma1_mask = 0;
	*dma1_enable = 0;
	*dma1_int_alloc = DMA1_INT_ALLOC_VALUE;
}

void ppro_init(void)
{

}

void tfm_s_2_ns_hook(void)
{
	extern void tcm_spe();
	tcm_spe();

	/* The SDK logging path belongs to the non-secure driver stack and is not
	 * safe to call from this secure partition context; no print here. The
	 * Non-Secure app brings up its own console after the jump. */

#if !defined(DOMAIN_NS) || (DOMAIN_NS != 1U)
	/* Initialize UART0 for NS side before jumping to NS */
	/* Directly use UART driver functions instead of CMSIS driver to avoid dependency */
	uart_config_t uart0_config = {
		.baud_rate = DEFAULT_UART_BAUDRATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_NONE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_FLOWCTRL_DISABLE,
		.src_clk = UART_SCLK_XTAL_26M,
	};
	
	bk_err_t ret;

	sys_drv_init();
	/* Do not initialise the NS UART0/GPIO from this secure partition context:
	 * those drivers belong to the non-secure stack. The Non-Secure app brings
	 * up its own UART0/GPIO after the jump. */
	(void)ret;
	(void)uart0_config;
#endif

	tfm_hal_dma_init();
	ppro_init();
	/* Exceptions target the Non-secure hardfault exception.
	 * BusFault, HardFault, and NMI Non-secure enable.
	 * SCB_AIRCR_PRIS_Msk: if bit[14]=0, invisible fault (implementation defined behavior)
	 */
	uint32_t reg_val = SCB->AIRCR;
	reg_val &= (~(uint32_t)SCB_AIRCR_VECTKEYSTAT_Msk);
	reg_val |= (uint32_t)((0x5FAUL << SCB_AIRCR_VECTKEY_Pos)
						| SCB_AIRCR_PRIS_Msk);
	SCB->AIRCR = reg_val;

	/* Permit Non-secure access to the Floating-point Extension.
	* Note: It is still necessary to set CPACR_NS to enable the FP Extension
	* in the NSPE. This configuration is left to NS privileged software.
	*/
	SCB->NSACR |= SCB_NSACR_CP10_Msk | SCB_NSACR_CP11_Msk;
}

uint32_t tfm_hal_get_ns_entry_point(void)
{
#if CONFIG_REG_ACCESS_NSC
	void psa_reg_nsc_stub(void);
	psa_reg_nsc_stub();
#endif
#if CONFIG_AP_BOOT_NSC
	void psa_ap_boot_nsc_stub(void);
	psa_ap_boot_nsc_stub();
#endif
#if CONFIG_DUBHE_KEY_LADDER_NSC
	void psa_key_ladder_nsc_stub(void);
	psa_key_ladder_nsc_stub();
#endif
#if CONFIG_MPC_NSC
	void psa_mpc_nsc_stub(void);
	psa_mpc_nsc_stub();
#endif
#if (CONFIG_FLASH_NSC) || (CONFIG_TFM_READ_FLASH_NSC)
	void psa_flash_nsc_stub(void);
	psa_flash_nsc_stub();
#endif
#if CONFIG_INT_TARGET_NSC
	void psa_int_target_nsc_stub(void);
	psa_int_target_nsc_stub();
#endif
#if CONFIG_PM_NSC
	void psa_pm_nsc_stub(void);
	psa_pm_nsc_stub();
#endif
#if CONFIG_OTP_NSC
	void psa_otp_nsc_stub(void);
	psa_otp_nsc_stub();
#endif
#if CONFIG_AES_GCM_NSC
	void psa_aes_gcm_nsc_stub(void);
	psa_aes_gcm_nsc_stub();
#endif
	bk_fih_set_src(FIH_DATA_RESERVE3, 0xff);
	bk_fih_set_dst(FIH_DATA_RESERVE3, 0xff);
	bk_fih_set_src(FIH_DATA_RESERVE4, 0x00);
	bk_fih_set_dst(FIH_DATA_RESERVE4, 0x00);
	bk_fih_set_src(FIH_DATA_MSP_PC, 0xcc);
	bk_fih_set_dst(FIH_DATA_MSP_PC, 0xcc);
	bk_fih_validate();
	tfm_s_2_ns_hook();
	uint32_t ns_ep = *((uint32_t *)(memory_regions.non_secure_code_start + 4));
	return ns_ep;
}
