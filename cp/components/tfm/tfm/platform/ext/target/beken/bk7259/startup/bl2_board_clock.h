/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BL2 self-contained clock bring-up (mirrors NS bootloader board_clock.c).
 * No dependency on CP middleware sys_drv / sys_hal.
 */
#pragma once

void bl2_clock_analog_early_init(void);
void bl2_clock_enable_pll(void);
void bl2_clock_enable_high_freq(void);
