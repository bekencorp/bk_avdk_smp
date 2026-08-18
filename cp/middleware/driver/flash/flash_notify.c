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
#include <os/os.h>
#include <driver/flash.h>
#include "flash_driver.h"
// #include "mb_ipc_cmd.h"

static void (*s_flash_op_notify)(uint32_t param) = NULL;

bk_err_t mb_flash_register_op_notify(void * notify_cb)
{
    s_flash_op_notify = (void (*)(uint32_t))notify_cb;

	return BK_OK;
}

bk_err_t mb_flash_unregister_op_notify(void * notify_cb)
{
	if(s_flash_op_notify == notify_cb)
	{
		s_flash_op_notify = NULL;
		return BK_OK;
	}

	return BK_ERR_FLASH_WAIT_CB_NOT_REGISTER;
}

bk_err_t mb_flash_ipc_init(void)
{
	return BK_OK;
}

bk_err_t mb_flash_op_prepare(void)
{
	// disable the LCD dev interrupt.
	if(s_flash_op_notify != NULL)
		s_flash_op_notify(0);

	return BK_OK;
}

bk_err_t mb_flash_op_finish(void)
{
	// enable the LCD dev interrupt.
	if(s_flash_op_notify != NULL)
		s_flash_op_notify(1);
	
	return BK_OK;
}

static volatile flash_op_status_t s_flash_op_status = 0;

__attribute__((section(".itcm_sec_code"))) bk_err_t bk_flash_set_operate_status(flash_op_status_t status)
{
	s_flash_op_status = status;
	return BK_OK;
}

__attribute__((section(".itcm_sec_code"))) flash_op_status_t bk_flash_get_operate_status(void)
{
	return s_flash_op_status;
}
