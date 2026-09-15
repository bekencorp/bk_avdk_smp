// Copyright 2026 Beken
//
// Project-level override for the register-level debug probe (dbg_probe) on the
// AP subsystem.
//
// This file is OPTIONAL. The SDK (dbg_probe_cfg.h) includes it only if present
// and falls back to a built-in default for every macro left undefined.
//
// Enable CONFIG_DBG_PROBE in menuconfig first, then uncomment/edit the macros
// your board needs (same spirit as usr_gpio_cfg.h).

#pragma once

/* --- AP dbg_probe: uncomment & edit only when CONFIG_DBG_PROBE=y ------------
 *
 * #define DBG_PROBE_UART_PORT         1u          // early port: 0..4
 * #define DBG_PROBE_UART_TX_PIN       1u          // GPIO id for TX
 * #define DBG_PROBE_UART_BAUD         1000000u
 *
 * #define DBG_PROBE_CORE1_UART_PORT   2u
 * #define DBG_PROBE_CORE1_TX_PIN      22u
 * #define DBG_PROBE_CORE1_BAUD        1000000u
 *
 * #define DBG_PROBE_UART_FORCE_POWER  0u
 * #define DBG_PROBE_UART_FORCE_CLOCK  1u
 * #define DBG_PROBE_UART_DROP_ON_FULL 1u
 *
 * When enabling on AP, you may also need GPIO_1 mapped to UART1_TXD in
 * usr_gpio_cfg.h so runtime TX is not re-muxed away from the probe port.
 * ------------------------------------------------------------------------- */
