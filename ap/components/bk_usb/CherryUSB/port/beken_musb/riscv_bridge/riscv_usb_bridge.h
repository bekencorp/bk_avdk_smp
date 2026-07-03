<<<<<<< HEAD   (b6736b [Jira BK7259SW-1794]: <multimedia> <fix> fix uvc output imag)
=======
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_USB_RISCV_BRIDGE

#include "modules/bk_riscv_types.h"
volatile riscv_usb_probe_t *get_riscv_usb_probe(void);
void riscv_usb_probe_init(void);
void riscv_usb_probe_cache_clean(void);

void usb_hc_riscv_start_core(uint32_t reset_vec);
int usb_hc_riscv_start_firmware(const unsigned char *fw, unsigned int fw_len, uint32_t reset_vec);
void usb_hc_riscv_stop_firmware(void);
int usb_hc_riscv_host_prepare(void);

/* Device-side entry point for the RISC-V USB bridge.
 *
 * MILESTONE A status: scaffolding only. Always returns -1 so the caller
 * (usb_dc_low_level_init() in usb_dc_beken_musb_mhdrc.c) falls back to
 * the legacy M55-resident USBD_IRQHandler path. Device traffic stays
 * 100% on M55 until the follow-up milestone wires up:
 *   - dual-role / device firmware in
 *     ap/properties/modules/bk_riscv/riscv_src/fw/usb_dual/
 *   - shared-memory device region (riscv_usb_probe_t device fields)
 *   - AP-side IPI poll loop usb_dc_riscv_poll_events()
 *
 * See docs/USB重构/07-M55_RISCV_USB桥设计.md §5 for the staging plan.
 *
 * Returns 0 on success (RISC-V firmware took over USBD IRQ), <0 on
 * failure (caller MUST keep registering USBD_IRQHandler on the M55).
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
>>>>>>> CHANGE (99d2ad [Jira BK7259SW-1431]: <multimedia> <fix> RISC-V USB bridge S)
