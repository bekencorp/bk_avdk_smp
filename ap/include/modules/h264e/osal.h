/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2019 VeriSilicon Inc                       --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
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

typedef struct {
  beken_mutex_t mutex;
} vsios_mutex_t;

typedef struct {
  beken_semaphore_t sem;
} vsios_sem_t;

typedef struct {
  beken_thread_t thread;
} vsios_thread_t;

#ifdef __FREERTOS__
#include "osal_freertos.h"
#elif defined(__linux__)
#include "osal_linux.h"
#elif defined(_WIN32)
#include "osal_win32.h"
#endif

#ifdef _HAVE_PTHREAD_H
//For static link pthread in win32, need init some global variables to prevent crash
#ifdef PTW32_STATIC_LIB
static void detach_ptw32(void) {
  pthread_win32_thread_detach_np();
  pthread_win32_process_detach_np();
}
#endif

static av_unused void osal_thread_init() {
#ifdef PTW32_STATIC_LIB
  pthread_win32_process_attach_np();
  pthread_win32_thread_attach_np();
  atexit(detach_ptw32);
#endif
}
#endif /* _HAVE_PTHREAD_H */

#endif /* defined(__FREERTOS__) || defined(__linux__) || defined(_WIN32) */

#define gettimeofday osal_gettimeofday
#define usleep osal_usleep
//#define open_memstream osal_open_memstream

#ifdef __linux__
#define OSAL_STRM_POS(A) (A).__pos
#else
#define OSAL_STRM_POS(A) (A)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_H_ */
