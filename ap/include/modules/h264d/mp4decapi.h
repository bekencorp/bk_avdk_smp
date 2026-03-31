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


#ifndef __MP4DECAPI_H__
    #define __MP4DECAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
#include "decapicommon.h"
#include "dwl.h"

/*------------------------------------------------------------------------------
    API type definitions
------------------------------------------------------------------------------*/

    typedef enum MP4DecStrmFmt_ {
        MP4DEC_MPEG4,
        MP4DEC_SORENSON,
        MP4DEC_CUSTOM_1
    } MP4DecStrmFmt;

    /* Return values */
    typedef enum MP4DecRet_{
        MP4DEC_OK = 0,
        MP4DEC_STRM_PROCESSED = 1,
        MP4DEC_PIC_RDY = 2,
        MP4DEC_PIC_DECODED = 3,
        MP4DEC_HDRS_RDY = 4,
        MP4DEC_DP_HDRS_RDY = 5,
        MP4DEC_NONREF_PIC_SKIPPED = 6,/* Skipped non-reference picture */

#ifdef USE_OUTPUT_RELEASE
        MP4DEC_ABORTED = 7,
        MP4DEC_END_OF_STREAM = 8,
#endif

#ifdef USE_EXTERNAL_BUFFER
        /** Waiting for external buffers allocated. */
        MP4DEC_WAITING_FOR_BUFFER = 9,
#endif
#ifdef USE_OUTPUT_RELEASE
        MP4DEC_FLUSHED = 10,
#endif

        MP4DEC_BUF_EMPTY = 11,
        MP4DEC_VOS_END = 14,
        MP4DEC_HDRS_NOT_RDY = 15,

        MP4DEC_PARAM_ERROR = -1,
        MP4DEC_STRM_ERROR = -2,
        MP4DEC_NOT_INITIALIZED = -4,
        MP4DEC_MEMFAIL = -5,
        MP4DEC_INITFAIL = -6,
        MP4DEC_FORMAT_NOT_SUPPORTED = -7,
        MP4DEC_STRM_NOT_SUPPORTED = -8,
#ifdef USE_EXTERNAL_BUFFER
        MP4DEC_EXT_BUFFER_REJECTED = -9,       /**<\hideinitializer */
#endif

        MP4DEC_HW_RESERVED = -254,
        MP4DEC_HW_TIMEOUT = -255,
        MP4DEC_HW_BUS_ERROR = -256,
        MP4DEC_SYSTEM_ERROR = -257,
        MP4DEC_DWL_ERROR = -258
    } MP4DecRet;

    /* decoder output picture format */
    typedef enum MP4DecOutFormat_
    {
        MP4DEC_SEMIPLANAR_YUV420 = 0x020001,
        MP4DEC_TILED_YUV420 = 0x020002
    } MP4DecOutFormat;

    typedef struct
    {
        u32 *pVirtualAddress;
        g1_addr_t busAddress;
    } MP4DecLinearMem;

    /* Decoder instance */
    typedef void *MP4DecInst;

    /* Input structure */
    typedef struct MP4DecInput_{
        const u8 *pStream;       /* Pointer to stream to be decoded  */
        g1_addr_t streamBusAddress; /* DMA bus address of the input stream */
        u32 dataLen;        /* Number of bytes to be decoded                */
        u32 enableDeblock;  /* Enable deblocking of post processed picture  */
                            /* NOTE: This parameter is not valid if the decoder
                             * is not used in pipeline mode with the post
                             * processor i.e. it has no effect on the
                             * decoding process */

        u32 picId;
        u32 skipNonReference; /* Flag to enable decoder skip non-reference 
                               * frames to reduce processor load */
    } MP4DecInput;

    /* Time code */
    typedef struct TimeCode_{
        u32 hours;
        u32 minutes;
        u32 seconds;
        u32 timeIncr;
        u32 timeRes;
    } MP4DecTime;

    typedef struct MP4DecOutput_{
        const u8  *pStrmCurrPos;
        g1_addr_t strmCurrBusAddress; /* DMA bus address location where the decoding
                                   ended */
        u32 dataLeft;
    } MP4DecOutput;

    /* stream info filled by MP4DecGetInfo */
    typedef struct MP4DecInfo_{
        u32 frameWidth;
        u32 frameHeight;
        u32 codedWidth;
        u32 codedHeight;
        u32 streamFormat;
        u32 profileAndLevelIndication;
        u32 videoFormat;
        u32 videoRange;
        u32 parWidth;
        u32 parHeight;
        u32 userDataVOSLen;
        u32 userDataVISOLen;
        u32 userDataVOLLen;
        u32 userDataGOVLen;
        u32 interlacedSequence;
        DecDpbMode dpbMode;         /* DPB mode; frame, or field interlaced */       
#ifdef USE_EXTERNAL_BUFFER
        u32 picBuffSize;
#endif
        u32 multiBuffPpSize;
        MP4DecOutFormat outputFormat;
    } MP4DecInfo;

    /* User data type */
    typedef enum {
        MP4DEC_USER_DATA_VOS = 0,
        MP4DEC_USER_DATA_VISO,
        MP4DEC_USER_DATA_VOL,
        MP4DEC_USER_DATA_GOV

    } MP4DecUserDataType;

    /* User data configuration */
    typedef struct {
        MP4DecUserDataType userDataType;
        u8  *pUserDataVOS;
        u32  userDataVOSMaxLen;
        u8  *pUserDataVISO;
        u32  userDataVISOMaxLen;
        u8  *pUserDataVOL;
        u32  userDataVOLMaxLen;
        u8  *pUserDataGOV;
        u32  userDataGOVMaxLen;
    } MP4DecUserConf;

    /* Version information */
    typedef struct MP4DecVersion_{
        u32 major;    /* API major version */
        u32 minor;    /* API minor version */
    } MP4DecApiVersion;

    typedef struct DecSwHwBuild_  MP4DecBuild;

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
    typedef struct MP4DecBufferInfo_ {
        u32 nextBufSize;
        u32 bufNum;
        DWLLinearMem_t bufToFree;
#ifdef ASIC_TRACE_SUPPORT
        u32 is_frame_buffer;
#endif
    }MP4DecBufferInfo;
#endif

    typedef struct
    {
        const u8 *pOutputPicture;
        g1_addr_t outputPictureBusAddress;
        u32 frameWidth;
        u32 frameHeight;
        u32 codedWidth;
        u32 codedHeight;
        u32 keyPicture;
        u32 picId;
        u32 decodeId;
        u32 picCodingType;
        u32 interlaced;
        u32 fieldPicture;
        u32 topField;
        u32 nbrOfErrMBs;
        DecOutFrmFormat outputFormat;
        MP4DecTime timeCode;
    } MP4DecPicture;

/*------------------------------------------------------------------------------
    Prototypes of Decoder API functions
------------------------------------------------------------------------------*/

    MP4DecApiVersion MP4DecGetAPIVersion(void);

    MP4DecBuild MP4DecGetBuild(void);

    MP4DecRet MP4DecInit(MP4DecInst * pDecInst,
#ifdef USE_EXTERNAL_BUFFER
                         const void *dwl,
#endif
                         MP4DecStrmFmt strmFmt,
                         DecErrorHandling errorHandling,
                         u32 numFrameBuffers,
                         DecDpbFlags dpbFlags,
                         u32 useAdaptiveBuffers,
                         u32 nGuardSize);

    MP4DecRet MP4DecDecode(MP4DecInst decInst,
                           const MP4DecInput   * pInput,
                           MP4DecOutput        * pOutput);

    MP4DecRet MP4DecSetInfo(MP4DecInst * pDecInst, 
                            const u32 width, 
                            const u32 height );

    MP4DecRet MP4DecGetInfo(MP4DecInst decInst,
                                MP4DecInfo  * pDecInfo);

    MP4DecRet MP4DecGetUserData(MP4DecInst        decInst,
                                const MP4DecInput * pInput,
                                MP4DecUserConf    * pUserDataConfig);

    MP4DecRet MP4DecNextPicture(MP4DecInst        decInst,
                                MP4DecPicture    *pPicture,
                                u32               endOfStream);

#ifdef USE_OUTPUT_RELEASE
    MP4DecRet MP4DecPictureConsumed(MP4DecInst    decInst,
                                    MP4DecPicture *pPicture);

    MP4DecRet MP4DecEndOfStream(MP4DecInst decInst, u32 strmEndFlag);
#endif

    void  MP4DecRelease(MP4DecInst decInst);

    MP4DecRet MP4DecPeek(MP4DecInst        decInst,
                         MP4DecPicture    *pPicture);
#ifdef USE_EXTERNAL_BUFFER
    MP4DecRet MP4DecGetBufferInfo(MP4DecInst decInst, MP4DecBufferInfo *memInfo);

    MP4DecRet MP4DecAddBuffer(MP4DecInst decInst, DWLLinearMem_t *info);
#endif

#ifdef USE_OUTPUT_RELEASE
    MP4DecRet MP4DecAbort(MP4DecInst decInst);

    MP4DecRet MP4DecAbortAfter(MP4DecInst decInst);
#endif

/*------------------------------------------------------------------------------
    Prototype of the API trace funtion. Traces all API entries and returns.
    This must be implemented by the application using the decoder API!
    Argument:
        string - trace message, a null terminated string
------------------------------------------------------------------------------*/
    void MP4DecTrace(const char *string);

#ifdef __cplusplus
}
#endif

#endif /* __MP4DECAPI_H__ */

