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

#ifndef __FRAMEWORKS_FREERTOS_
#define __FRAMEWORKS_FREERTOS_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal.h"
#include "io_tools.h"

#ifdef __XTENSA_FREERTOS__
//Netint xtensa cpu and FreeRTOS OS
#include "xtensa_api.h"
//#include "../src/libxmp/xmp-library.h" //For atomic operations
#include <xtensa/xtruntime.h> //interrupt for xtensa
#endif

#define SYS_REG_INT_TOP_BASE     (0x44010000)
//ENCODER
#if 0
#define CPU_INT_IRQ              8 /* All Encoder Modules' interrupt will be connected to CPU IRQ 8 */
#define SYS_INT_MASK             (0x220)
#define SYS_REG_INT_VAL          (SYS_REG_INT_TOP_BASE + 0x3c)
#define SYS_REG_INT_STAT         (SYS_REG_INT_TOP_BASE + 0x40)
#define SYS_REG_INT_EN           (SYS_REG_INT_TOP_BASE + 0x44)
#else
//DECODER
#define CPU_INT_IRQ              9
#define SYS_INT_MASK             (0x001)
#define SYS_REG_INT_VAL          (SYS_REG_INT_TOP_BASE + 0x48)
#define SYS_REG_INT_STAT         (SYS_REG_INT_TOP_BASE + 0x4c)
#define SYS_REG_INT_EN           (SYS_REG_INT_TOP_BASE + 0x12 * 4/*0x50*/)  //7259debug@zhilei
#endif

/* Interrupt */
/*********************request_irq, disable_irq, enable_irq need to be provided by customer*********************/
int RegisterIRQ(i32 i, IRQHandler isr, i32 flag, const char* name, void *data);
void IntEnableIRQ(u32 irq);
void IntDisableIRQ(u32 irq);
void IntClearIRQStatus(u32 irq);
u32 IntGetIRQStatus(u32 irq);
uint32_t ReadInterruptStatus(void);

#ifdef __cplusplus
}
#endif

#endif /*__FRAMEWORKS_FREERTOS_ */