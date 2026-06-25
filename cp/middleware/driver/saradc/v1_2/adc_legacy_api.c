// Copyright 2020-2025 Beken
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

#define TAG "sadc"

#include <common/bk_include.h>
#include <common/bk_compiler.h>
#include <os/os.h>
#include "adc_driver.h"
#include "common/bk_err.h"

#if CONFIG_SARADC_V1P2_COMPATIBLE_MODE
static adc_chan_t s_current_channel_id = 0;
static beken_mutex_t s_adc_session_mutex = NULL;
static bool s_adc_session_mutex_inited = false;

bk_err_t adc_session_mutex_init(void)
{
	if (s_adc_session_mutex_inited) {
		return BK_OK;
	}

	if (rtos_init_recursive_mutex(&s_adc_session_mutex) != kNoErr) {
		return BK_FAIL;
	}

	s_adc_session_mutex_inited = true;
	return BK_OK;
}

bk_err_t adc_session_mutex_deinit(void)
{
	if (!s_adc_session_mutex_inited) {
		return BK_OK;
	}

	if (rtos_deinit_recursive_mutex(&s_adc_session_mutex) != kNoErr) {
		return BK_FAIL;
	}

	s_adc_session_mutex = NULL;
	s_adc_session_mutex_inited = false;
	return BK_OK;
}

bk_err_t adc_session_lock(void)
{
	if (!s_adc_session_mutex_inited) {
		return BK_FAIL;
	}

	return (rtos_lock_recursive_mutex(&s_adc_session_mutex) == kNoErr) ? BK_OK : BK_FAIL;
}

bk_err_t adc_session_unlock(void)
{
	if (!s_adc_session_mutex_inited) {
		return BK_FAIL;
	}

	return (rtos_unlock_recursive_mutex(&s_adc_session_mutex) == kNoErr) ? BK_OK : BK_FAIL;
}
#endif

extern bk_err_t mb_saradc_op_prepare(void);
extern bk_err_t mb_saradc_op_finish(void);

/* These functions will be deprecated; obsolete interfaces*/
bk_err_t bk_adc_init(adc_chan_t adc_chan)
{
    return BK_OK;
}

bk_err_t bk_adc_deinit(adc_chan_t chan)
{
    #if CONFIG_SARADC_V1P2_COMPATIBLE_MODE
    return bk_adc_channel_deinit(chan);
    #else
    return BK_OK;
    #endif
}

bk_err_t bk_adc_acquire(void)
{
#if CONFIG_SARADC_V1P2_COMPATIBLE_MODE
	bk_err_t ret;

	ret = adc_session_lock();
	if (ret != BK_OK) {
		return ret;
	}
#endif

	mb_saradc_op_prepare();

	return BK_OK;
}

bk_err_t bk_adc_release(void)
{
	mb_saradc_op_finish();

#if CONFIG_SARADC_V1P2_COMPATIBLE_MODE
	return adc_session_unlock();
#else
	return BK_OK;
#endif
}

bk_err_t bk_adc_stop(void)
{
    return BK_OK;
}

bk_err_t bk_adc_start(void)
{
    return BK_OK;
}

bk_err_t bk_adc_enable_bypass_clalibration(void)
{
    return BK_OK;
}

bk_err_t bk_adc_disable_bypass_clalibration(void)
{
    return BK_OK;
}

bk_err_t bk_adc_set_config(adc_config_t *config)
{
    #if CONFIG_SARADC_V1P2_COMPATIBLE_MODE
    s_current_channel_id = config->chan;
    return bk_adc_channel_init(config);
    #else
    return BK_OK;
    #endif
}

bk_err_t bk_adc_set_channel(adc_chan_t adc_chan)
{
    #if CONFIG_SARADC_V1P2_COMPATIBLE_MODE
    s_current_channel_id = adc_chan;
    return BK_OK;
    #else
    return BK_OK;
    #endif
}

bk_err_t bk_adc_read_raw(uint16_t* buf, uint32_t size, uint32_t timeout)
{
    #if CONFIG_SARADC_V1P2_COMPATIBLE_MODE
    return bk_adc_channel_raw_read(s_current_channel_id, buf, size, timeout);
    #else
    return BK_OK;
    #endif
}

bk_err_t bk_adc_read(uint16_t* data, uint32_t timeout)
{
    #if CONFIG_SARADC_V1P2_COMPATIBLE_MODE
    return bk_adc_channel_read(s_current_channel_id, data, timeout);
    #else
    return BK_OK;
    #endif
}
// eof

