// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <components/log.h>
#include <components/avdk_utils/avdk_check.h>
#include "components/bk_encode/bk_jpeg_encode_ctlr.h"

#define TAG "bk_jpeg_enc"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

avdk_err_t bk_jpeg_encode_init(bk_jpeg_encode_ctlr_handle_t handle)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->init, AVDK_ERR_UNSUPPORTED, TAG, "init not supported");
	return handle->init(handle);
}

avdk_err_t bk_jpeg_encode_deinit(bk_jpeg_encode_ctlr_handle_t handle)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->deinit, AVDK_ERR_UNSUPPORTED, TAG, "deinit not supported");
	return handle->deinit(handle);
}

avdk_err_t bk_jpeg_encode_open(bk_jpeg_encode_ctlr_handle_t handle)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->open, AVDK_ERR_UNSUPPORTED, TAG, "open not supported");
	return handle->open(handle);
}

avdk_err_t bk_jpeg_encode_close(bk_jpeg_encode_ctlr_handle_t handle)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->close, AVDK_ERR_UNSUPPORTED, TAG, "close not supported");
	return handle->close(handle);
}

avdk_err_t bk_jpeg_encode_frame(bk_jpeg_encode_ctlr_handle_t handle, bk_jpeg_encode_input_t *input)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->encode_frame, AVDK_ERR_UNSUPPORTED, TAG, "encode_frame not supported");
	return handle->encode_frame(handle, input);
}

avdk_err_t bk_jpeg_encode_ioctl(bk_jpeg_encode_ctlr_handle_t handle, bk_jpeg_encode_ioctl_cmd_t cmd, void *arg)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, "ioctl not supported");
	return handle->ioctl(handle, cmd, arg);
}

avdk_err_t bk_jpeg_encode_delete(bk_jpeg_encode_ctlr_handle_t handle)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->del, AVDK_ERR_UNSUPPORTED, TAG, "del not supported");
	return handle->del(handle);
}

avdk_err_t bk_jpeg_encode_hw_flexa_new(bk_jpeg_encode_ctlr_handle_t *handle,
				       bk_jpeg_encode_hw_flexa_config_t *config)
{
	AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	extern avdk_err_t bk_jpeg_encode_hw_flexa_ctlr_new(bk_jpeg_encode_ctlr_handle_t *handle,
							   bk_jpeg_encode_hw_flexa_config_t *config);
	return bk_jpeg_encode_hw_flexa_ctlr_new(handle, config);
}

avdk_err_t bk_jpeg_encode_sw_flexa_new(bk_jpeg_encode_ctlr_handle_t *handle,
				       bk_jpeg_encode_sw_flexa_config_t *config)
{
	AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	extern avdk_err_t bk_jpeg_encode_sw_flexa_ctlr_new(bk_jpeg_encode_ctlr_handle_t *handle,
							   bk_jpeg_encode_sw_flexa_config_t *config);
	return bk_jpeg_encode_sw_flexa_ctlr_new(handle, config);
}
