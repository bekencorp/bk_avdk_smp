// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: minimal PRRO (peripheral RO / PPC) driver init.
//
// tfm_hal_platform.c calls bk_prro_driver_init() during secure init. The actual
// PPC configuration (SYS/Flash secure attribution, GPIO non-secure, AON WDT) is
// performed by common/secure/bk_tfm_ppc.c (bk_ppc_init/bk_ppc_cfg). This shim
// only provides the driver-init entry the SDK API contract expects; there is no
// per-driver bookkeeping needed in the secure-boot path.

#include <stdint.h>
#include <common/bk_include.h>
#include "prro_hal.h"

bk_err_t bk_prro_driver_init(void)
{
	return BK_OK;
}

bk_err_t prro_hal_set_secure(prro_dev_t dev, prro_secure_type_t secure_type)
{
	/* PPC secure attribution (incl. REG / AON_WDT / APB_WDT) is programmed at
	 * boot by common/secure/bk_tfm_ppc.c (bk_ppc_init). The only caller here is
	 * tfm_platform_system.c re-asserting secure right before a watchdog reset,
	 * which is redundant given the boot-time config; keep it best-effort no-op
	 * for Phase-1 self-containment. */
	(void)dev;
	(void)secure_type;
	return BK_OK;
}
