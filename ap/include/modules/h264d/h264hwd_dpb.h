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


#ifndef H264HWD_DPB_H
#define H264HWD_DPB_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "h264decapi.h"

#include "h264hwd_slice_header.h"
#include "h264hwd_image.h"
#include "h264hwd_dpb_lock.h"

#include "dwl.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/* enumeration to represent status of buffered image */
typedef enum
{
    UNUSED = 0,
    NON_EXISTING,
    SHORT_TERM,
    LONG_TERM,
    EMPTY
} dpbPictureStatus_e;

/* structure to represent a buffered picture */
typedef struct dpbPicture
{
    u32 memIdx;
    DWLLinearMem_t *data;
    i32 picNum;
    u32 frameNum;
    i32 picOrderCnt[2];
    dpbPictureStatus_e status[2];
    u32 toBeDisplayed;
    u32 picId;
    u32 picCodeType[2];
    i32 numErrMbs;
    u32 isIdr[2];
    u32 decodeId[2];
    u32 isFieldPic;
    u32 isBottomField;
    u32 tiledMode;
    H264CropParams crop;
    double dpbOutputTime[2];
    u32 picStruct;
    u32 picWidth;
    u32 picHeight;
//#ifdef SKIP_OPENB_FRAME
    u32 openBFlag;
//#endif
    u32 corruptedFirstFieldOrFrame;
    u32 corruptedSecondField;
} dpbPicture_t;

/* structure to represent display image output from the buffer */
typedef struct
{
    u32 memIdx;
    DWLLinearMem_t *data;
    u32 picId;
    u32 picCodeType[2];
    i32 numErrMbs;
    u32 isIdr[2];
    u32 decodeId[2];
    u32 interlaced;
    u32 fieldPicture;
    u32 topField;
    u32 tiledMode;
    H264CropParams crop;
    u32 picStruct;
    u32 picWidth;
    u32 picHeight;
    u32 corruptedSecondField;
} dpbOutPicture_t;

typedef struct buffStatus
{
    u32 nRefCount;
    u32 usageMask;
} buffStatus_t;

/* structure to represent DPB */
typedef struct dpbStorage
{
    dpbPicture_t buffer[16 + 1];
    u32 list[16 + 1];
    dpbPicture_t *currentOut;
    double cpbRemovalTime;
    u32 bumpingFlag;
    u32 currentOutPos;
    dpbOutPicture_t *outBuf;
    u32 numOut;
    u32 outIndexW;
    u32 outIndexR;
    u32 maxRefFrames;
    u32 dpbSize;
    u32 maxFrameNum;
    u32 maxLongTermFrameIdx;
    u32 numRefFrames;
    u32 fullness;
    u32 prevRefFrameNum;
    u32 lastContainsMmco5;
    u32 noReordering;
    u32 flushed;
    u32 picSizeInMbs;
    u32 dirMvOffset;
    u32 syncMcOffset;
    DWLLinearMem_t poc;
    u32 delayedOut;
    u32 delayedId;
    u32 interlaced;
    u32 ch2Offset;

    u32 totBuffers;
#ifdef USE_OUTPUT_RELEASE
    u32 totBuffersReserved;
#endif
    DWLLinearMem_t picBuffers[16+1+16+1];
    u32 picBuffID[16+1+16+1];

    /* flag to prevent output when display smoothing is used and second field
     * of a picture was just decoded */
    u32 noOutput;

    u32 prevOutIdx;

    FrameBufferList *fbList;
    u32 refId[16];
    u32 picWidth;
    u32 picHeight;

    /* Try to recover DPB from chaos with error streams. */
    u32 tryRecoverDpb;

#ifdef USE_EXTERNAL_BUFFER
    u32 useAdaptiveBuffers;
    u32 nGuardSize;
    u32 bUpdated;
    u32 nExtBufSizeAdded;   /* size of external buffer added */
    u32 nNewPicSize;        /* pic size for new sequence (temp). */
#endif
} dpbStorage_t;

typedef struct dpbInitParams
{
    u32 picSizeInMbs;
    u32 dpbSize;
    u32 maxRefFrames;
    u32 maxFrameNum;
    u32 noReordering;
    u32 displaySmoothing;
    u32 monoChrome;
    u32 isHighSupported;
    u32 enable2ndChroma;
    u32 multiBuffPp;
    u32 nCores;
    u32 mvcView;
} dpbInitParams_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdInitDpb(
#ifndef USE_EXTERNAL_BUFFER
                   const void *dwl,
#endif
                   dpbStorage_t * dpb,
                   struct dpbInitParams *pDpbParams);

u32 h264bsdResetDpb(
#ifndef USE_EXTERNAL_BUFFER
                    const void *dwl,
#endif
                    dpbStorage_t * dpb,
                    struct dpbInitParams *pDpbParams);

void h264bsdInitRefPicList(dpbStorage_t * dpb);

void *h264bsdAllocateDpbImage(dpbStorage_t * dpb);

i32 h264bsdGetRefPicData(const dpbStorage_t * dpb, u32 index);
u8 *h264bsdGetRefPicDataVlcMode(const dpbStorage_t * dpb, u32 index,
    u32 fieldMode);

u32 h264bsdReorderRefPicList(dpbStorage_t * dpb,
                             refPicListReordering_t * order,
                             u32 currFrameNum, u32 numRefIdxActive);

u32 h264bsdMarkDecRefPic(dpbStorage_t * dpb,
                         /*@null@ */ const decRefPicMarking_t * mark,
                         const image_t * image, u32 frameNum, i32 *picOrderCnt,
                         u32 isIdr, u32 picId, u32 numErrMbs, u32 tiledMode, u32 picCodeType );

u32 h264bsdCheckGapsInFrameNum(dpbStorage_t * dpb, u32 frameNum, u32 isRefPic,
    u32 gapsAllowed);

/*@null@*/ dpbOutPicture_t *h264bsdDpbOutputPicture(dpbStorage_t * dpb);

void h264bsdFlushDpb(dpbStorage_t * dpb);

void h264bsdFreeDpb(
#ifndef USE_EXTERNAL_BUFFER
                                          const void *dwl,
#endif
                                          dpbStorage_t * dpb);

void ShellSort(dpbStorage_t * dpb, u32 *list, u32 type, i32 par);
void ShellSortF(dpbStorage_t * dpb, u32 *list, u32 type, /*u32 parity,*/ i32 par);

void SetPicNums(dpbStorage_t * dpb, u32 currFrameNum);

void h264DpbUpdateOutputList(dpbStorage_t * dpb);
void h264DpbAdjStereoOutput(dpbStorage_t * dpb, u32 targetCount);
#ifdef USE_OUTPUT_RELEASE
void h264EmptyDpb(dpbStorage_t *dpb);
void h264DpbStateReset(dpbStorage_t *dpb);
#endif

u32 h264DpbHRDBumping(dpbStorage_t * dpb);
void h264ClearBump(dpbStorage_t * dpb);
void h264DpbRecover(dpbStorage_t *dpb, u32 currFrameNum, i32 currPoc);
void h264RemoveNoBumpOutput(dpbStorage_t *dpb, u32 size);
#endif /* #ifdef H264HWD_DPB_H */
