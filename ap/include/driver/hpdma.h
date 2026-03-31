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
//

#pragma once

#include <common/bk_include.h>
#include <driver/hpdma_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Init the DMA driver
 *
 * This API init the resoure common to all dma channels:
 *   - Init dma driver control memory
 *
 * This API should be called before any other dma APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_driver_init(void);

/**
 * @brief     Deinit the DMA driver
 *
 * This API free all resource related to dma and power down all dma channels.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_driver_deinit(void);

/**
 * @brief     Allocate a DMA channel
 *
 * @attention: This API can only be called in task context, 
 *             - and can't be called in context that interrupt is disabled.
 *
 * This API should be called before any other dma channel APIs.
 *
 * @param user_id DMA channel applicant
 *
 * @return DMA channel id.
 *     -  > DMA_ID_MAX:  no free DMA channel.
 */
hpdma_id_t bk_hpdma_alloc(u16 user_id);

/**
 * @brief     Allocate a fixed DMA channel.Maybe some APP needs to have a
 *            fixed channel forever, then it can use this API.
 *
 * @attention: This API is just for CONFIG_GDMA_HW_V2PX
 *             And CONFIG_GDMA_HW_V1PX doesn't support it.
 *
 * This API should be called before any other dma channel APIs.
 *
 * @param user_id DMA channel applicant
 * @param fixed_chnl_id application wants to get which fixed channel
 *
 * @return DMA channel id.
 *     -  > DMA_ID_MAX:  if the fixed_chnl_id has been allocated, it will
 *                       return DMA_ID_MAX which means error.
 *     -  > fixed_chnl_id: the fixed_chnl_id is allocated suc.
 */
hpdma_id_t bk_fixed_hpdma_alloc(u16 user_id, hpdma_id_t fixed_chnl_id);

/**
 * @brief     Free the DMA channel
 *
 * @attention: This API can only be called in task context, 
 *             - and can't be called in context that interrupt is disabled.
 *
 * @param user_id DMA channel applicant, the same as in bk_hpdma_alloc.
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_free(u16 user_id, hpdma_id_t id);

/**
 * @brief     get the user of DMA channel
 *
 * @param id DMA channel
 *
 * @return DMA channel user_id.
 *     -  u32:  high u16 is the CPU_ID, low 16 bits is the applicant_id.
 */
uint32_t bk_hpdma_user(hpdma_id_t id);

/**
 * @brief     Init the DMA channel
 *
 * @attention 1. the higher channel priority value, the higher the priority
 *
 * @param id DMA channel
 * @param config DMA configuration
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_DMA_NOT_INIT: DMA driver not init
 *    - BK_ERR_NULL_PARAM: config is NULL
 *    - BK_ERR_DMA_ID: invalid DMA channel
 *    - BK_ERR_DMA_INVALID_ADDR: invalid DMA address
 *    - others: other errors.
 */
bk_err_t bk_hpdma_init(hpdma_id_t id, const hpdma_config_t *config);

/**
 * @brief     Deinit a DMA channel
 *
 * This API deinit the DMA channel:
 *   - Stop the DMA channel
 *   - Reset all configuration of DMA channel to default value
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_deinit(hpdma_id_t id);

/**
 * @brief     Start a DMA channel
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_start(hpdma_id_t id);

/**
 * @brief     Stop a DMA channel
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_stop(hpdma_id_t id);


/**
 * @brief     Enable DMA finish intterrup
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 *
 * @NOTES before enable interrupt, please confirm have called bk_hpdma_register_isr.
 */
bk_err_t bk_hpdma_enable_finish_interrupt(hpdma_id_t id);

/**
 * @brief     Disable DMA finish intterrup
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_disable_finish_interrupt(hpdma_id_t id);

// /**
//  * @brief     Get finish interrupt enable status for a specific DMA channel
//  *
//  * @param id DMA channel ID (0-7)
//  *
//  * @return
//  *    - 1: Finish interrupt is enabled
//  *    - 0: Finish interrupt is disabled or channel is invalid/not initialized
//  */
// uint32_t bk_hpdma_get_finish_int_en(hpdma_id_t id);

/**
 * @brief     Enable DMA half finish intterrup
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 *
 * @NOTES before enable interrupt, please confirm have called bk_hpdma_register_isr.
 */
bk_err_t bk_hpdma_enable_half_finish_interrupt(hpdma_id_t id);

/**
 * @brief     Disable DMA half finish intterrup
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_disable_half_finish_interrupt(hpdma_id_t id);

/**
 * @brief     Enable DMA bus error interrupt
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_enable_bus_err_interrupt(hpdma_id_t id);

/**
 * @brief     Disable DMA bus error interrupt
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_disable_bus_err_interrupt(hpdma_id_t id);

/**
 * @brief     Register the interrupt service routine for DMA channel
 *
 * This API regist dma isr callback function with user data parameter.
 *
 * @param id DMA channel
 * @param half_finish_isr DMA half finish callback
 * @param half_finish_data User data for half finish callback (can be NULL)
 * @param finish_isr DMA finish callback
 * @param finish_data User data for finish callback (can be NULL)
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_register_isr(hpdma_id_t id, hpdma_isr_t half_finish_isr, void *half_finish_data,
                                hpdma_isr_t finish_isr, void *finish_data);


/**
 * @brief     Register the interrupt service routine for DMA channel
 *
 * This API regist dma isr callback function with user data parameter.
 *
 * @param id DMA channel
 * @param bus_err_isr DMA bus error callback
 * @param user_data User data for bus error callback (can be NULL)
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_register_bus_err_isr(hpdma_id_t id, hpdma_isr_t bus_err_isr, void *user_data);


/**
 * @brief     Set DMA source start address
 *
 * @param id DMA channel
 * @param start_addr DMA source start address
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_set_src_start_addr(hpdma_id_t id, uint32_t start_addr);


/**
 * @brief     Set DMA dest start address
 *
 * @param id DMA channel
 * @param start_addr DMA dest start address
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_set_dest_start_addr(hpdma_id_t id, uint32_t start_addr);

/**
 * @brief     Enable DMA source address increase
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_enable_src_addr_increase(hpdma_id_t id);

/**
 * @brief     Disable DMA source address increase
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_disable_src_addr_increase(hpdma_id_t id);

/**
 * @brief     Enable DMA source address loop
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_enable_src_addr_loop(hpdma_id_t id);

/**
 * @brief     Disable DMA source address loop
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_disable_src_addr_loop(hpdma_id_t id);

/**
 * @brief     Enable DMA dest address increase
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_enable_dest_addr_increase(hpdma_id_t id);

/**
 * @brief     Disable DMA dest address increase
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_disable_dest_addr_increase(hpdma_id_t id);

/**
 * @brief     Enable DMA dest address loop
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_enable_dest_addr_loop(hpdma_id_t id);

/**
 * @brief     Disable DMA dest address loop
 *
 * @param id DMA channel
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_disable_dest_addr_loop(hpdma_id_t id);

/**
 * @brief     Get DMA transfer remain length
 *
 * @param id DMA channel
 *
 * @return DMA transfer remain length
 */
uint32_t bk_hpdma_get_remain_len(hpdma_id_t id);

/**
 * @brief     Gets the current DMA channel working status
 *
 * @param id DMA channel
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
uint32_t bk_hpdma_get_enable_status(hpdma_id_t id);

/**
 * @brief    flush reserved data in dma internal buffer
 *           I.E:If source data width is not 4bytes, and data size isn't 4bytes align,
 *           but dest data width is 4bytes, then maybe 1~3 bytes data reserved in
 *           dma internal buffer, then the left 1~3 bytes data not copy to dest address.
 *
 * @param id DMA channel
 * @param attr DMA privileged attr
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
bk_err_t bk_hpdma_flush_src_buffer(hpdma_id_t id);

#ifdef CONFIG_SPE
/**
 * @brief    To configure the beat length,
 *           DMA actually applies for the bus one at a time and divides the total
 *           amount of data to be transmitted into small data blocks. For example, if
 *           you want to transfer 64 bytes, then dma may be divided into 2 times internally.
 *           64/2=32 bytes are transferred at one time.This 2(a) times is called burst.
 *
 * @param id DMA channel
 * @param len DMA dest burst len
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_set_dest_burst_len(hpdma_id_t id, hpdma_burst_len_t len);

/**
 * @brief    Get the configured length of burst
 *
 * @param id DMA channel
 *
 * @return DMA dest burst length
 */
uint32_t bk_hpdma_get_dest_burst_len(hpdma_id_t id);

/**
 * @brief    To configure the beat length,
 *           DMA actually applies for the bus one at a time and divides the total
 *           amount of data to be transmitted into small data blocks. For example, if
 *           you want to transfer 64 bytes, then dma may be divided into 2 times internally.
 *           64/2=32 bytes are transferred at one time.This 2(a) times is called burst.
 *
 * @param id DMA channel
 * @param len DMA dest burst len
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_set_src_burst_len(hpdma_id_t id, hpdma_burst_len_t len);

/**
 * @brief    Get the configured length of burst
 *
 * @param id DMA channel
 *
 * @return DMA dest burst length
 */
uint32_t bk_hpdma_get_src_burst_len(hpdma_id_t id);

/**
 * @brief    Select the conversion mode that needs to be configured for DMA conversion of
 *           video formats during data transfer.
 *
 * @param id DMA channel
 * @param type DMA Select conversion mode
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_set_pixel_trans_type(hpdma_id_t id, hpdma_pixel_trans_type_t type);

/**
 * @brief    Get the conversion mode
 *
 * @param id DMA channel
 *
 * @return DMA conversion mode
 */
uint32_t bk_hpdma_get_pixel_trans_type(hpdma_id_t id);


/**
 * @brief     Enable the current DMA channel bus err interrupt
 *
 * @param id DMA channel
 *
 * @return
 *    - 0: enable success state
 *    - others: Channel busy state.
 */
bk_err_t bk_hpdma_bus_err_int_enable(hpdma_id_t id);

/**
 * @brief     Disable the current DMA channel bus err interrupt
 *
 * @param id DMA channel
 *
 * @return
 *    - 0: disable success state
 *    - others: Channel busy state.
 */
bk_err_t bk_hpdma_bus_err_int_disable(hpdma_id_t id);

/**
 * @brief     Set the current DMA channel dest secure attr
 *
 * @param id DMA channel
 * @param attr DMA secure attr
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
bk_err_t bk_hpdma_set_dest_sec_attr(hpdma_id_t id, hpdma_sec_attr_t attr);

/**
 * @brief     Set the current DMA channel src secure attr
 *
 * @param id DMA channel
 * @param attr DMA secure attr
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
bk_err_t bk_hpdma_set_src_sec_attr(hpdma_id_t id, hpdma_sec_attr_t attr);
#endif

#if (CONFIG_SPE)
/**
 * @brief     Set the all DMA channel secure attr
 *
 * @param id DMA channel
 * @param attr DMA secure attr
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
bk_err_t bk_hpdma_set_sec_attr(hpdma_id_t id, hpdma_sec_attr_t attr);

/**
 * @brief     Set the all DMA channel privileded attr
 *
 * @param id DMA channel
 * @param attr DMA privileged attr
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
bk_err_t bk_hpdma_set_privileged_attr(hpdma_id_t id, hpdma_sec_attr_t attr);

/**
 * @brief     Configure the same DMA channel to trigger the interrupt state on a fixed core
 *
 * @param id DMA channel ID (0-7)
 * @param int_id DMA interrupt ID (0-4, maps to int0-int4)
 *
 * @return
 *    - BK_OK: Success
 *    - BK_ERR_HPDMA_NOT_INIT: HPDMA driver not initialized
 *    - BK_ERR_HPDMA_ID: Invalid channel ID
 * 
 * @note This function allocates interrupt from channel 'id' to interrupt 'int_id'.
 *       Configuration must be done in secure world.
 */
bk_err_t bk_hpdma_set_int_allocate(hpdma_id_t id, hpdma_int_id_t int_id);

/**
 * @brief     Get the interrupt allocation for a specific DMA channel
 *
 * @param id DMA channel ID (0-7)
 *
 * @return
 *    - Interrupt ID (0-4, maps to int0-int4) allocated to the channel
 *    - 0: Default value if channel is not configured
 */
hpdma_int_id_t bk_hpdma_get_int_allocate(hpdma_id_t id);

/**
 * @brief     Get repeat write pause count
 *
 * @param id DMA channel
 *
 * @return Repeat write pause count
 */
uint32_t bk_hpdma_get_repeat_wr_pause(hpdma_id_t id);

/**
 * @brief     Get repeat read pause count
 *
 * @param id DMA channel
 *
 * @return Repeat read pause count
 */
uint32_t bk_hpdma_get_repeat_rd_pause(hpdma_id_t id);

/**
 * @brief     Get finish interrupt count
 *
 * @param id DMA channel
 *
 * @return Finish interrupt count
 */
uint32_t bk_hpdma_get_finish_interrupt_cnt(hpdma_id_t id);

/**
 * @brief     Get half finish interrupt count
 *
 * @param id DMA channel
 *
 * @return Half finish interrupt count
 */
uint32_t bk_hpdma_get_half_finish_interrupt_cnt(hpdma_id_t id);

#endif

/**
 * @brief     Set next linked list address
 *
 * @param id DMA channel
 * @param ll_addr Linked list address
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hpdma_set_next_ll_addr(hpdma_id_t id, uint32_t ll_addr);

/**
 * @brief     Get next linked list address
 *
 * @param id DMA channel
 *
 * @return Next linked list address
 */
uint32_t bk_hpdma_get_next_ll_addr(hpdma_id_t id);

/**
 * @brief     Initialize linked list descriptor table
 *
 * This API allocates and initializes a descriptor table for linked list DMA transfers.
 *
 * @param link_cnt Number of descriptors in the linked list
 *
 * @return
 *    - Pointer to the first descriptor (16-byte aligned) on success
 *    - NULL on failure (memory allocation failed)
 */
void *bk_hpdma_link_init(uint32_t link_cnt);

/**
 * @brief     Deinitialize linked list descriptor table
 *
 * This API frees the memory allocated for the descriptor table.
 *
 * @param desc_table Descriptor table pointer returned by bk_hpdma_link_init
 */
void bk_hpdma_link_deinit(void *desc_table);

/**
 * @brief     Set a single descriptor in the linked list
 *
 * This API configures a descriptor at the specified index for DMA transfer.
 * Supports both 1D and 2D transfers.
 *
 * @param desc_table Descriptor table pointer
 * @param index Descriptor index
 * @param config Transfer configuration (for 1D: set ysize=0, step=0)
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: invalid parameters
 *    - BK_ERR_PARAM: address not 128-bit aligned
 */
bk_err_t bk_hpdma_link_set_desc(void *desc_table, uint32_t index, const hpdma_link_config_t *config);

/**
 * @brief     Set multiple descriptors in the linked list
 *
 * This API configures multiple descriptors for DMA transfer.
 * Supports both 1D and 2D transfers.
 *
 * @param desc_table Descriptor table pointer
 * @param configs Configuration array (for 1D: set ysize=0, step=0)
 * @param link_cnt Number of descriptors to configure
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: invalid parameters
 *    - others: other errors
 */
bk_err_t bk_hpdma_link_set_descs(void *desc_table, const hpdma_link_config_t *configs, uint32_t link_cnt);

/**
 * @brief     Execute linked list transfer (Asynchronous)
 *
 * This API configures the DMA channel and starts the transfer.
 * The DMA channel ID must be allocated by the application layer using bk_hpdma_alloc().
 * The function returns after the transfer is started (not necessarily completed).
 *
 * @param id DMA channel ID (allocated by application layer using bk_hpdma_alloc())
 * @param desc_table Descriptor table pointer
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: invalid descriptor table
 *    - BK_ERR_HPDMA_ID: invalid DMA channel ID
 *    - others: other errors
 */
bk_err_t bk_hpdma_link_transfer(hpdma_id_t id, void *desc_table);

/**
 * @brief     Memory copy using HPDMA
 *
 * This API performs memory copy operation using HPDMA.
 * It automatically allocates a DMA channel, performs the transfer, and frees the channel.
 *
 * @param out Destination address
 * @param in Source address
 * @param len Number of bytes to copy
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_hpdma_memcpy(void *out, const void *in, uint32_t len);


#ifdef __cplusplus
}
#endif

