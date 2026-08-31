
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __VI5302_ALGORITHM_H
#define __VI5302_ALGORITHM_H



/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

uint16_t VI5302_Calculate_Checksum(uint8_t *data_buff, int len);

int32_t vi5302_calculate_pileup_bias(uint8_t ma_sum, uint32_t peak, uint32_t noise, uint32_t integral_times);
int32_t vi5302_calculate_noise_bias(uint32_t noise, uint32_t integral_times);
void vi5302_calculate_confidence(uint32_t *peaks, uint8_t * tof_bins, uint32_t noise_r, uint32_t integral_times, uint32_t ma_sum, uint8_t flag, uint8_t *confidences);
#endif /* __LED_MANAGE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/



