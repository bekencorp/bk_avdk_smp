#include <os/os.h>
#include <os/mem.h>
#include <components/bk_decode/bk_h264_decode_ctlr.h>
#include <components/bk_gpu.h>

#include "private_h264_decode_ctlr.h"
#include "bk_flexa_bond_types.h"

#define TAG "bk_flexa_h264d_bond"

#define LOGD(...) BK_LOGD(TAG, __VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, __VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, __VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, __VA_ARGS__)

static void gpu_bond_h264d_flexa_done(uint32_t wr_ptr, void *args)
{
	bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)args;
	if (in_stream == NULL || in_stream->bond_config == NULL) {
		return;
	}

	bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)in_stream->bond_config->out_stream;
	if (out_stream == NULL) {
		return;
	}

	in_stream->last_lines = wr_ptr;
	bk_gpu_ctlr_handle_t gpu = (bk_gpu_ctlr_handle_t)out_stream->handle;
	if (gpu != NULL) {
		(void)bk_gpu_ioctl(gpu, BK_GPU_IOCTL_SET_FLEXA_LINES_READY, (void *)wr_ptr);
	}
}

static void gpu_bond_h264d_decode_error(uint32_t reason, void *args)
{
	(void)reason;
	(void)args;
}

static void h264d_gpu_flexa_done(uint32_t wr_ptr, void *args)
{
	bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)args;
	if (out_stream == NULL || out_stream->bond_config == NULL) {
		return;
	}

	bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)out_stream->bond_config->in_stream;
	if (in_stream == NULL) {
		return;
	}

	out_stream->last_lines = wr_ptr;
	bk_h264_decode_ctlr_handle_t src = (bk_h264_decode_ctlr_handle_t)in_stream->handle;
	if (src != NULL) {
		bk_h264_decode_port_rd_t rd_cmd = {
			.port_ptr = in_stream,
			.rd_blocks = wr_ptr,
		};
		(void)bk_h264_decode_ioctl(src, BK_H264_DECODE_IOCTL_PORT_SET_RD_PTR, &rd_cmd);
	}
}

static void h264d_gpu_frame_done(uint32_t status, void *args)
{
	bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)args;
	if (out_stream == NULL || out_stream->bond_config == NULL) {
		return;
	}

	bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)out_stream->bond_config->in_stream;
	if (in_stream == NULL) {
		return;
	}

	bk_h264_decode_ctlr_handle_t src = (bk_h264_decode_ctlr_handle_t)in_stream->handle;
	if (src != NULL) {
		if (status != BK_OK) {
			bk_h264_decode_port_rd_t rd_cmd = {
				.port_ptr = in_stream,
				.rd_blocks = out_stream->max_lines_per_frame,
			};
			(void)bk_h264_decode_ioctl(src, BK_H264_DECODE_IOCTL_PORT_SET_RD_PTR, &rd_cmd);
		}

		(void)bk_h264_decode_ioctl(src, BK_H264_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE, in_stream);
	}
}

avdk_err_t bk_flexa_h264d_gpu_bond_start(void **bond,
					 bk_h264_decode_ctlr_handle_t h264,
					 bk_gpu_ctlr_handle_t gpu)
{
	avdk_err_t ret = AVDK_ERR_OK;
	bk_flexa_bond_config_t *bond_new = NULL;
	bk_flexa_bond_t *in_stream = NULL;
	bk_flexa_bond_t *out_stream = NULL;
	uint8_t h264_registered = 0U;
	uint8_t gpu_registered = 0U;

	if (bond == NULL) {
		LOGE("%s bond out is NULL\r\n", __func__);
		return AVDK_ERR_INVAL;
	}
	if (*bond != NULL) {
		LOGE("%s already started\r\n", __func__);
		return AVDK_ERR_INVAL;
	}
	if (h264 == NULL || gpu == NULL) {
		LOGE("%s h264 %p, gpu %p invalid parameters\r\n", __func__, h264, gpu);
		return AVDK_ERR_INVAL;
	}

	bond_new = (bk_flexa_bond_config_t *)os_malloc(sizeof(bk_flexa_bond_config_t));
	if (bond_new == NULL) {
		LOGE("%s %d failed to allocate memory\r\n", __func__, __LINE__);
		return AVDK_ERR_NOMEM;
	}
	os_memset(bond_new, 0, sizeof(bk_flexa_bond_config_t));

	in_stream = (bk_flexa_bond_t *)os_malloc(sizeof(bk_flexa_bond_t));
	if (in_stream == NULL) {
		LOGE("%s %d failed to allocate memory\r\n", __func__, __LINE__);
		ret = AVDK_ERR_NOMEM;
		goto error;
	}
	os_memset(in_stream, 0, sizeof(bk_flexa_bond_t));

	out_stream = (bk_flexa_bond_t *)os_malloc(sizeof(bk_flexa_bond_t));
	if (out_stream == NULL) {
		LOGE("%s %d failed to allocate memory\r\n", __func__, __LINE__);
		ret = AVDK_ERR_NOMEM;
		goto error;
	}
	os_memset(out_stream, 0, sizeof(bk_flexa_bond_t));

	bond_new->in_stream = in_stream;
	bond_new->out_stream = out_stream;
	bond_new->in_stream_type = BK_FLEXA_TYPE_H264D;
	bond_new->out_stream_type = BK_FLEXA_TYPE_GPU;

	in_stream->handle = (void *)h264;
	in_stream->flexa_done = gpu_bond_h264d_flexa_done;
	in_stream->error = gpu_bond_h264d_decode_error;
	in_stream->bond_config = bond_new;

	{
		private_h264_decode_flexa_ctlr_t *ctrl = (private_h264_decode_flexa_ctlr_t *)h264;
		uint32_t seg_rows = 16U * ((ctrl->config.segment_height != 0U) ? (uint32_t)ctrl->config.segment_height : 1U);

		out_stream->max_lines_per_frame =
			((uint32_t)ctrl->config.out_height + seg_rows - 1U) / seg_rows;
	}

	out_stream->handle = (void *)gpu;
	out_stream->flexa_done = h264d_gpu_flexa_done;
	out_stream->frame_done = h264d_gpu_frame_done;
	out_stream->bond_config = bond_new;

	ret = bk_h264_decode_ioctl((bk_h264_decode_ctlr_handle_t)in_stream->handle,
				   BK_H264_DECODE_IOCTL_REGISTER_BOND, in_stream);
	if (ret != AVDK_ERR_OK) {
		LOGE("%s %d REGISTER_BOND H264D failed: %d\r\n", __func__, __LINE__, ret);
		goto error;
	}
	h264_registered = 1U;

	ret = bk_gpu_ioctl((bk_gpu_ctlr_handle_t)out_stream->handle, BK_GPU_IOCTL_REGISTER_BOND, out_stream);
	if (ret != AVDK_ERR_OK) {
		LOGE("%s %d REGISTER_BOND GPU failed: %d\r\n", __func__, __LINE__, ret);
		goto error;
	}
	gpu_registered = 1U;

	*bond = bond_new;
	LOGI("%s %d bond started\r\n", __func__, __LINE__);
	return AVDK_ERR_OK;

error:
	if (gpu_registered && out_stream != NULL && out_stream->handle != NULL) {
		(void)bk_gpu_ioctl((bk_gpu_ctlr_handle_t)out_stream->handle,
				   BK_GPU_IOCTL_UNREGISTER_BOND, out_stream);
	}
	if (h264_registered && in_stream != NULL && in_stream->handle != NULL) {
		(void)bk_h264_decode_ioctl((bk_h264_decode_ctlr_handle_t)in_stream->handle,
					   BK_H264_DECODE_IOCTL_UNREGISTER_BOND, in_stream);
	}
	if (bond_new != NULL) {
		os_free(bond_new);
	}
	if (in_stream != NULL) {
		os_free(in_stream);
	}
	if (out_stream != NULL) {
		os_free(out_stream);
	}
	LOGE("%s %d bond failed\r\n", __func__, __LINE__);
	return ret;
}

void bk_flexa_h264d_gpu_bond_stop(void *bond)
{
	bk_flexa_bond_config_t *bond_p = (bk_flexa_bond_config_t *)bond;
	if (bond_p == NULL) {
		return;
	}

	bk_flexa_bond_t *out_stream = bond_p->out_stream;
	if (out_stream != NULL && out_stream->handle != NULL) {
		(void)bk_gpu_ioctl((bk_gpu_ctlr_handle_t)out_stream->handle,
				   BK_GPU_IOCTL_UNREGISTER_BOND, out_stream);
	}

	bk_flexa_bond_t *in_stream = bond_p->in_stream;
	if (in_stream != NULL && in_stream->handle != NULL) {
		(void)bk_h264_decode_ioctl((bk_h264_decode_ctlr_handle_t)in_stream->handle,
					   BK_H264_DECODE_IOCTL_UNREGISTER_BOND, in_stream);
	}

	if (in_stream != NULL) {
		os_free(in_stream);
	}
	if (out_stream != NULL) {
		os_free(out_stream);
	}
	os_free(bond_p);
	LOGI("%s %d bond stopped\r\n", __func__, __LINE__);
}
