#ifndef __VI5302_FIRMWARE_H
#define __VI5302_FIRMWARE_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
//#include "VI5302_User_Handle.h"

/**
 * @brief   VI5302 固件存放的数据
 */
#ifdef INTERNAL_DEV
extern uint8_t VI5302_firmware_buff[8192];
#else
extern const uint8_t VI5302_firmware_buff[];
#endif

/**
 * @brief   VI5302 获取固件运行状态
 * @param   [none]
 * @return  [uint8_t]   ret:0-固件成功运行 && I2C读写无异常;other-异常（固件没有成功运行 || I2C读写有异常）
 */
uint8_t Get_VI5302_Download_Firmware_Status(void);

/**
 * @brief   VI5302 下固件前的配置
 * @param   [none]
 * @return  [uint8_t]   ret:0-操作成功（I2C读写无异常）;other-异常（I2C读写有异常）
 */
uint8_t VI5302_Write_Firmware_PreConfig(void);

/**
 * @brief 	VI5302 写完固件后的配置
 * @param 	[none]
 * @return 	[uint8_t]	ret:0-操作成功（I2C读写无异常）;other-异常（I2C读写有异常）
 */
uint8_t VI5302_Write_Firmware_Post_Config(void);

/**
 * @brief   VI5302 写固件
 * @param   [uint8_t] *mode：0-内部数据的固件;1-外部传输给MCU，保存在Flash中的固件
 * @param   [uint16_t] fw_size：固件长度
 * @return  [uint8_t]   ret：0-操作成功（I2C读写无异常）;other-异常（I2C读写有异常）
 */
//uint8_t VI5302_Download_Firmware(uint8_t mode, uint16_t fw_size);
uint8_t VI5302_Download_Firmware(uint8_t *Firmware_buff, uint16_t size);

uint16_t VI5302_FirmwareSize(void);


#endif /* __VI5302_FIRWMARE_H */
