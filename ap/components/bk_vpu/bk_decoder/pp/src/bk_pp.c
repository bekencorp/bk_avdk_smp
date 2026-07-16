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
#include "components/bk_decode/bk_pp_ctlr.h"

#define TAG "bk_pp"

avdk_err_t bk_pp_init(bk_pp_ctlr_handle_t handle)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->init, AVDK_ERR_UNSUPPORTED, TAG, "init not supported");
	return handle->init(handle);
}

avdk_err_t bk_pp_deinit(bk_pp_ctlr_handle_t handle)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->deinit, AVDK_ERR_UNSUPPORTED, TAG, "deinit not supported");
	return handle->deinit(handle);
}

avdk_err_t bk_pp_open(bk_pp_ctlr_handle_t handle)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->open, AVDK_ERR_UNSUPPORTED, TAG, "open not supported");
	return handle->open(handle);
}

avdk_err_t bk_pp_close(bk_pp_ctlr_handle_t handle)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->close, AVDK_ERR_UNSUPPORTED, TAG, "close not supported");
	return handle->close(handle);
}

avdk_err_t bk_pp_process(bk_pp_ctlr_handle_t handle, const bk_pp_process_req_t *req)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->process, AVDK_ERR_UNSUPPORTED, TAG, "process not supported");
	return handle->process(handle, req);
}

avdk_err_t bk_pp_delete(bk_pp_ctlr_handle_t handle)
{
	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(handle->del, AVDK_ERR_UNSUPPORTED, TAG, "del not supported");
	return handle->del(handle);
}
