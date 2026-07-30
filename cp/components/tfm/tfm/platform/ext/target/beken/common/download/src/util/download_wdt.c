// Copyright 2026 Beken
//
// BK7259 BL2 download watchdog handling.

#include "../download_internal.h"

/* On BK7259 this compatibility API disables both watchdogs; val is unused. */
#define DL_AON_WDT_CTRL_REG          (0x44000600 + 0x0 * 4)
#define DL_CPU_WWDT_CTRL_REG         (0xE0050000 + 0x4 * 4)
#define DL_WDT_RESET_CFG_REG         (0x44000000 + 0x2 * 4)
#define DL_WDT_RESET_ALL             (0x7u)

void wdt_time_set(uint32_t val)
{
	*((volatile unsigned long *)DL_AON_WDT_CTRL_REG) = (0x5A0000u | val);
	*((volatile unsigned long *)DL_AON_WDT_CTRL_REG) = (0xA50000u | val);

	*((volatile unsigned long *)DL_CPU_WWDT_CTRL_REG) = (0x5A0000u | val);
	*((volatile unsigned long *)DL_CPU_WWDT_CTRL_REG) = (0xA50000u | val);
}
