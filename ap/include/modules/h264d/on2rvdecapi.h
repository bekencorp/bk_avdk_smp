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


#ifndef __ON2RVDECAPI_H__
#define __ON2RVDECAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
#ifdef USE_EXTERNAL_BUFFER
#include "dwl.h"
#endif

/* return values */
#define MAKE_RESULT(sev,fac,code) \
    (((u32)sev << 31) | ((u32)4 << 16) | ((fac<<6) | (code)))

#define ON2RVDEC_OK                     MAKE_RESULT(0,0,0x0000)
#ifdef USE_EXTERNAL_BUFFER
#define ON2RVDEC_HDRS_RDY               MAKE_RESULT(0,1,0x0000)
#define ON2RVDEC_WAITING_BUFFER         MAKE_RESULT(0,4,0x0000)
#define ON2RVDEC_EXT_BUFFER_REJECTED    MAKE_RESULT(0,7,0x0000)
#endif

#ifdef USE_OUTPUT_RELEASE
#define ON2RVDEC_END_OF_STREAM          MAKE_RESULT(0,5,0x0000)
#endif

#define ON2RVDEC_OUTOFMEMORY            MAKE_RESULT(1,7,0x000e)
#define ON2RVDEC_INVALID_PARAMETER      MAKE_RESULT(1,7,0x0057)
#define ON2RVDEC_NOTIMPL                MAKE_RESULT(1,0,0x4001)
#define ON2RVDEC_POINTER                MAKE_RESULT(1,0,0x4003)
#define ON2RVDEC_FAIL                   MAKE_RESULT(1,0,0x4005)

typedef u32 On2RvDecRet;

/* custom message handling */
#define ON2RV_MSG_ID_Set_RVDecoder_RPR_Sizes 36

typedef u32 On2RvCustomMessage_ID;

typedef struct
{
    On2RvCustomMessage_ID message_id;
    u32 num_sizes;
    u32 *sizes;
} On2RvMsgSetDecoderRprSizes;

/* input and output flag definitions */
#define ON2RV_DECODE_MORE_FRAMES    0x00000001
#define ON2RV_DECODE_DONT_DRAW      0x00000002
#define ON2RV_DECODE_KEY_FRAME      0x00000004
#define ON2RV_DECODE_B_FRAME        0x00000008
#define ON2RV_DECODE_LAST_FRAME     0x00000200

/* frame formats */
/* Reference picture format types */
typedef enum 
{
    ON2RV_REF_FRM_RASTER_SCAN          = 0,
    ON2RV_REF_FRM_TILED_DEFAULT        = 1
} On2RvRefFrmFormat;

/* Output picture format types */
typedef enum
{
    ON2RV_OUT_FRM_RASTER_SCAN          = 0,
    ON2RV_OUT_FRM_TILED_8X4            = 1
} On2RvOutFrmFormat;

/* input and output structures */
typedef struct
{
    i32 bIsValid;
    u32 ulSegmentOffset;
} codecSegmentInfo;

typedef struct
{
    u32 dataLength;
    i32 bInterpolateImage;
    u32 numDataSegments;
    codecSegmentInfo *pDataSegments;
    u32 flags;
    u32 timestamp;
    g1_addr_t streamBusAddr;
    u32 skipNonReference;
} On2DecoderInParams;

typedef struct
{
    u32 numFrames;
    u32 notes;
    u32 timestamp;
    u32 width;
    u32 height;
    u8 *pOutFrame;
#ifdef USE_OUTPUT_RELEASE
    g1_addr_t outBusAddr;
#endif
    On2RvOutFrmFormat outputFormat;
} On2DecoderOutParams;

/* decoder initialization structure */
typedef struct
{
    u16 outtype;
    u16 pels;
    u16 lines;
    u16 nPadWidth;
    u16 nPadHeight;
    u16 pad_to_32;
    u32 ulInvariants;
    i32 packetization;
    u32 ulStreamVersion;
} On2DecoderInit;

#ifdef USE_EXTERNAL_BUFFER
typedef struct
    {
        u32 frameWidth;
        u32 frameHeight;
        u32 picBuffSize;
    } On2DecoderInfo;

typedef struct
    {
        u32 nextBufSize;
        u32 bufNum;
        DWLLinearMem_t bufToFree;
    }On2DecoderBufferInfo;
#endif

/* decoding function */
On2RvDecRet On2RvDecDecode(u8 *pRV10Packets,
    u8   *pDecodedFrameBuffer, /* unused */
    void *pInputParams,
    void *pOutputParams,
    void *decInst);

/* initialization function */
On2RvDecRet On2RvDecInit(void *pRV10Init,
#ifdef USE_EXTERNAL_BUFFER
    const void *dwl,
#endif
    void **pDecInst);

/* release function */
On2RvDecRet On2RvDecFree(void *decInst);

/* custom message handling function. Only Set_RPR_Sizes message implemented */
On2RvDecRet On2RvDecCustomMessage(void *msg_id, void *decInst);

/* unused, always returns DEC_NOTIMPL */
On2RvDecRet On2RvDecHiveMessage(void *msg, void *decInst);

/* function to obtain last decoded picture out from the decoder */
On2RvDecRet On2RvDecPeek(void *pOutputParams, void *decInst);

/* function to specify nbr of picture buffers to decoder */
On2RvDecRet On2RvDecSetNbrOfBuffers( u32 nbrBuffers, void *global );

/* function to specify reference frame format to use */
On2RvDecRet On2RvDecSetReferenceFrameFormat( 
    On2RvRefFrmFormat referenceFrameFormat, void *global );

#ifdef USE_EXTERNAL_BUFFER
On2RvDecRet On2RvDecGetInfo(On2DecoderInfo *pDecInfo, void *global);

On2RvDecRet On2RvDecGetBufferInfo(On2DecoderBufferInfo *mem_info, void *global);

On2RvDecRet On2RvDecAddBuffer(DWLLinearMem_t *info, void *global);
#endif

#ifdef USE_OUTPUT_RELEASE
On2RvDecRet On2RvDecNextPicture(void *pOutputParams, void *global);

On2RvDecRet On2RvDecPictureConsumed(void *pOutputParams, void *global);
#endif


#ifdef __cplusplus
}
#endif

#endif  /* __ON2RVDECAPI_H__ */
