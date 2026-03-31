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

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_err.h>

#define BK_ERR_DMA_HAL_INVALID_ADDR (BK_ERR_DMA_HAL_BASE - 1)

/**
 * @brief DMA defines
 * @addtogroup bk_api_dma_defs DMA API group
 * @{
 */

typedef uint8_t dma_unit_t;          /**< DMA uint id */
typedef uint8_t dma_chan_priority_t; /**< DMA channel priority:the value is higher then the priority is higher */

/**
 * @}
 */

/**
 * @brief DMA enum defines
 * @defgroup bk_api_dma_enum DMA enums
 * @ingroup bk_api_dma
 * @{
 */

typedef enum {
	DMA_ID_0 = 0,  /**< DMA channel 0 */
	DMA_ID_1,      /**< DMA channel 1 */
	DMA_ID_2,      /**< DMA channel 2 */
	DMA_ID_3,      /**< DMA channel 3 */
	DMA_ID_4,      /**< DMA channel 4 */
	DMA_ID_5,      /**< DMA channel 5 */
	DMA_ID_6    ,  /**< DMA channel 6 */
	DMA_ID_7,      /**< DMA channel 7 */
	DMA_ID_8,      /**< DMA channel 8 */
	DMA_ID_9,      /**< DMA channel 9 */
	DMA_ID_10,     /**< DMA channel 10 */
	DMA_ID_11,     /**< DMA channel 11 */
	DMA_ID_12,     /**< DMA channel 12 */
	DMA_ID_13,     /**< DMA channel 13 */
	DMA_ID_14,     /**< DMA channel 14 */
	DMA_ID_15,     /**< DMA channel 15 */
	DMA_ID_MAX,    /**< DMA channel max */
} dma_id_t;

/**
 * @brief CPU core for DMA Interrupt Differentiation Notification
 *
 * @Enumeration option selection core notifies DMA to interrupt
 * after completion of transport
 *
 * @Notification of terminal selection based on channel operation
 * @{
 */
typedef enum {
	DMA_INT_0 = 0,  /**< DMA INT 0  */
	DMA_INT_1,      /**< DMA INT 1  */
	DMA_INT_2,      /**< DMA INT 2  */
	DMA_INT_3,      /**< DMA INT 3  */
	DMA_INT_MAX,    /**< DMA channel max */
} dma_int_id_t;


typedef enum {
    DMA_DEV_DTCM = 0,    /**< DMA device DTCM */
    DMA_DEV_LA,          /**< DMA device LA */
    DMA_DEV_HSSPI,       /**< DMA device HSSPI */
    DMA_DEV_AUDIO,       /**< DMA device AUDIO */
    DMA_DEV_AUDIO_RX,    /**< DMA device AUDIO */
    DMA_DEV_SDIO,        /**< DMA device SDIO */
    DMA_DEV_SDIO_RX,     /**< DMA device SDIO */
    DMA_DEV_UART1,       /**< DMA device UART1 */
    DMA_DEV_UART1_RX,    /**< DMA device UART1 */
    DMA_DEV_UART2,       /**< DMA device UART2 */
    DMA_DEV_UART2_RX,    /**< DMA device UART 2 RX */
    DMA_DEV_UART3,       /**< DMA device UART3 */
    DMA_DEV_UART3_RX,    /**< DMA device UART3 */
    DMA_DEV_UART4,       /**< DMA device UART4 */
    DMA_DEV_UART4_RX,    /**< DMA device UART4 */
    DMA_DEV_I2S,         /**< DMA device I2S */
    DMA_DEV_I2S_CH1,     /**< DMA device I2S */
    DMA_DEV_I2S_CH2,     /**< DMA device I2S */
    DMA_DEV_I2S_RX,      /**< DMA device I2S */
    DMA_DEV_I2S_RX_CH1,  /**< DMA device I2S */
    DMA_DEV_I2S_RX_CH2,  /**< DMA device I2S */
    DMA_DEV_I2S1,        /**< DMA device I2S */
    DMA_DEV_I2S1_RX,     /**< DMA device I2S */
    DMA_DEV_I2S2,        /**< DMA device I2S */
    DMA_DEV_I2S2_RX,     /**< DMA device I2S */
    DMA_DEV_GSPI0,       /**< DMA device GSPI1 */
    DMA_DEV_GSPI0_RX,    /**< DMA device GSPI1 */
    DMA_DEV_GSPI1,       /**< DMA device GSPI2 */
    DMA_DEV_GSPI1_RX,    /**< DMA device GSPI2 */
    DMA_DEV_GSPI2,       /**< DMA device GSPI3 */
    DMA_DEV_JPEG,        /**< DMA device JPEG */
    DMA_DEV_PSRAM_VIDEO, /**< DMA device PSRAM VIDEO */
    DMA_DEV_PSRAM_AUDIO, /**< DMA device PSRAM AUDIO */
    DMA_DEV_USB,         /**< DMA device USB */
    DMA_DEV_LCD_CMD,     /**< DMA device LCD CMD */
    DMA_DEV_LCD_DATA,    /**< DMA device LCD DATA */
    DMA_DEV_DISP_RX,     /**< DMA device DISP RX */
    DMA_DEV_SDMADC_RX,   /**< DMA device SDMADC RX */
    DMA_DEV_AHB_MEM,     /**< DMA device AHB_MEM */
    DMA_DEV_SPI0,        /**< DMA device SPI 0 */
    DMA_DEV_SPI1,        /**< DMA device SPI 1 */
    DMA_DEV_H264,        /**< DMA device H264 */
    DMA_DEV_I2S0,        /**< DMA device I2S0 */
    DMA_DEV_I2S0_RX,     /**< DMA device I2S0 RX */
    DMA_DEV_GSPI2_RX,    /**< DMA device GSPI2 RX */
    DMA_DEV_AUD_DMIC,    /**< DMA device aud dmic */
    DMA_DEV_AUD_MIC0,    /**< DMA device aud mic0 */
    DMA_DEV_AUD_SPK0,    /**< DMA device aud spk0 */
    DMA_DEV_AUD_SPK0_HINT,    /**< DMA device aud spk0 hint */
    DMA_DEV_I2S3,             /**< DMA device I2S3 */
    DMA_DEV_I2S3_RX,          /**< DMA device I2S3 RX */
    DMA_DEV_SPDIF0,           /**< DMA device SPDIF0 */
    DMA_DEV_SPDIF1,           /**< DMA device SPDIF1 */
    DMA_DEV_SADC_RX,          /**< DMA device SADC RX */
    DMA_DEV_AUD_SPK0_CALL,    /**< DMA device aud spk0 call */
    DMA_DEV_AUD_MIC1,         /**< DMA device aud mic1 */
    DMA_DEV_AUD_SPK1_A2DP,   /**< DMA device aud spk1 a2dp */
    DMA_DEV_AUD_SPK1_HINT,   /**< DMA device aud spk1 hint dma */
    DMA_DEV_AUD_SPK1_CALL,   /**< DMA device aud spk1 call */
    DMA_DEV_GSPI3,       /**< DMA device GSPI3 */
    DMA_DEV_GSPI3_RX,    /**< DMA device GSPI3 RX */
    DMA_DEV_UART5,       /**< DMA device UART5 */
    DMA_DEV_UART5_RX,    /**< DMA device UART5 RX */
    DMA_DEV_I2S4,        /**< DMA device I2S4 */
    DMA_DEV_I2S4_RX,     /**< DMA device I2S4 RX */
    DMA_DEV_XDAC0,       /**< DMA device XDAC0 */
    DMA_DEV_XDAC1,       /**< DMA device XDAC1 */
    DMA_DEV_MAX,
} dma_dev_t;

typedef enum {
    DMA_DATA_WIDTH_8BITS = 0, /**< DMA data width 8bit */
    DMA_DATA_WIDTH_16BITS,    /**< DMA data width 16bit */
    DMA_DATA_WIDTH_32BITS,    /**< DMA data width 32bit */
} dma_data_width_t;

typedef enum {
    DMA_WORK_MODE_SINGLE = 0, /**< DMA work mode single_mode */
    DMA_WORK_MODE_REPEAT,     /**< DMA work mode repeat_mode (forever repeat until software clear dma_en) */
} dma_work_mode_t;

typedef enum {
    DMA_PRIO_MODE_ROUND_ROBIN = 0, /**< DMA priority mode round-robin(all dma priority are the same) */
    DMA_PRIO_MODE_FIXED_PRIO,      /**< DMA priority mode fixed prio(depend on chan_prio) */
} dma_priority_mode_t;

typedef enum {
    DMA_ADDR_INC_DISABLE = 0, /**< DMA disable addrress increase */
    DMA_ADDR_INC_ENABLE,      /**< DMA enable addrress increase */
} dma_addr_inc_t;

typedef enum {
    DMA_ADDR_LOOP_DISABLE = 0, /**< DMA disable addrress loop */
    DMA_ADDR_LOOP_ENABLE,      /**< DMA enable addrress loop */
} dma_addr_loop_t;

typedef enum {
	DMA_ATTR_NON_SEC = 0,  /**< DMA non-secure attr */
	DMA_ATTR_SEC,          /**< DMA secure attr */
} dma_sec_attr_t;


typedef enum {
	BURST_LEN_SINGLE = 0,
	BURST_LEN_INC4,
	BURST_LEN_INC8,
	BURST_LEN_INC16
} dma_burst_len_t;

typedef enum {
    DMA_TRANS_DEFAULT = 0,         /**< DMA TRANS DEFAULT */
    DMA_TRANS_RGB_TO_YUV,          /**< DMA TRANS RGB_TO_YUV */
    DMA_TRANS_YUV_TO_RGB,          /**< DMA TRANS YUV_TO_RGB */
    DMA_TRANS_MAX,
} dma_pixel_trans_type_t;

/**
 * @}
 */

/**
 * @brief DMA struct defines
 * @defgroup bk_api_dma_structs structs in DMA
 * @ingroup bk_api_dma
 * @{
 */

typedef struct {
    dma_dev_t dev;                /**< DMA device */
    dma_data_width_t width;       /**< DMA data width */
    dma_addr_inc_t addr_inc_en;   /**< enable/disable DMA address increase */
    dma_addr_loop_t addr_loop_en; /**< enable/disable DMA address loop */
    uint32_t start_addr;          /**< DMA start address */
    uint32_t end_addr;            /**< DMA end address */
} dma_port_config_t;

typedef struct {
    dma_work_mode_t mode;          /**< DMA work mode */
    dma_chan_priority_t chan_prio; /**< DMA channel prioprity */
    dma_port_config_t src;         /**< DMA source configuration */
    dma_port_config_t dst;         /**< DMA dest configuration */
    dma_pixel_trans_type_t trans_type; /**< DMA TRANS TYPE */
    uint32_t dest_wr_intlv; /**< DMA Destination Write operate interval.unit is cycle */
    uint32_t src_rd_intlv; /**< DMA Source Read operate interval.unit is cycle */
} dma_config_t;

#if CONFIG_GENERAL_DMA_LINK_MODE
typedef volatile struct {
    uint32_t src_addr;           // DES0: Source Address[31:0]
    uint32_t dst_addr;           // DES1: Destination Address[31:0]
    
    union {
        struct {
            // 注意：位域顺序从左到右对应高位到低位（高位在前）
            // 表格顺序：int_half_finish_en | int_finish_en | Length[29:0]
            uint32_t length : 30;      // Bit 29-0: Length
            uint32_t int_finish_en : 1;    // Bit 30: Finish interrupt enable
            uint32_t int_half_finish_en : 1; // Bit 31: Half finish interrupt enable (高位，表格最左边)
        } bits;
        uint32_t control;                  // DES2: Control word
    } ctrl;
    
    uint32_t next_desc_addr;               // DES3: Next descriptor address，address must be 128 bit aligned
} dma_descriptor_t;

typedef struct {
    uint32_t src_addr;           /**< Source address (DES0) */
    uint32_t dst_addr;           /**< Destination address (DES1) */
    
    union {
        struct {
            // Bit field order matches dma_descriptor_t.ctrl.bits
            // Bit order: int_half_finish_en | int_finish_en | length[29:0]
            uint32_t length : 30;              /**< Transfer length (Bit 29-0) */
            uint32_t int_finish_en : 1;        /**< Finish interrupt enable (Bit 30) */
            uint32_t int_half_finish_en : 1;    /**< Half finish interrupt enable (Bit 31) */
        } bits;
        uint32_t control;                      /**< Control word (DES2) - can be set directly */
    } ctrl;
} dma_link_config_t;

#endif // CONFIG_GENERAL_DMA_LINK_MODE

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

