// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include "isp_core.h"
#include <driver/isp.h>
#include <driver/isp_base.h>
#include <components/bk_encode/bk_h264_encode_ctlr.h>

#include "bk_flexa_bond_types.h"

#define TAG "bk_flexa_isp_h264e_bond"

#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define BK_FLEXA_ISP_BOND_STOP_WAIT_MS 2000U

static void isp_h264e_bond_wait_sbi_disabled(bk_flexa_bond_config_t *bond_p)
{
	bk_flexa_bond_t *in_stream;

	if (bond_p == NULL) {
		return;
	}

	bond_p->flexa_sbi = 0;
	bond_p->set_sbi_flag = 1;
	if (rtos_get_semaphore(&bond_p->sem, BK_FLEXA_ISP_BOND_STOP_WAIT_MS) == BK_OK) {
		return;
	}

	in_stream = (bk_flexa_bond_t *)bond_p->in_stream;
	if (in_stream != NULL && in_stream->handle != NULL) {
		isp_handle_t isp_h = (isp_handle_t)in_stream->handle;

		LOGW("%s SBI disable wait timeout, force disable\r\n", __func__);
		bk_isp_flexa_sbi_config(&isp_h, ISP_MP_CHN_ID, 0);
		bond_p->set_sbi_flag = 0;
	}
}

static void isp_h264e_handle_frame_end_cb(uint32_t seq, uint32_t line, uint8_t chnl, uint8_t ok, void *arg)
{
	bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)arg;
	(void)seq;
	(void)line;
	(void)chnl;

	if (in_stream == NULL || in_stream->bond_config == NULL) {
		return;
	}
	bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)in_stream->bond_config->out_stream;
	if (out_stream == NULL) {
		return;
	}
	bk_h264_encode_ctlr_handle_t enc = (bk_h264_encode_ctlr_handle_t)out_stream->handle;
	if (enc == NULL) {
		return;
	}
	if (in_stream->bond_config->set_sbi_flag == 1) {
		isp_handle_t isp_h = (isp_handle_t)in_stream->handle;
		if (in_stream->bond_config->flexa_sbi == 1) {
			in_stream->bond_config->set_sbi_flag = 0;
			bk_isp_flexa_sbi_config(&isp_h, ISP_MP_CHN_ID, 1);
		} else {
			bk_isp_deregister_isr_callback(&isp_h, ISP_FRAME_END_DONE, in_stream);
			return;
		}
	}
	if (ok == 0) {
		bk_h264_encode_force_idr(enc);
	}
	(void)bk_h264_encode_ioctl(enc, BK_H264_ENCODE_IOCTL_SET_FRAME_READY, (void *)0);
}

static void isp_h264e_enc_frame_done(uint32_t status, void *args)
{
	bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)args;
	if (out_stream == NULL || out_stream->handle == NULL) {
		return;
	}
	bk_flexa_bond_t *in_stream = out_stream->bond_config->in_stream;
	if (in_stream == NULL || in_stream->handle == NULL) {
		return;
	}
	isp_handle_t isp_h = (isp_handle_t)in_stream->handle;
	if (isp_h == NULL) {
		return;
	}
	if (out_stream->bond_config->set_sbi_flag == 1
		&& out_stream->bond_config->flexa_sbi == 0) {
		bk_isp_flexa_sbi_config(&isp_h, ISP_MP_CHN_ID, 0);
		out_stream->bond_config->set_sbi_flag = 0;
		(void)bk_h264_encode_ioctl((bk_h264_encode_ctlr_handle_t)out_stream->handle,
					   BK_H264_ENCODE_IOCTL_UNREGISTER_BOND, out_stream);
		rtos_set_semaphore(&out_stream->bond_config->sem);
		return;
	}
	if (status == BK_FAIL) {
		bk_isp_flexa_sbi_config(&isp_h, ISP_MP_CHN_ID, 0);
		out_stream->bond_config->set_sbi_flag = 1;
	}
}

static void isp_h264e_bond_isp_stream_error(uint32_t reason, void *args)
{
	(void)reason;
	bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)args;
	(void)in_stream;
}

avdk_err_t bk_flexa_isp_h264e_bond_start(void **bond, void *isp, bk_h264_encode_ctlr_handle_t h264)
{
	avdk_err_t ret = AVDK_ERR_OK;
	bk_err_t br;
	bk_flexa_bond_config_t *bond_new = NULL;
	bk_flexa_bond_t *in_stream = NULL;
	bk_flexa_bond_t *out_stream = NULL;
	isp_handle_t isp_h = NULL;
	uint8_t h264_registered = 0;

	if (bond == NULL || isp == NULL || h264 == NULL) {
		LOGE("%s invalid args bond %p isp %p h264 %p\r\n", __func__, bond, isp, h264);
		return AVDK_ERR_INVAL;
	}
	if (*bond != NULL) {
		LOGE("%s already started\r\n", __func__);
		return AVDK_ERR_INVAL;
	}

	bond_new = (bk_flexa_bond_config_t *)os_malloc(sizeof(bk_flexa_bond_config_t));
	if (bond_new == NULL) {
		LOGE("%s malloc bond_config failed\r\n", __func__);
		return AVDK_ERR_NOMEM;
	}
	os_memset(bond_new, 0, sizeof(bk_flexa_bond_config_t));

	in_stream = (bk_flexa_bond_t *)os_malloc(sizeof(bk_flexa_bond_t));
	if (in_stream == NULL) {
		LOGE("%s malloc in_stream failed\r\n", __func__);
		goto error;
	}
	os_memset(in_stream, 0, sizeof(bk_flexa_bond_t));

	out_stream = (bk_flexa_bond_t *)os_malloc(sizeof(bk_flexa_bond_t));
	if (out_stream == NULL) {
		LOGE("%s malloc out_stream failed\r\n", __func__);
		goto error;
	}
	os_memset(out_stream, 0, sizeof(bk_flexa_bond_t));

	bond_new->in_stream = in_stream;
	bond_new->out_stream = out_stream;
	bond_new->in_stream_type = BK_FLEXA_TYPE_ISP;
	bond_new->out_stream_type = BK_FLEXA_TYPE_H264E;
	bond_new->set_sbi_flag = 1;
	bond_new->flexa_sbi = 1;

	ret = rtos_init_semaphore(&bond_new->sem, 1);
	if (ret != BK_OK) {
		LOGE("%s init semaphore failed %d\r\n", __func__, ret);
		goto error;
	}

	in_stream->handle = isp;
	in_stream->error = isp_h264e_bond_isp_stream_error;
	in_stream->bond_config = bond_new;

	isp_h = (isp_handle_t)isp;
	{
		isp_control_t *isp_control = (isp_control_t *)isp_h;

		out_stream->max_lines_per_frame =
			(isp_control->chn[ISP_MP_CHN_ID].chn_attr.chnFormat.height + 15) / 16;
	}

	out_stream->handle = (void *)h264;
	out_stream->frame_done = isp_h264e_enc_frame_done;
	out_stream->bond_config = bond_new;

	ret = bk_h264_encode_ioctl(h264, BK_H264_ENCODE_IOCTL_REGISTER_BOND, out_stream);
	if (ret != AVDK_ERR_OK) {
		LOGE("%s H264 REGISTER_BOND failed %d\r\n", __func__, ret);
		goto error;
	}
	h264_registered = 1;

	br = bk_isp_register_isr_callback(&isp_h, ISP_FRAME_END_DONE, isp_h264e_handle_frame_end_cb, in_stream);
	if (br != BK_OK) {
		LOGE("%s ISP_FRAME_END_DONE register failed %d\r\n", __func__, br);
		ret = AVDK_ERR_GENERIC;
		goto error;
	}

	*bond = bond_new;
	LOGI("%s bond started\r\n", __func__);
	return ret;

error:
	if (isp_h != NULL && in_stream != NULL) {
		(void)bk_isp_deregister_isr_callback(&isp_h, ISP_FRAME_END_DONE, in_stream);
	}
	if (h264_registered && out_stream != NULL) {
		(void)bk_h264_encode_ioctl(h264, BK_H264_ENCODE_IOCTL_UNREGISTER_BOND, out_stream);
	}
	if (in_stream != NULL) {
		os_free(in_stream);
	}
	if (out_stream != NULL) {
		os_free(out_stream);
	}
	if (bond_new != NULL) {
		if (bond_new->sem != NULL) {
			rtos_deinit_semaphore(&bond_new->sem);
		}
		os_free(bond_new);
	}
	LOGE("%s bond failed\r\n", __func__);
	return ret;
}

void bk_flexa_isp_h264e_bond_stop(void *bond)
{
	bk_flexa_bond_config_t *bond_p = (bk_flexa_bond_config_t *)bond;
	if (bond_p == NULL) {
		return;
	}
	isp_h264e_bond_wait_sbi_disabled(bond_p);

	if (bond_p->in_stream != NULL) {
		os_free(bond_p->in_stream);
		bond_p->in_stream = NULL;
	}
	if (bond_p->out_stream != NULL) {
		os_free(bond_p->out_stream);
		bond_p->out_stream = NULL;
	}
	if (bond_p->sem != NULL) {
		rtos_deinit_semaphore(&bond_p->sem);
		bond_p->sem = NULL;
	}
	os_free(bond_p);
	bond_p = NULL;
	LOGI("%s bond stopped\r\n", __func__);
}
