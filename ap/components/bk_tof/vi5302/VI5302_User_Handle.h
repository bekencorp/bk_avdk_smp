/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __VI5302_USER_HANDLE_H
#define __VI5302_USER_HANDLE_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

//#include "api_dev.h" // 内部开发头文件，客户需删除
// 用户适配层，用户需要根据实际硬件平台进行适配
#define VI5302_EXIT_75MHZ_MODE      0

/**
	\brief 						I2C写函数
	\param dev_addr： 设备地址，默认0xD8
	\param addr：			寄存器地址,注意是16位的寄存器地址
	\param pValue：		写数据缓存区地址
	\param tlen：			长度
	\param return：		[uint8_t]0-操作成功（I2C写无异常）;other-异常（I2C写有异常）
	**/
uint8_t IIC_Write_X_Bytes(uint8_t dev_addr, uint16_t addr, uint8_t *pValue, uint16_t tlen);
/**
	\brief 						I2C读函数
	\param dev_addr： 设备地址，默认0xD8
	\param addr：			寄存器地址，注意是16位的寄存器地址
	\param pValue：		读数据缓存区地址
	\param tlen：			长度
	\param return：		[uint8_t]0-操作成功（I2C读无异常）;other-异常（I2C写读异常）
	**/
uint8_t IIC_Read_X_Bytes(uint8_t dev_addr, uint16_t addr, uint8_t *value, uint16_t tlen);
uint8_t VI5302_IIC_Read_One_Byte(uint16_t addr, uint8_t *value);
uint8_t VI5302_IIC_Read_X_Bytes(uint16_t addr, uint8_t *value, uint16_t tlen);
uint8_t VI5302_IIC_Write_One_Byte(uint16_t addr, uint8_t value);
uint8_t VI5302_IIC_Write_X_Bytes(uint16_t addr, uint8_t *pValue, uint16_t tlen);

void VI5302_GPIO_Interrupt_Handle(void);

void VI5302_XSHUT_Enable(uint8_t state);
void VI5302_Delay_Ms(uint16_t nMs);
void VI5302_7V5_Enable(uint8_t state);

#endif /* __VI5302_USER_HANDLE_H */


