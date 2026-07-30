// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: minimal PRRO HAL header.
//
// tfm_platform_system.c (system-reset service) calls prro_hal_set_secure() on
// REG / AON_WDT / APB_WDT before triggering a watchdog reset. The full SDK
// prro_hal.h pulls in the prro_hw/prro_ll/prro_types subsystem which is not
// ported to bk7259 cp (PPC secure attribution is done by
// common/secure/bk_tfm_ppc.c bk_ppc_init at boot). This header exposes only the
// enum values and the single entry that call site needs; the implementation in
// prro_min.c is best-effort (the devices are already attributed secure at boot
// by bk_ppc_init, so the reset path re-assert is not required for correctness).

#pragma once

#include <common/bk_include.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PRRO_DEV_AON_WDT,
	PRRO_DEV_APB_WDT,
	PRRO_DEV_REG,
} prro_dev_t;

typedef enum {
	PRRO_SECURE = 0,
	PRRO_NON_SECURE,
} prro_secure_type_t;

bk_err_t prro_hal_set_secure(prro_dev_t dev, prro_secure_type_t secure_type);

#ifdef __cplusplus
}
#endif
