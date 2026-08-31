#ifndef __VI5302_SYSTEM_DATA_H
#define __VI5302_SYSTEM_DATA_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

uint8_t VI5302_Write_System_Data(uint8_t offset_addr, uint8_t *buff, uint8_t len);
uint8_t VI5302_Read_System_Data(uint8_t offset_addr, uint8_t *buff, uint8_t len);

uint8_t VI5302_Set_Sys_RCO(uint8_t value);
uint8_t VI5302_Get_Sys_RCO(uint8_t *value);

uint8_t VI5302_Set_Sys_BVD(uint8_t value);
uint8_t VI5302_Get_Sys_BVD(uint8_t *value);

uint8_t VI5302_Set_Sys_TDC(uint8_t value);
uint8_t VI5302_Get_Sys_TDC(uint8_t *value);

uint8_t VI5302_Set_Sys_TDC_Delay(uint8_t value);
uint8_t VI5302_Get_Sys_TDC_Delay(uint8_t *value);

uint8_t VI5302_Set_Sys_PW_DS(uint8_t value);
uint8_t VI5302_Get_Sys_PW_DS(uint8_t *value);

uint8_t VI5302_Set_Sys_PW(uint8_t value);
uint8_t VI5302_Get_Sys_PW(uint8_t *value);

uint8_t VI5302_Set_Sys_Integral_Time(uint32_t integral_time);
uint8_t VI5302_Get_Sys_Integral_Time(uint32_t *integral_time);

uint8_t VI5302_Set_Sys_DelayTime(uint16_t delay_time);
uint8_t VI5302_Get_Sys_DelayTime(uint16_t *delay_time);

uint8_t VI5302_Set_Sys_Histogram_MA_Window_Data(uint8_t *setting_buff);
uint8_t VI5302_Get_Sys_Histogram_MA_Window_Data(uint8_t *getting_buff);

uint8_t VI5302_Set_Sys_Xtalk_Offset(uint8_t value);
uint8_t VI5302_Get_Sys_Xtalk_Offset(uint8_t *value);

uint8_t VI5302_Set_Sys_Xtalk_Position(uint8_t value);
uint8_t VI5302_Get_Sys_Xtalk_Position(uint8_t *value);

uint8_t VI5302_Set_Sys_Temperature_Enable(uint8_t value);
uint8_t VI5302_Get_Sys_Temperature_Enable(uint8_t *value);

uint8_t VI5302_Set_Integralcounts_Frame(uint8_t fps, uint32_t intecoutns);

#endif
