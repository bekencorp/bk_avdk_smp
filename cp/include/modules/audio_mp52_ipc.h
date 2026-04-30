#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEC_M52_SLOT_MAGIC                (0x41354332u) /* "A5C2" */
#define AEC_M52_SLOT_COUNT                (2u)

enum {
    AEC_M52_IPC_CMD_RUN = 1,
    AEC_M52_IPC_CMD_DONE,
    AEC_M52_IPC_CMD_CTRL,
};

enum {
    AEC_M52_IPC_STATUS_OK = 0,
    AEC_M52_IPC_STATUS_BUSY,
    AEC_M52_IPC_STATUS_INVALID,
    AEC_M52_IPC_STATUS_PROC_FAIL,
};

typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t ref_bytes;
    uint32_t mic_bytes;
    uint32_t out_bytes;
    int16_t *ref_addr;
    int16_t *mic_addr;
    int16_t *out_addr;
    uint32_t ecout_bytes;
    uint8_t *ecout_addr;
    uint8_t aec_test;
    uint8_t reserved_u8[3];
    int16_t aec_spcnt;
    int16_t aec_dcnt;
    int32_t aec_dc;
    int32_t aec_mic_max;
    int32_t aec_vad_hr;
    int32_t aec_phs_cur;
} aec_m52_slot_desc_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t fs;
    uint32_t frame_bytes;
    uint32_t ref_bytes;
    uint32_t mic_bytes;
    uint32_t out_bytes;
    uint16_t init_flags;
    uint16_t reserved_u16;
    uint32_t delay_points;
    uint32_t ec_depth;
    uint8_t ref_scale;
    uint8_t voice_vol;
    uint8_t ns_type;
    uint8_t ns_filter;
    uint8_t ns_level;
    uint8_t ns_para;
    uint8_t drc;
    uint8_t ec_filter;
    uint8_t interweave;
    int16_t dist;
    uint8_t mic_swap;
    uint8_t ec_only_output;
    uint8_t dual_perp;
    uint8_t multi_output_use_ec_out;
    uint8_t dual_ch;
    uint8_t vad_enable;
    uint16_t reserved_tail_u16;
    uint32_t max_delay_points;
    int32_t phs_s1;
    uint8_t spthr_valid;
    uint8_t reserved_tail_u8[3];
    int16_t spthr[14];
} aec_m52_ctrl_cfg_t;

#define AEC_M52_CTRL_MAGIC               (0x41454343u) /* "AECC" */
#define AEC_M52_CTRL_VERSION             (1u)

#ifdef __cplusplus
}
#endif
