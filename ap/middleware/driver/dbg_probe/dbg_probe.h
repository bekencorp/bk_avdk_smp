// Copyright 2026 Beken
//
// dbg_probe: two-layer register-level debug probe (frontend, backend-agnostic).
// CONFIG_DBG_PROBE=n compiles every API to a static-inline no-op (zero overhead).
//
// Ver5 frame formats (self-describing; length derived from header + kind):
//   Common header byte0: bit[7:6]=0b10 magic, bit5=has_ts, bit4=core_id,
//                        bit[3:0]=kind.  byte1 = module_id (dbg_probe_modules.h).
//
//   (A) value16 kinds  STAGE/IRQ/U16/EXC/CUSTOM:
//        4 bytes when has_ts=0:   [hdr, mod, val_lo, val_hi]
//        8 bytes when has_ts=1:   [hdr, mod, val_lo, val_hi, ts0..ts3]   (full 32-bit
//                                  AON-RTC tick, little-endian, in bytes 4..7)
//   (B) payload32 kinds  TASK/U32 (always 8 bytes):
//        [hdr, mod, sts_lo, sts_hi, p0, p1, p2, p3]
//        bytes2..3 = SHORT timestamp (AON-RTC low 16 bits) when has_ts=1, else 0;
//        bytes4..7 = 32-bit payload (TASK: 4 ASCII chars of the name; U32: value).
//        Host rebuilds the full time from a neighbouring full-ts frame.
//
// _ts API variants emit a timestamp; the plain variants do not (smaller frame).
// core_id is read at runtime via bk_multicore_get_cpu_id() (the legacy raw read
// of 0x20000000 is NOT used on BK7259). Early/boot stages use dbg_probe_init /
// dbg_probe_early_stage (RAM+UART tee on compile-time port, explicit core, no ts).

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "sdkconfig.h"
#include "dbg_probe_modules.h"

/* Ver9 sink / filter constants (valid even when CONFIG_DBG_PROBE=n). */
#define DBG_PROBE_SINK_UART       0x01u
#define DBG_PROBE_SINK_RAM        0x02u
#define DBG_PROBE_SINK_BOTH       (DBG_PROBE_SINK_UART | DBG_PROBE_SINK_RAM)
#define DBG_PROBE_MOD_MASK_WORDS  8u

#if CONFIG_DBG_PROBE

/* Runtime configuration (Ver3.5). ABI-stable: first field is struct_size so the
 * library can tell which fields a (possibly older) caller actually provided;
 * any scalar left 0 / 0xFF means "use the compile-time default" (dbg_probe_cfg.h).
 * Ver3.5 reconfigures the *compile-time* port (e.g. runtime baud/pin); switching
 * to a different port / multi-instance per-core binding lands in Ver4. */
typedef struct {
	uint32_t struct_size;   /* = sizeof(dbg_probe_cfg_t); set by dbg_probe_cfg_init() */
	uint8_t  uart_port;     /* 0xFF = default */
	uint8_t  uart_tx_pin;   /* 0xFF = default */
	uint8_t  flags;         /* reserved, 0 = default */
	uint8_t  rsv0;
	uint32_t baud;          /* 0 = default */
} dbg_probe_cfg_t;

/** Fill cfg with defaults (struct_size + all-default sentinels). */
void dbg_probe_cfg_init(dbg_probe_cfg_t *cfg);

/** Runtime (re)init, safe to call only after RAM (.data/.bss) is ready
 *  (i.e. after b_prep_entry_main on CP core0). cfg==NULL uses all defaults.
 *  Re-applies the resolved UART config and emits a runtime-start marker frame.
 *  Invalid/unsupported cfg -> safe no-op, never bricks. */
void dbg_probe_runtime_init(const dbg_probe_cfg_t *cfg);

/** Ver4 SMP: core1's own runtime init — call from core1 startup
 *  (Reset_Handler_Core1). Brings up core1's debug port — configured via
 *  DBG_PROBE_CORE1_UART_PORT / _CORE1_TX_PIN / _CORE1_BAUD (overridable in the
 *  project's usr_dbg_probe_cfg.h) — and publishes its per-core dispatch slot.
 *  Knows it is core1 (does not read the core-id register). */
void dbg_probe_runtime_init_core1(void);

/* Frame kind (byte0 bit[3:0]) */
#define DBG_PROBE_KIND_STAGE    0x0u
#define DBG_PROBE_KIND_IRQ      0x1u
#define DBG_PROBE_KIND_TASK     0x2u
#define DBG_PROBE_KIND_U16      0x3u
#define DBG_PROBE_KIND_U32      0x4u
#define DBG_PROBE_KIND_EXC      0x5u
#define DBG_PROBE_KIND_CUSTOM   0x6u
/* Kinds 0x7..0xF are RESERVED. Decoders derive the frame length from the kind
 * class (TASK/U32 = payload32, 8 bytes; everything else = value16, 4/8 bytes by
 * has_ts), so adding a NEW payload32-class kind changes the wire grammar: old
 * parsers would mis-size such frames and lose sync until the next SYNC frame.
 * Any new kind therefore requires a parser upgrade in the same release — old
 * byte streams stay parseable, but new streams are NOT forward-compatible with
 * old parsers. */

/* Header constants */
#define DBG_PROBE_HDR_MAGIC     0x80u   /* bit[7:6]=0b10 */
#define DBG_PROBE_HDR_HAS_TS    0x20u   /* bit5: frame carries a timestamp */

/** Initialize the active sink (UART direct-write + RAM ring on core0) and emit a
 *  stream-start magic frame. Safe to call early in the reset handler (pre-.bss;
 *  ring lives in .noinit). */
void dbg_probe_init(void);

/** EARLY/boot stage marker. Writes RAM ring (when enabled) + compile-time UART.
 *  4-byte frame, no timestamp. `core` is SELF-REPORTED by the caller.
 *  See dbg_probe_early_stage notes in the implementation for concurrency caveats.
 *  At runtime prefer dbg_probe_stage() (per-core routing, locking, timestamp). */
void dbg_probe_early_stage(uint8_t core, uint16_t stage);

/* ---- Runtime per-core API (post dbg_probe_runtime_init on each core) --------
 * Reads the current CPU core id (bk_multicore_get_cpu_id) and routes to that
 * core's bound instance (core0 -> DBG_PROBE_UART_PORT, core1 ->
 * DBG_PROBE_CORE1_UART_PORT; see usr_dbg_probe_cfg.h overrides) under its lock
 * mode (exclusive=irq-disable / shared=spinlock). The header carries the real
 * core_id bit. If the current core has no live instance (e.g. hot-unplugged)
 * the frame is dropped. Each call comes in two flavours: plain (no timestamp,
 * smaller frame) and _ts (carries an AON-RTC timestamp). */

/** Stage probe: module classifies, stage is the 16-bit detail id. */
void dbg_probe_stage(uint8_t module, uint16_t stage);
void dbg_probe_stage_ts(uint8_t module, uint16_t stage);

/** IRQ enter/exit / vector probe (16-bit irq detail). */
void dbg_probe_irq(uint8_t module, uint16_t irq);
void dbg_probe_irq_ts(uint8_t module, uint16_t irq);

/** Generic 16-bit value probe. */
void dbg_probe_u16(uint8_t module, uint16_t v);
void dbg_probe_u16_ts(uint8_t module, uint16_t v);

/** 32-bit value probe (8-byte frame; _ts adds a 16-bit short timestamp). */
void dbg_probe_u32(uint8_t module, uint32_t v);
void dbg_probe_u32_ts(uint8_t module, uint32_t v);

/** Task probe: tag = task name; first 4 ASCII chars are packed into the 32-bit
 *  payload (directly readable in a hex dump, no host dictionary needed). The
 *  4-byte id space makes name collisions far less likely than a 16-bit hash.
 *  8-byte frame; _ts adds a 16-bit short timestamp. */
void dbg_probe_task(uint8_t module, const char *tag);
void dbg_probe_task_ts(uint8_t module, const char *tag);

/** Exception / fault probe (16-bit code). */
void dbg_probe_exc(uint8_t module, uint16_t code);
void dbg_probe_exc_ts(uint8_t module, uint16_t code);

/** Lowest-level escape hatch: caller fills kind/module/value. */
void dbg_probe_raw(uint8_t kind, uint8_t module, uint16_t v);
void dbg_probe_raw_ts(uint8_t kind, uint8_t module, uint16_t v);

/* Deprecated Ver4 aliases (kept for source compatibility; == stage/raw). */
void dbg_probe_rt_stage(uint8_t module, uint16_t stage);
void dbg_probe_rt_raw(uint8_t kind, uint8_t module, uint16_t v);

/** VALIDATION/TEST helper (NOT a stable public API): switch the per-core lock
 *  model at runtime, to exercise both concurrency paths on one board.
 *  on=true  -> SHARED: both cores converge onto DBG_PROBE_SHARED_PORT (default =
 *              master/core0 port) and serialize via a spinlock.
 *  on=false -> EXCLUSIVE: restore each core to its own port (irq-disable only).
 *  PRECONDITION: call from one core while probes are QUIESCED (no in-flight
 *  bursts on either core). The switch takes no cross-core lock; its write order
 *  only guarantees a racing core never writes the converged wire lock-free —
 *  frames racing the transition may still interleave on the wire. The shared
 *  convergence port is parameterized (DBG_PROBE_SHARED_PORT); production users
 *  that genuinely need a single-wire/multi-core layout should configure ports
 *  via usr_dbg_probe_cfg.h rather than rely on this switch. */
void dbg_probe_set_shared(bool on);

/* ---- Ver9: runtime sink + UART module filter -------------------------------- */

void dbg_probe_set_sink_mask(uint8_t mask);
uint8_t dbg_probe_get_sink_mask(void);

void dbg_probe_mod_uart_set_all(bool enable);
void dbg_probe_mod_uart_set(uint8_t module, bool enable);
bool dbg_probe_mod_uart_get(uint8_t module);
void dbg_probe_mod_uart_get_mask(uint32_t out[DBG_PROBE_MOD_MASK_WORDS]);

#else /* !CONFIG_DBG_PROBE : zero-overhead no-op */

typedef struct { uint32_t struct_size; } dbg_probe_cfg_t;
static inline void dbg_probe_cfg_init(dbg_probe_cfg_t *cfg) { (void)cfg; }
static inline void dbg_probe_runtime_init(const dbg_probe_cfg_t *cfg) { (void)cfg; }
static inline void dbg_probe_runtime_init_core1(void) {}
static inline void dbg_probe_init(void) {}
static inline void dbg_probe_early_stage(uint8_t core, uint16_t stage) { (void)core; (void)stage; }
static inline void dbg_probe_stage(uint8_t module, uint16_t stage) { (void)module; (void)stage; }
static inline void dbg_probe_stage_ts(uint8_t module, uint16_t stage) { (void)module; (void)stage; }
static inline void dbg_probe_irq(uint8_t module, uint16_t irq) { (void)module; (void)irq; }
static inline void dbg_probe_irq_ts(uint8_t module, uint16_t irq) { (void)module; (void)irq; }
static inline void dbg_probe_u16(uint8_t module, uint16_t v) { (void)module; (void)v; }
static inline void dbg_probe_u16_ts(uint8_t module, uint16_t v) { (void)module; (void)v; }
static inline void dbg_probe_u32(uint8_t module, uint32_t v) { (void)module; (void)v; }
static inline void dbg_probe_u32_ts(uint8_t module, uint32_t v) { (void)module; (void)v; }
static inline void dbg_probe_task(uint8_t module, const char *tag) { (void)module; (void)tag; }
static inline void dbg_probe_task_ts(uint8_t module, const char *tag) { (void)module; (void)tag; }
static inline void dbg_probe_exc(uint8_t module, uint16_t code) { (void)module; (void)code; }
static inline void dbg_probe_exc_ts(uint8_t module, uint16_t code) { (void)module; (void)code; }
static inline void dbg_probe_raw(uint8_t kind, uint8_t module, uint16_t v) { (void)kind; (void)module; (void)v; }
static inline void dbg_probe_raw_ts(uint8_t kind, uint8_t module, uint16_t v) { (void)kind; (void)module; (void)v; }
static inline void dbg_probe_rt_stage(uint8_t module, uint16_t stage) { (void)module; (void)stage; }
static inline void dbg_probe_rt_raw(uint8_t kind, uint8_t module, uint16_t v) { (void)kind; (void)module; (void)v; }
static inline void dbg_probe_set_shared(bool on) { (void)on; }
static inline void dbg_probe_set_sink_mask(uint8_t mask) { (void)mask; }
static inline uint8_t dbg_probe_get_sink_mask(void) { return 0u; }
static inline void dbg_probe_mod_uart_set_all(bool enable) { (void)enable; }
static inline void dbg_probe_mod_uart_set(uint8_t module, bool enable) { (void)module; (void)enable; }
static inline bool dbg_probe_mod_uart_get(uint8_t module) { (void)module; return false; }
static inline void dbg_probe_mod_uart_get_mask(uint32_t out[DBG_PROBE_MOD_MASK_WORDS]) { (void)out; }

#endif /* CONFIG_DBG_PROBE */

#ifdef __cplusplus
}
#endif
