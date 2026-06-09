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

#pragma once

#include <common/bk_include.h>
#include <driver/hal/hal_sdio_host_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BK_ERR_SDIO_HOST_NOT_INIT            (BK_ERR_SDIO_HOST_BASE - 1) /**< sdio host not init */
#define BK_ERR_SDIO_HOST_CMD_RSP_TIMEOUT     (BK_ERR_SDIO_HOST_BASE - 2) /**< sdio host wait slave command over time */
#define BK_ERR_SDIO_HOST_CMD_RSP_CRC_FAIL    (BK_ERR_SDIO_HOST_BASE - 3) /**< sdio host wait slave command crc fail */
#define BK_ERR_SDIO_HOST_TX_FIFO_FULL        (BK_ERR_SDIO_HOST_BASE - 4) /**< sdio host tx fifo full */
#define BK_ERR_SDIO_HOST_RX_FIFO_EMPTY       (BK_ERR_SDIO_HOST_BASE - 5) /**< sdio host rx fifo empty */
#define BK_ERR_SDIO_HOST_DATA_TIMEOUT        (BK_ERR_SDIO_HOST_BASE - 6) /**< sdio host wait slave data timeout */
#define BK_ERR_SDIO_HOST_DATA_CRC_FAIL       (BK_ERR_SDIO_HOST_BASE - 7) /**< sdio host receive slave data crc fail */
#define BK_ERR_SDIO_HOST_INVALID_VOLT_RANGE  (BK_ERR_SDIO_HOST_BASE - 8) /**< sd card invalid voltage range */
#define BK_ERR_SDIO_HOST_UNSUPPORTED_FEATURE (BK_ERR_SDIO_HOST_BASE - 9) /**< sd card unsupport feature */
#define BK_ERR_SDIO_HOST_READ_DATA_FAIL (BK_ERR_SDIO_HOST_BASE - 10) /**< sdio host read data from FIFO fail */

typedef void (*sdio_host_isr_t)(void *param); /**< SDIO host interrupt service routine */

typedef enum {
	SDIO_HOST_DATA_DIR_RD = 0, /**< sdio host data direction read */
	SDIO_HOST_DATA_DIR_WR, 	   /**< sdio host data direction write */
} sdio_host_data_direction_t;

typedef struct {
	sdio_host_clock_freq_t clock_freq; /**< sdio host clock frequency */
	sdio_host_bus_width_t bus_width;   /**< sdio host data bus width */
	uint8_t dma_tx_en;	//write data to SDIO FIFO whether enable DMA(it depend on CONFIG_SDIO_GDMA_EN)
	uint8_t dma_rx_en;	//read data from SDIO FIFO whether enable DMA(it depend on CONFIG_SDIO_GDMA_EN)
} sdio_host_config_t;

typedef struct {
	uint32_t data_timeout;               /**< sdio host data timeout */
	uint32_t data_len;                   /**< sdio host data length */
	uint32_t data_block_size;            /**< sdio host data block size */
	sdio_host_data_direction_t data_dir; /**< sdio host data direction */
} sdio_host_data_config_t;

/* ========================================================================
 * Generic SDIO host controller interface (v2)
 *
 * This is the layered, controller-agnostic interface that upper protocol
 * drivers (sd_card, emmc, sdio_func) build on.  It is intentionally NOT
 * bound to a single physical controller: BK7259 has two SDIO host
 * controllers and every API selects one through sdio_host_id_t.
 * ===================================================================== */

/** @brief Physical SDIO host controller instance selector.
 *  BK7259 integrates two DesignWare MSHC controllers. */
typedef enum {
	SDIO_HOST_ID_0 = 0, /**< SDIO0, base 0x48040000, pinmux group GPIO2/3/4/5/10/11(/6~9) */
	SDIO_HOST_ID_1,     /**< SDIO1, base 0x48050000, pinmux group GPIO14~23 */
	SDIO_HOST_ID_MAX,
} sdio_host_id_t;

/** @brief Command response type.  Maps to controller RESP_TYPE field and
 *  also tells the host how many response words to read back.
 *  R4/R5 are required by SDIO peripherals (CMD5/CMD52/CMD53). */
typedef enum {
	SDIO_HOST_RESP_NONE = 0, /**< no response */
	SDIO_HOST_RESP_R1,       /**< 48-bit, normal */
	SDIO_HOST_RESP_R1B,      /**< 48-bit with busy */
	SDIO_HOST_RESP_R2,       /**< 136-bit long response (CID/CSD) */
	SDIO_HOST_RESP_R3,       /**< 48-bit OCR (ACMD41) */
	SDIO_HOST_RESP_R4,       /**< 48-bit SDIO OCR (CMD5) */
	SDIO_HOST_RESP_R5,       /**< 48-bit SDIO (CMD52/CMD53) */
	SDIO_HOST_RESP_R6,       /**< 48-bit published RCA (CMD3) */
	SDIO_HOST_RESP_R7,       /**< 48-bit card interface condition (CMD8) */
} sdio_host_resp_type_t;

/** @brief Data bus width. eMMC additionally supports 8-bit. */
typedef enum {
	SDIO_HOST_BUS_WIDTH_1 = 1, /**< 1-bit */
	SDIO_HOST_BUS_WIDTH_4 = 4, /**< 4-bit */
	SDIO_HOST_BUS_WIDTH_8 = 8, /**< 8-bit (eMMC) */
} sdio_host_bus_width2_t;

/** @brief Bus timing / speed mode. */
typedef enum {
	SDIO_HOST_TIMING_LEGACY = 0, /**< default / identification speed */
	SDIO_HOST_TIMING_SDR12,      /**< SD UHS-I SDR12 */
	SDIO_HOST_TIMING_SDR25,      /**< SD UHS-I SDR25 / SD high speed */
	SDIO_HOST_TIMING_SDR50,      /**< SD UHS-I SDR50 */
	SDIO_HOST_TIMING_SDR104,     /**< SD UHS-I SDR104 */
	SDIO_HOST_TIMING_DDR50,      /**< SD UHS-I DDR50 */
	SDIO_HOST_TIMING_MMC_HS,     /**< eMMC high speed SDR */
	SDIO_HOST_TIMING_MMC_DDR,    /**< eMMC high speed DDR */
	SDIO_HOST_TIMING_MMC_HS200,  /**< eMMC HS200 */
	SDIO_HOST_TIMING_MMC_HS400,  /**< eMMC HS400 */
} sdio_host_timing_t;

/** @brief Signal voltage for the bus. */
typedef enum {
	SDIO_HOST_SIGNAL_VOLTAGE_330 = 0, /**< 3.3V */
	SDIO_HOST_SIGNAL_VOLTAGE_180,     /**< 1.8V (UHS-I / eMMC HS200/HS400) */
} sdio_host_signal_voltage_t;

/** @brief One command request. */
typedef struct {
	uint8_t  index;                 /**< command index, e.g. 17 for READ_SINGLE_BLOCK */
	uint32_t arg;                   /**< 32-bit command argument */
	sdio_host_resp_type_t resp_type;/**< expected response type */
} sdio_host_cmd_t;

/** @brief Command response, filled by the host. */
typedef struct {
	uint32_t resp[4]; /**< response words; R2 fills all 4, others use resp[0] */
	bool     timeout; /**< CMD line timed out */
	bool     crc_err; /**< response CRC error */
} sdio_host_resp_t;

/** @brief Data transfer direction. */
typedef enum {
	SDIO_HOST_XFER_READ = 0, /**< card -> host */
	SDIO_HOST_XFER_WRITE,    /**< host -> card */
} sdio_host_xfer_dir_t;

/** @brief Data transfer engine. */
typedef enum {
	SDIO_HOST_XFER_PIO = 0,   /**< CPU FIFO copy */
	SDIO_HOST_XFER_DMA_ADMA2, /**< ADMA2 descriptor DMA */
} sdio_host_xfer_mode_t;

/** @brief One data-phase descriptor accompanying a command. */
typedef struct {
	sdio_host_xfer_dir_t  dir;        /**< read / write */
	sdio_host_xfer_mode_t mode;       /**< PIO / ADMA2 */
	uint8_t  *buf;                    /**< data buffer */
	uint32_t  block_size;             /**< bytes per block (e.g. 512; SDIO CMD53 may differ) */
	uint32_t  block_cnt;             /**< number of blocks */
	bool      byte_mode;              /**< SDIO CMD53 byte mode (reserved for sdio_func) */
} sdio_host_data_t;

/** @brief Init configuration for a host instance. */
typedef struct {
	bool     is_emmc;       /**< program controller eMMC card type bit */
	uint32_t init_clock_hz; /**< identification clock, 0 -> driver default (~400kHz) */
	sdio_host_bus_width2_t bus_width; /**< initial bus width (usually 1-bit) */
} sdio_host_cfg_t;

/** @brief Static capability description of a controller instance. */
typedef struct {
	bool     support_8bit;  /**< 8-bit bus (eMMC) supported */
	bool     support_adma2; /**< ADMA2 DMA supported */
	bool     support_sdio_irq; /**< async SDIO interrupt (DAT1) supported */
	uint32_t max_clock_hz;  /**< maximum bus clock */
} sdio_host_caps_t;

/** @brief Async SDIO interrupt callback (DAT1), used by SDIO peripherals. */
typedef void (*sdio_host_irq_cb_t)(void *arg);

#ifdef __cplusplus
}
#endif

