// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CP SRAM bad-point write-back / drop, called from tfm_fastboot_resume()
 * (SRAM-resident), ahead of anything that reads retained SRAM. Ships with
 * CONFIG_SLEEP_RETENTION_NSC. */
void cp_lv_reserve_restore(void);
void cp_lv_reserve_discard(void);

extern uint32_t sys_is_running_from_deep_sleep(void);

static inline __attribute__((always_inline)) void cp_lv_reserve_early_restore(void)
{
	if (sys_is_running_from_deep_sleep() != 0u) {
		cp_lv_reserve_restore();
	} else {
		cp_lv_reserve_discard();
	}
}

#ifdef __cplusplus
}
#endif
