#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <os/os.h>

#include <driver/i2c_types.h>

typedef struct
{
    int (*i2c_init)(uint32_t id);
    int (*i2c_deinit)(uint32_t id);
    int (*i2c_write)(uint32_t id, uint32_t addr, uint32_t data, uint8_t reg_bytes);
    uint32_t (*i2c_read)(uint32_t id, uint32_t addr, uint8_t reg_bytes);
} bk_isp_i2c_funcs_t;

bk_err_t bk_isp_i2c_funcs_init(void);

#ifdef __cplusplus
}
#endif
