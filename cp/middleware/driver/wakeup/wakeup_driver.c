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

#include <common/bk_include.h>
#include <os/mem.h>
#include <driver/wakeup.h>
#include <driver/int.h>

#include "wakeup_driver.h"
#include "sys_driver.h"
#include "aon_pmu_hal.h"

#define WAKEUP_SOURCE_COUNT_MAX WAKEUP_SOURCE_INT_NONE

#define WAKEUP_RETURN_ON_DRIVER_NOT_INIT()        \
	do                                            \
	{                                             \
		if (!s_wakeup.inited)                     \
		{                                         \
			return BK_ERR_WAKEUP_DRIVER_NOT_INIT; \
		}                                         \
	} while (0)

#define WAKEUP_RETURN_ON_INVALID_SOURCE_TYPE(src)     \
	do                                                \
	{                                                 \
		if ((src) >= WAKEUP_SOURCE_INT_NONE)          \
		{                                             \
			return BK_ERR_WAKEUP_INVALID_SOURCE_TYPE; \
		}                                             \
	} while (0)

typedef struct
{
	uint8_t en;
	wakeup_cb_conf_t wkup_conf;
	wakeup_cb_conf_t src_conf;
} wakeup_source_conf_t;

typedef struct
{
	bool inited;
	wakeup_source_conf_t confs[WAKEUP_SOURCE_COUNT_MAX];
} wakeup_driver_t;

static wakeup_driver_t s_wakeup;

static void __BK_IRQ wakeup_isr(void)
{
	wakeup_source_t wksrc = WAKEUP_SOURCE_INT_NONE;

	wksrc = aon_pmu_hal_get_wakeup_source();

	// clear interrupt
	sys_drv_int_group2_disable(ANA_GPIO_INTERRUPT_CTRL_BIT);
	for (int i = 0; i < WAKEUP_SOURCE_COUNT_MAX; i++)
	{
		if ((wksrc & BIT(i)) && (s_wakeup.confs[i].wkup_conf.cb != NULL))
		{
			WAKEUP_LOGV("wakeup int src:%d \r\n", i);
			s_wakeup.confs[i].wkup_conf.cb(s_wakeup.confs[i].wkup_conf.args);
		}
	}
}

bk_err_t bk_wakeup_driver_init(void)
{
	WAKEUP_LOGV("%s[+]\r\n", __func__);

	if (!s_wakeup.inited)
	{
		os_memset(&s_wakeup, 0, sizeof(s_wakeup));

		bk_int_isr_register(INT_SRC_ANA_GPIO, wakeup_isr, NULL);

		s_wakeup.inited = true;
	}

	WAKEUP_LOGV("%s[-]\r\n", __func__);

	return BK_OK;
}

bk_err_t bk_wakeup_driver_deinit(void)
{
	WAKEUP_LOGV("%s[+]\r\n", __func__);

	if (s_wakeup.inited)
	{
		//bk_int_isr_unregister(INT_SRC_ANA3V_WAKEUP);
		bk_int_isr_unregister(INT_SRC_ANA_GPIO);

		s_wakeup.inited = false;
	}

	WAKEUP_LOGV("%s[-]\r\n", __func__);

	return BK_OK;
}

bk_err_t bk_wakeup_register_cb(wakeup_source_t type, wakeup_cb_conf_t wkup_cfg, wakeup_cb_conf_t src_cfg)
{
	WAKEUP_RETURN_ON_DRIVER_NOT_INIT();
	WAKEUP_RETURN_ON_INVALID_SOURCE_TYPE(type);

	if (wkup_cfg.cb == NULL && src_cfg.cb == NULL)
	{
		WAKEUP_LOGW("The callback on registing is null, please double check it.\r\n");
	}

	if (wkup_cfg.cb) {
		WAKEUP_LOGD("The wkup callback of source %d registed\r\n", type);
		s_wakeup.confs[type].wkup_conf.cb = wkup_cfg.cb;
		s_wakeup.confs[type].wkup_conf.args = wkup_cfg.args;
	}

	if (src_cfg.cb) {
		WAKEUP_LOGD("The source callback of source %d registed\r\n", type);
		s_wakeup.confs[type].src_conf.cb = src_cfg.cb;
		s_wakeup.confs[type].src_conf.args = src_cfg.args;
	}

	return BK_OK;
}

bk_err_t bk_wakeup_unregister_cb(wakeup_source_t type)
{
	WAKEUP_RETURN_ON_DRIVER_NOT_INIT();
	WAKEUP_RETURN_ON_INVALID_SOURCE_TYPE(type);

	s_wakeup.confs[type].wkup_conf.cb = NULL;
	s_wakeup.confs[type].wkup_conf.args = NULL;
	s_wakeup.confs[type].src_conf.cb = NULL;
	s_wakeup.confs[type].src_conf.args = NULL;

	return BK_OK;
}

bk_err_t bk_wakeup_driver_source_set(wakeup_source_t type)
{
	WAKEUP_RETURN_ON_DRIVER_NOT_INIT();
	WAKEUP_RETURN_ON_INVALID_SOURCE_TYPE(type);

	WAKEUP_LOGV("wakeup source %d enabled\r\n", type);
	s_wakeup.confs[type].en = 1;

	return BK_OK;
}

bk_err_t bk_wakeup_driver_source_clr(wakeup_source_t type)
{
	WAKEUP_RETURN_ON_DRIVER_NOT_INIT();
	WAKEUP_RETURN_ON_INVALID_SOURCE_TYPE(type);

	s_wakeup.confs[type].en = 0;

	return BK_OK;
}

__IRAM_SEC bk_err_t wakeup_source_config(void)
{
	WAKEUP_RETURN_ON_DRIVER_NOT_INIT();
	bk_err_t ret = 0;

	// reset wakeup flag before sleep
	sys_drv_wakeup_source_clear();

	for (int i = 0; i < WAKEUP_SOURCE_COUNT_MAX; i++)
	{
		if ((s_wakeup.confs[i].en && s_wakeup.confs[i].src_conf.cb != NULL))
		{
			ret |= s_wakeup.confs[i].src_conf.cb(s_wakeup.confs[i].src_conf.args);
		}
	}

	return ret;
}

__IRAM_SEC bk_err_t wakeup_source_clear(void)
{
	for (int i = 0; i < WAKEUP_SOURCE_COUNT_MAX; i++)
	{
		s_wakeup.confs[i].en = 0;
	}

	return BK_OK;
}