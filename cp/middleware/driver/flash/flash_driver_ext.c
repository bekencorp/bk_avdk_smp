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
#include <components/ate.h>
#include <os/mem.h>
#include <driver/flash.h>
#include <os/os.h>
#include "bk_pm_model.h"
#include "flash_driver.h"
#include "flash_hal.h"
#include "sys_driver.h"
#include "driver/flash_partition.h"
#include <modules/chip_support.h>


/*
 * when bt INT effect the OTA update
 */
#define S_WAKE_UP    (0)
#define S_SLEEP      (1)
#define S_POWER_OFF  (2)
#define S_NO_BT      (3)

#define ERASE_TOUCH_TIMEOUT  (3000)//ms
#define ERASE_FLASH_TIMEOUT  (56)//ms
#define WRITE_FLASH_TIMEOUT  (4)//ms

static u32  bt_sleepend_time = -1;  //when cb return,bt is sleep ,recoeding the bt will sleep how long
static u32  bt_cb_anchor_time = 0;    //when cb return ,record current time ;
static u8   bt_sleep_state = S_NO_BT;  //record bt state;the default is 3(S_NO_BT);


void ble_sleep_cb(uint8_t is_sleeping, uint32_t slp_period)
{
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

    bt_sleep_state = is_sleeping ;
    bt_cb_anchor_time = rtos_get_time();
    if (is_sleeping == S_SLEEP)
    {
       bt_sleepend_time = bt_cb_anchor_time + slp_period/32;
    }

    GLOBAL_INT_RESTORE();
}

int ble_callback_deal_handler(uint32_t deal_flash_time)
{
    uint32_t  cur_time =0;
    uint32_t  temp_time = 0;
    int       ret_val = 0;

    cur_time = rtos_get_time();

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

    do
    {
        if(bt_sleep_state == S_POWER_OFF)     //poweroff
        {
            ret_val = 1;
            break;
        }
        else if(bt_sleep_state == S_WAKE_UP) //wakeup
        {
            if(cur_time >= bt_cb_anchor_time)
            {
                temp_time = (cur_time - bt_cb_anchor_time);
            }
            else
            {
                temp_time = 0xFFFFFFFF - bt_cb_anchor_time + cur_time;
            }

            if(temp_time >= ERASE_TOUCH_TIMEOUT)
            {
                bt_sleep_state = S_NO_BT;
                ret_val = 1;
                break;
            }

            ret_val = 0;
            break;
        }
        else if(bt_sleep_state == S_SLEEP) //sleep
        {
            if(bt_sleepend_time > bt_cb_anchor_time)
            {
                if(bt_sleepend_time < cur_time)
                {
                     ret_val = 1;
                     break;
                }
                else if(cur_time < bt_cb_anchor_time)
                {
                     ret_val = 1;
                     break;
                }
                else if((bt_sleepend_time - cur_time) >= deal_flash_time)
                {
                     ret_val = 1;
                     break;
                }
                else
                {
                     ret_val = 0;
                     break;
                }
            }
            else
            {
                temp_time = 0;
                if((cur_time > bt_sleepend_time)&&(bt_cb_anchor_time > cur_time))
                {
                     ret_val = 1;
                     break;
                }
                else if(bt_cb_anchor_time <= cur_time)
                {
                    temp_time = 0xFFFFFFFF - cur_time + bt_sleepend_time;
                }
                else
                {
                    temp_time = bt_sleepend_time - cur_time;
                }

                if(temp_time >= deal_flash_time )
                {
                     ret_val = 1;
                     break;
                }
                else
                {
                     ret_val = 0;
                     break;
                }
            }
        }
        else
        {
             ret_val = 1;
             break;
        }
    }while(0);

    GLOBAL_INT_RESTORE();

    return ret_val;
}


bool is_ble_erase_flash_ready(void) {
	return ble_callback_deal_handler(ERASE_FLASH_TIMEOUT);
}

bool is_ble_write_flash_ready(void) {
	return ble_callback_deal_handler(WRITE_FLASH_TIMEOUT);
}

extern bk_err_t bk_flash_erase_sector(uint32_t address);
extern bk_err_t bk_flash_erase_32k(uint32_t address);
extern bk_err_t bk_flash_erase_block(uint32_t address);

#define FLASH_OPERATE_SIZE_AND_OFFSET    (4096)
bk_err_t bk_spec_flash_write_bytes(bk_partition_t partition, const uint8_t *user_buf, uint32_t size,uint32_t offset)
{
	bk_logic_partition_t *bk_ptr = NULL;
	u8 *save_flashdata_buff  = NULL;

	bk_ptr = bk_flash_partition_get_info(partition);
	if((size + offset) > FLASH_OPERATE_SIZE_AND_OFFSET)
		return BK_FAIL;

	save_flashdata_buff= os_malloc(bk_ptr->partition_length);
	if(save_flashdata_buff == NULL)
	{
		BK_LOGD(NULL,"save_flashdata_buff malloc err\r\n");
		return BK_FAIL;
	}

	bk_flash_read_bytes((bk_ptr->partition_start_addr),(uint8_t *)save_flashdata_buff, bk_ptr->partition_length);

	bk_flash_erase_sector(bk_ptr->partition_start_addr);
	os_memcpy((save_flashdata_buff + offset), user_buf, size);
	bk_flash_write_bytes(bk_ptr->partition_start_addr ,(uint8_t *)save_flashdata_buff, bk_ptr->partition_length);

	os_free(save_flashdata_buff);
	save_flashdata_buff = NULL;

	return BK_OK;

}

__attribute__((section(".iram")))
#if CONFIG_ARCH_RISCV
void * __attribute__((no_execit, optimize("-O3"))) bk_memcpy_4w(void *dst, void *src, unsigned int size)
#else
void * __attribute__((optimize("-O3"))) bk_memcpy_4w(void *dst, const void *src, unsigned int size)  /* 4 words copy. */
#endif
{
	unsigned char *dst_ptr = (unsigned char *)dst;
	const unsigned char *src_ptr = (const unsigned char *)src;

	unsigned int temp1, temp2, temp3, temp4;

	if((((unsigned int)src_ptr ^ (unsigned int)dst_ptr) & (sizeof(unsigned int) - 1)) == 0)
	{
		while( (unsigned int)src_ptr & (sizeof(unsigned int) - 1) )
		{
			if(size == 0)
				return dst;

			size--;
			*dst_ptr++ = *src_ptr++;
		}

		const unsigned int *src_wptr = (const unsigned int *)src_ptr;
		unsigned int *dst_wptr = (unsigned int *)dst_ptr;

		while( size >= (sizeof(unsigned int) * 4) )
		{
			temp1 = src_wptr[0];
			temp2 = src_wptr[1];
			temp3 = src_wptr[2];
			temp4 = src_wptr[3];

			dst_wptr[0] = temp1;
			dst_wptr[1] = temp2;
			dst_wptr[2] = temp3;
			dst_wptr[3] = temp4;

			src_wptr += 4;
			dst_wptr += 4;
			size -= (sizeof(unsigned int) * 4);
		}

		while( size >= sizeof(unsigned int) )
		{
			*dst_wptr++ = *src_wptr++;
			size -= sizeof(unsigned int);
		}

		src_ptr = (const unsigned char *)src_wptr;
		dst_ptr = (unsigned char *)dst_wptr;
	}

	while( size > 0 )
	{
		*dst_ptr++ = *src_ptr++;
		size --;
	}

	return dst;
}

static inline bool is_64k_aligned(uint32_t addr)
{
	return ((addr & (KB(64) - 1)) == 0);
}

static inline bool is_32k_aligned(uint32_t addr)
{
	return ((addr & (KB(32) - 1)) == 0);
}

/*make sure wdt is closed!*/
bk_err_t bk_flash_erase_fast(uint32_t erase_off, uint32_t len)
{
	uint32_t erase_size = 0;
	int erase_remain = len;

	while (erase_remain > 0) {
		if ((erase_remain >= KB(64)) && is_64k_aligned(erase_off)) {
			FLASH_LOGV("64k erase: off=%x remain=%x\r\n", erase_off, erase_remain);
			bk_flash_erase_block(erase_off);
			erase_size = KB(64);
		} else if ((erase_remain >= KB(32)) && is_32k_aligned(erase_off)) {
			FLASH_LOGV("32k erase: off=%x remain=%x\r\n", erase_off, erase_remain);
			bk_flash_erase_32k(erase_off);
			erase_size = KB(32);
		} else {
			FLASH_LOGV("4k erase: off=%x remain=%x\r\n", erase_off, erase_remain);
			bk_flash_erase_sector(erase_off);
			erase_size = KB(4);
		}
		erase_off += erase_size;
		erase_remain -= erase_size;
	}

	return BK_OK;
}


