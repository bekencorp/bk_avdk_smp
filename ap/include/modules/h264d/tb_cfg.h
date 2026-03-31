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

#ifndef TB_CFG_H
#define TB_CFG_H

#include "refbuffer.h"
#include "tb_defs.h"
#include "dwl.h"

/*------------------------------------------------------------------------------
    Parse mode constants
------------------------------------------------------------------------------*/
typedef enum {
    TB_CFG_OK,
    TB_CFG_ERROR           = 500,
    TB_CFG_INVALID_BLOCK   = 501,
    TB_CFG_INVALID_PARAM   = 502,
    TB_CFG_INVALID_VALUE   = 503,
    TB_CFG_INVALID_CODE    = 504,
    TB_CFG_DUPLICATE_BLOCK = 505
} TBCfgCallbackResult;

/*------------------------------------------------------------------------------
    Parse mode constants
------------------------------------------------------------------------------*/
typedef enum {
    TB_CFG_CALLBACK_BLK_START  = 300,
    TB_CFG_CALLBACK_VALUE      = 301,
} TBCfgCallbackParam;

/*------------------------------------------------------------------------------
    Interface to parsing
------------------------------------------------------------------------------*/
typedef TBCfgCallbackResult (*TBCfgCallback)(char*, char*, char*, TBCfgCallbackParam, void* );
TBCfgCallbackResult TBReadParam( char * block, char * key, char * value, TBCfgCallbackParam state, void *cbParam );

TBBool TBParseConfig( char * filename, TBCfgCallback callback, void *cbParam );
void TBSetDefaultCfg(TBCfg* tbCfg);
void TBPrintCfg(const TBCfg* tbCfg);
u32 TBCheckCfg(const TBCfg* tbCfg);

u32 TBGetPPDataDiscard(const TBCfg* tbCfg);
u32 TBGetPPClockGating(const TBCfg* tbCfg);
u32 TBGetPPWordSwap(const TBCfg* tbCfg);
u32 TBGetPPWordSwap16(const TBCfg* tbCfg);
u32 TBGetPPInputPictureEndian(const TBCfg* tbCfg);
u32 TBGetPPOutputPictureEndian(const TBCfg* tbCfg);

u32 TBGetDecErrorConcealment(const TBCfg* tbCfg);
u32 TBGetDecRlcModeForced(const TBCfg* tbCfg);
u32 TBGetDecMemoryAllocation(const TBCfg* tbCfg);
u32 TBGetDecDataDiscard(const TBCfg* tbCfg);
u32 TBGetDecClockGating(const TBCfg* tbCfg);
u32 TBGetDecOutputFormat(const TBCfg* tbCfg);
u32 TBGetDecOutputPictureEndian(const TBCfg* tbCfg);

u32 TBGetTBPacketByPacket(const TBCfg* tbCfg);
u32 TBGetTBNalUnitStream(const TBCfg* tbCfg);
u32 TBGetTBStreamHeaderCorrupt(const TBCfg* tbCfg);
u32 TBGetTBStreamTruncate(const TBCfg* tbCfg);
u32 TBGetTBSliceUdInPacket(const TBCfg* tbCfg);
u32 TBGetTBFirstTraceFrame(const TBCfg* tbCfg);
u32 TBGetTBMultiBuffer(const TBCfg* tbCfg);

u32 TBGetDecRefbuEnabled(const TBCfg* tbCfg);
u32 TBGetDecRefbuDisableEvalMode(const TBCfg* tbCfg);
u32 TBGetDecRefbuDisableCheckpoint(const TBCfg* tbCfg);
u32 TBGetDecRefbuDisableOffset(const TBCfg* tbCfg);
u32 TBGetDec64BitEnable(const TBCfg* tbCfg);
u32 TBGetDecSupportNonCompliant(const TBCfg* tbCfg);
u32 TBGetDecIntraFreezeEnable(const TBCfg* tbCfg);
u32 TBGetDecDoubleBufferSupported(const TBCfg* tbCfg);
u32 TBGetDecTopBotSumSupported(const TBCfg* tbCfg);
void TBGetHwConfig( const TBCfg* tbCfg, DWLHwConfig_t *pHwCfg );
void TBSetRefbuMemModel( const TBCfg* tbCfg, u32 *regBase, refBuffer_t *pRefbu );
u32 TBGetDecForceMpeg4Idct(const TBCfg* tbCfg);
u32 TBGetDecCh8PixIleavSupported(const TBCfg* tbCfg);
void TBRefbuTestMode( refBuffer_t *pRefbu, u32 *regBase,
                      u32 isIntraFrame, u32 mode );

u32 TBGetDecApfThresholdEnabled( const TBCfg* tbCfg );

u32 TBGetDecServiceMergeDisable( const TBCfg* tbCfg );

#endif /* TB_CFG_H */
