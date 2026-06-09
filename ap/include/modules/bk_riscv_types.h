#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Must match sys_sw_regs_shared.h / RISC-V firmware */
#define RISCV_USB_PROBE_PIPE_NUM 16U

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
} riscv_usb_probe_t;


#ifdef __cplusplus
}
#endif
