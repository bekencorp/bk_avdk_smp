/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2015 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Preprocessor setup
--
------------------------------------------------------------------------------*/
#ifndef __ENC_HELPER_H__
#define __ENC_HELPER_H__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "base_type.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
/**
 * \defgroup api_helper Helper API
 *
 * Helper API will provide some function to help users to implement
 * features in application. currently, the helpers include,
 *  - buffer calculation for DEC400;
 *  - Prepare T35 packet for HDR10+ or Dolby Vision.
 *
 * @{
 */

/** Calculat the size of tile status buffer for save the side information of
 * DEC400 compression engine.
 *
 * \param [in] type the picture format defined in EncPixelFormat.
 * \param [in] width picture width in pixels
 * \param [in] height picture height in pixels
 * \param [in] alignment the alignement in byte for picture width which is
 *           reuqired for compress one tile. when 0 is set, assume 256 bytes
 *           are used. currenly, only256 bytes alignment is verified now.
 * \param [out] luma_Size the pointer will be filled with bytes size for luma
 *           tile status buffer.
 * \param [out] chroma_Size the pointer will be filled with bytes size for
 *           chroma tile status buffer.
 * \param [out] picture_Size the pointer will be filled with bytes size for
 *           status buffer of both luma and chroma.
 */
void EncGetDec400TsBufferSize(u32 type, u32 width, u32 height,
                               u32 alignment, u32 *luma_Size, u32 *chroma_Size,
                               u32 *picture_Size);

/**
 * Calculate the buffer size for aving one frame of cu infor.
 *
 * \param [in] width picture width of the encoded stream, not aligned.
 * \param [in] height picture height of the encoded stream, not aligned.
 * \param [in] ctu_size the mb or ctu size of one coding unit. it should be 16 for h264,
 *             and 64 for hevc/av1/vp9.
 * \param [in] cuInfoVersion indicate format of the cuinfo output
 * \param [in] cuinfoAlignment alignement bytes for save cuinfo in one ctb row
 * \param [in] aqInfoAlignment alignement bytes for save aqinfo in one ctb row
 *
 * \param [out] cuInfoSize buffer size for saving one frame cuinfo
 * \param [out] cuinfoStride buffer stride for one mb/ctb row cuinfo
 * \param [out] cuInfoTableSize buffer size for saving cuinfo table of one frame
 * \param [out] aqInfoSize buffer size for saving aqinfo of one frame
 * \param [out] aqInfoStride buffer stride for saving aqinfo of one frame
 *
 * \return total cuinfo buffer size for one frame, include cuinfo, cuinfo table and
 *         aq info.
 */
i32 EncAsicGetCuInfoBufferSize(u32 width, u32 height, u32 ctu_size,
    u32 cuInfoVersion, u32 cuinfoAlignment, u32 aqInfoAlignment,
    u32 *cuInfoSize, u32 *cuinfoStride, u32 *cuInfoTableSize,
    u32 *aqInfoSize, u32 *aqInfoStride);
/** Calculat the size of ufbc header and payload buffer.
 *
 * \param [in] inputFormat the picture format defined in EncPixelFormat.
 * \param [in] blockType ufbc superblock type(0 - 32x8 1 - 16x16)
 * \param [in] width picture width in pixels
 * \param [in] height picture height in pixels
 * \param [out] asic_format the pointer will be filled with input format
              mapping API values to SW/HW register values.
 * \param [out] headerSize the pointer will be filled with bytes size for
 *           ufbc header buffer.
 * \param [out] frameSize the pointer will be filled with bytes size for
 *           ufbc payload buffer.
 */
void EncUfbcGetSize(u32 inputFormat, u32 blockType, u32 width, u32 height, u32 *asic_format,
                        u32 *headerSize, u64 *frameSize);
/** @} */



#ifdef __cplusplus
}
#endif

#endif  //__BUFFER_INFO_H__
