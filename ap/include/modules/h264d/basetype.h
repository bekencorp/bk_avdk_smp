/*------------------------------------------------------------------------------
--         Copyright (c) 2015, VeriSilicon Inc. All rights reserved           --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/


/*!\file
 * \brief Basic type definitions.
 *
 * Basic numeric data type definitions used in the decoder software.
 */


#ifndef __BASETYPE_H__
#define __BASETYPE_H__

/*! \addtogroup common Common definitions
 *  @{
 */

#if defined( __linux__ ) || defined( WIN32 )
#include <stddef.h>
#endif

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

/* Macro to signal unused parameter. */
#define UNUSED(x) (void)(x)

typedef unsigned char u8; /**< unsigned 8 bits integer value */
typedef signed char i8; /**< signed 8 bits integer value */
typedef unsigned short u16; /**< unsigned 16 bits integer value */
typedef signed short i16; /**< signed 16 bits integer value */
typedef unsigned int u32; /**< unsigned 32 bits integer value */
typedef signed int i32; /**< signed 8 bits integer value */

typedef long long  i64;
typedef unsigned long long u64;


#if defined(_WIN64)
typedef unsigned long long g1_addr_t;
#else
typedef unsigned long g1_addr_t; /**< unsigned 64 bits integer value */
//typedef size_t g1_addr_t;
#endif
typedef g1_addr_t addr_t;


/*!\cond SWDEC*/
/* SW decoder 16 bits types */
#if defined(VC1SWDEC_16BIT) || defined(MP4ENC_ARM11)
typedef unsigned short u16x;
typedef signed short i16x;
#else
typedef unsigned int u16x;
typedef signed int i16x;
#endif
/*!\endcond */

/*! @} - end group common */

#endif /* __BASETYPE_H__ */
