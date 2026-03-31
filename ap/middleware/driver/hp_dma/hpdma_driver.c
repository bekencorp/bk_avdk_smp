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
#include <os/os.h>
#include "icu_driver.h"
#include "hpdma_driver.h"
#include "hpdma_hal.h"
#include <driver/hpdma.h>
#include <driver/int.h>
#include "sys_driver.h"

#if CONFIG_SUPPORT_CACHEABLE_SRAM
#include "cache.h"
#endif

#include "bk_misc.h"

#include <os/mem.h>

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static volatile spinlock_t hpdma_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP


static inline uint32_t hpdma_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
	spin_lock(&hpdma_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	return flags;
}

static inline void hpdma_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
	spin_unlock(&hpdma_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	rtos_enable_int(flags);
}

static void hpdma_isr(void) __BK_SECTION(".itcm");	//dma-hw-0


typedef struct {
    hpdma_hal_t hal;
    uint32_t id_init_bits;
} hpdma_driver_t;

static hpdma_driver_t s_hpdma;
static hpdma_isr_info_t s_hpdma_finish_isr[SOC_HPDMA_CHAN_NUM_PER_UNIT] = {{NULL, NULL}};
static hpdma_isr_info_t s_hpdma_half_finish_isr[SOC_HPDMA_CHAN_NUM_PER_UNIT] = {{NULL, NULL}};
static hpdma_isr_info_t s_hpdma_bus_err_isr[SOC_HPDMA_CHAN_NUM_PER_UNIT] = {{NULL, NULL}};

static bool s_hpdma_driver_is_init = false;
static hpdma_chnl_pool_t s_hpdma_chnl_pool = {0};

#define HPDMA_RETURN_ON_NOT_INIT() do {\
        if (!s_hpdma_driver_is_init) {\
            return BK_ERR_HPDMA_NOT_INIT;\
        }\
    } while(0)

#define HPDMA_RETURN_ON_INVALID_ID(channel) do {\
        if (((channel) < CONFIG_HPDMA_LOGIC_CHAN_ID_MIN) || ((channel) >= CONFIG_HPDMA_LOGIC_CHAN_ID_MIN + CONFIG_HPDMA_LOGIC_CHAN_CNT)) {\
            return BK_ERR_HPDMA_ID;\
        }\
    } while(0)

#define HPDMA_RETURN_ON_ID_NOT_INIT(hpdma_num,id) do {\
        if (!(s_hpdma.id_init_bits & BIT((id)))) {\
            return BK_ERR_HPDMA_ID_NOT_INIT;\
        }\
    } while(0)

#define HPDMA_LOG_ON_ID_IS_STARTED(hpdma_num,channel) do {\
        if (hpdma_hal_is_id_started(&s_hpdma.hal, (channel))) {\
        }\
    } while(0)

#define HPDMA_RETURN_ON_INVALID_ADDR(start_addr, end_addr) do {\
        if ((0 < (end_addr)) && ((end_addr) < (start_addr))) {\
            return BK_ERR_HPDMA_INVALID_ADDR;\
        }\
    } while(0)

static void hpdma_id_init_common(hpdma_id_t id)
{
    // 2: cache enabled
    hpdma_hal_set_cfg_cache(&s_hpdma.hal, id, 2);
    s_hpdma.id_init_bits |= BIT(id);

}

static void hpdma_id_deinit_common(hpdma_id_t id)
{
    s_hpdma.id_init_bits &= ~BIT(id);
    hpdma_hal_stop_common(&s_hpdma.hal, id);
    hpdma_hal_reset_config_to_default(&s_hpdma.hal, id);
}

static void hpdma_id_enable_interrupt_common(hpdma_id_t id)
{
    //0:int route to M55 core 0
    hpdma_hal_set_int_allocate(&s_hpdma.hal, id, HPDMA_INT_0);
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_HPDMA, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_HPDMA, 1);
#endif
}

/* used internally, called in context of interrupt disabled. */
u8 hpdma_chnl_alloc(u32 user_id)
{
	u8 chnl_id;

    for(chnl_id = CONFIG_HPDMA_LOGIC_CHAN_ID_MIN; chnl_id < CONFIG_HPDMA_LOGIC_CHAN_ID_MIN + CONFIG_HPDMA_LOGIC_CHAN_CNT; chnl_id++)
	{
		if((s_hpdma_chnl_pool.chnl_bitmap & (0x01 << chnl_id)) == 0)
		{
			s_hpdma_chnl_pool.chnl_bitmap |= (0x01 << chnl_id);
			s_hpdma_chnl_pool.chnl_user[chnl_id] = user_id;
			return chnl_id;
		}
	}

	//alloc failed
	HPDMA_LOGE("%s:chnl_id=%d\r\n", __func__, chnl_id);
	return HPDMA_ID_MAX;
}

hpdma_id_t hpdma_fixed_chnl_alloc(u32 user_id, hpdma_id_t fixed_chnl_id)
{

    if((fixed_chnl_id >= CONFIG_HPDMA_LOGIC_CHAN_ID_MIN) && (fixed_chnl_id < CONFIG_HPDMA_LOGIC_CHAN_ID_MIN + CONFIG_HPDMA_LOGIC_CHAN_CNT))
	{
		if((s_hpdma_chnl_pool.chnl_bitmap & (0x01 << fixed_chnl_id)) == 0)
		{
			s_hpdma_chnl_pool.chnl_bitmap |= (0x01 << fixed_chnl_id);
			s_hpdma_chnl_pool.chnl_user[fixed_chnl_id] = user_id;
			return fixed_chnl_id;
		}
	}

	HPDMA_LOGE("chan=%d has been allocated\r\n", fixed_chnl_id);
	return HPDMA_ID_MAX;
}

/* used internally, called in context of interrupt disabled. */
bk_err_t hpdma_chnl_free(u32 user_id, hpdma_id_t chnl_id)
{
	if( chnl_id >= HPDMA_ID_MAX )
		return BK_ERR_HPDMA_ID;

	if( s_hpdma_chnl_pool.chnl_user[chnl_id] != user_id )
		return BK_ERR_PARAM;

	s_hpdma_chnl_pool.chnl_bitmap &= ~(0x01 << chnl_id);
	s_hpdma_chnl_pool.chnl_user[chnl_id] = -1;

	return BK_OK;
}

/* used internally. */
u32 hpdma_chnl_user(hpdma_id_t chnl_id)
{
	if( chnl_id >= HPDMA_ID_MAX )
		return -1;

	return s_hpdma_chnl_pool.chnl_user[chnl_id];
}

bk_err_t bk_hpdma_driver_init(void)
{
    if (s_hpdma_driver_is_init) {
        return BK_OK;
    }

	s_hpdma_chnl_pool.chnl_bitmap = 0;
	for(u8 i = CONFIG_HPDMA_LOGIC_CHAN_ID_MIN; i < CONFIG_HPDMA_LOGIC_CHAN_ID_MIN + CONFIG_HPDMA_LOGIC_CHAN_CNT; i++)
	{
		s_hpdma_chnl_pool.chnl_user[i] = -1;
	}

    // workaround: must uncomment it after mailbox problem fixed
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA0, PM_POWER_MODULE_STATE_ON);
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA1, PM_POWER_MODULE_STATE_ON);

    /* 1)intc_service_register
     * 2)init hpdma_finish_int handler, hpdma_half_finish_int handler
     * 3)disable hpdma_en (0~6), clear int status
     * 4)init hpdma_config
     */
    os_memset(&s_hpdma, 0, sizeof(s_hpdma));
    os_memset(&s_hpdma_finish_isr, 0, sizeof(s_hpdma_finish_isr));
    os_memset(&s_hpdma_half_finish_isr, 0, sizeof(s_hpdma_half_finish_isr));
    os_memset(&s_hpdma_bus_err_isr, 0, sizeof(s_hpdma_bus_err_isr));

    bk_int_isr_register(INT_SRC_HPDMA, hpdma_isr, NULL);
    

    for (uint32_t uint_id = 0; uint_id < SOC_HPDMA_UNIT_NUM; uint_id++) {
	    s_hpdma.hal.id = uint_id;
		hpdma_hal_init(&s_hpdma.hal);
	}

    s_hpdma_driver_is_init = true;

#if CONFIG_HIGH_PERFORMANCE_DMA_TEST
    extern int bk_hpdma_register_cli_test_feature(void);
    BK_LOG_ON_ERR(bk_hpdma_register_cli_test_feature());
#endif

    return BK_OK;
}

bk_err_t bk_hpdma_driver_deinit(void)
{
    if (!s_hpdma_driver_is_init) {
        return BK_OK;
    }

    for (int id = 0; id < (SOC_HPDMA_CHAN_NUM_PER_UNIT*SOC_HPDMA_UNIT_NUM); id++) {
        hpdma_id_deinit_common(id);
    }

#if CONFIG_PM_ENABLE
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA0, PM_POWER_MODULE_STATE_OFF);
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA1, PM_POWER_MODULE_STATE_OFF);
#endif


#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_HPDMA, 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_HPDMA, 0);
#endif

    s_hpdma_driver_is_init = false;

    return BK_OK;
}

hpdma_id_t bk_hpdma_alloc(u16 user_id)
{
    if (!s_hpdma_driver_is_init)
    {
        return HPDMA_ID_MAX;
    }

    u32 int_mask = hpdma_enter_critical();

    u8 chnl_id = hpdma_chnl_alloc(user_id);

    hpdma_exit_critical(int_mask);

    return chnl_id;
}

hpdma_id_t bk_fixed_hpdma_alloc(u16 user_id, hpdma_id_t chnl_id)
{
    if (!s_hpdma_driver_is_init)
    {
        return HPDMA_ID_MAX;
    }

    u32 int_mask = hpdma_enter_critical();

    u8 chnl_id_ret = hpdma_fixed_chnl_alloc(user_id, chnl_id);

    hpdma_exit_critical(int_mask);

    return chnl_id_ret;
}

bk_err_t bk_hpdma_free(u16 user_id, hpdma_id_t chnl_id)
{
    if (!s_hpdma_driver_is_init)
    {
        return BK_ERR_HPDMA_NOT_INIT;
    }

    u32  int_mask = hpdma_enter_critical();

    bk_err_t ret_val = hpdma_chnl_free(user_id, chnl_id);

    hpdma_exit_critical(int_mask);

    return ret_val;
}

uint32_t bk_hpdma_user(hpdma_id_t chnl_id)
{
    if (!s_hpdma_driver_is_init)
    {
        return -1;
    }

    return hpdma_chnl_user(chnl_id);
}

bk_err_t bk_hpdma_init(hpdma_id_t id, const hpdma_config_t *config)
{
    // Validate ysize: user input should be >= 1 (1 = 1 row, 2 = 2 rows, etc.)
    if (config->src.ysize == 0) {
        HPDMA_LOGE("Source ysize must be >= 1 (1 = 1 row, 2 = 2 rows, etc.)\r\n");
        return BK_ERR_PARAM;
    }
    if (config->dst.ysize == 0) {
        HPDMA_LOGE("Destination ysize must be >= 1 (1 = 1 row, 2 = 2 rows, etc.)\r\n");
        return BK_ERR_PARAM;
    }

    hpdma_hal_init_without_channels(&s_hpdma.hal);	//TODO:special codes for DMA init after enter low voltage

#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Calculate total transfer size for cache flush
    // User input: ysize = 1 means 1 row, ysize = 2 means 2 rows, etc.
    // Total size = xsize * ysize (user input)
    uint32_t src_total_size = config->src.xsize * config->src.ysize;
    uint32_t dst_total_size = config->dst.xsize * config->dst.ysize;
    
    // Flush source cache to ensure DMA reads latest CPU-written data
    if (src_total_size > 0) {
        flush_dcache((void *)config->src.start_addr, src_total_size);
    }
    
    // Flush destination cache to prepare for DMA write
    // This ensures any dirty cache lines are written back before DMA overwrites memory
    if (dst_total_size > 0) {
        flush_dcache((void *)config->dst.start_addr, dst_total_size);
    }
#endif

    // Create a modified config with ysize decremented by 1 for hardware
    // Hardware expects: ysize = 0 means 1 row, ysize = 1 means 2 rows, etc.
    // User input: ysize = 1 means 1 row, ysize = 2 means 2 rows, etc.
    hpdma_config_t hw_config = *config;
    hw_config.src.ysize = config->src.ysize - 1;
    hw_config.dst.ysize = config->dst.ysize - 1;

    hpdma_id_init_common(id);
    return hpdma_hal_init_dma(&s_hpdma.hal, id, &hw_config);
}

bk_err_t bk_hpdma_deinit(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_id_deinit_common(id);
    bk_hpdma_register_isr(id, NULL, NULL, NULL, NULL);
    return BK_OK;
}

bk_err_t bk_hpdma_start(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    hpdma_hal_start_common(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_stop(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();

    hpdma_hal_stop_common(&s_hpdma.hal, id);
    return BK_OK;
}

uint32_t bk_hpdma_get_enable_status(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();

    uint32_t ret;
    ret = hpdma_hal_get_enable_status(&s_hpdma.hal, id);
    return ret;
}

#define HPDMA_MAX_BUSY_TIME (10000)  //us
uint32_t hpdma_wait_to_idle(hpdma_id_t id)
{
	if(hpdma_hal_get_work_mode(&s_hpdma.hal, id) == HPDMA_WORK_MODE_SINGLE)
	{
		uint32_t i = 0;
		while(hpdma_hal_get_enable_status(&s_hpdma.hal, id))
		{
			bk_delay_us(1);

			i++;
			if(i > HPDMA_MAX_BUSY_TIME)
			{
				HPDMA_LOGE("ch%d busy,remain len=%d,dst_addr=%x\r\n", id,
							hpdma_hal_get_remain_len(&s_hpdma.hal, id),
							hpdma_hal_get_dest_write_addr(&s_hpdma.hal, id));
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

bk_err_t bk_hpdma_enable_finish_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_id_enable_interrupt_common(id);
    hpdma_hal_enable_finish_interrupt(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_finish_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_disable_finish_interrupt(&s_hpdma.hal, id);
    hpdma_hal_clear_finish_interrupt_status(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_enable_half_finish_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_id_enable_interrupt_common(id);
    hpdma_hal_enable_half_finish_interrupt(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_half_finish_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_disable_half_finish_interrupt(&s_hpdma.hal, id);
    hpdma_hal_clear_half_finish_interrupt_status(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_enable_bus_err_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_id_enable_interrupt_common(id);
    hpdma_hal_enable_bus_err_interrupt(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_bus_err_interrupt(hpdma_id_t id)
{
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_disable_bus_err_interrupt(&s_hpdma.hal, id);
    hpdma_hal_clear_bus_err_interrupt_status(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_register_isr(hpdma_id_t id, hpdma_isr_t half_finish_isr, void *half_finish_data, 
                                hpdma_isr_t finish_isr, void *finish_data)
{
    HPDMA_RETURN_ON_NOT_INIT();

    HPDMA_RETURN_ON_INVALID_ID(id);
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    s_hpdma_half_finish_isr[id].callback = half_finish_isr;
    s_hpdma_half_finish_isr[id].user_data = half_finish_data;
    s_hpdma_finish_isr[id].callback = finish_isr;
    s_hpdma_finish_isr[id].user_data = finish_data;
    GLOBAL_INT_RESTORE();

    return BK_OK;
}

bk_err_t bk_hpdma_register_bus_err_isr(hpdma_id_t id, hpdma_isr_t bus_err_isr, void *user_data)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    s_hpdma_bus_err_isr[id].callback = bus_err_isr;
    s_hpdma_bus_err_isr[id].user_data = user_data;
    GLOBAL_INT_RESTORE();

    return BK_OK;
}

bk_err_t bk_hpdma_set_src_start_addr(hpdma_id_t id, uint32_t start_addr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_hal_set_src_start_addr(&s_hpdma.hal, id, start_addr);
    return BK_OK;
}

bk_err_t bk_hpdma_set_dest_start_addr(hpdma_id_t id, uint32_t start_addr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_wait_to_idle(id);
    hpdma_hal_set_dest_start_addr(&s_hpdma.hal, id, start_addr);
    return BK_OK;
}

bk_err_t bk_hpdma_enable_src_addr_increase(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_hal_enable_src_addr_inc(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_src_addr_increase(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_hal_disable_src_addr_inc(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_enable_src_addr_loop(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_hal_enable_src_addr_loop(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_src_addr_loop(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_hal_disable_src_addr_loop(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_enable_dest_addr_increase(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_hal_enable_dest_addr_inc(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_dest_addr_increase(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_hal_disable_dest_addr_inc(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_enable_dest_addr_loop(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_hal_enable_dest_addr_loop(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_disable_dest_addr_loop(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_hal_disable_dest_addr_loop(&s_hpdma.hal, id);
    return BK_OK;
}

uint32_t bk_hpdma_get_remain_len(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    return hpdma_hal_get_remain_len(&s_hpdma.hal, id);
}

bk_err_t hpdma_set_src_pause_addr(hpdma_id_t id, uint32_t addr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_set_src_pause_addr(&s_hpdma.hal, id, addr);

    return BK_OK;
}

bk_err_t hpdma_set_dst_pause_addr(hpdma_id_t id, uint32_t addr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_set_dest_pause_addr(&s_hpdma.hal, id, addr);

    return BK_OK;
}

uint32_t hpdma_get_src_read_addr(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    return hpdma_hal_get_src_read_addr(&s_hpdma.hal, id);
}

uint32_t hpdma_get_dest_write_addr(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    return hpdma_hal_get_dest_write_addr(&s_hpdma.hal, id);
}

bk_err_t bk_hpdma_set_src_data_width(hpdma_id_t id, hpdma_data_width_t data_width)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_wait_to_idle(id);
    hpdma_hal_set_src_data_width(&s_hpdma.hal, id, data_width);
    return BK_OK;
}

bk_err_t bk_hpdma_set_dest_data_width(hpdma_id_t id, hpdma_data_width_t data_width)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_wait_to_idle(id);
    hpdma_hal_set_dest_data_width(&s_hpdma.hal, id, data_width);
    return BK_OK;
}


bk_err_t bk_hpdma_set_pixel_trans_type(hpdma_id_t id, hpdma_pixel_trans_type_t type)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);


    hpdma_hal_set_pixel_trans_type(&s_hpdma.hal, id, type);
    return BK_OK;
}

uint32_t bk_hpdma_get_pixel_trans_type(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);


    return hpdma_hal_get_pixel_trans_type(&s_hpdma.hal, id);
}

bk_err_t bk_hpdma_bus_err_int_enable(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_bus_err_int_enable(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_bus_err_int_diable(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_bus_err_int_disable(&s_hpdma.hal, id);
    return BK_OK;
}

bk_err_t bk_hpdma_set_dest_sec_attr(hpdma_id_t id, hpdma_sec_attr_t attr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_set_dest_sec_attr(&s_hpdma.hal, id, attr);
    return BK_OK;
}

bk_err_t bk_hpdma_set_src_sec_attr(hpdma_id_t id, hpdma_sec_attr_t attr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_set_src_sec_attr(&s_hpdma.hal, id, attr);
    return BK_OK;
}

bk_err_t bk_hpdma_set_dest_burst_len(hpdma_id_t id, hpdma_burst_len_t len)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_set_dest_burst_len(&s_hpdma.hal, id, len);
    return BK_OK;
}

uint32_t bk_hpdma_get_dest_burst_len(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    return hpdma_hal_get_dest_burst_len(&s_hpdma.hal, id);
}

bk_err_t bk_hpdma_set_src_burst_len(hpdma_id_t id, hpdma_burst_len_t len)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_set_src_burst_len(&s_hpdma.hal, id, len);
    return BK_OK;
}

uint32_t bk_hpdma_get_src_burst_len(hpdma_id_t id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    return hpdma_hal_get_src_burst_len(&s_hpdma.hal, id);
}

bk_err_t bk_hpdma_set_sec_attr(hpdma_id_t id, hpdma_sec_attr_t attr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_set_sec_attr(&s_hpdma.hal, id, attr);
    return BK_OK;
}

bk_err_t bk_hpdma_set_privileged_attr(hpdma_id_t id, hpdma_sec_attr_t attr)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);

    hpdma_hal_set_privileged_attr(&s_hpdma.hal, id, attr);
    return BK_OK;
}

bk_err_t bk_hpdma_set_int_allocate(hpdma_id_t id, hpdma_int_id_t int_id)
{
    HPDMA_RETURN_ON_NOT_INIT();
    HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_hal_set_int_allocate(&s_hpdma.hal, id, int_id);
    return BK_OK;
}

hpdma_int_id_t bk_hpdma_get_int_allocate(hpdma_id_t id)
{
    return hpdma_hal_get_int_allocate(&s_hpdma.hal, id);
}


uint32_t bk_hpdma_get_repeat_wr_pause(hpdma_id_t id)
{
	return hpdma_hal_repeat_wr_pause(&s_hpdma.hal, id);
}

uint32_t bk_hpdma_get_repeat_rd_pause(hpdma_id_t id)
{
	return hpdma_hal_repeat_rd_pause(&s_hpdma.hal, id);
}

uint32_t bk_hpdma_get_finish_interrupt_cnt(hpdma_id_t id)
{
	return hpdma_hal_finish_interrupt_cnt(&s_hpdma.hal, id);
}

uint32_t bk_hpdma_get_half_finish_interrupt_cnt(hpdma_id_t id)
{
	return hpdma_hal_half_finish_interrupt_cnt(&s_hpdma.hal, id);
}

bk_err_t hpdma_memcpy_by_chnl(void *out, const void *in, uint32_t len, hpdma_id_t cpy_chnl)
{
    hpdma_config_t hpdma_config;

    os_memset(&hpdma_config, 0, sizeof(hpdma_config_t));

    hpdma_config.mode = HPDMA_WORK_MODE_SINGLE;
    hpdma_config.chan_prio = 0;

    hpdma_config.src.dev = HPDMA_DEV_DTCM;
    hpdma_config.src.width = HPDMA_DATA_WIDTH_32BITS;
    hpdma_config.src.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    hpdma_config.src.start_addr = (uintptr_t)in;
    hpdma_config.src.xsize = len;
    hpdma_config.src.ysize = 1;  // 1 row (user input: 1 = 1 row, will be decremented in bk_hpdma_init)
    hpdma_config.src.step = 0;

    hpdma_config.dst.dev = HPDMA_DEV_DTCM;
    hpdma_config.dst.width = HPDMA_DATA_WIDTH_32BITS;
    hpdma_config.dst.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    hpdma_config.dst.start_addr = (uintptr_t)out;
    hpdma_config.dst.xsize = len;
    hpdma_config.dst.ysize = 1;  // 1 row (user input: 1 = 1 row, will be decremented in bk_hpdma_init)
    hpdma_config.dst.step = 0;

    HPDMA_LOGD("hpdma_memcpy cpy_chnl: %d\r\n", cpy_chnl);

    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();

    hpdma_wait_to_idle(cpy_chnl);

    // Note: Cache operations for source and destination are handled in bk_hpdma_init
    // which flushes both source and destination cache before DMA transfer starts
    bk_hpdma_init(cpy_chnl, &hpdma_config);
#if (CONFIG_SPE)
    bk_hpdma_set_src_sec_attr(cpy_chnl, HPDMA_ATTR_SEC);
    bk_hpdma_set_dest_sec_attr(cpy_chnl, HPDMA_ATTR_SEC);
#endif

    hpdma_hal_start_common(&s_hpdma.hal, cpy_chnl);
    GLOBAL_INT_RESTORE();

    //TODO:I think no need to wait copy data finish,just confirm before copy start, the previous one is finish.
    BK_WHILE(hpdma_hal_get_enable_status(&s_hpdma.hal, cpy_chnl));

#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Invalidate destination cache to ensure CPU reads DMA-written data
    flush_dcache((void *)out, len);
#endif

    return BK_OK;
}

bk_err_t bk_hpdma_memcpy(void *out, const void *in, uint32_t len)
{
    HPDMA_RETURN_ON_NOT_INIT();

    bk_err_t ret;
    hpdma_id_t cpy_chnl = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    HPDMA_RETURN_ON_INVALID_ID(cpy_chnl);

    ret = hpdma_memcpy_by_chnl(out, in, len, cpy_chnl);

    // Note: Cache operations are handled inside hpdma_memcpy_by_chnl:
    //   - Before transfer: bk_hpdma_init() may flush source and destination cache
    //   - After transfer: flush_dcache() invalidates destination cache
    bk_hpdma_free(HPDMA_DEV_DTCM, cpy_chnl);

    return ret;
}


bk_err_t bk_hpdma_set_next_ll_addr(hpdma_id_t id, uint32_t ll_addr)
{
	HPDMA_RETURN_ON_NOT_INIT();
	HPDMA_RETURN_ON_INVALID_ID(id);
    hpdma_hal_set_next_ll_addr(&s_hpdma.hal, id, ll_addr);
    return BK_OK;
}

uint32_t bk_hpdma_get_next_ll_addr(hpdma_id_t id)
{
	HPDMA_RETURN_ON_NOT_INIT();
	HPDMA_RETURN_ON_INVALID_ID(id);
    HPDMA_LOGV("%s\r\n", __func__);
	return hpdma_hal_get_next_ll_addr(&s_hpdma.hal, id);
}

// ============================================================================
// New descriptor format implementation (24 bytes, 6 words)
// ============================================================================

void hpdma_link_wait_ready(hpdma_id_t hpdma_chn)
{
    HPDMA_LOGV("%s wait ready\r\n", __func__);
	while(bk_hpdma_get_next_ll_addr(hpdma_chn));
	while(bk_hpdma_get_enable_status(hpdma_chn));
    HPDMA_LOGV("%s\r\n", __func__);
}


static inline hpdma_descriptor_t *hpdma_get_desc_by_index(void *desc_table, uint32_t index)
{
    const uint32_t desc_aligned_size = (sizeof(hpdma_descriptor_t) + 15) & ~15;  // 32 bytes
    uintptr_t first_desc_addr = (uintptr_t)desc_table;
    uintptr_t desc_addr = first_desc_addr + index * desc_aligned_size;
    return (hpdma_descriptor_t *)desc_addr;
}


void *bk_hpdma_link_init(uint32_t link_cnt)
{
    // Calculate aligned size per descriptor (24 bytes -> 32 bytes for 16-byte alignment)
    const uint32_t desc_size = sizeof(hpdma_descriptor_t);  // 24 bytes
    const uint32_t desc_aligned_size = (desc_size + 15) & ~15;  // 32 bytes (aligned to 16)
    
    // Allocate memory: each descriptor needs aligned_size, plus space for alignment and raw pointer storage
    // Total: link_cnt * desc_aligned_size + 15 (for first descriptor alignment) + sizeof(void*) (for raw pointer)
    void *raw_ptr = os_malloc(link_cnt * desc_aligned_size + 15 + sizeof(void*));
    if (raw_ptr == NULL) {
        HPDMA_LOGE("Failed to allocate descriptor table\r\n");
        return NULL;
    }
    
    // Align first descriptor to 16-byte boundary
    uintptr_t first_desc_addr = ((uintptr_t)raw_ptr + 15 + sizeof(void*)) & ~15;
    
    // Store raw pointer before aligned address for deinit
    void **raw_ptr_storage = (void **)(first_desc_addr - sizeof(void*));
    *raw_ptr_storage = raw_ptr;
    
    HPDMA_LOGV("%s raw_ptr=0x%x first_desc=0x%x desc_size=%d aligned_size=%d\r\n", 
               __func__, raw_ptr, first_desc_addr, desc_size, desc_aligned_size);
    
    // Clear all descriptors
    os_memset((void *)first_desc_addr, 0, link_cnt * desc_aligned_size);
    
    // Link descriptors together - each descriptor's next_desc_addr points to next 16-byte aligned address
    for (int i = 0; i < link_cnt - 1; i++) {
        uintptr_t curr_desc_addr = first_desc_addr + i * desc_aligned_size;
        uintptr_t next_desc_addr = first_desc_addr + (i + 1) * desc_aligned_size;
        hpdma_descriptor_t *curr_desc = (hpdma_descriptor_t *)curr_desc_addr;
        
        // Verify alignment
        if ((curr_desc_addr & 15) != 0 || (next_desc_addr & 15) != 0) {
            HPDMA_LOGE("Descriptor alignment error: curr=0x%x next=0x%x\r\n", curr_desc_addr, next_desc_addr);
        }
        
        curr_desc->next_desc_addr = (uint32_t)next_desc_addr;
        HPDMA_LOGV("Desc[%d] addr=0x%x next_addr=0x%x\r\n", i, curr_desc_addr, curr_desc->next_desc_addr);
    }
    
    // Last descriptor: next_addr = 0 (end of list)
    uintptr_t last_desc_addr = first_desc_addr + (link_cnt - 1) * desc_aligned_size;
    hpdma_descriptor_t *last_desc = (hpdma_descriptor_t *)last_desc_addr;
    last_desc->next_desc_addr = 0;
    HPDMA_LOGV("Desc[%d] addr=0x%x next_addr=0 (end of list)\r\n", link_cnt - 1, last_desc_addr);
    
#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Flush descriptor table to ensure DMA sees latest data after initialization
    // This ensures next_desc_addr is visible to DMA
    flush_dcache((void *)first_desc_addr, link_cnt * desc_aligned_size);
#endif
    
    return (void *)first_desc_addr;
}


void bk_hpdma_link_deinit(void *desc_table)
{
    if (desc_table == NULL) {
        return;
    }
    
    // Get raw pointer stored before aligned address
    void **raw_ptr_storage = (void **)((uint32_t)desc_table - sizeof(void*));
    void *raw_ptr = *raw_ptr_storage;
    
    // Validate: raw_ptr should be within reasonable range
    // (desc_table - 15 - sizeof(void*) to desc_table - sizeof(void*))
    if (raw_ptr != NULL && 
        (uint32_t)raw_ptr >= ((uint32_t)desc_table - 15 - sizeof(void*)) &&
        (uint32_t)raw_ptr < (uint32_t)desc_table) {
        // Free raw pointer (which was allocated with extra space)
        os_free(raw_ptr);
        HPDMA_LOGD("%s freed desc_table=0x%x raw_ptr=0x%x\r\n", __func__, desc_table, raw_ptr);
    } else {
        HPDMA_LOGE("%s invalid raw_ptr: desc_table=0x%x raw_ptr=0x%x\r\n", __func__, desc_table, raw_ptr);
    }
}


bk_err_t bk_hpdma_link_set_desc(void *desc_table, uint32_t index,
                                 const hpdma_link_config_t *config)
{
    if (desc_table == NULL || config == NULL) {
        HPDMA_LOGE("Invalid parameters\r\n");
        return BK_ERR_NULL_PARAM;
    }
    
    // Validate ysize: user input should be >= 1 (1 = 1 row, 2 = 2 rows, etc.)
    if (config->src_ysize == 0) {
        HPDMA_LOGE("Source ysize must be >= 1 (1 = 1 row, 2 = 2 rows, etc.)\r\n");
        return BK_ERR_PARAM;
    }
    if (config->dst_ysize == 0) {
        HPDMA_LOGE("Destination ysize must be >= 1 (1 = 1 row, 2 = 2 rows, etc.)\r\n");
        return BK_ERR_PARAM;
    }
    
    // Get descriptor by index (handles 16-byte alignment spacing)
    hpdma_descriptor_t *desc = hpdma_get_desc_by_index(desc_table, index);
    
    // Configure descriptor for 2D transfer
    // User input: ysize = 1 means 1 row, ysize = 2 means 2 rows, etc.
    // Hardware expects: ysize = 0 means 1 row, ysize = 1 means 2 rows, etc.
    // So we need to decrement by 1 for hardware
    desc->src_addr = config->src_addr;
    desc->dst_addr = config->dst_addr;
    desc->xdim.bits.source_xsize = config->src_xsize;
    desc->xdim.bits.dest_xsize = config->dst_xsize;
    desc->ydim.bits.source_ysize = config->src_ysize - 1;
    desc->ydim.bits.dest_ysize = config->dst_ysize - 1;
    desc->ctrl.bits.source_step = config->src_step;
    desc->ctrl.bits.dest_step = config->dst_step;
    desc->ctrl.bits.int_finish_en = config->finish_int_en;
    desc->ctrl.bits.int_half_finish_en = config->half_finish_int_en;
    
    HPDMA_LOGV("Desc[%d] src=0x%x dst=0x%x src_x=%d src_y=%d dst_x=%d dst_y=%d step_src=%d step_dst=%d\r\n",
               index, config->src_addr, config->dst_addr,
               config->src_xsize, config->src_ysize,
               config->dst_xsize, config->dst_ysize,
               config->src_step, config->dst_step);
    
#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Flush descriptor to ensure DMA sees latest data
    // Note: Source and destination addresses cache will be flushed in bk_hpdma_link_transfer
    // using flush_all_dcache(), so no need to flush them here
    const uint32_t desc_aligned_size = (sizeof(hpdma_descriptor_t) + 15) & ~15;  // 32 bytes
    flush_dcache((void *)desc, desc_aligned_size);
#endif
    return BK_OK;
}


bk_err_t bk_hpdma_link_set_descs(void *desc_table,
                                  const hpdma_link_config_t *configs,
                                  uint32_t link_cnt)
{
    if (desc_table == NULL || configs == NULL) {
        HPDMA_LOGE("Invalid parameters\r\n");
        return BK_ERR_NULL_PARAM;
    }
    
    for (uint32_t i = 0; i < link_cnt; i++) {
        bk_err_t ret = bk_hpdma_link_set_desc(desc_table, i, &configs[i]);
        if (ret != BK_OK) {
            HPDMA_LOGE("Failed to set desc[%d]\r\n", i);
            return ret;
        }
    }
    
    return BK_OK;
}


bk_err_t bk_hpdma_link_transfer(hpdma_id_t id, void *desc_table)
{
    if (desc_table == NULL) {
        HPDMA_LOGE("Invalid descriptor table\r\n");
        return BK_ERR_NULL_PARAM;
    }
    
    if (id >= HPDMA_ID_MAX) {
        HPDMA_LOGE("Invalid DMA channel ID: %d\r\n", id);
        return BK_ERR_HPDMA_ID;
    }
    
    HPDMA_LOGV("%s DMA channel = %d\r\n", __func__, id);

    hpdma_config_t hpdma_config;

    os_memset(&hpdma_config, 0, sizeof(hpdma_config_t));
    
    // Configure DMA for linked list mode (all info in descriptors)
    // Mode 1: Set register values to 0, hardware reads all info from descriptors
    hpdma_config.mode = HPDMA_WORK_MODE_SINGLE;
    hpdma_config.chan_prio = 0;
    hpdma_config.src.dev = HPDMA_DEV_DTCM;
    hpdma_config.src.width = HPDMA_DATA_WIDTH_128BITS;
    hpdma_config.src.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    hpdma_config.src.xsize = 0;
    hpdma_config.src.ysize = 1;  // 1 row (user input: 1 = 1 row, will be decremented in bk_hpdma_init)
    hpdma_config.src.step = 0;
    hpdma_config.src.addr_loop_en = HPDMA_ADDR_LOOP_DISABLE;
    
    hpdma_config.dst.dev = HPDMA_DEV_DTCM;
    hpdma_config.dst.width = HPDMA_DATA_WIDTH_128BITS;
    hpdma_config.dst.addr_inc_en = HPDMA_ADDR_INC_ENABLE;
    hpdma_config.dst.xsize = 0;
    hpdma_config.dst.ysize = 1;  // 1 row (user input: 1 = 1 row, will be decremented in bk_hpdma_init)
    hpdma_config.dst.step = 0;
    hpdma_config.dst.addr_loop_en = HPDMA_ADDR_LOOP_DISABLE;

    bk_hpdma_set_next_ll_addr(id, (uint32_t)desc_table);

    // Initialize and configure DMA
    bk_hpdma_init(id, &hpdma_config);

#if CONFIG_SPE
    bk_hpdma_set_src_sec_attr(id, HPDMA_ATTR_SEC);
    bk_hpdma_set_dest_sec_attr(id, HPDMA_ATTR_SEC);
#endif


//TODO: 12.30 need ASIC verify
#if 0
    bk_hpdma_set_src_burst_len(id, HPDMA_BURST_LEN_INC16);
    bk_hpdma_set_dest_burst_len(id, HPDMA_BURST_LEN_INC16);
#endif

#if CONFIG_SUPPORT_CACHEABLE_SRAM
    // Flush all cache to ensure DMA sees all descriptors and data
    // This is more efficient than traversing all descriptors when link_cnt is large
    // flush_all_dcache() will flush:
    // 1. Descriptor table (already flushed in bk_hpdma_link_init and bk_hpdma_link_set_desc, but flush again for safety)
    // 2. All source addresses (CPU-written data)
    // 3. All destination addresses (prepare for DMA write)
    flush_all_dcache();
#endif

    bk_hpdma_start(id);
    
    HPDMA_LOGV("%s DMA started\r\n", __func__);

    return BK_OK;
}

static void hpdma_isr_common(hpdma_unit_t hpdma_unit_id)
{
    hpdma_hal_t *hal = &s_hpdma.hal;
    uint32_t channel = 0;
    for (int id = 0; id < SOC_HPDMA_CHAN_NUM_PER_UNIT; id++) {
        channel = id + hpdma_unit_id * SOC_HPDMA_CHAN_NUM_PER_UNIT;

        if (hpdma_hal_is_half_finish_interrupt_triggered(hal, id)) {
            HPDMA_LOGV("hpdma_isr HALF FINISH TRIGGERED! id: %d\r\n", id);
            //NOTES:clear intrrupt in condition because maybe multi-core(two CPU) access one DMA
            //it can't cleared peer-side channels status.
            if (s_hpdma_half_finish_isr[id].callback) {
                HPDMA_LOGV("hpdma_isr HALF_finish_isr! id: %d\r\n", id);
                hpdma_hal_clear_half_finish_interrupt_status(hal, id);
                s_hpdma_half_finish_isr[id].callback(channel, s_hpdma_half_finish_isr[id].user_data);
            }
        }
        if (hpdma_hal_is_finish_interrupt_triggered(hal, id)) {
#if CONFIG_SUPPORT_CACHEABLE_SRAM
            flush_all_dcache();
#endif
            HPDMA_LOGV("hpdma_isr ALL FINISH TRIGGERED! id: %d\r\n", id);
            if (s_hpdma_finish_isr[id].callback) {
                HPDMA_LOGV("hpdma_isr ALL_finish_isr! id: %d\r\n", id);
                hpdma_hal_clear_finish_interrupt_status(hal, id);
                s_hpdma_finish_isr[id].callback(channel, s_hpdma_finish_isr[id].user_data);
            }
        }

           if (hpdma_hal_is_bus_err_interrupt_triggered(hal, id)) {
           HPDMA_LOGE("hpdma_isr BUS ERR! id: %d\r\n", id);
           if (s_hpdma_bus_err_isr[id].callback) {
               HPDMA_LOGE("hpdma_isr BUS ERR CALLBACK! id: %d\r\n", id);
               hpdma_hal_clear_finish_interrupt_status(hal, id);
               s_hpdma_bus_err_isr[id].callback(channel, s_hpdma_bus_err_isr[id].user_data);
           }
        }

   }
}

static void hpdma_isr(void)
{
	HPDMA_LOGV("hpdma_isr hw 0\r\n");
	hpdma_isr_common(0);
}