// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int tfm_sleep_context_restore(void);

bool tfm_sleep_context_is_valid(void);
int tfm_sleep_context_build_snapshot(void);
int tfm_sleep_context_refresh_ppro(void);

#ifdef __cplusplus
}
#endif
