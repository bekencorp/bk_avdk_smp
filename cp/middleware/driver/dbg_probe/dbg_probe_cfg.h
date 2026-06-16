// Copyright 2026 Beken
//
// dbg_probe compile-time configuration (Ver3, layer 1).
//
// Customer parameters flow in via the PROJECT header "usr_dbg_probe_cfg.h"
// placed under projects/<proj>/cp/config/<target>/ (that dir is on the driver
// include path; same convention as usr_gpio_cfg.h). This SDK header optionally
// includes it (file may be absent) and falls back to a default for any macro
// the project did not define. So: no project header / unset macro -> default.

#pragma once

#include "sdkconfig.h"

#if CONFIG_DBG_PROBE

/* Optional project override (absent file is fine; needs a compiler with
 * __has_include, which the armino GCC toolchain provides). */
#if defined(__has_include)
#  if __has_include("usr_dbg_probe_cfg.h")
#    include "usr_dbg_probe_cfg.h"
#  endif
#endif

/* ---- UART backend parameters (override any subset in usr_dbg_probe_cfg.h) ---- */

#ifndef DBG_PROBE_UART_PORT
#define DBG_PROBE_UART_PORT        1u          /* index into k_dbg_uart_hw[] */
#endif

#ifndef DBG_PROBE_UART_TX_PIN
#define DBG_PROBE_UART_TX_PIN      1u          /* GPIO id for TX */
#endif

/* Constraint (caller-guaranteed): 0 < DBG_PROBE_UART_BAUD <= SRC_CLK_HZ, and the
 * baud should divide SRC_CLK_HZ cleanly. The driver does NO range check or
 * rounding — clk_div = SRC/baud - 1 is an integer divide that truncates, so an
 * unrepresentable baud silently yields a frequency error. */
#ifndef DBG_PROBE_UART_BAUD
#define DBG_PROBE_UART_BAUD        1000000u
#endif

#ifndef DBG_PROBE_UART_SRC_CLK_HZ
#define DBG_PROBE_UART_SRC_CLK_HZ  26000000u   /* clk_div = src/baud - 1 */
#endif

#ifndef DBG_PROBE_UART_DATA_BITS
#define DBG_PROBE_UART_DATA_BITS   3u          /* register value: 3 = 8 bit */
#endif

#ifndef DBG_PROBE_UART_STOP_BITS
#define DBG_PROBE_UART_STOP_BITS   0u          /* 0 = 1 bit */
#endif

#ifndef DBG_PROBE_UART_FORCE_POWER
#define DBG_PROBE_UART_FORCE_POWER 1u          /* force-on the port's power domain */
#endif

#ifndef DBG_PROBE_UART_FORCE_CLOCK
#define DBG_PROBE_UART_FORCE_CLOCK 1u          /* force-on the port's clock */
#endif

#ifndef DBG_PROBE_UART_DROP_ON_FULL
#define DBG_PROBE_UART_DROP_ON_FULL 1u         /* 1=drop when FIFO stuck (capped spin) */
#endif

/* ---- Ver4 SMP: per-core exclusive instance. core0 uses the params above;
 *      core1 uses a SECOND port below (defaults are just this board's wiring —
 *      override per project via usr_dbg_probe_cfg.h, same 0/0xFF convention). */

#ifndef DBG_PROBE_CORE1_UART_PORT
#define DBG_PROBE_CORE1_UART_PORT  2u          /* index into k_dbg_uart_hw[] */
#endif

#ifndef DBG_PROBE_CORE1_TX_PIN
#define DBG_PROBE_CORE1_TX_PIN     22u         /* GPIO22 (P22), UART2_TXD func=100 */
#endif

#ifndef DBG_PROBE_CORE1_BAUD
#define DBG_PROBE_CORE1_BAUD       1000000u
#endif

/* SHARED-mode convergence port: when dbg_probe_set_shared(true) is used, BOTH
 * cores converge onto this ONE wire (serialized by s_dbg_spin). Default = the
 * master/core0 port (DBG_PROBE_UART_PORT) — semantically the natural choice.
 * Override in usr_dbg_probe_cfg.h only if the master port is unusable at
 * runtime on a given board (e.g. its pad is re-muxed away by the project's
 * usr_gpio_cfg.h default GPIO map). */
#ifndef DBG_PROBE_SHARED_PORT
#define DBG_PROBE_SHARED_PORT      DBG_PROBE_UART_PORT
#endif

/* Number of CP cores the probe dispatches across (SMP: core0 + core1). */
#ifndef DBG_PROBE_NUM_CORES
#define DBG_PROBE_NUM_CORES        2u
#endif

/* Bounded spin retries for the SMP-shared lock before dropping the frame
 * (prevents dead-wait if the lock owner core was hot-unplugged). */
#ifndef DBG_PROBE_SHARED_SPIN_MAX
#define DBG_PROBE_SHARED_SPIN_MAX  100000u
#endif

/* ---- Ver6 RAM ring buffer (per-core, overwrite). Sizes are per core; total
 *      RAM = (MAIN + CRIT) * DBG_PROBE_NUM_CORES + headers. ------------------- */
#ifndef DBG_PROBE_RING_MAIN_SIZE
#define DBG_PROBE_RING_MAIN_SIZE   4096u   /* default stream (high volume) */
#endif

#ifndef DBG_PROBE_RING_CRIT_SIZE
#define DBG_PROBE_RING_CRIT_SIZE   1024u   /* critical (exceptions) — never flooded */
#endif

/* ---- Ver7 periodic sync frame: every N user frames (per core) emit a sync
 *      frame (U32 kind + SYNC module) carrying a 32-bit running frame counter.
 *      Lets the host resync after corruption and detect dropped frames. ------- */
#ifndef DBG_PROBE_SYNC_EVERY
#define DBG_PROBE_SYNC_EVERY       64u
#endif

#endif /* CONFIG_DBG_PROBE */
