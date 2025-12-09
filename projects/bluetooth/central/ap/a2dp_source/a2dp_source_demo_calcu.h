#pragma once

#include <stdint.h>
#include <stdio.h>

enum
{
    //EVENT_BT_AUDIO_INIT_REQ,
	//EVENT_BT_AUDIO_DEINIT_REQ,
	EVENT_BT_PCM_RESAMPLE_INIT_REQ,
	EVENT_BT_PCM_RESAMPLE_DEINIT_REQ,
	EVENT_BT_PCM_RESAMPLE_REQ,
	EVENT_BT_PCM_ENCODE_INIT_REQ,
	EVENT_BT_PCM_ENCODE_DEINIT_REQ,
	EVENT_BT_PCM_ENCODE_REQ,

    EVENT_BT_PCM_EXIT,
};

typedef struct
{
    aud_rsp_cfg_t rsp_cfg;
} bt_audio_resample_init_req_t;

typedef struct
{
    uint8 *in_addr;
    uint32_t *in_bytes_ptr;
    uint8_t *out_addr;
    uint32_t *out_bytes_ptr;
} bt_audio_resample_req_t;

typedef struct
{
    void *handle;
    uint8_t type;
    uint8_t *in_addr;
    uint32_t *out_len_ptr;
} bt_audio_encode_req_t;

typedef struct
{
    uint8_t type;

    union
    {
        bt_audio_resample_init_req_t rsp_init;
        bt_audio_resample_req_t rsp_req;
        bt_audio_encode_req_t encode_req;
    };

    beken_semaphore_t *ret_sem;

} a2dp_source_calcu_req_t;

bk_err_t a2dp_source_demo_calcu_init(void);
bk_err_t a2dp_source_demo_calcu_deinit(void);

bk_err_t a2dp_source_demo_calcu_rsp_init_req(bt_audio_resample_init_req_t *req, uint8_t is_init);
bk_err_t a2dp_source_demo_calcu_rsp_req(bt_audio_resample_req_t *req);
bk_err_t a2dp_source_demo_calcu_encode_init_req(bt_audio_encode_req_t *req, uint8_t is_init);
bk_err_t a2dp_source_demo_calcu_encode_req(bt_audio_encode_req_t *req);
