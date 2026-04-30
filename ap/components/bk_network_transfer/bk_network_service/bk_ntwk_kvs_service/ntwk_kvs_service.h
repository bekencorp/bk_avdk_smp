#pragma once

#include <stdint.h>
#include "network_type.h"
#include <common/avdk_pixel_types.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>

#include <com/amazonaws/kinesis/video/webrtcclient/Include.h>

#ifdef __cplusplus
extern "C" {
#endif

VOID ntwk_kvs_on_data_channel(UINT64 customData, PRtcDataChannel pRtcDataChannel);

bk_err_t ntwk_kvs_init(void);
bk_err_t ntwk_kvs_deinit(void);

bk_err_t ntwk_kvs_ctrl_chan_start(void *param);
bk_err_t ntwk_kvs_ctrl_chan_stop(void);
int ntwk_kvs_ctrl_send_packet(uint8_t *data, uint32_t length);

bk_err_t ntwk_kvs_video_chan_start(void *param);
bk_err_t ntwk_kvs_video_chan_stop(void);
int ntwk_kvs_video_send_packet(uint8_t *data, uint32_t length, image_format_t video_type);

bk_err_t ntwk_kvs_audio_chan_start(void *param);
bk_err_t ntwk_kvs_audio_chan_stop(void);
int ntwk_kvs_audio_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type);

bk_err_t ntwk_kvs_ctrl_register_receive_cb(int (*cb)(uint8_t *data, uint32_t length));
bk_err_t ntwk_kvs_video_register_receive_cb(int (*cb)(uint8_t *data, uint32_t length));
bk_err_t ntwk_kvs_audio_register_receive_cb(int (*cb)(uint8_t *data, uint32_t length));

#ifdef __cplusplus
}
#endif
