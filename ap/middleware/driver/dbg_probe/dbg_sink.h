// Copyright 2026 Beken
//
// dbg_probe backend (Sink) abstraction. The frontend encodes a frame then hands
// the bytes to a Sink. Ver2 wires a single compile-time sink (UART1 direct
// write) called inline for minimal overhead; the ops table here documents the
// interface that pluggable sinks (SD/SPI/RAM-ringbuf) will implement later.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_DBG_PROBE

/* NOTE (Ver9): runtime tee honours dbg_probe_set_sink_mask() and the UART-only
 * 256-bit module filter (dbg_probe_mod_uart_*). RAM ring records all user
 * frames when DBG_PROBE_SINK_RAM is set; SYNC/DROP/EXC bypass the module filter.
 * This ops table documents future pluggable backends (SD/SPI/...). */
typedef struct {
	void (*init)(const void *cfg);                   /* bring up backend hw/resource */
	void (*write)(const uint8_t *buf, uint32_t len); /* emit one frame (4 or 8 bytes) */
	void (*flush)(void);                             /* optional: batch flush (SD, ...) */
} dbg_sink_ops_t;

#endif /* CONFIG_DBG_PROBE */

#ifdef __cplusplus
}
#endif
