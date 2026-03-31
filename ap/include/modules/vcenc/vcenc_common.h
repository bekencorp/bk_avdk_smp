#pragma once

#include "os/os.h"

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
} vcenc_ret_e;

typedef enum
{
	VCENC_OUT_IFRAME,
	VCENC_OUT_PFRAME,
	VCENC_OUT_BFRAME,
	VCENC_OUT_HDR,
	VCENC_OUT_ENDING,
} venc_out_type_e;

typedef enum {
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

	SYNTAX_PROFILE_H264_CAVLC_444 = 44,
	/** The H.264 Baseline profile. */
	SYNTAX_PROFILE_H264_BASELINE = 66,
	/** The H.264 Main profile. */
	SYNTAX_PROFILE_H264_MAIN = 77,
	/** The H.264 Extended profile. */
	SYNTAX_PROFILE_H264_EXTENDED = 88,
	/** The H.264 High profile. */
	SYNTAX_PROFILE_H264_HIGH = 100,
	/** The H.264 High 10 profile. */
	SYNTAX_PROFILE_H264_HIGH_10 = 110,
} vcenc_profile_e;

typedef enum {
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
} vcenc_level_e;

typedef enum
{
	/** \brief (Only for H.265/AVC) YUV400. */
	VCENC_CHROMA_IDC_400 = 0,
	/** \brief YUV420. */
	VCENC_CHROMA_IDC_420 = 1,
	/** \brief (Under development) YUV422. */
	VCENC_CHROMA_IDC_422 = 2,
	VCENC_CHROMA_IDC_444 = 3,
} vcenc_chroma_idc_e;

//sync with reg38[28:31]
typedef enum 
{
	VCENC_INPUT_YUV420P,
	VCENC_INPUT_NV12,
	VCENC_INPUT_YUV422P,
	VCENC_INPUT_YUV444P,
} vcenc_input_e;

/* See table Table 7-1 NAL unit type codes and NAL unit type classes */
typedef enum {
  TRAIL_N = 0,  // 0
  TRAIL_R,      // 1
  TSA_N,        // 2
  TSA_R,        // 3
  STSA_N,       // 4
  STSA_R,       // 5
  RADL_N,       // 6
  RADL_R,       // 7
  RASL_N,       // 8
  RASL_R,       // 9
  RSV_VCL_N10,
  RSV_VCL_R11,
  RSV_VCL_N12,
  RSV_VCL_R13,
  RSV_VCL_N14,
  RSV_VCL_R15,

  BLA_W_LP = 16,
  BLA_W_RADL = 17,
  BLA_N_LP = 18,
  IDR_W_RADL = 19,
  IDR_N_LP = 20,
  CRA_NUT = 21,
  RSV_IRAP_VCL22 = 22,
  RSV_IRAP_VCL23 = 23,
  VPS_NUT = 32,
  SPS_NUT = 33,
  PPS_NUT = 34,
  AUD_NUT = 35,
  EOS_NUT = 36,
  EOB_NUT = 37,
  FD_NUT = 38,
  PREFIX_SEI_NUT = 39,
  SUFFIX_SEI_NUT = 40,

  /* Reference picture sets uses same store than vps/sps/vps */
  RPS = 64,

  /*-----------------------------
	H264 Defination
      -----------------------------*/
  H264_NONIDR = 1,  /* Coded slice of a non-IDR picture */
  H264_IDR = 5,     /* Coded slice of an IDR picture */
  H264_SEI = 6,     /* SEI message */
  H264_SPS_NUT = 7, /* Sequence parameter set */
  H264_PPS_NUT = 8, /* Picture parameter set */
  H264_AUD_NUT = 9,
  H264_ENDOFSEQUENCE = 10, /* End of sequence */
  H264_ENDOFSTREAM = 11,   /* End of stream */
  H264_FILLERDATA = 12,    /* Filler data */
  H264_PREFIX = 14,        /* Prefix */
  H264_SSPSET = 15,        /* Subset sequence parameter set */
  H264_MVC = 20            /* Coded slice of a view picture */
} nal_type_e;

typedef enum
{
	SBI_STREAM_ID_Y = 0x14,
	SBI_STREAM_ID_CB = 0x15,
	SBI_STREAM_ID_CR = 0x16,
} sbi_stream_id_e;

typedef enum
{
	VCENC_FRAME_MODE = 0,
	VCENC_SW_SLICE_MODE,
	VCENC_HW_SLICE_MODE,
	// FRAME_TYPE_B,
} vcenc_mode_e;


typedef void *vcenc_handle;
typedef void (*vcenc_frame_done_cb)(void*, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t (*vcenc_slice_done_cb)(uint8_t *, uint8_t *, uint8_t *, uint32_t);

typedef struct h264_enc_param_t
{
	vcenc_handle instance;
	uint16_t width;
	uint16_t height;
	vcenc_mode_e enc_mode; /*0:frame mode 1:hw slice mode 2:sw slice mode*/
	uint32_t slice_count; /* only valid for hw/sw slice mode */
	uint32_t in_buffer;
	uint32_t in_lines;
	vcenc_input_e in_type;
	void *out_frame;
	uint32_t out_buffer;
	uint32_t out_len;
	uint32_t idr_interval;
	uint32_t update_flag;
	uint32_t force_idr_flag;
	vcenc_frame_done_cb frame_done_cb; /*callback of frame or sps/pps done*/
	vcenc_slice_done_cb slice_done_cb; /*callback of 16 lines done*/
  uint32_t args;
} h264_enc_param_t;