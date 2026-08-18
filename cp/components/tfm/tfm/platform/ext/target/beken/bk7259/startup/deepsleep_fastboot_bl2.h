// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#pragma once

/*
 * Try the BL2 deep-sleep fast path.
 *
 * Returns 0 when the wakeup checks do not allow fastboot (caller continues
 * with a normal BL2 boot). A successful fastboot transfers control to TF-M
 * and never returns.
 *
 * DIRECT_XIP: validate retention + restore A/B flash remap, then jump.
 * OTA_OVERWRITE: on fast_boot, jump straight to TF-M (no remap / no log).
 */
int bl2_deepsleep_fastboot(void);
