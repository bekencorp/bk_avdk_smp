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

#define BK_ERR_HPDMA_HAL_INVALID_ALIGN (BK_ERR_HPDMA_HAL_BASE - 1) /**< HPDMA addr is not 128-bit aligned */
#define BK_ERR_HPDMA_HAL_INVALID_YSIZE (BK_ERR_HPDMA_HAL_BASE - 2) /**< HPDMA ysize must be 0 when addr loop is enabled */


/**
 * @brief DMA defines
 * @addtogroup bk_api_dma_defs DMA API group
 * @{
 */

typedef uint8_t hpdma_unit_t;          /**< DMA uint id */
typedef uint8_t hpdma_chan_priority_t; /**< DMA channel priority:the value is higher then the priority is higher */

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
	HPDMA_ID_0 = 0,  /**< DMA channel 0 */
	HPDMA_ID_1,      /**< DMA channel 1 */
	HPDMA_ID_2,      /**< DMA channel 2 */
	HPDMA_ID_3,      /**< DMA channel 3 */
	HPDMA_ID_MAX,    /**< DMA channel max */
} hpdma_id_t;

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
	HPDMA_INT_0 = 0,  /**< DMA INT 0  m55 cpu core 0*/
	HPDMA_INT_1,      /**< DMA INT 1  m55 cpu core 1*/
	HPDMA_INT_2,      /**< DMA INT 2  m52 */
	HPDMA_INT_3,      /**< DMA INT 3  NA*/
    HPDMA_INT_4,      /**< DMA INT 4  NA*/
	HPDMA_INT_MAX,    /**< DMA channel max */
} hpdma_int_id_t;


typedef enum {
    HPDMA_DEV_DTCM = 0,    /**< DMA device DTCM */
    HPDMA_DEV_UART5,       /**< DMA device UART5 */
    HPDMA_DEV_MAX,
} hpdma_dev_t;

typedef enum {
    HPDMA_DATA_WIDTH_8BITS = 0, /**< DMA data width 8bit */
    HPDMA_DATA_WIDTH_16BITS,    /**< DMA data width 16bit */
    HPDMA_DATA_WIDTH_32BITS,    /**< DMA data width 32bit */
    HPDMA_DATA_WIDTH_64BITS,    /**< DMA data width 32bit */
    HPDMA_DATA_WIDTH_128BITS,    /**< DMA data width 32bit */
} hpdma_data_width_t;

typedef enum {
    HPDMA_WORK_MODE_SINGLE = 0, /**< DMA work mode single_mode */
    HPDMA_WORK_MODE_REPEAT,     /**< DMA work mode repeat_mode (forever repeat until software clear dma_en) */
} hpdma_work_mode_t;

typedef enum {
    HPDMA_PRIO_MODE_ROUND_ROBIN = 0, /**< DMA priority mode round-robin(all dma priority are the same) */
    HPDMA_PRIO_MODE_FIXED_PRIO,      /**< DMA priority mode fixed prio(depend on chan_prio) */
} hpdma_priority_mode_t;

typedef enum {
    HPDMA_ADDR_INC_DISABLE = 0, /**< DMA disable addrress increase */
    HPDMA_ADDR_INC_ENABLE,      /**< DMA enable addrress increase */
} hpdma_addr_inc_t;

typedef enum {
    HPDMA_ADDR_LOOP_DISABLE = 0, /**< DMA disable addrress loop */
    HPDMA_ADDR_LOOP_ENABLE,      /**< DMA enable addrress loop */
} hpdma_addr_loop_t;

typedef enum {
	HPDMA_ATTR_NON_SEC = 0,  /**< DMA non-secure attr */
	HPDMA_ATTR_SEC,          /**< DMA secure attr */
} hpdma_sec_attr_t;


/*
 * P1 (HPDMA review): Reg23 hardware encodes 4 burst lengths in 2 bits:
 *   0=SINGLE, 1=INC4, 2=INC8, 3=INC16.
 * Previously the enum stopped at INC8 while LVGL/GPU passed the raw
 * value 0x03 to bk_hpdma_set_*_burst_len(); the named constant was
 * missing, making intent unclear and breaking the few sites that tried
 * to use HPDMA_BURST_LEN_INC16 (e.g. the #if 0 block in
 * bk_hpdma_link_transfer).
 */
typedef enum {
	HPDMA_BURST_LEN_SINGLE = 0,
	HPDMA_BURST_LEN_INC4,
	HPDMA_BURST_LEN_INC8,
	HPDMA_BURST_LEN_INC16,
} hpdma_burst_len_t;

typedef enum {
    HPDMA_TRANS_DEFAULT = 0,         /**< DMA TRANS DEFAULT */
    HPDMA_TRANS_YUV_TO_RGB,          /**< DMA TRANS YUV_TO_RGB */
    HPDMA_TRANS_MAX,
} hpdma_pixel_trans_type_t;

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
    hpdma_dev_t dev;                /**< DMA device */
    hpdma_data_width_t width;       /**< DMA data width */
    hpdma_addr_inc_t addr_inc_en;   /**< enable/disable DMA address increase */
    hpdma_addr_loop_t addr_loop_en; /**< enable/disable DMA address loop */
    uint32_t start_addr;          /**< DMA start address */
    uint16_t xsize;                /**< X-direction length (Reg20: src_xsize[15:0] or dest_xsize[31:16]) (number of bytes per row)*/
    uint16_t ysize;                /**< Y-direction length (Reg22: src_ysize[15:0] or dest_ysize[31:16]) (number of rows, 1 means 1 row)*/ 
    uint16_t step;                  /**< Address step between adjacent data (Reg29: src_step[14:0] or dest_step[29:15]) */
} hpdma_port_config_t;

typedef struct {
    hpdma_work_mode_t mode;          /**< DMA work mode */
    hpdma_chan_priority_t chan_prio; /**< DMA channel prioprity */
    hpdma_port_config_t src;         /**< DMA source configuration */
    hpdma_port_config_t dst;         /**< DMA dest configuration */
    hpdma_pixel_trans_type_t trans_type; /**< DMA TRANS TYPE */
} hpdma_config_t;

// New descriptor format (24 bytes, 6 words)
typedef volatile struct {
    uint32_t src_addr;           // DES0: Source Address[31:0]
    uint32_t dst_addr;           // DES1: Destination Address[31:0]
    
    union {
        struct {
            // 注意：位域顺序从左到右对应高位到低位（高位在前）
            // 表格顺序：int_half_finish_en | int_finish_en | dest_step[14:0] | source_step[14:0]
            uint32_t source_step : 15;      // Bit 14-0: Source step (低位，表格最右边)
            uint32_t dest_step : 15;       // Bit 29-15: Destination step
            uint32_t int_finish_en : 1;    // Bit 30: Finish interrupt enable
            uint32_t int_half_finish_en : 1; // Bit 31: Half finish interrupt enable (高位，表格最左边)
        } bits;
        uint32_t control;                  // DES2: Control word
    } ctrl;
    
    union {
        struct {
            // 注意：位域顺序从左到右对应高位到低位（高位在前）
            // 表格顺序：dest_xsize[15:0] | source_xsize[15:0]
            uint32_t source_xsize : 16;    // Bit 15-0: Source X dimension (低位，表格右边)
            uint32_t dest_xsize : 16;      // Bit 31-16: Destination X dimension (高位，表格左边)
        } bits;
        uint32_t xsize;                    // DES3: X dimension
    } xdim;
    
    union {
        struct {
            // 注意：位域顺序从左到右对应高位到低位（高位在前）
            // 表格顺序：dest_ysize[15:0] | source_ysize[15:0]
            uint32_t source_ysize : 16;    // Bit 15-0: Source Y dimension (低位，表格右边)
            uint32_t dest_ysize : 16;      // Bit 31-16: Destination Y dimension (高位，表格左边)
        } bits;
        uint32_t ydim;                     // DES4: Y dimension
    } ydim;
    
    uint32_t next_desc_addr;               // DES5: Next descriptor address
} __attribute__((aligned(16))) hpdma_descriptor_t;


typedef struct {
    uint32_t src_addr;
    uint32_t dst_addr;
    uint16_t src_xsize;           // Source X dimension (bytes per row)
    uint16_t src_ysize;           // Source Y dimension (number of rows, 1 means 1 row)
    uint16_t dst_xsize;           // Destination X dimension (bytes per row)
    uint16_t dst_ysize;           // Destination Y dimension (number of rows, 1 means 1 row)
    uint16_t src_step;            // Source step (row stride)
    uint16_t dst_step;            // Destination step (row stride)
    uint8_t finish_int_en;         // Finish interrupt enable
    uint8_t half_finish_int_en;    // Half finish interrupt enable
} hpdma_link_config_t;


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

