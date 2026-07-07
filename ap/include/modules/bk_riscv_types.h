#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Must match sys_sw_regs_shared.h / RISC-V firmware */
#define RISCV_USB_PROBE_PIPE_NUM 16U

/* ---- SPSC event ring for host USB completions ----
 * RISC-V is the single producer (advances evt_wr); the AP (M55) is the single
 * consumer (advances evt_rd). This replaces the host pending_pipe_* and event
 * level signalling. An edge-triggered RISC-V->M55 IPI has no handshake: a 2nd IPI
 * raised before the AP entered the 1st ISR is hardware-merged, so with level
 * flags the AP could service the merged state once and lose a completion. The
 * ring records every completion as its own entry, so the AP drains
 * evt_rd..evt_wr and never misses one even when the IPI coalesced. Each entry
 * is (type << 8) | (ep & 0xFF). */
#define RISCV_USB_EVT_RING_SIZE 128U   /* power of 2 */
#define RISCV_USB_EVT_RING_MASK (RISCV_USB_EVT_RING_SIZE - 1U)
#define RISCV_USB_EVT_TYPE_EP0   1U
#define RISCV_USB_EVT_TYPE_TX    2U
#define RISCV_USB_EVT_TYPE_RX    3U
#define RISCV_USB_EVT_TYPE_CONN  4U
#define RISCV_USB_EVT_TYPE_DISC  5U
#define RISCV_USB_EVT_MAKE(type, ep) (((uint32_t)(type) << 8) | ((uint32_t)(ep) & 0xFFU))
#define RISCV_USB_EVT_TYPE(e)        (((uint32_t)(e) >> 8) & 0xFFU)
#define RISCV_USB_EVT_EP(e)          ((uint32_t)(e) & 0xFFU)

/* dual-role: which view the firmware ISR dispatches to.
 * NONE is the default after a zeroed handshake region; the firmware treats
 * NONE as HOST so legacy AP builds that never set role keep working. */
#define RISCV_USB_ROLE_NONE            0U
#define RISCV_USB_ROLE_HOST            1U
#define RISCV_USB_ROLE_DEVICE          2U

typedef struct {
    volatile uint32_t magic;
    volatile uint32_t owner;
    volatile uint32_t irq_seq;
    volatile uint32_t event;
    volatile uint32_t event_data;
    volatile uint32_t g_musb_hcd_addr;
    volatile uint32_t usb_ep0_state_addr;
    volatile uint32_t pending_ep0;
    volatile uint32_t pending_pipe_tx[RISCV_USB_PROBE_PIPE_NUM];
    volatile uint32_t pending_pipe_rx[RISCV_USB_PROBE_PIPE_NUM];
    /* dual-role dispatch selector (RISCV_USB_ROLE_*). Appended at the tail so
     * existing field offsets stay stable; 0 (NONE) is treated as HOST by the
     * firmware, so AP builds that never write this field keep host behaviour. */
    volatile uint32_t role;
    /* device role: AP pointer to its struct musb_udc. The RISC-V firmware casts
     * it to musb_udc_t (mirror in riscv_usb_bridge.h) and reads/writes the EP
     * xfer state + SETUP packet in place, mirroring how host uses
     * g_musb_hcd_addr. 0 until the device path is started (gap B). */
    volatile uint32_t g_musb_udc_addr;
    /* device role: batched ISR-drain pending state, mirroring the host
     * pending_ep0 / pending_pipe_* mechanism. The firmware ACCUMULATES these
     * flags during one ISR and raises a single RISCV_USBD_EVT_ISR_DRAIN; the AP
     * poll snapshots+clears them and replays every set flag. Because the state
     * lives in persistent per-item flags (not the single overwritable
     * event/event_data slot), N coalesced IPIs still deliver all N events --
     * this is what lets back-to-back EP0 control transactions survive
     * enumeration. pending_usbd_evt is a bitmask (RISCV_USBD_PEND_*); index 0 of
     * the EP arrays is EP0. */
    volatile uint32_t pending_usbd_evt;
    volatile uint32_t pending_setup;
    volatile uint32_t pending_ep_in[RISCV_USB_PROBE_PIPE_NUM];
    volatile uint32_t pending_ep_out[RISCV_USB_PROBE_PIPE_NUM];
    /* SPSC host-completion event ring (see the macros' comment above). Appended
     * at the tail so existing field offsets stay stable for any cached layout. */
    volatile uint32_t evt_ring[RISCV_USB_EVT_RING_SIZE];
    volatile uint32_t evt_wr;    /* producer cursor: RISC-V advances only */
    volatile uint32_t evt_rd;    /* consumer cursor: AP advances only */
    volatile uint32_t evt_drop;  /* producer-side overflow counter (diagnostic) */
} riscv_usb_probe_t;

/* Cross-core mutual exclusion for the shared MUSB EPIDX indexed-register window.
 *
 * Both the AP (M55) URB (re)arm path and the CP (RISC-V) USB ISR select an
 * endpoint via the single EPIDX selector and then touch indexed registers. When
 * two ISO IN endpoints stream concurrently (e.g. UVC video + UAC mic) the AP's
 * ~1kHz re-arm can move EPIDX mid-sequence in the CP ISR, steering the CP's
 * REQPKT re-arm to the wrong endpoint and permanently stalling the other stream.
 *
 * This is now serialised with the BK7259 hardware spin lock (HSPL) instead of a
 * software Peterson lock. BK_HSPL_RES_USB (resource id 29) maps to HSPL block 1
 * (base SOC_HSPL1_REG_BASE = 0x480C0000) channel 13 (29 - 16). Owner identity is
 * assigned by the bus master that reads the LOCK register, so M55 and the RISC-V
 * CP contend correctly even though they share the same channel address.
 *
 * The AP takes it through the hspl driver (bk_hspl_try_lock/bk_hspl_unlock on
 * BK_HSPL_ID_1 channel RISCV_USB_HSPL_CHANNEL). The RISC-V CP firmware has no
 * hspl driver, so it drives the same hardware channel directly through the
 * register helpers below. */
#define RISCV_USB_HSPL_BASE         0x480C0000U   /* SOC_HSPL1_REG_BASE */
#define RISCV_USB_HSPL_CHANNEL      13U           /* BK_HSPL_RES_USB(29) - 16 */
#define RISCV_USB_HSPL_LOCK_WOFFS   0x10U         /* word offset of LOCK0 */
#define RISCV_USB_HSPL_LOCK_OK      0x1U          /* LOCK read == 1 -> acquired */
#define RISCV_USB_HSPL_UNLOCK_MAGIC 0xA55A80AFU   /* write to LOCK to release */

#if defined(__riscv)
static inline volatile uint32_t *riscv_usb_hspl_lock_reg(void)
{
    return (volatile uint32_t *)(uintptr_t)(RISCV_USB_HSPL_BASE +
        (RISCV_USB_HSPL_LOCK_WOFFS + RISCV_USB_HSPL_CHANNEL) * 4U);
}

/* Spin until the hardware grants the channel. Reading the LOCK register *is* the
 * try-lock; it returns 1 on a successful unlock->lock and 6'b1xxxx0 (bit0=0)
 * while another master owns it. The AP holds it only for a short indexed-reg
 * window, so the spin is bounded in practice. */
static inline void riscv_usb_hspl_lock(void)
{
    volatile uint32_t *reg = riscv_usb_hspl_lock_reg();

    /* The AP (M55) holds the EPIDX window for only a short indexed-register
     * sequence, so this spin is bounded in practice. Never force-steal the
     * channel: that lets the AP and this ISR drive EPIDX concurrently and
     * corrupts the transfer. */
    while (((*reg) & RISCV_USB_HSPL_LOCK_OK) == 0U) {
        /* spin: the AP holds the EPIDX window */
    }
    __asm__ volatile ("fence rw, rw" ::: "memory");
}

static inline void riscv_usb_hspl_unlock(void)
{
    volatile uint32_t *reg = riscv_usb_hspl_lock_reg();

    __asm__ volatile ("fence rw, rw" ::: "memory");
    *reg = RISCV_USB_HSPL_UNLOCK_MAGIC;
}
#endif /* __riscv */


#ifdef __cplusplus
}
#endif
