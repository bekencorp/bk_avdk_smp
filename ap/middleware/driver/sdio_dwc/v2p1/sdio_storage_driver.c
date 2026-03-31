// Copyright 2020-2024 Beken
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

#include <driver/io_matrix.h>
#include <driver/hal/hal_gpio_types.h>
#include "sdio_storage_driver.h"
#include "sys_a35_ll.h"
#include "sys_ana_ll.h"
#include "driver/sdio_storage.h"
#include "mmc_dev.h"
#include "sdhci.h"
#include "sd_card.h"
#include "sd_cmds.h"
#include "common.h"

bk_err_t bk_sdio_storage_initialize(uint32_t physical_drive_number, SDIO_STORAGE_CFG_T *cfg_ptr)
{
    int ret;
    uint32_t instance_id;
    struct sd_card_t *sdcard_ptr = NULL;

    /*TODO: convert physical drive index to instance_id*/
    instance_id = physical_drive_number - 1;

    if(sd_card_is_enumerate_ok(instance_id)){
        return BK_OK;
    }

    ret = sd_card_initialize(instance_id, &sdcard_ptr);
    if(RET_SUCCESS != ret){
        return BK_FAIL;
    }
    
    ret = sd_card_register(instance_id, sdcard_ptr);
    if(RET_SUCCESS != ret){
        return BK_FAIL;
    }

    ret = mmc_dev_enumerate(sdcard_ptr, cfg_ptr->speed_mode, 
                        cfg_ptr->bus_width, cfg_ptr->xfer_mode,
                        cfg_ptr->emmc_vdd, cfg_ptr->mmcm_clock);
    if(MSHC_SUCCESS != ret){
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_sdio_storage_uninitialize(uint32_t physical_drive_number)
{
    int ret;
    uint32_t instance_id;

    /*TODO: convert physical drive index to instance_id*/
    instance_id = physical_drive_number - 1;
    
    sd_card_unregister(instance_id);
    ret = sd_card_uninitialize(instance_id);
    if(RET_SUCCESS != ret){
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_sdio_storage_read(uint32_t physical_drive_number, uint8_t *buf, uint32_t sector, uint32_t blk_count)
{
    bk_err_t ret;
    int ret_val;
    uint32_t block_addr;
    uint32_t instance_id;
    struct sd_card_t *sdcard_ptr;
    struct mmc_dev_t *mmc_dev;

    instance_id = physical_drive_number - 1;
    sdcard_ptr  = sd_card_get_object(instance_id);
    mmc_dev = (struct mmc_dev_t *)sdcard_ptr->mmc_dev;
    block_addr = sector;
    ret_val = mmc_read_block(mmc_dev, mmc_dev->cur_mmc_cmd, block_addr, blk_count, buf, SDIO_STORAGE_BLOCK_SIZE * blk_count);
    if((XSTATE_CMD_DONE == mmc_dev->cur_mmc_cmd->state) 
        && (IO_STATUS_SUCCESS == mmc_dev->cur_mmc_cmd->cur_cmd.status)){
        ret = BK_OK;
    }else{
        ret = ERROR_OPER_FAIL;
    }

    (void)ret_val;

    return ret;
}

bk_err_t bk_sdio_storage_write(uint32_t physical_drive_number, uint8_t *buf, uint32_t sector, uint32_t blk_count)
{
    bk_err_t ret;
    int ret_val;
    uint32_t block_addr;
    uint32_t instance_id;
    struct sd_card_t *sdcard_ptr;
    struct mmc_dev_t *mmc_dev;

    instance_id = physical_drive_number - 1;
    sdcard_ptr  = sd_card_get_object(instance_id);
    mmc_dev = (struct mmc_dev_t *)sdcard_ptr->mmc_dev;
    block_addr = sector;
    ret_val = mmc_write_block(mmc_dev, mmc_dev->cur_mmc_cmd, block_addr, blk_count, buf, SDIO_STORAGE_BLOCK_SIZE * blk_count);
    if((XSTATE_CMD_DONE == mmc_dev->cur_mmc_cmd->state) 
        && (IO_STATUS_SUCCESS == mmc_dev->cur_mmc_cmd->cur_cmd.status)){
        ret = BK_OK;
    }else{
        ret = ERROR_OPER_FAIL;
    }

    (void)ret_val;

    return ret;
}

bk_err_t bk_sdio_storage_status(uint32_t physical_drive_number)
{
    /*FIXME:todo mmc_cmd13_send_status*/
    return BK_OK;
}

bk_err_t bk_sdio_storage_ioctl(uint32_t physical_drive_number, uint32_t cmd, void *buf)
{
    bk_err_t ret = BK_OK;
    uint32_t instance_id;
    struct sd_card_t *sdcard_ptr;

    instance_id = physical_drive_number - 1;
    sdcard_ptr  = sd_card_get_object(instance_id);

    if(!sd_card_is_enumerate_ok(instance_id)){
        return BK_FAIL;
    }

    switch (cmd){    
        case CMD_CTRL_SYNC:
            /* Makes sure that the device has finished pending write process*/
        	break;

        case CMD_GET_SECTOR_COUNT:
            /* Retrieves number of available sectors, used by f_mkfs and f_fdisk function*/
            *((uint32_t *)buf) = sdcard_ptr->csd.word[2];
            SDIOD_LOGI("CMD_GET_SECTOR_COUNT:0x%x\r\n", sdcard_ptr->csd.word[2]);
        	break;

        case CMD_GET_SECTOR_SIZE:
            /* Valid sector sizes are 512, 1024, 2048 and 4096*/
            *((uint32_t *)buf) = sdcard_ptr->csd.word[3];
            SDIOD_LOGI("CMD_GET_SECTOR_SIZE:0x%x\r\n", sdcard_ptr->csd.word[3]);
        	break;

        case CMD_GET_BLOCK_SIZE:
            /* Retrieves erase block size in unit of sector, 1 to 32768 in power of 2*/
            ret = 1;//Return 1 if it is unknown or in non flash memory media
        	break;

        case CMD_CTRL_TRIM:
        	break;

        case CMD_CTRL_POWER:
        	break;
        case CMD_CTRL_LOCK:
        	break;
        case CMD_CTRL_EJECT:
        	break;
        case CMD_CTRL_FORMAT:
        	break;
        case CMD_MMC_GET_TYPE:
        	break;
        case CMD_MMC_GET_CSD:
        	break;
        case CMD_MMC_GET_CID:
        	break;
        case CMD_MMC_GET_OCR:
        	break;
        case CMD_MMC_GET_SDSTAT:
        	break;
        case CMD_ISDIO_READ:
        	break;
        case CMD_ISDIO_WRITE:
        	break;
        case CMD_ISDIO_MRITE:
        	break;
        case CMD_ATA_GET_REV:
        	break;
        case CMD_ATA_GET_MODEL:
        	break;
        case CMD_ATA_GET_SN:
        	break;
        default:
            break;
    }

    return ret;
}

bk_err_t bk_sdio_storage_driver_init(void)
{
#if CONFIG_SDIO_DWC_TEST
        int bk_sdio_host_register_cli_test_feature(void);
        bk_sdio_host_register_cli_test_feature();
#endif

        return BK_OK;
}

bk_err_t bk_sdio_storage_driver_deinit(void)
{
        return BK_OK;
}

// eof

