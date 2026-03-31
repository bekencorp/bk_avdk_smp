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

#ifndef __VSIOS_TYPE_H__
#define __VSIOS_TYPE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include <stdbool.h>

typedef unsigned char      vsi_u8_t;
typedef signed char        vsi_s8_t;
typedef unsigned short     vsi_u16_t;
typedef signed short       vsi_s16_t;
typedef unsigned int       vsi_u32_t;
typedef signed int         vsi_s32_t;
typedef unsigned long long vsi_u64_t;
typedef long long          vsi_s64_t;
typedef unsigned int       vsi_dma_t;
typedef unsigned int       vsi_phy_t;
typedef unsigned int       vsi_reg_t;

typedef bool               vsi_bool_t;

#define VSI_SUCCESS        0
#define VSI_FAILURE        (-1)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
