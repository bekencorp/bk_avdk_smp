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


#ifndef _OSAL_H_
#define _OSAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__FREERTOS__) || defined(_WIN32) || defined(__linux__)

#if defined(__GNUC__) || defined(__clang__)
#define av_unused __attribute__((unused))
#else
#define av_unused
#endif

// #ifdef __FREERTOS__    //7259debug@zhilei
// #include "osal_freertos.h"
// #elif defined(__linux__)
// #include "osal_linux.h"
// #elif defined(_WIN32)
// #include "osal_win32.h"
// #endif
#include "osal_freertos.h"

#ifdef _HAVE_PTHREAD_H
/*For static link pthread in win32, need init some global variables to prevent crash*/
#ifdef PTW32_STATIC_LIB
static void detach_ptw32(void) {
  pthread_win32_thread_detach_np();
  pthread_win32_process_detach_np();
}
#endif

static av_unused void osal_thread_init()
{
#ifdef PTW32_STATIC_LIB
  pthread_win32_process_attach_np();
  pthread_win32_thread_attach_np();
  atexit(detach_ptw32);
#endif
}
#endif /* _HAVE_PTHREAD_H */

#endif /* defined(__FREERTOS__) || defined(__linux__) || defined(_WIN32) */

#define gettimeofday                osal_gettimeofday
// #define usleep                      osal_usleep
// #define open_memstream              osal_open_memstream

#ifdef __linux__
#define OSAL_STRM_POS(A) (A).__pos
#else
#define OSAL_STRM_POS(A) (A)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_H_ */
