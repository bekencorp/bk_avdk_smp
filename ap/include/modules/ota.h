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

#ifndef _OTA_H_
#define _OTA_H_

#include <driver/flash_partition.h>
#ifdef __cplusplus
extern "C" {
#endif

/** @brief   This enumeration defines which partition needs update.  */
typedef enum S_PART
{
    UPDATE_A_PART = 0,
    UPDATE_B_PART 
}part_flag;

/** @brief   This enumeration defines which partition executes finally.  */
typedef enum OTA_EXEC_PART
{
    EXEX_A_PART = 0,
    EXEC_B_PART 
}exec_flag;

/** @brief   This enumeration defines which partition executes, when customer provisionally and finally decided.  */
typedef enum EXEC_CONFIRM_TAG
{
    CONFIRM_EXEC_A = 3, //excute A partiotion 
    CONFIRM_EXEC_B = 4, //excute B partiotion 
}ota_confirm_flag,ota_temp_exec_flag;

/** @brief   This enumeration defines where resources are written during an OTA update.  */
typedef enum
{
	OTA_WR_TO_FLASH,    //ota write to flash
	OTA_WR_TO_SD_CARD,  //ota write to sdcard
	OTA_WR_TO_MAX,
}ota_wr_destination_t;

/** 
* @brief  Get ota current execute partition info.
* 
* @return ota_exec_flag
* 	 0:  means current execute A partition.
* 	 1:  means current execute B partition.
*/
uint8 bk_ota_get_current_partition(void);

/**
* @brief  Get the OTA target (update) partition: the slot opposite the one
*         currently running. Derived on demand from bk_ota_get_current_partition(),
*         so there is no cached global state to keep in sync.
*
* @return part_flag
* 	 UPDATE_A_PART(0): OTA writes/verifies slot A (currently running B).
* 	 UPDATE_B_PART(1): OTA writes/verifies slot B (currently running A).
*/
part_flag bk_ota_get_update_partition(void);

/** 
* @brief  Accept the OTA image
*         This API is for MCUBOOT SWAP strategy only, if the image is accepted, then
*         the MCUBOOT will make the image permanently, otherwise it will revert the
*         OTA process.
*/
void bk_ota_accept_image(void);

/** 
* @brief  when do ota update,customer can call this interface .
* 
* @param  Enables to execute a single HTTP request on a given URL
*         (for example: http://192.168.32.78/C%3A/Users/app_pack7237.rbl)
* 
* @return ret;
* 	 0:  means update sucess.
* 	 -others:means update failure. 
*/
int bk_http_ota_download(const char *uri);

/** 
* @brief  when do ota update,customer can call this new interface .(This interface supports both regular firmware OTA updates and resource upgrades..)
* 
* @param1  Enables to execute a single HTTP request on a given URL
*         (for example: http://192.168.32.78/C%3A/Users/app_pack7237.rbl)
*
* @param2  ota_dest_id : Where should OTA downloaded files be written? e.g., 0: flash, 1: SD card.
*
* @return ret;
* 	 0:  means update sucess.
* 	 -others:means update failure. 
*/
int bk_ota_start_download(const char *url,  ota_wr_destination_t ota_dest_id);

#if CONFIG_OTA_POSITION_INDEPENDENT_AB
/** 
* @brief  when do ota position independent update,customer finally decided which partition to execute.
*/
void bk_ota_double_check_for_execution(void);
#endif

#define INS_NO_CRC_CHUNK  (32)
#define INS_CRC_CHUNK     (34)
//#endif

#ifdef __cplusplus
}
#endif

#endif /* __HTTPCLIENT_H__ */
