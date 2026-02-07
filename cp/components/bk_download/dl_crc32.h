/*
 * crc32.h
 *
 *  Created on: 2017-5-15
 *      Author: gang.cheng
 */

#ifndef __DL_CRC32_H_
#define __DL_CRC32_H_

#include <stdint.h>


#define TABLE_CRC

#ifdef __cplusplus
extern "C" {
#endif

uint32_t dl_calc_crc32(uint32_t crc, const uint8_t *buf, int len);
unsigned int dl_make_crc32_table(void) ;

#ifdef __cplusplus
}
#endif

#endif /* __DL_CRC32_H_ */

