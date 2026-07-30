// Copyright 2026 Beken
//
// Public BL2 download entry points.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void legacy_boot_main(void);
void UART_InterruptHandler(void);

#ifdef __cplusplus
}
#endif
