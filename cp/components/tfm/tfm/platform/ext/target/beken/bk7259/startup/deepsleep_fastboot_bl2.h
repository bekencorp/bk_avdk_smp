// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#pragma once

/*
 * Try the BL2 deep-sleep fast path.
 *
 * Returns 0 when the wakeup/A-B/remap/vector checks do not allow fastboot.
 * A successful fastboot transfers control to TF-M and never returns.
 */
int bl2_deepsleep_fastboot(void);
