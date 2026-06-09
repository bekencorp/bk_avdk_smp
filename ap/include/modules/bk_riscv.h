#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_include.h>

const unsigned char *bk_riscv_usb_fw_addr(void);
const unsigned int   bk_riscv_usb_fw_len(void);

static inline const unsigned char *bk_riscv_usb_host_fw_addr(void) { return bk_riscv_usb_fw_addr(); }
static inline const unsigned int   bk_riscv_usb_host_fw_len(void)  { return bk_riscv_usb_fw_len(); }

#ifdef __cplusplus
}
#endif
