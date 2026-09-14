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

#define FLASH_MAX_OP_NOTIFY_CNT (4)

typedef struct {
	flash_op_notify_callback_t cb;
	void *args;
} flash_op_notify_slot_t;

/* Array-based op-notify registry: the single registration path. */
static flash_op_notify_slot_t s_flash_op_notify_slots[FLASH_MAX_OP_NOTIFY_CNT] = {0};

bk_err_t mb_flash_register_op_notify_cb(flash_op_notify_callback_t notify_cb, void *args)
{
	uint32_t i;

	if (notify_cb == NULL)
	{
		return BK_ERR_PARAM;
	}

	/* Same callback already registered: just refresh its args. */
	for (i = 0; i < FLASH_MAX_OP_NOTIFY_CNT; i++)
	{
		if (s_flash_op_notify_slots[i].cb == notify_cb)
		{
			s_flash_op_notify_slots[i].args = args;
			return BK_OK;
		}
	}

	for (i = 0; i < FLASH_MAX_OP_NOTIFY_CNT; i++)
	{
		if (s_flash_op_notify_slots[i].cb == NULL)
		{
			s_flash_op_notify_slots[i].cb = notify_cb;
			s_flash_op_notify_slots[i].args = args;
			return BK_OK;
		}
	}

	return BK_ERR_FLASH_WAIT_CB_FULL;
}

bk_err_t mb_flash_unregister_op_notify_cb(flash_op_notify_callback_t notify_cb)
{
	uint32_t i;

	for (i = 0; i < FLASH_MAX_OP_NOTIFY_CNT; i++)
	{
		if (s_flash_op_notify_slots[i].cb == notify_cb)
		{
			s_flash_op_notify_slots[i].cb = NULL;
			s_flash_op_notify_slots[i].args = NULL;
			return BK_OK;
		}
	}

	return BK_ERR_FLASH_WAIT_CB_NOT_REGISTER;
}

static void flash_op_notify_dispatch(uint32_t busy)
{
	uint32_t i;

	for (i = 0; i < FLASH_MAX_OP_NOTIFY_CNT; i++)
	{
		if (s_flash_op_notify_slots[i].cb != NULL)
		{
			s_flash_op_notify_slots[i].cb(busy, s_flash_op_notify_slots[i].args);
		}
	}
}

bk_err_t mb_flash_ipc_init(void)
{
	return BK_OK;
}

bk_err_t mb_flash_op_prepare(void)
{
	/* notify every registered peripheral: flash is about to erase/write. */
	flash_op_notify_dispatch(1);

	return BK_OK;
}

bk_err_t mb_flash_op_finish(void)
{
	/* notify every registered peripheral: flash erase/write finished. */
	flash_op_notify_dispatch(0);

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
