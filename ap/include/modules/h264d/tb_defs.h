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

#ifndef TB_DEFS_H
#define TB_DEFS_H

#ifdef _ASSERT_USED
#include <assert.h>
#endif

//#include <stdio.h>
#include "os/mem.h" //7259debug@zhilei
#include "basetype.h"

// #define malloc(a)                    pvPortMalloc(a)
// #define realloc(a,b)                 pvPortRealloc(a,b)
// #define free(a)                      vPortFree(a)

/*------------------------------------------------------------------------------
    Generic data type stuff
------------------------------------------------------------------------------*/


typedef enum {
    TB_FALSE = 0,
    TB_TRUE  = 1
} TBBool;

/*------------------------------------------------------------------------------
    Defines
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    Test bench configuration    u32 strideEnable;

------------------------------------------------------------------------------*/
typedef struct {
    char packetByPacket[9];
    char nalUnitStream[9];
    u32 seedRnd;
    char streamBitSwap[24];
    char streamBitLoss[24];
    char streamPacketLoss[24];
    char streamHeaderCorrupt[9];
    char streamTruncate[9];
    char sliceUdInPacket[9];
    u32 firstTraceFrame;

    u32 memoryPageSize;
    i32 refFrmBufferSize;

} TBParams;

typedef struct {
    char outputPictureEndian[14];
    u32 busBurstLength;
    u32 asicServicePriority;
    char outputFormat[12];
    u32 latencyCompensation;
    char clockGating[9];
    char dataDiscard[9];

    char memoryAllocation[9];
    char rlcModeForced[9];
    char errorConcealment[15];

    u32 jpegMcusSlice;
    u32 jpegInputBufferSize;

    u32 refbuEnable;
    u32 refbuDisableInterlaced;
    u32 refbuDisableDouble;
    u32 refbuDisableEvalMode;
    u32 refbuDisableCheckpoint;
    u32 refbuDisableOffset;
    u32 refbuDataExcessMaxPct;
    u32 refbuDisableTopBotSum;

    u32 mpeg2Support;
    u32 vc1Support;
    u32 jpegSupport;
    u32 mpeg4Support;
    u32 customMpeg4Support;
    u32 h264Support;
    u32 vp6Support;
    u32 vp7Support;
    u32 vp8Support;
    u32 pJpegSupport;
    u32 sorensonSupport;
    u32 avsSupport;
    u32 rvSupport;
    u32 mvcSupport;
    u32 webpSupport;
    u32 ecSupport;
    u32 maxDecPicWidth;
    u32 hwVersion;
    u32 hwBuild;
    u32 busWidth64bitEnable;
    u32 latency;
    u32 nonSeqClk;
    u32 seqClk;
    u32 supportNonCompliant;    
    u32 jpegESupport;

    u32 forceMpeg4Idct;
    u32 ch8PixIleavOutput;

    u32 refBufferTestModeOffsetEnable;
    i32 refBufferTestModeOffsetMin;
    i32 refBufferTestModeOffsetMax;
    i32 refBufferTestModeOffsetStart;
    i32 refBufferTestModeOffsetIncr;

    u32 apfThresholdDisable;
    i32 apfThresholdValue;

    u32 tiledRefSupport;
    u32 strideSupport;
    i32 fieldDpbSupport;
    i32 addr64Support;

    u32 serviceMergeDisable;

} TBDecParams;

typedef struct {
    char outputPictureEndian[14];
    char inputPictureEndian[14];
    char wordSwap[9];
    char wordSwap16[9];
    u32 busBurstLength;
    char clockGating[9];
    char dataDiscard[9];
    char multiBuffer[9];

    u32 maxPpOutPicWidth;
    u32 ppdExists;
    u32 ditheringSupport;
    u32 scalingSupport;
    u32 deinterlacingSupport;
    u32 alphaBlendingSupport;
    u32 ablendCropSupport;
    u32 ppOutEndianSupport;
    u32 tiledSupport;
    u32 tiledRefSupport;

    i32 fastHorDownScaleDisable;
    i32 fastVerDownScaleDisable;
    i32 vertDownScaleStripeDisableSupport;
    u32 pixAccOutSupport;

    u32 PpOutRbSupport;
    u32 PpOutRbSegmentHeight;
    u32 PpOutRbSegmentNum;
    u32 PpOutRbIrqMask;

} TBPpParams;

typedef struct {
    TBParams tbParams;
    TBDecParams decParams;
    TBPpParams ppParams;
} TBCfg;


#endif /* TB_DEFS_H */
