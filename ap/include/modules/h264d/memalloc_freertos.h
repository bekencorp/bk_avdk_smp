/*------------------------------------------------------------------------------
--        Copyright (c) 2015, VeriSilicon Inc. All rights reserved            --

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
--------------------------------------------------------------------------------
*/

#ifndef _MEMALLOC_FREERTOS_H_
#define _MEMALLOC_FREERTOS_H_

// Use OSAL instead of posix headers
#include "osal.h"
#include "user_freertos.h"
#include "dev_common_freertos.h"

#undef PDEBUG   /* undef it, just in case */
#ifdef MEMALLOC_DEBUG
/* This one for user space */
#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args...)  /* not debugging: nothing */
#endif
/*
 * Ioctl definitions
 */
/* Use 'k' as magic number */
#define MEMALLOC_IOC_MAGIC  'k'
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */
#define MEMALLOC_IOCXGETBUFFER    _IOWR(MEMALLOC_IOC_MAGIC, 1, MemallocParams*)
#define MEMALLOC_IOCSFREEBUFFER   _IOW(MEMALLOC_IOC_MAGIC, 2, unsigned long*)
#define MEMALLOC_IOCGMEMBASE	  _IOR(MEMALLOC_IOC_MAGIC, 3, unsigned long *)

/* ... more to come */
#define MEMALLOC_IOCHARDRESET     _IO(MEMALLOC_IOC_MAGIC, 15) /* debugging tool */
#define MEMALLOC_IOC_MAXNR 15


typedef struct {
  unsigned long long busAddress;
  unsigned int size;
  unsigned long translationOffset;
  unsigned int mem_type;
} MemallocParams;
#define USE_VSI_IMPLEMENTATION  (0)
#if     USE_VSI_IMPLEMENTATION
//Mem device
int __init memalloc_init(void);
int memalloc_open(int *inode, int filp);
int memalloc_release(int *inode, int filp);
long memalloc_ioctl(int filp, unsigned int cmd, void *arg);
void * DirectMemoryMap(unsigned long long busaddr, unsigned long map_size);
unsigned long long GetBusAddrForIODevide(unsigned int size);  //7259debug@zhilei
#else
#define memalloc_init(...)     0
#define memalloc_open(...)     0
#define memalloc_release(...)  0
#define memalloc_ioctl         vcdec_memalloc_ioctl

long memalloc_ioctl(int filp, unsigned int cmd, void *arg);
#endif

#endif /* _MEMALLOC_FREERTOS_H_ */
