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


/*------------------------------------------------------------------------------

    Table of contents
   
    1. Include headers
    2. Module defines
    3. Data types
    4. Function prototypes

------------------------------------------------------------------------------*/

#ifndef H264HWD_STORAGE_H
#define H264HWD_STORAGE_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

#include "h264hwd_cfg.h"
#include "h264hwd_seq_param_set.h"
#include "h264hwd_pic_param_set.h"
#include "h264hwd_macroblock_layer.h"
#include "h264hwd_nal_unit.h"
#include "h264hwd_slice_header.h"
#include "h264hwd_seq_param_set.h"
#include "h264hwd_dpb.h"
#include "h264hwd_pic_order_cnt.h"
#include "h264hwd_sei.h"
/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

typedef struct
{
    u32 sliceId;
    u32 numDecodedMbs;
    u32 lastMbAddr;
} sliceStorage_t;

/* structure to store parameters needed for access unit boundary checking */
typedef struct
{
    nalUnit_t nuPrev[1];
    u32 prevFrameNum;
    u32 prevModFrameNum;
    u32 prevIdrPicId;
    u32 prevPicOrderCntLsb;
    i32 prevDeltaPicOrderCntBottom;
    i32 prevDeltaPicOrderCnt[2];
    u32 prevFieldPicFlag;
    u32 prevBottomFieldFlag;
    u32 firstCallFlag;
    u32 newPicture;
} aubCheck_t;

/* storage data structure, holds all data of a decoder instance */
typedef struct
{
    /* active paramet set ids and pointers */
    u32 oldSpsId;
    u32 activePpsId;
    u32 activeSpsId;
    u32 activeViewSpsId[MAX_NUM_VIEWS];
    picParamSet_t *activePps;
    seqParamSet_t *activeSps;
    seqParamSet_t *activeViewSps[MAX_NUM_VIEWS];
    seqParamSet_t *sps[MAX_NUM_SEQ_PARAM_SETS];
    picParamSet_t *pps[MAX_NUM_PIC_PARAM_SETS];

    /* current slice group map, recomputed for each slice */
    u32 *sliceGroupMap;

    u32 picSizeInMbs;

    /* this flag is set after all macroblocks of a picture successfully
     * decoded -> redundant slices not decoded */
    u32 skipRedundantSlices;
    u32 picStarted;

    /* flag to indicate if current access unit contains any valid slices */
    u32 validSliceInAccessUnit;

    /* store information needed for handling of slice decoding */
    sliceStorage_t slice[1];

    /* number of concealed macroblocks in the current image */
    u32 numConcealedMbs;

    /* picId given by application */
    u32 currentPicId;

    /* macroblock specific storages, size determined by image dimensions */
    mbStorage_t *mb;

    /* flag to store noOutputReordering flag set by the application */
    u32 noReordering;

    /* pointer to DPB of current view */
    dpbStorage_t *dpb;

    /* DPB */
    dpbStorage_t dpbs[MAX_NUM_VIEWS][2];

    /* structure to store picture order count related information */
    pocStorage_t poc[2];

    /* access unit boundary checking related data */
    aubCheck_t aub[1];

    /* current processed image */
    image_t currImage[1];

    /* last valid NAL unit header is stored here */
    nalUnit_t prevNalUnit[1];

    /* slice header, second structure used as a temporary storage while 
     * decoding slice header, first one stores last successfully decoded
     * slice header */
    sliceHeader_t *sliceHeader;
    sliceHeader_t sliceHeaders[MAX_NUM_VIEWS][2];

    seiParameters_t sei;
    /* fields to store old stream buffer pointers, needed when only part of
     * a stream buffer is processed by h264bsdDecode function */
    u32 prevBufNotFinished;
    const u8 *prevBufPointer;
    u32 prevBytesConsumed;
    strmData_t strm[1];

    /* macroblock layer structure, there is no need to store this but it 
     * would have increased the stack size excessively and needed to be
     * allocated from heap -> easiest to put it here */
    macroblockLayer_t mbLayer[1];

    u32 asoDetected;
    u32 secondField;
    u32 checkedAub; /* signal that AUB was checked already */
    u32 prevIdrPicReady; /* for FFWD workaround */

    u32 intraFreeze;
    u32 partialFreeze;
    u32 pictureBroken;

    u32 enable2ndChroma;     /* by default set according to ENABLE_2ND_CHROMA
                                compiler flag, may be overridden by testbench */

    /* pointers to 2nd chroma output, only available if extension enabled */
    u32 *pCh2;
    g1_addr_t bCh2;

    u32 ppUsed;
    u32 useSmoothing;
    u32 currentMarked;
    u32 prevPicWidth;
    u32 prevPicHeight;
    u32 pendingFlush;

    u32 mvc;
    u32 mvcStream;
    u32 view;
    u32 viewId[MAX_NUM_VIEWS];
    u32 outView;
    u32 numViews;
    u32 baseOppositeFieldPic;
    u32 nonInterViewRef;

    u32 nextView;
    u32 lastBaseNumOut;

    u32 multiBuffPp;

    const dpbOutPicture_t *pendingOutPic;
} storage_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

void h264bsdInitStorage(storage_t * pStorage);
void h264bsdResetStorage(storage_t * pStorage);
u32 h264bsdIsStartOfPicture(storage_t * pStorage);
u32 h264bsdIsEndOfPicture(storage_t * pStorage);
u32 h264bsdStoreSeqParamSet(storage_t * pStorage, seqParamSet_t * pSeqParamSet);
u32 h264bsdStorePicParamSet(storage_t * pStorage, picParamSet_t * pPicParamSet);
u32 h264bsdActivateParamSets(storage_t * pStorage, u32 ppsId, u32 sliceType, u32 isIdr);
void h264bsdComputeSliceGroupMap(storage_t * pStorage,
                                 u32 sliceGroupChangeCycle);

u32 h264bsdCheckAccessUnitBoundary(strmData_t * strm,
                                   nalUnit_t * nuNext,
                                   storage_t * storage,
                                   u32 * accessUnitBoundaryFlag);

u32 h264bsdValidParamSets(storage_t * pStorage);

u32 h264bsdAllocateSwResources(
#ifndef USE_EXTERNAL_BUFFER
                               const void *dwl,
#endif
                               storage_t * pStorage,
                               u32 isHighSupported, u32 nCores);
#ifdef USE_EXTERNAL_BUFFER
u32 h264bsdMVCAllocateSwResources(storage_t * pStorage,
                               u32 isHighSupported, u32 nCores);
#endif
#ifdef USE_OUTPUT_RELEASE
void h264bsdClearStorage(storage_t * pStorage);
#endif
u32 h264bsdStoreSEIInfoForCurrentPic(storage_t * pStorage);
#endif /* #ifdef H264HWD_STORAGE_H */
