#ifndef __VI5302_EFUSE_H
#define __VI5302_EFUSE_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

uint8_t VI5302_Efuse_Write(uint8_t base, uint8_t *write_buff, uint8_t len);
uint8_t VI5302_Efuse_Read(uint8_t base, uint8_t *write_buff, uint8_t len);

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

