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


/*!\file
 * \brief Describes the VP8 decoder algorithm interface to applications.
 *
 * This file describes the interface between an application and a
 * video decoder.
 *
 */

#ifndef __VP8DECAPI_H__
#define __VP8DECAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
#include "decapicommon.h"
#include "dwl.h"


/*!\defgroup VP8API VP8 Decoder*/

/*! \addtogroup VP8API
 *  @{
 */

/*------------------------------------------------------------------------------
    API type definitions
------------------------------------------------------------------------------*/

    /*!\enum VP8DecRet_
     * Return values for API
     *
     * \typedef VP8DecRet
     * A typename for #VP8DecRet_
     */

    /* Return values */
    typedef enum VP8DecRet_
    {
        /** Success */
        VP8DEC_OK = 0, /**<\hideinitializer */
        /** Stream processed */
        VP8DEC_STRM_PROCESSED = 1, /**<\hideinitializer */
        /** Picture available for output */
        VP8DEC_PIC_RDY = 2, /**<\hideinitializer */
        /** Picture decoded */
        VP8DEC_PIC_DECODED = 3, /**<\hideinitializer */
        /** New stream headers decoded */
        VP8DEC_HDRS_RDY = 4, /**<\hideinitializer */
        /** Advanced coding tools detected in stream */
        VP8DEC_ADVANCED_TOOLS = 5, /**<\hideinitializer */
        /** A slice was decoded */
        VP8DEC_SLICE_RDY = 6, /**<\hideinitializer */
        /** End-of-stream state set in the decoder */
        VP8DEC_END_OF_STREAM = 7, /**<\hideinitializer */
#ifdef USE_OUTPUT_RELEASE
        /** Decoder is aborted */
        VP8DEC_ABORTED = 8, /**<\hideinitializer */
#endif
        /** Waiting for external buffers allocated. */
        VP8DEC_WAITING_FOR_BUFFER = 9,

#ifdef USE_OUTPUT_RELEASE
        VP8DEC_FLUSHED = 10,
#endif

        /** Invalid parameter was used */
        VP8DEC_PARAM_ERROR = -1, /**<\hideinitializer */
        /* An unrecoverable error in decoding */
        VP8DEC_STRM_ERROR = -2, /**<\hideinitializer */
        /** The decoder has not been initialized */
        VP8DEC_NOT_INITIALIZED = -3, /**<\hideinitializer */
        /** Memory allocation failed */
        VP8DEC_MEMFAIL = -4, /**<\hideinitializer */
        /** Initialization failed */
        VP8DEC_INITFAIL = -5, /**<\hideinitializer */
        /** Video sequence information is not available because
         stream headers have not been decoded */
        VP8DEC_HDRS_NOT_RDY = -6, /**<\hideinitializer */
        /** Video sequence frame size or tools not supported */
        VP8DEC_STREAM_NOT_SUPPORTED = -8,  /**<\hideinitializer */
#ifdef USE_EXTERNAL_BUFFER
        VP8DEC_EXT_BUFFER_REJECTED = -9, /**<\hideinitializer */
#endif
        /** Driver could not reserve decoder hardware */
        VP8DEC_HW_RESERVED = -254, /**<\hideinitializer */
        /** Hardware timeout occurred */
        VP8DEC_HW_TIMEOUT = -255, /**<\hideinitializer */
        /** Hardware received error status from system bus */
        VP8DEC_HW_BUS_ERROR = -256, /**<\hideinitializer */
        /** Hardware encountered an unrecoverable system error */
        VP8DEC_SYSTEM_ERROR = -257, /**<\hideinitializer */
        /** Decoder wrapper encountered an error */
        VP8DEC_DWL_ERROR = -258, /**<\hideinitializer */

        /** Evaluation limit exceeded */
        VP8DEC_EVALUATION_LIMIT_EXCEEDED = -999, /**<\hideinitializer */
        /** Video format not supported */
        VP8DEC_FORMAT_NOT_SUPPORTED = -1000 /**<\hideinitializer */
    } VP8DecRet;

    /*!\enum VP8DecOutFormat_
     *  Decoder output picture format.
     *
     * \typedef VP8DecOutFormat
     * A typename for #VP8DecOutFormat_.
     */
    typedef enum VP8DecOutFormat_
    {
        VP8DEC_SEMIPLANAR_YUV420 = 0x020001, /**<\hideinitializer */
        VP8DEC_TILED_YUV420 = 0x020002 /**<\hideinitializer */
    } VP8DecOutFormat;

    /*!\enum VP8DecFormat_
     *  Format of the input sequence.
     *
     * \typedef VP8DecFormat
     * A typename for #VP8DecFormat_.
     */
    typedef enum VP8DecFormat_
    {
        VP8DEC_VP7 = 0x01, /**<\hideinitializer */
        VP8DEC_VP8 = 0x02, /**<\hideinitializer */
        VP8DEC_WEBP = 0x03 /**<\hideinitializer */
    } VP8DecFormat;

/*!\struct VP8DecInput_
     * \brief Decode input structure
     *
     * \typedef VP8DecInput
     * A typename for #VP8DecInput_.
     */
    /* Output structure */

    typedef struct VP8DecInput_
    {
        const u8 *pStream;   /**< Pointer to the input buffer */
        g1_addr_t streamBusAddress; /**< DMA bus address of the input buffer */
        u32 dataLen;          /**< Number of bytes to be decoded */
        u32 picId;
        u32 sliceHeight;     /**< height of WebP slice, unused for other formats */
        u32 *pPicBufferY;    /**< luminance output address of user allocated buffer,
                             used in conjunction with external buffer allocation */
        g1_addr_t picBufferBusAddressY; /**< DMA bus address for luminance output */
        u32 *pPicBufferC;    /**< chrominance output address of user allocated buffer,
                             used in conjunction with external buffer allocation */
        g1_addr_t picBufferBusAddressC; /**< DMA bus address for luminance output */
        void *pUserData; /**< user data to be passed in multicore callback,
                             used in conjunction with multicore decoding */
    } VP8DecInput;

/*!\struct VP8DecOutput_
     * \brief Decode output structure
     *
     * \typedef VP8DecOutput
     * A typename for #VP8DecOutput_.
     */
    /* Output structure */

    typedef struct VP8DecOutput_
    {
        u32 unused; /**< This structure currently unused */
        u32 dataLeft;
    } VP8DecOutput;

    #define VP8_SCALE_MAINTAIN_ASPECT_RATIO     0 /**<\hideinitializer */
    #define VP8_SCALE_TO_FIT                    1 /**<\hideinitializer */
    #define VP8_SCALE_CENTER                    2 /**<\hideinitializer */
    #define VP8_SCALE_OTHER                     3 /**<\hideinitializer */

    #define VP8_STRIDE_NOT_USED                 0 /**<\hideinitializer */

    /*!\struct VP8DecInfo_
     * \brief Decoded stream information
     *
     * A structure containing the decoded stream information, filled by
     * VP8DecGetInfo()
     *
     * \typedef VP8DecInfo
     * A typename for #VP8DecInfo_.
     */
    /* stream info ddfilled by VP8DecGetInfo */
    typedef struct VP8DecInfo_
    {
        u32 vpVersion;       /**< VP codec version defined in input stream */
        u32 vpProfile;       /**< VP cocec profile defined in input stream */
#ifdef USE_EXTERNAL_BUFFER
        u32 picBuffSize;
#endif
        u32 codedWidth;      /**< coded width of the picture */
        u32 codedHeight;     /**< coded height of the picture */
        u32 frameWidth;      /**< pixels width of the frame as stored in memory */
        u32 frameHeight;     /**< pixel height of the frame as stored in memory */
        u32 scaledWidth;     /**< scaled width of the displayed video */
        u32 scaledHeight;    /**< scaled height of the displayed video */
        DecDpbMode dpbMode;             /**< DPB mode; frame, or field interlaced */
        VP8DecOutFormat outputFormat;   /**< format of the output frame */
    } VP8DecInfo;

     /*!\struct VP8DecApiVersion_
     * \brief API Version information
     *
     * A structure containing the major and minor version number of the API.
     *
     * \typedef VP8DecApiVersion
     * A typename for #VP8DecApiVersion_.
     */
    typedef struct VP8DecApiVersion_
    {
        u32 major;           /**< API major version */
        u32 minor;           /**< API minor version */
    } VP8DecApiVersion;

    typedef struct DecSwHwBuild_ VP8DecBuild;
    /*!\struct VP8DecPicture_
     * \brief Decoded picture information
     *
     * Parameters of a decoded picture, filled by VP8DecNextPicture().
     *
     * \typedef VP8DecPicture
     * A typename for #VP8DecPicture_.
     */
    typedef struct VP8DecPicture_
    {
        u32 codedWidth;      /**< coded width of the picture */
        u32 codedHeight;     /**< coded height of the picture */
        u32 frameWidth;      /**< pixels width of the frame as stored in memory */
        u32 frameHeight;     /**< pixel height of the frame as stored in memory */
        u32 lumaStride;      /**< pixel row stride for luminance */
        u32 chromaStride;    /**< pixel row stride for chrominance */
        const u32 *pOutputFrame;    /**< Pointer to the frame */
        g1_addr_t outputFrameBusAddress;  /**< DMA bus address of the output frame buffer */
        const u32 *pOutputFrameC;   /**< Pointer to chrominance output */
        g1_addr_t outputFrameBusAddressC;  /**< DMA bus address of the chrominance
                                      *output frame buffer */
        u32 picId;           /**< Identifier of the Frame to be displayed */
        u32 decodeId;
        u32 isIntraFrame;    /**< Indicates if Frame is an Intra Frame */
        u32 isGoldenFrame;   /**< Indicates if Frame is a Golden reference Frame */
        u32 nbrOfErrMBs;     /**< Number of concealed macroblocks in the frame  */
        u32 numSliceRows;    /**< Number of luminance pixels rows in WebP output picture buffer.
                             If set to 0, whole picture ready.*/
#ifdef USE_OUTPUT_RELEASE
        u32 lastSlice;       /**< last slice flag  */
#endif
        DecOutFrmFormat outputFormat;
    } VP8DecPicture;

    /*!\struct VP8DecPictureBufferProperties_
     * \brief Decoded picture information
     *
     * Parameters of a decoded picture, filled by VP8DecNextPicture().
     *
     * \typedef VP8DecPictureBufferProperties
     * A typename for #VP8DecPictureBufferProperties_.
     */
    typedef struct VP8DecPictureBufferProperties_
    {
        u32 lumaStride; /**< Specifies stride for luminance buffer in bytes.
                         *Stride must be a power of 2.  */
        u32 chromaStride; /**< Specifies stride for chrominance buffer in
                           *bytes. Stride must be a power of 2. */

        u32 **pPicBufferY;    /**< Pointer to luma buffers */
        g1_addr_t *picBufferBusAddressY;  /**< DMA bus address of the luma buffers */
        u32 **pPicBufferC;    /**< Pointer to chroma buffers */
        g1_addr_t *picBufferBusAddressC;  /**< DMA bus address of the chroma buffers */
        u32 numBuffers; /**< Number of buffers supplied in the above arrays.
                         * Minimum value is 4 and maximum 16 */

    } VP8DecPictureBufferProperties;
#ifdef USE_EXTERNAL_BUFFER
    /*!\struct VP8DecBufferInfo_
     * \brief Reference buffer information
     *
     * A structure containing the reference buffer information, filled by
     * VP8DecGetBufferInfo()
     *
     * \typedef VP8DecBufferInfo
     * A typename for #VP8DecBufferInfo_.
     */
    typedef struct VP8DecBufferInfo_ {
        u32 nextBufSize;
        u32 bufNum;
        DWLLinearMem_t bufToFree;
#ifdef ASIC_TRACE_SUPPORT
        u32 is_frame_buffer;
#endif
    }VP8DecBufferInfo;
#endif

    /*!\brief Stream consumed callback prototype
     *
     * This callback is invoked by the decoder to notify the application that
     * a stream buffer was fully processed and can be reused.
     *
     * \param pStream base address of a buffer that was set as input when
     *                calling VP8DecDecode().
     * \param pUserData application provided pointer to some private data.
     *                  This is set at decoder initialization time.
     *
     * \sa VP8DecMCInit();
     */
    typedef void VP8DecMCStreamConsumed(u8 *pStream, void *pUserData);

    /*!\struct VP8DecMCConfig_
     * \brief Multicore decoder init configuration
     *
     * \typedef VP8DecMCConfig
     *  A typename for #VP8DecMCConfig_.
     */
    typedef struct VP8DecMCConfig_
    {
        /*! Application provided callback for stream buffer processed. */
        VP8DecMCStreamConsumed *streamConsumedCallback;
    } VP8DecMCConfig;

    /* Decoder instance */
    typedef const void *VP8DecInst;

    typedef struct VP8DecPpBufferInfo_ {
        u32 nextBufSize;
        u32 bufNum;
    } VP8DecPpBufferInfo;
/*------------------------------------------------------------------------------
    Prototypes of Decoder API functions
------------------------------------------------------------------------------*/
    /*!\brief Get API version information
     *
     * Return the version information of the SW API.
     * Static implementation, does not require a decoder instance.
     */
    VP8DecApiVersion VP8DecGetAPIVersion(void);

    /*!\brief Read SW/HW build information
     *
     * Returns the hardware and software build information of the decoder.
     * Static implementation, does not require a decoder instance.
     */
    VP8DecBuild VP8DecGetBuild(void);

    /*!\brief Create a single core decoder instance
     *
     * Single core decoder can decode VP7 and WebP stream in addition to VP8.
     *
     * Every instance has to be released with VP8DecRelease().
     *
     *\note Use VP8DecMCInit() for creating an instance with multicore support.
     *
     */
    VP8DecRet VP8DecInit(VP8DecInst *pDecInst,
#ifdef USE_EXTERNAL_BUFFER
                         const void *dwl,
#endif
                         VP8DecFormat decFormat,
                         DecErrorHandling errorHandling,
                         u32 numFrameBuffers,
                         DecDpbFlags dpbFlags,
                         u32 useAdaptiveBuffers,
                         u32 nGuardSize);

    /*!\brief Setup custom frame buffers.
     *
     */
    VP8DecRet VP8DecSetPictureBuffers(VP8DecInst decInst,
                                      VP8DecPictureBufferProperties *pPbp);

    /*!\brief Release a decoder instance
     *
     * VP8DecRelease closes the decoder instance \c decInst and releases all
     * internally allocated resources.
     *
     * When connected with the Hantro HW post-processor, the post-processor
     * must be disconnected before releasing the decoder instance.
     * Refer to \ref PPDOC "PP API manual" for details.
     *
     * \param decInst instance to be released
     *
     * \retval #VP8DEC_OK for success
     * \returns A negative return value signals an error.
     *
     * \sa VP8DecInit()
     * \sa VP8DecMCInit()
     *
     */

    void VP8DecRelease(VP8DecInst decInst);

    /*!\brief Decode data
     *
     * \warning Do not use with multicore instances!
     *       Use instead VP8DecMCDecode().
     *
     */

     /* Single core specific */

    VP8DecRet VP8DecDecode(VP8DecInst decInst,
                           const VP8DecInput *pInput,
                           VP8DecOutput *pOutput);

    /*!\brief Read next picture in display order
     *
     * \warning Do not use with multicore instances!
     *       Use instead VP8DecMCNextPicture().
     *
     * \sa VP8DecMCNextPicture()
     */
    VP8DecRet VP8DecNextPicture(VP8DecInst decInst,
                                VP8DecPicture *pPicture, u32 endOfStream);

#ifdef USE_OUTPUT_RELEASE
    VP8DecRet VP8DecPictureConsumed(VP8DecInst decInst,
                                      const VP8DecPicture *pPicture);

    VP8DecRet VP8DecEndOfStream(VP8DecInst decInst, u32 strmEndFlag);
#endif

    /*!\brief Read decoded stream information
     *
     */
    VP8DecRet VP8DecGetInfo(VP8DecInst decInst, VP8DecInfo *pDecInfo);

    /*!\brief Read last decoded picture
     *
     * \warning Do not use with multicore instances!
     *       Use instead VP8DecNextPicture().
     *
     * \sa VP8DecMCNextPicture()
     */
    VP8DecRet VP8DecPeek(VP8DecInst decInst, VP8DecPicture *pPicture);
#ifdef USE_EXTERNAL_BUFFER
    VP8DecRet VP8DecGetBufferInfo(VP8DecInst decInst, VP8DecBufferInfo *mem_info);

    VP8DecRet VP8DecAddBuffer(VP8DecInst decInst, DWLLinearMem_t *info);
#endif

#ifdef USE_OUTPUT_RELEASE
    VP8DecRet VP8DecAbort(VP8DecInst decInst);

    VP8DecRet VP8DecAbortAfter(VP8DecInst decInst);
#endif


    /* Multicore extension */

    /*!\brief Read number of HW cores available
     *
     * Multicore specific.
     *
     * Static implementation, does not require a decoder instance.
     *
     * \returns The number of available hardware decoding cores.
     */
    u32 VP8DecMCGetCoreCount(void);

    /*!\brief Create a multicore decoder instance
     *
     * Multicore specific.
     *
     * Use this to initialize a new multicore decoder instance. Only VP8 format
     * is supported.
     *
     *
     * Every instance has to be released with VP8DecRelease().
     *
     * \param pDecInst pointer where the newly created instance will be stored.
     * \param pMCInitCfg initialization parameters, which  cannot be altered later.
     *
     * \retval #VP8DEC_OK for success
     * \returns A negative return value signals an error.
     *
     * \sa VP8DecRelease()
     */
    VP8DecRet VP8DecMCInit(VP8DecInst *pDecInst,
                           VP8DecMCConfig *pMCInitCfg);

    /*!\brief Decode data
     *
     * Multicore specific.
     * This function decodes a frame from the current stream.
     * The input buffer shall contain the picture data for exactly one frame.
     *
     * \retval #VP8DEC_PIC_DECODED
                A new picture decoded.
     * \retval #VP8DEC_HDRS_RDY
     *          Headers decoded and activated. Stream header information is now
     *          readable with the function #VP8DecGetInfo.
     * \returns A negative return value signals an error.
     */
    VP8DecRet VP8DecMCDecode(VP8DecInst decInst,
                           const VP8DecInput *pInput,
                           VP8DecOutput *pOutput);

    /*!\brief Release picture buffer back to decoder
     *
     * Multicore specific.
     *
     * Use this function to return a picture back to the decoder once
     * consumed by application. Pictures are given to application by
     * VP8DecMCNextPicture().
     *
     * \param decInst a multicore decoder instance.
     * \param pPicture pointer to data that identifies the picture buffer.
     *        Shall be the exact data returned by VP8DecMCNextPicture().
     *
     * \retval #VP8DEC_OK for success
     * \returns A negative return value signals an error.
     *
     * \sa  VP8DecMCNextPicture()
     */
    VP8DecRet VP8DecMCPictureConsumed(VP8DecInst decInst,
                                      const VP8DecPicture *pPicture);

    /*!\brief Get next picture in display order
     *
     * Multicore specific.
     *
     * This function is used to get the decoded pictures out from the decoder.
     * The call will block until a picture is available for output or
     * an end-of-stream state is set in the decoder. Once processed, every
     * output picture has to be released back to decoder by application using
     * VP8DecMCPictureConsumed(). Pass to this function the same data returned
     * in \c pPicture.
     *
     * \param decInst a multicore decoder instance.
     * \param pPicture pointer to a structure that will be filled with
     *        the output picture parameters.
     *
     * \retval #VP8DEC_PIC_RDY to signal that a picture is available.
     * \retval #VP8DEC_END_OF_STREAM to signal that the decoder is
     *          in end-of-stream state.
     * \returns A negative return value signals an error.
     *
     *
     * \sa VP8DecMCPictureConsumed()
     * \sa VP8DecMCEndOfStream()
     *
     */
    VP8DecRet VP8DecMCNextPicture(VP8DecInst decInst,
                                  VP8DecPicture *pPicture);
    /*!\brief Set end-of-stream state in multicore decoder
     *
     * Multicore specific.
     *
     * This function is used to signal the decoder that the current decoding process ends.
     *  It must be called at the end of decoding so that all potentially
     * buffered pictures are flushed out and VP8DecNextPicture()
     * is unblocked.
     *
     * This call will block until all cores have finished processing and all
     * output pictures are processed by application
     * (i.e. VP8DecNextPicture() returns #VP8DEC_END_OF_STREAM).
     *
     * \param decInst a multicore decoder instance.
     *
     * \retval #VP8DEC_OK for success
     * \returns A negative return value signals an error.
     *
     * \sa  VP8DecMCNextPicture()
     */

     VP8DecRet VP8DecMCEndOfStream(VP8DecInst decInst);

     /*!\brief API internal tracing function prototype
     *
     * Traces all API entries and returns. This must be implemented by
     * the application using the decoder API.
     *
     * \param string Pointer to a NULL terminated char string.
     *
     */
    void VP8DecTrace(const char *string);


    /**
     * \brief This function is called by the client to add an external PP frame buffer into the decoder. \n
     * \param [in]   decInst      A decoder instance created earlier with a call to VP8DecInit. \n
                                  VP8DecInst is defined as: typedef const void *VP8DecInst.
     * \param [in]   info         Pointer to a DWLLinearMem structure.
     * \param [out]  VP8DecRet    value: VP8DEC_WAITING_FOR_BUFFER, VP8DEC_OK, VP8DEC_PARAM_ERROR
     */
    VP8DecRet VP8DecAddPpBuffer(VP8DecInst decInst,
                                DWLLinearMem_t *info);

    /**
     * \brief This function is called by the client to get the PP frame buffer information
     * requested by the decoder, including the requested buffer size and buffer numbers. \n
     * \param [in]     decInst     A decoder instance created earlier with a call to VP8DecInit. \n
                                   VP8DecInst is defined as: typedef const void *VP8DecInst.
     * \param [in]     buf_info    Pointer to a VP8DecPpBufferInfo structure.
     * \param [out]    VP8DecRet  value: VP8DEC_WAITING_FOR_BUFFER, VP8DEC_OK, VP8DEC_PARAM_ERROR
     */
    VP8DecRet VP8DecGetPpBufferInfo(VP8DecInst decInst,
                                    VP8DecPpBufferInfo *buf_info);

    /**
     * \brief This function informs the decoder that the client has finished processing the specific
     * PP picture and releases the PP picture buffer for the decoder to process the next picture. \n
     * \param [in]     decInst       A decoder instance created earlier with a call to VP8DecInit. \n
                                     VP8DecInst is defined as: typedef const void *VP8DecInst.
     * \param [in]     pPicture      Pointer to an VP8DecPicture structure used to return the picture parameters.

     * \param [out]    VP8DecRet    value: VP8DEC_OK, VP8DEC_PARAM_ERROR
     */
    VP8DecRet VP8DecPpPictureConsumed(VP8DecInst decInst,
                                      const VP8DecPicture *pPicture);
    /*! @}*/
#ifdef __cplusplus
}
#endif

#endif                       /* __VP8DECAPI_H__ */
