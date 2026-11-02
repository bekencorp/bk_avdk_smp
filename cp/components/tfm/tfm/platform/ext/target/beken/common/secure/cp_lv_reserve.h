// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* CP SRAM bad-point write-back / drop, called from tfm_fastboot_resume()
 * (SRAM-resident), ahead of anything that reads retained SRAM. Ships with
 * CONFIG_SLEEP_RETENTION_NSC. */
void cp_lv_reserve_restore(void);
void cp_lv_reserve_discard(void);

#ifdef __cplusplus
}
#endif
