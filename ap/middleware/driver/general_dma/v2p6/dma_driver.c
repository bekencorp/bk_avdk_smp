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

#include <common/bk_include.h>
#include <common/bk_compiler.h>
#include <os/mem.h>
#include "icu_driver.h"
#include "dma_hal.h"
#include <driver/dma.h>
#include "dma_driver.h"
// #include "bk_high_performance_dma.h"
#include <driver/int.h>
#include "sys_driver.h"
#include "cpu_id.h"
#include "interrupt_base.h"
#include "arch_interrupt.h"
#include "interrupt.h"

#if CONFIG_SUPPORT_CACHEABLE_SRAM
#include "cache.h"
#endif

#include "bk_misc.h"

#include <os/mem.h>

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static volatile spinlock_t dma_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP

static inline uint32_t dma_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
	spin_lock(&dma_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	return flags;
}

static inline void dma_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
	spin_unlock(&dma_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	rtos_enable_int(flags);
}

static void dma_isr(void) __BK_SECTION(".itcm");	//dma-hw-0

typedef struct {
    dma_hal_t hal;
    uint32_t id_init_bits;
} dma_driver_t;

static dma_driver_t s_dma;
static dma_isr_t s_dma_finish_isr[SOC_DMA_CHAN_NUM_PER_UNIT] = {NULL};
static dma_isr_t s_dma_half_finish_isr[SOC_DMA_CHAN_NUM_PER_UNIT] = {NULL};
static dma_isr_t s_dma_bus_err_isr[SOC_DMA_CHAN_NUM_PER_UNIT] = {NULL};

static bool s_dma_driver_is_init = false;
static dma_chnl_pool_t s_dma_chnl_pool = {0};

#define DMA_RETURN_ON_NOT_INIT() do {\
        if (!s_dma_driver_is_init) {\
            return BK_ERR_DMA_NOT_INIT;\
        }\
    } while(0)

#define DMA_RETURN_ON_INVALID_ID(channel) do {\
        if (((channel) < CONFIG_DMA_LOGIC_CHAN_ID_MIN) || ((channel) >= CONFIG_DMA_LOGIC_CHAN_ID_MIN + CONFIG_DMA_LOGIC_CHAN_CNT)) {\
            return BK_ERR_DMA_ID;\
        }\
    } while(0)

#define DMA_RETURN_ON_ID_NOT_INIT(dma_num,id) do {\
        if (!(s_dma.id_init_bits & BIT((id)))) {\
            return BK_ERR_DMA_ID_NOT_INIT;\
        }\
    } while(0)

#define DMA_LOG_ON_ID_IS_STARTED(dma_num,channel) do {\
        if (dma_hal_is_id_started(&s_dma.hal, (channel))) {\
        }\
    } while(0)

#define DMA_RETURN_ON_INVALID_ADDR(start_addr, end_addr) do {\
        if ((0 < (end_addr)) && ((end_addr) < (start_addr))) {\
            return BK_ERR_DMA_INVALID_ADDR;\
        }\
    } while(0)

static void dma_id_init_common(dma_id_t id)
{
    dma_hal_set_cachable(&s_dma.hal, id, 1);
    s_dma.id_init_bits |= BIT(id);
}

static void dma_id_deinit_common(dma_id_t id)
{
    s_dma.id_init_bits &= ~BIT(id);
    dma_hal_stop_common(&s_dma.hal, id);
    dma_hal_set_cachable(&s_dma.hal, id, 0);
    dma_hal_reset_config_to_default(&s_dma.hal, id);
}

static void dma_id_enable_interrupt_common(dma_id_t id)
{

    dma_hal_set_int_allocate(&s_dma.hal, id, DMA_INT_2);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GDMA0, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GDMA0, 1);
#endif
}

/* used internally, called in context of interrupt disabled. */
u8 dma_chnl_alloc(u32 user_id)
{
	u8 chnl_id;

    for(chnl_id = CONFIG_DMA_LOGIC_CHAN_ID_MIN; chnl_id < CONFIG_DMA_LOGIC_CHAN_ID_MIN + CONFIG_DMA_LOGIC_CHAN_CNT; chnl_id++)
	{
		if((s_dma_chnl_pool.chnl_bitmap & (0x01 << chnl_id)) == 0)
		{
			s_dma_chnl_pool.chnl_bitmap |= (0x01 << chnl_id);
			s_dma_chnl_pool.chnl_user[chnl_id] = user_id;
			return chnl_id;
		}
	}

	//alloc failed
	DMA_LOGE("%s:chnl_id=%d\r\n", __func__, chnl_id);
	return DMA_ID_MAX;
}

dma_id_t dma_fixed_chnl_alloc(u32 user_id, dma_id_t fixed_chnl_id)
{

    if((fixed_chnl_id >= CONFIG_DMA_LOGIC_CHAN_ID_MIN) && (fixed_chnl_id < CONFIG_DMA_LOGIC_CHAN_ID_MIN + CONFIG_DMA_LOGIC_CHAN_CNT))
	{
		if((s_dma_chnl_pool.chnl_bitmap & (0x01 << fixed_chnl_id)) == 0)
		{
			s_dma_chnl_pool.chnl_bitmap |= (0x01 << fixed_chnl_id);
			s_dma_chnl_pool.chnl_user[fixed_chnl_id] = user_id;
			return fixed_chnl_id;
		}
	}

	DMA_LOGE("chan=%d has been allocated\r\n", fixed_chnl_id);
	return DMA_ID_MAX;
}

/* used internally, called in context of interrupt disabled. */
bk_err_t dma_chnl_free(u32 user_id, dma_id_t chnl_id)
{
	if( chnl_id >= DMA_ID_MAX )
		return BK_ERR_DMA_ID;

	if( s_dma_chnl_pool.chnl_user[chnl_id] != user_id )
		return BK_ERR_PARAM;

	s_dma_chnl_pool.chnl_bitmap &= ~(0x01 << chnl_id);
	s_dma_chnl_pool.chnl_user[chnl_id] = -1;

	return BK_OK;
}

/* used internally. */
u32 dma_chnl_user(dma_id_t chnl_id)
{
	if( chnl_id >= DMA_ID_MAX )
		return -1;

	return s_dma_chnl_pool.chnl_user[chnl_id];
}

bk_err_t bk_dma_driver_init(void)
{
    if (s_dma_driver_is_init) {
        return BK_OK;
    }

	s_dma_chnl_pool.chnl_bitmap = 0;
	uint32_t chnl_id_min = CONFIG_DMA_LOGIC_CHAN_ID_MIN;
	uint32_t chnl_id_max = chnl_id_min + CONFIG_DMA_LOGIC_CHAN_CNT;

	if (chnl_id_max > SOC_DMA_CHAN_NUM_PER_UNIT) {
		chnl_id_max = SOC_DMA_CHAN_NUM_PER_UNIT;
	}

	for(uint32_t i = chnl_id_min; i < chnl_id_max; i++)
	{
		s_dma_chnl_pool.chnl_user[i] = -1;
	}

    // workaround: must uncomment it after mailbox problem fixed
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA0, PM_POWER_MODULE_STATE_ON);
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA1, PM_POWER_MODULE_STATE_ON);

    /* 1)intc_service_register
     * 2)init dma_finish_int handler, dma_half_finish_int handler
     * 3)disable dma_en (0~6), clear int status
     * 4)init dma_config
     */
    os_memset(&s_dma, 0, sizeof(s_dma));
    os_memset(&s_dma_finish_isr, 0, sizeof(s_dma_finish_isr));
    os_memset(&s_dma_half_finish_isr, 0, sizeof(s_dma_half_finish_isr));
    os_memset(&s_dma_bus_err_isr, 0, sizeof(s_dma_bus_err_isr));

	//TODO:if the channels in the selected DMA, it should register ISR, else no need to register it.

	bk_int_isr_register(INT_SRC_GDMA0, dma_isr, NULL);

    for (uint32_t uint_id = 0; uint_id < SOC_DMA_UNIT_NUM; uint_id++) {
	    s_dma.hal.id = uint_id;
		dma_hal_init(&s_dma.hal);
	}

    s_dma_driver_is_init = true;

    return BK_OK;
}

bk_err_t bk_dma_driver_deinit(void)
{
    if (!s_dma_driver_is_init) {
        return BK_OK;
    }

    for (int id = 0; id < (SOC_DMA_CHAN_NUM_PER_UNIT*SOC_DMA_UNIT_NUM); id++) {
        dma_id_deinit_common(id);
    }

    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA0, PM_POWER_MODULE_STATE_OFF);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA1, PM_POWER_MODULE_STATE_OFF);

#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GDMA0, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GDMA0, 0);
#endif

    s_dma_driver_is_init = false;

    return BK_OK;
}

dma_id_t bk_dma_alloc(u16 user_id)
{
    if (!s_dma_driver_is_init)
    {
        return DMA_ID_MAX;
    }

    u32 int_mask = dma_enter_critical();

    u8 chnl_id = dma_chnl_alloc(user_id);

    dma_exit_critical(int_mask);

    return chnl_id;
}

dma_id_t bk_fixed_dma_alloc(u16 user_id, dma_id_t chnl_id)
{
    if (!s_dma_driver_is_init)
    {
        return DMA_ID_MAX;
    }

    u32 int_mask = dma_enter_critical();

    u8 chnl_id_ret = dma_fixed_chnl_alloc(user_id, chnl_id);

    dma_exit_critical(int_mask);

    return chnl_id_ret;
}

bk_err_t bk_dma_free(u16 user_id, dma_id_t chnl_id)
{
    if (!s_dma_driver_is_init)
    {
        return BK_ERR_DMA_NOT_INIT;
    }

    u32  int_mask = dma_enter_critical();

    bk_err_t ret_val = dma_chnl_free(user_id, chnl_id);

    dma_exit_critical(int_mask);

    return ret_val;
}

uint32_t bk_dma_user(dma_id_t chnl_id)
{
    if (!s_dma_driver_is_init)
    {
        return -1;
    }

    return dma_chnl_user(chnl_id);
}

bk_err_t bk_dma_init(dma_id_t id, const dma_config_t *config)
{
    dma_hal_init_without_channels(&s_dma.hal);	//TODO:special codes for DMA init after enter low voltage

#if CONFIG_SUPPORT_CACHEABLE_SRAM
    flush_dcache((void *)config->src.start_addr, config->src.end_addr - config->src.start_addr);
    flush_dcache((void *)config->dst.start_addr, config->dst.end_addr - config->dst.start_addr);
#endif

    dma_id_init_common(id);
    return dma_hal_init_dma(&s_dma.hal, id, config);
}

bk_err_t bk_dma_deinit(dma_id_t id)
{
    DMA_RETURN_ON_INVALID_ID(id);
    dma_id_deinit_common(id);
    bk_dma_register_isr(id, NULL, NULL);
    return BK_OK;
}

bk_err_t bk_dma_start(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    dma_hal_start_common(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_stop(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();

    dma_hal_stop_common(&s_dma.hal, id);
    return BK_OK;
}

uint32_t bk_dma_get_enable_status(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();

    uint32_t ret;
    ret = dma_hal_get_enable_status(&s_dma.hal, id);
    return ret;
}

#define DMA_MAX_BUSY_TIME (10000)  //us
uint32_t dma_wait_to_idle(dma_id_t id)
{
	if(dma_hal_get_work_mode(&s_dma.hal, id) == DMA_WORK_MODE_SINGLE)
	{
		uint32_t i = 0;
		while(dma_hal_get_enable_status(&s_dma.hal, id))
		{
			bk_delay_us(1);

			i++;
			if(i > DMA_MAX_BUSY_TIME)
			{
				DMA_LOGE("ch%d busy,remain len=%d,dst_addr=%x\r\n", id,
							dma_hal_get_remain_len(&s_dma.hal, id),
							dma_hal_get_dest_write_addr(&s_dma.hal, id));
				break;
			}
		}

		return i;
	}
	else
	{
		//TODO:
	}

	return 0;
}

/* DTCM->peripheral
 */
bk_err_t bk_dma_write(dma_id_t id, const uint8_t *data, uint32_t size)
{
    DMA_RETURN_ON_NOT_INIT();
	dma_wait_to_idle(id);

    dma_hal_set_src_start_addr(&s_dma.hal, id, (uintptr_t)data);
    dma_hal_set_src_loop_addr(&s_dma.hal, id, (uintptr_t)data, (uintptr_t)(data + size));
    dma_hal_set_transfer_len(&s_dma.hal, id, size);
    dma_hal_start_common(&s_dma.hal, id);

    return BK_OK;
}

/* peripheral->DTCM
 */
bk_err_t bk_dma_read(dma_id_t id, uint8_t *data, uint32_t size)
{
    DMA_RETURN_ON_NOT_INIT();
	dma_wait_to_idle(id);

    dma_hal_set_dest_start_addr(&s_dma.hal, id, (uintptr_t)data);
    dma_hal_set_dest_loop_addr(&s_dma.hal, id, (uintptr_t)data, (uintptr_t)(data + size));
    dma_hal_set_transfer_len(&s_dma.hal, id, size);
    dma_hal_start_common(&s_dma.hal, id);

    return BK_OK;
}

bk_err_t bk_dma_enable_finish_interrupt(dma_id_t id)
{
    DMA_RETURN_ON_INVALID_ID(id);
    dma_id_enable_interrupt_common(id);
    dma_hal_enable_finish_interrupt(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_disable_finish_interrupt(dma_id_t id)
{
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_disable_finish_interrupt(&s_dma.hal, id);
    dma_hal_clear_finish_interrupt_status(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_enable_half_finish_interrupt(dma_id_t id)
{
    DMA_RETURN_ON_INVALID_ID(id);
    dma_id_enable_interrupt_common(id);
    dma_hal_enable_half_finish_interrupt(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_disable_half_finish_interrupt(dma_id_t id)
{
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_disable_half_finish_interrupt(&s_dma.hal, id);
    dma_hal_clear_half_finish_interrupt_status(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_enable_bus_err_interrupt(dma_id_t id)
{
    DMA_RETURN_ON_INVALID_ID(id);
    dma_id_enable_interrupt_common(id);
    dma_hal_enable_bus_err_interrupt(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_disable_bus_err_interrupt(dma_id_t id)
{
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_disable_bus_err_interrupt(&s_dma.hal, id);
    dma_hal_clear_bus_err_interrupt_status(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_register_isr(dma_id_t id, dma_isr_t half_finish_isr, dma_isr_t finish_isr)
{
    DMA_RETURN_ON_NOT_INIT();

    DMA_RETURN_ON_INVALID_ID(id);
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    s_dma_half_finish_isr[id] = half_finish_isr;
    s_dma_finish_isr[id] = finish_isr;
    GLOBAL_INT_RESTORE();

    return BK_OK;
}

bk_err_t bk_dma_register_bus_err_isr(dma_id_t id, dma_isr_t bus_err_isr)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    s_dma_bus_err_isr[id] = bus_err_isr;
    GLOBAL_INT_RESTORE();

    return BK_OK;
}

uint32_t bk_dma_get_transfer_len_max(dma_id_t id)
{
    return dma_hal_get_transfer_len_max(&s_dma.hal);
}

bk_err_t bk_dma_set_transfer_len(dma_id_t id, uint32_t tran_len)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    if(tran_len > dma_hal_get_transfer_len_max(&s_dma.hal))
    {
        DMA_LOGE("%s:dma_id=%d,len=%d over 1GB\r\n",__func__, id, tran_len);
        return BK_ERR_DMA_TRANS_LEN;
    }

#if !CONFIG_SOC_BK7259
    dma_wait_to_idle(id);
#endif
    dma_hal_set_transfer_len(&s_dma.hal, id, tran_len);
    return BK_OK;
}

bk_err_t bk_dma_set_src_addr(dma_id_t id, uint32_t start_addr, uint32_t end_addr)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_set_src_start_addr(&s_dma.hal, id, start_addr);
    dma_hal_set_src_loop_addr(&s_dma.hal, id, start_addr, end_addr);
    return BK_OK;
}

bk_err_t bk_dma_set_src_start_addr(dma_id_t id, uint32_t start_addr)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_set_src_start_addr(&s_dma.hal, id, start_addr);
    return BK_OK;
}

bk_err_t bk_dma_set_dest_addr(dma_id_t id, uint32_t start_addr, uint32_t end_addr)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_wait_to_idle(id);
    dma_hal_set_dest_start_addr(&s_dma.hal, id, start_addr);
    dma_hal_set_dest_loop_addr(&s_dma.hal, id, start_addr, end_addr);
    return BK_OK;
}

bk_err_t bk_dma_set_dest_start_addr(dma_id_t id, uint32_t start_addr)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
#if !CONFIG_SOC_BK7259
    dma_wait_to_idle(id);
#endif
    dma_hal_set_dest_start_addr(&s_dma.hal, id, start_addr);
    return BK_OK;
}

bk_err_t bk_dma_enable_src_addr_increase(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_enable_src_addr_inc(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_disable_src_addr_increase(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_disable_src_addr_inc(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_enable_src_addr_loop(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_enable_src_addr_loop(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_disable_src_addr_loop(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_disable_src_addr_loop(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_enable_dest_addr_increase(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_enable_dest_addr_inc(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_disable_dest_addr_increase(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_disable_dest_addr_inc(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_enable_dest_addr_loop(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_enable_dest_addr_loop(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_disable_dest_addr_loop(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_disable_dest_addr_loop(&s_dma.hal, id);
    return BK_OK;
}

uint32_t bk_dma_get_remain_len(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    return dma_hal_get_remain_len(&s_dma.hal, id);
}

bk_err_t dma_set_src_pause_addr(dma_id_t id, uint32_t addr)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_set_src_pause_addr(&s_dma.hal, id, addr);

    return BK_OK;
}

bk_err_t dma_set_dst_pause_addr(dma_id_t id, uint32_t addr)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_set_dest_pause_addr(&s_dma.hal, id, addr);

    return BK_OK;
}

uint32_t dma_get_src_read_addr(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    return dma_hal_get_src_read_addr(&s_dma.hal, id);
}

uint32_t dma_get_dest_write_addr(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    return dma_hal_get_dest_write_addr(&s_dma.hal, id);
}

bk_err_t bk_dma_set_src_data_width(dma_id_t id, dma_data_width_t data_width)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_wait_to_idle(id);
    dma_hal_set_src_data_width(&s_dma.hal, id, data_width);
    return BK_OK;
}

bk_err_t bk_dma_set_dest_data_width(dma_id_t id, dma_data_width_t data_width)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_wait_to_idle(id);
    dma_hal_set_dest_data_width(&s_dma.hal, id, data_width);
    return BK_OK;
}

bk_err_t bk_dma_flush_src_buffer(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_flush_src_buffer(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_set_pixel_trans_type(dma_id_t id, dma_pixel_trans_type_t type)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);


    dma_hal_set_pixel_trans_type(&s_dma.hal, id, type);
    return BK_OK;
}

uint32_t bk_dma_get_pixel_trans_type(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);


    return dma_hal_get_pixel_trans_type(&s_dma.hal, id);
}

bk_err_t bk_dma_bus_err_int_enable(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_bus_err_int_enable(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_bus_err_int_diable(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_bus_err_int_disable(&s_dma.hal, id);
    return BK_OK;
}

bk_err_t bk_dma_set_dest_sec_attr(dma_id_t id, dma_sec_attr_t attr)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_set_dest_sec_attr(&s_dma.hal, id, attr);
    return BK_OK;
}

bk_err_t bk_dma_set_src_sec_attr(dma_id_t id, dma_sec_attr_t attr)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_set_src_sec_attr(&s_dma.hal, id, attr);
    return BK_OK;
}

bk_err_t bk_dma_set_dest_burst_len(dma_id_t id, dma_burst_len_t len)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_set_dest_burst_len(&s_dma.hal, id, len);
    return BK_OK;
}

uint32_t bk_dma_get_dest_burst_len(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    return dma_hal_get_dest_burst_len(&s_dma.hal, id);
}

bk_err_t bk_dma_set_src_burst_len(dma_id_t id, dma_burst_len_t len)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_set_src_burst_len(&s_dma.hal, id, len);
    return BK_OK;
}

uint32_t bk_dma_get_src_burst_len(dma_id_t id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    return dma_hal_get_src_burst_len(&s_dma.hal, id);
}

bk_err_t bk_dma_set_sec_attr(dma_id_t id, dma_sec_attr_t attr)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_set_sec_attr(&s_dma.hal, id, attr);
    return BK_OK;
}

bk_err_t bk_dma_set_privileged_attr(dma_id_t id, dma_sec_attr_t attr)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);

    dma_hal_set_privileged_attr(&s_dma.hal, id, attr);
    return BK_OK;
}

bk_err_t bk_dma_set_int_allocate(dma_id_t id,dma_int_id_t int_id)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_set_int_allocate(&s_dma.hal, id, int_id);
    return BK_OK;
}


uint32_t bk_dma_get_repeat_wr_pause(dma_id_t id)
{
	return dma_hal_repeat_wr_pause(&s_dma.hal, id);
}

uint32_t bk_dma_get_repeat_rd_pause(dma_id_t id)
{
	return dma_hal_repeat_rd_pause(&s_dma.hal, id);
}

uint32_t bk_dma_get_finish_interrupt_cnt(dma_id_t id)
{
	return dma_hal_finish_interrupt_cnt(&s_dma.hal, id);
}

uint32_t bk_dma_get_half_finish_interrupt_cnt(dma_id_t id)
{
	return dma_hal_half_finish_interrupt_cnt(&s_dma.hal, id);
}

bk_err_t bk_dma_stateless_judgment_configuration(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl, void *finish_isr)
{
    dma_config_t dma_config;

    os_memset(&dma_config, 0, sizeof(dma_config_t));

    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 0;

    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.src.start_addr = (uintptr_t)in;
    dma_config.src.end_addr = (uintptr_t)(in + len + 4);

    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.start_addr = (uintptr_t)out;
    dma_config.dst.end_addr = (uintptr_t)(out + len + 4);

    /* init */
    s_dma.id_init_bits |= BIT(cpy_chnl);
#if CONFIG_SUPPORT_CACHEABLE_SRAM
    flush_dcache((void *)dma_config.src.start_addr, dma_config.src.end_addr - dma_config.src.start_addr);
    flush_dcache((void *)dma_config.dst.start_addr, dma_config.dst.end_addr - dma_config.dst.start_addr);
#endif
    dma_hal_init_dma(&s_dma.hal, cpy_chnl, &dma_config);

    /* register isr */
    s_dma_finish_isr[cpy_chnl] = (dma_isr_t)finish_isr;

    /* enable or disable finish interrupt*/
    if(finish_isr) {
        dma_id_enable_interrupt_common(cpy_chnl);
        dma_hal_enable_finish_interrupt(&s_dma.hal, cpy_chnl);
    }

    return BK_OK;

}

bk_err_t dma_memcpy_by_chnl(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl)
{
    DMA_RETURN_ON_NOT_INIT();
    DMA_RETURN_ON_INVALID_ID(cpy_chnl);

    dma_config_t dma_config;

    os_memset(&dma_config, 0, sizeof(dma_config_t));

    dma_config.mode = DMA_WORK_MODE_SINGLE;
    dma_config.chan_prio = 0;

    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.start_addr = (uint32_t)in;
    dma_config.src.end_addr = (uint32_t)(in + len);

    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.start_addr = (uint32_t)out;
    dma_config.dst.end_addr = (uint32_t)(out + len);

    DMA_LOGV("dma_memcpy cpy_chnl: %d\r\n", cpy_chnl);

    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();

    dma_wait_to_idle(cpy_chnl);
    
    bk_dma_init(cpy_chnl, &dma_config);
    dma_hal_set_transfer_len(&s_dma.hal, cpy_chnl, len);

#if (CONFIG_SPE)
    dma_hal_set_src_sec_attr(&s_dma.hal, cpy_chnl, DMA_ATTR_SEC);
    dma_hal_set_dest_sec_attr(&s_dma.hal, cpy_chnl, DMA_ATTR_SEC);
#endif

    dma_hal_start_common(&s_dma.hal, cpy_chnl);
    GLOBAL_INT_RESTORE();

    // Wait for DMA transfer to complete
    BK_WHILE(dma_hal_get_enable_status(&s_dma.hal, cpy_chnl));

#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Invalidate destination cache to ensure CPU reads DMA-written data
    flush_dcache((void *)out, len);
#endif

    return BK_OK;

}

bk_err_t dma_memcpy(void *out, const void *in, uint32_t len)
{
    DMA_RETURN_ON_NOT_INIT();

    bk_err_t ret;
    dma_id_t cpy_chnl = bk_dma_alloc(DMA_DEV_DTCM);
    DMA_RETURN_ON_INVALID_ID(cpy_chnl);

    // Note: Cache operations are handled inside dma_memcpy_by_chnl:
    //   - Before transfer: bk_dma_init() flushes source and destination cache
    //   - After transfer: flush_dcache() invalidates destination cache
    ret = dma_memcpy_by_chnl(out, in, len, cpy_chnl);

    bk_dma_free(DMA_DEV_DTCM, cpy_chnl);

    return ret;
}

#if CONFIG_GENERAL_DMA_LINK_MODE
bk_err_t bk_dma_set_next_ll_addr(dma_id_t id, uint32_t ll_addr)
{
	DMA_RETURN_ON_NOT_INIT();
	DMA_RETURN_ON_INVALID_ID(id);
    dma_hal_set_next_ll_addr(&s_dma.hal, id, ll_addr);
    return BK_OK;
}

uint32_t bk_dma_get_next_ll_addr(dma_id_t id)
{
	DMA_RETURN_ON_NOT_INIT();
	DMA_RETURN_ON_INVALID_ID(id);
	return dma_hal_get_next_ll_addr(&s_dma.hal, id);
}


/**
 * @brief Get descriptor address by index
 * @param desc_table Descriptor table pointer (first descriptor address)
 * @param index Descriptor index
 * @return Pointer to the descriptor at index
 * 
 * @note Each descriptor is 16-byte aligned (dma_descriptor_t is 16 bytes and already aligned)
 */
static inline dma_descriptor_t *dma_get_desc_by_index(void *desc_table, uint32_t index)
{
    // dma_descriptor_t is 16 bytes and already 16-byte aligned, so no additional alignment needed
    const uint32_t desc_size = sizeof(dma_descriptor_t);  // 16 bytes
    uint32_t first_desc_addr = (uint32_t)desc_table;
    uint32_t desc_addr = first_desc_addr + index * desc_size;
    return (dma_descriptor_t *)desc_addr;
}

/**
 * @brief Initialize descriptor table for linked list transfer
 * @param link_cnt Number of descriptors
 * @return Descriptor table pointer, NULL on failure
 * 
 * @note Each descriptor is 16 bytes and already 16-byte aligned (dma_descriptor_t).
 *       Descriptors are placed consecutively with 16-byte spacing.
 */
void *bk_dma_link_init(uint32_t link_cnt)
{
    // dma_descriptor_t is 16 bytes and already 16-byte aligned
    const uint32_t desc_size = sizeof(dma_descriptor_t);  // 16 bytes
    
    // Allocate memory: each descriptor needs desc_size, plus space for alignment and raw pointer storage
    // Total: link_cnt * desc_size + 15 (for first descriptor alignment) + sizeof(void*) (for raw pointer)
    void *raw_ptr = os_malloc(link_cnt * desc_size + 15 + sizeof(void*));
    if (raw_ptr == NULL) {
        DMA_LOGE("Failed to allocate descriptor table\r\n");
        return NULL;
    }
    
    // Align first descriptor to 16-byte boundary
    uint32_t first_desc_addr = ((uint32_t)raw_ptr + 15 + sizeof(void*)) & ~15;
    
    // Store raw pointer before aligned address for deinit
    void **raw_ptr_storage = (void **)(first_desc_addr - sizeof(void*));
    *raw_ptr_storage = raw_ptr;
    
    DMA_LOGV("%s raw_ptr=0x%x first_desc=0x%x desc_size=%d\r\n", 
               __func__, raw_ptr, first_desc_addr, desc_size);
    
    // Clear all descriptors
    os_memset((void *)first_desc_addr, 0, link_cnt * desc_size);
    
    // Link descriptors together - each descriptor's next_desc_addr points to next 16-byte aligned address
    for (int i = 0; i < link_cnt - 1; i++) {
        uintptr_t curr_desc_addr = first_desc_addr + i * desc_size;
        uintptr_t next_desc_addr = first_desc_addr + (i + 1) * desc_size;
        dma_descriptor_t *curr_desc = (dma_descriptor_t *)curr_desc_addr;
        
        // Verify alignment
        if ((curr_desc_addr & 15) != 0 || (next_desc_addr & 15) != 0) {
            DMA_LOGE("Descriptor alignment error: curr=0x%x next=0x%x\r\n", curr_desc_addr, next_desc_addr);
        }
        
        curr_desc->next_desc_addr = (uint32_t)next_desc_addr;
        DMA_LOGV("Desc[%d] addr=0x%x next_addr=0x%x\r\n", i, curr_desc_addr, curr_desc->next_desc_addr);
    }
    
    // Last descriptor: next_addr = 0 (end of list)
    uintptr_t last_desc_addr = first_desc_addr + (link_cnt - 1) * desc_size;
    dma_descriptor_t *last_desc = (dma_descriptor_t *)last_desc_addr;
    last_desc->next_desc_addr = 0;
    DMA_LOGV("Desc[%d] addr=0x%x next_addr=0 (end of list)\r\n", link_cnt - 1, last_desc_addr);
    
#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Flush descriptor table to ensure DMA sees latest data after initialization
    flush_dcache((void *)first_desc_addr, link_cnt * desc_size);
#endif
    
    return (void *)first_desc_addr;
}

/**
 * @brief Deinitialize descriptor table (dma_descriptor_t)
 * @param desc_table Descriptor table pointer returned by bk_dma_link_init
 * 
 * @note This function retrieves the raw pointer stored before the aligned address and frees it.
 */
void bk_dma_link_deinit(void *desc_table)
{
    if (desc_table == NULL) {
        return;
    }
    
    // Get raw pointer stored before aligned address
    void **raw_ptr_storage = (void **)((uintptr_t)desc_table - sizeof(void*));
    void *raw_ptr = *raw_ptr_storage;
    
    // Validate: raw_ptr should be within reasonable range
    // (desc_table - 15 - sizeof(void*) to desc_table - sizeof(void*))
    if (raw_ptr != NULL && 
        (uintptr_t)raw_ptr >= ((uintptr_t)desc_table - 15 - sizeof(void*)) &&
        (uintptr_t)raw_ptr < (uintptr_t)desc_table) {
        // Free raw pointer (which was allocated with extra space)
        os_free(raw_ptr);
        DMA_LOGV("%s freed desc_table=0x%x raw_ptr=0x%x\r\n", __func__, desc_table, raw_ptr);
    } else {
        DMA_LOGE("%s invalid raw_ptr: desc_table=0x%x raw_ptr=0x%x\r\n", __func__, desc_table, raw_ptr);
    }
}

/**
 * @brief Set descriptor for transfer
 * @param desc_table Descriptor table pointer
 * @param index Descriptor index
 * @param config Link configuration
 * @return BK_OK on success
 */
bk_err_t bk_dma_link_set_desc(void *desc_table, uint32_t index,
                               const dma_link_config_t *config)
{
    if (desc_table == NULL || config == NULL) {
        DMA_LOGE("Invalid parameters\r\n");
        return BK_ERR_NULL_PARAM;
    }
    
    // Get descriptor by index
    dma_descriptor_t *desc = dma_get_desc_by_index(desc_table, index);
    
    if ((config->src_addr & 0xF) != 0 || (config->dst_addr & 0xF) != 0) {
        DMA_LOGE("Address not 128-bit aligned: src=0x%x dst=0x%x\r\n", config->src_addr, config->dst_addr);
        return BK_ERR_PARAM;
    }
    
    // Configure descriptor - directly copy fields since structures match
    desc->src_addr = config->src_addr;
    desc->dst_addr = config->dst_addr;
    desc->ctrl.control = config->ctrl.control;  // Copy control word (includes length and interrupt enables)
    
    DMA_LOGV("Desc[%d] src=0x%x dst=0x%x length=%d finish_int=%d half_finish_int=%d\r\n",
               index, config->src_addr, config->dst_addr,
               config->ctrl.bits.length, config->ctrl.bits.int_finish_en, config->ctrl.bits.int_half_finish_en);
    
#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Flush descriptor to ensure DMA sees latest data
    // Note: Source and destination addresses cache will be flushed in bk_dma_link_transfer
    // using flush_all_dcache(), so no need to flush them here
    flush_dcache((void *)desc, sizeof(dma_descriptor_t));
#endif
    return BK_OK;
}

/**
 * @brief Set multiple descriptors for transfer
 * @param desc_table Descriptor table pointer
 * @param configs Configuration array
 * @param link_cnt Number of descriptors
 * @return BK_OK on success
 */
bk_err_t bk_dma_link_set_descs(void *desc_table,
                                const dma_link_config_t *configs,
                                uint32_t link_cnt)
{
    if (desc_table == NULL || configs == NULL) {
        DMA_LOGE("Invalid parameters\r\n");
        return BK_ERR_NULL_PARAM;
    }
    
    for (uint32_t i = 0; i < link_cnt; i++) {
        bk_err_t ret = bk_dma_link_set_desc(desc_table, i, &configs[i]);
        if (ret != BK_OK) {
            DMA_LOGE("Failed to set desc[%d]\r\n", i);
            return ret;
        }
    }
    
    return BK_OK;
}

/**
 * @brief Execute linked list transfer (synchronous)
 * @param id DMA channel ID (allocated by application layer using bk_dma_alloc())
 * @param desc_table Descriptor table pointer
 * @return BK_OK on success
 */
bk_err_t bk_dma_link_transfer(dma_id_t id, void *desc_table)
{
    if (desc_table == NULL) {
        DMA_LOGE("Invalid descriptor table\r\n");
        return BK_ERR_NULL_PARAM;
    }
    
    if (id >= SOC_DMA_CHAN_NUM_PER_UNIT * SOC_DMA_UNIT_NUM) {
        DMA_LOGE("Invalid DMA channel ID: %d\r\n", id);
        return BK_ERR_DMA_ID;
    }
    
    DMA_LOGV("%s DMA channel = %d\r\n", __func__, id);

    dma_config_t dma_config;

    os_memset(&dma_config, 0, sizeof(dma_config_t));
    
    // Configure DMA for linked list mode (all info in descriptors)
    // Mode: Set register values to 0, hardware reads all info from descriptors
    dma_config.mode = DMA_WORK_MODE_SINGLE;
    dma_config.chan_prio = 0;
    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;  // Use 32-bit width for linked list mode
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
    
    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;  // Use 32-bit width for linked list mode
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;

    dma_hal_set_transfer_len(&s_dma.hal, id, 0);

    // Set next linked list address
    bk_dma_set_next_ll_addr(id, (uint32_t)desc_table);

    // Initialize and configure DMA
    // Note: In linked list mode, src/dst addresses are in descriptors, not in dma_config
    bk_dma_init(id, &dma_config);
    
#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Flush all cache to ensure DMA sees all descriptors and data
    // This is more efficient than traversing all descriptors when link_cnt is large
    // flush_all_dcache() will flush:
    // 1. Descriptor table (already flushed in bk_dma_link_init and bk_dma_link_set_desc, but flush again for safety)
    // 2. All source addresses (CPU-written data)
    // 3. All destination addresses (prepare for DMA write)
    flush_all_dcache();
#endif

#if CONFIG_SPE
    bk_dma_set_src_sec_attr(id, DMA_ATTR_SEC);
    bk_dma_set_dest_sec_attr(id, DMA_ATTR_SEC);
#endif

    bk_dma_set_src_burst_len(id, BURST_LEN_INC16);
    bk_dma_set_dest_burst_len(id, BURST_LEN_INC16);

    bk_dma_start(id);
    
    DMA_LOGV("%s DMA started\r\n", __func__);

    return BK_OK;
}

#endif // CONFIG_GENERAL_DMA_LINK_MODE

static void dma_isr_common(dma_unit_t dma_unit_id)
{
    dma_hal_t *hal = &s_dma.hal;
    uint32_t channel = 0;
    for (int id = 0; id < SOC_DMA_CHAN_NUM_PER_UNIT; id++) {
        channel = id + dma_unit_id * SOC_DMA_CHAN_NUM_PER_UNIT;

        if (dma_hal_is_half_finish_interrupt_triggered(hal, id)) {
            DMA_LOGV("dma_isr HALF FINISH TRIGGERED! id: %d\r\n", id);
            //NOTES:clear intrrupt in condition because maybe multi-core(two CPU) access one DMA
            //it can't cleared peer-side channels status.
            if (s_dma_half_finish_isr[id]) {
                DMA_LOGV("dma_isr HALF_finish_isr! id: %d\r\n", id);
                dma_hal_clear_half_finish_interrupt_status(hal, id);
                s_dma_half_finish_isr[id](channel);
            }
        }
        if (dma_hal_is_finish_interrupt_triggered(hal, id)) {
#if CONFIG_SUPPORT_CACHEABLE_SRAM
            flush_all_dcache();
#endif
            DMA_LOGV("dma_isr ALL FINISH TRIGGERED! id: %d\r\n", id);
            if (s_dma_finish_isr[id]) {
                DMA_LOGV("dma_isr ALL_finish_isr! id: %d\r\n", id);
                dma_hal_clear_finish_interrupt_status(hal, id);
                s_dma_finish_isr[id](channel);
            }
        }

           if (dma_hal_is_bus_err_interrupt_triggered(hal, id)) {
           DMA_LOGE("dma_isr BUS ERR! id: %d\r\n", id);
           if (s_dma_bus_err_isr[id]) {
               DMA_LOGE("dma_isr BUS ERR CALLBACK! id: %d\r\n", id);
               dma_hal_clear_finish_interrupt_status(hal, id);
               s_dma_bus_err_isr[id](channel);
           }
        }

   }
}

static void dma_isr(void)
{
	DMA_LOGV("dma_isr hw 0\r\n");
	dma_isr_common(0);
}
