/*------------------------------------------------------------------------------
--         Copyright (c) 2015, VeriSilicon Inc. All rights reserved.          --
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

#ifndef __CFG_H_
#define __CFG_H_

#include "defs.h"

/*------------------------------------------------------------------------------
    Parse mode constants
------------------------------------------------------------------------------*/
typedef enum {
    CFG_OK,
    CFG_ERROR           = 500,
    CFG_INVALID_BLOCK   = 501,
    CFG_INVALID_PARAM   = 502,
    CFG_INVALID_VALUE   = 503,
    CFG_INVALID_CODE    = 504,
    CFG_DUPLICATE_BLOCK = 505
} CfgCallbackResult;

/*------------------------------------------------------------------------------
    Parse mode constants
------------------------------------------------------------------------------*/
typedef enum {
    CFG_CALLBACK_BLK_START  = 300,
    CFG_CALLBACK_VALUE      = 301,
} CfgCallbackParam;

/*------------------------------------------------------------------------------
    Interface to callback function
------------------------------------------------------------------------------*/
typedef CfgCallbackResult (*CfgCallback)(char*, char*, char*, CfgCallbackParam,
    void**,  u32* num_pp);

u32 ParseConfig( char * filename, CfgCallback callback, void **cbParam, u32 num_pp );
CfgCallbackResult ReadParam( char * block, char * key, char * value,
    CfgCallbackParam state, void **cbParam, u32* num_pp);

#endif /* __CFG_H_ */
