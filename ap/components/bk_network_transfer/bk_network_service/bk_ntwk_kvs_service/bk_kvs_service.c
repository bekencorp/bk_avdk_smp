#include "bk_private/bk_init.h"
#include <os/os.h>
#include <string.h>

#include "common/network_transfer_common.h"
#include "network_transfer.h"
#include "network_type.h"
#include "network_transfer_internal.h"
#include "ntwk_kvs_service.h"

#define TAG "bk-kvs-svc"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

bk_err_t bk_kvs_trans_service_init(char *service_name)
{
	bk_err_t ret = BK_OK;
	ntwk_trans_ctxt_t *ctxt = NULL;

	LOGI("%s start\r\n", __func__);

	ret = ntwk_trans_ctxt_init(ctxt);
	if (ret != BK_OK) {
		LOGE("ntwk_trans_ctxt_init failed\n");
		goto error;
	}

	ret = ntwk_kvs_init();
	if (ret != BK_OK) {
		LOGE("ntwk_kvs_init failed\n");
		goto error;
	}

	ctxt = ntwk_trans_get_ctxt();
	if (ctxt == NULL) {
		LOGE("ctxt is NULL\n");
		goto error;
	}

	strncpy(ctxt->service_name, service_name, sizeof(ctxt->service_name) - 1);
	ctxt->service_name[sizeof(ctxt->service_name) - 1] = '\0';

	if (ctxt->cntrl_chan != NULL) {
		ctxt->cntrl_chan->type = NTWK_TRANS_CHAN_CTRL;
		ctxt->cntrl_chan->send = ntwk_kvs_ctrl_send_packet;
		ctxt->cntrl_chan->pack = NULL;
		ctxt->cntrl_chan->unpack = NULL;
		ctxt->cntrl_chan->fragment = NULL;
		ctxt->cntrl_chan->unfragment = NULL;
		ntwk_in_register_ctrl_start_cb(ntwk_kvs_ctrl_chan_start);
		ntwk_in_register_ctrl_stop_cb(ntwk_kvs_ctrl_chan_stop);
		ntwk_kvs_ctrl_register_receive_cb(ntwk_trans_ctrl_recv_handler);
	}

	if (ctxt->video_chan != NULL) {
		ctxt->video_chan->type = NTWK_TRANS_CHAN_VIDEO;
		ctxt->video_chan->vid_type = IMAGE_H264;
		ctxt->video_chan->send = ntwk_kvs_video_send_packet;
		ctxt->video_chan->pack = NULL;
		ctxt->video_chan->unpack = NULL;
		ctxt->video_chan->fragment = NULL;
		ctxt->video_chan->unfragment = NULL;
		ctxt->video_chan->drop_check = NULL;
		ntwk_in_register_video_start_cb(ntwk_kvs_video_chan_start);
		ntwk_in_register_video_stop_cb(ntwk_kvs_video_chan_stop);
		ntwk_kvs_video_register_receive_cb(ntwk_trans_video_recv_handler);
	}

	if (ctxt->audio_chan != NULL) {
		ctxt->audio_chan->type = NTWK_TRANS_CHAN_AUDIO;
		ctxt->audio_chan->aud_type = AUDIO_ENC_TYPE_G711A;
		ctxt->audio_chan->send = ntwk_kvs_audio_send_packet;
		ctxt->audio_chan->pack = NULL;
		ctxt->audio_chan->unpack = NULL;
		ctxt->audio_chan->fragment = NULL;
		ctxt->audio_chan->unfragment = NULL;
		ntwk_in_register_audio_start_cb(ntwk_kvs_audio_chan_start);
		ntwk_in_register_audio_stop_cb(ntwk_kvs_audio_chan_stop);
		ntwk_kvs_audio_register_receive_cb(ntwk_trans_audio_recv_handler);
	}

	LOGI("%s end\r\n", __func__);
	return BK_OK;

error:
	bk_kvs_trans_service_deinit();
	return BK_FAIL;
}

bk_err_t bk_kvs_trans_service_deinit(void)
{
	LOGI("%s start\r\n", __func__);
	ntwk_kvs_deinit();
	ntwk_trans_ctxt_deinit();
	LOGI("%s end\r\n", __func__);
	return BK_OK;
}
