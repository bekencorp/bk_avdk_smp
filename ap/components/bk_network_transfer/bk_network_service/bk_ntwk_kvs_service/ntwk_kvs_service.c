#include <common/bk_include.h>
#include <os/mem.h>
#include <os/os.h>
#include <components/log.h>

#include "network_transfer_internal.h"
#include "ntwk_kvs_service.h"
#include "ntwk_kvs_bridge.h"

#include <com/amazonaws/kinesis/video/mkvgen/Include.h>
#include <com/amazonaws/kinesis/video/webrtcclient/Include.h>
#include <common/avdk_pixel_types.h>
#include <modules/vcenc/vcenc_common.h>

/* PTS step for writeFrame; align with sample encoder cadence (see doorbell_kvs Samples.h DEFAULT_FPS_VALUE). */
#ifndef NTWK_KVS_VIDEO_FPS
#define NTWK_KVS_VIDEO_FPS 25
#endif
#define NTWK_KVS_VIDEO_FRAME_DURATION (HUNDREDS_OF_NANOS_IN_A_SECOND / (INT64)NTWK_KVS_VIDEO_FPS)
#define NTWK_KVS_AUDIO_FRAME_DURATION (20 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND)

#define TAG "ntwk-kvs"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

typedef int (*ntwk_kvs_rx_cb_t)(uint8_t *data, uint32_t length);

static beken_mutex_t s_kvs_mtx;
static void *s_sess_opaque;
static PRtcRtpTransceiver s_video_tx;
static PRtcRtpTransceiver s_audio_tx;
static PRtcDataChannel s_dc;
static UINT64 s_video_pts;
static UINT64 s_audio_pts;

static ntwk_kvs_rx_cb_t s_ctrl_rx;
static ntwk_kvs_rx_cb_t s_video_rx;
static ntwk_kvs_rx_cb_t s_audio_rx;

static bool ntwk_kvs_is_h264_key(const frame_buffer_t *fb)
{
	if (fb == NULL) {
		return false;
	}
	if (fb->h264_type == (uint32_t)VCENC_OUT_IFRAME) {
		return true;
	}
	return false;
}

#if CONFIG_KVS_NTWK_BRIDGE

void ntwk_kvs_bridge_attach_session(void *streaming_session, PRtcRtpTransceiver video_transceiver,
				    PRtcRtpTransceiver audio_transceiver)
{
	if (rtos_lock_mutex(&s_kvs_mtx) != kNoErr) {
		return;
	}
	s_sess_opaque = streaming_session;
	s_video_tx = video_transceiver;
	s_audio_tx = audio_transceiver;
	rtos_unlock_mutex(&s_kvs_mtx);
	ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, 0, NTWK_TRANS_CHAN_CTRL);
	ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, 0, NTWK_TRANS_CHAN_VIDEO);
	ntwk_msg_event_report(NTWK_TRANS_EVT_CONNECTED, 0, NTWK_TRANS_CHAN_AUDIO);
}

void ntwk_kvs_bridge_detach_session(void *streaming_session)
{
	if (rtos_lock_mutex(&s_kvs_mtx) != kNoErr) {
		return;
	}
	if (s_sess_opaque == streaming_session) {
		s_sess_opaque = NULL;
		s_video_tx = NULL;
		s_audio_tx = NULL;
		s_dc = NULL;
		s_video_pts = 0;
		s_audio_pts = 0;
	}
	rtos_unlock_mutex(&s_kvs_mtx);
	ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, 0, NTWK_TRANS_CHAN_CTRL);
	ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, 0, NTWK_TRANS_CHAN_VIDEO);
	ntwk_msg_event_report(NTWK_TRANS_EVT_DISCONNECTED, 0, NTWK_TRANS_CHAN_AUDIO);
}

#endif /* CONFIG_KVS_NTWK_BRIDGE */

static VOID ntwk_kvs_dc_on_message(UINT64 customData, PRtcDataChannel pDataChannel, BOOL isBinary, PBYTE pMessage, UINT32 pMessageLen)
{
	(void)customData;
	(void)pDataChannel;
	(void)isBinary;
	if (s_ctrl_rx != NULL && pMessage != NULL && pMessageLen > 0) {
		s_ctrl_rx(pMessage, pMessageLen);
	}
}

VOID ntwk_kvs_on_data_channel(UINT64 customData, PRtcDataChannel pRtcDataChannel)
{
	(void)customData;
	if (pRtcDataChannel == NULL) {
		return;
	}
	if (rtos_lock_mutex(&s_kvs_mtx) != kNoErr) {
		return;
	}
	s_dc = pRtcDataChannel;
	rtos_unlock_mutex(&s_kvs_mtx);
	dataChannelOnMessage(pRtcDataChannel, 0, ntwk_kvs_dc_on_message);
}

static VOID ntwk_kvs_video_in(UINT64 customData, PFrame pFrame)
{
	(void)customData;
	if (s_video_rx == NULL || pFrame == NULL || pFrame->frameData == NULL || pFrame->size == 0) {
		return;
	}
	s_video_rx(pFrame->frameData, pFrame->size);
}

static VOID ntwk_kvs_audio_in(UINT64 customData, PFrame pFrame)
{
	(void)customData;
	if (s_audio_rx == NULL || pFrame == NULL || pFrame->frameData == NULL || pFrame->size == 0) {
		return;
	}
	s_audio_rx(pFrame->frameData, pFrame->size);
}

bk_err_t ntwk_kvs_init(void)
{
	os_memset(&s_kvs_mtx, 0, sizeof(s_kvs_mtx));
	if (rtos_init_mutex(&s_kvs_mtx) != kNoErr) {
		return BK_FAIL;
	}
	s_sess_opaque = NULL;
	s_video_tx = NULL;
	s_audio_tx = NULL;
	s_dc = NULL;
	s_video_pts = 0;
	s_audio_pts = 0;
	return BK_OK;
}

bk_err_t ntwk_kvs_deinit(void)
{
	if (rtos_lock_mutex(&s_kvs_mtx) == kNoErr) {
		s_sess_opaque = NULL;
		s_video_tx = NULL;
		s_audio_tx = NULL;
		s_dc = NULL;
		rtos_unlock_mutex(&s_kvs_mtx);
	}
	rtos_deinit_mutex(&s_kvs_mtx);
	os_memset(&s_kvs_mtx, 0, sizeof(s_kvs_mtx));
	return BK_OK;
}

bk_err_t ntwk_kvs_ctrl_chan_start(void *param)
{
	(void)param;
	return BK_OK;
}

bk_err_t ntwk_kvs_ctrl_chan_stop(void)
{
	return BK_OK;
}

int ntwk_kvs_ctrl_send_packet(uint8_t *data, uint32_t length)
{
	STATUS st;
	if (data == NULL || length == 0) {
		return -1;
	}
	if (rtos_lock_mutex(&s_kvs_mtx) != kNoErr) {
		return -1;
	}
	PRtcDataChannel dc = s_dc;
	rtos_unlock_mutex(&s_kvs_mtx);
	if (dc == NULL) {
		return -1;
	}
	st = dataChannelSend(dc, TRUE, (PBYTE)data, length);
	return (st == STATUS_SUCCESS) ? (int)length : -1;
}

bk_err_t ntwk_kvs_video_chan_start(void *param)
{
	PRtcRtpTransceiver video_tx = NULL;
	(void)param;
	if (rtos_lock_mutex(&s_kvs_mtx) != kNoErr) {
		return BK_FAIL;
	}
	video_tx = s_video_tx;
	rtos_unlock_mutex(&s_kvs_mtx);
	if (video_tx != NULL && s_video_rx != NULL) {
		transceiverOnFrame(video_tx, 0, ntwk_kvs_video_in);
	}
	return BK_OK;
}

bk_err_t ntwk_kvs_video_chan_stop(void)
{
	return BK_OK;
}

int ntwk_kvs_video_send_packet(uint8_t *data, uint32_t length, image_format_t video_type)
{
	STATUS st;
	Frame frame;
	frame_buffer_t *fb = (frame_buffer_t *)data;
	PRtcRtpTransceiver video_tx = NULL;

	(void)video_type;
	(void)length;
	if (fb == NULL || fb->frame == NULL || fb->length == 0) {
		return -1;
	}

	if (rtos_lock_mutex(&s_kvs_mtx) != kNoErr) {
		return -1;
	}
	video_tx = s_video_tx;
	rtos_unlock_mutex(&s_kvs_mtx);
	if (video_tx == NULL) {
		return -1;
	}

	os_memset(&frame, 0, sizeof(frame));
	frame.version = FRAME_CURRENT_VERSION;
	frame.frameData = fb->frame;
	frame.size = fb->length;
	frame.presentationTs = s_video_pts;
	s_video_pts += NTWK_KVS_VIDEO_FRAME_DURATION;
	if (ntwk_kvs_is_h264_key(fb)) {
		frame.flags = FRAME_FLAG_KEY_FRAME;
	}

	st = writeFrame(video_tx, &frame);
	if (st == STATUS_SRTP_NOT_READY_YET) {
		return 0;
	}
	return (st == STATUS_SUCCESS) ? (int)fb->length : -1;
}

bk_err_t ntwk_kvs_audio_chan_start(void *param)
{
	PRtcRtpTransceiver audio_tx = NULL;
	(void)param;
	if (rtos_lock_mutex(&s_kvs_mtx) != kNoErr) {
		return BK_FAIL;
	}
	audio_tx = s_audio_tx;
	rtos_unlock_mutex(&s_kvs_mtx);
	if (audio_tx != NULL && s_audio_rx != NULL) {
		transceiverOnFrame(audio_tx, 0, ntwk_kvs_audio_in);
	}
	return BK_OK;
}

bk_err_t ntwk_kvs_audio_chan_stop(void)
{
	return BK_OK;
}

int ntwk_kvs_audio_send_packet(uint8_t *data, uint32_t length, audio_enc_type_t audio_type)
{
	STATUS st;
	Frame frame;
	PRtcRtpTransceiver audio_tx = NULL;

	(void)audio_type;
	if (data == NULL || length == 0) {
		return -1;
	}

	if (rtos_lock_mutex(&s_kvs_mtx) != kNoErr) {
		return -1;
	}
	audio_tx = s_audio_tx;
	rtos_unlock_mutex(&s_kvs_mtx);
	if (audio_tx == NULL) {
		return -1;
	}

	os_memset(&frame, 0, sizeof(frame));
	frame.version = FRAME_CURRENT_VERSION;
	frame.frameData = data;
	frame.size = length;
	frame.presentationTs = s_audio_pts;
	s_audio_pts += NTWK_KVS_AUDIO_FRAME_DURATION;

	st = writeFrame(audio_tx, &frame);
	if (st == STATUS_SRTP_NOT_READY_YET) {
		return 0;
	}
	return (st == STATUS_SUCCESS) ? (int)length : -1;
}

bk_err_t ntwk_kvs_ctrl_register_receive_cb(int (*cb)(uint8_t *data, uint32_t length))
{
	s_ctrl_rx = cb;
	return BK_OK;
}

bk_err_t ntwk_kvs_video_register_receive_cb(int (*cb)(uint8_t *data, uint32_t length))
{
	s_video_rx = cb;
	return BK_OK;
}

bk_err_t ntwk_kvs_audio_register_receive_cb(int (*cb)(uint8_t *data, uint32_t length))
{
	s_audio_rx = cb;
	return BK_OK;
}
