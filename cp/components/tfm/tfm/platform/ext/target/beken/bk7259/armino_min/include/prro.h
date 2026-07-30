// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: minimal PRRO API header.
//
// tfm_hal_platform.c includes <prro.h> and only uses bk_prro_driver_init().
// The full SDK prro.h pulls in prro_types.h (prro_dev_t / cmp ids) for the
// peripheral secure-attribution API which the secure-boot path does not use
// (PPC config is done in common/secure/bk_tfm_ppc.c). Keep this header minimal.

#pragma once

#include <common/bk_include.h>

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_prro_driver_init(void);
bk_err_t bk_prro_driver_deinit(void);

#ifdef __cplusplus
}
#endif
