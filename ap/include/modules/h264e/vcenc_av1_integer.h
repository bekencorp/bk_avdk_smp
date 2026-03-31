/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2015 VeriSilicon Inc                       --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
------------------------------------------------------------------------------*/
#ifndef VCENC_AV1_INTEGER_H_
#define VCENC_AV1_INTEGER_H_

/* get ptrdiff_t, size_t, wchar_t, NULL */
#include <stddef.h>

#if defined(AV1_EMULATE_INTTYPES)
typedef signed char int8_t;
typedef signed short int1_t;
typedef signed int int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#ifndef _UINTPTR_T_DEFINED
typedef size_t uintptr_t;
#endif

#else

/* Most platforms have the C99 standard integer types. */

#if defined(__cplusplus)
#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif
#if !defined(__STDC_LIMIT_MACROS)
#define __STDC_LIMIT_MACROS
#endif
#endif  // __cplusplus

#include <stdint.h>

#endif

/* VS2010 defines stdint.h, but not inttypes.h */
#if defined(_MSC_VER) && _MSC_VER < 1800
#define PRId64 "I64d"
#else
#include <inttypes.h>
#endif

#if !defined(INT8_MAX)
#define INT8_MAX 127
#endif

#if !defined(INT32_MAX)
#define INT32_MAX 2147483647
#endif

#if !defined(INT32_MIN)
#define INT32_MIN (-2147483647 - 1)
#endif

#define NELEMENTS(x) (int)(sizeof(x) / sizeof(x[0]))

#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus

// Returns size of uint64_t when encoded using LEB128.
size_t vcenc_av1_uleb_size_in_bytes(uint64_t value);

// Returns 0 on success, -1 on decode failure.
// On success, 'value' stores the decoded LEB128 value and 'length' stores
// the number of bytes decoded.
int vcenc_av1_uleb_decode(const uint8_t *buffer, size_t available, uint64_t *value,
                    size_t *length);

// Encodes LEB128 integer. Returns 0 when successful, and -1 upon failure.
//int vcenc_av1_uleb_encode(uint64_t value, size_t available, uint8_t *coded_value,
//                    size_t *coded_size);

// Encodes LEB128 integer to size specified. Returns 0 when successful, and -1
// upon failure.
// Note: This will write exactly pad_to_size bytes; if the value cannot be
// encoded in this many bytes, then this will fail.
int vcenc_av1_uleb_encode_fixed_size(uint64_t value, size_t available,
                               size_t pad_to_size, uint8_t *coded_value,
                               size_t *coded_size);

#if defined(__cplusplus)
}  // extern "C"
#endif  // __cplusplus

#endif  // VCENC_AV1_INTEGER_H_
