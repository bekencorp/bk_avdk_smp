/*------------------------------------------------------------------------------
--       Copyright (c) 2015, VeriSilicon Inc. All rights reserved        --
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


/*------------------------------------------------------------------------------

    Table of contents
   
    1. Include headers
    2. Module defines
    3. Data types
    4. Function prototypes

------------------------------------------------------------------------------*/

#ifndef H264HWD_SEQ_PARAM_SET_H
#define H264HWD_SEQ_PARAM_SET_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

#include "h264hwd_cfg.h"
#include "h264hwd_stream.h"
#include "h264hwd_vui.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/* structure to store sequence parameter set information decoded from the
 * stream */
typedef struct
{
    u32 profileIdc;
    u32 levelIdc;
    u8 constrained_set0_flag;
    u8 constrained_set1_flag;
    u8 constrained_set2_flag;
    u8 constrained_set3_flag;
    u32 seqParameterSetId;
    u32 maxFrameNum;
    u32 picOrderCntType;
    u32 maxPicOrderCntLsb;
    u32 deltaPicOrderAlwaysZeroFlag;
    i32 offsetForNonRefPic;
    i32 offsetForTopToBottomField;
    u32 numRefFramesInPicOrderCntCycle;
    i32 *offsetForRefFrame;
    u32 numRefFrames;
    u32 gapsInFrameNumValueAllowedFlag;
    u32 picWidthInMbs;
    u32 picHeightInMbs;
    u32 frameCroppingFlag;
    u32 frameCropLeftOffset;
    u32 frameCropRightOffset;
    u32 frameCropTopOffset;
    u32 frameCropBottomOffset;
    u32 vuiParametersPresentFlag;
    vuiParameters_t *vuiParameters;
    u32 maxDpbSize;
    u32 frameMbsOnlyFlag;
    u32 mbAdaptiveFrameFieldFlag;
    u32 direct8x8InferenceFlag;
    u32 chromaFormatIdc;
    u32 monoChrome;
    u32 scalingMatrixPresentFlag;
    u32 scalingListPresent[8];
    u32 useDefaultScaling[8];
    u8 scalingList[8][64];

    /* mvc extension */
    struct {
        u32 numViews;
        u32 viewId[MAX_NUM_VIEWS];
    } mvc;
} seqParamSet_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdDecodeSeqParamSet(strmData_t *pStrmData,
    seqParamSet_t *pSeqParamSet, u32 mvcFlag);

u32 h264bsdCompareSeqParamSets(seqParamSet_t *pSps1, seqParamSet_t *pSps2);

#endif /* #ifdef H264HWD_SEQ_PARAM_SET_H */
