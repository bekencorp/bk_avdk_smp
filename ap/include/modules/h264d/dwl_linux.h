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


#ifndef __DWL_LINUX_H__
#define __DWL_LINUX_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
#include "dwl_defs.h"
#include "dwl.h"
#include "dwlthread.h"

#ifdef __FREERTOS__
#include "dev_common_freertos.h"
#include "user_freertos.h"
#include "memalloc_freertos.h"
#include "vsios_sys.h"
#elif defined(__linux__)
#include "memalloc.h"

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#else //For other os
//TODO...
#endif

#ifndef DWL_DEBUG
#ifdef _DWL_DEBUG
#define DWL_DEBUG(fmt, args...) printf(__FILE__ ":%d:%s() " fmt, __LINE__ ,\
                                        __func__, ## args)
#else
#define DWL_DEBUG(fmt, args...) do {} while (0); /* not debugging: nothing */
#endif
#endif

#ifndef DEC_MODULE_PATH
#define DEC_MODULE_PATH         "/tmp/dev/hx170"
#endif

#ifndef MEMALLOC_MODULE_PATH
#define MEMALLOC_MODULE_PATH    "/tmp/dev/memalloc"
#endif

#define HX170PP_REG_START       0xF0
#define HX170DEC_REG_START      0x4
#define HX170_MSB_START         (119 * 4)

#define HX170PP_FUSE_CFG         99
#define HX170DEC_FUSE_CFG        57

#define DWL_DECODER_INT ((DWLReadReg(dec_dwl, HX170DEC_REG_START) >> 11) & 0xFFU)
#define DWL_PP_INT      ((DWLReadReg(dec_dwl, HX170PP_REG_START) >> 11) & 0xFFU)

#define DEC_IRQ_ABORT          (1 << 11)
#define DEC_IRQ_RDY            (1 << 12)
#define DEC_IRQ_BUS            (1 << 13)
#define DEC_IRQ_BUFFER         (1 << 14)
#define DEC_IRQ_ASO            (1 << 15)
#define DEC_IRQ_ERROR          (1 << 16)
#define DEC_IRQ_SLICE          (1 << 17)
#define DEC_IRQ_TIMEOUT        (1 << 18)

#define PP_IRQ_RDY             (1 << 12)
#define PP_IRQ_BUS             (1 << 13)

#define DWL_HW_ENABLE_BIT       0x000001    /* 0th bit */

#ifdef _DWL_HW_PERFORMANCE
/* signal that decoder/pp is enabled */
void DwlDecoderEnable(void);
void DwlPpEnable(void);
#endif

typedef struct mc_listener_thread_params_
{
    int fd;
    int bStopped;
    unsigned int nDecCores;
    unsigned int nPPCores;
    vsios_sem_t sc_dec_rdy_sem[MAX_ASIC_CORES];
    vsios_sem_t sc_pp_rdy_sem[MAX_ASIC_CORES];
    DWLIRQCallbackFn *pCallback[MAX_ASIC_CORES];
    void *pCallbackArg[MAX_ASIC_CORES];
} mc_listener_thread_params;

/* wrapper information */
typedef struct hX170dwl
{
    u32 clientType;
    int fd;                  /* decoder device file */
    int fd_mem;              /* /dev/mem for mapping */
    int fd_memalloc;         /* linear memory allocator */
    u32 numCores;
    u32 regSize;             /* IO mem size */
    g1_addr_t freeLinMem;    /* Start address of free linear memory */
    g1_addr_t freeRefFrmMem; /* Start address of free reference frame memory */
    int semid;
    int sigio_needed;
    struct mc_listener_thread_params_ *sync_params;

    u32 bPPReserved;
} hX170dwl_t;

i32 DWLWaitPpHwReady(const void *instance, i32 coreID, u32 timeout);
i32 DWLWaitDecHwReady(const void *instance, i32 coreID, u32 timeout);
u32 *DWLMapRegisters(int mem_dev, unsigned long base,
                     unsigned int regSize, u32 write);
void DWLUnmapRegisters(const void *io, unsigned int regSize);

#ifdef __cplusplus
}
#endif

#endif                       /* __DWL_LINUX__H__ */
