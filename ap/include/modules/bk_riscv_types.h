#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Must match sys_sw_regs_shared.h / RISC-V firmware */
#define RISCV_USB_PROBE_PIPE_NUM 16U

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
} riscv_usb_probe_t;


#ifdef __cplusplus
}
#endif
