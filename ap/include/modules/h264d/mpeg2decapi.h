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


#ifndef __MPEG2DECAPI_H__
#define __MPEG2DECAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
#include "decapicommon.h"
#ifdef USE_EXTERNAL_BUFFER
#include "dwl.h"
#endif


/*------------------------------------------------------------------------------
    API type definitions
------------------------------------------------------------------------------*/

    /* Return values */
    typedef enum
    {
        MPEG2DEC_OK = 0,
        MPEG2DEC_STRM_PROCESSED = 1,
        MPEG2DEC_PIC_RDY = 2,
        MPEG2DEC_HDRS_RDY = 3,
        MPEG2DEC_HDRS_NOT_RDY = 4,
        MPEG2DEC_PIC_DECODED = 5,
        MPEG2DEC_NONREF_PIC_SKIPPED = 6,/* Skipped non-reference picture */
#ifdef USE_OUTPUT_RELEASE
        MPEG2DEC_ABORTED = 7,
        MPEG2DEC_END_OF_STREAM = 8,
#endif

#ifdef USE_EXTERNAL_BUFFER
        /** Waiting for external buffers allocated. */
        MPEG2DEC_WAITING_FOR_BUFFER = 9,
#endif
#ifdef USE_OUTPUT_RELEASE
        MPEG2DEC_FLUSHED = 10,
#endif
        MPEG2DEC_BUF_EMPTY = 11,

        MPEG2DEC_PARAM_ERROR = -1,
        MPEG2DEC_STRM_ERROR = -2,
        MPEG2DEC_NOT_INITIALIZED = -3,
        MPEG2DEC_MEMFAIL = -4,
        MPEG2DEC_INITFAIL = -5,
        MPEG2DEC_STREAM_NOT_SUPPORTED = -8,
#ifdef USE_EXTERNAL_BUFFER
        MPEG2DEC_EXT_BUFFER_REJECTED = -9, /**<\hideinitializer */
#endif

        MPEG2DEC_HW_RESERVED = -254,
        MPEG2DEC_HW_TIMEOUT = -255,
        MPEG2DEC_HW_BUS_ERROR = -256,
        MPEG2DEC_SYSTEM_ERROR = -257,
        MPEG2DEC_DWL_ERROR = -258,
        MPEG2DEC_FORMAT_NOT_SUPPORTED = -1000
    } Mpeg2DecRet;

    /* decoder output picture format */
    typedef enum
    {
        MPEG2DEC_SEMIPLANAR_YUV420 = 0x020001,
        MPEG2DEC_TILED_YUV420 = 0x020002
    } Mpeg2DecOutFormat;

    /* DAR (Display aspect ratio) */
    typedef enum
    {
        MPEG2DEC_1_1 = 0x01,
        MPEG2DEC_4_3 = 0x02,
        MPEG2DEC_16_9 = 0x03,
        MPEG2DEC_2_21_1 = 0x04
    } Mpeg2DecDARFormat;

    typedef struct
    {
        u32 *pVirtualAddress;
        g1_addr_t busAddress;
    } Mpeg2DecLinearMem;

    /* Decoder instance */
    typedef void *Mpeg2DecInst;

    /* Input structure */
    typedef struct
    {
        u8 *pStream;         /* Pointer to stream to be decoded              */
        g1_addr_t streamBusAddress;   /* DMA bus address of the input stream */
        u32 dataLen;         /* Number of bytes to be decoded                */
        u32 picId;
        u32 skipNonReference; /* Flag to enable decoder skip non-reference
                               * frames to reduce processor load */
    } Mpeg2DecInput;

    /* Time code */
    typedef struct
    {
        u32 hours;
        u32 minutes;
        u32 seconds;
        u32 pictures;
    } Mpeg2DecTime;

    typedef struct
    {
        u8 *pStrmCurrPos;
        g1_addr_t strmCurrBusAddress; /* DMA bus address location where the decoding
                                 * ended */
        u32 dataLeft;
    } Mpeg2DecOutput;

    /* stream info filled by Mpeg2DecGetInfo */
    typedef struct
    {
        u32 frameWidth;
        u32 frameHeight;
        u32 codedWidth;
        u32 codedHeight;
        u32 profileAndLevelIndication;
        u32 displayAspectRatio;
        u32 streamFormat;
        u32 videoFormat;
        u32 videoRange;      /* ??? only [0-255] */
        u32 interlacedSequence;
        DecDpbMode dpbMode;         /* DPB mode; frame, or field interlaced */
#ifdef USE_EXTERNAL_BUFFER
        u32 picBuffSize;
#endif
        u32 multiBuffPpSize;
        Mpeg2DecOutFormat outputFormat;
    } Mpeg2DecInfo;

    typedef struct
    {
        u8 *pOutputPicture;
        g1_addr_t outputPictureBusAddress;
        u32 frameWidth;
        u32 frameHeight;
        u32 codedWidth;
        u32 codedHeight;
        u32 keyPicture;
        u32 picId;
        u32 decodeId;
        u32 picCodingType[2];
        u32 interlaced;
        u32 fieldPicture;
        u32 topField;
        u32 firstField;
        u32 repeatFirstField;
        u32 singleField;
        u32 outputOtherField;
        u32 repeatFrameCount;
        u32 numberOfErrMBs;
        DecDpbMode dpbMode;         /* DPB mode; frame, or field interlaced */
        DecOutFrmFormat outputFormat;
        Mpeg2DecTime timeCode;
    } Mpeg2DecPicture;

    /* Version information */
    typedef struct
    {
        u32 major;           /* API major version */
        u32 minor;           /* API minor version */

    } Mpeg2DecApiVersion;

    typedef struct DecSwHwBuild_  Mpeg2DecBuild;
#ifdef USE_EXTERNAL_BUFFER
    /*!\struct Mpeg2DecBufferInfo_
     * \brief Reference buffer information
     *
     * A structure containing the reference buffer information, filled by
     * Mpeg2DecGetBufferInfo()
     *
     * \typedef Mpeg2DecBufferInfo
     * A typename for #Mpeg2DecBufferInfo_.
     */
    typedef struct Mpeg2DecBufferInfo_ {
        u32 nextBufSize;
        u32 bufNum;
        DWLLinearMem_t bufToFree;
#ifdef ASIC_TRACE_SUPPORT
        u32 is_frame_buffer;
#endif
    }Mpeg2DecBufferInfo;
#endif

/*------------------------------------------------------------------------------
    Prototypes of Decoder API functions
------------------------------------------------------------------------------*/

    Mpeg2DecApiVersion Mpeg2DecGetAPIVersion(void);

    Mpeg2DecBuild Mpeg2DecGetBuild(void);

    Mpeg2DecRet Mpeg2DecInit(Mpeg2DecInst * pDecInst,
#ifdef USE_EXTERNAL_BUFFER
                             const void *dwl,
#endif
                             DecErrorHandling errorHandling,
                             u32 numFrameBuffers,
                             DecDpbFlags dpbFlags,
                             u32 useAdaptiveBuffers,
                             u32 nGuardSize);

    Mpeg2DecRet Mpeg2DecDecode(Mpeg2DecInst decInst,
                               Mpeg2DecInput * pInput,
                               Mpeg2DecOutput * pOutput);

    Mpeg2DecRet Mpeg2DecGetInfo(Mpeg2DecInst decInst, Mpeg2DecInfo * pDecInfo);

    Mpeg2DecRet Mpeg2DecNextPicture(Mpeg2DecInst decInst,
                                    Mpeg2DecPicture * pPicture,
                                    u32 endOfStream);
#ifdef USE_OUTPUT_RELEASE
    Mpeg2DecRet Mpeg2DecPictureConsumed(Mpeg2DecInst decInst,
                                        Mpeg2DecPicture *pPicture);

    Mpeg2DecRet Mpeg2DecEndOfStream(Mpeg2DecInst decInst, u32 strmEndFlag);
    Mpeg2DecRet Mpeg2DecAbort(Mpeg2DecInst decInst);
    Mpeg2DecRet Mpeg2DecAbortAfter(Mpeg2DecInst decInst);
#endif

    void Mpeg2DecRelease(Mpeg2DecInst decInst);

    Mpeg2DecRet Mpeg2DecPeek(Mpeg2DecInst decInst, Mpeg2DecPicture * pPicture);
#ifdef USE_EXTERNAL_BUFFER
    Mpeg2DecRet Mpeg2DecGetBufferInfo(Mpeg2DecInst decInst, Mpeg2DecBufferInfo *mem_info);

    Mpeg2DecRet Mpeg2DecAddBuffer(Mpeg2DecInst decInst, DWLLinearMem_t *info);
#endif

/*------------------------------------------------------------------------------
    Prototype of the API trace funtion. Traces all API entries and returns.
    This must be implemented by the application using the decoder API!
    Argument:
        string - trace message, a null terminated string
------------------------------------------------------------------------------*/
    void Mpeg2DecTrace(const char *string);

#ifdef __cplusplus
}
#endif

#endif                       /* __MPEG2DECAPI_H__ */
