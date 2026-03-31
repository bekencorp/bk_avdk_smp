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

#ifndef _IO_TOOLS_
#define _IO_TOOLS_

#ifdef __cplusplus
extern "C" {
#endif
#include "basetype.h"

u32 ioread32(volatile void* addr);
void iowrite32(u32 val,volatile void *addr);
u16 ioread16(volatile void* addr);
u8 ioread8(volatile void* addr);
void iowrite16(u16 val,volatile void *addr);
void iowrite8(u8 val,volatile void *addr);
u32 readl(volatile void* addr);
void writel(unsigned int v, volatile void *addr);
#define read_mreg32(addr) ioread32((void*)(addr))
#define write_mreg32(addr,val) iowrite32(val, (void*)(addr))
#define read_mreg16(addr) ioread16((void*)addr)
#define write_mreg16(addr,val) iowrite16(val, (void*)(addr))
#define read_mreg8(addr) ioread8((void*)addr)
#define write_mreg8(addr,val) iowrite8(val, (void*)(addr))

//Dec
int hantrodec_init(void);
int hantrodec_open(int *inode, int filp);
int hantrodec_release(int *inode, int filp);
long hantrodec_ioctl(int filp, unsigned int cmd, void *arg);

//VCMD interfaces funcs
int hantrovcmd_init(void);
int hantrovcmd_open(int *inode, int filp);
int hantrovcmd_release(int *inode, int filp);
long hantrovcmd_ioctl(int filp, unsigned int cmd, void *arg);
//typedef int (*IRQHandler)(i32 i, void* data);
// typedef void (*IRQHandler)(void* data);
typedef void (*IRQHandler)(i32 i, void* data);  //7259debug@zhilei

#ifdef __cplusplus
}
#endif

#endif //_IO_TOOLS_