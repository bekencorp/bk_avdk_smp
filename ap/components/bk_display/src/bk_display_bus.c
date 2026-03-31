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

#include "components/bk_display_bus.h"

#define TAG "bk_disp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


avdk_err_t bk_display_bus_enable(bk_display_bus_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->enable, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->enable(handle);
}

avdk_err_t bk_display_bus_disable(bk_display_bus_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->disable, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->disable(handle);
}

avdk_err_t bk_display_bus_set_clock(bk_display_bus_handle_t handle, bk_panel_clock_config_t *clock)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->set_clock, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->set_clock(handle, clock);
}

avdk_err_t bk_display_bus_read(bk_display_bus_handle_t handle, bk_display_bus_rw_type_t type, uint32_t cmd, void *param, size_t size)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->read, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->read(handle, type, cmd, param, size);
}

avdk_err_t bk_display_bus_write(bk_display_bus_handle_t handle, bk_display_bus_rw_type_t type, uint32_t cmd, const void *param, size_t size)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->write, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->write(handle, type, cmd, param, size);
}

avdk_err_t bk_display_bus_flush(bk_display_bus_handle_t handle, uint8_t *frame, flush_free_cb_t cb)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->flush, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->flush(handle, frame, cb);
}

avdk_err_t bk_display_bus_delete(bk_display_bus_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->delete, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->delete(handle);
}