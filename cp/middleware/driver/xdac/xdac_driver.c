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
#include <os/os.h>
#include <os/mem.h>
#include "xdac_driver.h"
#include "xdac_hal.h"
#include "icu_driver.h"
#include "power_driver.h"
#include <components/system.h>
#include "sys_driver.h"
#include <driver/timer.h>
#include <gpio_driver.h>
#include <driver/dma.h>

#if CONFIG_XDAC

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static SPINLOCK_SECTION volatile spinlock_t xdac_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP

typedef struct {
    xdac_hal_t hal;
    uint16_t is_occupied;
} xdac_driver_t;

#define XDAC_RETURN_ON_DRIVER_NOT_INIT() do {\
            if (false == xdac_init_flag)\
            {\
                XDAC_LOGE("%s,line:%d,XDAC driver not init!\n",__func__,__LINE__);\
                return BK_ERR_XDAC_DRVER_NOT_INIT;\
            }\
        } while(0)

#define XDAC_RETURN_ON_CH_INVALID(ch) do {\
            if((XDAC_0 != ch) && (XDAC_1 != ch))\
            {\
                XDAC_LOGE("%s,line:%d ch:%d is invalid!\r\n",__func__,__LINE__,ch);\
                return BK_ERR_XDAC_CH_INVALID;\
            }\
        } while(0)

#define GPIO_XDAC0 (GPIO_39)
#define GPIO_XDAC1 (GPIO_38)

xdac_driver_t s_xdac[XDAC_CH_CNT] = {0};
bool xdac_init_flag = false;

void __BK_IRQ xdac0_isr(void);
void __BK_IRQ xdac1_isr(void);

xdac_config_t xdac_default_cfg[XDAC_CH_CNT] = 
{
    {
        .ch = XDAC_0,
        .dac_wthrd = 14,
        .dac_rthrd = 4,
        .dac_clk_div = 1624,//sample rate = ref freq/(div + 1) = 26m/(1624+1) = 16000
        .xdac_isr = xdac0_isr,
    },
    {
        .ch = XDAC_1,
        .dac_wthrd = 14,
        .dac_rthrd = 4,
        .dac_clk_div = 3249,//sample rate = ref freq/(div + 1) = 26m/(3249+1) = 8000
        .xdac_isr = xdac1_isr,
    },
};

static inline uint32_t xdac_enter_critical(void)
{
	uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
	spin_lock(&xdac_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	return flags;
}

static inline void xdac_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
	spin_unlock(&xdac_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	rtos_enable_int(flags);
}
            
void __BK_IRQ xdac0_isr(void)
{
    if(xdac_hal_get_fifo_empty_int(0) && xdac_hal_get_fifo_empty_int_en(0))
    {
        //XDAC_LOGE("xdac0 fifo empty!\n");
    }

    if(xdac_hal_get_fifo_full_int(0) && xdac_hal_get_fifo_full_int_en(0))
    {
        //XDAC_LOGE("xdac0 fifo full!\n");
    }

    if(xdac_hal_get_fifo_near_full_int(0) && xdac_hal_get_fifo_near_full_int_en(0))
    {
        //XDAC_LOGD("xdac0 fifo near full!\n");
    }

    if(xdac_hal_get_fifo_near_empty_int(0) && xdac_hal_get_fifo_near_empty_int_en(0))
    {
        //XDAC_LOGD("xdac0 fifo near full!\n");
    }
}

void __BK_IRQ xdac1_isr(void)
{
    if(xdac_hal_get_fifo_empty_int(1) && xdac_hal_get_fifo_empty_int_en(1))
    {
       // XDAC_LOGE("xdac0 fifo empty!\n");
    }

    if(xdac_hal_get_fifo_full_int(1) && xdac_hal_get_fifo_full_int_en(1))
    {
       // XDAC_LOGE("xdac0 fifo full!\n");
    }

    if(xdac_hal_get_fifo_near_full_int(1) && xdac_hal_get_fifo_near_full_int_en(1))
    {
      //  XDAC_LOGD("xdac0 fifo near full!\n");
    }

    if(xdac_hal_get_fifo_near_empty_int(1) && xdac_hal_get_fifo_near_empty_int_en(1))
    {
      //  XDAC_LOGD("xdac0 fifo near full!\n");
    }
}

bk_err_t bk_xdac_get_cfg(uint8_t ch, xdac_config_t *cfg)
{
    bk_err_t ret = BK_OK;

    XDAC_RETURN_ON_DRIVER_NOT_INIT();

    if(!cfg)
    {
        XDAC_LOGE("%s,line:%d,cfg:%d is NULL!\n", __func__, __LINE__, cfg);
        return BK_ERR_XDAC_PTR_IS_INVALID;
    }

    XDAC_RETURN_ON_CH_INVALID(ch);

    cfg->ch = ch;
    cfg->dac_clk_div = xdac_hal_get_dac_clk_div(ch);    
    cfg->dac_rthrd = xdac_hal_get_dac_rthrd(ch);
    cfg->dac_wthrd = xdac_hal_get_dac_rthrd(ch);

    return ret;
}

void bk_xdac_clr_int_status(uint8_t ch)
{
    xdac_hal_get_fifo_empty_int(ch);
    xdac_hal_get_fifo_full_int(ch);
    xdac_hal_get_fifo_near_full_int(ch);
    xdac_hal_get_fifo_near_empty_int(ch);
}

bk_err_t bk_xdac_set_cfg(xdac_config_t *cfg)
{
    bk_err_t ret = BK_OK;

    if(!cfg)
    {
        XDAC_LOGE("%s,line:%d,cfg:%d is NULL!\n", __func__, __LINE__, cfg);
        return BK_ERR_XDAC_PTR_IS_INVALID;
    }

    XDAC_RETURN_ON_CH_INVALID(cfg->ch);
    
    xdac_hal_set_soft_reset(cfg->ch,1);//sw_reset
    ret = xdac_hal_set_dac_clk_div(cfg->ch, cfg->dac_clk_div);
    
    XDAC_LOGI("%s,line:%d,ch%d cfg->div:%d,dac_clk_div:%d!\n",
        __func__, __LINE__,
        cfg->ch, cfg->dac_clk_div, xdac_hal_get_dac_clk_div(cfg->ch));

    xdac_hal_set_dac_rthrd(cfg->ch,cfg->dac_rthrd);
    xdac_hal_set_dac_wthrd(cfg->ch,cfg->dac_wthrd);

    return ret;
}

bk_err_t bk_xdac_set_sample_rate(uint8_t ch, uint32_t sample_rate)
{
    uint16_t clk_div;
    clk_div = 26000000 / sample_rate - 1;
    XDAC_LOGI("%s,line:%d,ch%d sample_rate:%d,clk_div:%d!\n",
    __func__, __LINE__, ch, sample_rate, clk_div);
    bk_err_t ret = xdac_hal_set_dac_clk_div(ch, clk_div);

    return ret;
}

bk_err_t bk_xdac_driver_init(void)
{
    uint32_t i;
    uint8_t int_map[XDAC_CH_CNT] = {INT_SRC_XDAC0,INT_SRC_XDAC1};
#if CONFIG_USR_GPIO_CFG_EN
    gpio_id_t gpio_map[XDAC_CH_CNT] = {GPIO_XDAC0,GPIO_XDAC1};
#endif

    if(true == xdac_init_flag)
    {
        return BK_OK;
    }

    #if CONFIG_PM_ENABLE
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_XDAC0, PM_POWER_MODULE_STATE_ON);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_XDAC1, PM_POWER_MODULE_STATE_ON);
    #endif

    for(i = 0; i < XDAC_CH_CNT; i++)
    {
        os_memset(&s_xdac[i], 0, sizeof(xdac_driver_t));
        xdac_hal_init(i, &s_xdac[i].hal);

        /*gpio map,set to high-z state*/
#if CONFIG_USR_GPIO_CFG_EN
        gpio_dev_map(gpio_map[i], GPIO_DEV_NONE);
#endif
        sys_hal_xdac_set_enspi(i,1);
        sys_hal_xdac_set_endigspi_sel(i,1);

        /*initialize with default config*/
        bk_xdac_set_cfg(&xdac_default_cfg[i]);
        
        /*register & enable xdac interrupt*/
        bk_xdac_clr_int_status(i);
        bk_int_isr_register(int_map[i], xdac_default_cfg[i].xdac_isr, NULL);
        sys_hal_xdac_int_en(0,i,1);//enable xdac interrupt on cpu0
        
        s_xdac[i].is_occupied = 0;
    }

    xdac_init_flag = true;

    return BK_OK;
}

bk_err_t bk_xdac_driver_deinit(void)
{
    uint8_t int_map[XDAC_CH_CNT] = {INT_SRC_XDAC0,INT_SRC_XDAC1};
    uint32_t i;

    if(false == xdac_init_flag)
    {
        return BK_OK;
    }
    
    for(i = 0; i < XDAC_CH_CNT; i++)
    {
        if(s_xdac[i].is_occupied)
        {
            XDAC_LOGE("%s,line:%d,xdac[%d] is occupied,force released by xdac drv deinit!\n", __func__, __LINE__,i);
        }

        bk_xdac_deinit(i);
        sys_hal_xdac_int_en(0,i,0);//disable xdac interrupt on cpu0
        bk_int_isr_unregister(int_map[i]);

        s_xdac[i].is_occupied = 0;
    }

    #if CONFIG_PM_ENABLE
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_XDAC0, PM_POWER_MODULE_STATE_OFF);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_XDAC1, PM_POWER_MODULE_STATE_OFF);
    #endif

    xdac_init_flag = false;
    return BK_OK;
}

bk_err_t bk_xdac_start(uint8_t ch)
{
    XDAC_RETURN_ON_DRIVER_NOT_INIT();
    XDAC_RETURN_ON_CH_INVALID(ch);

    sys_hal_xdac_clk_en(ch,1);
    xdac_hal_set_dac_clk_en(ch,1);
    xdac_hal_set_fifo_enable(ch,1);
    
    xdac_hal_set_dac_enable(ch,1);

    return BK_OK;
}

bk_err_t bk_xdac_stop(uint8_t ch)
{
    XDAC_RETURN_ON_DRIVER_NOT_INIT();
    XDAC_RETURN_ON_CH_INVALID(ch);
    
    xdac_hal_set_dac_enable(ch,0);
    xdac_hal_set_fifo_enable(ch,0);
    xdac_hal_set_dac_clk_en(ch,0);
    sys_hal_xdac_clk_en(ch,0);
    return BK_OK;
}

bk_err_t bk_xdac_deinit(uint8_t ch)
{
    XDAC_RETURN_ON_DRIVER_NOT_INIT();
    XDAC_RETURN_ON_CH_INVALID(ch);

    bk_xdac_set_int_en(ch, XDAC_EMPTY_INT, 0);
    bk_xdac_set_int_en(ch, XDAC_FULL_INT, 0);
    bk_xdac_set_int_en(ch, XDAC_NEAR_FULL_INT, 0);
    bk_xdac_set_int_en(ch, XDAC_NEAR_EMPTY_INT, 0);
    bk_xdac_stop(ch);
    bk_xdac_clr_int_status(ch);
    bk_xdac_free(ch);
    return BK_OK;
}

bk_err_t bk_xdac_update_cfg(xdac_config_t *cfg)
{
    bk_err_t ret = BK_OK;

    XDAC_RETURN_ON_DRIVER_NOT_INIT();

    if(!cfg)
    {
        XDAC_LOGE("%s,line:%d,cfg:%d is NULL!\n", __func__, __LINE__, cfg);
        return BK_ERR_XDAC_PTR_IS_INVALID;
    }

    XDAC_RETURN_ON_CH_INVALID(cfg->ch);

    bk_xdac_stop(cfg->ch);
    bk_xdac_set_cfg(cfg);
    bk_xdac_start(cfg->ch);

    return ret;
}

bk_err_t bk_xdac_alloc(xdac_ch_t *ch)
{
    uint32_t i;
    bk_err_t ret = BK_FAIL;
    u32 int_mask;
    
    XDAC_RETURN_ON_DRIVER_NOT_INIT();

    *ch = 0;

    int_mask = xdac_enter_critical();
        
    for(i = 0; i < XDAC_CH_CNT; i++)
    {
        if(!s_xdac[i].is_occupied)
        {
            *ch = i;
            s_xdac[i].is_occupied = 1;
            ret = BK_OK;
            break;
        }
    }

    xdac_exit_critical(int_mask);

    return ret;
}


bk_err_t bk_xdac_free(xdac_ch_t ch)
{
    bk_err_t ret = BK_FAIL;
    u32 int_mask;

    XDAC_RETURN_ON_DRIVER_NOT_INIT();
    XDAC_RETURN_ON_CH_INVALID(ch);

    int_mask = xdac_enter_critical();
    s_xdac[ch].is_occupied = 0;
    xdac_exit_critical(int_mask);

    return ret;
}

bk_err_t bk_xdac_get_fifo_addr(uint8_t ch, uint32_t *fifo_addr)
{
    XDAC_RETURN_ON_CH_INVALID(ch);
    *fifo_addr = xdac_hal_get_fifo_addr(ch);
    return BK_OK;
}

bk_err_t bk_xdac_set_int_en(uint8_t ch, uint8_t int_type, uint8_t en)
{
    XDAC_RETURN_ON_CH_INVALID(ch);
    return xdac_hal_set_int_en(ch, int_type, en);
}

bool bk_xdac_is_driver_inited(void)
{
    return xdac_init_flag;
}

#endif // CONFIG_XDAC

