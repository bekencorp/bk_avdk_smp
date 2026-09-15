#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_USB_RISCV_BRIDGE

#include "modules/bk_riscv_types.h"
volatile riscv_usb_probe_t *get_riscv_usb_probe(void);
void riscv_usb_probe_init(void);

void usb_hc_riscv_start_core(uint32_t reset_vec);
int usb_hc_riscv_start_firmware(const unsigned char *fw, unsigned int fw_len, uint32_t reset_vec);
void usb_hc_riscv_stop_firmware(void);
int usb_hc_riscv_host_prepare(void);

/* Device-side entry point for the RISC-V USB bridge.
 *
 * Starts the dual-role RISC-V firmware so it owns the AP-side USBD IRQ; device
 * events reach the CherryUSB core via the shared-memory probe + IPI poll loop
 * (usb_dc_riscv_poll_events()). The caller (usb_dc_low_level_init() in
 * usb_dc_beken_musb_mhdrc.c) skips the M55 USBD_IRQHandler when this succeeds.
 *
 * Returns 0 on success (RISC-V firmware took over USBD IRQ), <0 on failure --
 * e.g. firmware image missing (caller MUST keep the M55-resident USBD_IRQHandler
 * as the fallback).
 */
int usb_dc_riscv_device_prepare(void);

#else

static inline void usb_hc_riscv_start_core(uint32_t reset_vec)
{
    (void)reset_vec;
}

static inline int usb_hc_riscv_start_firmware(const unsigned char *fw, unsigned int fw_len, uint32_t reset_vec)
{
    (void)fw;
    (void)fw_len;
    (void)reset_vec;
    return -1;
}

static inline void usb_hc_riscv_stop_firmware(void)
{
}

static inline int usb_hc_riscv_host_prepare(void)
{
    return -1;
}

static inline int usb_dc_riscv_device_prepare(void)
{
    return -1;
}

#endif

#ifdef __cplusplus
}
#endif