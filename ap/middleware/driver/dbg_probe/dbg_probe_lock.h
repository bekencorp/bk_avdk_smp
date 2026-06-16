// Copyright 2026 Beken
//
// dbg_probe TX serialization (include only from dbg_probe.c).
// Ver2: single-core-safe critical section (disable IRQ around the frame write)
// to prevent IRQ-vs-task concurrency on the same core. The full three-state
// strategy (SMP spinlock for shared instances + dynamic core-unplug downgrade)
// lands in Ver4 together with the binding model.

#pragma once

#if CONFIG_DBG_PROBE

#include "cmsis_compiler.h"

#define DBG_PROBE_LOCK_DECLARE()    uint32_t _dbg_probe_primask
#define DBG_PROBE_LOCK_ENTER() \
	do { \
		_dbg_probe_primask = __get_PRIMASK(); \
		__disable_irq(); \
	} while (0)
#define DBG_PROBE_LOCK_EXIT() \
	do { \
		__set_PRIMASK(_dbg_probe_primask); \
	} while (0)

#endif /* CONFIG_DBG_PROBE */
