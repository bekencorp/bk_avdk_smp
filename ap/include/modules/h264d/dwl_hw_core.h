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

#ifndef __DWL_HW_CORE_H__
#define __DWL_HW_CORE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
#include <dwlthread.h>

typedef const void* core;

core hw_core_init(void);
void hw_core_release(core instance);

int hw_core_try_lock(core inst);
void hw_core_unlock(core inst);

u32* hw_core_get_base_address(core instance);
void hw_core_setid(core instance, int id);
int hw_core_getid(core instance);

/* Starts the execution on a core. */
void hw_core_dec_enable(core instance);
void hw_core_pp_enable(core instance, int start);

/* Tries to interrupt the execution on a core as soon as possible. */
void hw_core_disable(core instance);

int  hw_core_wait_dec_rdy(core instance);
int  hw_core_wait_pp_rdy(core instance);

int  hw_core_post_pp_rdy(core instance);
int  hw_core_post_dec_rdy(core instance);

int hw_core_is_dec_rdy(core instance);
int hw_core_is_pp_rdy(core instance);

void hw_core_set_hw_rdy_sem(core instance, sem_t *rdy);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __DWL_HW_CORE_H__ */

