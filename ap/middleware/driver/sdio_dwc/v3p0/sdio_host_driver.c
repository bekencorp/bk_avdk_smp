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
#include <driver/int.h>
#include <driver/gpio.h>
#include <driver/sdio_host.h>
#include "clock_driver.h"
#include "gpio_driver.h"
#include "gpio_map.h"
#include "pmu.h"
#include "power_driver.h"
#include "sdio_host_driver.h"
//#include "sdio_host_hal.h"
#include "sys_driver.h"
#include "icu_driver.h"
#include "drv_model_pub.h"
#if (!CONFIG_SYSTEM_CTRL)
#include "bk_sys_ctrl.h"
#endif
#include "bk_misc.h"
#if CONFIG_SDIO_PM_CB_SUPPORT
#include <modules/pm.h>
#endif

#if ((CONFIG_SDIO_V3P0) && (CONFIG_SDIO_GDMA_EN))
#include <driver/dma.h>
#endif

#if (CONFIG_TASK_WDT)
#include "bk_wdt.h"
#endif


bk_err_t bk_sdio_host_driver_init(void)
{
	return BK_OK;
}

