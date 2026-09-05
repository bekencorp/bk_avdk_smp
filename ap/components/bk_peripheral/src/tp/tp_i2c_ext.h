#pragma once

#include <stdint.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t tp_i2c_wr_reg(uint8_t dev_addr, uint32_t reg_addr, uint8_t reg_len,
                       uint8_t *rbuf, uint16_t rlen);
bk_err_t tp_i2c_read_raw(uint8_t dev_addr, uint8_t *buf, uint16_t len);

#ifdef __cplusplus
}
#endif
