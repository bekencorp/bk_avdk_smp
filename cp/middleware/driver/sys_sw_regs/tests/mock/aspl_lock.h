#pragma once

/* Minimal stub for unit tests on PC (gtest).
 * Provides no-op implementations of the SYS_SW_REGS HSPL lock API. */
#include <stdint.h>

static inline uint32_t bk_aspl_sys_sw_regs_enter_critical(void) { return 0; }
static inline void bk_aspl_sys_sw_regs_exit_critical(uint32_t flags) {}
