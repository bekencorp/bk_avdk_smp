/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2015 VeriSilicon Inc                       --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
------------------------------------------------------------------------------*/
/*!\defgroup codec Common Algorithm Interface
 * This abstraction allows applications to easily support multiple video
 * formats with minimal code duplication. This section describes the interface
 * common to all codecs (both encoders and decoders).
 * @{
 */

/*!\file
 * \brief Describes the codec algorithm interface to applications.
 *
 * This file describes the interface between an application and a
 * video codec algorithm.
 *
 */
#ifndef VCENC_AV1_CODEC_H_
#define VCENC_AV1_CODEC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vcenc_av1_image.h"
#include "vcenc_av1_integer.h"

/*!\brief Decorator indicating a function is deprecated */
#ifndef VCENC_AV1_DEPRECATED
#if defined(__GNUC__) && __GNUC__
#define VCENC_AV1_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define VCENC_AV1_DEPRECATED
#else
#define VCENC_AV1_DEPRECATED
#endif
#endif /* VCENC_AV1_DEPRECATED */

#ifndef VCENC_AV1_DECLSPEC_DEPRECATED
#if defined(__GNUC__) && __GNUC__
#define VCENC_AV1_DECLSPEC_DEPRECATED /**< \copydoc #VCENC_AV1_DEPRECATED */
#elif defined(_MSC_VER)
/*!\brief \copydoc #VCENC_AV1_DEPRECATED */
#define VCENC_AV1_DECLSPEC_DEPRECATED __declspec(deprecated)
#else
#define VCENC_AV1_DECLSPEC_DEPRECATED /**< \copydoc #VCENC_AV1_DEPRECATED */
#endif
#endif /* VCENC_AV1_DECLSPEC_DEPRECATED */

/*!\brief Decorator indicating a function is potentially unused */
#ifdef VCENC_AV1_UNUSED
#elif defined(__GNUC__) || defined(__clang__)
#define VCENC_AV1_UNUSED __attribute__((unused))
#else
#define VCENC_AV1_UNUSED
#endif

/*!\brief Decorator indicating that given struct/union/enum is packed */
#ifndef ATTRIBUTE_PACKED
#if defined(__GNUC__) && __GNUC__
#define ATTRIBUTE_PACKED __attribute__((packed))
#elif defined(_MSC_VER)
#define ATTRIBUTE_PACKED
#else
#define ATTRIBUTE_PACKED
#endif
#endif /* ATTRIBUTE_PACKED */

/*!\brief Bit depth for codec
 * *
 * This enumeration determines the bit depth of the codec.
 */
typedef enum vcenc_av1_bit_depth {
  VCENC_AV1_BITS_8 = 8,   /**<  8 bits */
  VCENC_AV1_BITS_10 = 10, /**< 10 bits */
  VCENC_AV1_BITS_12 = 12, /**< 12 bits */
} vcenc_av1_bit_depth_t;

/*!\brief Superblock size selection.
 *
 * Defines the superblock size used for encoding. The superblock size can
 * either be fixed at 64x64 or 128x128 pixels, or it can be dynamically
 * selected by the encoder for each frame.
 */
typedef enum vcenc_av1_superblock_size {
  VCENC_AV1_SUPERBLOCK_SIZE_64X64,   /**< Always use 64x64 superblocks. */
  VCENC_AV1_SUPERBLOCK_SIZE_128X128, /**< Always use 128x128 superblocks. */
  VCENC_AV1_SUPERBLOCK_SIZE_DYNAMIC  /**< Select superblock size dynamically. */
} vcenc_av1_superblock_size_t;

/*!\brief OBU types. */
typedef enum ATTRIBUTE_PACKED {
  OBU_SEQUENCE_HEADER = 1,
  OBU_TEMPORAL_DELIMITER = 2,
  OBU_FRAME_HEADER = 3,
  OBU_TILE_GROUP = 4,
  OBU_METADATA = 5,
  OBU_FRAME = 6,
  OBU_REDUNDANT_FRAME_HEADER = 7,
  OBU_TILE_LIST = 8,
  OBU_PADDING = 15,
} OBU_TYPE;

/*!\brief OBU metadata types. */
typedef enum {
  OBU_METADATA_TYPE_VCENC_AV1_RESERVED_0 = 0,
  OBU_METADATA_TYPE_HDR_CLL = 1,
  OBU_METADATA_TYPE_HDR_MDCV = 2,
  OBU_METADATA_TYPE_SCALABILITY = 3,
  OBU_METADATA_TYPE_ITUT_T35 = 4,
  OBU_METADATA_TYPE_TIMECODE = 5,
} OBU_METADATA_TYPE;

/*!\brief Returns string representation of OBU_TYPE.
 *
 * \param[in]     type            The OBU_TYPE to convert to string.
 */
const char *vcenc_av1_obu_type_to_string(OBU_TYPE type);

/*!@} - end defgroup codec*/
#ifdef __cplusplus
}
#endif
#endif  // VCENC_AV1_CODEC_H_
