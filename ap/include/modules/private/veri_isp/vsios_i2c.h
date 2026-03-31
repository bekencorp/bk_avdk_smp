/****************************************************************************
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2024 Vivante Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#ifndef __VSIOS_I2C_H__
#define __VSIOS_I2C_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "vsios_type.h"

typedef struct {
    vsi_u8_t slave_addr;
    vsi_u8_t reg_bytes;
    vsi_u8_t data_bytes;
} vsios_i2c_attr_t;

int vsios_i2c_sys_init(vsi_u32_t dev_id);
int vsios_i2c_sys_exit(vsi_u32_t dev_id);
int vsios_i2c_write(vsi_u32_t dev_id, vsios_i2c_attr_t *i2c_attr,
                    vsi_u32_t addr, vsi_u32_t data);
vsi_u32_t vsios_i2c_read(vsi_u32_t dev_id, vsios_i2c_attr_t *i2c_attr,
                    vsi_u32_t addr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif