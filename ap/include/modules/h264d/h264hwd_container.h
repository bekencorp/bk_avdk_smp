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


#ifndef H264HWD_CONTAINER_H
#define H264HWD_CONTAINER_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "h264hwd_storage.h"
#include "h264hwd_util.h"
#include "refbuffer.h"
#include "deccfg.h"
#include "decppif.h"
#include "workaround.h"

#include "h264hwd_dpb_lock.h"

#include "ppapi.h"
#include "vsios_sys.h"
/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

#define H264DEC_UNINITIALIZED   0U
#define H264DEC_INITIALIZED     1U
#define H264DEC_BUFFER_EMPTY    2U
#define H264DEC_NEW_HEADERS     3U

#ifdef USE_RANDOM_TEST
#include <stdio.h>
struct ErrorParams {
  u32 seed;
  u8 truncateStream;
  char truncateStreamOdds[24];
  u8 swapBitsInStream;
  char swapBitOdds[24];
  u8 losePackets;
  char packetLossOdds[24];
  u32 randomErrorEnabled;
};
#endif

/* asic interface */
typedef struct DecAsicBuffers
{
    u32 buff_status;
    DWLLinearMem_t mbCtrl;
    DWLLinearMem_t mv;
    DWLLinearMem_t intraPred;
    DWLLinearMem_t residual;
    DWLLinearMem_t *outBuffer;
    DWLLinearMem_t cabacInit[MAX_ASIC_CORES];
    g1_addr_t refPicList[16];
    u32 maxRefFrm;
    u32 filterDisable;
    i32 chromaQpIndexOffset;
    i32 chromaQpIndexOffset2;
    u32 currentMB;
    u32 notCodedMask;
    u32 rlcWords;
    u32 picSizeInMbs;
    u32 wholePicConcealed;
    u32 disableOutWriting;
    u32 enableDmvAndPoc;
} DecAsicBuffers_t;

typedef struct
{
    u32 coreID;
    const u8 *pStream;
    const void *pUserData;
    u32 isFieldPic;
    u32 isBottomField;
    u32 outId;
    dpbStorage_t *currentDpb;
    u32 refId[16];
} H264HwRdyCallbackArg;

typedef struct decContainer
{
    const void *checksum;
    u32 decStat;
    u32 picNumber;
    u32 asicRunning;
    u32 rlcMode;
    u32 tryVlc;
    u32 reallocate;
    const u8 *pHwStreamStart;
    g1_addr_t hwStreamStartBus;
    u32 hwBitPos;
    u32 hwLength;
    u32 streamPosUpdated;
    u32 nalStartCode;
    u32 modeChange;
    u32 gapsCheckedForThis;
    u32 packetDecoded;
    u32 forceRlcMode;        /* by default stays 0, testing can set it to 1 for RLC mode */

    u32 h264Regs[TOTAL_X170_REGISTERS];
    storage_t storage;       /* h264bsd storage */
    DecAsicBuffers_t asicBuff[1];
    const void *dwl;         /* DWL instance */
    i32 coreID;
    u32 refBufSupport;
    u32 tiledModeSupport;
    u32 tiledReferenceEnable;
    u32 h264ProfileSupport;
    u32 is8190;
    u32 maxDecPicWidth;
    u32 allowDpbFieldOrdering;
    u32 dpbMode;
    refBuffer_t refBufferCtrl;

    u32 keepHwReserved;
    u32 skipNonReference;

    workaround_t workarounds;
    u32 frameNumMask; /* for workaround */
    u32 forceNalMode;

    DWLHwConfig_t hwCfg[MAX_ASIC_CORES];

    FrameBufferList fbList;
    u32 bMC; /* flag to indicate MC mode status */
    u32 nCores;
    struct
    {
        H264DecMCStreamConsumed *fn;
        const u8 *pStrmBuff; /* stream buffer passed in callback */
        const void *pUserData; /* user data to be passed in callback */
    } streamConsumedCallback;

    H264HwRdyCallbackArg hwRdyCallbackArg[MAX_ASIC_CORES];
    i32 poc[34];

    struct pp_
    {
        const void *ppInstance;
        void (*PPDecStart) (const void *, const DecPpInterface *);
        void (*PPDecWaitEnd) (const void *);
        void (*PPConfigQuery) (const void *, DecPpQuery *);
        void (*PPNextDisplayId)(const void *, u32); /* set the next PP outpic ID (multibuffer) */
        u32 (*PPGetFreeMultiBufferId)(const void *, u32);
        u32 (*PPReturnMultiBufferId)(const void *, g1_addr_t);
        u32 (*PPFindDisplayId)(const void *, u32, u32);
        void (*PPRecoverMemInfo) (const void *, u32, u32);
        void (*PPResetMultiBuffer)(const void *);
        void (*PPAddMultiBuffer)(const void *, const void *);
        PPResult (*PPGetNextOutput)(const void *, PPOutput *pOut);
        DecPpInterface decPpIf;
        DecPpQuery ppInfo;
        const DWLLinearMem_t * sentPicToPp[17]; /* list of pictures sent to pp */
        const DWLLinearMem_t * queuedPicToPp[2]; /* queued picture that should be processed next */
        u32 multiMaxId; /* maximum position used in sentPicToPp[] */
    } pp;
    u32 PrePpBufSize;/* prvious PP buffer size.*/
    u32 PrePpBufNum;/* previous PP buffer number.*/

#ifdef USE_EXTERNAL_BUFFER
    u32 nextBufSize;  /* size of the requested external buffer */
    u32 bufNum;        /* number of buffers (with size of nextBufSize) requested to be allocated externally */
    struct DWLLinearMem *bufToFree;
    u32 bufferIndex[2];
    u32 bMVC;
    u32 nExtBufSize;
#endif
#ifdef USE_OUTPUT_RELEASE
    u32 abort;
#endif
    vsios_mutex_t protect_mutex;

#ifdef USE_RANDOM_TEST
  struct ErrorParams errorParams;
  u32 streamNotConsumed;
  u32 prevInputLen;
  FILE *ferrorStream;
#endif

//#ifdef SKIP_OPENB_FRAME
    u32 firstEntryPoint;
    u32 entryIsIDR;
    u32 entryIsI;
    i32 entryPOC;
    u32 skipB;
//#endif
    u32 useAdaptiveBuffers;
    u32 nGuardSize;
    u32 secureMode;
    g1_addr_t secureDataBusAddress;
    u32 secureDataLen;
} decContainer_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

#endif /* #ifdef H264HWD_CONTAINER_H */
