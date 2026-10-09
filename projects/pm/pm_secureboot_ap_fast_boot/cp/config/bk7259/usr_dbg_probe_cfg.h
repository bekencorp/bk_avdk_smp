// Copyright 2026 Beken
//
// Project-level override for the register-level debug probe (dbg_probe).
//
// This file is OPTIONAL. The SDK (dbg_probe_cfg.h) includes it only if present
// and falls back to a built-in default for every macro you leave undefined.
// So you only need to define the few parameters your board actually differs on;
// delete this file entirely to use all defaults.
//
// (Same spirit as usr_gpio_cfg.h in this directory.)

#pragma once

/* --- UART backend: uncomment & edit only what your board needs --------------
 *
 * #define DBG_PROBE_UART_PORT         1          // 0..5, must exist in k_dbg_uart_hw[]
 * #define DBG_PROBE_UART_TX_PIN       1          // GPIO id for TX
 * #define DBG_PROBE_UART_BAUD         1000000
 * #define DBG_PROBE_UART_SRC_CLK_HZ   26000000
 * #define DBG_PROBE_UART_FORCE_POWER  1          // force-on power domain
 * #define DBG_PROBE_UART_FORCE_CLOCK  1          // force-on clock
 * #define DBG_PROBE_UART_DROP_ON_FULL 1          // 1=never block on stuck FIFO
 * #define DBG_PROBE_SHARED_PORT       1          // set_shared() convergence port
 *                                                // (default = master/core0 port)
 *
 * Defaults (when this file is absent or a macro is left undefined):
 *   PORT=1  TX_PIN=GPIO1  BAUD=1Mbps 8N1  SRC_CLK=26MHz  FORCE_*=1  DROP=1
 *   SHARED_PORT = DBG_PROBE_UART_PORT (master/core0)
 * ------------------------------------------------------------------------- */
