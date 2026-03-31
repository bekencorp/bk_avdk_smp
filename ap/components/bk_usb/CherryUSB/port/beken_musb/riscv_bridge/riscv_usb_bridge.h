#ifndef RISCV_USB_BRIDGE_H
#define RISCV_USB_BRIDGE_H

#include <stdint.h>

#if CONFIG_USB_RISCV_BRIDGE

#ifdef __cplusplus
extern "C" {
#endif

void usb_hc_riscv_start_core(uint32_t reset_vec);
int usb_hc_riscv_start_firmware(const unsigned char *fw, unsigned int fw_len, uint32_t reset_vec);
void usb_hc_riscv_stop_firmware(void);
int usb_hc_riscv_host_prepare(void);

#ifdef __cplusplus
}
#endif

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

#endif

#endif
