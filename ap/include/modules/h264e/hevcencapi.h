/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : VCX Encoder API
--
------------------------------------------------------------------------------*/

#ifndef API_H
#define API_H

#include "base_type.h"
#include "enccommon.h"
// #include "enc_log.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup api_video
 *
 * @{
 */

#ifdef MULTI_FRAME_SUPPORT
/** \brief The supported maximum frames of aggregation encoding. */
#define MAX_AGGREGATED_FRAMES	20
/** \brief The supported minimum frames of aggregation encoding. */
#define MIN_AGGREGATED_FRAMES	2
#endif

/** \brief The supported maximum number of cores. */
#ifdef MULTI_FRAME_SUPPORT
#define MAX_CORE_NUM MAX_AGGREGATED_FRAMES
#else
#ifndef VCECFG_MAX_CORE_NUM
#define MAX_CORE_NUM 1//4//FIXME@RAMOPT
#else
#define MAX_CORE_NUM VCECFG_MAX_CORE_NUM
#endif
#endif

/** \brief The supported maximum number of reference frames. */
#define VCENC_MAX_REF_FRAMES 2//8//FIXME@RAMOPT
/** \brief The maximum number of RPS descriptions in SPS. */
#define MAX_GOP_PIC_CONFIG_NUM 8//48//FIXME@RAMOPT
/** \brief The supported maximum GOP structure size. */
#define MAX_GOP_SIZE 8//16//FIXME@RAMOPT
/** \brief The maximum GOP structure size for adaptive size decision. */
#define MAX_ADAPTIVE_GOP_SIZE 8
/** @} */
#define MAX_GOP_SPIC_CONFIG_NUM 16
#define VCENC_STREAM_MIN_BUF0_SIZE 1024 * 11
#define LONG_TERM_REF_ID2DELTAPOC(id) ((id) + 10000)
#define LONG_TERM_REF_DELTAPOC2ID(poc) ((poc)-10000)
#define IS_LONG_TERM_REF_DELTAPOC(poc) ((poc) >= 10000)
/**
 * \addtogroup api_video
 *
 * @{
 */
/** \brief The supported maximum number of long-term reference (LTR) frames.
 *  \n Currently, a maximum of two LTR frames are supported. */
#define VCENC_MAX_LT_REF_FRAMES VCENC_MAX_REF_FRAMES
/** @} */
#define MMO_STR2LTR(poc) ((poc) | 0x40000000)
#define VCENC_LOOKAHEAD_MAX 64
#define ROIMAP_PREFETCH_EXT_SIZE 1536

/* define for HEVC*/
#define HEVC_MAX_TILE_COLS (1)
#define HEVC_MAX_TILE_ROWS (1)

/* define for special configure */
#define QPOFFSET_RESERVED -255
#define QPFACTOR_RESERVED -255
#define TEMPORALID_RESERVED -255
#define NUMREFPICS_RESERVED -255
#define FRAME_TYPE_RESERVED -255
#define INVALITED_POC -1

/* define for AV1 */
#define AV1_REF_FRAME_NUM 8
#define AV1_REFS_PER_FRAME 7
#define REGULAR_FRAME 0
/*#define LAST_REF_FRAME     1
#define LAST2_REF_FRAME    2
#define LAST3_REF_FRAME    3
#define GOLDEN_FRAME        4
#define BWDREF_FRAME        5
#define ALTREF2_FRAME       6
#define ALTREF_FRAME        7
#define SW_NONE_FRAME    (-1)
#define SW_INTRA_FRAME   0
#define SW_LAST_FRAME    1
#define SW_LAST2_FRAME   2
#define SW_LAST3_FRAME   3
#define SW_GOLDEN_FRAME  4
#define SW_BWDREF_FRAME  5
#define SW_ALTREF2_FRAME 6
#define SW_ALTREF_FRAME  7
*/

#define AQ_STRENGTH_SCALE_BITS 7
#define PSY_FACTOR_SCALE_BITS 8

/* define for video codec detection */
#define IS_HEVC(a) (a == VCENC_VIDEO_CODEC_HEVC)
#define IS_H264(a) (a == VCENC_VIDEO_CODEC_H264)
#define IS_AV1(a) (a == VCENC_VIDEO_CODEC_AV1)
#define IS_VP9(a) (a == VCENC_VIDEO_CODEC_VP9)

/**
 * \defgroup api_video Basic Encoder API
 *
 * @{
 */

/** \brief The maximum GOP structure size available. */
#define MAX_GOP_SIZE_INUSE(gopSize) \
  (gopSize <= MAX_ADAPTIVE_GOP_SIZE ? MAX_ADAPTIVE_GOP_SIZE : MAX_GOP_SIZE)

/* define for cutree buffer count */
/** \brief The number of look-ahead frames for CuTree use.
 *  \n CuTree requires at most (<tt> \ref MAX_GOP_SIZE</tt> - 1) frames for completing a GOP.
 *  \n For any GOP structure size, a maximum of (depth + <tt> \ref MAX_GOP_SIZE</tt> / 2) frames are
 *  queued in CuTree.
 *  \n If the current GOP is still incomplete, CuTree works on frames excluding the current GOP. */
#define CUTREE_MAX_LOOKAHEAD_FRAMES(depth, gopSize) \
  ((depth) + MAX_GOP_SIZE_INUSE(gopSize) / 2)

/** \brief The number of CuTree buffers.
 *  \n The extra buffer is used for the first frame in the CuTree queue, which is already processed
 *  or output. */
#define CUTREE_BUFFER_CNT(depth, gopSize) \
  ((depth) + MAX_GOP_SIZE_INUSE(gopSize) / 2 + 1)

/*------------------------------------------------------------------------------
      1. Type definition for encoder instance
  ------------------------------------------------------------------------------*/
/** \brief The encoder instance. */
typedef const void *VCEncInst;

/*------------------------------------------------------------------------------
      2. Enumerations for API parameters
  ------------------------------------------------------------------------------*/

/** Specifies function return values. */
typedef enum {
  /** The API call is successful. */
  VCENC_OK = 0,
  /** Encoding of a frame is finished. */
  VCENC_FRAME_READY = (1<<0),
  /** A frame is inside the encoder's internal queue but not encoded. It will be encoded and output
   *  in a subsequent encoder API call. */
  VCENC_FRAME_ENQUEUE = (1<<1),
  /** A encoding frame task is cached and will drive the hardware in batch after accumulate some tasks. */
  VCENC_FRAME_CACHE = (1<<2),
  /** A sideline frame is encoded and application should continue to execute
   * job in the working queue. */
  VCENC_FRAME_CONTINUE = (1<<3),
  /** (Error) An encoder error occurs. */
  VCENC_ERROR = -1,
  /** (Error) A pointer argument has an invalid NULL value. */
  VCENC_NULL_ARGUMENT = -2,
  /** (Error) One of the arguments is invalid. None of the argument settings takes effect. */
  VCENC_INVALID_ARGUMENT = -3,
  /** (Error) The encoder fails to allocate memory. */
  VCENC_MEMORY_ERROR = -4,
  /** (Error) Initialization of the encoder system interface fails. */
  VCENC_EWL_ERROR = -5,
  /** (Error) The EWL fails to allocate memory. */
  VCENC_EWL_MEMORY_ERROR = -6,
  /** (Error) The stream is started with HRD enabled and rate control parameters fails to be
   *  altered. */
  VCENC_INVALID_STATUS = -7,
  /** (Error) The output buffer is too small to hold the generated stream. Please allocate a
   *  larger buffer and try again. */
  VCENC_OUTPUT_BUFFER_OVERFLOW = -8,
  /** (Error) Memory access fails due to invalid bus address. Please reset the hardware. */
  VCENC_HW_BUS_ERROR = -9,
  /** (Error) An error occurs in the hardware data. */
  VCENC_HW_DATA_ERROR = -10,
  /** (Error) Hardware execution has timed out. The current frame is lost. Please do some clean-up
   *  and then encode a new frame. */
  VCENC_HW_TIMEOUT = -11,
  /** (Error) The hardware fails to be reserved for exclusive access. */
  VCENC_HW_RESERVED = -12,
  /** (Error) A fatal system error occurs and the encoding is terminated. Please release the
   *  encoder instance.*/
  VCENC_SYSTEM_ERROR = -13,
  /** (Error) The encoder instance is invalid or corrupted. */
  VCENC_INSTANCE_ERROR = -14,
  /** (Error) An HRD error occurs. */
  VCENC_HRD_ERROR = -15,
  /** The hardware is reset by an external operation, for example, resetting the encoder core to
   *  clean up the error status when an error occurs. */
  VCENC_HW_RESET = -16,
  /** (Error) During low-latency encoding in DDR mode, excessive cycles have been used to poll for
   *  an input row to be encoded but no valid data is found in input picture buffers. For details,
   *  see Section <em> \ref appex_sss63</em>. */
  VCENC_HW_POLL_SLICEINFO_TIMEOUT = -17,
  /** (Error) An UFBC decoding error occurs when the input picture is fetched. */
  VCENC_HW_UFBC_ERROR = -18
} VCEncRet;

/** Specifies video codec standards. */
typedef enum {
  /** Codec standard HEVC (H.265). */
  VCENC_VIDEO_CODEC_HEVC = 0,
  /** Codec standard H.264 (AVC). */
  VCENC_VIDEO_CODEC_H264 = 1,
  /** Codec standard AV1. */
  VCENC_VIDEO_CODEC_AV1 = 2,
  /** Codec standard VP9. */
  VCENC_VIDEO_CODEC_VP9 = 3
} VCEncVideoCodecFormat;

/** Specifies stream types for HEVC (H.265) or H.264 (AVC) encoding. */
typedef enum {
  /** Byte stream, where each NAL unit starts with hexadecimal bytes \n '00 00 00 01'. */
  VCENC_BYTE_STREAM = 0,
  /** NAL unit stream, which consists of plain NAL units without a start code. */
  VCENC_NAL_UNIT_STREAM = 1
} VCEncStreamType;

/** Specifies stream levels for initialization. */
/**
 * <table>
 * <caption id='tab_vencLevel'>VCEnc Level Enumeration Values Defined for HEVC (H.265)</caption>
 * <tr>
 *   <th align="center"> Value
 *   <th align="center"> Encoded Picture Size
 *   <th align="center"> Frame Rate (FPS)
 *   <th align="center"> Max Luma Picture Size (Samples)
 *   <th align="center"> Max Luma Sample Rate (Samples/Sec)
 *   <th align="center"> Max Bit Rate (kbps)
 * <tr>
 *   <td align="center"> VCENC_HEVC\n _LEVEL_1
 *   <td align="center"> QCIF
 *   <td align="center"> 15
 *   <td align="center"> 36,864
 *   <td align="center"> 552,960
 *   <td align="center"> 128
 * <tr>
 *   <td align="center"> VCENC_HEVC\n _LEVEL_2
 *   <td align="center"> CIF
 *   <td align="center"> 30
 *   <td align="center"> 122,880
 *   <td align="center"> 3,686,400
 *   <td align="center"> 1,500
 * <tr>
 *   <td align="center" > VCENC_HEVC\n _LEVEL_2_1
 *   <td align="center"> Q720p
 *   <td align="center"> 30
 *   <td align="center"> 245,760
 *   <td align="center"> 7,372,800
 *   <td align="center"> 3,000
 * <tr>
 *   <td align="center"> VCENC_HEVC\n _LEVEL_3
 *   <td align="center"> QHD
 *   <td align="center"> 30
 *   <td align="center"> 552,960
 *   <td align="center"> 16,588,800
 *   <td align="center"> 6,000
 * <tr>
 *   <td align="center"> VCENC_HEVC\n _LEVEL_3_1
 *   <td align="center"> 1280x720
 *   <td align="center"> 30
 *   <td align="center"> 983,040
 *   <td align="center"> 33,177,600
 *   <td align="center"> 10,000
 * <tr>
 *   <td align="center"> VCENC_HEVC\n _LEVEL_4
 *   <td align="center"> 2Kx1080
 *   <td align="center"> 30
 *   <td align="center"> 2,228,224
 *   <td align="center"> 66,846,720
 *   <td align="center"> 12,000
 * <tr>
 *   <td align="center" > VCENC_HEVC\n _LEVEL_4_1
 *   <td align="center"> 2Kx1080
 *   <td align="center"> 60
 *   <td align="center"> 2,228,224
 *   <td align="center"> 133,693,440
 *   <td align="center"> 20,000
 * <tr>
 *   <td align="center" > VCENC_HEVC\n _LEVEL_5
 *   <td align="center"> 4096x2160
 *   <td align="center"> 30
 *   <td align="center"> 8,912,896
 *   <td align="center"> 267,386,880
 *   <td align="center"> 25,000
 * <tr>
 *   <td align="center" > VCENC_HEVC\n _LEVEL_5_1
 *   <td align="center"> 4096x2160
 *   <td align="center"> 60
 *   <td align="center"> 8,912,896
 *   <td align="center"> 534,773,760
 *   <td align="center"> 40,000
 * <tr>
 *   <td align="center" > VCENC_HEVC\n _LEVEL_5_2
 *   <td align="center"> 4096x2160
 *   <td align="center"> 120
 *   <td align="center"> 8,912,896
 *   <td align="center"> 1,069,547,520
 *   <td align="center"> 60,000
 * <tr>
 *   <td align="center"> VCENC_HEVC\n _LEVEL_6
 *   <td align="center"> 8192x4320
 *   <td align="center"> 30
 *   <td align="center"> 35,651,584
 *   <td align="center"> 1,069,547,520
 *   <td align="center"> 60,000
 * <tr>
 *   <td align="center"> VCENC_HEVC\n _LEVEL_6_1
 *   <td align="center"> 8192x4320
 *   <td align="center"> 60
 *   <td align="center"> 35,651,584
 *   <td align="center"> 2,139,095,040
 *   <td align="center"> 120,000
 * <tr>
 *   <td align="center" > VCENC_HEVC\n _LEVEL_6_2
 *   <td align="center"> 8192x4320
 *   <td align="center"> 120
 *   <td align="center"> 35,651,584
 *   <td align="center"> 4,278,190,080
 *   <td align="center"> 240,000
 * </table>
 *
 * <table>
 * <caption id='tab_vencLevel1'>VCEnc Level Enumeration Values Defined for H.264 (AVC)</caption>
 * <tr>
 *   <th align="center"> Value
 *   <th align="center"> Encoded Picture Size
 *   <th align="center"> Frame Rate (FPS)
 *   <th align="center"> Max Luma Picture Size (Samples)
 *   <th align="center"> Max Luma Sample Rate (Samples/Sec)
 *   <th align="center"> Max Bit Rate (kbps)
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_1
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> Sub-QCIF
 *       <tr><td align="center"> QCIF
 *     </table>
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> 15
 *       <tr><td align="center"> 15
 *     </table>
 *   <td align="center"> 99 (QCIF)
 *   <td align="center"> 1,485
 *   <td align="center"> 64
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_1_b
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> Sub-QCIF
 *       <tr><td align="center"> QCIF
 *     </table>
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> 15
 *       <tr><td align="center"> 15
 *     </table>
 *   <td align="center"> 99 (QCIF)
 *   <td align="center"> 1,485
 *   <td align="center"> 128
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_1_1
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> QCIF
 *       <tr><td align="center"> QVGA
 *     </table>
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> 30
 *       <tr><td align="center"> 10
 *     </table>
 *   <td align="center"> 396 (QCIF)
 *   <td align="center"> 3,000
 *   <td align="center"> 192
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_1_2
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> QVGA
 *       <tr><td align="center"> CIF
 *     </table>
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> 20
 *       <tr><td align="center"> 15
 *     </table>
 *   <td align="center"> 396 (QCIF)
 *   <td align="center"> 6,000
 *   <td align="center"> 384
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_1_3
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> QVGA
 *       <tr><td align="center"> CIF
 *     </table>
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> 30
 *       <tr><td align="center"> 30
 *     </table>
 *   <td align="center"> 396 (QCIF)
 *   <td align="center"> 11,880
 *   <td align="center"> 768
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_2
 *   <td align="center"> CIF
 *   <td align="center"> 30
 *   <td align="center"> 396 (QCIF)
 *   <td align="center"> 11,880
 *   <td align="center"> 2,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_2_1
 *   <td align="center"> 512x384
 *   <td align="center"> 25
 *   <td align="center"> 792
 *   <td align="center"> 19,800
 *   <td align="center"> 4,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_2_2
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> VGA
 *       <tr><td align="center"> 720x480
 *     </table>
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> 15
 *       <tr><td align="center"> 15
 *     </table>
 *   <td align="center"> 1,620 (PAL)
 *   <td align="center"> 20,250
 *   <td align="center"> 4,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_3
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> VGA
 *       <tr><td align="center"> 720x576
 *       <tr><td align="center"> 720x480
 *     </table>
 *   <td align="center">
 *     <table>
 *       <tr><td align="center"> 30
 *       <tr><td align="center"> 25
 *       <tr><td align="center"> 30
 *     </table>
 *   <td align="center"> 1,620 (PAL)
 *   <td align="center"> 40,500
 *   <td align="center"> 10,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_3_1
 *   <td align="center"> 1280x720
 *   <td align="center"> 30
 *   <td align="center"> 3,600\n (HD 720p)
 *   <td align="center"> 108,000
 *   <td align="center"> 14,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_3_2
 *   <td align="center"> 1280x720
 *   <td align="center"> 60
 *   <td align="center"> 5,120 (SXGA)
 *   <td align="center"> 216,000
 *   <td align="center"> 20,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_4
 *   <td align="center"> 2048x1024
 *   <td align="center"> 30
 *   <td align="center"> 8,192 (1080p)
 *   <td align="center"> 245,760
 *   <td align="center"> 20,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_4_1
 *   <td align="center"> 2048x1024
 *	  <td align="center"> 30
 *   <td align="center"> 8,192 (1080p)
 *   <td align="center"> 245,760
 *   <td align="center"> 50,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_4_2
 *   <td align="center"> 2048x1024
 *   <td align="center"> 60
 *   <td align="center"> 8,192 (1080p)
 *   <td align="center"> 49,1520
 *   <td align="center"> 50,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_5
 *   <td align="center"> 3680x1536
 *   <td align="center"> 25
 *   <td align="center"> 22,080
 *   <td align="center"> 58,9824
 *   <td align="center"> 135,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_5_1
 *   <td align="center"> 4096x2048
 *   <td align="center"> 30
 *   <td align="center"> 36,864
 *   <td align="center"> 983,040
 *   <td align="center"> 240,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_5_2
 *   <td align="center"> 4096x2160
 *   <td align="center"> 60
 *   <td align="center"> 36,864
 *   <td align="center"> 2,073,600
 *   <td align="center"> 240,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_6
 *   <td align="center"> 8192x4320
 *   <td align="center"> 30
 *   <td align="center"> 35,651,584
 *   <td align="center"> 4,177,920
 *   <td align="center"> 240,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_6_1
 *   <td align="center"> 8192x4320
 *   <td align="center"> 60
 *   <td align="center"> 35,651,584
 *   <td align="center"> 8,355,840
 *   <td align="center"> 480,000
 * <tr>
 *   <td align="center"> VCENC_H264\n _LEVEL_6_2
 *   <td align="center"> 8192x4320
 *   <td align="center"> 120
 *   <td align="center"> 35,651,584
 *   <td align="center"> 16,711,680
 *   <td align="center"> 800,000
 * </table>
 */
typedef enum {
  VCENC_HEVC_LEVEL_1 = 30,
  VCENC_HEVC_LEVEL_2 = 60,
  VCENC_HEVC_LEVEL_2_1 = 63,
  VCENC_HEVC_LEVEL_3 = 90,
  VCENC_HEVC_LEVEL_3_1 = 93,
  VCENC_HEVC_LEVEL_4 = 120,
  VCENC_HEVC_LEVEL_4_1 = 123,
  VCENC_HEVC_LEVEL_5 = 150,
  VCENC_HEVC_LEVEL_5_1 = 153,
  VCENC_HEVC_LEVEL_5_2 = 156,
  VCENC_HEVC_LEVEL_6 = 180,
  VCENC_HEVC_LEVEL_6_1 = 183,
  VCENC_HEVC_LEVEL_6_2 = 186,

  /* H.264 Definition*/
  VCENC_H264_LEVEL_1 = 10,
  VCENC_H264_LEVEL_1_b = 99,
  VCENC_H264_LEVEL_1_1 = 11,
  VCENC_H264_LEVEL_1_2 = 12,
  VCENC_H264_LEVEL_1_3 = 13,
  VCENC_H264_LEVEL_2 = 20,
  VCENC_H264_LEVEL_2_1 = 21,
  VCENC_H264_LEVEL_2_2 = 22,
  VCENC_H264_LEVEL_3 = 30,
  VCENC_H264_LEVEL_3_1 = 31,
  VCENC_H264_LEVEL_3_2 = 32,
  VCENC_H264_LEVEL_4 = 40,
  VCENC_H264_LEVEL_4_1 = 41,
  VCENC_H264_LEVEL_4_2 = 42,
  VCENC_H264_LEVEL_5 = 50,
  VCENC_H264_LEVEL_5_1 = 51,
  VCENC_H264_LEVEL_5_2 = 52,
  VCENC_H264_LEVEL_6 = 60,
  VCENC_H264_LEVEL_6_1 = 61,
  VCENC_H264_LEVEL_6_2 = 62,

  VCENC_AUTO_LEVEL = 0xFFFF
} VCEncLevel;

/** Specifies stream profiles for initialization.
 *
 * For the profile support of the video encoder, see <em>Hantro VC9000E Series Video Encoder
 * Hardware Features</em>. */
typedef enum {
  /** Set profile according to codec and feature selected for encoding */
  VCENC_ADAPTIVE_PROFILE = -1,
  /** The HEVC (H.265) profile for a bit depth of 8 bits per sample with YUV420 sampling. */
  VCENC_HEVC_MAIN_PROFILE = 0,
  /** The HEVC profile for a single still picture to be encoded with the same constraints as the
   *  Main profile. */
  VCENC_HEVC_MAIN_STILL_PICTURE_PROFILE = 1,
  /** The HEVC profile for a bit depth of 8 to 10 bits per sample with YUV420 sampling. */
  VCENC_HEVC_MAIN_10_PROFILE = 2,
  /** The HEVC Monochrome profile for bit depth of 8 bits per sample with YUV400 sampling */
  VCENC_HEVC_MONOCHROME_PROFILE = 3,
  /** The HEVC Monochrome profile for bit depth of 10 bits per sample with YUV400 sampling. */
  VCENC_HEVC_MONOCHROME_10_PROFILE = 4,
  /** The HEVC profile for a single still picture to be encoded with the same constraints as the
   *  Main 10 profile. */
  VCENC_HEVC_MAIN_10_STILL_PICTURE_PROFILE = 5,
  /** The HEVC Monochrome profile for bit depth of 10 bits per sample with YUV422 sampling. */
  VCENC_HEVC_MAIN_422_10_PROFILE = 6,
  VCENC_HEVC_MAIN_444_10_PROFILE = 7,
  VCENC_HEVC_SCC_MAIN_PROFILE = 8,
  VCENC_HEVC_SCC_444_10_PROFILE = 9,
  VCENC_HEVC_PROFILE_NUM,

  /* H264 Definition*/
  /** The H.264 (AVC) Baseline profile for video conferencing and mobile applications. */
  VCENC_H264_BASE_PROFILE = 9,
  /** The H.264 profile for standard-definition digital TV broadcasts that use the MPEG-4 format.*/
  VCENC_H264_MAIN_PROFILE = 10,
  /** The H.264 primary profile for broadcast and disk storage applications, particularly for
   *  high-definition TV applications. */
  VCENC_H264_HIGH_PROFILE = 11,
  /** The H.264 profile for up to 10 bits per sample, which is built on top of the High profile. */
  VCENC_H264_HIGH_10_PROFILE = 12,
  VCENC_H264_PROFILE_NUM,

  /* AV1 Definition*/
  /** The AV1 Main profile for a bit depth of 8 or 10 bits per sample with YUV420 sampling. */
  VCENC_AV1_MAIN_PROFILE = 0,
  /** The AV1 High profile for a bit depth of 8 or 10 bits per sample with YUV420 sampling.
   * NOT SUPPORT. */
  VCENC_AV1_HIGH_PROFILE = 1,
  /** The AV1 Professional profile for a bit depth of 8 or 10 bits per sample with YUV420
   *  sampling. NOT SUPPORT. */
  VCENC_AV1_PROFESSIONAL_PROFILE = 2,
  VCENC_AV1_PROFILE_NUM,

  /*Vp9 Definition*/
  /** The VP9 Main profile for a bit depth of 8 bits per sample with YUV420 non-sRGB sampling. */
  VCENC_VP9_0_PROFILE = 0,
  /** The VP9 Main profile for a bit depth of 8 bits per sample with YUV422, YUV440, or YUV444
   *  sRGB sampling. NOT SUPPORT. */
  VCENC_VP9_1_PROFILE = 1,
  /** The VP9 Main profile for a bit depth of 10 or 12 bits per sample with YUV420 non-sRGB
   *  sampling. */
  VCENC_VP9_2_PROFILE = 2,
  /** The VP9 Main profile for a bit depth of 10 bits per sample with YUV422, YUV440, or YUV444
   *  sRGB sampling. NOT SUPPORT. */
  VCENC_VP9_3_PROFILE = 3,
  VCENC_VP9_PROFILE_NUM,
} VCEncProfile;

/** Specifies stream tiers for initialization. */
typedef enum {
  /** The Main tier. */
  VCENC_HEVC_MAIN_TIER = 0,
  /** The High tier. */
  VCENC_HEVC_HIGH_TIER = 1,
} VCEncTier;

/** Specifies input color formats for initialization.
 *
 * For details about color formats, see <em>Hantro VC9000E Series Memory Buffer and Format
 * Organization</em>. */
typedef enum {
  /** 0 - YUV420P8b_Raster_A16N. */
  VCENC_YUV420_PLANAR = ENC_PIXFMT_YUV420_PLANAR,
  /** 1 - YUV420SP8b_Raster_A16N. */
  VCENC_YUV420_SEMIPLANAR = ENC_PIXFMT_YUV420_SEMIPLANAR,
  /** 2 - YVU420SP8b_Raster_A16N. */
  VCENC_YUV420_SEMIPLANAR_VU = ENC_PIXFMT_YUV420_SEMIPLANAR_VU,
  /** 3 - YUV422YUYV8b_Raster_A16N. */
  VCENC_YUV422_INTERLEAVED_YUYV = ENC_PIXFMT_YUV422_INTERLEAVED_YUYV,
  /** 4 - YUV422UYVY8b_Raster_A16N. */
  VCENC_YUV422_INTERLEAVED_UYVY = ENC_PIXFMT_YUV422_INTERLEAVED_UYVY,
  /** 5 - RGB565_Raster_A16N. */
  VCENC_RGB565 = ENC_PIXFMT_RGB565,
  /** 6 - BGR565_Raster_A16N. */
  VCENC_BGR565 = ENC_PIXFMT_BGR565,
  /** 7 - XRGB1555_Raster_A16N. */
  VCENC_RGB555 = ENC_PIXFMT_RGB555,
  /** 8 - XBGR1555_Raster_A16N. */
  VCENC_BGR555 = ENC_PIXFMT_BGR555,
  /** 9 - XRGB4444_Raster_A16N. */
  VCENC_RGB444 = ENC_PIXFMT_RGB444,
  /** 10 - XBGR4444_Raster_A16N. */
  VCENC_BGR444 = ENC_PIXFMT_BGR444,
  /** 11 - XRGB8888_Raster_A16N. */
  VCENC_RGB888 = ENC_PIXFMT_RGB888,
  /** 12 - XBGR8888_Raster_A16N. */
  VCENC_BGR888 = ENC_PIXFMT_BGR888,
  /** 13 - XRGB2101010_Raster_A16N. */
  VCENC_RGB101010 = ENC_PIXFMT_RGB101010,
  /** 14 - XBGR2101010_Raster_A16N. */
  VCENC_BGR101010 = ENC_PIXFMT_BGR101010,
  /** 15 - YUV420P10bWL_Raster_A16N. */
  VCENC_YUV420_PLANAR_10BIT_I010 = ENC_PIXFMT_YUV420_PLANAR_10BIT_I010,
  /** 16 - YUV420SP10bWH_Raster_A16N. */
  VCENC_YUV420_PLANAR_10BIT_P010 = ENC_PIXFMT_YUV420_PLANAR_10BIT_P010,
  /** 17 - YUV420P10b_Raster_A16N. */
  VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR =
      ENC_PIXFMT_YUV420_PLANAR_10BIT_PACKED_PLANAR,
  /** 18 - YUV420Y0L210b_Raster_A16N. */
  VCENC_YUV420_10BIT_PACKED_Y0L2 = ENC_PIXFMT_YUV420_10BIT_PACKED_Y0L2,
  /** 19 - (Reserved) YUV420P8b_Tile32x32_A16N for HEVC. */
  VCENC_YUV420_PLANAR_8BIT_TILE_32_32 =
      ENC_PIXFMT_YUV420_PLANAR_8BIT_TILE_32_32,
  /** 20 - (Reserved) YUV420P8b_Tile16x16_A16N for H.264 (AVC). */
  VCENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4 =
      ENC_PIXFMT_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4,
  /** 21 - YUV420SP8b_YuvSp4x4_A16N. */
  VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4 =
      ENC_PIXFMT_YUV420_SEMIPLANAR_8BIT_TILE_4_4,
  /** 22 - YVU420SP8b_YuvSp4x4_A16N. */
  VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4 =
      ENC_PIXFMT_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4,
  /** 23 - YUV420SP10bWH_YuvSp4x4_A16N. */
  VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4 =
      ENC_PIXFMT_YUV420_PLANAR_10BIT_P010_TILE_4_4,
  /** 24 - YUV420SP10bDWL_Raster_A16N. */
  VCENC_YUV420_SEMIPLANAR_101010 = ENC_PIXFMT_YUV420_SEMIPLANAR_101010,
  /** 25 - YUV422SP8b_Raster_A16N */
  VCENC_YUV422_SEMIPLANAR_888 = ENC_PIXFMT_YUV422_888,
  /** 26 - (Reserved) YVU420SP8b_Tile64x4_A16N. */
  VCENC_YUV420_8BIT_TILE_64_4 = ENC_PIXFMT_YUV420_8BIT_TILE_64_4,
  /** 27 - (Reserved) YUV420SP8b_Tile64x4_A16N. */
  VCENC_YUV420_UV_8BIT_TILE_64_4 = ENC_PIXFMT_YUV420_UV_8BIT_TILE_64_4,
  /** 28 - (Reserved) YUV420SP10bWH_Tile32x4_A16N. */
  VCENC_YUV420_10BIT_TILE_32_4 = ENC_PIXFMT_YUV420_10BIT_TILE_32_4,
  /** 29 - (Reserved) YUV420SP10bWH_Tile48x4_A16N. */
  VCENC_YUV420_10BIT_TILE_48_4 = ENC_PIXFMT_YUV420_10BIT_TILE_48_4,
  /** 30 - (Reserved) YVU420SP10bWH_Tile48x4_A16N. */
  VCENC_YUV420_VU_10BIT_TILE_48_4 = ENC_PIXFMT_YUV420_VU_10BIT_TILE_48_4,
  /** 31 - (Reserved) YVU420SP8b_Tile128x2_A16N. */
  VCENC_YUV420_8BIT_TILE_128_2 = ENC_PIXFMT_YUV420_8BIT_TILE_128_2,
  /** 32 - (Reserved) YUV420SP8b_Tile128x2_A16N. */
  VCENC_YUV420_UV_8BIT_TILE_128_2 = ENC_PIXFMT_YUV420_UV_8BIT_TILE_128_2,
  /** 33 - (Reserved) YUV420SP10bWH_Tile96x2_A16N. */
  VCENC_YUV420_10BIT_TILE_96_2 = ENC_PIXFMT_YUV420_10BIT_TILE_96_2,
  /** 34 - (Reserved) YVU420SP10bWH_Tile96x2_A16N. */
  VCENC_YUV420_VU_10BIT_TILE_96_2 = ENC_PIXFMT_YUV420_VU_10BIT_TILE_96_2,
  /** 35 - YUV420SP8b_YuvSp8x8_A16N. */
  VCENC_YUV420_8BIT_TILE_8_8 = ENC_PIXFMT_YUV420_8BIT_TILE_8_8,
  /** 36 - YUV420SP10b_YuvSp8x8_A16N. */
  VCENC_YUV420_10BIT_TILE_8_8 = ENC_PIXFMT_YUV420_10BIT_TILE_8_8,
  /** 37 - YVU420P8b_Raster_A16N. */
  VCENC_YVU420_PLANAR = ENC_PIXFMT_YVU420_PLANAR,
  /** 38 - (Reserved) YUV420SP8b_Tile64x2_A16N. */
  VCENC_YUV420_UV_8BIT_TILE_64_2 = ENC_PIXFMT_YUV420_UV_8BIT_TILE_64_2,
  /** 39 - (Reserved) YUV420SP10bWH_Tile128x2_A16N. */
  VCENC_YUV420_UV_10BIT_TILE_128_2 = ENC_PIXFMT_YUV420_UV_10BIT_TILE_128_2,
  /** 40 - RGB888_Raster_A16N. */
  VCENC_RGB888_24BIT = ENC_PIXFMT_RGB888_24BIT,
  /** 41 - BGR888_Raster_A16N. */
  VCENC_BGR888_24BIT = ENC_PIXFMT_BGR888_24BIT,
  /** 42 - RBG888_Raster_A16N. */
  VCENC_RBG888_24BIT = ENC_PIXFMT_RBG888_24BIT,
  /** 43 - GBR888_Raster_A16N. */
  VCENC_GBR888_24BIT = ENC_PIXFMT_GBR888_24BIT,
  /** 44 - BRG888_Raster_A16N. */
  VCENC_BRG888_24BIT = ENC_PIXFMT_BRG888_24BIT,
  /** 45 - GRB888_Raster_A16N. */
  VCENC_GRB888_24BIT = ENC_PIXFMT_GRB888_24BIT,
  /** 46 - YYYY... VUVU... */
  VCENC_YVU422_SEMIPLANAR_888 = ENC_PIXFMT_YVU422SP_888,
  /** YYYY... UUUU... VVVV...  */
  VCENC_YUV444_PLANAR = ENC_PIXFMT_YUV444_PLANAR, // 444 formats not merged
  /** X8Y8U8V8...*/
  VCENC_YUV444_XYUV8888 = ENC_PIXFMT_YUV444_XYUV8888,
  /** X2Y10U10V10...*/
  VCENC_YUV444_XYUV2101010 = ENC_PIXFMT_YUV444_XYUV2101010,
  /** 50 - RGBX8888_Raster_A16N */
  VCENC_RGBX8888 = ENC_PIXFMT_RGBX8888,
  /** 51 - BGRX8888_Raster_A16N */
  VCENC_BGRX8888 = ENC_PIXFMT_BGRX8888,
  /** 52 - RGBX1010102_Raster_A16N */
  VCENC_RGBX1010102 = ENC_PIXFMT_RGBX1010102,
  /** 53 - BGRX1010102_Raster_A16N */
  VCENC_BGRX1010102 = ENC_PIXFMT_BGRX1010102,
  /** 54 - YVU420SP10bWH_Raster_A16N  */
  VCENC_YVU420_PLANAR_10BIT_P010 = ENC_PIXFMT_YVU420_PLANAR_10BIT_P010,
  /** 55 - Y8b_Raster_A16N (YCbCr 4:0:0 ) Monochrome 8-bit */
  VCENC_Y8b = ENC_PIXFMT_Y8b,
  /** 56 - Y10bWL_Raster_A16N (YCbCr 4:0:0 ) Monochrome 10-bit occupies 2B [9:0] */
  VCENC_Y10bWL = ENC_PIXFMT_Y10bWL,
  /** 57 - Y10bWH_Raster_A16N (YCbCr 4:0:0 ) Monochrome 10-bit occupies 2B [15:6] */
  VCENC_Y10bWH = ENC_PIXFMT_Y10bWH,
  /** 58 - YUV420SP10b_Raster_A16N */
  VCENC_YUV420SP10b = ENC_PIXFMT_YUV420SP10b,
  /** 59 - YUV422P10bWL_Raster_A16N. */
  VCENC_YUV422P10bWL = ENC_PIXFMT_YUV422P10bWL,
  VCENC_FORMAT_MAX = ENC_PIXFMT_FORMAT_MAX

} VCEncPictureType;

/** Specifies picture scan types for pre-processing.
 *
 * This enumeration is valid only if the value of <tt>EWLHwConfig_t.superTileXSupport</tt>
 *  is <tt>0</tt>. */
typedef enum {
  /** raster scan */
  VCENC_RASTER_SCAN = 0,
  /** supertileX scan */
  VCENC_SUPERTILEX_SCAN = 1
} VCEncPictureScanType;

/** Specifies picture rotation types for pre-processing.
 *
 * This enumeration is valid only if the value of <tt>EWLHwConfig_t.NonRotationSupport</tt>
 *  is <tt>0</tt>. */
typedef enum {
  /** Does not rotate. */
  VCENC_ROTATE_0 = 0,
  /** Rotates 90 degrees clockwise. */
  VCENC_ROTATE_90R = 1,
  /** Rotates 90 degrees counterclockwise. */
  VCENC_ROTATE_90L = 2,
  /** Rotates 180 degrees clockwise. */
  VCENC_ROTATE_180R = 3
} VCEncPictureRotation;

/** Specifies picture mirroring modes for pre-processing.
 *
 * This enumeration is valid only if the value of <tt>EWLHwConfig_t.prpMirrorSupport</tt>
 *  is <tt>1</tt>. */
typedef enum {
  /** Does not mirror the pictures. */
  VCENC_MIRROR_NO = 0,
  /** Mirrors the pictures. */
  VCENC_MIRROR_YES = 1
} VCEncPictureMirror;

/** Specifies RGB-to-YUV conversion types for pre-processing. */
typedef enum {
  /** Conversion to YUV values without any range change according to Rec. ITU-R BT.601. */
  VCENC_RGBTOYUV_BT601 = 0,
  /** Conversion to YUV values without any range change according to Rec. ITU-R BT.709. */
  VCENC_RGBTOYUV_BT709 = 1,
  /** Conversion using custom coefficients. For the conversion equations, see Section
   *  <em> @ref s_csc</em>. */
  VCENC_RGBTOYUV_USER_DEFINED = 2,
  /** Conversion according to Rec. ITU-R BT.2020. */
  VCENC_RGBTOYUV_BT2020 = 3,
  /** Conversion from full range RGB to limited range YUV according to Rec. ITU-R BT.601. */
  VCENC_RGBFULL_TO_YUVLIMIT_BT601 = 4,
  /** (obsolete) Same as VCENC_RGBFULL_TO_YUVLIMIT_BT601. */
  VCENC_RGBTOYUV_BT601_FULL_RANGE = VCENC_RGBFULL_TO_YUVLIMIT_BT601,
  /** (Just for test) Conversion from RGB in range of (0~219) to limited range YUV
   * according to Rec. ITU-R BT.601. */
  VCENC_RGBTOYUV_BT601_LIMITED_RANGE = 5,
  /** Conversion from full range RGB to limited range YUV according to Rec. ITU-R BT.709. */
  VCENC_RGBFULL_TO_YUVLIMIT_BT709 = 6,
  /** (obsolete) Same as VCENC_RGBFULL_TO_YUVLIMIT_BT709. */
  VCENC_RGBTOYUV_BT709_FULL_RANGE = VCENC_RGBFULL_TO_YUVLIMIT_BT709,
  /** number of supported color conversion type */
  VCENC_RGBTOYUV_MAX
} VCEncColorConversionType;

/** Specifies picture coding types for encoding. */
typedef enum {
  /** Intra-codes the picture. */
  VCENC_INTRA_FRAME = 0,
  /** Inter-codes the picture by using a previous picture as a predictor. */
  VCENC_PREDICTED_FRAME = 1,
  /** Inter-codes the picture by using previous and/or subsequent pictures as predictors. */
  VCENC_BIDIR_PREDICTED_FRAME = 2,
  /** Does not code the picture. */
  VCENC_NOTCODED_FRAME
} VCEncPictureCodingType;

/** Specifies frame types for AV1 encoding. */
typedef enum {
  /** Key frame. */
  VCENC_AV1_KEY_FRAME = 0,
  /** Inter frame. */
  VCENC_AV1_INTER_FRAME = 1,
  /** Intra-only frame. */
  VCENC_AV1_INTRA_ONLY_FRAME = 2,  // replaces intra-only
  /** S-frame. */
  VCENC_AV1_S_FRAME = 3,
  VCENC_AV1_FRAME_TYPES,
} VCENC_AV1_FRAME_TYPE;

/** Specifies HDR10 transfer function types. */
typedef enum {
  /** The transfer functions defined in ITU-R-REC-BT.2020. */
  VCENC_HDR10_BT2020 = 14,
  /** The transfer functions defined in SMPTE ST 2084. */
  VCENC_HDR10_ST2084 = 16,
  /** The transfer functions defined in ARIB STD-B67. */
  VCENC_HDR10_STDB67 = 18,
} VCEncHDRTransferType;

/** Specifies chroma sampling modes relative to luma sampling. For details, see Section
 *  <em> \ref s_fmt</em>.
 *
 *  This enumeration is used as the SPS syntax element <tt>chroma_format_idc</tt> specified
 *  in ITU-T Rec. H.265 and H.264.
*/
typedef enum {
  /** \brief (Only for H.265/AVC) YUV400. */
  VCENC_CHROMA_IDC_400 = 0,
  /** \brief YUV420. */
  VCENC_CHROMA_IDC_420 = 1,
  /** \brief (Under development) YUV422. */
  VCENC_CHROMA_IDC_422 = 2,
  VCENC_CHROMA_IDC_444 = 3,
} VCEncChromaIdcType;

/** Specifies target types based on which encoding parameters are forced for quality tuning.
 *  For details, see Section <em> \ref ss_tune</em>. */
typedef enum {
  /** Peak signal to noise ratio (PSNR).
   *
   *  Forced parameters include <tt>aq_mode=0</tt> and <tt>psyFactor=0</tt>. */
  VCENC_TUNE_PSNR = 0,
  /** Structural similarity index measure (SSIM).
   *
   *  Forced parameters include <tt>aq_mode=2</tt> and <tt>psyFactor=0</tt>. */
  VCENC_TUNE_SSIM = 1,
  /** Visual.
   *
   *  Forced parameters include <tt>aq_mode=2</tt> and <tt>psyFactor=0.75</tt>. */
  VCENC_TUNE_VISUAL = 2,
  /** Sharpness-based visual.
   *
   *  Forced parameters include <tt>aq_mode=2</tt>, <tt>psyFactor=0.75</tt>,
   *  <tt>inLoopDSRatio=0</tt>, and <tt>enableRdoQuant=0</tt>. */
  VCENC_TUNE_SHARP_VISUAL = 3
} VCENC_TuneType;

/*------------------------------------------------------------------------------
      3. Structures for API function parameters
  ------------------------------------------------------------------------------*/
/** \brief For 1pass agop */
struct VCEncAGopInfo {
  int sliceType;  // Slice type decided by lookahead
  i32 gopSize;
  u32 motionScore[2][2];
  u32 poc;
  double intra_ratio;
  double skip_ratio;
  u32 PBFrame4NRdCost;
};

typedef struct {
  int gop_frm_num;
  double sum_intra_vs_interskip;
  double sum_skip_vs_interskip;
  double sum_intra_vs_interskipP;
  double sum_intra_vs_interskipB;
  int sum_costP;
  int sum_costB;
  int last_gopsize;
  struct VCEncAGopInfo gop_info[8];
} VCENCAdapGopCtr;

/** \brief Contains the CU or MB statistics of a picture, which is output by the hardware. */
typedef struct {
  /** \brief A pointer to the memory that stores the total CU number by the end of each CTU. */
  u32 *ctuOffset;
  /** \brief The CU/MB statistics of a picture that is output by the hardware. */
  u8 *cuData;
} VCEncCuOutData;

/** \brief Contains the motion information of a coding unit (CU) in inter mode. */
typedef struct {
  /** \brief The index of the reference list in use.
   *  \n <tt>0</tt>: reference list 0.
   *  \n <tt>1</tt>: reference list 1. */
  u8 refIdx;
  /** \brief The horizontal motion in the unit of 1/4 pixel. */
  i16 mvX;
  /** \brief The vertical motion in the unit of 1/4 pixel. */
  i16 mvY;
} VCEncMv;


/** \brief Contains ITU-T T.35 information configurations. */
typedef struct {
  /** \brief Whether to insert dynamic HDR information as metadata.
   *  \n <tt>0</tt>: do not insert.
   *  \n <tt>1</tt>: insert. */
  u8 t35_enable;
  /** \brief The country code defined in ITU-T Rec. T.35. */
  u8 t35_country_code;
  /** \brief The country code extension defined in ITU-T Rec. T.35. */
  u8 t35_country_code_extension_byte;
  /** \brief A pointer to the buffer that stores T.35 payload data. */
  const u8 *p_t35_payload_bytes;
  /** \brief The size of valid T.35 payload data in bytes. */
  u32 t35_payload_size;
} ITU_T_T35;

/** \brief Contains the color volume information of the mastering display for HDR10. */
typedef struct {
  /** \brief Whether to insert mastering display color colume information.
   *  \n <tt>0</tt>: do not insert.
   *  \n <tt>1</tt>: insert. */
  u8 hdr10_display_enable;

  /** \brief The normalized X chromaticity coordinate of component 0.
   *  \n The value range is from <tt>0</tt> to <tt>50000</tt>, inclusive.
   *  \n This field is equivalent to <tt>display_primaries_x[0]</tt> in the mastering display
   *  color colume information. */
  u16 hdr10_dx0;
  /** \brief The normalized Y chromaticity coordinate of component 0.
   *  \n The value range is from <tt>0</tt> to <tt>50000</tt>, inclusive.
   *  \n This field is equivalent to <tt>display_primaries_y[0]</tt> in the mastering display
   *  color colume information. */
  u16 hdr10_dy0;
  /** \brief The normalized X chromaticity coordinate of component 1.
   *  \n The value range is from <tt>0</tt> to <tt>50000</tt>, inclusive.
   *  \n This field is equivalent to <tt>display_primaries_x[1]</tt> in the mastering display
   *  color colume information. */
  u16 hdr10_dx1;
  /** \brief The normalized Y chromaticity coordinate of component 1.
   *  \n The value range is from <tt>0</tt> to <tt>50000</tt>, inclusive.
   *  \n This field is equivalent to <tt>display_primaries_y[1]</tt> in the mastering display
   *  color colume information. */
  u16 hdr10_dy1;
  /** \brief The normalized X chromaticity coordinate of component 2.
   *  \n The value range is from <tt>0</tt> to <tt>50000</tt>, inclusive.
   *  \n This field is equivalent to <tt>display_primaries_x[2]</tt> in the mastering display
   *  color colume information. */
  u16 hdr10_dx2;
  /** \brief The normalized Y chromaticity coordinate of component 2.
   *  \n The value range is from <tt>0</tt> to <tt>50000</tt>, inclusive.
   *  \n This field is equivalent to <tt>display_primaries_y[2]</tt> in the mastering display
   *  color colume information. */
  u16 hdr10_dy2;
  /** \brief The normalized X chromaticity coordinate of the white point.
   *  \n The value range is from <tt>0</tt> to <tt>50000</tt>, inclusive.
   *  \n This field is equivalent to <tt>white_point_x</tt> in the mastering display color
   *  \n colume information. */
  u16 hdr10_wx;
  /** \brief The normalized X chromaticity coordinate of the white point.
   *  \n The value range is from <tt>0</tt> to <tt>50000</tt>, inclusive.
   *  \n This field is equivalent to <tt>white_point_y</tt> in the mastering display color
   *  \n colume information. */
  u16 hdr10_wy;
  /** \brief The nominal maximum display luminance.
   *  \n This field is equivalent to <tt>max_display_mastering_luminance</tt> in the mastering
   *  display color colume information. */
  u32 hdr10_maxluma;
  /** \brief The nominal minimum display luminance.
   *  \n This field is equivalent to <tt>min_display_mastering_luminance</tt> in the mastering
   *  display color colume information. */
  u32 hdr10_minluma;
} Hdr10DisplaySei;

/** \brief Contains the content light level information for HDR10. */
typedef struct {
  /** \brief Whether to insert content light level information.
   *  \n <tt>0</tt>: do not insert.
   *  \n <tt>1</tt>: insert. */
  u8 hdr10_lightlevel_enable;

  /** \brief The <tt>max_content_light_level</tt> field in the content light level information. */
  u16 hdr10_maxlight;
  /** \brief The <tt>max_pic_average_light_level</tt> field in the content light level information. */
  u16 hdr10_avglight;
} Hdr10LightLevelSei;

/**
 *  \brief Defines the color description in the VUI, which is coded in the sequence parameter set (SPS).
 *  \n This structure is valid only if the value of <tt>VCEncCodingCtrl.vuiVideoSignalTypePresentFlag</tt>
 *  is <tt>1</tt>.
 */
typedef struct {
  /** \brief Whether to present the color description in the VUI.
   *  \n <tt>0</tt>: do not present.
   *  \n <tt>1</tt>: present. */
  u8 vuiColorDescripPresentFlag;
  /** \brief The index of chromaticity coordinates.
   *  \n The value range is from <tt>0</tt> to <tt>9</tt>, inclusive.
   *  \n For details, see Table E.3 in ITU-T Rec. H.265. */
  u8 vuiColorPrimaries;
  /** \brief The reference of the opto-electronic transfer function of the source picture.
   *  \n <tt>0</tt>: ITU-R Rec. BT.2020.
   *  \n <tt>1</tt>: SMPTE ST 2084.
   *  \n <tt>2</tt>: ARIB STD-B67.
   *  \n For details, see Table E.4 in ITU-T Rec. H.265. */
  u8 vuiTransferCharacteristics;
  /** \brief The index of matrix coefficients used for deriving luma and chroma signals from green,
   *  blue, and red or Y, Z, and X primaries.
   *  \n The value range is from <tt>0</tt> to <tt>9</tt>, inclusive.
   *  \n For details, see Table E.5 in ITU-T Rec. H.265. */
  u8 vuiMatrixCoefficients;
} VuiColorDescription;

/**
   * \brief Contains the statistics of a CU from the video encoder.
   * \n You can enable or disable CU statistics output through <tt>VCEncConfig.enableOutputCuInfo</tt>.
   * When CU statistics output is enabled, you can obtain the CU information by calling
   * <tt>VCEncGetCuInfo()</tt>.
   * \n The video encoder supports CU information output in different formats. Only fields valid for
   * the version selected through <tt>VCEncConfig.cuInfoVersion</tt> are parsed and filled into this
   * structure. For details about CU information formats, see <em>Hantro VC9000E Series Memory Buffer
   * and Format Organization</em>.
   */
typedef struct {
  /** \brief The X coordinate of the top-left corner of the CU relative to its CTU. */
  u8 cuLocationX;
  /** \brief The Y coordinate of the top-left corner of the CU relative to its CTU. */
  u8 cuLocationY;
  /** \brief The CU size.
   *  \n <tt>8</tt>: 8x8 pixels.
   *  \n <tt>16</tt>: 16x16 pixels.
   *  \n <tt>32</tt>: 32x32 pixels.
   *  \n <tt>64</tt>: 64x64 pixels. */
  u8 cuSize;
  /** \brief The CU mode.
   *  \n <tt>0</tt>: inter mode.
   *  \n <tt>1</tt>: intra mode. */
  u8 cuMode;
  /** \brief The SSE cost of the selected CU mode.
   *  \n This field is invalid for the IPCM mode. */
  u32 cost;
  /** \brief The SSE cost of the inter or intra CU mode other than the selected CU mode.
   *  \n This field is invalid for the IPCM mode. */
  u32 costOfOtherMode;
  /** \brief The SATD cost of the intra mode. */
  u32 costIntraSatd;
  /** \brief The SATD cost of the inter mode. */
  u32 costInterSatd;
  /** \brief The prediction direction.
   *  \n <tt>0</tt>: unidirectional by list 0.
   *  \n <tt>1</tt>: unidirectional by list 1.
   *  \n <tt>2</tt>: bi-directional.
   *  \n This field is valid only for an inter-mode CU. */
  u8 interPredIdc;
  /** \brief The motion information.
   *  \n <tt>mv[0]</tt> is for list 0 if list 0 is valid.
   *  \n <tt>mv[1]</tt> is for list 1 if list 1 is valid.
   *  \n This field is valid only for an inter-mode CU. */
  VCEncMv mv[2];
  /** \brief The partition mode.
   *  \n <tt>0</tt>: 2Nx2N.
   *  \n <tt>1</tt>: NxN.
   *  \n This field is valid only for an intra-mode CU. */
  u8 intraPartMode;
  /** \brief The prediction mode.
   *  \n Each entry can be set to a value in range 0 to 34, inclusive.
   *  \n - <tt>0</tt>: planar.
   *  \n - <tt>1</tt>: DC.
   *  \n - <tt>2</tt> to <tt>34</tt>: angular in ITU-T Rec. H.265.
   *  \n
   *  \n For HEVC (H.265) and AV1 encoding:
   *  \n - If the value of <tt>intraPartMode</tt> is 1, <tt>intraPredMode[0]</tt> is valid.
   *  \n - If the value of <tt>intraPartMode</tt> is 0, <tt>intraPredMode[0]</tt> to
   *  <tt>intraPredMode[3]</tt> are valid.
   *  \n For AV1, this field is equivalent to y_mode in the specification.
   *  \n For H.264 (AVC) encoding, <tt>intraPredMode[0]</tt> to <tt>intraPredMode[15]</tt> are valid. */
  u8 intraPredMode[16];
  /** \brief The QP used to encode the CU. */
  u8 qp;
  /** \brief The mean value of the CU in the input picture. */
  u32 mean;
  /** \brief The variance value of the CU in the input picture. */
  u32 variance;
} VCEncCuInfo;

/** Defines re-encoding strategies. */
typedef enum {
  /** Does not re-encode the frame. */
  NO_RE_ENCODE,
  /** Re-encodes the frame with a new slice QP value. */
  NEW_QP = 1,
  /** Re-encodes the frame with a new output stream buffer. */
  NEW_OUTPUT_BUFFER = 2,
  /** Re-encodes the frame with a new target bit rate for block-level rate control. */
  NEW_TARGET_BIT = 3,
  /** Re-encodes the frame for timeout. */
  NEW_RE_ENCODE_FOR_TIMEOUT = 4
} ReEncodeStrategy;

/** \brief Contains statistics collected during the encoding of a frame. */
typedef struct {
  /** \brief The EWL instance handle. */
  const void *kEwl;
  /** \brief The average luma QP. */
  u32 avg_qp_y;
  /** \brief The target number of bits estimated for the current frame. */
  u32 frame_target_bits;
  /** \brief The actual number of bits generated for the current frame. */
  u32 frame_real_bits;
  /** \brief The luma PSNR. */
  double psnr_y;
  /** \brief The stream size generated by software, in bytes. */
  u32 header_stream_byte;
  /** \brief The list of stream output buffers. */
  const void *kOutputbufferMem[MAX_STRM_BUF_NUM];
  /** \brief Whether buffer overflow occurs in the encoding of the current frame.
   *  \n <tt>0</tt>: does not occur.
   *  \n <tt>1</tt>: occurs. */
  u32 output_buffer_over_flow;
  /** \brief Whether scene change occurs in the encoding of the current frame.
   *  \n <tt>0</tt>: does not occur.
   *  \n <tt>1</tt>: occurs. */
  u32 scene_change;
  /** \brief The minimum target picture size used for block-level rate control during encoding
   *  of the current frame. */
  u32 frame_min_size;
  /** \brief The maximum target picture size used for block-level rate control during encoding
   *  of the current frame. */
  u32 frame_max_size;
  /** \brief The slice QP used to encode the current frame. */
  u32 targetQp;
  /** \brief (For AV1 only) The POC of frame to be displayed and recovered during re-encoding. */
  u32 POCtobeDisplayAV1;
  /** \brief Whether security encoding is enabled.
   *  \n <tt>0</tt>: disabled.
   *  \n <tt>1</tt>: enabled. */
  u32 secure_mode;
  /** \brief Whether timeout.
   *  \n <tt>0</tt>: does not occur.
   *  \n <tt>1</tt>: occurs. */
  i32 encodeTimeout;
  /** \brief The AXI write pending when timeout. */
  u32 regValueAXIWritePending;
  /** \brief The AXI read pending when timeout. */
  u32 regValueAXIReadPending;
  /** \brief The AXI total pending when timeout. */
  u32 regValueAXITotalPending;
  /** \brief The frame number in decoding order. */
  u32 frameNum;
} VCEncStatisticOut;

/** \brief Contains the updated parameters for re-encoding. */
typedef struct {
  /** \brief The re-encoding strategy. */
  ReEncodeStrategy strategy;
  /** \brief The information about new output buffers.
   *  \n This field is valid only if the value of <tt>NewEncodeParams.strategy</tt> is
   *  <tt> \ref NEW_OUTPUT_BUFFER</tt>. */
  EWLLinearMem_t output_buffer_mem[MAX_STRM_BUF_NUM];
  /** \brief The new slice QP value.
   *  \n This field is valid only if the value of <tt>NewEncodeParams.strategy</tt> is
   *  <tt> \ref NEW_QP</tt>. */
  u32 qp;
  /** \brief The number of times the callback function has tried to re-encode the current picture. */
  u32 try_counter;
  /** \brief The new target picture size used by the block-level rate control algorithm.
   *  \n This field is valid only if the value of <tt>NewEncodeParams.strategy</tt> is
   *  <tt> \ref NEW_TARGET_BIT</tt>. */
  u32 targetPicSize;
  /** \brief The minimum target picture size used by the block-level rate control algorithm.
   *  \n This field is valid only if the value of <tt>NewEncodeParams.strategy</tt> is
   *  <tt> \ref NEW_TARGET_BIT</tt>. */
  u32 minPicSize;
  /** \brief The maximum target picture size used by the block-level rate control algorithm.
   *  \n This field is valid only if the value of <tt>NewEncodeParams.strategy</tt> is
   *  <tt> \ref NEW_TARGET_BIT</tt>. */
  u32 maxPicSize;
} NewEncodeParams;

/** \brief The prototype of the callback function that is called after encoding statistics are
 *  collected. This callback function needs to determine whether the current frame is to be
 *  re-encoded with new parameter settings based on the statistics.
 *  \n <tt>stat</tt>: the input that indicates the statistics collected for the current frame.
 *  \n <tt>new_params</tt>: the output that contains the updated encoding parameter settings.
 *  \n For information about the returned value, see Section <em> \ref ReEncodeStrategy</em>. */
typedef ReEncodeStrategy (*VCEncTryNewParamsCallBackFunc)(
    const VCEncStatisticOut *stat, NewEncodeParams *new_params);

/** \brief Encoding mode controls the flow of encoding. */
typedef struct {
#if 0
  /** \brief enable or disable security mode for encoding
   * \n 0 = disable security mode
   * \n 1 = enable security mode */
  u32 secure_flag                            : 1;
  /** \brief enable or disable multi-core mode for encoding
   * \n 0 = disable multi-core mode
   * \n 1 = enable multi-core mode */
  u32 mc_flag                                : 1;
#endif
  /** \brief enable or disable batch mod for multi-frame aggregation
   * \n 0 = disable batch mode
   * \n 1 = enable batch mode */
  u32 batch_flag                             : 1;
} VCEncMode;

/**
 * \brief Defines the configurations for encoder instance initialization.
 */
typedef struct {
  /** \brief encoding mode */
  VCEncMode enc_mode;
  /** \brief (Only for HEVC/H.264) The stream type. */
  VCEncStreamType streamType;
  /** \brief The stream profile. */
  VCEncProfile profile;
  /** \brief The stream level.
   *  \n If the field value is <tt>0</tt>, the level is selected automatically. For details,
   *  see Section <em> \ref ss_autoLevel</em>. */
  VCEncLevel level;
  /** \brief The stream tier. */
  VCEncTier tier;
  /** \brief The width of the encoded picture after rotation, in pixels.
   *  \n The field value is a multiple of 2.
   *  \n The width is restricted by level limitations. For details, see \ref VCEncLevel. */
  u32 width;
  /** \brief The height of the encoded picture after rotation, in pixels.
   *  \n The field value is a multiple of 2.
   *  \n The width is restricted by level limitations. For details, see \ref VCEncLevel. */
  u32 height;
  /** \brief The numerator for calculating the target frame rate. For details, see Section
   *  <em> @ref ss_frameRate</em>.
   *  \n The value range is from <tt>1</tt> to <tt>1048575</tt>, inclusive. */
  u32 frameRateNum;
  /** \brief The denominator for calculating the target frame rate. For details, see Section
   *  <em> @ref ss_frameRate</em>.
   *  \n The value range is from <tt>1</tt> to <tt>VCEncConfig.frameRateNum</tt>, inclusive. */
  u32 frameRateDenom;
  /** \brief The number of reference frame buffers.
   *  \n The value range is from <tt>0</tt> to <tt>8</tt>, inclusive. For example:
   *  \n - For encoding of I-frames only, the field value is <tt>0</tt>.
   *  \n - If <tt>gopSize=1</tt> and <tt>interlacedFrame=0</tt>, the field value is <tt>1</tt>.
   *  \n - If <tt>gopSize=1</tt> and <tt>interlacedFrame=1</tt>, the field value is <tt>2</tt>.
   *  \n - If <tt>gopSize=2</tt> or <tt>3</tt>, the field value is <tt>2</tt>.
   *  \n - If <tt>gopSize=4</tt>, <tt>5</tt>, <tt>6</tt>, or <tt>7</tt>, the field value is
   *  <tt>3</tt>.
   *  \n - If <tt>gopSize=8</tt>, the field value is <tt>4</tt>.
   *  \n - If <tt>gopSize=8</tt> with SVC-T, 7 B-frames, 1 P-frame, and 4 layers, the field value
   *  is <tt>8</tt>. */
  u32 refFrameAmount;
  /** \brief Whether to enable normal or strong smoothing.
   *  \n <tt>0</tt>: normal smoothing.
   *  \n <tt>1</tt>: strong smoothing. */
  u32 strongIntraSmoothing;
  /** \brief Whether to enable reference frame compression (RFC). For details, see Section
   *  <em> \ref ss_rfc</em>.
   *  \n <tt>0</tt>: disables RFC.
   *  \n <tt>1</tt>: enables RFC only for luma data. Currently, luma-only RFC is not supported.
   *  \n <tt>2</tt>: enables RFC only for chroma data. Currently, chroma-only RFC is not supported.
   *  \n <tt>3</tt>: enables RFC for both luma and chroma data. */
  u32 compressor;
  /** \brief Whether the input frames are progressive or interlaced.
   *  \n Interlaced frame input is supported only for HEVC (H.265). For details, see Section
   *  <em> @ref ss_interlace</em>.
   *  \n <tt>0</tt>: progressive frame input.
   *  \n <tt>1</tt>: interlace frame input. */
  u32 interlacedFrame;
  /** \brief The bit depth of luma samples in the encoded stream.
   *  \n <tt>8</tt>: 8-bit luma samples.
   *  \n <tt>10</tt>: 10-bit luma samples. */
  u32 bitDepthLuma;
  /** \brief The bit depth of chroma samples in the encoded stream.
   *  \n <tt>8</tt>: 8-bit chroma samples.
   *  \n <tt>10</tt>: 10-bit chroma samples. */
  u32 bitDepthChroma;
  /** \brief Whether to output CU/MB statistics. For details, see Section <em> @ref ss_outCuInfo</em>.
   *  \n <tt>0</tt>: do not output.
   *  \n <tt>1</tt>: output. */
  u32 enableOutputCuInfo;
  /** \brief Whether to output frame statistics. For details, see Section <em> @ref ss_outFrmInfo</em>.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 enableFrameInfo;
  /** \brief Whether to output luma statistics. For details, see Section <em> @ref ss_outLumaInfo</em>.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 enablelumaInfo;
  /** \brief Whether to output CTB bit statistics. For details, see Section <em> @ref ss_outCtbBits</em>.
   *  \n <tt>0</tt>: do not output.
   *  \n <tt>1</tt>: output. */
  u32 enableOutputCtbBits;
  /** \brief Whether to enable SSIM calculation.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 enableSsim;
  /** \brief Whether to enable PSNR calculation.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 enablePsnr;
  /** \brief The maximum number of temporal layers. */
  u32 maxTLayers;
  /** \brief The RDO level, which controls how much effort the video encoder uses for mode selection.
   *  For details, see Section <em> @ref ss_rdolevel</em>.
   *  \n <tt>0</tt>: runs 1x candidates.
   *  \n <tt>1</tt>: runs 2x candidates.
   *  \n <tt>2</tt>: runs 3x candidates. */
  u32 rdoLevel;
  /** \brief The video codec standard. */
  VCEncVideoCodecFormat codecFormat;
  /** \brief Obsoleted. Use <tt>VCEncLogSetting.out_level</tt> instead. */
  u32 verbose;
  /** \brief The alignment requirement for both luma and chroma input frame buffers.
   *  \n The start address of each line in the buffer must be aligned to
   *  power(2, <tt>exp_of_input_alignment</tt>).
   *  \n <tt>exp_of_input_alignment</tt> must be greater than or equal to the
   *  <tt>AXI_burst_align_rd_prp</tt> subfield of <tt>VCEncConfig.AXIAlignment</tt>. */
  u32 exp_of_input_alignment;
  /** \brief The alignment requirement for luma reference frame buffers.
   *  \n The start address of each line in the buffer must be aligned to
   *  power(2, <tt>exp_of_ref_alignment</tt>).
   *  \n <tt>exp_of_ref_alignment</tt> must be greater than or equal to the
   *  <tt>AXI_burst_align_rd_lu_ref_prefetch</tt> and <tt>AXI_burst_align_wr_luma_ref</tt>
   *  subfields of <tt>VCEncConfig.AXIAlignment</tt>. */
  u32 exp_of_ref_alignment;
  /** \brief The alignment requirement of chroma reference frame buffers.
   *  \n The start address of each line in the buffer must be aligned to
   *  power(2, <tt>exp_of_ref_ch_alignment</tt>).
   *  \n <tt>exp_of_ref_ch_alignment</tt> must be greater than or equal to
   *  the <tt>AXI_burst_align_rd_ch_ref_prefetch</tt> and <tt>AXI_burst_align_wr_chroma_ref</tt>
   *  subfields of <tt>VCEncConfig.AXIAlignment</tt>. */
  u32 exp_of_ref_ch_alignment;
  /** \brief The alignment requirement of adaptive quantization output buffers.
   *  \n The start address of each line in the buffer must be aligned to
   *  power(2, <tt>exp_of_aqinfo_alignment</tt>). */
  u32 exp_of_aqinfo_alignment;
  /** \brief The alignment requirement of output stream buffers.
   *  \n The buffer size must be aligned to power(2, <tt>exp_of_tile_stream_alignment</tt>).
   *  \n <tt>exp_of_tile_stream_alignment</tt> must be greater than or equal to
   *  the <tt>AXI_burst_align_wr_stream</tt> subfield of <tt>VCEncConfig.AXIAlignment</tt>. */
  u32 exp_of_tile_stream_alignment;
  /** \brief The allocation mode of reconstructed frames.
   *  \n <tt>0</tt>: Reconstructed frames are allocated by the encoder.
   *  \n <tt>1</tt>: Reconstructed frames are allocated by the application. */
  u32 exteralReconAlloc;
  /** \brief Whether to store reference frames in tiled or raster P010 format in buffers.
   *  \n <tt>0</tt> (default): tile.
   *  \n <tt>1</tt>: raster. */
  u32 P010RefEnable;
  /** \brief The block-level QP adjustment mode for precise and subjective rate control.
    * \n <tt>0</tt>: no block-level QP adjustment.
    * \n <tt>1</tt>: block-level QP adjustment for subjective rate control only.
    * \n <tt>2</tt>: block-level QP adjustment for precise rate control only.
    * \n <tt>3</tt>: block-level QP adjustment for both precise and subjective rate control. */
  u32 ctbRcMode;
  /** \brief The method to encode POC in the slice header.
   *  \n This field is equivalent to <tt>pic_order_cnt_type</tt> defined in the ITU-T Rec. H.264. */
  u32 picOrderCntType;
  /** \brief The number of bits used to encode POC in the slice header.
   *  \n This field is equal to (4 + <tt>log2_max_pic_order_cnt_lsb_minus4</tt>), which is
   *  defined in the ITU-T Rec. H.264. */
  u32 log2MaxPicOrderCntLsb;
  /** \brief The number of bits used to encode frame_num in the H.264 (AVC) slice header.
   *  \n This field is equal to (4 + <tt>log2_max_frame_num_minus4</tt>), which is defined in
   *  the ITU-T Rec. H.264. */
  u32 log2MaxFrameNum;
  /** \brief Whether to dump register values for debugging.
   *  \n <tt>0</tt>: do not dump.
   *  \n <tt>1</tt>: dump. */
  u32 dumpRegister;
  /** \brief Whether to dump CU/MB statistics for debugging after encoding each frame.
   *  \n <tt>0</tt>: do not dump.
   *  \n <tt>1</tt>: dump. */
  u32 dumpCuInfo;
  /** \brief Whether to dump CTB bit statistics after encoding one frame.
   *  \n <tt>0</tt>: do not dump.
   *  \n <tt>1</tt>: dump. */
  u32 dumpCtbBits;
  /** \brief Whether to dump reconstructed YUV frames in tiled or raster format for debugging
   *  when the encoder runs on FPGA and hardware.
   *  \n <tt>0</tt> (default): tiled format.
   *  \n <tt>1</tt>: raster format. */
  u32 rasterscan;
  /** \brief The number of cores that run in parallel at the frame level. */
  u32 parallelCoreNum;
  /** \brief A multi-pass coding configuration, for internal testing only. */
  u32 pass;
  /** \brief Whether to enable GOP structure size adaption based on pass-1 information.
   *  \n <tt>true</tt>: enable.
   *  \n <tt>false</tt>: disable. */
  bool bPass1AdaptiveGop;
  /** \brief The number of look-ahead frames used to encode each frame.
   *  \n <tt>0</tt>: disables look-ahead encoding.
   *  \n <tt>4</tt> to <tt>40</tt>: enables look-ahead encoding with the specified number of
   *  look-ahead frames. */
  u8 lookaheadDepth;
  /** \brief The external down-scaling ratio for the pass-1 encoder.
   *  \n <tt>0</tt>: down-scaling disabled.
   *  \n <tt>1</tt>: 1/2 down-scaling ratio. */
  u32 extDSRatio;
  /** \brief The in-loop down-scaling ratio for the pass-1 encoder.
   *  \n <tt>0</tt>: down-scaling disabled.
   *  \n <tt>1</tt>: 1/2 down-scaling ratio. */
  u32 inLoopDSRatio;
  /** \brief The CU information version.
   *  \n If the field value is <tt>-1</tt>, the version is decided by the hardware support.
   *  \n Other valid values include <tt>0</tt>, <tt>1</tt>, and <tt>2</tt>. For details,
   *  see <em>Hantro VC9000E Series Memory Buffer and Format Organization</em>. */
  i32 cuInfoVersion;
  /** \brief The GOP structure size.
   *  \n To enable GOP structure size adaption, set this field to <tt>0</tt>. */
  u32 gopSize;
    /** \brief enable or disable anti intra flicker (aif)
   * \n 0 - disable aif
   * \n 1 - enable aif */
  u32 aifEnable;
  /** \brief reference frame number of P frame, numRefP=1|2 */
  u32 numRefP;
  /** \brief The index of the server slice node. */
  u32 slice_idx;

  /* External SRAM */
  /** \brief The capacity of external SRAM for luma backward reference, in the unit of reference line.
   *  \n This field is valid only if external SRAM is enabled to save motion estimation (ME) bandwidth. */
  u32 extSramLumHeightBwd;
  /** \brief The capacity of external SRAM for chroma backward reference, in the unit of reference line.
   *  \n This field is valid only if external SRAM is enabled to save ME bandwidth. */
  u32 extSramChrHeightBwd;
  /** \brief The capacity of external SRAM for luma forward reference, in the unit of reference line.
   *  \n This field is valid only if external SRAM is enabled to save ME bandwidth. */
  u32 extSramLumHeightFwd;
  /** \brief The capacity of external SRAM for chroma forward reference, in the unit of reference line.
   *  \n This field is valid only if external SRAM is enabled to save ME bandwidth. */
  u32 extSramChrHeightFwd;

  /* AXI alignment */
  /** \brief The AXI alignment. The start address of each line in the buffer must be aligned to
   *  two to the <em>N</em>th power where <em>N</em> is the value of the corresponding field.
   *  \n Bits 35:32: <tt>AXI_burst_align_wr_cuinfo</tt>
   *  \n Bits 31:28: <tt>AXI_burst_align_wr_common</tt>
   *  \n Bits 27:24: <tt>AXI_burst_align_wr_stream</tt>
   *  \n Bits 23:20: <tt>AXI_burst_align_wr_chroma_ref</tt>
   *  \n Bits 19:16: <tt>AXI_burst_align_wr_luma_ref</tt>
   *  \n Bits 15:12: <tt>AXI_burst_align_rd_common</tt>
   *  \n Bits 11:8: <tt>AXI_burst_align_rd_prp</tt>
   *  \n Bits 7:4: <tt>AXI_burst_align_rd_ch_ref_prefetch</tt>
   *  \n Bits 3:0: <tt>AXI_burst_align_rd_lu_ref_prefetch</tt> */
  u64 AXIAlignment;

  /* Irq Type Mask */
  /** \brief The interrupt type bitmap of the encoder.
   *  \n Bit 8: <tt>irq_type_sw_reset_mask</tt>
   *  \n Bit 7: <tt>irq_type_fuse_error_mask</tt>
   *  \n Bit 6: <tt>irq_type_buffer_full_mask</tt>
   *  \n Bit 5: <tt>irq_type_bus_error_mask</tt>
   *  \n Bit 4: <tt>irq_type_timeout_mask</tt>
   *  \n Bit 3: <tt>irq_type_strm_segment_mask</tt>
   *  \n Bit 2: <tt>irq_type_line_buffer_mask</tt>
   *  \n Bit 1: <tt>irq_type_slice_rdy_mask</tt>
   *  \n Bit 0: <tt>irq_type_frame_rdy_mask</tt>
   *  \n
   *  \n Each bit indicates whether a normal or abnormal interrupt is raised.
   *  \n <tt>0</tt>: normal interrupt. The interrupt makes VCMD exit the stall status and
   *  continue with subsequent commands in the VCMD buffer.
   *  \n <tt>1</tt>: abnormal interrupt. The interrupt keeps VCMD in stall status and directly
   *  transfers the interrupt to CPU. */
  u32 irqTypeMask;

  /* Irq Type Cutree Mask */
  /** \brief The interrupt type bitmap of CuTree.
   *  \n Bit 5: <tt>irq_type_bus_error_mask</tt>
   *  \n Bit 4: <tt>irq_type_timeout_mask</tt>
   *  \n Bit 0: <tt>irq_type_frame_rdy_mask</tt>
   *  \n
   *  \n Each bit indicates whether a normal or abnormal interrupt is raised.
   *  \n <tt>0</tt>: normal interrupt. The interrupt makes VCMD exit the stall status and
   *  continue with subsequent commands in the VCMD buffer.
   *  \n <tt>1</tt>: abnormal interrupt. The interrupt keeps VCMD in stall status and directly
   *  transfers the interupt to CPU. */
  u32 irqTypeCutreeMask;

   /** \brief The chroma sampling modes relative to luma sampling. For details, see Section
    *  <em> \ref s_fmt</em>.
    *  \n This field is used as the SPS syntax element chroma_format_idc specified in ITU-T
    *  Rec. H.265 and H.264. */
  VCEncChromaIdcType codedChromaIdc;
  /** \brief The type of the target based on which some encoding parameters are forced for
   *  quality tuning. For details, see Section <em> \ref ss_tune</em>.
   *  \n The default value is <tt> \ref VCENC_TUNE_PSNR</tt>. */
  VCENC_TuneType tune;
  /** \brief Whether the hardware writes reconstructed frames to DDR for pure I-frame encoding.
   *  \n <tt>0</tt>: does not write.
   *  \n <tt>1</tt>: writes. */
  u32 writeReconToDDR;
  /** \brief (For AV1 only) Whether to enable TX type search.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt> (default): enable. */
  u32 TxTypeSearchEnable;
  /** \brief (For AV1 only) Whether to enable interpolation filter switch.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt> (default): enable. */
  u32 av1InterFiltSwitch;
  /** \brief A flag for stream-level encoding control.
   *  \n Set this flag to <tt>1</tt> only if <tt>VCEncSetStrmCtrl()</tt> is used to reset the
   *  instance status for a new stream. */
  u32 fileListExist;
  /** \brief The maximum AXI burst length, in the unit of AXI bus width. */
  u32 burstMaxLength;
  /** \brief Whether to enable temporal motion vector prediction (TMVP).
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 enableTMVP;
  /** \brief Whether to enable reference ring buffer.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 refRingBufEnable;

  /** \brief Trailing area detection strength */
  u32 ctbRcTrailStrength;
  /* tile */
  /** \brief (For HEVC only) Whether to enable tile-based encoding.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  i32 tiles_enabled_flag;
  /** \brief The number of tile columns to divide the picture.
   * \n The only available value is <tt>2</tt>. */
  i32 num_tile_columns;
  /** \brief The number of tile rows to divide the picture. */
  i32 num_tile_rows;
  /** \brief Whether to enable the loop filter across the tile boundary.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  i32 loop_filter_across_tiles_enabled_flag;
  /** \brief An array with a total of <tt>VCEncConfig.num_tile_columns</tt> entries.
   *  \n Each entry indicates the width of a tile column. */
  i32 *tile_width;
  /** \brief An array with a total of <tt>VCEncConfig.num_tile_rows</tt> entries.
   *  \n Each entry indicates the width of a tile column. */
  i32 *tile_height;
  /** \brief Reserved. */
  i32 tileMvConstraint;

  /** \brief Whether to enable security encoding.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 secure_mode;
  /** \brief Whether to bind input and output buffers.
   *  \n <tt>0</tt>: do not bind.
   *  \n <tt>1</tt>: bind. */
  u32 bIOBufferBinding;
  /** \brief The callback function called to obtain new parameters for re-encoding. */
  VCEncTryNewParamsCallBackFunc cb_try_new_params;
  /** \brief Whether to enable external frame buffer compression (UFBC) for input frame buffers.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 ufbcEnable;
  /** \brief Whether to enable RGB-to-YUV conversion in UFBC.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 ufbcYuvTrans;

  /** \brief The superblock type of UFBC.
   *  \n <tt>0</tt>: 32x8.
   *  \n <tt>1</tt>: 16x16. */
  u32 ufbcBlockType;

  /** \brief Whether to enable block split mode in UFBC.
   *  \n <tt>0</tt>: Block split mode off.
   *  \n <tt>1</tt>: Block split mode on. */
  u32 ufbcBlockSplit;

  /** \brief the priority of current instance */
  u32 priority;

  /** \brief enable encoding of leading pictures */
  u32 enableLeadingPictures;

  /** \brief the testId for internal test */
  u32 testId;
  /** \brief the device name of enc driver.
   *  \n default is "vsi_vcx" */
  char *enc_dev;

  /** \brief the device name of memalloc driver
   *  \n default is "memalloc" */
  char *mem_dev;
  /** \brief Whether to use vcmd. */
  u32 useVcmd;

  /** \brief lowlatency handshake auto gating disable
   *  \n <tt>0</tt>: enable.
   *  \n <tt>1</tt>: disable.    */
  u32 lowlatGatingDisable;
  /** \brief lowlatency handshake auto gating type*/
  u32 lowlatGatingType;
  /** \brief bus idle counter max value*/
  u32 lowlatGatingCyc;
  /** \brief disable TU32. */
  u32 disableTU32;
} VCEncConfig;

/** \brief Contains reference frame information. */
typedef struct {
  /** \brief The POC delta of the short-term reference frame relative to the current picture,
   *  or the index of LTR. */
  i32 ref_pic;
  /** \brief Whether the current picture uses the reference frame.
   *  \n <tt>0</tt>: does not use.
   *  \n <tt>1</tt>: uses. */
  u32 used_by_cur;
} VCEncGopPicRps;

/**
   * \brief Defines reference description for a frame in a GOP.
   */
typedef struct {
  /** \brief The picture order count (POC) of the frame in the GOP.
   *  \n The value range is from <tt>1</tt> to <tt>gopSize</tt>, inclusive. */
  i32 poc;
  /** \brief The quantization parameter (QP) offset of the frame. */
  i32 QpOffset;
  /** \brief The QP factor of the frame. */
  double QpFactor;
  /** \brief The temporal layer ID where the frame locates. */
  i32 temporalId;
  /** \brief The coding type of the frame. */
  VCEncPictureCodingType codingType;
  /** \brief Whether the frame is used for future reference.
   *  \n <tt>0</tt>: used for future reference.
   *  \n <tt>1</tt>: not used for future reference. */
  u32 nonReference;

  /** \brief The number of reference frames kept for the current frame.
   *  \n The value range is from <tt>0</tt> to <tt> \ref VCENC_MAX_REF_FRAMES</tt>, inclusive. */
  u32 numRefPics;
  /** \brief The reference frame information used to encode the current frame. */
  VCEncGopPicRps refPics[VCENC_MAX_REF_FRAMES];
} VCEncGopPicConfig;

/**
 * \brief Defines reference description for a special frame in a GOP.
 */
typedef struct {
  /** \brief The picture order count (POC) of the frame in the GOP.
   *  \n The value range is from <tt>1</tt> to <tt>gopSize</tt>, inclusive. */
  u32 poc;
  /** \brief The quantization parameter (QP) offset of the frame. */
  i32 QpOffset;
  /** \brief The QP factor of the frame. */
  double QpFactor;
  /** \brief The temporal layer ID where the frame locates. */
  i32 temporalId;
  /** \brief The coding type of the frame. */
  VCEncPictureCodingType codingType;
  /** \brief Whether the frame is used for future reference.
   *  \n <tt>0</tt>: used for future reference.
   *  \n <tt>1</tt>: not used for future reference. */
  u32 nonReference;
  /** \brief The number of reference frames kept for the current frame.
   *  \n The value range is from <tt>0</tt> to <tt> \ref VCENC_MAX_REF_FRAMES</tt>, inclusive. */
  u32 numRefPics;
  /** \brief The reference frame information used to encode the current frame. */
  VCEncGopPicRps refPics[VCENC_MAX_REF_FRAMES];
  /** \brief The index of the long-term reference (LTR) frame.
   *  \n <tt>0</tt>: The current frame is a frame uses LTR.
   *  \n A value in the range from <tt>1</tt> to <tt> \ref VCENC_MAX_LT_REF_FRAMES</tt>:
   *  The current frame is an LTR frame. */
  i32 i32Ltr;
  /** \brief The POC of the first special frame relative to the IDR frame.
   *  \n The value must be less than <tt>VCEncGopPicSpecialConfig.i32Interval</tt>. */
  i32 i32Offset;
  /** \brief The interval between two pictures to which the special frame configurations
   *  are applied. */
  i32 i32Interval;
  /** \brief Whether only short-term reference frames are involved in the structure configurations.
   *  \n <tt>0</tt>: Long-term reference frames are involved.
   *  \n <tt>1</tt>: Only short-term reference frames are involved.
   *  \n The field value is derived from <tt>VCEncGopPicSpecialConfig.i32Ltr</tt> and
   *  <tt>VCEncGopPicSpecialConfig.refPics</tt>, with no need for manual configuration. */
  i32 i32short_change;
} VCEncGopPicSpecialConfig;

/** \brief Defines the GOP structure. */
typedef struct {
  /** \brief A pointer to the array that contains the reference descriptions
   *  (<tt>VCEncGopPicConfig</tt>) of all frames in the GOP. */
  VCEncGopPicConfig *pGopPicCfg;
  /** \brief The number of <tt>VCEncGopPicConfig</tt> entries in <tt>VCEncGopConfig.pGopPicCfg</tt>.
   *  \n The value range is from <tt>0</tt> to <tt> \ref MAX_GOP_PIC_CONFIG_NUM</tt>, inclusive. */
  u8 size;
  /** \brief The index of the <tt>VCEncGopPicConfig</tt> entry used by the current frame
   *  in <tt>VCEncGopConfig.pGopPicCfg</tt>.
   *  \n The value range is from <tt>0</tt> to (<tt>VCEncGopConfig.size</tt> - 1), inclusive. */
  u8 id;
  /** \brief The index of the <tt>VCEncGopPicConfig</tt> entry used by the next frame
   *  in <tt>VCEncGopConfig.pGopPicCfg</tt>.
   *  \n The value range is from <tt>0</tt> to (<tt>VCEncGopConfig.size</tt> - 1), inclusive. */
  u8 id_next;
  /** \brief The number of <tt>VCEncGopPicSpecialConfig</tt> entries in
   *  <tt>VCEncGopConfig.pGopPicSpecialCfg</tt>.
   *  \n The value range is from <tt>0</tt> to <tt> \ref MAX_GOP_PIC_CONFIG_NUM</tt>, inclusive. */
  u8 special_size;
  /** \brief The difference between the POC of the next frame and the POC of the current frame. */
  i32 delta_poc_to_next;
  /** \brief A pointer to the array that contains the reference descriptions
   *  (<tt>VCEncGopPicSpecialConfig</tt>) of all special frames in the GOP. */
  VCEncGopPicSpecialConfig *pGopPicSpecialCfg;

  /** \brief The number of long-term reference (LTR) frames.
   *  \n The value range is from <tt>0</tt> to <tt> \ref VCENC_MAX_LT_REF_FRAMES</tt>, inclusive. */
  u8 ltrcnt;
  /** \brief The index of each LTR frame. */
  u32 u32LTR_idx[VCENC_MAX_LT_REF_FRAMES];
  /** \brief The interval between two IDR frames, which defines the length of the GOP. */
  i32 idr_interval;
  /** \brief The interval between two non-IDR intra frames within the GOP. */
  i32 intraPeriod;
  /** \brief The first frame in the input file to be encoded. */
  i32 firstPic;
  /** \brief The last frame in the input file to be encoded. */
  i32 lastPic;
  /** \brief The numerator of the output frame rate. */
  i32 outputRateNumer;
  /** \brief The denominator of the output frame rate. */
  i32 outputRateDenom;
  /** \brief The numerator of the input frame rate. */
  i32 inputRateNumer;
  /** \brief The denominator of the input frame rate. */
  i32 inputRateDenom;
  /** \brief Whether the GOP configuration is low-delay configuration. */
  i32 gopLowdelay;
  /** \brief Whether input formats are interfaced.
   *  \n The interlaced frame input format is valid only for HEVC (H.265). For details, see
   *  Section <em> @ref ss_interlace</em>. */
  i32 interlacedFrame;
  /** \brief The start index of frames in <tt>VCEncGopConfig.pGopPicCfg</tt> for the GOP
   *  structure of each size. */
  u8 gopCfgOffset[MAX_GOP_SIZE + 1];
  /** \brief The start index of frames in <tt>VCEncGopConfig.pGopPicCfg</tt> for the GOP
   *  structure of leading pictures. */
  u8 gopCfgOffsetLP;
  /** \brief A pointer to the array that contains the reference descriptions
   *  (<tt>VCEncGopPicConfig</tt>) of all frames for use of pass-1 encoder. */
  VCEncGopPicConfig *pGopPicCfgPass1;
  /** \brief A pointer to the array that contains the reference descriptions
   *  (<tt>VCEncGopPicConfig</tt>) of all frames for use of pass-2 encoder. */
  VCEncGopPicConfig *pGopPicCfgPass2;
} VCEncGopConfig;

/** \brief Defines a rectangular area in a picture. */
typedef struct {
  /** \brief Whether to enable the area.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 enable;
  /** \brief The top MB or CTB row inside the area.
   *  \n The value range is from <tt>0</tt> to (heightMbs - 1), inclusive, where heightMbs
   *  indicates the picture height in the unit of MB or CTB. */
  u32 top;
  /** \brief The leftmost MB or CTB column inside the area.
   *  \n The value range is from <tt>0</tt> to (widthMbs - 1), inclusive, where widthMbs
   *  indicates the picture width in the unit of MB or CTB. */
  u32 left;
  /** \brief The bottom MB or CTB row inside the area.
   *  \n The value range is from <tt>VCEncPictureArea.top</tt> to (heightMbs - 1), inclusive,
   *  where heightMbs indicates the picture height in the unit of MB or CTB. */
  u32 bottom;
  /** \brief The rightmost MB or CTB column inside the area.
   *  \n The value range is from <tt>VCEncPictureArea.left</tt> to (widthMbs - 1), inclusive,
   *  where widthMbs indicates the picture width in the unit of MB or CTB. */
  u32 right;
} VCEncPictureArea;

/** \brief Defines an OSD region. For details, see Section <em> @ref s_osd</em>. */
typedef struct {
  /** \brief Whether to enable the OSD region.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 enable;
  /** \brief The input format of the OSD region.
    * \n <tt>0</tt>: ARGB8888.
    * \n <tt>1</tt>: NV12.
    * \n <tt>2</tt>: bitmap. */
  u32 format;
  /** \brief The global alpha value for the OSD region.
   *  \n This field is invalid for ARGB8888. */
  u32 alpha;
  /** \brief The horizontal offset, in pixels, of the top-left corner of the OSD region
   *  relative to the encoder picture. */
  u32 xoffset;
  /** \brief The horizontal offset, in pixels, of the top-left corner of the crop area in
   *  the OSD input relative to the OSD input picture. */
  u32 cropXoffset;
  /** \brief The vertical offset, in pixels, of the top-left corner of the OSD region
   *  relative to the encoder picture. */
  u32 yoffset;
  /** \brief The vertical offset, in pixels, of the top-left corner of the crop area in the
   *  OSD input relative to the OSD input picture. */
  u32 cropYoffset;
  /** \brief The width of the OSD input. */
  u32 width;
  /** \brief The width of the crop area in the OSD input, which is the width of the
   *  final OSD region. */
  u32 cropWidth;
  /** \brief (Reserved) The width of the upscaled OSD input.
   *  \n This field is valid only for specific custom versions. */
  u32 scaleWidth;
  /** \brief The height of the OSD input. */
  u32 height;
  /** \brief The width of the crop area in the OSD input, which is the height of the
   *  final OSD region. */
  u32 cropHeight;
  /** \brief (Reserved) The height of the upscaled OSD input.
   *  \n This field is valid only for specific custom versions. */
  u32 scaleHeight;
  /** \brief The Y stride of the OSD input, in bytes.
   *  \n The OSD region data is also stored with this specified stride. */
  u32 Ystride;
  /** \brief The UV stride of the OSD input, in bytes.
   *  \n The OSD region data is also stored with this specified stride. */
  u32 UVstride;
  /** \brief The global Y value for the bitmap format. */
  u32 bitmapY;
  /** \brief The global U value for the bitmap format. */
  u32 bitmapU;
  /** \brief The global V value for the bitmap format. */
  u32 bitmapV;
  /** \brief Whether the OSD input data is organized in supertile mode.
    * \n <tt>0</tt>: non-supertile mode.
    * \n <tt>1</tt>: X-major supertile mode.
    * \n <tt>2</tt>: Y-major supertile mode.
    * \n This field is valid only for the ARGB8888 format. */
  u32 superTile;
} VCEncOverlayArea;

/** \brief Defines the stream-level encoding control parameters. */
typedef struct {
  /** \brief The width of a new stream.
   *  \n The field value must be less than or equal to <tt>VCEncConfig.width</tt>. */
  u32 width;
  /** \brief The height of a new stream.
   *  \n The field value must be less than or equal to <tt>VCEncConfig.height</tt>. */
  u32 height;
} VCEncStrmCtrl;
/** @} */

/**
 * \defgroup api_codingctrl Coding Control API
 *
 * @{
 */

/** \brief Defines the coding control parameters. */
typedef struct {
  /** \brief The number of CTB or MB rows in each slice.
   *  \n If the field value is <tt>0</tt>, slice encoding is disabled and the entire frame
   *  is encoded as a slice.
   *  \n If the field value is not <tt>0</tt>, a total of [(height + ctu_size - 1)/ctu_size]
   *  slices are created, with the height of each slice equal to (ctu_size * sliceSize)
   *  except the last slice. */
  u32 sliceSize;
  /** \brief Whether to insert picture timing and buffering period SEI messages into the stream.
   *  \n <tt>0</tt>: do not insert.
   *  \n <tt>1</tt>: insert. */
  u32 seiMessages;
  /** \brief The input pixel sample range for VUI encoding.
   *  \n <tt>0</tt>: Y samples in range from 16 to 235.
   *  \n <tt>1</tt>: Y samples in range from 0 to 255. */
  u32 vuiVideoFullRange;
  /** \brief Whether to disable de-blocking filters. For HEVC (H.265), this field is used to
   *  generate <tt>deblocking_filter_disabled_flag</tt>.
   *  \n <tt>0</tt>: enables de-blocking filters.
   *  \n <tt>1</tt>: disables de-blocking filters.
   *  \n <tt>2</tt>: disables de-blocking filters for slice edges, and enables them for other regions. */
  u32 disableDeblockingFilter;
  /** \brief Whether to disable the automatic SRAM power-down mode.
   *  \n <tt>0</tt>: enable. SRAM is powered on only when it is in use.
   *  \n <tt>1</tt>: disable. SRAM is always powered on. */
  u32 sramPowerdownDisable;
  u32 sramPowerdownMode;
  u32 sramPowerdownTimerDiv32;
  /** \brief The de-blocking parameter <tt>tc_offset</tt>. */
  i32 tc_Offset;
  /** \brief The de-blocking parameter <tt>beta_offset</tt>. */
  i32 beta_Offset;
  /** \brief (For HEVC only) Whether to enable de-blocking override between frames.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. De-blocking parameters can be changed for each frame.
   *  \n This field is used to generate <tt>pps.deblocking_filter_override_enabled_flag</tt>
   *  for HEVC (H.265). */
  u32 enableDeblockOverride;

  /** \brief (For HEVC only) Whether to enable de-blocking override between slices. This field
   *  is used to generate <tt>deblocking_filter_override_flag</tt> in the slice header.
   *  \n <tt>0</tt>: disable. <tt>deblocking_filter_disabled_flag</tt>, <tt>tc_offset</tt>,
   *  and <tt>beta_offset</tt> in PPS are used.
   *  \n <tt>1</tt>: enable. <tt>deblocking_filter_disabled_flag</tt>, <tt>tc_offset</tt>,
   *  and <tt>beta_offset</tt> in the slice header are used.
   *  \n This field is valid only if the value of <tt>VCEncCodingCtrl.enableDeblockOverride</tt>
   *  is <tt>1</tt>. */
  u32 deblockOverride;
  /**< \brief Enable Transform Skip */
  u32 enableTS;
  /**< \brief log2 Max Transform Skip Size - 2 */
  u32 log2MaxTSBlockSizeMinus2;
  /** \brief Whether to enable sample adaptive offset (SAO) for HEVC (H.265) or CDEF filter for AV1.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 enableSao;
  /** \brief (For HEVC only) Whether to enable the default scaling list.
   *  \n <tt>0</tt>: disable. The average scaling list is used.
   *  \n <tt>1</tt>: enable. The default scaling list is used. */
  u32 enableScalingList;
  /** \brief The sample aspect ratio in horizontal direction, in an arbitrary unit.
   *  \n The value range is from <tt>0</tt> to <tt>65535</tt>, inclusive. If the field value is
   *  <tt>0</tt>, it indicates that the sample aspect ratio in horizontal direction is unspecified. */
  u32 sampleAspectRatioWidth;
  /** \brief The sample aspect ratio in vertical direction, in the same arbitrary unit as
   *  <tt>VCEncCodingCtrl.sampleAspectRatioWidth</tt>.
   *  \n The value range is from <tt>0</tt> to <tt>65535</tt>, inclusive. If the field value is
   *  <tt>0</tt>, it indicates that the sample aspect ratio in vertical direction is unspecified. */
  u32 sampleAspectRatioHeight;
  /** \brief (Only for AVC) The entropy coding mode.
   *  \n <tt>0</tt>: content-adaptive variable-length coding (CAVLC).
   *  \n <tt>1</tt>: content-based adaptive binary arithmetic coding (CABAC). */
  u32 enableCabac;
  /** \brief The CABAC table initialization flag <tt>cabac_init_flag</tt> for HEVC (H.265) in
   *  the slice header.
   *  \n Valid values include <tt>0</tt> and <tt>1</tt>. Currently, the hardware supports only
   *  value <tt>0</tt>. */
  u32 cabacInitFlag;
  /** \brief The order count of the first MB or CTB for cyclic intra refresh (CIR).
   *  \n The value range is from <tt>0</tt> to (ctbPerFrame - 1), inclusive. ctbPerFrame is the total
   *  number of MB or CTB in each frame. */
  u32 cirStart;
  /** \brief The interval for CIR, in MB or CTB.
   *  \n The value range is from <tt>0</tt> to ctbPerFrame, inclusive. ctbPerFrame is the total number
   *  of MB or CTB in each frame. */
  u32 cirInterval;
  /** \brief Whether to enable IPCM encoding.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable.
   *  \n IPCM encoding can be lossless. It is specified in SPS. */
  /* TODO: move it to VCEncConfig */
  i32 pcm_enabled_flag;
  /** \brief (For HEVC only) Whether to disable de-blocking and SAO filters for IPCM.
   *  \n <tt>0</tt>: enables de-blocking and SAO filters around IPCM ROIs.
   *  \n <tt>1</tt>: disables de-blocking and SAO filters for IPCM. This prevents the lossless
   *  content in the ROIs from being changed by the filters. */
  /* TODO: move it to VCEncConfig */
  i32 pcm_loop_filter_disabled_flag;
  /** \brief The ROI for forcing the intra mode. */
  VCEncPictureArea intraArea;
  /** \brief ROI 1 for forcing the IPCM mode. */
  VCEncPictureArea ipcm1Area;
  /** \brief ROI 2 for forcing the IPCM mode. */
  VCEncPictureArea ipcm2Area;
  /** \brief ROI 3 for forcing the IPCM mode. */
  VCEncPictureArea ipcm3Area;
  /** \brief ROI 4 for forcing the IPCM mode. */
  VCEncPictureArea ipcm4Area;
  /** \brief ROI 5 for forcing the IPCM mode. */
  VCEncPictureArea ipcm5Area;
  /** \brief ROI 6 for forcing the IPCM mode. */
  VCEncPictureArea ipcm6Area;
  /** \brief ROI 7 for forcing the IPCM mode. */
  VCEncPictureArea ipcm7Area;
  /** \brief ROI 8 for forcing the IPCM mode. */
  VCEncPictureArea ipcm8Area;
  /** \brief ROI 1 for forcing a QP delta or an absolute QP value. */
  VCEncPictureArea roi1Area;
  /** \brief ROI 2 for forcing a QP delta or an absolute QP value. */
  VCEncPictureArea roi2Area;
  /** \brief AIF QP delta is QP delta between AIF frame and its reference frame. */
  i32 aifQpDelta;

  /** \brief ROI 3 for forcing a QP delta or an absolute QP value.
   *  \n This field is valid only if the value of <tt>EWLHwConfig_t.ROI8Support</tt> is <tt>1</tt>. */
  VCEncPictureArea roi3Area;
  /** \brief ROI 4 for forcing a QP delta or an absolute QP value.
   *  \n This field is valid only if the value of <tt>EWLHwConfig_t.ROI8Support</tt> is <tt>1</tt>. */
  VCEncPictureArea roi4Area;
  /** \brief ROI 5 for forcing a QP delta or an absolute QP value.
   *  \n This field is valid only if the value of <tt>EWLHwConfig_t.ROI8Support</tt> is <tt>1</tt>. */
  VCEncPictureArea roi5Area;
  /** \brief ROI 6 for forcing a QP delta or an absolute QP value.
   *  \n This field is valid only if the value of <tt>EWLHwConfig_t.ROI8Support</tt> is <tt>1</tt>. */
  VCEncPictureArea roi6Area;
  /** \brief ROI 7 for forcing a QP delta or an absolute QP value.
   *  \n This field is valid only if the value of <tt>EWLHwConfig_t.ROI8Support</tt> is <tt>1</tt>. */
  VCEncPictureArea roi7Area;
  /** \brief ROI 8 for forcing a QP delta or an absolute QP value.
   *  \n This field is valid only if the value of <tt>EWLHwConfig_t.ROI8Support</tt> is <tt>1</tt>. */
  VCEncPictureArea roi8Area;
  /** \brief The QP delta for ROI 1.
   *  \n The value range is from <tt>-30</tt> to <tt>0</tt>, inclusive. */
  i32 roi1DeltaQp;
  /** \brief The QP delta for ROI 2.
   *  \n The value range is from <tt>-30</tt> to <tt>0</tt>, inclusive. */
  i32 roi2DeltaQp;
  /** \brief The QP delta for ROI 3.
   *  \n The value range is from <tt>-30</tt> to <tt>0</tt>, inclusive. */
  i32 roi3DeltaQp;
  /** \brief The QP delta for ROI 4.
   *  \n The value range is from <tt>-30</tt> to <tt>0</tt>, inclusive. */
  i32 roi4DeltaQp;
  /** \brief The QP delta for ROI 5.
   *  \n The value range is from <tt>-30</tt> to <tt>0</tt>, inclusive. */
  i32 roi5DeltaQp;
  /** \brief The QP delta for ROI 6.
   *  \n The value range is from <tt>-30</tt> to <tt>0</tt>, inclusive. */
  i32 roi6DeltaQp;
  /** \brief The QP delta for ROI 7.
   *  \n The value range is from <tt>-30</tt> to <tt>0</tt>, inclusive. */
  i32 roi7DeltaQp;
  /** \brief The QP delta for ROI 8.
   *  \n The value range is from <tt>-30</tt> to <tt>0</tt>, inclusive. */
  i32 roi8DeltaQp;
  /** \brief The absolute QP value for ROI 1.
   *  \n The value range is from <tt>-1</tt> to <tt>51</tt>, inclusive. A value of <tt>-1</tt>
   *  indicates that the absolute QP value is not used. */
  i32 roi1Qp;
  /** \brief The absolute QP value for ROI 2.
   *  \n The value range is from <tt>-1</tt> to <tt>51</tt>, inclusive. A value of <tt>-1</tt>
   *  indicates that the absolute QP value is not used. */
  i32 roi2Qp;
  /** \brief The absolute QP value for ROI 3.
   *  \n The value range is from <tt>-1</tt> to <tt>51</tt>, inclusive. A value of <tt>-1</tt>
   *  indicates that the absolute QP value is not used. */
  i32 roi3Qp;
  /** \brief The absolute QP value for ROI 4.
   *  \n The value range is from <tt>-1</tt> to <tt>51</tt>, inclusive. A value of <tt>-1</tt>
   *  indicates that the absolute QP value is not used. */
  i32 roi4Qp;
  /** \brief The absolute QP value for ROI 5.
   *  \n The value range is from <tt>-1</tt> to <tt>51</tt>, inclusive. A value of <tt>-1</tt>
   *  indicates that the absolute QP value is not used. */
  i32 roi5Qp;
  /** \brief The absolute QP value for ROI 6.
   *  \n The value range is from <tt>-1</tt> to <tt>51</tt>, inclusive. A value of <tt>-1</tt>
   *  indicates that the absolute QP value is not used. */
  i32 roi6Qp;
  /** \brief The absolute QP value for ROI 7.
   *  \n The value range is from <tt>-1</tt> to <tt>51</tt>, inclusive. A value of <tt>-1</tt>
   *  indicates that the absolute QP value is not used. */
  i32 roi7Qp;
  /** \brief The absolute QP value for ROI 8.
   *  \n The value range is from <tt>-1</tt> to <tt>51</tt>, inclusive. A value of <tt>-1</tt>
   *  indicates that the absolute QP value is not used. */
  i32 roi8Qp;
  /** \brief The field order for HEVC (H.265) interlaced coding.
   *  \n <tt>0</tt>: bottom field first.
   *  \n <tt>1</tt>: top field first. */
  u32 fieldOrder;
  /** \brief The chroma QP offset.
   *  \n The value range is from <tt>-12</tt> to <tt>12</tt>, inclusive. */
  i32 chroma_qp_offset;
  /** \brief Whether to enable the ROI map for forcing QP delta or absolute QP values.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 roiMapDeltaQpEnable;
  /** \brief The size of the unit block for the QP map.
   *  \n <tt>0</tt>: 64x64.
   *  \n <tt>1</tt>: 32x32.
   *  \n <tt>2</tt>: 16x16.
   *  \n <tt>3</tt>: 8x8. */
  u32 roiMapDeltaQpBlockUnit;
  /** \brief Whether to enable the ROI map for forcing IPCM.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 ipcmMapEnable;
  /** \brief Whether to enable the index for the CU control map.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 RoimapCuCtrl_index_enable;
  /** \brief (Reserved) Whether to enable the CU control map.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 RoimapCuCtrl_enable;
  /** \brief The version of the CU information format used in the CU control map. */
  u32 RoimapCuCtrl_ver;
  /** \brief The version of the QP information format used in the QP map. */
  u32 RoiQpDelta_ver;
  /** \brief Whether to enable the skip-mode ROI map.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 skipMapEnable;
  /** \brief Whether to enable the RDOQ map.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 rdoqMapEnable;
  /** \brief Region 0 for SSE information statistics.
   *  \n This field is valid only if the value of <tt>VCEncConfig.enableFrameInfo</tt> is <tt>1</tt>. */
  VCEncPictureArea rect0;
  /** \brief Region 1 for SSE information statistics.
   *  \n This field is valid only if the value of <tt>VCEncConfig.enableFrameInfo</tt> is <tt>1</tt>. */
  VCEncPictureArea rect1;
  /** \brief Region 2 for SSE information statistics.
   *  \n This field is valid only if the value of <tt>VCEncConfig.enableFrameInfo</tt> is <tt>1</tt>. */
  VCEncPictureArea rect2;
  /** \brief Region 3 for SSE information statistics.
   *  \n This field is valid only if the value of <tt>VCEncConfig.enableFrameInfo</tt> is <tt>1</tt>. */
  VCEncPictureArea rect3;
  /** \brief Region 4 for SSE information statistics.
   *  \n This field is valid only if the value of <tt>VCEncConfig.enableFrameInfo</tt> is <tt>1</tt>. */
  VCEncPictureArea rect4;
  /** \brief Region 5 for SSE information statistics.
   *  \n This field is valid only if the value of <tt>VCEncConfig.enableFrameInfo</tt> is <tt>1</tt>. */
  VCEncPictureArea rect5;
  /** \brief Region 6 for SSE information statistics.
   *  \n This field is valid only if the value of <tt>VCEncConfig.enableFrameInfo</tt> is <tt>1</tt>. */
  VCEncPictureArea rect6;
  /** \brief Region 7 for SSE information statistics.
   *  \n This field is valid only if the value of <tt>VCEncConfig.enableFrameInfo</tt> is <tt>1</tt>. */
  VCEncPictureArea rect7;

  //wiener denoise parameters
  /** \brief Obsoleted. */
  u32 noiseReductionEnable;
  /** \brief Obsoleted. */
  u32 noiseLow;
  /** \brief Obsoleted. */
  u32 firstFrameSigma;

  // 3DNR
  u32 noiseSigmaEst; /**< \brief 0 = disable noise sigma estimation; 1 = enable noise sigma estimation */
  u32 noiseSigmaY;  /**< \brief valid value range :[1...30] , default: 10 */
  u32 noiseSigmaU;  /**< \brief valid value range :[1...30] , default: 10 */
  u32 noiseSigmaV;  /**< \brief valid value range :[1...30] , default: 10 */
  u32 noiseReductionStrength_IntraY;  /**< \brief valid value range :[0...32] , default: 7 */
  u32 noiseReductionStrength_IntraU;  /**< \brief valid value range :[0...32] , default: 7 */
  u32 noiseReductionStrength_IntraV;  /**< \brief valid value range :[0...32] , default: 7 */
  u32 noiseReductionStrength_InterY;  /**< \brief valid value range :[0...32] , default: 7 */
  u32 noiseReductionStrength_InterU;  /**< \brief valid value range :[0...32] , default: 7 */
  u32 noiseReductionStrength_InterV;  /**< \brief valid value range :[0...32] , default: 7 */
  u32 noiseReduction_ChromaMaxMV;  /**< \brief valid value range :[0...15] , default: 4 */


  /** \brief The duration it takes to perform gradual decoder refresh (GDR), in the unit of frame.
   *  \n The value range is from <tt>0</tt> to <tt>0xFFFFFFFF</tt>, inclusive. If the field value
   *  is <tt>0</tt>, GDR is disabled. */
  u32 gdrDuration;

  /* for low latency */
  /** \brief Whether to enable low-latency encoding.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 inputLineBufEn;
  /** \brief Whether to enable the loopback mode for the input buffer.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 inputLineBufLoopBackEn;
  /** \brief The depth of the input buffer, in the unit of MB line. */
  u32 inputLineBufDepth;
  /** \brief Whether to enable hardware handshake.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 inputLineBufHwModeEn;
  /** \brief The number of handshake synchronizations every loopback. */
  u32 amountPerLoopBack;
  /** \brief The callback function that is called when a line buffer interrupt is detected. */
  EncInputLineBufCallBackFunc inputLineBufCbFunc;
  /** \brief The context to be used by <tt>VCEncCodingCtrl.inputLineBufCbFunc</tt>. */
  void *inputLineBufCbData;
  /** \brief The ID of FLEXA SBI 0. */
  u32 sbi_id_0;
  /** \brief The ID of FLEXA SBI 1. */
  u32 sbi_id_1;
  /** \brief The ID of FLEXA SBI 2.*/
  u32 sbi_id_2;
  /** \brief The height of each FLEXA SBI segment unit, in pixels. */
  u32 segmentUnitHeight;
  /** \brief lowlatency handshake auto gating disable
   *  \n <tt>0</tt>: enable.
   *  \n <tt>1</tt>: disable.    */
  u32 lowlatGatingDisable;
  /** \brief lowlatency handshake auto gating type*/
  u32 lowlatGatingType;
  /** \brief bus idle counter max value*/
  u32 lowlatGatingCyc;
  /** \brief disable TU32. */

  /** \brief Whether to enable slice interrupts.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable.
   *  \n The callback function <tt>VCEncSliceReadyCallBackFunc()</tt> can be called only if
   *  slice interrupts and slice encoding are enabled. */
  u32 enable_slice_irq;

  /*stream multi-segment*/
  /** \brief (Reserved) The multi-segment mode of output stream buffers.
   *  \n <tt>0</tt>: single-segment.
   *  \n <tt>1</tt>: multi-segment with no synchronization.
   *  \n <tt>2</tt>: multi-segment with software handshake. */
  u32 streamMultiSegmentMode;
  /** \brief (Reserved) The number of segments in a stream output buffer.
   *  \n The field value is greater than or equal to <tt>2</tt>. */
  u32 streamMultiSegmentAmount;

  /** \brief (Reserved) The callback function that is called when a stream segment interrupt
   *  is detected. */
  EncStreamMultiSegCallBackFunc streamMultiSegCbFunc;
  /** \brief (Reserved) The context to be used by <tt>VCEncCodingCtrl.streamMultiSegCbFunc</tt>. */
  void *streamMultiSegCbData;

  /** \brief Batch Mode Control Parameters. only work when using VCMD.
   * In batch mode, software will cache the encoding tasks in current instance. The maximum
   * cache size is defined as parallelCoreNum. When accumulate task to this value, software
   * will drive the hardware to run these tasks in batch way. */
  u32 batchCount;

  /* for smart (obsolete) */
  /** \brief Obsoleted. */
  i32 smartModeEnable;
  /** \brief Obsoleted. */
  i32 smartH264Qp;
  /** \brief Obsoleted. */
  i32 smartHevcLumQp;
  /** \brief Obsoleted. */
  i32 smartHevcChrQp;
  /** \brief Obsoleted. */
  i32 smartH264LumDcTh;
  /** \brief Obsoleted. */
  i32 smartH264CbDcTh;
  /** \brief Obsoleted. */
  i32 smartH264CrDcTh;

  /* threshold for hevc cu8x8/16x16/32x32 */
  /** \brief Obsoleted. */
  i32 smartHevcLumDcTh[3];
  /** \brief Obsoleted. */
  i32 smartHevcChrDcTh[3];
  /** \brief Obsoleted. */
  i32 smartHevcLumAcNumTh[3];
  /** \brief Obsoleted. */
  i32 smartHevcChrAcNumTh[3];

  /* back ground */
  /** \brief Obsoleted. */
  i32 smartMeanTh[4];

  /* foreground/background threashold: maximum foreground pixels in background block */
  /** \brief Obsoleted. */
  i32 smartPixNumCntTh;

  /* for ctbRc v2*/
  /** \brief Negative qp delta of skin area. */
  u32 ctbRcSkinQPDelta;
  /** \brief Skin area not set too small QPDelta(complexity + edge + skin),
   * which use ctbRcSkinMinQPDelta - direction to limit it.
   * ctbRcSkinMinQPDelta - direction always less than 0.*/
  u32 ctbRcSkinMinQPDelta;
  /* Range be regarded as skin area, otherwize use default algorithm to detect skin area.*/
  /** \brief  Min cb value to be regarded as skin Area. */
  u32 ctbRcSkinCbMin;
  /** \brief Max cb value to be regarded as skin Area. */
  u32 ctbRcSkinCbMax;
  /** \brief Min cr value to be regarded as skin Area. */
  u32 ctbRcSkinCrMin;
  /** \brief Max cr value to be regarded as skin Area. */
  u32 ctbRcSkinCrMax;
  /** \brief Min lum value to be regarded as skin Area. */
  u32 ctbRcSkinLumMin;
  /** \brief Max lum value to be regarded as skin Area. */
  u32 ctbRcSkinLumMax;
  /** \brief Skin Grad threshold for skin detection. */
  u32 ctbRcSkinGradTh;
  /** \brief Trailing area detection max strength. */
  u32 ctbRcTrailStrengthMax;
  /** \brief Trailing area base qp delta. */
  i32 ctbRcTrailDeltaQp;
  /** \brief Strength to calculate edge threshold. The texture will be
   * more easily regarded as edge with bigger value  */
  u32 ctbRcEdgeTh;
  /** \brief Edge area not set too big QPDelta(complexity + edge),
   * which use ctbRcEdgeMaxQPDelta - direction to limit it. */
  u32 ctbRcEdgeMaxQpDelta;
  /** \brief Subscript of ctbRcThresholdI/ctbRcThresholdP/ctbRcThresholdB,
   * It used with ctbRcThreshold to decrease/increase block QP.
   * When the block complexity is between ctbRcThreshold[0] and
   * ctbRcThreshold[Direction] will decrease QP, and the
   * complexity is bigger than ctbRcThreshold[Direction] will
   * increase QP.                                          */
  u32 ctbRcDirection;
  /** \brief A threshold array, it used with ctbRcDirection to
   *  decrease/increase block QP of Intra frame */
  u32 ctbRcThresholdI[16];
  /** \brief A threshold array, it used with ctbRcDirection to
   *  decrease/increase block QP of P frame  */
  u32 ctbRcThresholdP[16];
  /** \brief A threshold array, it used with ctbRcDirection to
   *  decrease/increase block QP of B frame  */
  u32 ctbRcThresholdB[16];

  /** \visual quality prior mode decision*/
  /** \brief Subjective prefer cause bitrate increasment tolerance */
  i32 visualBitRateTolerance;
  /** \brief Chroma error check strength for subjective quality
    * prior mode decision */
  u32 IntraBiasChromaStrength;
  /** \brief Intra mode bias strength for subjective quality
    * prior mode decision */
  u32 IntraBiasStrength;
  /** \brief Min MV threshold for Intra mode bias */
  u32 IntraBiasMvThreshold;
  /** \brief Whether intraBias is avoided in trail area */
  u32 bTrailAvoidIntraBias;

  /* for T35 */
  /** \brief The T.35 code and payload data information for HDR10. */
  ITU_T_T35 itu_t_t35;

  /* for HDR10 */
  /** \brief Whether to write HDR10 information only before the first IDR frame.
    * \n <tt>0</tt>: write before each IDR frame.
    * \n <tt>1</tt>: write only before the first IDR frame. */
  u32 write_once_HDR10;
  /** \brief The colume information of the mastering display for HDR10. */
  Hdr10DisplaySei Hdr10Display;
  /** \brief The HDR10 light-level information. */
  Hdr10LightLevelSei Hdr10LightLevel;

  /** \brief The color description in the VUI.
   *  \n For details, see Section <em> \ref ss_vui</em>. */
  VuiColorDescription vuiColorDescription;
  /** \brief Whether to present the video signal type in the VUI.
    * \n <tt>0</tt> (default): do not present.
    * \n <tt>1</tt>: present.
    * \n For more information about VUI, see Section <em> \ref ss_vui</em>. */
  u32 vuiVideoSignalTypePresentFlag;
  /** \brief The video format in the VUI. For details, see Section <em> \ref ss_vui</em>.
    * \n <tt>0</tt>: component.
    * \n <tt>1</tt>: PAL.
    * \n <tt>2</tt>: NTSC.
    * \n <tt>3</tt>: SECAM.
    * \n <tt>4</tt>: MAC.
    * \n <tt>5</tt> (default): UNDEF. */
  u32 vuiVideoFormat;
  /** \brief (For HEVC only) Whether to specify the RPS in the slice header instead of
   *  using the pre-defined RPS from the SPS.
   *  \n <tt>0</tt>: use the pre-defined RPS from the SPS.
    * \n <tt>1</tt>: specify the RPS in the slice header. */
  u32 RpsInSliceHeader;
  /** \brief Whether to enable rate-distortion optimized quantization (RDOQ). For details,
   *  see Section <em> \ref ss_rdoq</em>.
    * \n <tt>0</tt>: disable.
    * \n <tt>1</tt>: enable. */
  u32 enableRdoQuant;
  /** \brief Whether to enable dynamic RDO level selection. For details, see Section
   *  <em> \ref ss_tune</em>.
    * \n <tt>0</tt>: disable.
    * \n <tt>1</tt>: enable. */
  u32 enableDynamicRdo;
  /** \brief The bias used for 16x16 CU cost calculation in dynamic RDO level selection. */
  u32 dynamicRdoCu16Bias;
  /** \brief The factor used for 16x16 CU cost calculation in dynamic RDO level selection. */
  u32 dynamicRdoCu16Factor;
  /** \brief The bias used for 32x32 CU cost calculation in dynamic RDO level selection. */
  u32 dynamicRdoCu32Bias;
  /** \brief The factor used for 32x32 CU cost calculation in dynamic RDO level selection. */
  u32 dynamicRdoCu32Factor;
  /** \brief The ME vertical search range, in pixels.
   *  \n The value must be less than or equal to the maximum vertical search range allowed,
   *  which can be queried through <tt>EWLHwConfig_t.meVertSearchRangeHEVC</tt> or
   *  <tt>EWLHwConfig_t.meVertSearchRangeH264</tt>. */
  u32 meVertSearchRange;
  /** \brief (Only for H.264) Whether to enable layer information in the <tt>nal_ref_idc</tt>
   *  syntax element.
    * \n <tt>0</tt>: disable. In this case, the value of nal_ref_idc can be 0 or 1.
    * \n <tt>1</tt>: enable. In this case, the value of nal_ref_idc can be 0 to 3.
   *  \n The feature is supported only for specific custom versions. */
  u32 layerInRefIdcEnable;
  /** \brief (Only for H.264) Whether to add prefix NAL units between slices for SVC-T.
    * \n <tt>0</tt>: do not add.
    * \n <tt>1</tt>: add. */
  u32 prefixNalSvcFlag;
  /** \brief (Only for H.264) Whether to enable SVC-T.
    * \n <tt>0</tt>: disable.
    * \n <tt>1</tt>: enable. */
  u32 svctEnable;
  /** \brief The adaptive quantization mode. For details, see Section <em> \ref ss_aqmode</em>.
   *  \n <tt>0</tt> (default): none.
   *  \n <tt>1</tt>: uniform AQ.
   *  \n <tt>2</tt>: auto variance.
   *  \n <tt>3</tt>: auto variance with bias to dark scenes. */
  u32 aq_mode;
  /** \brief The strength of adaptive quantization. A great value reduces blocking and
   *  blurring in flat and textured areas. For details, see Section <em> \ref ss_aqmode</em>.
   *  \n The value range is from <tt>0.0</tt> to <tt>3.0</tt>, inclusive. The default value
   *  is <tt>1.0</tt>. */
  float aq_strength;
  /** \brief The strength of psycho-visual encoding.
   *  \n The value range is <tt>0</tt> to <tt>4</tt>, inclusive. If the field value is
   *  <tt>0</tt>, physcho-visual encoding is disabled. Otherwise, the greater the field
   *  value, the better the subjective quality. */
  float psyFactor;
  /** \brief Whether to enable polling line input.
   * \n <tt>0</tt>: disable.
   * \n <tt>1</tt>: enable. */
  /** \brief Whether to enable intraRecon.
   * \n <tt>0</tt>: disable.
   * \n <tt>1</tt>: enable. */
  u32 intraReconEnable;
  u32 inputSliceInfoPollEn;
  u32 sw_skip_flag;
} VCEncCodingCtrl;

/** @} */

/**
   * \defgroup api_rc Bit Rate Control API
   *
   * @{
   */
/** \brief Contains bit rate control parameters. */
typedef struct {
  /** \brief The constant rate factor for CRF rate control mode.
   *  \n The value range is from <tt>0</tt> to <tt>51</tt>, inclusive. */
  i32 crf;
  /** \brief Whether to enable picture-level rate control to adjust QP between pictures.
   *  \n <tt>0</tt> (default): disable.
   *  \n <tt>1</tt>: enable.
   *  \n Set this field to <tt>0</tt> if a target bit rate is specified. */
  u32 pictureRc;
  /** \brief The block-level rate control mode for adjusting QP inside a frame.
   *  \n <tt>0</tt>: block-level rate control disabled.
   *  \n <tt>1</tt>: subjective rate control only.
   *  \n <tt>2</tt>: precise rate control only.
   *  \n <tt>3</tt>: mixed subjective and precise rate control. */  //CTB_RC
  u32 ctbRc;
  /** \brief The block size for block-level rate control.
   *  \n <tt>0</tt>: 64x64 pixels.
   *  \n <tt>1</tt>: 32x32 pixels.
   *  \n <tt>2</tt>: 16x16 pixels. */
  u32 blockRCSize;
  /** \brief Whether to allow skipping pictures for bit rate control.
   *  \n <tt>0</tt> (default): disallow.
   *  \n <tt>1</tt>: allow. */
  u32 pictureSkip;
  /** \brief The default or initial picture-level QP value.
   *  \n - If picture-level rate control is disabled, this QP value is used as the default
   *  QP value.
   *  \n - In other cases, this QP value is used as the initial QP value for the first
   *  encoded picture.
   *  \n When block-level rate control is enabled, this QP value is used as the base QP value.
   *  The encoder calculates delta values based on this base QP value.
   *  \n The value range is from <tt>-1</tt> to <tt>51</tt>, inclusive. Values 0 to 10 are
   *  not recommended.
   *  \n The default value is <tt>-1</tt>, which indicates that the encoder calculates the
   *  default or initial QP value according to the target bit rate. */
  i32 qpHdr;
  /** \brief The minimum QP for P- and B-pictures.
   *  \n The value range is from <tt>0</tt> to <tt>51</tt>, inclusive. The default value is
   *  <tt>0</tt>. A value less than <tt>10</tt> is not recommended. */
  u32 qpMinPB;
  /** \brief The maximum QP for P- and B-pictures.
   *  \n The value range is from <tt>0</tt> to <tt>51</tt>, inclusive. */
  u32 qpMaxPB;
  /** \brief The minimum QP for I-pictures.
   *  \n The value range is from <tt>0</tt> to <tt>51</tt>, inclusive. The default value is
   *  <tt>0</tt>. A value less than <tt>10</tt> is not recommended. */
  u32 qpMinI;
  /** \brief The maximum QP for I-pictures.
   *  \n The value range is from <tt>0</tt> to <tt>51</tt>, inclusive. */
  u32 qpMaxI;
  /** \brief The target bit rate, in bits per second.
   *  \n Set this field if picture-level rate control is enabled.
   *  \n The value must be greater than or equal to <tt>10000</tt>. If HRD is enabled, the
   *  maximum target bit rate is limited by the stream tier and level selected during encoder
   *  initialization. For detailed limitations, see ITU-T Rec. H.265. */
  u32 bitPerSecond;
  /** \brief The maximum bit rate of the coded picture buffer (CPB), in bits per second.
   *  \n Set this field if the CBR or VBR mode is used. */
  u32 cpbMaxRate;
  /** \brief Whether to enable filler data when HRD is disabled.
   *  \n <tt>false</tt>: disable.
   *  \n <tt>true</tt>: enable. */
  true_e fillerData;
  /** \brief Whether to enable the Hypothetical Reference Decoder (HRD) model to restrict the
   *  instantaneous bit rate and the total number of bits per coded picture.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable.
   *  \n Enabling HRD triggers picture-level rate control to be automatically enabled and
   *  imposes tight constraints on the operation of the HRD model. */
  u32 hrd;
  /** \brief The size of the coded picture buffer (CPB) in bits.
   *  \n When HRD is enabled, each encoded frame cannot be larger than the specified size.
   *  \n By default, the video encoder uses the maximum size allowed by the stream level.
   *  Setting this field to <tt>0</tt> restores the default value. A value of (2 *
   *  <tt>VCEncRateCtrl.bitPerSecond</tt>) is recommended. */
  u32 hrdCpbSize;
  /** \brief The number of frames within which the rate control algorithm tries to achieve
   *  the target bit rate.
   *  \n The value range is from <tt>1</tt> to <tt>300</tt>, inclusive. The default value is
   *  <tt>VCEncGopConfig.idr_interval</tt>. Recommended values vary with video use cases.
   *  \n The rate control algorithm uses the GOP length to match the average bit rate of each
   *  GOP to the target bit rate. */
  u32 bitrateWindow;
  /** \brief The delta between QP and intra QP values.
   *  \n The intra QP delta can be used to reduce intra flashing.
   *  \n The value range is from <tt>-12</tt> to <tt>12</tt>, inclusive.
   *  \n - A smaller QP value increases the relative quality of the intra frame.
   *  \n - A larger QP value lowers the size of encoded intra frames.
   *  \n This field is invalid if a fixed QP is specified through <tt>VCEncRateCtrl.fixedIntraQp</tt>. */
  i32 intraQpDelta;
  /** \brief The fixed QP value for all intra frames.
   *  \n The value range is from <tt>0</tt> to <tt>51</tt>, inclusive. If the field value is
   *  <tt>0</tt>, the encoder automatically calculates the intra QP value. */
  u32 fixedIntraQp;
  /** \brief Obsoleted. */
  i32 bitVarRangeI;
  /** \brief The permitting variance percentage over average bits per P-frame from the target
   *  bit rate.
   *  \n The value range is from <tt>10</tt> to <tt>10000</tt>, inclusive. The default value
   *  is <tt>10000</tt>, which indicates no limitation on the P-frame size. */
  i32 bitVarRangeP;
  /** \brief The permitting variance percentage over average bits per B-frame from the target
   *  bit rate.
   *  \n The value range is from <tt>10</tt> to <tt>10000</tt>, inclusive. The default value
   *  is <tt>10000</tt>, which indicates no limitation on the B-frame size. */
  i32 bitVarRangeB;
  /** \brief The tolerance percentage of the maximum moving bit rate over the target bit rate,
   *  which means that the average bit rate of the monitored frames is limited to
   *  [<tt>VCEncRateCtrl.bitPerSecond</tt> * (1 + <tt>VCEncRateCtrl.tolMovingBitRate</tt>/100)].
   *  \n The value range is from <tt>0</tt> to <tt>2000</tt>, inclusive. The default value is
   *  <tt>100</tt>, which indicates the moving bit rate can tolerate maximal 100% over the
   *  target bit rate. */
  i32 tolMovingBitRate;
  /** \brief The number of frames to be monitored for calculating the moving bit rate.
   *  \n The value range is from <tt>10</tt> to <tt>120</tt>, inclusive. */
  i32 monitorFrames;
  /** \brief The target size of the picture output by the block-level rate control algorithm. */
  i32 targetPicSize;
  /** \brief Obsoleted. */
  i32 smoothPsnrInGOP;
  /** \brief The percentage of I-frame bits in static scenes. */
  u32 u32StaticSceneIbitPercent;
  /** \brief The maximum absolute delta between block-level and picture-level QP values.
   *  \n If the value of <tt>EWLHwConfig_t.CtbRcVersion</tt> is less than 1, the option value
   *  \n range is from <tt>0</tt> to <tt>15</tt>, inclusive.
   *  \n If the value of <tt>EWLHwConfig_t.CtbRcVersion</tt> is greater than or equal to 1,
   *  \n the option value range is from <tt>0<tt> to <tt>51</tt>, inclusive. */
  u32 rcQpDeltaRange;
  /** \brief The MB complexity threshold for subjective block-level rate control.
   *  \n When the complexity equals the threshold, the block-level QP is not adjusted. The QP
   *  value increases when the complexity exceeds the threshold, and vice versa. */
  u32 rcBaseMBComplexity;
  /** \brief The minimum QP delta in picture-level rate control.
   *  \n The value range is from <tt>-10</tt> to <tt>-1</tt>, inclusive. The default value is
   *  <tt>-2</tt>. */
  i32 picQpDeltaMin;
  /** \brief The maximum QP delta in picture-level rate control.
   *  \n The value range is from <tt>1</tt> to <tt>10</tt>, inclusive. The default value is
   *  <tt>3</tt>. */
  i32 picQpDeltaMax;
  /** \brief The QP delta of frames using long-term reference (LTR).
   *  \n The value range is from <tt>-51</tt> to <tt>51</tt>, inclusive. The default value is
   *  <tt>0</tt>. */
  i32 longTermQpDelta;
  /** \brief Whether to enable variable bit rate control based on the minimum QP allowed.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  i32 vbr;
  /** \brief The rate control mode.
   *  \n By default, the CVBR mode is used. */
  rcMode_e rcMode;
  /** \brief The tolerance of block-level rate control for inter frames, which defaults to
   *  <tt>-1.0</tt>.
   *  \n The rate control algorithm tries to limit inter frame bits within range from
   *  <tt>targetPicSize</tt>/(1 + <tt>tolCtbRcInter</tt>) to <tt>targetPicSize</tt> * (1 + <tt>tolCtbRcInter</tt>).
   *  \n A negative <tt>tolCtbRcInter</tt> value indicates no bit rate limit for inter frames in
   *  block-level rate control. */
  float tolCtbRcInter;
  /** \brief The tolerance of block-level rate control for intra frames, which defaults to
   *  <tt>-1.0</tt>.
   *  \n The rate control algorithm tries to limit inter frame bits within range from
   *  <tt>targetPicSize</tt>/(1 + <tt>tolCtbRcIntra</tt>) to <tt>targetPicSize</tt> * (1 + <tt>tolCtbRcIntra</tt>).
   *  \n A negative <tt>tolCtbRcIntra</tt> value indicates no bit rate limit for intra frames in
   *  block-level rate control. */
  float tolCtbRcIntra;
  /**< \brief Tolerance of frame RC controlling underflow bitrate for better quality */
  i32 tolRcUnderflow;
  /** \brief The ratio of the maximum size of intra frames to inter frames. */
  u32 maxIprop;
  /** \brief The ratio of the minimum size of intra frames to inter frames. */
  u32 minIprop;
  /** \brief ratio of bitrate to the maximun bitrate at initial of the qp adjustment*/
  i32 changePos;
  /** \brief The maximum accumulated QP adjustment step per CTB/MB row allowed by block-level rate
   *  control. */
  i32 ctbRcRowQpStep;
  /** \brief	max QP delta adjustment based on picQP by Ctb Rate Control*/
  i32 ctbRcRowQpDeltaRange;
  /** \brief Whether to reverse the QP adjustment direction for subjective block-level rate control.
   *  \n <tt>0</tt>: do not reverse.
   *  \n <tt>1</tt>: reverse. */
  u32 ctbRcQpDeltaReverse;
  /** \brief The numerator for calculating the target frame rate.
   *  \n The value range is from <tt>1</tt> to <tt>1048575</tt>, inclusive. The default value is
   *  <tt>30</tt>.
   *  \n It is recommended to set this field to the same value as <tt>VCEncConfig.frameRateNum</tt>. */
  u32 frameRateNum;
  /** \brief The denominator for calculating the target frame rate.
   *  \n The value range is from <tt>1</tt> to <tt>VCEncRateCtrl.frameRateNum</tt>, inclusive.
   *  The default value is <tt>1</tt>.
   *  \n It is recommended to set this field to the same value as <tt>VCEncConfig.frameRateDenom</tt>. */
  u32 frameRateDenom;
  u32 hieQpDeltaEnable; /**< \brief GOP Hierarchical frame QP delta Enable */
} VCEncRateCtrl;

/** @} */ /* end of def api_rc*/

/**
  * \addtogroup api_video
  *
  * @{
  */

/** \brief Defines the external SEI information. */
typedef struct {
  /** \brief The type of the external SEI NAL unit.
   *  \n For HEVC (H.265), the field value can be <tt>39</tt> or <tt>40</tt> where <tt>39</tt>
   *  indicates <tt>PREFIX_SEI_NUT</tt> and <tt>40</tt> indicates <tt>SUFFIX_SEI_NUT</tt>.
   *  \n For H.264 (AVC), the field can be left unspecified and the value is fixed to <tt>6</tt>.
   *  \n For details about the field values, see ITU-T Rec. H.264. */
  u8 nalType;
  /** \brief The payload type of the external SEI.
   *  \n For example, the field value is <tt>1</tt> for picture timing.
   *  \n For details, see Annex D.2.1 in ITU-T Rec. H.265 for HEVC (H.265) or Annex D.1.1 in
   *  ITU-T Rec. H.264 for H.264 (AVC). */
  u8 payloadType;
  /** \brief The payload data length of the external SEI. */
  u32 payloadDataSize;
  /** \brief The payload data of the external SEI. */
  u8 *pPayloadData;
  /** \brief The frame ID of the external SEI. */
  u32 frame_id;
} ExternalSEI;

/** \brief Defines input buffers and output stream buffers for the second tile to the last tile
 *  in multi-tile scenarios. */
typedef struct {
  /** \brief The bus address of an input picture buffer of the video encoder.
   *  \n This field specifies the bus address of the Y-plane buffer if the picture data is stored
   *  in a planar or semi-planar YUV format, or the bus address of the buffer that stores the
   *  complete picture data if the picture data is stored in an interleaved YUV or RGB format. */
  ptr_t busLuma;
  /** \brief The bus address of an input picture buffer of the video encoder.
   *  \n This field specifies the bus address of the U-plane buffer if the picture data is stored
   *  in a planar YUV format, \n or the bus address of the UV-plane buffer if the picture data is
   *  stored in a semi-planar YUV format.
   *  \n This field is invalid if the input picture data is stored in an interleaved YUV or RGB format. */
  ptr_t busChromaU;
  /** \brief The bus address of an input picture buffer of the video encoder.
   *  \n This field specifies the bus address of the V-plane buffer if the picture data is stored
   *  in a planar YUV format.
   *  \n This field is invalid if the input picture data is stored in a semi-planar YUV format or
   *  an interleaved YUV or RGB format. */
  ptr_t busChromaV;
  /** \brief The virtual address of each output stream buffer. */
  u32 *pOutBuf[MAX_STRM_BUF_NUM];
  /** \brief The bus address of each output stream buffer. */
  ptr_t busOutBuf[MAX_STRM_BUF_NUM];
  /** \brief The size of each output stream buffer, in bytes. */
  u32 outBufSize[MAX_STRM_BUF_NUM];
  /** \brief A buffer identifier to re-encode when the output buffer overflows.
   *  \n The application needs to check this buffer and replace the output buffer maintained by
   *  the application layer. This function is not used internally by the video encoder. */
  void *cur_out_buffer[MAX_STRM_BUF_NUM];
} VCEncInTileExtra;

/**
 * \brief Defines the input to the video encoder.
 */
typedef struct {
  /** \brief The bus address of an input picture buffer of the video encoder.
   *  \n This field specifies the bus address of the Y-plane buffer if the picture data is
   *  stored in a planar or semi-planar YUV format, or the bus address of the buffer that
   *  store the complete picture data if the picture data is stored in an interleaved YUV
   *  or RGB format. */
  ptr_t busLuma;
  /** \brief The bus address of an input picture buffer of the video encoder.
   *  \n This field specifies the bus address of the U-plane buffer if the picture data is
   *  stored in a planar YUV format, \n or the bus address of the UV-plane buffer if the
   *  picture data is stored in a semi-planar YUV format.
   *  \n This field is invalid if the input picture data is stored in an interleaved YUV
   *  or RGB format. */
  ptr_t busChromaU;
  /** \brief The bus address of an input picture buffer of the video encoder.
   *  \n This field specifies the bus address of the V-plane buffer if the picture data is
   *  stored in a planar YUV format.
   *  \n This field is invalid if the input picture data is stored in a semi-planar YUV
   *  format or an interleaved YUV or RGB format. */
  ptr_t busChromaV;
  /** \brief A bus address of the externally down-scaled picture input to the pass-1 encoder.
   *  \n This field specifies the bus address of the Y-plane buffer if the picture data is
   *  stored in a planar or semi-planar YUV format, or the bus address of the buffer that
   *  store the complete picture data if the picture data is stored in an interleaved YUV
   *  or RGB format. */
  ptr_t busLumaOrig;
  /** \brief A bus address of the externally down-scaled picture input to the pass-1 encoder.
   *  \n This field specifies the bus address of the U-plane buffer if the picture data is
   *  stored in a planar YUV format, \n or the bus address of the UV-plane buffer if the
   *  picture data is stored in a semi-planar YUV format.
   *  \n This field is invalid if the input picture data is stored in an interleaved YUV
   *  or RGB format. */
  ptr_t busChromaUOrig;
  /** \brief A bus address of the externally down-scaled picture input to the pass-1 encoder.
   *  \n This field specifies the bus address of the V-plane buffer if the picture data is
   *  stored in a planar YUV format.
   *  \n This field is invalid if the input picture data is stored in a semi-planar YUV
   *  format or an interleaved YUV or RGB format. */
  ptr_t busChromaVOrig;
  /** \brief The duration of the previous picture, in the unit of 1/FPS.
   *  \n The value is <tt>0</tt> for the first picture, and typically equal to
   *  <tt>frameRateDenom</tt> for the subsequent pictures. */
  u32 timeIncrement;
  /** \brief Whether to write VUI timing information in SPS.
   *  \n <tt>0</tt>: do not write.
   *  \n <tt>1</tt>: write. */
  u32 vui_timing_info_enable;
  /** \brief The virtual address of each output stream buffer. */
  u32 *pOutBuf[MAX_STRM_BUF_NUM];
  /** \brief The bus address of each output stream buffer. */
  ptr_t busOutBuf[MAX_STRM_BUF_NUM];
  /** \brief The size of each output stream buffer, in bytes. */
  u32 outBufSize[MAX_STRM_BUF_NUM];
  /** \brief The addresses of the buffers used in case of output buffer overflow.
   *  \n The application needs to check these buffers, and replace the application layer
   *  maintained output buffers with these buffers if valid output data is detected in
   *  these buffers. */
  void *cur_out_buffer[MAX_STRM_BUF_NUM];
  /** \brief The proposed picture coding type.
   *  \n Valid values include <tt> \ref VCENC_INTRA_FRAME</tt>, <tt> \ref VCENC_PREDICTED_FRAME</tt>,
   *  and <tt> \ref VCENC_BIDIR_PREDICTED_FRAME</tt>. */
  VCEncPictureCodingType codingType;
  /** \brief The bus address of the Y-plane buffer of the next picture for stabilization. For
   *  details, see Section <em> @ref s_videoStab</em>. */
  ptr_t busLumaStab;

  //VCEncPicConfig picConfig;
  /** \brief The picture order count (POC). */
  i32 poc;
  /** \brief The configurations of the current GOP structure. */
  VCEncGopConfig gopConfig;
  /** \brief The size of the current GOP structure. */
  i32 gopSize;
  /** \brief The encoded order count of the current picture within the GOP structure.
   *  \n The value range is from 0 to (<tt>gopSize</tt> - 1), inclusive. */
  i32 gopPicIdx;

  /** \brief The virtual address of the ROI map buffer. */
  i8 *pRoiMapDelta;
  /** \brief The ROI map size. */
  u32 roiMapDeltaSize;
  /** \brief The bus address of the ROI map buffer. */
  ptr_t roiMapDeltaQpAddr;
  /** \brief The bus address of the CU control map buffer. */
  ptr_t RoimapCuCtrlAddr;
  /** \brief Obsoleted. */
  ptr_t RoimapCuCtrlIndexAddr;
  /** \brief The base bus address of the slice information for low-latency encoding. */
  ptr_t SliceInfo_Base;
  /** \brief The base bus address of external SRAM for luma backward reference.
   *  \n This field is valid only if external SRAM is enabled to save motion estimation
   *  (ME) bandwidth. */
  ptr_t extSRAMLumBwdBase;
  /** \brief The base bus address of external SRAM for luma forward reference.
   *  \n This field is valid only if external SRAM is enabled to save ME bandwidth. */
  ptr_t extSRAMLumFwdBase;
  /** \brief The base bus address of external SRAM for chroma backward reference.
   *  \n This field is valid only if external SRAM is enabled to save ME bandwidth. */
  ptr_t extSRAMChrBwdBase;
  /** \brief The base bus address of external SRAM for chroma forward reference.
   *  \n This field is valid only if external SRAM is enabled to save ME bandwidth. */
  ptr_t extSRAMChrFwdBase;
  /** \brief The base bus address of the compression table.
   *  \n This field specifies the bus address of the Y-plane buffer if the input picture
   *  data is stored in a planar or semi-planar YUV format, or the bus address of the
   *  buffer that stores the complete picture data if the input picture data is stored in
   *  an interleaved YUV or RGB format.
   *  \n This field is valid only if DEC400 is used to decompress the input picture. */
  ptr_t dec400LumTableBase;
  /** \brief The base bus address of the compression table.
   *  \n This field specifies the bus address of the U-plane buffer if the input picture
   *  data is stored in a planar YUV format, \n or the bus address of the UV-plane buffer
   *  if the input picture data is stored in a semi-planar YUV format.
   *  \n This field is invalid if the input picture data is stored in an interleaved YUV
   *  or RGB format.
   *  \n This field is valid only if DEC400 is used to decompress the input picture. */
  ptr_t dec400CbTableBase;
  /** \brief The base bus address of the compression table.
   *  \n This field specifies the bus address of the V-plane buffer if the input picture
   *  data is stored in a planar YUV format.
   *  \n This field is invalid if the input picture data is stored in a semi-planar YUV
   *  format or an interleaved YUV or RGB format.
   *  \n This field is valid only if DEC400 is used to decompress the input picture. */
  ptr_t dec400CrTableBase;
  /** \brief The encoded picture GOP count. */
  i32 picture_gopIdx;
  /** \brief The encoded picture count of the current picture. */
  i32 picture_cnt;
  /** \brief The encoded picture count of the next picture. */
  i32 picture_cnt_next;
  /** \brief The encoded picture count of the last IDR frame. */
  i32 last_idr_picture_cnt;

  /* for low latency */
  /** \brief The number of MB lines that have been fed to the input MB line buffer. */
  u32 lineBufWrCnt;
  /** \brief The number of input data segments stored for SBI. */
  u32 initSegNum;

  /* for crc32/checksum */
  /** \brief The method to calculate the hash value of the output stream.
   *  \n <tt>0</tt>: hash value calculation disabled.
   *  \n <tt>1</tt>: standard CRC32.
   *  \n <tt>2</tt>: 32-bit checksum. */
  u32 hashType;

  /** \brief Whether the current frame is a scene change frame. For details, see Section
   *  <em> @ref sss_extern_detect</em>. */
  u32 sceneChange;
  /** \brief The global motion vectors (MVs). */
  i16 gmv[2][2];
  /** \brief Whether to encode the current picture as an IDR frame, after which a new GOP
   *  is followed.
   *  \n <tt>0</tt>: do not encode as an IDR frame.
   *  \n <tt>1</tt>: encode as an IDR frame. */
  u32 bIsIDR;
  /** \brief Whether to encode the current picture as a non-IDR intra frame.
   *  \n <tt>0</tt>: do not encode as a non-IDR intra frame.
   *  \n <tt>1</tt>: encode as a non-IDR intra frame. */
  u32 bIsIntraOnly;

  /* long-term reference relatived info */
  /** \brief Whether to periodically use long-term reference (LTR).
   *  \n <tt>0</tt>: do not periodically use.
   *  \n <tt>1</tt>: periodically use. */
  u32 bIsPeriodUsingLTR;
  /** \brief Whether to periodically update LTR frames.
   *  \n <tt>0</tt>: do not periodically update.
   *  \n <tt>1</tt>: periodically update. */
  u32 bIsPeriodUpdateLTR;
  /** \brief The reference descriptions of the current picture. */
  VCEncGopPicConfig gopCurrPicConfig;
  /** \brief The reference descriptions of the next picture. */
  VCEncGopPicConfig gopNextPicConfig;
  /** \brief The POC of each LTR frame.
   *  \n A value greater than or equal to <tt>0</tt> indicates the POC of an LTR frame.
   *  \n A value of <tt>-1</tt> is indicates that the frame is not an LTR frame.*/
  i32 long_term_ref_pic[VCENC_MAX_LT_REF_FRAMES];
  /** \brief Whether the current picture uses each LTR frame.
   *  \n <tt>0</tt>: does not use.
   *  \n <tt>1</tt>: uses. */
  u32 bLTR_used_by_cur[VCENC_MAX_LT_REF_FRAMES];
  /** \brief Whether to update the POC of each LTR frame after encoding the current frame.
   *  \n <tt>0</tt>: do not update.
   *  \n <tt>1</tt>: update. */
  u32 bLTR_need_update[VCENC_MAX_LT_REF_FRAMES];
  /** \brief The special RPS index used by the current picture.
   *  \n The field value is greater than or equal to <tt>-1</tt>.
   *  \n If the field value is <tt>-1</tt>, it indicates that the current picture does not
   *  use special RPS index. */
  i8 i8SpecialRpsIdx;
  /** \brief (For H.264 only) The special RPS index used by the next picture.
   *  \n The field value is greater than or equal to <tt>-1</tt>.
   *  \n If the field value is <tt>-1</tt>, it indicates that the next picture does not use
   *  special RPS index. */
  i8 i8SpecialRpsIdx_next;
  /** \brief The LTR index of the current frame if it is encoded as an LTR frame.
   *  \n The value range is from <tt>0</tt> to <tt> @ref VCENC_MAX_LT_REF_FRAMES</tt>, inclusive.
   *  \n If the field value is <tt>0</tt>, it indicates that the current frame is not encoded
   *  as an LTR frame. */
  u8 u8IdxEncodedAsLTR;

  /** \brief Whether to re-send the video parameter set (VPS). For details, see Section
   *  <em> @ref ss_resendHeaders</em>.
   *  \n <tt>0</tt>: do not re-send.
   *  \n <tt>1</tt>: re-send. */
  u32 resendVPS;
  /** \brief Whether to re-send the sequence parameter set (SPS). For details, see Section
   *  <em> @ref ss_resendHeaders</em>.
   *  \n <tt>0</tt>: do not re-send.
   *  \n <tt>1</tt>: re-send. */
  u32 resendSPS;
  /** \brief Whether to re-send the picture parameter set (PPS). For details, see Section
   *  <em> @ref ss_resendHeaders</em>.
   *  \n <tt>0</tt>: do not re-send.
   *  \n <tt>1</tt>: re-send. */
  u32 resendPPS;
  /** \brief Whether to send the access unit delimiter (AUD).
   *  \n <tt>0</tt>: do not send.
   *  \n <tt>1</tt>: send. */
  u32 sendAUD;
  /** \brief Whether to code all MBs or CTBs in the current frame in skip mode. For details,
   *  see Section <em> @ref ss_skipFrame</em>.
   *  \n <tt>0</tt>: do not code in skip mode.
   *  \n <tt>1</tt>: code in skip mode. */
  u32 bSkipFrame;
  /** \brief Whether to enable external reference frame selection. For details, see Section
   *  <em> @ref ss_flexRefs</em>.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 flexRefsEnable;
  /** \brief Whether to enable DEC400.
   *  \n <tt>0</tt>: global-bypass or indicate dec400 doesn't exit.
   *  \n <tt>1</tt>: stream-bypass.
   *  \n <tt>2</tt>: enable. */
  u32 dec400Enable;
  /** \brief Whether to enable AXI FE.
   *  \n <tt>0</tt>: disable AXI FE.
   *  \n <tt>1</tt>: enable AXI FE in normal mode.
   *  \n <tt>2</tt>: enable AXI FE in bypass mode.
   *  \n <tt>3</tt>: enable AXI FE in security mode. */
  u32 axiFEEnable;
  /** \brief Whether to enable APB filters.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 apbFTEnable;

  /*  Overlay */
  /** \brief The base bus address of the input buffer for each OSD channel.
   *  \n This field specifies the bus address of the Y-plane buffer if the picture data is
   *  stored in a planar or semi-planar YUV format, or the bus address of the buffer that
   *  store the complete picture data if the picture data is stored in an interleaved YUV
   *  or RGB format. */
  ptr_t overlayInputYAddr[MAX_OVERLAY_NUM];
  /** \brief The base bus address of the input buffer for each OSD channel.
   *  \n This field specifies the bus address of the U-plane buffer if the picture data is
   *  stored in a planar YUV format, \n or the bus address of the UV-plane buffer if the
   *  picture data is stored in a semi-planar YUV format.
   *  \n This field is invalid if the input picture data is stored in an interleaved YUV
   *  or RGB format. */
  ptr_t overlayInputUAddr[MAX_OVERLAY_NUM];
  /** \brief The base bus address of the input buffer for each OSD channel.
   *  \n This field specifies the bus address of the V-plane buffer if the picture data
   *  is stored in a planar YUV format.
   *  \n This field is invalid if the input picture data is stored in a semi-planar YUV
   *  format or an interleaved YUV or RGB format. */
  ptr_t overlayInputVAddr[MAX_OVERLAY_NUM];
  /** \brief Whether to enable each OSD channel.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 overlayEnable[MAX_OVERLAY_NUM];
  /** \brief Whether the OSD input picture is DEC400 compressed.
   *  \n <tt>0</tt>: not compressed.
   *  \n <tt>1</tt>: bypassed.
   *  \n <tt>2</tt>: compressed. */
  u32 osdDec400Enable[MAX_OVERLAY_NUM];
  /** \brief The base bus address of the compression table for OSD input pictures. */
  ptr_t osdDec400TableBase[MAX_OVERLAY_NUM][3];
  /** \brief (Under development) Whether to enable the OSD map.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 osdMapEnable;
  /** \brief (Under development) The base bus address of the OSD map buffer. */
  ptr_t osdMapInputAddr;
  /** \brief The number of <tt>ExternalSEI</tt> entries in <tt>VCEncIn.pExternalSEI</tt>.
   *  For details, see Section <em> @ref ss_externSEI</em>. */
  u32 externalSEICount;
  /** \brief The external SEI information. For details, see Section <em> @ref ss_externSEI</em>. */
  ExternalSEI *pExternalSEI;
  /** \brief The parameters for the second tile to the last tile in the input picture in
   *  multi-tile scenarios.
   *  \n If <tt>VCEncIn.busLuma</tt> is not specified for the second tile, this field is automatically filled
   *  for the second tile to the last tile according to the tile distribution. */
  VCEncInTileExtra *tileExtra;

  /** \brief internal context for the input, API user should not use it. */
  void *internal;
  /** extension parameters */
  void *extension;
} VCEncIn;
/** @} */

#define SHORT_TERM_REFERENCE 0x01
#define LONG_TERM_REFERENCE 0x02

/**
  * \addtogroup api_video
  *
  * @{
  */
/** \brief Contains reconstructed frame information. */
typedef struct {
  /*recon buffer*/
  /** \brief The picture order count (POC). */
  i32 poc;
  /** \brief The frame number. */
  i32 frame_num;
  /** \brief The LTR index.
   *  \n This field is valid only if the frame is used as an LTR frame. */
  i32 frame_idx;
  /** \brief The reference type flag.
   *  \n <tt>1</tt>: short-term reference.
   *  \n <tt>2</tt>: long-term reference. */
  i32 flags;
  /** \brief The temporal layer ID. */
  i32 temporalId;
  /** \brief The virtual address of the multi-core synchronization word. */
  u32 *mc_sync_va;
  /** \brief The bus address of the multi-core synchronization word. */
  ptr_t mc_sync_pa;
  /** \brief The bus address of the Y plane of the reconstructed frame. */
  ptr_t busReconLuma;
  /** \brief The bus address of the UV plane of the reconstructed frame. */
  ptr_t recon_luma_va;
  ptr_t busReconChromaUV;
  ptr_t recon_chroma_va;

  /* private buffers with recon frame*/
  /** \brief The bus address of the down-sampled reconstructed frame. */
  ptr_t reconLuma_4n;
  ptr_t reconLuma_4n_va;
  /** \brief The bus address of the compression table of the Y plane. */
  ptr_t compressTblReconLuma;
  ptr_t compressTblReconLuma_va;
  /** \brief The bus address of the compression table of the UV plane. */
  ptr_t compressTblReconChroma;
  ptr_t compressTblReconChroma_va;
  /** \brief The bus address of the column buffer for H.264 (AVC). */
  ptr_t colBufferH264Recon;
  ptr_t colBufferH264Recon_va;
    /* Bus & Virtual address for frame context of AV1 and VP9 */
  ptr_t framectx_pa;
  ptr_t framectx_va;
  ptr_t cuInfoMemRecon;  /** \brief The bus address of the CU information buffer. */
} VCEncReconPara;

/** \brief Contains H.264 (AVC) parameters for libva. */
typedef struct {
  //recon :    short or long term ref
  //ref list0 :    short or long term ref
  //ref list1 :    short or long term ref

  /** \brief Whether the picture is an IDR picture.
   *  \n <tt>0</tt>: The picture is not an IDR picture.
   *  \n <tt>1</tt>: The picture is an IDR picture. */
  unsigned int idr_pic_flag;
  /** \brief Whether the picture is a reference picture.
   *  \n <tt>0</tt>: The picture is not a reference picture.
   *  \n <tt>1</tt>: The picture is a reference picture. */
  unsigned int reference_pic_flag;

  /* \brief The picture identifier.
     *   Range: 0 to \f$2^{log2\_max\_frame\_num\_minus4 + 4} - 1\f$, inclusive.
     */
  // unsigned short  frame_num;

  /** \brief The slice type.
   *  \n Valid value ranges include <tt>0</tt> to <tt>2</tt> and <tt>5</tt> to <tt>7</tt>.
   *  \n - Values <tt>0</tt> and <tt>5</tt> indicate P slice.
   *  \n - Values <tt>1</tt> and <tt>6</tt> indicate B slice.
   *  \n - Values <tt>2</tt> and <tt>7</tt> indicate I slice.
   *  \n Switching slices are not supported.
   *  \n For more details about the values, see ITU-T Rec. H.264. */
  unsigned char slice_type;

  /* \brief Same as the H.264 bitstream syntax element. */
  /*unsigned char   pic_parameter_set_id;*/

  /** \brief The ID of an IDR picture.
   *  \n This field is equivalent to the <tt>idr_pic_id</tt> syntax element in ITU-T Rec. H.264. */
  unsigned short idr_pic_id;

  /* \brief Specifies if
     * see Section \ref _VAEncPictureParameterBufferH264::num_ref_idx_l0_active_minus1 or
     * see Section \ref _VAEncPictureParameterBufferH264::num_ref_idx_l1_active_minus1 are
     * overriden by the values for this slice.
     */
  //unsigned char   num_ref_idx_active_override_flag;

  /** \brief The maximum reference index on reference list 0.
   *  \n The valid value range is from <tt>0</tt> to <tt>31</tt>, inclusive. */
  unsigned char num_ref_idx_l0_active_minus1;
  /** \brief The maximum reference index on reference list 1.
   *  \n The valid value range is from <tt>0</tt> to <tt>31</tt>, inclusive. */
  unsigned char num_ref_idx_l1_active_minus1;
  /** \brief The POC modulo MaxPicOrderCntLsb,
   *  \n where MaxPicOrderCntLsb equals power(2, <tt>VCEncConfig.log2MaxPicOrderCntLsb</tt>).
   *  \n This field is valid only if the value of <tt>VCEncConfig.picOrderCntType</tt> is <tt>0</tt>. */
  unsigned short pic_order_cnt_lsb;
} VCEncH264Para;

/** \brief Contains HEVC (H.265) parameters for libva. */
typedef struct {
  //recon :    short or long term ref
  //ref list0 :    short or long term ref
  //ref list1 :    short or long term ref

  /** \brief Whether the picture is an IDR picture.
   *  \n <tt>0</tt>: The picture is not an IDR picture.
   *  \n <tt>1</tt>: The picture is an IDR picture. */
  unsigned int idr_pic_flag;
  /** \brief Whether the picture is a reference picture.
   *  \n <tt>0</tt>: The picture is not a reference picture.
   *  \n <tt>1</tt>: The picture is a reference picture. */
  unsigned int reference_pic_flag;

  /*  The picture identifier.
     *   Range: 0 to \f$2^{log2\_max\_frame\_num\_minus4 + 4} - 1\f$, inclusive.
     */
  //unsigned short  frame_num;

  /** \brief The slice type.
   *  \n <tt>0</tt>: P slice.
   *  \n <tt>1</tt>: B slice.
   *  \n <tt>2</tt>: I slice.
   *  \n Switching slices are not supported.
   *  \n For more details about the values, see ITU-T Rec. H.265. */
  unsigned char slice_type;

  /* Same as the H.264 bitstream syntax element. */
  /*unsigned char   pic_parameter_set_id;*/

  /** \brief The ID of an IDR picture.
   *  \n This field is equivalent to the idr_pic_id syntax element in ITU-T Rec. H.264. */
  unsigned short idr_pic_id;
  /** \brief The maximum reference index on reference list 0.
   *  \n The valid value range is from <tt>0</tt> to <tt>31</tt>, inclusive. */
  unsigned char num_ref_idx_l0_active_minus1;
  /** \brief The maximum reference index on reference list 1.
   *  \n The valid value range is from <tt>0</tt> to <tt>31</tt>, inclusive. */
  unsigned char num_ref_idx_l1_active_minus1;
  /** \brief The POC modulo MaxPicOrderCntLsb,
   *  \n where MaxPicOrderCntLsb equals power(2, <tt>VCEncConfig.log2MaxPicOrderCntLsb</tt>).
   *  \n This field is valid only if the value of <tt>VCEncConfig.picOrderCntType</tt> is
   *  <tt>0</tt>. */
  unsigned short pic_order_cnt_lsb;
  /** \brief The number of reference pictures that have negative POC delta compared with the
   *  current frame. */
  unsigned char rps_neg_pic_num;
  /** \brief The number of reference pictures that have positive POC delta compared with the
   *  current frame. */
  unsigned char rps_pos_pic_num;
  /** \brief The POC delta of reference frames, in ascending order. */
  signed int rps_delta_poc[VCENC_MAX_REF_FRAMES];
  /** \brief The flags that indicate whether each reference frame is used during the encoding
   *  of the current frame. */
  unsigned char rps_used_by_cur[VCENC_MAX_REF_FRAMES];
} VCEncHevcPara;

/** Input parameters extension for av1 */
typedef struct
{
  /** \brief frame_type in uncompressed header */
  unsigned int frame_type;
  /** \brief converted from frame_type to adapt for general codec */
  unsigned char slice_type;
  /** \brief if current frame is an KEY frame */
  unsigned int idr_pic_flag;
  /** \brief if current frame is used as reference */
  unsigned int reference_pic_flag;
  /** \brief number of forward reference pictures. */
  unsigned char   num_ref_idx_l0;
  /** \brief number of backward reference pictures. */
  unsigned char   num_ref_idx_l1;

  /** \brief refresh_frame_flags in uncompressed header */
  unsigned int refresh_frame_flags;
  /** \brief ref_order_hint[] in uncompressed header */
  unsigned int ref_order_hint[AV1_REF_FRAME_NUM];
  /** \brief ref_frame_idx[] in uncompressed header */
  unsigned int ref_frame_idx[AV1_REFS_PER_FRAME];

  /** \brief The order_hint in uncompressed header. */
  unsigned short  order_hint;

} VCEncAv1Para;

/** \brief Defines the libva-private input to the encoder. */
typedef struct {
  /** \brief A pointer to the next VCEncExtParaIn instance. */
  struct node *next;
  /** \brief The parameters for the reconstructed frame. */
  VCEncReconPara recon;
  /** \brief The parameters for reference list 0. */
  VCEncReconPara reflist0[2];
  /** \brief The parameters for reference list 1. */
  VCEncReconPara reflist1[2];
  /** \brief The parameters for HEVC (H.265) or H.264 (AVC). */
  union {
    /** \brief The parameters for H.264. */
    VCEncH264Para h264Para;
    /** \brief The parameters for HEVC. */
    VCEncHevcPara hevcPara;
    /** \brief The parameters for av1. */
    VCEncAv1Para av1Para;
  } params;
} VCEncExtParaIn;

/** \brief The CU statistics for adaptive GOP structure size decision. */
typedef struct {
  /** \brief The number of 8x8 blocks in intra mode. */
  u32 intraCu8Num;
  /** \brief The number of 8x8 blocks in skip mode. */
  u32 skipCu8Num;
  /** \brief The rate distortion (RD) cost used in reconstructed frame down-sampling ME
   *  search for inter-frames. */
  u32 PBFrame4NRdCost;
} VCEncCuStatis;

/** \brief Contains the bus addresses consumed by the encoder for encoding of a picture. */
typedef struct {
  /** \brief The bus address of the input buffer. */
  ptr_t inputbufBusAddr;
  /** \brief The bus address of the output buffer. */
  ptr_t outbufBusAddr;
  /** \brief The bus address of the DEC400 compression table buffer. */
  ptr_t dec400TableBusAddr;
  /** \brief The bus address of the QP map buffer. */
  ptr_t roiMapDeltaQpBusAddr;
  /** \brief The bus address of the CU control map buffer. */
  ptr_t roimapCuCtrlInfoBusAddr;
  /** \brief The bus addresses of the OSD inputs. */
  ptr_t overlayInputBusAddr[MAX_OVERLAY_NUM];
  /** \brief (Under development) The bus address of the OSD map buffer. */
  ptr_t osdMapInputBusAddr;
} VCEncConsumedAddr;

/** \brief Contains the output parameters for the second tile column to the last tile column
 *  in multi-tile scenarios. */
typedef struct {
  /** \brief The size of the output stream in bytes. */
  u32 streamSize;
  /** \brief The output buffer that stores the size of each NAL unit in bytes.
   *  \n <tt>pNaluSizeBuf[<em>n</em>]</tt> indicates the size of NAL unit <em>n</em>.
   *  \n A zero value is written after the last NAL unit. */
  u32 *pNaluSizeBuf;
  /** \brief The number of NAL units. */
  u32 numNalus;
  /** \brief A pointer to the CU/MB statistics that is output by the hardware. */
  VCEncCuOutData cuOutData;
} VCEncOutTileExtra;

/** \brief Contains the frame information of a picture that is output by the hardware. */
typedef union {
  /** \brief The frame information for HEVC (H.265) encoding. */
  struct hevcFramInfo hevcInfo;
  /** \brief The frame information for H.264 (AVC) encoding. */
  struct h264FramInfo h264Info;
} VCEncOutFrameInfo;

/** \brief Defines the output from the video encoder. */
typedef struct {
  /** \brief The realized picture coding type.
   *  \n Valid values include <tt> \ref VCENC_INTRA_FRAME</tt>, <tt> \ref VCENC_PREDICTED_FRAME</tt>,
   *  <tt> \ref VCENC_BIDIR_PREDICTED_FRAME</tt>, and <tt> \ref VCENC_NOTCODED_FRAME</tt>. */
  VCEncPictureCodingType codingType;
  /** \brief The size of the output stream in bytes. */
  u32 streamSize;
  /** \brief The output buffer that stores the size of each NAL unit in bytes.
   *  \n <tt>pNaluSizeBuf[<em>n</em>]</tt> indicates the size of NAL unit <em>n</em>.
   *  \n A zero value is written after the last NAL unit. */
  u32 *pNaluSizeBuf;
  /** \brief The number of NAL units. */
  u32 numNalus;
  /** \brief The frame information of a picture. */
  VCEncOutFrameInfo *frameInfo;
  /** \brief The virtual address of luma information . */
  u16 *lumaInfo;
  /** \brief (For AV1/VP9 only) A 32-bit flag for IVF timestamp generation.
   *  \n Bit <em>n</em> indicates whether frame <em>n</em> (an OBU frame for AV1 or raw
   *  frame for VP9) in the output buffer is a not-show frame.
   *  \n <tt>0</tt>: show frame.
   *  \n <tt>1</tt>: not-show frame. */
  u32 av1Vp9FrameNotShow;
  /** \brief The maximum size of output slices in bytes. */
  u32 maxSliceStreamSize;
  /** \brief The bus address of the buffer of the scaled encoder picture. */
  ptr_t busScaledLuma;
  /** \brief The virtual address of the scaled encoder picture. */
  u8 *scaledPicture;
  /** \brief A pointer to the CU/MB statistics that is output by the hardware. */
  VCEncCuOutData cuOutData;
  /** \brief A pointer to the CTB encoded bit statistics that are output by the hardware. */
  u16 *ctbBitsData;
  /** \brief Whether the current frame is used for long-term reference (LTR) by a subsequent frame.
   *  \n <tt>true</tt>: used for LTR by a subsequent frame.
   *  \n <tt>false</tt>: not used for LTR by a subsequent frame. */
  u8 boolCurIsLongTermRef;
  /** \brief The SSIM values calculated by the hardware for Y, U, and V.
   *  \n The field is valid only if the value of <tt>EWLHwConfig_t.ssimSupport</tt> is <tt>1</tt>. */
  double ssim[3];
  /** \brief The PSNR values calculated by the hardware for Y, U, and V.
   *  \n The field is valid only if the value of <tt>EWLHwConfig_t.psnrSupport</tt> is <tt>1</tt>. */
  double psnr[3];
  /** \brief An approximate PSNR value for Y, which excludes the eight bottom lines of each CTB.
   *  \n The hardware calculates SSE, and the software calculates log10f. */
  double psnr_y;
  /** \brief The PSNR value for Y before de-blocking.
   *  \n The hardware calculates SSE, and the software calculates log10f. */
  double psnr_y_predeb;
  /** \brief The PSNR value for U before de-blocking.
   *  \n The hardware calculates SSE, and the software calculates log10f. */
  double psnr_cb_predeb;
  /** \brief The PSNR value for V before de-blocking.
   *  \n The hardware calculates SSE, and the software calculates log10f. */
  double psnr_cr_predeb;
  /** \brief The size of the output slice header in multi-tile scenarios. */
  u32 sliceHeaderSize;
  /** \brief The CU statistics for adaptive GOP structure size decision. */
  VCEncCuStatis cuStatis;
  /** \brief The scene change detection result for the next frame, output by the video
   *  stabilization module.
   *  \n  For details, see Section <em> @ref sss_stab_detect</em>. */
  u32 nextSceneChanged;
  /** \brief The encoded picture count. */
  i32 picture_cnt;
  /** \brief The number of bytes in the stream before hardware encoding, excluding the initial
   *  stream header data. */
  u32 PreDataSize;
  /** \brief The number of NAL units in the stream before hardware encoding, excluding the initial
   *  stream header data. */
  u32 PreNaluNum;
  /** \brief The number of bytes in the stream after hardware encoding. */
  u32 PostDataSize;
  /** \brief The picture order count (POC). */
  i32 poc;
  /** \brief The bus addresses of the consumed buffers that should be released after this frame
   *  is encoded. */
  VCEncConsumedAddr consumedAddr;
  /** \brief The parameters for the second tile column to the last tile column in multi-tile
   *  scenarios. */
  VCEncOutTileExtra *tileExtra;
} VCEncOut;

/**@}*/

/**
  * \defgroup api_preprocessing Pre-Processing API
  *
  * @{
  */
/* Input pre-processing */

/** \brief Contains configurations of RGB-to-YUV conversion. For details, see Section
 *  <em> @ref s_csc</em>. */
typedef struct {
  /** \brief The RGB-to-YUV conversion type. */
  VCEncColorConversionType type;
  /** \brief The custom coefficient A. */
  u16 coeffA;
  /** \brief The custom coefficient B. */
  u16 coeffB;
  /** \brief The custom coefficient C. */
  u16 coeffC;
  /** \brief The custom coefficient E. */
  u16 coeffE;
  /** \brief The custom coefficient F. */
  u16 coeffF;
  /** \brief The custom coefficient G. */
  u16 coeffG;
  /** \brief The custom coefficient H. */
  u16 coeffH;
  /** \brief The custom luma offset. */
  u16 LumaOffset;
} VCEncColorConversion;

/* Input pre-processing */

/** \brief Contains input pre-processing configurations for the second tile to the last tile in
 *  multi-tile scenarios. */
typedef struct {
  /** \brief The width of the input picture, in pixels. */
  u32 origWidth;
  /** \brief The height of the input picture, in pixels. */
  u32 origHeight;
  /** \brief The horizontal offset, in pixels, of the top-left corner of the encoded picture
   *  relative to the input picture. */
  u32 xOffset;
  /** \brief The vertical offset, in pixels, of the top-left corner of the encoded picture
   *  relative to the input picture. */
  u32 yOffset;
} VCEncPrpTileExtra;

/** \brief Contains pre-processing configurations. */
typedef struct {
  /** \brief The width of the input picture, in pixels. */
  u32 origWidth;
  /** \brief The height of the input picture, in pixels. */
  u32 origHeight;
  /** \brief The horizontal offset, in pixels, of the top-left corner of the encoded picture
   *  relative to the input picture.
   *  \n The value must be an even integer. */
  u32 xOffset;
  /** \brief The vertical offset, in pixels, of the top-left corner of the encoded picture
   *  relative to the input picture.
   *  \n The value must be an even integer. */
  u32 yOffset;
  /** \brief The aligned width of the input picture, in pixels. */
  u32 alignmentWidth;
  /** \brief The aligned height of the input picture, in pixels. */
  u32 alignmentHeight;
  /** \brief The color format of the input picture. */
  VCEncPictureType inputType;
  /** \brief The rotation type. */
  VCEncPictureRotation rotation;
  /** \brief The mirroring mode. */
  VCEncPictureMirror mirror;
  /** \brief Whether to enable video stabilization.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 videoStabilization;
  /** \brief The RGB-to-YUV conversion configurations. */
  VCEncColorConversion colorConversion;
  /** \brief (Optional) The width of the down-scaled picture.
   *  \n The value is a multiple of 4, in the range from <tt>16</tt> to <tt>VCEncConfig.width</tt>,
   *  inclusive.
   *  \n If either the value of this field or that of <tt>VCEncPreProcessingCfg.scaledHeight</tt>
   *  is <tt>0</tt>, down-scaling is disabled. */
  u32 scaledWidth;
  /** \brief (Optional) The height of the down-scaled picture.
   *  \n The value is a multiple of 2, in the range from <tt>96</tt> to <tt>VCEncConfig.height</tt>,
   *  inclusive.
   *  \n If either the value of this field or that of <tt>VCEncPreProcessingCfg.scaledWidth</tt> is
   *  <tt>0</tt>, down-scaling is disabled. */
  u32 scaledHeight;
  /** \brief Whether to output the down-scaled picture to a buffer.
   * \n <tt>0</tt>: do not output.
   * \n <tt>1</tt>: output. */
  u32 scaledOutput;
  /** \brief The color format of the output down-scaled picture.
   *  \n <tt>0</tt> (default): YUV422 interleaved.
   *  \n <tt>1</tt>: YUV420 semi-planar. */
  u32 scaledOutputFormat;
  /** \brief The virtual address of the down-scaled picture buffer. */
  u32 *virtualAddressScaledBuff;
  /** \brief The physical address of the down-scaled picture buffer. */
  ptr_t busAddressScaledBuff;
  /** \brief The size of the down-scaled picture buffer.
   *  \n The size is greater than or equal to
   *  (<tt>VCEncPreProcessingCfg.scaledWidth</tt> * <tt>VCEncPreProcessingCfg.scaledOutput</tt> * 2)
   *  bytes. */
  u32 sizeScaledBuff;
  /** \brief The down-scaling ratio of the inloop down-scaler in the pass-1 encoder for look-ahead
   *  encoding.
   *  \n <tt>0</tt>: down-scaling disabled.
   *  \n <tt>1</tt>: 1/2 down-scaling ratio.
   *  \n This field is used only for parameter query through the <tt>VCEncGetPreProcessing()</tt>
   *  function. */
  u32 inLoopDSRatio;
  /** \brief Whether to encode the picture with constant U and V values.
   *  \n <tt>0</tt>: do not encode with a constant U or V value.
   *  \n <tt>1</tt>: encode with constant U and V values. */
  i32 constChromaEn;
  /** \brief The constant U value.
   *  \n The field value is 128 for 8-bit encoding and 512 for 10-bit encoding.
   *  \n This field is valid only if the value of <tt>VCEncPreProcessingCfg.constChromaEn</tt> is
   *  <tt>1</tt>. */
  u32 constCb;
  /** \brief The constant V value.
   *  \n The field value is 128 for 8-bit encoding and 512 for 10-bit encoding.
   *  \n This field is valid only if the value of <tt>VCEncPreProcessingCfg.constChromaEn</tt> is
   *  <tt>1</tt>. */
  u32 constCr;
  /** \brief The input alignment of the input picture, in bytes. */
  u32 input_alignment;

  /* Overlay area */
  /** \brief The OSD regions. */
  VCEncOverlayArea overlayArea[MAX_OVERLAY_NUM];

  /* Mosaic region parameters */
  /** \brief Whether to enable each mosaic region.
   *  \n <tt>0</tt>: disable.
   *  \n <tt>1</tt>: enable. */
  u32 mosEnable[MAX_MOSAIC_NUM];
  /** \brief Mosaic size index */
  u32 mosSizeIndex;
  /** \brief The horizontal offset of the top-left corner of each mosaic region. */
  u32 mosXoffset[MAX_MOSAIC_NUM];
  /** \brief The vertical offset of the top-left corner of each mosaic region. */
  u32 mosYoffset[MAX_MOSAIC_NUM];
  /** \brief The width of each mosaic region. */
  u32 mosWidth[MAX_MOSAIC_NUM];
  /** \brief The height of each mosaic region. */
  u32 mosHeight[MAX_MOSAIC_NUM];

  /* OSD_MAP */
  u32 osdMapEnable;
  char * osdMapInput;
  u32 osdMapStride;
  u32 osdMapAlpha[MAX_OSDMAP_COLOR_NUM];
  u32 osdMapY[MAX_OSDMAP_COLOR_NUM];
  u32 osdMapU[MAX_OSDMAP_COLOR_NUM];
  u32 osdMapV[MAX_OSDMAP_COLOR_NUM];
  /** \brief The input pre-processing configurations for the second tile to the last tile in
   *  multi-tile scenarios. */
  VCEncPrpTileExtra *tileExtra;
  VCEncPictureScanType scanType;
} VCEncPreProcessingCfg;

/**@}*/

/**
 * \defgroup api_pps PPS Configuration API
 *
 * @{
 */

/** \brief Contains user-provided picture parameter set (PPS) parameters. */
typedef struct {
  /** \brief The chroma QP offset. */
  i32 chroma_qp_offset;
  /** \brief The de-blocking parameter <tt>tc_offset</tt>. */
  i32 tc_Offset;
  /** \brief The de-blocking parameter <tt>beta_offset</tt>. */
  i32 beta_Offset;
} VCEncPPSCfg;
/**@}*/

/**
 * \addtogroup api_video
 *
 * @{
 */
/** \brief Contains output stream buffers. */
typedef struct {
  /** \brief A pointer to the beginning of each output stream buffer. */
  u8 *buf[MAX_STRM_BUF_NUM];
  /** \brief The length of each output stream buffer. */
  u32 bufLen[MAX_STRM_BUF_NUM];
  /** \brief The offset of each output stream buffer. */
  u32 bufOffset[MAX_STRM_BUF_NUM];
} VCEncStrmBufs;

/**
 *  \brief Defines the input to the callback function <tt>VCEncSliceReadyCallBackFunc()</tt>.
 *  For details, see Section <em> @ref ss_multislice</em>.
 */
typedef struct {
   /** \brief The number of slices that have been completed by the previous callback.
    *  \n This field is given because one or more slices can be completed between the callbacks. */
  u32 slicesReadyPrev;
  /** \brief The number of slices that have been completed. */
  u32 slicesReady;
  /** \brief The number of information NAL units that have been completed, including all kinds
   *  of SEI information.*/
  u32 nalUnitInfoNum;
  /** \brief The number of information NAL units that have been completed by the previous callback,
   *  including all kinds of SEI information.*/
  u32 nalUnitInfoNumPrev;
  /** \brief A pointer to the size of each completed slice, in bytes. */
  u32 *sliceSizes;
  /** \brief The output stream buffers. */
  VCEncStrmBufs streamBufs;
  /** \brief A pointer to the application data. */
  void *pAppData;
  /** \brief The output memory. */
  EWLLinearMem_t *outbufMem[MAX_STRM_BUF_NUM];
  /** \brief The number of bytes in the stream before hardware encoding, excluding the initial
   *  stream header data. */
  u32 PreDataSize;
  /** \brief The number of NAL units in the stream before hardware encoding, excluding the initial
   *  stream header data. */
  u32 PreNaluNum;
  /** \brief Whether to enable one or more stream buffers.
   *  \n <tt>0</tt>: one output stream buffer. Only <tt>outbufMem[0]</tt> is valid.
   *  \n <tt>1</tt>: two chained output stream buffers. All entries in the <tt>outbufMem</tt>
   *  array are valid. */
  u32 streamBufChain;
} VCEncSliceReady;

/** \brief The prototype of the callback function that is called when one or some slice streams
 *  are generated by the hardware and the software can then process the data in the output stream
 *  buffer without waiting for encoding of the whole frame to be completed.
 *  \n For information about the input parameters, see Section <em> \ref VCEncSliceReady<em>.
*/
typedef void (*VCEncSliceReadyCallBackFunc)(VCEncSliceReady *sliceReady);

/** \brief The API version information. */
typedef VceApiVersion VCEncApiVersion;

/** \brief The build information. */
typedef VceBuildInfo VCEncBuild;

/** \brief Specifies the encoder status. */
enum VCEncStatus {
  VCENCSTAT_INIT = 0xA1,
  VCENCSTAT_START_STREAM,
  VCENCSTAT_START_FRAME,
  VCENCSTAT_ERROR
};

/*------------------------------------------------------------------------------
      4. Encoder API function prototypes
  ------------------------------------------------------------------------------*/
/**
 * Returns the API version information.
 *
 * This function requires no parameters.
 *
 * \return \ref VCEncApiVersion
 */
VCEncApiVersion VCEncGetApiVersion(void);

/**
 * Returns software or hardware build information of a client type.
 *
 * This function can be called either before or after encoder instance initialization.
 *
 * \param [in] client_type The type of the client whose build information is to be queried.
 *
 * \return \ref VCEncBuild
 */
VCEncBuild VCEncGetBuild(u32 client_type);

/**
 * Queries the ROI map version supported by the hardware.
 *
 * For details about ROI map versions, see <em>Hantro VC9000E Series Memory Buffer and Format Organization</em>.
 *
 * \param [in] core_id The ID of the subsystem to be queried.
 * \param [in] ctx The context of the current device.
 *
 * \return The ROI map version
 */
u32 VCEncGetRoiMapVersion(u32 core_id, void *ctx);

/**
   * Returns the number of bits per pixel of an input color format.
   *
   * \param [in] type The input color format to be queried.
   *
   * \return The number of bits per pixel of the specified input color format
   */
u32 VCEncGetBitsPerPixel(VCEncPictureType type);

/**
   * Returns the stride in bytes based on the given input picture information.
   *
   * \param [in] width	The width of the input picture.
   * \param [in] input_format	The color format of the input picture.
   * \param [out] luma_stride A pointer to the storage space for receiving the luma stride.
   * \param [out] chroma_stride	A pointer to the storage space for receiving the chroma stride.
   * \param [in] input_alignment The alignment of the input picture.
   *
   * \return The stride in bytes
   */
u32 VCEncGetAlignedStride(int width, i32 input_format, u32 *luma_stride,
                          u32 *chroma_stride, u32 input_alignment, u32 scan_type);

/**
   * Creates and initializes an encoder instance.
   *
   * Before instance initialization, make sure that the EWL layer and kernel driver are ported
   * to your platform.
   *
   * The encoder instance must be freed using <tt>VCEncRelease()</tt> when it is no longer needed.
   *
   * \param [in] config \parblock A pointer to the configurations for initialization of the
   * encoder instance.
   *
   * Most configurations cannot be altered during encoding. \endparblock
   * \param [out] instAddr A pointer to the storage space for receiving the created encoder instance.
   * \param [in] ctx \parblock A pointer to the context from the application.
   *
   * The context may be used by EWL. For example, when the device file is open before
   * initialization, this parameter can be used to transfer the file data.
   *
   * Set it to <tt>NULL</tt> in case of no context. \endparblock
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncInit(const VCEncConfig *config, VCEncInst *instAddr, void *ctx);

/**
   * Releases an encoder instance created using <tt>VCEncInit()</tt>, and frees all the resources
   * allocated during encoder initialization.
   *
   * \param [in] inst \parblock The encoder instance to be released.
   *
   * After this function is successfully executed, the specified pointer is no longer valid. \endparblock
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncRelease(VCEncInst inst);

/**
   * Queries the number of hardware clock cycles used to encode the previous frame.
   *
   * \param [in] inst The encoder instance to be queried.
   *
   * \return The number of clock cycles used to encode the previous frame
   */
u32 VCEncGetPerformance(VCEncInst inst);

/**@}*/ /* end of addto api_video */

/**
 * \addtogroup api_codingctrl
 *
 * @{
 */

/**
   * Configures coding control for an encoder instance.
   *
   * This function can be called to alter all the coding control parameters for encoder instances
   * in INIT state. For encoder instances in START_STREAM or START_FRAME state, some parameters
   * are alterable between frames without the need to restart the stream. However, to alter the
   * other parameters, you need to restart the stream. For more information, see Section
   * <em> @ref ss_codectlSet</em>.
   *
   * \param [in] instAddr The encoder instance to be configured.
   * \param [in] pCodeParams A pointer to the coding control parameters.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncSetCodingCtrl(VCEncInst instAddr,
                            const VCEncCodingCtrl *pCodeParams);

/**
   * Queries the coding control settings of an encoder instance.
   *
   * \param [in] inst The encoder instance to be queried.
   * \param [out] pCodeParams A pointer to the storage space for receiving the coding control settings.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncGetCodingCtrl(VCEncInst inst, VCEncCodingCtrl *pCodeParams);

/**@}*/

/**
 * \addtogroup api_video
 *
 * @{
 */

/**
   * Configures stream-level encoding control for an encoder instance.
   *
   * This function is applicable only to encoder instances in INIT state. For details, see
   * Section <em> @ref ss_streamControl</em>.
   *
   * \param [in] inst The encoder instance to be configured.
   * \param [in] params A pointer to the stream-level encoding control parameters.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncSetStrmCtrl(VCEncInst inst, VCEncStrmCtrl *params);

/**@}*/ /* end of addto api_video */

/**
  * \addtogroup api_rc
  *
  * @{
  */

/**
  * Configures bit rate control for an encoder instance.
  *
  * This function can be called to alter all the bit rate control parameters for encoder instances
  * in INIT state. For encoder instances in START_STREAM or START_FRAME state, all the bit rate
  * control parameters except <tt>ctbRc</tt>, <tt>hrd</tt>, <tt>frameRateNum</tt>, and
  * <tt>frameRateDenom</tt> are alterable without the need to restart the stream if HRD is disabled.
  * However, if HRD is enabled, the stream needs to be restarted for altering most parameters. For
  * details, see Section <em> @ref ss_brConfig</em>.
  *
  * \param [in] inst The encoder instance to be configured.
  * \param [in] pRateCtrl A pointer to the rate control parameters.
  *
  * \return \ref VCEncRet
  */
VCEncRet VCEncSetRateCtrl(VCEncInst inst, const VCEncRateCtrl *pRateCtrl);

/**
   * Queries the bit rate control settings of an encoder instance.
   *
   * \param [in] inst The encoder instance to be queried.
   * \param [out] pRateCtrl A pointer to the storage space for receiving the bit rate control settings.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncGetRateCtrl(VCEncInst inst, VCEncRateCtrl *pRateCtrl);

/**@}*/

/**
   * \addtogroup api_preprocessing
   *
   * @{
   */

/**
   * Configures pre-processing blocks for an encoder instance.
   *
   * All pre-processing parameters are configurable before stream generation, and only some are
   * configurable during stream generation.
   *
   * \param [in] inst The encoder instance to be configured.
   * \param [in] pPreProcCfg \parblock A pointer to the pre-processing parameters.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncSetPreProcessing(VCEncInst inst,
                               const VCEncPreProcessingCfg *pPreProcCfg);

/**
   * Queries the pre-processing settings of an encoder instance.
   *
   * This function can be called before and during stream generation.
   *
   * \param [in] inst The encoder instance to be queried.
   * \param [out] pPreProcCfg A pointer to the storage space for receiving the pre-processing settings.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncGetPreProcessing(VCEncInst inst,
                               VCEncPreProcessingCfg *pPreProcCfg);

/**@}*/

/**
 * \addtogroup api_video
 *
 * @{
 */

/**
   * Enables or disables writing user data to the encoded stream as a supplemental enhancement
   * information (SEI) message to all subsequent encoded frames.
   *
   * The payload type of the SEI message is <tt>user_data_unregistered</tt>. For details, see
   * ITU-T Rec. H.265.
   *
   * \param [in] inst The encoder instance to be configured.
   * \param [in] pUserData \parblock A pointer to the buffer that contains the user data.
   *
   * The video encoder reads data straight from the buffer during encoding of the subsequent
   * frames. Therefore, the specified pointer to the buffer must not be freed until the user
   * data writing is disabled. \endparblock
   * \param [in] userDataSize \parblock The size of the user data in the buffer, in bytes.
   *
   * This parameter can be set to <tt>0</tt> or any value from <tt>16</tt> to <tt>2048</tt>.
   * If this parameter is set to <tt>0</tt>, the user data writing is disabled. \endparblock
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncSetSeiUserData(VCEncInst inst, const u8 *pUserData,
                             u32 userDataSize);

/**@}*/

/**
 * \defgroup group_strmPrd Stream Production API
 *
 * @{
 */

/* Stream generation API */

/**
   * Starts stream to generate SPS and PPS. SPS is carried in the first NAL unit and PPS in
   * the second NAL unit.
   *
   * \param [in] inst The encoder instance for which the stream is to be started.
   * \param [in] pEncIn A pointer to the input parameters.
   * \param [out] pEncOut A pointer to the storage space for receiving the output parameters,
   * in which <tt>NaluSizeBuf</tt> indicates the size of NAL units.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncStrmStart(VCEncInst inst, const VCEncIn *pEncIn,
                        VCEncOut *pEncOut);

/**
 * Encodes a video frame in display order. The frame may not be encoded immediately.
 *
 * \param [in] inst The encoder instance to encode the frame.
 * \param [in] pEncIn A pointer to the input parameters.
 * \param [out] pEncOut A pointer to the storage space for receiving the output parameters.
 * \param [in] sliceReadyCbFunc The callback function to be called when the hardware finishes
 * encoding a slice.
 * \param [in] pAppData A pointer to the application data to be input to the callback function.
 *
 * \return \ref VCEncRet
 */
VCEncRet VCEncStrmEncode(VCEncInst inst, VCEncIn *pEncIn, VCEncOut *pEncOut,
                         VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                         void *pAppData);

/**
 * Encodes a video frame in decoder order.
 *
 * \param [in] inst The encoder instance to encode the frame.
 * \param [in] pEncIn A pointer to the input parameters.
 * \param [in] pEncExtParaIn A pointer to the input parameters extended for libva.
 * \param [out] pEncOut A pointer to the storage space for receiving the output parameters.
 * \param [in] sliceReadyCbFunc The callback function to be called when the hardware finishes
 * encoding a slice.
 * \param [in] pAppData A pointer to the application data to be input to the callback function.
 * \param [in] useExtFlag \parblock Whether to enable the extended parameters.
 *
 * <tt>1</tt>: enables
 *
 * <tt>0</tt>: disables
 * \endparblock
 *
 * \return \ref VCEncRet
 */
VCEncRet VCEncStrmEncodeExt(VCEncInst inst, VCEncIn *pEncIn,
                            const VCEncExtParaIn *pEncExtParaIn,
                            VCEncOut *pEncOut,
                            VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                            void *pAppData, i32 useExtFlag);

/** Ends a stream with an end-of-sequence (EOS) code.
  *
  * \param [in] inst The encoder instance for which the stream is to be ended.
  * \param [in] pEncIn A pointer to the input parameters.
  * \param [out] pEncOut A pointer to the storage space for receiving the output parameters.
  *
  * \return \ref VCEncRet
  */
VCEncRet VCEncStrmEnd(VCEncInst inst, const VCEncIn *pEncIn, VCEncOut *pEncOut);

/**
   * Flushes a frame out from the internal queue of an encoder instance and encodes the frame.
   *
   * Call this function after all the frames to be encoded have been sent to the encoder.
   *
   * \param [in] inst The encoder instance from whose queue a frame is to be flushed out.
   * \param [in] pEncIn A pointer to the input parameters.
   * \param [out] pEncOut A pointer to the storage space for receiving the output parameters.
   * \param [in] sliceReadyCbFunc The callback function to be called when the hardware finishes
   * encoding a slice.
   * \param [in] pAppData A pointer to the application data to be input to the callback function.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncFlush(VCEncInst inst, VCEncIn *pEncIn, VCEncOut *pEncOut,
                    VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                    void *pAppData);

/**@}*/


/**
 * \addtogroup api_video
 *
 * @{
 */
/**
   * Flushes out remaining frames after all the frames to be encoded have been send into encoder.
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] pEncIn Pointer to user provided input parameters.
   * \param [in] sliceReadyCbFunc Pointer to the VCEncSliceReadyCallBackFunc callback function that will be called after a slice encoding has been
   *             finished by the hardware.
   * \param [in] pAppdata Pointer to application-specific data to be passed on to the callback function.
   * \param [out] pEncOut Pointer to the VCEncOut structure where the output parameters will be saved.
   * \return VCEncRet
   */
VCEncRet VCEncStrmGetOutput(VCEncInst inst, VCEncIn *pEncIn, VCEncOut *pEncOut,
                    VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                    void *pAppData);


/* Internal encoder testing */
/**
   * Imports encoder configurations from a test vector.
   *
   * \param [in] inst The encoder instance to use the configurations.
   * \param [in] testId The ID of the test vector.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncSetTestId(VCEncInst inst, u32 testId);

/**@}*/

/**
   * \addtogroup api_pps
   *
   * @{
   */

/* PPS config*/
/**
   * Creates a picture parameter set (PPS).
   *
   * \param [in] inst The encoder instance for which the PPS is to be created.
   * \param [in] pPPSCfg A pointer to the PPS parameters.
   * \param [out] newPPSId A pointer to the storage space for receiving the ID of the created PPS.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncCreateNewPPS(VCEncInst inst, const VCEncPPSCfg *pPPSCfg,
                           i32 *newPPSId);

/**
   * Modifies a picture parameter set (PPS).
   *
   * \param [in] inst The encoder instance whose PPS is to be modified.
   * \param [in] pPPSCfg A pointer to the PPS parameters.
   * \param [in] ppsId The ID of the PPS to be modified.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncModifyOldPPS(VCEncInst inst, const VCEncPPSCfg *pPPSCfg,
                           i32 ppsId);

/**
   * Queries the settings of a picture parameter set (PPS).
   *
   * \param [in] inst The encoder instance whose PPS is to be queried.
   * \param [out] pPPSCfg A pointer to the storage space for receiving the PPS parameters.
   * \param [in] ppsId The ID of the PPS to be queried.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncGetPPSData(VCEncInst inst, VCEncPPSCfg *pPPSCfg, i32 ppsId);

/**
   * Activates a picture parameter set (PPS) to replace the current PPS.
   *
   * \param [in] inst The encoder instance whose PPS is to be activated.
   * \param [in] ppsId The ID of the PPS to be activated.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncActiveAnotherPPS(VCEncInst inst, i32 ppsId);

/**
   * Queries the ID of the active picture parameter set (PPS).
   *
   * \param [in] inst The encoder instance to be queried.
   * \param [out] ppsId A pointer to the storage space for receiving the ID of the active PPS.
   *
   * \return \ref VCEncRet
   */
VCEncRet VCEncGetActivePPSId(VCEncInst inst, i32 *ppsId);
/**@}*/

/**
 * \addtogroup api_video
 *
 * @{
 */
/* low latency */
/** Sets the valid input MB line to be encoded with low-latency encoding by an encoder instance.
  *
  * \param [in] inst The encoder instance to be configured.
  * \param [in] lines The order count of the input MB line to be encoded.
  *
  * \return \ref VCEncRet
  */
i32 VCEncSetInputMBLines(VCEncInst inst, u32 lines);

/** Queries the MB line encoded with low-latency encoding by an encoder instance.
  *
  * \param [in] inst The encoder instance to be queried.
  *
  * \return The order count of the MB line encoded with low-latency encoding
  */
u32 VCEncGetEncodedMbLines(VCEncInst inst);

/**
   * Enables the error status for an encoder instance.
   *
   * This function is useful in look-ahead encoding.
   *
   * \param [in] inst The encoder instance for which the error status is to be set.
   *
   * \return None
   */
void VCEncSetError(VCEncInst inst);

/*------------------------------------------------------------------------------
       API Functions only valid for HEVC.
   ------------------------------------------------------------------------------*/
/**
 * (For HEVC only) Parses the encoding information of a CU from the output of a picture.
 *
 * \param [in] inst The encoder instance that outputs the picture.
 * \param [in] pEncCuOutData A pointer to the CU information of the picture output.
 * \param [in] ctuNum The index of the CTU within the picture.
 * \param [in] cuNum The index of the CU within the CTU.
 * \param [out] pEncCuInfo A pointer to the storage space for receiving the CU information.
 *
 * \return \ref VCEncRet
 */
VCEncRet VCEncGetCuInfo(VCEncInst inst, VCEncCuOutData *pEncCuOutData,
                        u32 ctuNum, u32 cuNum, VCEncCuInfo *pEncCuInfo);

/**
 * Queries the updated GOP structure size (<tt>gopSize</tt>) after pass-1 encoding.
 *
 * \param [in] inst The encoder instance whose GOP structure size is to be queried.
 *
 * \return The updated GOP structure size after pass-1 encoding
 * */
#ifdef USE_LIBVA
i32 VCEncGetPass1UpdatedGopSize(VCEncInst inst);
#endif
/**
 * Queries the maximum encoding delay of an encoder instance.
 *
 * \param [in] inst The encoder instance whose encoding buffer delay is to be queried.
 *
 * \return The maximum encoding delay, in the unit of frame
 * */
i32 VCEncGetEncodeMaxDelay(VCEncInst inst);

/**
 * Translates an API-defined codec standard into a EWL-defined client type.
 *
 * \param [in] codecFormat The codec standard defined in API.
 *
 * \return <tt>CLIENT_TYPE</tt> defined in EWL
 */
CLIENT_TYPE VCEncGetClientType(VCEncVideoCodecFormat codecFormat);
/**@}*/

uint32_t vc_enc_get_instance_length(void);

#ifdef __cplusplus
}
#endif

#endif
