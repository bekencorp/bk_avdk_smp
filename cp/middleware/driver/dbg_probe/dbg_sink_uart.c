// Copyright 2026 Beken
//
// dbg_probe UART direct-write backend (CP). Register sequence ported from
// uart5_reg_debug (reference), adapted to the release SDK register structs.
// Early path = compile-time constants (no data-memory dependency); runtime path
// (Ver3.5) = flash-const table + weak board hook. Both share dbg_sink_uart_apply.

#include "dbg_sink_uart.h"

#if CONFIG_DBG_PROBE

#include <stdint.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "dbg_probe_cfg.h"
#include <soc/soc.h>
#include <soc/bk7259/reg_base.h>
#include "uart_hw.h"
#include "sys_struct.h"

/* ---- Compile-time port -> hardware resource mapping (EARLY/pre-RAM path).
 *   Regular UART block follows a fixed pattern, all 坐实 against the SDK headers:
 *     func_tx  = 96 + 2*n   (hal_io_matrix_types_impl.h: UART0..4_TXD=96/98/100/102/104)
 *     cken bit = 4 + n      (sys_struct.h: uartN_cken     = bit 4/5/6/7/8)
 *     cksel bit= 2 + n      (sys_struct.h: cksel_uartN    = bit 2/3/4/5/6)
 *     base     = SOC_UARTn_REG_BASE  (non-contiguous, must use the per-port macro)
 *   So we derive func/cken/cksel by formula and only switch the (non-contiguous)
 *   base + power domain per port.
 *   NOTE: UART5 is NOT supported here — it has no FUNC_CODE_UART5_TXD in the IO
 *   matrix and is the dedicated debug UART (uart5_reg_debug, base 0x4811_0000)
 *   with a different bring-up; route it via the runtime board hook instead.
 *   U1..U4 all live in the CPU1 power domain (per user, 2026-06-10). --------- */
#if   DBG_PROBE_UART_PORT == 0
#  define DBG_SINK_UART_BASE   SOC_UART0_REG_BASE
#  define DBG_SINK_UART_PWR    DBG_UART_PWR_NONE
#elif DBG_PROBE_UART_PORT == 1
#  define DBG_SINK_UART_BASE   SOC_UART1_REG_BASE
#  define DBG_SINK_UART_PWR    DBG_UART_PWR_CPU1
#elif DBG_PROBE_UART_PORT == 2
#  define DBG_SINK_UART_BASE   SOC_UART2_REG_BASE
#  define DBG_SINK_UART_PWR    DBG_UART_PWR_CPU1
#elif DBG_PROBE_UART_PORT == 3
#  define DBG_SINK_UART_BASE   SOC_UART3_REG_BASE
#  define DBG_SINK_UART_PWR    DBG_UART_PWR_CPU1
#elif DBG_PROBE_UART_PORT == 4
#  define DBG_SINK_UART_BASE   SOC_UART4_REG_BASE
#  define DBG_SINK_UART_PWR    DBG_UART_PWR_CPU1
#else
#  error "dbg_probe: early port must be UART0..4 (UART5 has no IO-matrix TXD func; use the runtime board hook)."
#endif

#define DBG_SINK_UART_FUNC    (96u + 2u * (DBG_PROBE_UART_PORT))
#define DBG_SINK_UART_CKEN    (4u + (DBG_PROBE_UART_PORT))
#define DBG_SINK_UART_CKSEL   (2u + (DBG_PROBE_UART_PORT))

/* Caller-guaranteed: 0 < DBG_PROBE_UART_BAUD <= DBG_PROBE_UART_SRC_CLK_HZ.
 * No range/rounding handling here (integer divide, truncates) — picking a baud
 * the source clock can't divide cleanly is the integrator's responsibility. */
#define DBG_SINK_UART_CLK_DIV     ((DBG_PROBE_UART_SRC_CLK_HZ / DBG_PROBE_UART_BAUD) - 1u)
#define DBG_SINK_UART_TX_FIFO_THR 0x20u
#define DBG_SINK_UART_RX_FIFO_THR 0x40u
/* Bounded busy-wait on a full TX FIFO. Kept small (256): the whole frame is
 * written inside an IRQ-disabled critical section, so the worst-case IRQ-off
 * latency is len * this cap. On expiry the byte (and rest of frame) is dropped
 * and the per-core drop counter advances (reported later via a DROP frame). */
#define DBG_SINK_UART_TX_WAIT_MAX (DBG_PROBE_UART_DROP_ON_FULL ? 256u : 0xFFFFFFFFu)

/* Software settle delays (no status bit exposed for these on this SoC, so we
 * spin a fixed, empirically-sufficient number of CPU cycles). volatile loop
 * counter => not optimized away. */
#define DBG_SINK_UART_PWR_SETTLE   30000u   /* after enabling the power domain */
#define DBG_SINK_UART_CLK_SETTLE   10000u   /* after gating on the UART clock */
#define DBG_SINK_UART_RST_SETTLE   1000u    /* after global soft-reset pulse */

/* GPIO config register fields (AON_GPIO[pin]) */
#define DBG_SINK_GPIO_FUN_SEL_SHIFT 24u
#define DBG_SINK_GPIO_PULL_UP_CFG   ((1u << 5) | (1u << 4))
#define DBG_SINK_GPIO_DRIVE_CAP_3   (3u << 8)

#define DBG_SINK_UART_HW   ((volatile uart_hw_t *)DBG_SINK_UART_BASE)
#define DBG_SINK_SYS_HW    ((volatile sys_hw_t *)SOC_SYS_REG_BASE)
#define DBG_SINK_SYS_R10   ((volatile sys_reserver_reg0x10_t *)(SOC_SYS_REG_BASE + (0x10u << 2)))

/* ---- Shared register-apply (all params by value -> stack/regs only, so the
 *      early caller stays data-memory-independent). ---------------------------*/
static void dbg_sink_uart_apply(volatile uart_hw_t *hw, uint8_t tx_pin, uint16_t func_tx,
	uint8_t cken_bit, uint8_t cksel_bit, uint8_t pwr_domain, uint32_t clk_div)
{
	volatile uint32_t *gpio;
	volatile sys_hw_t *sys = DBG_SINK_SYS_HW;
	uint32_t cfg;

#if DBG_PROBE_UART_FORCE_POWER
	if (pwr_domain == DBG_UART_PWR_CPU1) {
		DBG_SINK_SYS_R10->pwd_cpu1 = 0;          /* UART power domain in CPU1 */
		for (volatile uint32_t d = 0; d < DBG_SINK_UART_PWR_SETTLE; d++) {
		}
	}
#else
	(void)pwr_domain;   /* AP: this domain is guaranteed already-on by CP */
#endif

	gpio = (volatile uint32_t *)(SOC_AON_GPIO_REG_BASE + ((uint32_t)tx_pin * 4u));
	*gpio = 0u;
	*gpio = ((uint32_t)func_tx << DBG_SINK_GPIO_FUN_SEL_SHIFT)
		| DBG_SINK_GPIO_DRIVE_CAP_3 | DBG_SINK_GPIO_PULL_UP_CFG;

#if DBG_PROBE_UART_FORCE_CLOCK
	sys->cpu_clk_div_mode2.v &= ~(1u << cksel_bit);    /* clk source select = 0 */
	sys->cpu_device_clk_enable.v |= (1u << cken_bit);   /* gate on UART clock */
	for (volatile uint32_t d = 0; d < DBG_SINK_UART_CLK_SETTLE; d++) {
	}
#else
	(void)sys; (void)cken_bit; (void)cksel_bit;
#endif

	hw->global_ctrl.soft_reset = 1;
	for (volatile uint32_t t = 0; t < DBG_SINK_UART_RST_SETTLE; t++) {
	}

	hw->int_enable.v = 0;
	hw->int_status.v = 0xff;

	cfg = (clk_div << 8) | (DBG_PROBE_UART_STOP_BITS << 7)
		| (DBG_PROBE_UART_DATA_BITS << 3) | 0x3u;
	hw->config.v = cfg;

	hw->fifo_config.v = 0;
	hw->fifo_config.tx_fifo_threshold = DBG_SINK_UART_TX_FIFO_THR;
	hw->fifo_config.rx_fifo_threshold = DBG_SINK_UART_RX_FIFO_THR;
	hw->fifo_config.rx_stop_detect_time = 0;

	hw->flow_ctrl_config.v = 0;
	hw->wake_config.v = 0;
}

void dbg_sink_uart_init(void)
{
	/* EARLY path: every argument is a compile-time constant. */
	dbg_sink_uart_apply(DBG_SINK_UART_HW, DBG_PROBE_UART_TX_PIN, DBG_SINK_UART_FUNC,
		DBG_SINK_UART_CKEN, DBG_SINK_UART_CKSEL, DBG_SINK_UART_PWR, DBG_SINK_UART_CLK_DIV);
}

/* ---- Runtime per-port table + weak board hook (Ver3.5). Flash .rodata. -------*/
#define DBG_UART_HW_COUNT 6u
static const dbg_uart_hw_desc_t k_dbg_uart_hw[DBG_UART_HW_COUNT] = {
	[0] = { (void *)SOC_UART0_REG_BASE, 96u, 4u, 2u, DBG_UART_PWR_NONE },
	[1] = { (void *)SOC_UART1_REG_BASE, 98u, 5u, 3u, DBG_UART_PWR_CPU1 },
	/* U2 (Ver4 core1 debug port): UART2_TXD func=100, cken bit6, cksel bit4
	 * (sys_struct.h). CPU1 power domain — always on when core1 (==CPU1) runs. */
	[2] = { (void *)SOC_UART2_REG_BASE, 100u, 6u, 4u, DBG_UART_PWR_CPU1 },
	/* U3/U4: same regular-block pattern (func=96+2n, cken=4+n, cksel=2+n), all
	 * 坐实 against hal_io_matrix_types_impl.h / sys_struct.h. Per user (2026-06-10)
	 * they share the SAME power domain as U1/U2 -> CPU1. */
	[3] = { (void *)SOC_UART3_REG_BASE, 102u, 7u, 5u, DBG_UART_PWR_CPU1 },
	[4] = { (void *)SOC_UART4_REG_BASE, 104u, 8u, 6u, DBG_UART_PWR_CPU1 },
	/* U5: dedicated debug UART (uart5_reg_debug), no IO-matrix TXD func -> NULL;
	 * supply it via a board override of dbg_probe_board_uart_desc(). */
};

const dbg_uart_hw_desc_t *__attribute__((weak)) dbg_probe_board_uart_desc(uint8_t port)
{
	if (port >= DBG_UART_HW_COUNT || k_dbg_uart_hw[port].hw == NULL) {
		return NULL;
	}
	return &k_dbg_uart_hw[port];
}

bool dbg_sink_uart_reconfig(uint8_t port, uint8_t tx_pin, uint32_t baud)
{
	const dbg_uart_hw_desc_t *d = dbg_probe_board_uart_desc(port);
	uint32_t clk_div;

	if (d == NULL || d->hw == NULL || baud == 0u) {
		return false;                            /* safe no-op, never bricks */
	}
	/* Idempotent: if the request matches what the early (compile-time) path
	 * already brought up, the port is live and transmitting — do NOT tear it
	 * down. A soft-reset here would flush the just-queued frame (and any byte
	 * still shifting out). Report success without touching the hardware. */
	if (port == (uint8_t)DBG_PROBE_UART_PORT
		&& tx_pin == (uint8_t)DBG_PROBE_UART_TX_PIN
		&& baud == (uint32_t)DBG_PROBE_UART_BAUD) {
		return true;
	}
	clk_div = (DBG_PROBE_UART_SRC_CLK_HZ / baud) - 1u;
	dbg_sink_uart_apply((volatile uart_hw_t *)d->hw, tx_pin, d->func_tx,
		d->cken_bit, d->cksel_bit, d->pwr_domain, clk_div);
	return true;
}

static inline bool dbg_sink_uart_putc(volatile uart_hw_t *hw, uint8_t ch)
{
	uint32_t cnt = 0;

	while (!hw->fifo_status.fifo_wr_ready) {
		if (++cnt > DBG_SINK_UART_TX_WAIT_MAX) {
			return false;                       /* never dead-wait */
		}
	}
	hw->fifo_port.v = ch;
	return true;
}

void dbg_sink_uart_write(const uint8_t *buf, uint32_t len)
{
	volatile uart_hw_t *hw = DBG_SINK_UART_HW;   /* compile-time const base */

	for (uint32_t i = 0; i < len; i++) {
		if (!dbg_sink_uart_putc(hw, buf[i])) {
			break;
		}
	}
}

/* Ver4: write to an arbitrary resolved base (per-core instance dispatch).
 * base==NULL -> nothing to do (offline instance), reported as success so the
 * caller doesn't count it as a UART drop. Returns false only when the TX FIFO
 * stayed full and one or more bytes were dropped. */
bool dbg_sink_uart_write_base(void *base, const uint8_t *buf, uint32_t len)
{
	volatile uart_hw_t *hw = (volatile uart_hw_t *)base;

	if (hw == NULL) {
		return true;
	}
	for (uint32_t i = 0; i < len; i++) {
		if (!dbg_sink_uart_putc(hw, buf[i])) {
			return false;
		}
	}
	return true;
}

#endif /* CONFIG_DBG_PROBE */
