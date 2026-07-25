// Copyright 2026 Beken
//
// dbg_probe frontend (Ver2 minimal): encode self-describing 4B frame, serialize,
// hand to the UART direct-write sink. Hot path is always-inline, no formatting.

#include "dbg_probe.h"

#if CONFIG_DBG_PROBE

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cmsis_compiler.h"
#include "dbg_probe_lock.h"
#include "dbg_probe_cfg.h"
#include "dbg_sink_uart.h"
#include "dbg_sink_ram.h"
#include <soc/soc.h>
#include <soc/bk7259/reg_base.h>

/* Early/compile-time path (boot stages on core0): explicit core, compile-time UART
 * base + .noinit RAM ring (see dbg_probe_emit_early). */

/* ---- Ver9: sink enable + UART-only 256-bit module filter ------------------- */
static uint8_t  s_sink_mask = DBG_PROBE_SINK_BOTH;
static uint32_t s_uart_mod_mask[DBG_PROBE_MOD_MASK_WORDS] = {
	0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
	0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
};

static inline bool dbg_probe_sink_ram_on(void)
{
	return (s_sink_mask & DBG_PROBE_SINK_RAM) != 0u;
}

static inline bool dbg_probe_sink_uart_on(void)
{
	return (s_sink_mask & DBG_PROBE_SINK_UART) != 0u;
}

static inline bool dbg_probe_uart_filter_bypass(uint8_t kind, uint8_t module)
{
	if (kind == DBG_PROBE_KIND_EXC) {
		return true;
	}
	if (module == DBG_MOD_SYNC || module == DBG_MOD_DROP) {
		return true;
	}
	return false;
}

static inline bool dbg_probe_uart_allowed(uint8_t kind, uint8_t module)
{
	uint32_t bit;

	if (!dbg_probe_sink_uart_on()) {
		return false;
	}
	if (dbg_probe_uart_filter_bypass(kind, module)) {
		return true;
	}
	bit = 1u << (module & 31u);
	return (s_uart_mod_mask[module >> 5] & bit) != 0u;
}

static inline void dbg_probe_ram_write(uint8_t core, uint8_t kind,
	const uint8_t *frame, uint32_t len)
{
#if CONFIG_DBG_PROBE_RAMRING
	if (dbg_probe_sink_ram_on()) {
		dbg_sink_ram_write(core, dbg_sink_ram_route(kind), frame, len);
	}
#else
	(void)core; (void)kind; (void)frame; (void)len;
#endif
}

/* ---- AON RTC timestamp source (Ver5) --------------------------------------
 * Global 32 kHz free-running counter, cross-core comparable. We read it raw:
 *   ctrl   @ SOC_AON_RTC_REG_BASE + 0x00, bit6 = enable
 *   counter_val (low 32 bits) @ +0x0C  (see aon_rtc_struct.h)
 * CP force-enables AON RTC at boot; if it is not yet enabled we emit no ts
 * (frame degrades to the no-ts form) so a probe never blocks on an idle RTC. */
#if CONFIG_DBG_PROBE_TIMESTAMP
#define DBG_PROBE_RTC_CTRL   (*(volatile uint32_t *)(SOC_AON_RTC_REG_BASE))
#define DBG_PROBE_RTC_CNT    (*(volatile uint32_t *)(SOC_AON_RTC_REG_BASE + 0x0Cu))
#define DBG_PROBE_RTC_EN     (1u << 6)
static inline bool dbg_probe_ts_ready(void)
{
	return (DBG_PROBE_RTC_CTRL & DBG_PROBE_RTC_EN) != 0u;
}
static inline uint32_t dbg_probe_ts32(void)
{
	return DBG_PROBE_RTC_CNT;
}
/* CP force-enables the AON RTC 32 kHz core clock so timestamps are available
 * even if the application never opened the RTC driver. Idempotent: if it is
 * already running, leave the counter untouched (never disturb SDK timekeeping);
 * we only set en=1 / clear cnt_stop when it was off. */
static inline void dbg_probe_rtc_force_enable(void)
{
	if ((DBG_PROBE_RTC_CTRL & DBG_PROBE_RTC_EN) == 0u) {
		DBG_PROBE_RTC_CTRL |= DBG_PROBE_RTC_EN;   /* bit6: enable 32k clk */
		DBG_PROBE_RTC_CTRL &= ~(1u << 1);         /* bit1: cnt_stop=0 -> counting */
	}
}
#else
static inline bool dbg_probe_ts_ready(void) { return false; }
static inline uint32_t dbg_probe_ts32(void) { return 0u; }
static inline void dbg_probe_rtc_force_enable(void) {}
#endif

/* ---- Ver4 SMP per-core instance table (RAM; valid only post runtime_init) ----
 * Each online core points at one sink. Exclusive: each core on its own
 * configured port (DBG_PROBE_UART_PORT / DBG_PROBE_CORE1_UART_PORT, irq-disable
 * only). Shared: both cores -> one sink, but UART writes are skipped so the
 * spinlock layer does not need a non-blocking acquire API. A core with a NULL slot
 * (never inited / hot-unplugged) silently drops. .bss => 0. */
typedef struct {
	void    *base;     /* uart_hw base; NULL = inactive -> drop */
	uint8_t  shared;   /* 0 = exclusive UART, 1 = shared UART skipped */
} dbg_sink_inst_t;

static dbg_sink_inst_t  s_dbg_sink[DBG_PROBE_NUM_CORES];
static dbg_sink_inst_t *s_core_sink[DBG_PROBE_NUM_CORES];

#if CONFIG_DBG_PROBE_SYNC
/* Per-core running counter of user frames emitted; every DBG_PROBE_SYNC_EVERY a
 * sync frame is injected carrying this value. .bss => 0. */
static uint32_t s_frame_cnt[DBG_PROBE_NUM_CORES];
static void dbg_probe_emit_sync(uint32_t core, uint32_t counter);
#endif

/* Per-core UART-drop accounting. A frame's UART copy is lost when the TX FIFO
 * stays full past the bounded cap, or when shared UART mode is enabled. The
 * RAM-ring copy is never affected.
 * The accumulated count is reported as a DROP frame (U32 kind + DBG_MOD_DROP)
 * the next time that core successfully writes UART, then cleared. .bss => 0. */
static uint32_t s_drop_cnt[DBG_PROBE_NUM_CORES];
static void dbg_probe_emit_drop(uint32_t core, uint32_t count);

/* SMP core id for the RUNTIME dispatch path only. Uses the SDK-blessed accessor
 * (multicore HAL reads the WWDT cpuid register with a magic check). The legacy
 * raw read of 0x20000000 is NOT used on BK7259. The early/explicit-core path
 * (dbg_probe_emit_inst / dbg_probe_early_stage) passes its own known core id and
 * never calls this, so pre-RAM boot is unaffected.
 *
 * WWDT / magic caveats (see also wwdt_hal_get_cpu_id):
 *   - magic invalid -> returns 0xFF; 0xFF & 1 == 1, so core0 traffic would be
 *     silently routed to the core1 slot (wrong core bit, no crash).
 *   - magic valid but cpu_id is a chip-global number (AP empirically reads 0x2
 *     on core0 before/after wwdt_hal_init, not 0). We fold to a 2-slot index
 *     via "& 0x1" (even -> slot 0, odd -> slot 1). This only matches reality if
 *     the SoC assigns global ids with consistent parity per local core — do NOT
 *     assume cpu_id is already 0/1. If parity ever diverges, replace this fold
 *     with an explicit global-id -> slot map. */
extern uint32_t bk_multicore_get_cpu_id(void);
static inline uint32_t dbg_probe_core_id(void)
{
	uint32_t c = bk_multicore_get_cpu_id() & 0x1u;
	return (c < (uint32_t)DBG_PROBE_NUM_CORES) ? c : 0u;
}

/* Write one frame to inst's UART under the proper lock; returns true if the whole
 * frame was handed to the FIFO (false = UART copy dropped). Caller MUST already
 * hold the per-core critical section (IRQs off).
 *   exclusive instance -> direct write (irq-disable is enough);
 *   shared instance    -> skip UART to avoid blocking on cross-core serialization.
 * base==NULL (no wire bound / hot-unplugged) is NOT a drop: the RAM ring still
 * captured the frame, so it returns true. */
static bool dbg_probe_write_uart_locked(dbg_sink_inst_t *inst,
	const uint8_t *frame, uint32_t len)
{
	if (inst == NULL || inst->base == NULL) {
		return true;
	}
	if (inst->shared) {
		return false;
	}
	return dbg_sink_uart_write_base(inst->base, frame, len);
}

/* Tee one frame into BOTH sinks (RAM ring + the given UART instance) under a
 * single per-core critical section. Used for EXPLICIT-core frames only: boot
 * markers and the framework SYNC/DROP frames. `core` is supplied by the caller
 * (never read from the core-id register), and these frames are NOT user-counted
 * and do NOT drive drop accounting (avoids recursion via emit_sync/emit_drop). */
static void dbg_probe_tee(dbg_sink_inst_t *inst, uint32_t core, uint8_t kind,
	const uint8_t *frame, uint32_t len)
{
	uint32_t saved = __get_PRIMASK();

	__disable_irq();
	dbg_probe_ram_write((uint8_t)core, kind, frame, len);
	if (dbg_probe_uart_allowed(kind, (len >= 2u) ? frame[1] : 0u)) {
		(void)dbg_probe_write_uart_locked(inst, frame, len);
	}
	__set_PRIMASK(saved);
}

/* Runtime user-frame tee: resolve the core id INSIDE the critical section so a
 * task migration in the window between dispatch and the IRQ-disable cannot make
 * us write another core's ring. The core bit in frame[0] is back-filled here
 * (builders pass core=0). Drives per-core user counting (sync) and UART-drop
 * accounting; the sync/drop emits happen AFTER the crit section (no recursion). */
static void dbg_probe_tee_rt(uint8_t kind, uint8_t *frame, uint32_t len)
{
	uint32_t saved = __get_PRIMASK();
	uint32_t core;
	uint32_t drop_report = 0u;
	dbg_sink_inst_t *inst;
#if CONFIG_DBG_PROBE_SYNC
	bool do_sync = false;
	uint32_t cnt = 0u;
#endif

	__disable_irq();
	core = dbg_probe_core_id();   /* inside crit: this core can't migrate now */
	frame[0] = (uint8_t)((frame[0] & ~(0x1u << 4)) | ((core & 0x1u) << 4));

	dbg_probe_ram_write((uint8_t)core, kind, frame, len);

	inst = s_core_sink[core];
	if (inst != NULL && inst->base != NULL
		&& dbg_probe_uart_allowed(kind, frame[1])) {
		if (!dbg_probe_write_uart_locked(inst, frame, len)) {
			s_drop_cnt[core]++;
		} else if (s_drop_cnt[core] != 0u) {
			drop_report = s_drop_cnt[core];   /* UART recovered: report+clear */
			s_drop_cnt[core] = 0u;
		}
	}

#if CONFIG_DBG_PROBE_SYNC
	cnt = ++s_frame_cnt[core];
	if ((cnt % (uint32_t)DBG_PROBE_SYNC_EVERY) == 0u) {
		do_sync = true;
	}
#endif

	__set_PRIMASK(saved);

	if (drop_report != 0u) {
		dbg_probe_emit_drop(core, drop_report);
	}
#if CONFIG_DBG_PROBE_SYNC
	if (do_sync) {
		dbg_probe_emit_sync(core, cnt);
	}
#endif
}

static inline void dbg_probe_encode4_core(uint8_t kind, uint8_t module,
	uint16_t value, uint8_t core, uint8_t *b)
{
	b[0] = (uint8_t)(DBG_PROBE_HDR_MAGIC
		| ((core & 0x1u) << 4)
		| (kind & 0x0Fu));                 /* has_ts=0 */
	b[1] = module;
	b[2] = (uint8_t)(value & 0xFFu);
	b[3] = (uint8_t)((value >> 8) & 0xFFu);
}

/* Ver5 value16 builder: 4 bytes (no ts) or 8 bytes with a full 32-bit AON-RTC
 * timestamp in bytes 4..7. Returns the frame length. */
static inline uint32_t dbg_probe_build_v16(uint8_t *b, uint8_t kind, uint8_t module,
	uint16_t value, uint8_t core, bool want_ts)
{
	bool ts = want_ts && dbg_probe_ts_ready();

	b[0] = (uint8_t)(DBG_PROBE_HDR_MAGIC | (ts ? DBG_PROBE_HDR_HAS_TS : 0u)
		| ((core & 0x1u) << 4) | (kind & 0x0Fu));
	b[1] = module;
	b[2] = (uint8_t)(value & 0xFFu);
	b[3] = (uint8_t)((value >> 8) & 0xFFu);
	if (ts) {
		uint32_t t = dbg_probe_ts32();
		b[4] = (uint8_t)(t & 0xFFu);
		b[5] = (uint8_t)((t >> 8) & 0xFFu);
		b[6] = (uint8_t)((t >> 16) & 0xFFu);
		b[7] = (uint8_t)((t >> 24) & 0xFFu);
		return 8u;
	}
	return 4u;
}

/* Ver5 payload32 builder (TASK/U32): always 8 bytes. bytes2..3 = 16-bit short
 * timestamp (AON-RTC low 16 bits) when want_ts, else 0; bytes4..7 = 32-bit
 * payload. Returns 8. */
static inline uint32_t dbg_probe_build_p32(uint8_t *b, uint8_t kind, uint8_t module,
	uint32_t payload, uint8_t core, bool want_ts)
{
	bool ts = want_ts && dbg_probe_ts_ready();
	uint16_t sts = ts ? (uint16_t)(dbg_probe_ts32() & 0xFFFFu) : 0u;

	b[0] = (uint8_t)(DBG_PROBE_HDR_MAGIC | (ts ? DBG_PROBE_HDR_HAS_TS : 0u)
		| ((core & 0x1u) << 4) | (kind & 0x0Fu));
	b[1] = module;
	b[2] = (uint8_t)(sts & 0xFFu);
	b[3] = (uint8_t)((sts >> 8) & 0xFFu);
	b[4] = (uint8_t)(payload & 0xFFu);
	b[5] = (uint8_t)((payload >> 8) & 0xFFu);
	b[6] = (uint8_t)((payload >> 16) & 0xFFu);
	b[7] = (uint8_t)((payload >> 24) & 0xFFu);
	return 8u;
}

/* Pack up to the first 4 ASCII chars of a name into a 32-bit tag, little-endian
 * (first char in the lowest byte -> reads left-to-right in a hex dump). */
static inline uint32_t dbg_probe_pack4(const char *s)
{
	uint32_t t = 0u;

	if (s != NULL) {
		for (uint32_t i = 0; i < 4u && s[i] != '\0'; i++) {
			t |= (uint32_t)(uint8_t)s[i] << (8u * i);
		}
	}
	return t;
}

/* Emit a 4-byte value16 frame through an explicit instance (per-core init
 * markers): the caller knows its own core, so no core-id read is needed. */
static void dbg_probe_emit_inst(dbg_sink_inst_t *inst, uint8_t core,
	uint8_t kind, uint8_t module, uint16_t value)
{
	uint8_t frame[8];
	uint32_t len = dbg_probe_build_v16(frame, kind, module, value, core, false);

	dbg_probe_tee(inst, core, kind, frame, len);   /* markers don't count */
}

#if CONFIG_DBG_PROBE_SYNC
/* Periodic sync frame: U32 kind + SYNC module, payload = per-core frame counter,
 * short timestamp when the RTC is up. Routed through the explicit-core tee so it
 * never recurses into the counter. */
static void dbg_probe_emit_sync(uint32_t core, uint32_t counter)
{
	uint8_t frame[8];
	uint32_t len = dbg_probe_build_p32(frame, DBG_PROBE_KIND_U32, DBG_MOD_SYNC,
		counter, (uint8_t)core, true);

	dbg_probe_tee(s_core_sink[core], core, DBG_PROBE_KIND_U32, frame, len);
}
#endif

/* DROP frame: U32 kind + DBG_MOD_DROP module, payload = number of UART frames
 * dropped on this core since the last DROP report. Routed through the explicit-
 * core tee (no counting / no drop accounting -> no recursion). */
static void dbg_probe_emit_drop(uint32_t core, uint32_t count)
{
	uint8_t frame[8];
	uint32_t len = dbg_probe_build_p32(frame, DBG_PROBE_KIND_U32, DBG_MOD_DROP,
		count, (uint8_t)core, true);

	dbg_probe_tee(s_core_sink[core], core, DBG_PROBE_KIND_U32, frame, len);
}

/* Per-core dispatch (runtime). The core id is resolved inside dbg_probe_tee_rt's
 * critical section; builders here pass core=0 (tee_rt back-fills the core bit). */
static void dbg_probe_emit_rt_v16(uint8_t kind, uint8_t module, uint16_t value, bool want_ts)
{
	uint8_t frame[8];
	uint32_t len = dbg_probe_build_v16(frame, kind, module, value, 0u, want_ts);

	dbg_probe_tee_rt(kind, frame, len);
}

static void dbg_probe_emit_rt_p32(uint8_t kind, uint8_t module, uint32_t payload, bool want_ts)
{
	uint8_t frame[8];
	uint32_t len = dbg_probe_build_p32(frame, kind, module, payload, 0u, want_ts);

	dbg_probe_tee_rt(kind, frame, len);
}

/* Bring up one core's debug port and publish its dispatch slot, then emit a
 * per-core runtime-start marker via the instance (core0->0x52A5, core1->0x52A6,
 * each on its own configured port). `core` is passed by the caller (its own
 * startup), never read from the core-id register here. */
static void dbg_probe_setup_core(uint32_t core, uint8_t port, uint8_t pin, uint32_t baud)
{
	const dbg_uart_hw_desc_t *desc;

	if (core >= (uint32_t)DBG_PROBE_NUM_CORES) {
		return;
	}
	if (!dbg_sink_uart_reconfig(port, pin, baud)) {
		return;   /* unsupported port -> bypass (early UART stays live) */
	}
	desc = dbg_probe_board_uart_desc(port);
	if (desc == NULL || desc->hw == NULL) {
		return;
	}
	dbg_sink_ram_init_core((uint8_t)core, false);   /* preserve early boot frames */
	s_dbg_sink[core].base   = desc->hw;
	s_dbg_sink[core].shared = 0u;        /* Ver4 default: per-core exclusive */
	__DMB();
	s_core_sink[core] = &s_dbg_sink[core];
	__DMB();

	dbg_probe_emit_inst(&s_dbg_sink[core], (uint8_t)core,
		DBG_PROBE_KIND_CUSTOM, DBG_MOD_SYNC,
		(core == 0u) ? 0x52A5u : 0x52A6u);
}

/* Boot/early emit: explicit core, RAM+compile-time UART tee, no timestamp.
 * Caller self-reports core; lock is IRQ-disable only on this core. */
static void dbg_probe_emit_early(uint8_t core, uint8_t kind, uint8_t module, uint16_t value)
{
	uint8_t frame[4];
	uint32_t saved = __get_PRIMASK();

	dbg_probe_encode4_core(kind, module, value, core, frame);
	__disable_irq();
	dbg_probe_ram_write(core, kind, frame, sizeof(frame));
	if (dbg_probe_uart_allowed(kind, module)) {
		dbg_sink_uart_write(frame, sizeof(frame));
	}
	__set_PRIMASK(saved);
}

void dbg_probe_init(void)
{
	dbg_sink_uart_init();
#if CONFIG_DBG_PROBE_RAMRING
	/* Fresh core0 ring for this boot stream; .noinit survives pre-prep writes. */
	dbg_sink_ram_init_core(0u, true);
#endif
	/* Stream-start marker: CUSTOM frame on the framework SYNC module, value
	 * 0xA55A — easy to spot in a raw hex capture (86 F0 5A A5). */
	dbg_probe_emit_early(0u, DBG_PROBE_KIND_CUSTOM, DBG_MOD_SYNC, 0xA55Au);
}

void dbg_probe_cfg_init(dbg_probe_cfg_t *cfg)
{
	if (cfg == NULL) {
		return;
	}
	cfg->struct_size = (uint32_t)sizeof(*cfg);
	cfg->uart_port = 0xFFu;     /* default */
	cfg->uart_tx_pin = 0xFFu;   /* default */
	cfg->flags = 0u;
	cfg->rsv0 = 0u;
	cfg->baud = 0u;             /* default */
}

void dbg_probe_runtime_init(const dbg_probe_cfg_t *cfg)
{
	/* CORE0 path: called from core0 startup (Reset_Handler_Core0), so the core
	 * is known to be 0 — we must NOT read the 0x20000000 core-id here (not yet
	 * stable that early). Brings up the configured core0 port (DBG_PROBE_UART_PORT
	 * / _TX_PIN / _BAUD or cfg overrides; idempotent over the already-live
	 * compile-time port) and emits the 0x52A5 marker on it. */
	uint8_t  port = (uint8_t)DBG_PROBE_UART_PORT;
	uint8_t  pin  = (uint8_t)DBG_PROBE_UART_TX_PIN;
	uint32_t baud = (uint32_t)DBG_PROBE_UART_BAUD;

	/* Honor cfg only if it is at least the size this build knows about (ABI:
	 * older/smaller structs fall back to defaults). 0/0xFF = "use default". */
	if (cfg != NULL && cfg->struct_size >= (uint32_t)sizeof(dbg_probe_cfg_t)) {
		if (cfg->uart_port != 0xFFu) {
			port = cfg->uart_port;
		}
		if (cfg->uart_tx_pin != 0xFFu) {
			pin = cfg->uart_tx_pin;
		}
		if (cfg->baud != 0u) {
			baud = cfg->baud;
		}
	}

	dbg_probe_rtc_force_enable();   /* Ver5: ensure AON RTC ts source is live */
	dbg_probe_setup_core(0u, port, pin, baud);
}

void dbg_probe_runtime_init_core1(void)
{
	/* CORE1 path: called from core1 startup (Reset_Handler_Core1). Core is known
	 * to be 1; brings up the configured core1 port (DBG_PROBE_CORE1_UART_PORT /
	 * _CORE1_TX_PIN / _CORE1_BAUD) and emits the 0x52A6 marker on it. */
	dbg_probe_setup_core(1u,
		(uint8_t)DBG_PROBE_CORE1_UART_PORT,
		(uint8_t)DBG_PROBE_CORE1_TX_PIN,
		(uint32_t)DBG_PROBE_CORE1_BAUD);
}

/* ---- EARLY/boot API (RAM+UART tee, explicit core, no ts) ------------------ */
void dbg_probe_early_stage(uint8_t core, uint16_t stage)
{
	dbg_probe_emit_early(core, DBG_PROBE_KIND_STAGE, DBG_MOD_BOOT, stage);
}

/* ---- Ver5 runtime per-core API (post-RAM; both cores) --------------------- */
void dbg_probe_stage(uint8_t module, uint16_t stage)
{
	dbg_probe_emit_rt_v16(DBG_PROBE_KIND_STAGE, module, stage, false);
}

void dbg_probe_stage_ts(uint8_t module, uint16_t stage)
{
	dbg_probe_emit_rt_v16(DBG_PROBE_KIND_STAGE, module, stage, true);
}

void dbg_probe_irq(uint8_t module, uint16_t irq)
{
	dbg_probe_emit_rt_v16(DBG_PROBE_KIND_IRQ, module, irq, false);
}

void dbg_probe_irq_ts(uint8_t module, uint16_t irq)
{
	dbg_probe_emit_rt_v16(DBG_PROBE_KIND_IRQ, module, irq, true);
}

void dbg_probe_u16(uint8_t module, uint16_t v)
{
	dbg_probe_emit_rt_v16(DBG_PROBE_KIND_U16, module, v, false);
}

void dbg_probe_u16_ts(uint8_t module, uint16_t v)
{
	dbg_probe_emit_rt_v16(DBG_PROBE_KIND_U16, module, v, true);
}

void dbg_probe_exc(uint8_t module, uint16_t code)
{
	dbg_probe_emit_rt_v16(DBG_PROBE_KIND_EXC, module, code, false);
}

void dbg_probe_exc_ts(uint8_t module, uint16_t code)
{
	dbg_probe_emit_rt_v16(DBG_PROBE_KIND_EXC, module, code, true);
}

void dbg_probe_raw(uint8_t kind, uint8_t module, uint16_t v)
{
	dbg_probe_emit_rt_v16(kind, module, v, false);
}

void dbg_probe_raw_ts(uint8_t kind, uint8_t module, uint16_t v)
{
	dbg_probe_emit_rt_v16(kind, module, v, true);
}

void dbg_probe_u32(uint8_t module, uint32_t v)
{
	dbg_probe_emit_rt_p32(DBG_PROBE_KIND_U32, module, v, false);
}

void dbg_probe_u32_ts(uint8_t module, uint32_t v)
{
	dbg_probe_emit_rt_p32(DBG_PROBE_KIND_U32, module, v, true);
}

void dbg_probe_task(uint8_t module, const char *tag)
{
	dbg_probe_emit_rt_p32(DBG_PROBE_KIND_TASK, module, dbg_probe_pack4(tag), false);
}

void dbg_probe_task_ts(uint8_t module, const char *tag)
{
	dbg_probe_emit_rt_p32(DBG_PROBE_KIND_TASK, module, dbg_probe_pack4(tag), true);
}

/* Deprecated Ver4 aliases. */
void dbg_probe_rt_stage(uint8_t module, uint16_t stage)
{
	dbg_probe_stage(module, stage);
}

void dbg_probe_rt_raw(uint8_t kind, uint8_t module, uint16_t v)
{
	dbg_probe_raw(kind, module, v);
}

void dbg_probe_set_shared(bool on)
{
	/* PRECONDITION (test helper, see dbg_probe.h): call while probes are
	 * quiesced — no in-flight bursts on either core. This function takes no
	 * cross-core lock, so a probe racing the switch could briefly see a mixed
	 * (base, shared) pair. The write ORDER below keeps every transient on the
	 * safe side: the `shared` flag is raised BEFORE bases converge and cleared
	 * only AFTER bases diverge, so a racing core may at worst skip one UART write.
	 * The CLI additionally refuses bare shared/excl flips while a burst is running. */
	if (on) {
		/* SHARED: converge BOTH cores onto ONE wire, but CP skips UART writes in
		 * this mode because spinlock has no non-blocking acquire API. */
		const dbg_uart_hw_desc_t *ds =
			dbg_probe_board_uart_desc((uint8_t)DBG_PROBE_SHARED_PORT);
		void *shared_base = (ds != NULL) ? ds->hw : NULL;
		s_dbg_sink[0].shared = 1u;   /* 1) skip model first */
		s_dbg_sink[1].shared = 1u;
		__DMB();                     /* 2) publish flags before bases move */
		s_dbg_sink[0].base   = shared_base;
		s_dbg_sink[1].base   = shared_base;
		__DMB();
	} else {
		/* EXCLUSIVE: restore each core to its own port (irq-disable only). */
		const dbg_uart_hw_desc_t *d0 =
			dbg_probe_board_uart_desc((uint8_t)DBG_PROBE_UART_PORT);
		const dbg_uart_hw_desc_t *d1 =
			dbg_probe_board_uart_desc((uint8_t)DBG_PROBE_CORE1_UART_PORT);
		s_dbg_sink[0].base   = (d0 != NULL) ? d0->hw : NULL;   /* 1) diverge bases */
		s_dbg_sink[1].base   = (d1 != NULL) ? d1->hw : NULL;
		__DMB();                     /* 2) bases settled before dropping the lock model */
		s_dbg_sink[0].shared = 0u;
		s_dbg_sink[1].shared = 0u;
		__DMB();
	}
}

/* ---- Ver9: sink mask + UART module filter API ----------------------------- */
void dbg_probe_set_sink_mask(uint8_t mask)
{
	s_sink_mask = (uint8_t)(mask & DBG_PROBE_SINK_BOTH);
}

uint8_t dbg_probe_get_sink_mask(void)
{
	return s_sink_mask;
}

void dbg_probe_mod_uart_set_all(bool enable)
{
	uint32_t v = enable ? 0xFFFFFFFFu : 0u;

	for (uint32_t i = 0; i < (uint32_t)DBG_PROBE_MOD_MASK_WORDS; i++) {
		s_uart_mod_mask[i] = v;
	}
}

void dbg_probe_mod_uart_set(uint8_t module, bool enable)
{
	uint32_t bit = 1u << (module & 31u);

	if (enable) {
		s_uart_mod_mask[module >> 5] |= bit;
	} else {
		s_uart_mod_mask[module >> 5] &= ~bit;
	}
}

bool dbg_probe_mod_uart_get(uint8_t module)
{
	uint32_t bit = 1u << (module & 31u);

	return (s_uart_mod_mask[module >> 5] & bit) != 0u;
}

void dbg_probe_mod_uart_get_mask(uint32_t out[DBG_PROBE_MOD_MASK_WORDS])
{
	if (out == NULL) {
		return;
	}
	for (uint32_t i = 0; i < (uint32_t)DBG_PROBE_MOD_MASK_WORDS; i++) {
		out[i] = s_uart_mod_mask[i];
	}
}

#endif /* CONFIG_DBG_PROBE */
