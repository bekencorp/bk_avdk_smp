// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* SRAM-resident fastboot resume entry, entered from Reset_Handler when the
 * retained AON PMU fast_boot marker is set. Never returns. */
void tfm_fastboot_resume(void);

#ifdef __cplusplus
}
#endif
