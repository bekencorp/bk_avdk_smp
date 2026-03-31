/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2015 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Encoder common definitions for control code and system model
--
------------------------------------------------------------------------------*/

#ifndef __ENC_COMMON_H__
#define __ENC_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
    1. External compiler flags
------------------------------------------------------------------------------*/

/* Encoder global definitions
 *
 * _ASSERT_USED     # Asserts enabled
 * _DEBUG_PRINT     # Prints debug information on stdout
 * TEST_DATA        # Creates test data files
 *
 * Can be defined here or using compiler flags */

/*------------------------------------------------------------------------------
    2. Include headers
------------------------------------------------------------------------------*/

#include "base_type.h"
#include "ewl.h"
#include "ewl_memsync.h"

/* Stream tracing requires encdebug.h */
#if 0

#ifdef H2_HAVE_ENCTRACE_H
#include "enctrace.h"
#endif
#endif

//#define RCP_PRINT_INTFO

//#define RCP_FIXED_POINT_OPT

#ifdef RCP_FIXED_POINT_OPT
typedef i64 RCP_64bit;
typedef i32 RCP_32bit;
#else
typedef double RCP_64bit;
typedef double RCP_32bit;
#endif


/*------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/**
 * \addtogroup base_type
 *
 * @{
 */
/** Specifies whether a result is correct. */
typedef enum {
  /** \brief The result is incorrect. */
  ENCHW_NOK = -1,
  /** \brief The result is correct. */
  ENCHW_OK = 0
} bool_e;

/** Specifies a Boolean logic. */
typedef enum {
  /** \brief False. */
  ENCHW_NO = 0,
  /** \brief True. */
  ENCHW_YES = 1
} true_e;
/** @} */

/**
 * \addtogroup api_rc
 *
 * @{
 */
/** Specifies rate control modes. */
typedef enum {
  /** \brief The constrained variable bit rate (CVBR) mode. */
  VCE_RC_CVBR,
  /** \brief The constant bit rate (CBR) mode. */
  VCE_RC_CBR,
  /** \brief The variable bit rate (VBR) mode. */
  VCE_RC_VBR,
  /** \brief The average bit rate (ABR) mode. */
  VCE_RC_ABR,
  /** \brief The constant rate factor (CRF) mode. */
  VCE_RC_CRF,
  /** \brief The constant quantization parameter (CQP) mode. */
  VCE_RC_CQP
} rcMode_e;

/** @} */

typedef enum {
  CU_INFO_LOC_X_BITS = 3,
  CU_INFO_LOC_Y_BITS = 3,
  CU_INFO_SIZE_BITS = 2,
  CU_INFO_MODE_BITS = 1,
  CU_INFO_COST_BITS = 25,
  CU_INFO_INTER_PRED_IDC_BITS = 2,
  CU_INFO_MV_REF_IDX_BITS = 2,
  CU_INFO_MV_X_BITS = 14,
  CU_INFO_MV_Y_BITS = 14,
  CU_INFO_INTRA_PART_BITS = 1,
  CU_INFO_INTRA_PART_BITS_H264 = 2,
  CU_INFO_INTRA_PRED_MODE_BITS = 6,
  CU_INFO_INTRA_PRED_MODE_BITS_H264 = 4,
  CU_INFO_INTRA_DUMMY_BITS = 37,
  CU_INFO_MEAN_BITS = 10,
  CU_INFO_VAR_BITS = 18,
  CU_INFO_VAR_BITS_V3 = 26,
  CU_INFO_QP_BITS = 6,
  CU_INFO_OUTPUT_SIZE = 12,     //96 bits
  CU_INFO_TABLE_ITEM_SIZE = 4,  //32 bits
  CU_INFO_OUTPUT_SIZE_V1 = 26,  //208 bits
  CU_INFO_OUTPUT_SIZE_V2 = 16,  //128 bits
  CU_INFO_OUTPUT_SIZE_V3 = 19,  //152 bits
} cuInfoBits_e;

typedef enum {
  /** YYYY... UUUU... VVVV... */
  ENC_PIXFMT_YUV420_PLANAR = 0,
  /** YYYY... UVUVUV... */
  ENC_PIXFMT_YUV420_SEMIPLANAR = 1,
  /** YYYY... VUVUVU... */
  ENC_PIXFMT_YUV420_SEMIPLANAR_VU = 2,
  /** YUYVYUYV... */
  ENC_PIXFMT_YUV422_INTERLEAVED_YUYV = 3,
  /** UYVYUYVY... */
  ENC_PIXFMT_YUV422_INTERLEAVED_UYVY = 4,
  /** 16-bit RGB 16bpp */
  ENC_PIXFMT_RGB565 = 5,
  /** 16-bit RGB 16bpp */
  ENC_PIXFMT_BGR565 = 6,
  /** 15-bit RGB 16bpp */
  ENC_PIXFMT_RGB555 = 7,
  /** 15-bit RGB 16bpp */
  ENC_PIXFMT_BGR555 = 8,
  /** 12-bit RGB 16bpp */
  ENC_PIXFMT_RGB444 = 9,
  /** 12-bit RGB 16bpp */
  ENC_PIXFMT_BGR444 = 10,
  /** 24-bit RGB 32bpp */
  ENC_PIXFMT_RGB888 = 11,
  /** 24-bit RGB 32bpp */
  ENC_PIXFMT_BGR888 = 12,
  /** 30-bit RGB 32bpp */
  ENC_PIXFMT_RGB101010 = 13,
  /** 30-bit RGB 32bpp */
  ENC_PIXFMT_BGR101010 = 14,
  /** YYYY... UUUU... VVVV... */
  ENC_PIXFMT_YUV420_PLANAR_10BIT_I010 = 15,
  /** YYYY... UVUVUV... */
  ENC_PIXFMT_YUV420_PLANAR_10BIT_P010 = 16,
  /** YYYY... UUUU... VVVV... */
  ENC_PIXFMT_YUV420_PLANAR_10BIT_PACKED_PLANAR = 17,
  /** Y0U0Y1a0a1Y2V0Y3a2a3Y4U1Y5a4a5Y6V1Y7a6a7... */
  ENC_PIXFMT_YUV420_10BIT_PACKED_Y0L2 = 18,
  ENC_PIXFMT_YUV420_PLANAR_8BIT_TILE_32_32 = 19,
  ENC_PIXFMT_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4 = 20,
  /** YYYY... UVUVUV... */
  ENC_PIXFMT_YUV420_SEMIPLANAR_8BIT_TILE_4_4 = 21,
  /** YYYY... VUVUVU... */
  ENC_PIXFMT_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4 = 22,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_PLANAR_10BIT_P010_TILE_4_4 = 23,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_SEMIPLANAR_101010 = 24,
  ENC_PIXFMT_YUV422_888 = 25,
  /** YYYY... VUVU... */
  ENC_PIXFMT_YUV420_8BIT_TILE_64_4 = 26,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_UV_8BIT_TILE_64_4 = 27,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_10BIT_TILE_32_4 = 28,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_10BIT_TILE_48_4 = 29,
  /** YYYY... VUVU... */
  ENC_PIXFMT_YUV420_VU_10BIT_TILE_48_4 = 30,
  /** YYYY... VUVU... */
  ENC_PIXFMT_YUV420_8BIT_TILE_128_2 = 31,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_UV_8BIT_TILE_128_2 = 32,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_10BIT_TILE_96_2 = 33,
  /** YYYY... VUVU... */
  ENC_PIXFMT_YUV420_VU_10BIT_TILE_96_2 = 34,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_8BIT_TILE_8_8 = 35,
  /** YYYY... UVUV... */
  ENC_PIXFMT_YUV420_10BIT_TILE_8_8 = 36,
  /** YYYY... VVVV... UUUU... */
  ENC_PIXFMT_YVU420_PLANAR = 37,
  /** YYYY... UVUVUV 64x2 tile */
  ENC_PIXFMT_YUV420_UV_8BIT_TILE_64_2 = 38,
  /** YYYY... UVUVUV 64x2 tile */
  ENC_PIXFMT_YUV420_UV_10BIT_TILE_128_2 = 39,
  /** 24-bit RGB 24bpp (MSB)BGR(LSB) */
  ENC_PIXFMT_RGB888_24BIT = 40,
  /** 24-bit RGB 24bpp (MSB)RGB(LSB) */
  ENC_PIXFMT_BGR888_24BIT = 41,
  /** 24-bit RGB 24bpp (MSB)GBR(LSB) */
  ENC_PIXFMT_RBG888_24BIT = 42,
  /** 24-bit RGB 24bpp (MSB)RBG(LSB) */
  ENC_PIXFMT_GBR888_24BIT = 43,
  /** 24-bit RGB 24bpp (MSB)GRB(LSB) */
  ENC_PIXFMT_BRG888_24BIT = 44,
  /** 24-bit RGB 24bpp (MSB)BRG(LSB) */
  ENC_PIXFMT_GRB888_24BIT = 45,
  ENC_PIXFMT_YVU422SP_888 = 46,
  /** YYYY... UUUU... VVVV... */
  ENC_PIXFMT_YUV444_PLANAR = 47,
  /** X8Y8U8V8... */
  ENC_PIXFMT_YUV444_XYUV8888 = 48,
  /** X2Y10U10V10... */
  ENC_PIXFMT_YUV444_XYUV2101010 = 49,
  /** 24-bit RGB 32bpp */
  ENC_PIXFMT_RGBX8888 = 50,
  /** 24-bit RGB 32bpp */
  ENC_PIXFMT_BGRX8888 = 51,
  /** 30-bit RGB 32bpp */
  ENC_PIXFMT_RGBX1010102 = 52,
  /** 30-bit RGB 32bpp */
  ENC_PIXFMT_BGRX1010102 = 53,
  /** YYYY... VUVUVU... */
  ENC_PIXFMT_YVU420_PLANAR_10BIT_P010 = 54,
  ENC_PIXFMT_Y8b = 55,
  ENC_PIXFMT_Y10bWL = 56,
  ENC_PIXFMT_Y10bWH = 57,
  ENC_PIXFMT_YUV420SP10b = 58,
  ENC_PIXFMT_YUV422P10bWL = 59,
  ENC_PIXFMT_FORMAT_MAX
} EncPixelFormat;


/**
 * \addtogroup api_video
 *
 * @{
 */
/** \brief Defines the information of the encoder API version. */
typedef struct {
  /** \brief A major version with achitecture update or product promotion. */
  u32 major;
  /** \brief A minor version with milestone features added. */
  u32 minor;
  /** \brief A micro version with feature improvement or interface changes. */
  u32 micro;
} VceApiVersion;

/** \brief Defines the build information. */
typedef struct {
  /** \brief The software build ID, which equals the changelist number in Perforce. */
  u32 swBuild;
  /** \brief The hardware build ID, which comes from swreg0. */
  u32 hwBuild;
} VceBuildInfo;

/** @} */

/* H.264 collocated mb */
struct h264_mb_col {
  u8 colZeroFlagStore;  // per 2 mbs, bit[0..3] for 1st, bit[4..7] for second
};

/* VLC TABLE */
typedef struct {
  i32 value;  /* Value of bits  */
  i32 number; /* Number of bits */
} table_s;

/* used in stream buffer handling */
typedef struct {
#ifdef TEST_DATA
  struct stream_trace *stream_trace;
#endif
  u8 *stream;           /* Pointer to next byte of stream */
  u32 size;             /* Byte size of stream buffer */
  u32 byteCnt;          /* Byte counter */
  u32 bitCnt;           /* Bit counter */
  u32 byteBuffer;       /* Byte buffer */
  u32 bufferedBits;     /* Amount of bits in byte buffer, [0-7] */
  u32 zeroBytes;        /* Amount of consecutive zero bytes */
  i32 overflow;         /* This will signal a buffer overflow */
  u32 emulCnt;          /* Counter for emulation_3_byte, needed in SEI */
  i32 *table;           /* Video packet or Gob sizes */
  i32 tableSize;        /* Size of above table */
  i32 tableCnt;         /* Table counter of above table */
  u32 bufferedLeftBits; /* Amount of left bits in byte buffer, [0-7] */
} stream_s;

#if 0
/* General tools */
#define ABS(x) ((x) < (0) ? -(x) : (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SIGN(a) ((a) < (0) ? (-1) : (1))
#define OUT_OF_RANGE(x, a, b) \
  ((((i32)x) < (a) ? (1) : (0)) || ((x) > (b) ? (1) : (0)))
#define CLIP3(v, min, max) ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))
#endif

/* Encoder MB output information defined as a struct.

   NOTE!    By defining this struct we rely that ASIC output endianess
            is configured properly to produce output that is identical
            to the way the compiler constructs this struct. */


typedef struct EncOutForCutree {
  int poc;
  int qp;
  u32 codingType;
  i32 hierarchial_bit_allocation_GOP_size;
  i32 encoded_frame_number;
  u32 dsRatio;
  u32 extDSRatio;
  i32 p0;
  i32 p1;
  u32 cuDataIdx;
  u32 roiMapEnable;
  i8 *pRoiMapDelta;
  ptr_t roiMapDeltaQpAddr;
  u32 roiMapDeltaSize;
  u32 motionScore[2][2];
} encOutForCutree_s;

/**
 * \addtogroup api_video
 *
 * @{
 */

/** \brief Contains the information of a frame that the video encoder outputs for HEVC (H.265). */
struct hevcFramInfo {
  /** \brief The number of bytes encoded. */
  u32 picBytes;
  /** \brief The number of inter 64x64 CUs. */
  u32 inter64x64CuNum;
  /** \brief The number of inter 32x32 CUs. */
  u32 inter32x32CuNum;
  /** \brief The number of inter 16x16 CUs. */
  u32 inter16x16CuNum;
  /** \brief The number of inter 8x8 CUs. */
  u32 inter8x8CuNum;
  /** \brief The number of intra 32x32 CUs. */
  u32 intra32x32CuNum;
  /** \brief The number of intra 16x16 CUs. */
  u32 intra16x16CuNum;
  /** \brief The number of intra 8x8 CUs. */
  u32 intra8x8CuNum;
  /** \brief The number of intra 4x4 CUs. */
  u32 intra4x4CuNum;
  /** \brief The number of TU residual bits. */
  u32 residualBits;
  /** \brief The number of CU mode bits and MV bits. */
  u32 headBits;
  /** \brief The average MAD for all blocks of the input picture, which can be used to measure the
   * space complexity of the input picture. */
  u32 madiVal;
  /** \brief The average MAD for all blocks of predicted residual, which can be used to measure the
   * time complexity of the input picture. */
  u32 madpVal;
  /** \brief The number of LCUs (that is, CTBs) in the frame. */
  u32 LCU_cnt;
  /** \brief The frame SSE.
   *  \n <tt>sseSum[0]</tt>: the frame SSE for the Y component.
   *  \n <tt>sseSum[1]</tt>: the frame SSE for the U component.
   *  \n <tt>sseSum[2]</tt>: the frame SSE for the V component. */
  u64 sseSum[3];
  /** \brief The SSE for a maximum of eight regions.
   *  \n <tt>sse_info[<em>n</em>][0]</tt>: the SSE for the Y component in region <em>n</em>.
   *  \n <tt>sse_info[<em>n</em>][1]</tt>: the SSE for the U component in region <em>n</em>.
   *  \n <tt>sse_info[<em>n</em>][2]</tt>: the SSE for the V component in region <em>n</em>. */
  u64 sse_info[8][3];
  /** \brief The fixed-point PSNR in 16.16 format.
   *  \n <tt>psnrVal[0]</tt>: the fixed-point PSNR for the Y component.
   *  \n <tt>psnrVal[1]</tt>: the fixed-point PSNR for the U component.
   *  \n <tt>psnrVal[2]</tt>: the fixed-point PSNR for the V component. */
  u32 psnrVal[3];
  /** \brief The QP histogram. */
  u32 qp_hist[52];
  /** \brief The POC of the frame. */
  u32 picPoc;
  /** \brief The number of slices in the frame. */
  u32 picSliceNum;
  /** \brief The number of intra blocks in each 8x8 unit. */
  u32 picNumIntra;
  /** \brief The number of inter merge blocks in each 8x8 unit. */
  u32 picNumMerge;
  /** \brief The number of skip blocks in each 8x8 unit. */
  u32 pSkipNum;
  /** \brief The number of ipcm blocks in each 8x8 unit. */
  u32 ipcmNum;
  /** \brief The initial QP of the frame, which refers to the <tt>sliceQp</tt> in the slice header. */
  u8  startQp;
  /** \brief Whether the CUs in the frame are skip CUs.
   *  \n <tt>0</tt>: the CUs in the frame are not skip CUs.
   *  \n <tt>1</tt>: the CUs in the frame are skip CUs. */
  u8  isPSkip;
  /** \brief The type of the frame.
   *  \n For the available values, see Section <em> \ref VCEncPictureCodingType</em>. */
  u8  picType;
  /** \brief The mean QP of the frame. */
  u8  meanQp;
  /** \brief The index of the picture in the current GOP.
   *  \n The pictures are indexed starting from 0 at each IDR frame. */
  u32 gopPicIdx;
  /** \brief The number of the picture. */
  u32 picNum;
  /** \brief The Sum luma of the picture. */
  u64 pixLumaSum;
  /** \brief The Max luma of the picture. */
  u16  pixLumaMax;
  /** \brief The min luma of the picture. */
  u16  pixLumaMin;
};


/** \brief Contains the information of a frame that the video encoder outputs for H.264 (AVC). */
struct h264FramInfo {
  /** \brief The number of bytes encoded. */
  u32 picBytes;
  /** \brief The number of skip MBs. */
  u32 pSkipMbNum;
  /** \brief The number of IPCM MBs. */
  u32 IpcmMbNum;
  /** \brief The number of inter 16x16 MBs. */
  u32 inter16x16MbNum;
  /** \brief The number of inter 16x8 MBs. */
  u32 inter16x8MbNum;
  /** \brief The number of inter 8x16 MBs. */
  u32 inter8x16MbNum;
  /** \brief The number of inter 8x8 MBs. */
  u32 inter8x8MbNum;
  /** \brief The number of intra 16x16 MBs. */
  u32 intra16MbNum;
  /** \brief The number of intra 8x8 MBs. */
  u32 intra8MbNum;
  /** \brief The number of intra 4x4 MBs. */
  u32 intra4MbNum;
  /** \brief The number of residual bits. */
  u32 residualBits;
  /** \brief The number of CU mode bits. */
  u32 headBits;
  /** \brief The average MAD for all blocks of the input picture, which can be used to measure the
   * space complexity of the input picture. */
  u32 madiVal;
  /** \brief The average MAD for all blocks of predicted residual, which can be used to measure the
   * time complexity of the input picture. */
  u32 madpVal;
  /** \brief The number of LCUs (that is, CTBs) in the frame. */
  u32 LCU_cnt;
  /** \brief The fixed-point PSNR in 16.16 precision.
   *  \n <tt>psnrVal[0]</tt>: the fixed-point PSNR for the Y component.
   *  \n <tt>psnrVal[1]</tt>: the fixed-point PSNR for the U component.
   *  \n <tt>psnrVal[2]</tt>: the fixed-point PSNR for the V component. */
  u32 psnrVal[3];
  /** \brief The frame SSE.
   *  \n <tt>sseSum[0]</tt>: the frame SSE for the Y component.
   *  \n <tt>sseSum[1]</tt>: the frame SSE for the U component.
   *  \n <tt>sseSum[2]</tt>: the frame SSE for the V component. */
  u64 sseSum[3];
  /** \brief The SSE for a maximum of eight regions.
   *  \n <tt>sse_info[<em>n</em>][0]</tt>: the SSE for the Y component in region <em>n</em>.
   *  \n <tt>sse_info[<em>n</em>][1]</tt>: the SSE for the U component in region <em>n</em>.
   *  \n <tt>sse_info[<em>n</em>][2]</tt>: the SSE for the V component in region <em>n</em>. */
  u64 sse_info[8][3];
  /** \brief The QP histogram. */
  u32 qp_hist[52];
  /** \brief The POC of the frame. */
  u32 picPoc;
  /** \brief The number of slices in the frame. */
  u32 picSliceNum;
  /** \brief The number of intra blocks in each 8x8 unit. */
  u32 picNumIntra;
  /** \brief The initial QP of the frame, which refers to the <tt>sliceQp</tt> in the slice header. */
  u8  startQp;
  /** \brief Whether the CUs in the frame are skip CUs.
   *  \n <tt>0</tt>: the CUs in the frame are not skip CUs.
   *  \n <tt>1</tt>: the CUs in the frame are skip CUs. */
  u8  isPSkip;
  /** \brief The type of the frame.
   *  \n For the available values, see Section <em> \ref VCEncPictureCodingType</em>. */
  u8  picType;
  /** \brief The mean QP of the frame. */
  u8  meanQp;
  /** \brief The index of the picture in the current GOP.
   *  \n The pictures are indexed starting from 0 at each IDR frame. */
  u32 gopPicIdx;
  /** \brief The number of the picture. */
  u32 picNum;
  /** \brief The Sum luma of the picture. */
  u64 pixLumaSum;
  /** \brief The Max luma of the picture. */
  u16  pixLumaMax;
  /** \brief The min luma of the picture. */
  u16  pixLumaMin;
};
/** @} */

/**
 * \addtogroup api_video
 *
 * @{
 */

/** \brief The prototype of the callback function that is called when a line buffer interrupt is
 * detected during encoding of a frame. */
typedef void (*EncInputLineBufCallBackFunc)(void *pAppData);

/** \brief The prototype of the callback function that is called when a stream segment interrupt is
 * detected during encoding of a frame. */
typedef void (*EncStreamMultiSegCallBackFunc)(void *pAppData);

/** @} */


typedef struct {
  u32 inputLineBufEn;         /* enable input image control signals */
  u32 inputLineBufLoopBackEn; /* input buffer loopback mode enable */
  u32 inputLineBufDepth;      /* input loopback buffer size in mb lines */
  u32 inputLineBufHwModeEn;   /* hw handshake mode */
  u32 amountPerLoopBack;      /* Handshake sync amount for every loopback */
  u32 wrCnt;
  EncInputLineBufCallBackFunc
      cbFunc;     /* call back function for line buffer interrupt */
  void *cbData;   /* call back function data for line buffer interrupt */
  u32 initSegNum; /* The number of segments which input data is stored for SBI */
  u32 sbi_id_0;   /* flexa sbi id 0 */
  u32 sbi_id_1;   /* flexa sbi id 1 */
  u32 sbi_id_2;   /* flexa sbi id 2 */
  u32 segmentUnitHeight;   /* flexa sbi segment unit height */
  u32 enable_slice_irq;
} inputLineBuf_s;

typedef struct {
  u32 streamMultiSegmentMode; /* 0:single segment 1:multi-segment with no sync 2:multi-segment with sw handshake mode*/
  u32 streamMultiSegmentSize;
  u32 streamMultiSegmentAmount; /* total segment amount must be more than 1*/
  u32 streamMultiSegmentOffset;
  u32 rdCnt;
  EncStreamMultiSegCallBackFunc
      cbFunc;   /* call back function for segment ready interrupt */
  void *cbData; /* call back function data for segment ready interrupt */
} streamMultiSeg_s;

/* Reference frame Mv info for tmv */
struct MvInfo {
  u8 pred_mode;
  u8 pred_dir;
  u8 partMode;
  u8 cuSize;
  u8 puIndex;
  short m_iHor[2];
  short m_iVer[2];
  i32 refIdx[2];
  u8 longTermRef[2];
};
#undef Pel
#if 0
typedef       unsigned char       Pel;        ///< 8-bit pixel type
#else
typedef unsigned short Pel;  // 16-bit pixel type
#endif

/* deblock info when multi-tile */
typedef struct {
  //data
  Pel leftY[16];
  Pel leftC[16];

  //ctrl info
  i32 bmvy;
  i32 bmvx;
  i32 fmvy;
  i32 fmvx;
  u8 bmvid;
  u8 fmvid;
  u8 tile_border;  //above or below
  /* HW-SW_size: 0-8, 1-16, 2-32, 3-64 */
  u8 cu_size;
  u8 pu_split_type;
  u8 tu_border;
  u8 pcm_flag;
  u8 block_coded_block_flag;
  u8 block_bi_dir;
  u8 block_qp;
  u8 block_intra;
  /* HW-SW_size: 0-4, 1-8, 2-16, 3-32 */
  u8 tu_size;
} debTileCtx;

typedef struct {  // per CTB
  // control
  u8 sao_filter_type[2];       // luma/chroma; disable/EO/BO
  u8 sao_filter_info[3];       // yuv; eo_class or band position
  u8 left_sao_disable;         // disable sao for PCM
  i8 sao_filter_offset[3][4];  // yuv; offsets

  // data
  Pel leftY[16][4][5];    // [blkIdxY]
  Pel leftC[2][8][4][5];  // [cb/cr][blkIdxY]
} saoTileCtx;

#define DEB_TILE_SYNC_SIZE ((sizeof(debTileCtx)) * 16)
#define SAODEC_TILE_SYNC_SIZE (sizeof(saoTileCtx))
#define SAOPP_TILE_SYNC_SIZE (2 * sizeof(Pel) * 16)

/* the max ctb size allowed for hevc */
#define MAX_CTB_SIZE 64

/* Masks for encOutputMbInfo.mode */
#define MBOUT_9_MODE_MASK 0x0F
#define MBOUT_9_CHROMA_MODE_MASK 0x30
#define MBOUT_9_REFIDX_MASK 0xC0

/* MB output information. Amount of data/mb in 16-bit words / bytes.
   This is how things used to be before Foxtail release.
   Older models rely on these defines.
   For Foxtail data has increased to 48 bytes/MB, this is defined in model. */
#define MBOUT_16 16
#define MBOUT_8 32

/* 16-bit pointer offset to field. */
#define MBOUT_16_BITCOUNT 1
#define MBOUT_16_MV_B0_X 4
#define MBOUT_16_MV_B1_X 5
#define MBOUT_16_MV_B2_X 6
#define MBOUT_16_MV_B3_X 7
#define MBOUT_16_INTRA_SAD 12
#define MBOUT_16_INTER_SAD 13

/* 8-bit pointer offset to field. */
#define MBOUT_8_MODE 0
#define MBOUT_8_NONZERO_CNT 1
#define MBOUT_8_MV_B0_Y 4
#define MBOUT_8_MV_B1_Y 5
#define MBOUT_8_MV_B2_Y 6
#define MBOUT_8_MV_B3_Y 7
#define MBOUT_8_I4X4 16
#define MBOUT_8_INPUT_MAD 28
#define MBOUT_8_LUMA_MEAN 29
#define MBOUT_8_CB_MEAN 30
#define MBOUT_8_CR_MEAN 31

#define MBOUT_8_BOOS_QP 32
#define MBOUT_8_VAR_REC_LUM 34 /* varRecon[0] */
#define MBOUT_8_VAR_INP_LUM 42 /* varInput[0] */

/* allocate 64 bytes for sliceinfo data */
#define SLICEINFO_SIZE (64)

/* General tools */
#define ABS(x) ((x) < (0) ? -(x) : (x))
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#define SIGN(a) ((a) < (0) ? (-1) : (1))
#define CLIP3(x, y, z) ((z) < (x) ? (x) : ((z) > (y) ? (y) : (z)))
#define WIDTH_64_ALIGN(w) (((w) + 63) & (~63))
#define RAND(a, b) (rand() % ((b) - (a) + 1) + (a))
#define SWAP(a, b, type) \
  {                      \
    type tmp;            \
    tmp = a;             \
    a = b;               \
    b = tmp;             \
  }
#define STRIDE(variable, alignment) \
  (((variable) + (alignment) - 1) & (~((alignment) - 1)))

#define WIENER_DENOISE (1)
#if WIENER_DENOISE
#define USE_TOP_CTRL_DENOISE (1)
#define FIX_POINT_BIT_WIDTH (10)
#define SIG_RECI_BIT_WIDTH (12)
#define MB_NUM_BIT_WIDTH (20)

#define SNR_MAX (2 << FIX_POINT_BIT_WIDTH)
#define SNR_MIN (1 << FIX_POINT_BIT_WIDTH)
#define SIGMA_RANGE (20)
#define SIGMA_MAX (30)
#define FILTER_STRENGTH_H (1 << FIX_POINT_BIT_WIDTH)
#define FILTER_STRENGTH_L (1 << (FIX_POINT_BIT_WIDTH - 1))
#define FILTER_STRENGTH_PARAM \
  ((FILTER_STRENGTH_H - FILTER_STRENGTH_L) / SIGMA_RANGE)
#define SIGMA_SMOOTH_NUM (5)
#endif

#define DENOISE_3DNR
#ifdef DENOISE_3DNR
#define DENOISE_3DNR_UV                  (1)            // 1 to enable denoise for chroma
// #define DENOISE_3DNR_UV_MVUSING_YME1N
#define DENOISE_3DNR_UV_MVUSING_YMEMD

#define DENOISE_3DNR_INTRAUSENLM3D       (1)            // 1 to enable denoise for I-frame using ME1N
#define DENOISE_RECALC_INTRABLK_SATD     (0)            // 1 to enable re-calculate intra satd using denoised yuv, will copy deniose yuv back before cu_tree_decision()
                                                        // 0 to copy denoise yuv back for ctu_coding() only
#define DENOISE_PRECODE_1STFRAME         (0)            // pre-code 1st I-slice, using its input copy as reference frame.
#endif

#define MOTION_LAMBDA_SSE_SHIFT 6
#define MOTION_LAMBDA_SAD_SHIFT 5
#define IMS_LAMBDA_SAD_SHIFT 6
#define MOTION_LAMBDA_PSY_SHIFT 6

#define AQ_NONE 0
#define AQ_VARIANCE 1
#define AQ_AUTO_VARIANCE 2
#define AQ_AUTO_VARIANCE_BIASED 3

#define QPFACTOR_FIX_POINT 14
#define LAMBDA_FIX_POINT 10
#define SUBJECT_THRESH_FIX_POINT 6

#define SSIM_FIX_POINT_FOR_8BIT 16
#define SSIM_FIX_POINT_FOR_10BIT 24

#define DOWN_SCALING_MAX_WIDTH 8192
//#define SEARCH_RANGE_ROW_OFFSET_TEST

#define CTB_RC_BUF_NUM 4

#define GMV_STEP_X 64
#define GMV_STEP_Y 16

/**
 * \addtogroup api_video
 *
 * @{
 */
/** \brief The supported maximum number of stream buffers. */
#define MAX_STRM_BUF_NUM 2

/** \brief The supported maximum number of OSD regions. */
#define MAX_OVERLAY_NUM 2//12//FIXME@RAMOPT

/** \brief The supported maximum number of mosaic regions. */
#define MAX_MOSAIC_NUM 2//12//FIXME@RAMOPT

/** \brief The supported maximum number of MB. */
#define MAX_LINE_MB_NUM 2048

/** \brief The supported maximum number of OSD map colors, which is the same as maximum number of OSD regions. */
#define MAX_OSDMAP_COLOR_NUM MAX_OVERLAY_NUM

/** @} */

#define SUBJECTIVE_TUNING
#ifdef SUBJECTIVE_TUNING
#define SUBJECTIVE_LAMBDA_TUNING
#define LA_RC_VBV
#define H264_LA_INTRA_MODE_NON_4x4
#define H264_LA_INTRA_MODE_NON_8x8
#define LA_REFERENCE_USE_INPUT
#define H264_LA_BLOCK_SIZE 2
//#define CUTREE_FRAME_COST_NBORDER
//#define H264_DECIMATE
#define RDOQ_LAMBDA_ADJ
//#define DISABLE_LOWDELAYB_FOR_P
#define BILINEAR_DOWNSAMPLE

#define LA_HEVC_SIMPLE_RDO
#define SUBJECTIVE_INITIAL_QP_TUNING
#define LA_INTRA_BY_SATD
//#define GLOBAL_MV_ON_SEARCH_RANGE
//#define PSY_RDOQ
#endif

// optimazation scabac performance for HW
// #define RDO_SCABAC_SIMPLE_EN

// #define TILE_CMODEL_CTB_NUM
#define TILE_CMODEL_SIMULATION

/* macros to improve hevc quality of psnr */
#define VCE_VIDEO_QUALITY_IMPROVE
#ifdef VCE_VIDEO_QUALITY_IMPROVE
/* tools tuning */
#define HEVC_QUALITY_LA_TUNE
#define RDO_CHECK_ZERO_TU
#define REFINE_MEXN_BINCOST

/* rate control */
#define VBR_RC 2
#define FIRST_I_TUNE 1
#define RC_MODEL_DECAY 1.0
#define RC_MODEL_MIN_DIV 2
#define RC_IFRAME_EST_WITH_DELTA 1

#else
#define VBR_RC 0
#define FIRST_I_TUNE 0
#define RC_MODEL_DECAY 0.5
#define RC_MODEL_MIN_DIV 4
#define RC_IFRAME_EST_WITH_DELTA 0
#endif

#define SRD_ENABLE 1
/** * below are const parameters, not for control *** */
#define RDOQ_LMD_INTRA_FACTOR 0.65
#define RDOQ_LMD_INTER_FACTOR 0.75
#define RDOQ_LMD_INTRA_FACTOR_SCALED                    \
  (u32)(RDOQ_LMD_INTRA_FACTOR * RDOQ_LMD_INTRA_FACTOR * \
        (1 << QPFACTOR_FIX_POINT))
#define RDOQ_LMD_INTER_FACTOR_SCALED                    \
  (u32)(RDOQ_LMD_INTER_FACTOR * RDOQ_LMD_INTER_FACTOR * \
        (1 << QPFACTOR_FIX_POINT))

#define MAX_SEGMENTS 8
static const int segment_delta_qp[] = {-8, -6, -4, -2, 0, 2, 4, 6};

#define CDEF_STRENGTH_NUM 8

#define VCENC_FREE(p) \
  do {                \
    if (p) {          \
      EWLfree(p);     \
      p = NULL;       \
    }                 \
  } while (0)

#ifdef SUPPORT_AV1
//#include "av1enccommon.h"
#endif

#ifdef SUPPORT_VP9
#include "vp9enccommon.h"
#endif

#define MC_SYNC_WORD_BYTES (64 * 2)

#define CH_WIDTH_SHIFT(codedChromaIdc) (codedChromaIdc == VCENC_CHROMA_IDC_444 ? 0 : 1)
#define CH_HEIGHT_SHIFT(codedChromaIdc) ((codedChromaIdc == VCENC_CHROMA_IDC_444 || codedChromaIdc == VCENC_CHROMA_IDC_422) ? 0 : 1)

#define VISUAL_INTRA_BIAS_MAX_STRENGTH 15
#define VISUAL_CHROMA_DETECTION_MAX_STRENGTH 10

#ifdef __cplusplus
}
#endif

#endif

/**
 * Get HW configuration.
 *
 * \param [in] codecFormat The codec format.
 *
 * \return EWLHwConfig_t structure containing the ASIC hardware configuration. Refer to the Hantro
 * EWL API document for the definition of EWLHwConfig_t.
 */
const EWLHwConfig_t *EncGetAsicConfig(u32 codecFormat, const void *ctx);
u32 EncAsicGetAsicHWid(u32 client_type, const void *ctx);
