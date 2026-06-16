// Copyright 2026 Beken
//
// dbg_probe UART direct-write backend (CP, no UART driver framework).
//
// Two init paths share one register-apply helper:
//   - dbg_sink_uart_init()      : EARLY path. Port/pin/baud resolved at COMPILE
//       time (dbg_probe_cfg.h / #if). Uses only immediates+stack, ZERO data-memory
//       dependency, safe at the very first boot instruction (pre .data/.bss).
//   - dbg_sink_uart_reconfig()  : RUNTIME path (Ver3.5). Resolves a port via a
//       flash-const table + weak board hook, then re-applies. Call only AFTER RAM
//       is ready (after b_prep_entry_main on CP core0).
// Force-on power+clock+gpio, bypassing bk_uart_*.

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_DBG_PROBE

/* Power domain that must be forced on before a port is usable. */
typedef enum {
	DBG_UART_PWR_NONE = 0,   /* always-on domain, no action */
	DBG_UART_PWR_CPU1 = 1,   /* SYS R10.pwd_cpu1 */
} dbg_uart_pwr_t;

/* Per-port hardware resource descriptor (runtime path). Lives in flash .rodata;
 * read at runtime (post-RAM) is fine. hw==NULL => port not provided. */
typedef struct {
	void    *hw;          /* uart_hw_t* base; NULL = not provided -> disabled */
	uint16_t func_tx;     /* FUNC_CODE_UARTx_TXD for the TX pinmux */
	uint8_t  cken_bit;    /* bit index in SYS cpu_device_clk_enable */
	uint8_t  cksel_bit;   /* bit index in SYS cpu_clk_div_mode2 */
	uint8_t  pwr_domain;  /* dbg_uart_pwr_t */
} dbg_uart_hw_desc_t;

/* Board hook (weak): return resource descriptor for `port`, or NULL to disable.
 * Default impl serves the validated SDK table (U0/U1); a board may override to
 * add/replace ports without touching the SDK. Runtime path only. */
const dbg_uart_hw_desc_t *dbg_probe_board_uart_desc(uint8_t port);

/** EARLY path: force-bring-up the compile-time UART + 8N1. No data-memory dep. */
void dbg_sink_uart_init(void);

/** RUNTIME path (Ver3.5): resolve `port` via the board hook and re-apply with
 *  the given tx_pin/baud. Returns false (safe no-op) on invalid/unprovided port.
 *  NOTE: the write path still targets the compile-time port; runtime port-switch
 *  / per-core multi-instance routing lands in Ver4. */
bool dbg_sink_uart_reconfig(uint8_t port, uint8_t tx_pin, uint32_t baud);

/** Blocking-with-cap FIFO write of one frame (compile-time base; early path). */
void dbg_sink_uart_write(const uint8_t *buf, uint32_t len);

/** Ver4: same write to an arbitrary resolved base (per-core instance dispatch).
 *  base==NULL -> drop. Returns true if every byte was handed to the TX FIFO,
 *  false if the FIFO stayed full past the bounded cap (the rest of the frame is
 *  dropped) — the caller uses this to maintain a per-core drop counter. */
bool dbg_sink_uart_write_base(void *base, const uint8_t *buf, uint32_t len);

#endif /* CONFIG_DBG_PROBE */

#ifdef __cplusplus
}
#endif
