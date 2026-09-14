#pragma once

/**
 * @file vcenc_types.h
 * @brief Common public types shared by all vcenc codec front-ends (JPEG / H264 / ...).
 *
 * This header only contains types that are codec-agnostic AND that the SDK's
 * external consumers actually reference:
 *   - return codes (vcenc_ret_e)
 *   - output frame type tag (venc_out_type_e)
 *   - encoding mode (vcenc_mode_e)
 *   - input pixel layout (vcenc_input_e)
 *   - opaque encoder handle (vcenc_handle)
 *   - frame / slice completion callbacks
 *   - shared rate-control / noise-reduction parameters
 *
 * Module-private enums (sbi_stream_id_e, vcenc_chroma_idc_e, vcenc_ctbrc_mode_e,
 * nal_type_e) used to live here too but were demoted to vcenc_common_private.h
 * once the audit showed no external caller relied on them.
 *
 * Codec-specific public types live in dedicated headers:
 *   - vcenc_jpeg_types.h  (JPEG-only types, e.g. jpeg_enc_param_t)
 *   - vcenc_h264_types.h  (H264-only types, e.g. h264_enc_param_t)
 */

#include <stdint.h>
#include "os/os.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	/** The API call is successful. */
	VCENC_OK = 0,
	/** Encoding of a frame is finished. */
	VCENC_FRAME_READY = (1 << 0),
	/** A frame is inside the encoder's internal queue but not encoded. It will be encoded and output
	 *  in a subsequent encoder API call. */
	VCENC_FRAME_ENQUEUE = (1 << 1),
	/** A encoding frame task is cached and will drive the hardware in batch after accumulate some tasks. */
	VCENC_FRAME_CACHE = (1 << 2),
	/** A sideline frame is encoded and application should continue to execute job in the working queue. */
	VCENC_FRAME_CONTINUE = (1 << 3),
	/** (Error) An encoder error occurs. */
	VCENC_ERROR = -1,
	/** (Error) A pointer argument has an invalid NULL value. */
	VCENC_NULL_ARGUMENT = -2,
	/** (Error) One of the arguments is invalid. */
	VCENC_INVALID_ARGUMENT = -3,
	/** (Error) The encoder fails to allocate memory. */
	VCENC_MEMORY_ERROR = -4,
	/** (Error) Initialization of the encoder system interface fails. */
	VCENC_EWL_ERROR = -5,
	/** (Error) The EWL fails to allocate memory. */
	VCENC_EWL_MEMORY_ERROR = -6,
	/** (Error) The stream is started with HRD enabled and rate control parameters fails to be altered. */
	VCENC_INVALID_STATUS = -7,
	/** (Error) The output buffer is too small to hold the generated stream. */
	VCENC_OUTPUT_BUFFER_OVERFLOW = -8,
	/** (Error) Memory access fails due to invalid bus address. */
	VCENC_HW_BUS_ERROR = -9,
	/** (Error) An error occurs in the hardware data. */
	VCENC_HW_DATA_ERROR = -10,
	/** (Error) Hardware execution has timed out. */
	VCENC_HW_TIMEOUT = -11,
	/** (Error) The hardware fails to be reserved for exclusive access. */
	VCENC_HW_RESERVED = -12,
	/** (Error) A fatal system error occurs and the encoding is terminated. */
	VCENC_SYSTEM_ERROR = -13,
	/** (Error) The encoder instance is invalid or corrupted. */
	VCENC_INSTANCE_ERROR = -14,
	/** (Error) An HRD error occurs. */
	VCENC_HRD_ERROR = -15,
	/** The hardware is reset by an external operation. */
	VCENC_HW_RESET = -16,
	/** (Error) Excessive cycles have been used to poll for an input row to be encoded. */
	VCENC_HW_POLL_SLICEINFO_TIMEOUT = -17,
	/** (Error) An UFBC decoding error occurs when the input picture is fetched. */
	VCENC_HW_UFBC_ERROR = -18
} vcenc_ret_e;

typedef enum {
	VCENC_OUT_IFRAME,
	VCENC_OUT_PFRAME,
	VCENC_OUT_BFRAME,
	VCENC_OUT_HDR,
	VCENC_OUT_ENDING,
} venc_out_type_e;

/* sync with reg38[28:31] */
typedef enum {
	VCENC_INPUT_YUV420P,
	VCENC_INPUT_NV12,
	VCENC_INPUT_YUV422P,
	VCENC_INPUT_YUV444P,
} vcenc_input_e;

typedef enum {
	VCENC_FRAME_MODE = 0,
	VCENC_SW_SLICE_MODE,
	VCENC_HW_SLICE_MODE,
} vcenc_mode_e;

/**
 * @brief Rate-control parameters; shared between codec front-ends because the
 * underlying VCENC HW uses the same control fields (only a subset is consumed
 * for JPEG vs H.264).
 */
typedef struct vcenc_rate_ctrl_t {
	int crf;                         /* Constant-rate-factor quality target; -1 disables CRF. */
	uint32_t picture_rc;             /* Enable picture-level rate control. */
	uint32_t ctb_rc;                 /* CTB-level rate-control mode. */
	uint32_t block_rc_size;          /* HW test: HWIF_ENC_RC_BLOCK_SIZE write has no effect; default is 64x64. */
	uint32_t picture_skip;           /* Allow rate control to skip pictures. */
	int qp_hdr;                      /* Forced picture QP; -1 lets rate control choose QP. */
	uint32_t qp_min_pb;              /* Minimum QP for P/B frames. */
	uint32_t qp_max_pb;              /* Maximum QP for P/B frames. */
	uint32_t qp_min_i;               /* Minimum QP for I frames. */
	uint32_t qp_max_i;               /* Maximum QP for I frames. */
	uint32_t bit_per_second;         /* Target bitrate in bit/s; 0 uses the default budget. */
	uint32_t cpb_max_rate;           /* HRD CPB max bitrate in bit/s; 0 disables max-rate cap. */
	uint32_t filler_data;            /* Enable filler data insertion. */
	uint32_t hrd;                    /* Enable HRD virtual buffer model. */
	uint32_t hrd_cpb_size;           /* HRD CPB buffer size in bits; 0 lets RC choose. */
	uint32_t bitrate_window;         /* Moving bitrate window, in frames. */
	int intra_qp_delta;              /* I-frame QP delta from the RC-selected base QP. */
	uint32_t fixed_intra_qp;         /* Fixed I-frame QP when non-zero. */
	int bit_var_range_i;             /* I-frame bit variation range used for min/max I-frame bit budget. */
	int bit_var_range_p;             /* P-frame bit variation range, percent over target frame bits. */
	int bit_var_range_b;             /* B-frame bit variation range, percent over target frame bits. */
	int tol_moving_bit_rate;         /* Moving bitrate tolerance, percent. */
	int monitor_frames;             /* Number of frames monitored by rate control. */
	int target_pic_size;             /* Internal per-picture target bits; set_rate_ctrl does not consume this field. */
	int smooth_psnr_in_gop;          /* Obsolete in current H.264 path; value is stored but not consumed. */
	uint32_t static_scene_i_bit_percent; /* I-frame bit percent used for static scenes. */
	uint32_t rc_qp_delta_range;      /* Block RC QP delta range; larger values may raise max QP and output size; default 10. */
	uint32_t rc_base_mb_complexity;  /* Hardware base MB complexity offset. */
	int pic_qp_delta_min;            /* Minimum picture QP delta. */
	int pic_qp_delta_max;            /* Maximum picture QP delta. */
	int long_term_qp_delta;          /* Long-term reference QP delta. */
	int vbr;                         /* Enable variable bitrate mode. */
	uint32_t rc_mode;                /* Rate-control mode, see VCE_RC_* values. */
	float tol_ctb_rc_inter;          /* CTB RC tolerance for inter blocks. */
	float tol_ctb_rc_intra;          /* CTB RC tolerance for intra blocks. */
	int tol_rc_underflow;            /* Virtual-buffer underflow tolerance, clipped to 0..99. */
	uint32_t max_i_prop;             /* Maximum I-frame bit proportion. */
	uint32_t min_i_prop;             /* Minimum I-frame bit proportion. */
	int change_pos;                  /* RC change position in GOP, valid range 50..100. */
	int ctb_rc_row_qp_step;          /* CTB row QP step. */
	int ctb_rc_row_qp_delta_range;   /* CTB row QP delta range for CTB RC v2. */
	uint32_t ctb_rc_qp_delta_reverse; /* Not connected in current H.264 path; value is stored but not written to HW. */
	uint32_t frame_rate_num;         /* Actual encode fps numerator; wrong fps breaks RC bit budget. default 20 */
	uint32_t frame_rate_denom;       /* Actual encode fps denominator. */
	uint32_t hie_qp_delta_enable;    /* Enable hierarchical QP delta when supported. */
} vcenc_rate_ctrl_t;

/**
 * @brief Noise-reduction parameters; shared between codecs (HW field set).
 */
typedef struct vcenc_nr_t {
	uint32_t noiseReductionEnable;
	uint32_t noiseReductionStrength_IntraY;
	uint32_t noiseReductionStrength_IntraU;
	uint32_t noiseReductionStrength_IntraV;
	uint32_t noiseReductionStrength_InterY;
	uint32_t noiseReductionStrength_InterU;
	uint32_t noiseReductionStrength_InterV;
	uint32_t noiseReduction_ChromaMaxMV;
	uint32_t blocks_threshold;
	uint32_t rd_cost_threshold;
} vcenc_nr_t;

/** Opaque encoder instance handle returned by every codec's _init(). */
typedef void *vcenc_handle;

/** Frame-done callback. Invoked from the encoder ISR/worker context. */
typedef void (*vcenc_frame_done_cb)(void *, uint32_t, uint32_t, uint32_t, uint32_t);

/** Slice / line-buffer-done callback. Invoked from the encoder ISR context. */
typedef uint32_t (*vcenc_slice_done_cb)(uint8_t *, uint8_t *, uint8_t *, uint32_t);

#ifdef __cplusplus
}
#endif
