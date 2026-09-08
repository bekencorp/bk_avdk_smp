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

#ifndef __FLASH_BYPASS_H__
#define __FLASH_BYPASS_H__

#include <soc/soc.h>

#ifdef __cplusplus
extern "C" {
#endif

/*Write Enable for Status Register*/
#define FLASH_CMD_WR_EN_SR              (0x6)
#define FLASH_CMD_WR_DISABLE           (0x04)

/*Write Enable for Volatile Status Register (BDK-1335: write SR with volatile way)*/
#define FLASH_CMD_WR_EN_VSR            (0x50)

/*Write Status Register*/
#define FLASH_CMD_WR_SR                 (0x31)

/*Standard Write Status Register-1 command, used by the volatile SR write sequence*/
#define FLASH_CMD_WR_SR_S0_S7          (0x01)

/*Others*/
#define FLASH_CMD_QUAD_IO_FAST_READ     (0xEB)
#define FLASH_GD25Q32C_SR_QUAD_EN       (0x2)
#define FLASH_CMD_ENTER_DEEP_PWR_DW     (0xB9)
#define FLASH_CMD_EXIT_DEEP_PWR_DW      (0xAB)

/*SYS regs define*/
#define SYS_R_ADD_X(x)                  (0x44010000+((x)*4) + SOC_ADDR_OFFSET)

/*SPI0 regs define (mirror of middleware soc/spi_reg.h, not present under TFM)*/
#define SPI_R_BASE(_id)                 (SOC_SPI_REG_BASE + (_id) * 0x1010000)
#define SPI_R_CTRL(_id)                 (SPI_R_BASE(_id) + 4 * 0x04)
#define SPI_R_CFG(_id)                  (SPI_R_BASE(_id) + 4 * 0x05)
#define SPI_R_INT_STATUS(_id)           (SPI_R_BASE(_id) + 4 * 0x06)
#define SPI_R_DATA(_id)                 (SPI_R_BASE(_id) + 4 * 0x07)

#define SPI_CFG_TRX_LEN_MASK            (0xffffU)
#define SPI_CFG_TX_TRAHS_LEN_POSI       (8)
#define SPI_CFG_RX_TRAHS_LEN_POSI       (20)
#define SPI_CFG_RX_FIN_INT_EN           (BIT(3))
#define SPI_CFG_TX_FIN_INT_EN           (BIT(2))
#define SPI_CFG_RX_EN                   (BIT(1))
#define SPI_CFG_TX_EN                   (BIT(0))
#define SPI_CFG_TX_EN_ONE_BYTE          (0x10D)
#define SPI_CFG_TX_EN_TWO_BYTE          (0x20D)
#define SPI_STATUS_TX_FINISH_INT        (BIT(13))
#define SPI_STATUS_TXFIFO_WR_READY      (BIT(1))
#define SPI_STATUS_RXFIFO_RD_READY      (BIT(2))

#define FLASH_ID_GD25Q32C               (0xC84016)
#define FLASH_ID_TH25Q64                (0xCD6017)
#define QE_RETRY_TIMES                  (10)

#if CONFIG_SOC_BK7236XX
/*
 * SPI-controlled flash write: directly drives the SPI master controller to bypass
 * the flash controller and issue raw flash commands. Used by the volatile status
 * register write sequence (0x50 + 0x01). Must execute from RAM because the flash
 * interface is switched to SPI during the transfer (no XIP fetch possible).
 */
__attribute__((section(".iram"))) int flash_bypass_op_write(uint8_t *op_code, uint8_t *tx_buf, uint32_t tx_len);
#endif

#ifdef __cplusplus
}
#endif
#endif //__FLASH_BYPASS_H__
// eof
