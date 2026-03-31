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

#ifndef H264HWD_VUI_H
#define H264HWD_VUI_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

#include "h264hwd_stream.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

#define MAX_CPB_CNT 32

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/* enumerated sample aspect ratios, ASPECT_RATIO_M_N means M:N */
enum
{
    ASPECT_RATIO_UNSPECIFIED = 0,
    ASPECT_RATIO_1_1,
    ASPECT_RATIO_12_11,
    ASPECT_RATIO_10_11,
    ASPECT_RATIO_16_11,
    ASPECT_RATIO_40_33,
    ASPECT_RATIO_24_11,
    ASPECT_RATIO_20_11,
    ASPECT_RATIO_32_11,
    ASPECT_RATIO_80_33,
    ASPECT_RATIO_18_11,
    ASPECT_RATIO_15_11,
    ASPECT_RATIO_64_33,
    ASPECT_RATIO_160_99,
    ASPECT_RATIO_EXTENDED_SAR = 255
};

/* structure to store Hypothetical Reference Decoder (HRD) parameters */
typedef struct
{
    u32 cpbCnt;
    u32 bitRateScale;
    u32 cpbSizeScale;
    u32 bitRateValue[MAX_CPB_CNT];
    u32 cpbSizeValue[MAX_CPB_CNT];
    u32 cbrFlag[MAX_CPB_CNT];
    u32 initialCpbRemovalDelayLength;
    u32 cpbRemovalDelayLength;
    u32 dpbOutputDelayLength;
    u32 timeOffsetLength;
} hrdParameters_t;

/* storage for VUI parameters */
typedef struct
{
    u32 aspectRatioPresentFlag;
    u32 aspectRatioIdc;
    u32 sarWidth;
    u32 sarHeight;
    u32 overscanInfoPresentFlag;
    u32 overscanAppropriateFlag;
    u32 videoSignalTypePresentFlag;
    u32 videoFormat;
    u32 videoFullRangeFlag;
    u32 colourDescriptionPresentFlag;
    u32 colourPrimaries;
    u32 transferCharacteristics;
    u32 matrixCoefficients;
    u32 chromaLocInfoPresentFlag;
    u32 chromaSampleLocTypeTopField;
    u32 chromaSampleLocTypeBottomField;
    u32 timingInfoPresentFlag;
    u32 numUnitsInTick;
    u32 timeScale;
    u32 fixedFrameRateFlag;
    u32 nalHrdParametersPresentFlag;
    hrdParameters_t nalHrdParameters;
    u32 vclHrdParametersPresentFlag;
    hrdParameters_t vclHrdParameters;
    u32 lowDelayHrdFlag;
    u32 picStructPresentFlag;
    u32 bitstreamRestrictionFlag;
    u32 motionVectorsOverPicBoundariesFlag;
    u32 maxBytesPerPicDenom;
    u32 maxBitsPerMbDenom;
    u32 log2MaxMvLengthHorizontal;
    u32 log2MaxMvLengthVertical;
    u32 numReorderFrames;
    u32 maxDecFrameBuffering;
    u32 updateHRDParameterFlag;
    u32 errorHRDParameterFlag;
} vuiParameters_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdDecodeVuiParameters(strmData_t *pStrmData,
    vuiParameters_t *pVuiParameters);

u32 h264bsdDecodeHrdParameters(
  strmData_t *pStrmData,
  hrdParameters_t *pHrdParameters);

#endif /* #ifdef H264HWD_VUI_H */
