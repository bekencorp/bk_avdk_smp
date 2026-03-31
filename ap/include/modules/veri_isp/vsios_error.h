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

#ifndef __VSIOS_ERROR_H__
#define __VSIOS_ERROR_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

typedef enum VsiErrCode_e
{
    VSI_ERR_INVALID_DEVID     = 1,
    VSI_ERR_INVALID_PORTID    = 2,
    VSI_ERR_INVALID_CHNID     = 3,
    VSI_ERR_NULL_PTR          = 4,
    VSI_ERR_ILLEGAL_PARAM     = 5,
    VSI_ERR_NOT_CONFIG        = 6,
    VSI_ERR_NOT_SUPPORT       = 7,
    VSI_ERR_NOT_PERM          = 8,
    VSI_ERR_NOT_READY         = 9,
    VSI_ERR_EXIST             = 10,
    VSI_ERR_UNEXIST           = 11,
    VSI_ERR_BUF_EMPTY         = 12,
    VSI_ERR_BUF_FULL          = 13,
    VSI_ERR_NOMEM             = 14,
    VSI_ERR_NOBUF             = 15,
    VSI_ERR_TIMEOUT           = 16,
    VSI_ERR_BUSY              = 17,
} VsiErrCode_t;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
