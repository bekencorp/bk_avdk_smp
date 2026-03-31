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


#ifndef _OSAL_LINUX_H_
#define _OSAL_LINUX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/timeb.h>

#include <limits.h> /* NAME_MAX */  

#ifdef _HAVE_PTHREAD_H
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <unistd.h>
#endif /* _HAVE_PTHREAD_H */

#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>

typedef struct timeval    osal_timeval;
typedef struct timezone   osal_timezone;

typedef void* (*process_main_ptr) (void *arg);

struct stream_trace
{
  struct node *next;
  char *buffer;
  char comment[256];
  size_t size;
  FILE *fp;
  unsigned int cnt;
};

typedef struct {
  int pid;
  pthread_t *tid;
}osal_pid;

#define _FILE_OFFSET_BITS 64  // for 64 bit fseeko
#define fseek fseeko
#define int16 int16_t
#define int64 int64_t
#define __int64 long long

#ifdef _DWL_DEBUG
#define DWL_DEBUG(fmt, args...) \
    printf(__FILE__ ":%d:%s() " fmt, __LINE__, __func__, ##args)
#else
#define DWL_DEBUG(fmt, args...) \
	do {                        \
	} while (0)
#endif

#ifndef CMODEL_DLL_API
#define CMODEL_DLL_API
#endif

static av_unused int osal_gettimeofday(osal_timeval *tp, void *tzp)
{
  return gettimeofday(tp, (osal_timezone *)tzp);
}

static av_unused void osal_usleep(unsigned long usec)
{
  if(usec == 0) {
#ifdef _HAVE_PTHREAD_H
    sched_yield();
#else
  #define sched_yield()
#endif
  }
  else {
#ifdef _HAVE_PTHREAD_H
    usleep(usec);
#else
//osfree for usleep
    ;
#if 0
  int i, j, tick_in_us;
  unsigned long long freq = CPUFreq;

  tick_in_us = freq * 17ul >> 24; /* 17 / 2 ^ 24 = 1000000L */
  for (i = 0; i < n; i++) {
    for (j = 0; j < tick_in_us; j++);
  }
#endif
#endif
  }
}

static av_unused void osal_open_memstream(struct stream_trace *stream_trace)
{
  stream_trace->fp = open_memstream(&stream_trace->buffer, &stream_trace->size);
}

static av_unused void * osal_aligned_malloc(unsigned int boundary, unsigned int memory_size)
{
  return memalign(boundary, memory_size);
}

static av_unused void osal_aligned_free(void * aligned_momory)
{
  free(aligned_momory);
}

#ifdef _HAVE_PTHREAD_H
static av_unused osal_pid osal_fork(process_main_ptr process_main)
{
  int pid;
  osal_pid osal_pid_t;
  osal_pid_t.tid = NULL;
  if (0 == (pid = fork())) {
    process_main(NULL);
    exit(0);
  }
  else if (pid > 0) {
    osal_pid_t.pid = pid;
  }
  else {
    perror("failed to fork new process to process streams");
    exit(pid);
  }
  printf("osal_pid_t.osal_pid is %d\n", osal_pid_t.pid);
  return osal_pid_t;
}

static av_unused void osal_wait(osal_pid wait_pid, int* status)
{
  waitpid(wait_pid.pid, status, 0);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_LINUX_H_ */
