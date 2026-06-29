// Copyright 2025-2026 Beken
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_algorithms/eq_algorithm.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <os/os.h>
#include <components/bk_audio/audio_pipeline/ringbuf.h>
#include <components/bk_audio/audio_utils/debug_dump_util.h>
#include "aud_hal.h"


#define TAG  "EQ_ALGORITHM"

//#define EQ_DEBUG   //GPIO debug

#ifdef EQ_DEBUG

#define EQ_ALGORITHM_START()                       do { GPIO_DOWN(32); GPIO_UP(32);} while (0)
#define EQ_ALGORITHM_END()                         do { GPIO_DOWN(32); } while (0)

#define EQ_PROCESS_START()                         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define EQ_PROCESS_END()                           do { GPIO_DOWN(33); } while (0)

#define EQ_INPUT_START()                           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define EQ_INPUT_END()                             do { GPIO_DOWN(34); } while (0)

#define EQ_OUTPUT_START()                          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define EQ_OUTPUT_END()                            do { GPIO_DOWN(35); } while (0)

#else

#define EQ_ALGORITHM_START()
#define EQ_ALGORITHM_END()

#define EQ_PROCESS_START()
#define EQ_PROCESS_END()

#define EQ_INPUT_START()
#define EQ_INPUT_END()

#define EQ_OUTPUT_START()
#define EQ_OUTPUT_END()

#endif


/* AEC data dump depends on debug utils, so must config CONFIG_ADK_UTILS=y when dump aec data. */
#if CONFIG_ADK_UTILS

//#define EQ_DATA_DUMP

#ifdef EQ_DATA_DUMP

/* dump aec data by uart or tfcard, only choose one */
#define EQ_DATA_DUMP_BY_UART
//#define EQ_DATA_DUMP_BY_TFCARD       /* you must sure CONFIG_FATFS=y */

#ifdef EQ_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static uart_util_handle_t g_eq_uart_util = {0};
#define EQ_DATA_DUMP_UART_ID            (1)
#define EQ_DATA_DUMP_UART_BAUD_RATE     (2000000)
#endif

#ifdef EQ_DATA_DUMP_BY_VFS
#include <components/bk_audio/audio_utils/vfs_util.h>
static struct vfs_util g_eq_vfs_util_in = {0};
static struct vfs_util g_eq_vfs_util_out = {0};

#define EQ_DATA_DUMP_VFS_IN_NAME     "/sd0/eq_in.pcm"
#define EQ_DATA_DUMP_VFS_OUT_NAME    "/sd0/eq_out.pcm"
#endif

#endif  //EQ_DATA_DUMP

#endif  //CONFIG_ADK_UTILS

typedef struct eq_algorithm
{
    eq_cfg_t      eq_cfg;
    eq_handle_t   eq_handle;
    app_eq_load_t eq_load;
    int           eq_mode;
    beken_mutex_t cfg_lock;
} eq_algorithm_t;

#define EQ_HW_DAC_EQ_FILTER_NUM          (10)
#define EQ_HW_DAC_EQ_BPS_BITS            (10)
#define EQ_HW_DAC_EQ_BPS_ALL_ON          ((1u << EQ_HW_DAC_EQ_BPS_BITS) - 1u)
#define EQ_HW_DAC_EQ_COEF_NUM_PER_FILTER (5)

#define EQ_HW_DAC0_EQ_COEF_MEM_BASE_ADDR ((volatile int32_t *)0x41016600)
#define EQ_HW_DAC1_EQ_COEF_MEM_BASE_ADDR ((volatile int32_t *)0x41016800)

static void eq_hw_write_coef_mem(volatile int32_t *base, const eq_algorithm_t *eq, uint32_t filters)
{
    uint32_t i = 0;
    const int32_t b0_identity = (int32_t)(1 << FILTER_COEFS_FRA_BITS);

    for (i = 0; i < EQ_HW_DAC_EQ_FILTER_NUM; i++)
    {
        uint32_t o = i * EQ_HW_DAC_EQ_COEF_NUM_PER_FILTER;

        if (filters > 0 && i < filters)
        {
            eq_para_t const *p = &eq->eq_cfg.eq_para[i];

            base[o + 0] =  p->b[0] * 1024;
            base[o + 1] =  p->b[1] * 1024;
            base[o + 2] =  p->b[2] * 1024;
            base[o + 3] = -p->a[0] * 1024;
            base[o + 4] = -p->a[1] * 1024;
        }
        else
        {
            base[o + 0] = 0;
            base[o + 1] = 0;
            base[o + 2] = b0_identity;
            base[o + 3] = 0;
            base[o + 4] = 0;
        }
    }
}

static void eq_hw_apply(eq_algorithm_t *eq)
{
    uint32_t bps_mask;
    uint32_t filters = (uint32_t)eq->eq_cfg.eq_valid_num;

    if (filters > EQ_HW_DAC_EQ_FILTER_NUM)
    {
        BK_LOGW(TAG, "HW EQ: cap filters %u to %d\n", filters, EQ_HW_DAC_EQ_FILTER_NUM);
        filters = EQ_HW_DAC_EQ_FILTER_NUM;
    }

    if (filters == 0)
    {
        bps_mask = EQ_HW_DAC_EQ_BPS_ALL_ON;
    }
    else
    {
        bps_mask = EQ_HW_DAC_EQ_BPS_ALL_ON & ~((1u << filters) - 1u);
    }

    BK_LOGD(TAG, "HW EQ apply: eq_valid_num=%u filters=%u bps_mask=0x%x\n",
            (unsigned)eq->eq_cfg.eq_valid_num, (unsigned)filters, (unsigned)bps_mask);

    audio_reg_hal_set_sys_cfg_mem_dac_eq_sw_init(1);
    eq_hw_write_coef_mem(EQ_HW_DAC0_EQ_COEF_MEM_BASE_ADDR, eq, filters);
    eq_hw_write_coef_mem(EQ_HW_DAC1_EQ_COEF_MEM_BASE_ADDR, eq, filters);
    audio_reg_hal_set_sys_cfg_mem_dac_eq_sw_init(0);

    audio_reg_hal_set_dac_cfg1_dac_eq_bps(bps_mask);

}


#ifdef EQ_DATA_DUMP

static void eq_data_dump_open(void)
{
#ifdef EQ_DATA_DUMP_BY_UART
    uart_util_create(&g_eq_uart_util,EQ_DATA_DUMP_UART_ID, EQ_DATA_DUMP_UART_BAUD_RATE);
#endif

#ifdef EQ_DATA_DUMP_BY_VFS
    vfs_util_create(&g_eq_vfs_util_in, EQ_DATA_DUMP_VFS_IN_NAME);
    vfs_util_create(&g_eq_vfs_util_out, EQ_DATA_DUMP_VFS_OUT_NAME);
#endif
}

static void eq_data_dump_close(void)
{
#ifdef EQ_DATA_DUMP_BY_UART
    uart_util_destroy(&g_eq_uart_util);
    g_eq_uart_util = NULL;
#endif

#ifdef EQ_DATA_DUMP_BY_VFS
    vfs_util_destroy(&g_eq_vfs_util_in);
    vfs_util_destroy(&g_eq_vfs_util_out);
#endif
}

static void eq_data_dump_in_data(void *data_buf, uint32_t len)
{
#ifdef EQ_DATA_DUMP_BY_UART
    uart_util_tx_data(&g_eq_uart_util, data_buf, len);
#endif

#ifdef EQ_DATA_DUMP_BY_VFS
    vfs_util_tx_data(&g_eq_vfs_util_in, data_buf, len);
#endif
}

static void eq_data_dump_out_data(void *data_buf, uint32_t len)
{
#ifdef EQ_DATA_DUMP_BY_UART
    uart_util_tx_data(g_eq_uart_util, data_buf, len);
#endif

#ifdef AEC_DATA_DUMP_BY_VFS
    vfs_util_tx_data(&g_eq_vfs_util_out, data_buf, len);
#endif
}

#define EQ_DATA_DUMP_OPEN()                        eq_data_dump_open()
#define EQ_DATA_DUMP_CLOSE()                       eq_data_dump_close()
#define EQ_DATA_DUMP_IN_DATA(data_buf, len)        eq_data_dump_in_data(data_buf, len)
#define EQ_DATA_DUMP_OUT_DATA(data_buf, len)       eq_data_dump_out_data(data_buf, len)

#else

#define EQ_DATA_DUMP_OPEN()
#define EQ_DATA_DUMP_CLOSE()
#define EQ_DATA_DUMP_IN_DATA(data_buf, len)
#define EQ_DATA_DUMP_OUT_DATA(data_buf, len)

#endif  //EQ_DATA_DUMP

static bk_err_t _eq_algorithm_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);
    eq_algorithm_t *eq = (eq_algorithm_t *)audio_element_getdata(self);

    if (eq->eq_mode == EQ_MODE_SOFTWARE)
    {
        eq->eq_handle = eq_create(&eq->eq_cfg);

        if (!eq->eq_handle)
        {
            BK_LOGE(TAG, "%s, %d, eq element create fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    return BK_OK;
}

static bk_err_t _eq_algorithm_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);
    return BK_OK;
}

static int _eq_algorithm_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] %s, in_len: %d \n", audio_element_get_tag(self), __func__, in_len);
    eq_algorithm_t *eq = (eq_algorithm_t *)audio_element_getdata(self);

    EQ_PROCESS_START();

    EQ_INPUT_START();
    int r_size = audio_element_input(self, in_buffer, in_len);
    BK_LOGV(TAG, "[%s] r_size=%d, in_len=%d \n",audio_element_get_tag(self), r_size, in_len);
    if (r_size != in_len)
    {
        BK_LOGE(TAG, "eq input warning: r_size=%d, in_len=%d \n", r_size, in_len);
    }

    EQ_INPUT_END();

    int w_size = 0;
    if (r_size > 0)
    {
        EQ_DATA_DUMP_IN_DATA(in_buffer, r_size);
        if(is_aud_dump_valid(DUMP_TYPE_EQ_IN_DATA))
        {
            /*update header*/
            DEBUG_DATA_DUMP_UPDATE_HEADER_DUMP_FILE_TYPE(DUMP_TYPE_EQ_IN_DATA, 0, DUMP_FILE_TYPE_PCM);
            DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(DUMP_TYPE_EQ_IN_DATA, 0, r_size);
            DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(DUMP_TYPE_EQ_IN_DATA);

            /*dump data function is called by multi-thread,need suspend task scheduler until data dump finished*/
            DEBUG_DATA_DUMP_SUSPEND_ALL;

            /*dump header*/
            DEBUG_DATA_DUMP_BY_UART_HEADER(DUMP_TYPE_EQ_IN_DATA);

            /*dump data*/
            DEBUG_DATA_DUMP_BY_UART_DATA(in_buffer, r_size);
            DEBUG_DATA_DUMP_RESUME_ALL;

            /*update seq*/
            DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(DUMP_TYPE_EQ_IN_DATA);
        }
        EQ_ALGORITHM_START();

        rtos_lock_mutex(&eq->cfg_lock);
        if (eq->eq_mode == EQ_MODE_HARDWARE)
        {
            /* EQ in DAC hardware; pipeline only passes data */
        }
        else
        {
            eq_process(eq->eq_handle, (int16_t *)in_buffer, r_size / 2);
        }
        rtos_unlock_mutex(&eq->cfg_lock);

        EQ_ALGORITHM_END();

        EQ_DATA_DUMP_OUT_DATA(in_buffer, r_size);

        if(is_aud_dump_valid(DUMP_TYPE_EQ_OUT_DATA))
        {
            /*update header*/
            DEBUG_DATA_DUMP_UPDATE_HEADER_DUMP_FILE_TYPE(DUMP_TYPE_EQ_OUT_DATA, 0, DUMP_FILE_TYPE_PCM);
            DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(DUMP_TYPE_EQ_OUT_DATA, 0, r_size);
            DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(DUMP_TYPE_EQ_OUT_DATA);

            /*dump data function is called by multi-thread,need suspend task scheduler until data dump finished*/
            DEBUG_DATA_DUMP_SUSPEND_ALL;

            /*dump header*/
            DEBUG_DATA_DUMP_BY_UART_HEADER(DUMP_TYPE_EQ_OUT_DATA);

            /*dump data*/
            DEBUG_DATA_DUMP_BY_UART_DATA(in_buffer, r_size);
            DEBUG_DATA_DUMP_RESUME_ALL;

            /*update seq*/
            DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(DUMP_TYPE_EQ_OUT_DATA);
        }

        EQ_OUTPUT_START();
        w_size = audio_element_output(self, in_buffer, r_size);
        EQ_OUTPUT_END();

        /* write data to multiple audio port */
        /* unblock write, and not check write result */
        //TODO
        audio_element_multi_output(self, in_buffer, r_size, 0);
    }
    else
    {
        w_size = r_size;
    }

    EQ_PROCESS_END();
    BK_LOGV(TAG, "[%s] w_size=%d\n",audio_element_get_tag(self), w_size);

    return w_size;
}

static bk_err_t _eq_algorithm_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] %s \n", audio_element_get_tag(self), __func__);

    eq_algorithm_t *eq = (eq_algorithm_t *)audio_element_getdata(self);

    if (eq)
    {
        if (eq->eq_handle)
        {
            eq_destroy(eq->eq_handle);
        }
        if (eq->cfg_lock)
        {
            rtos_deinit_mutex(&eq->cfg_lock);
        }
        audio_free(eq);
    }

    EQ_DATA_DUMP_CLOSE();

    return BK_OK;
}

audio_element_handle_t eq_algorithm_init(eq_algorithm_cfg_t *config)
{
    audio_element_handle_t el;

    /* check config */
    if (!config->eq_cal_para.eq_en)
    {
        BK_LOGE(TAG, "check config->eq_cal_para.eq_en:%d fail \n",config->eq_cal_para.eq_en);
        return NULL;
    }

    eq_algorithm_t *eq_alg = audio_calloc(1, sizeof(eq_algorithm_t));
    AUDIO_MEM_CHECK(TAG, eq_alg, return NULL);

    if (rtos_init_mutex(&eq_alg->cfg_lock) != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, eq cfg_lock create fail\n", __func__, __LINE__);
        audio_free(eq_alg);
        return NULL;
    }

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open  = _eq_algorithm_open;
    cfg.close = _eq_algorithm_close;
    cfg.seek  = NULL;
    cfg.process = _eq_algorithm_process;
    cfg.destroy = _eq_algorithm_destroy;
    cfg.in_type = PORT_TYPE_RB;
    cfg.read    = NULL;
    cfg.out_type = PORT_TYPE_RB;
    cfg.write    = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio  = config->task_prio;
    cfg.task_core  = config->task_core;

    cfg.out_block_size     = config->eq_frame_size * config->eq_chl_num;
    cfg.out_block_num      = config->out_block_num;
    cfg.multi_out_port_num = config->multi_out_port_num;
    cfg.buffer_len         = config->eq_frame_size * config->eq_chl_num;

    cfg.tag = "eq_algorithm";
    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _eq_algorithm_init_exit);

    /* config */
    eq_alg->eq_mode             = config->eq_mode;
    eq_alg->eq_cfg.chl_num      = config->eq_chl_num;
    eq_alg->eq_cfg.eq_gain      = config->eq_cal_para.globle_gain;
    eq_alg->eq_cfg.eq_valid_num = config->eq_cal_para.filters;
    os_memcpy(&eq_alg->eq_cfg.eq_para ,&config->eq_cal_para.eq_para,sizeof(eq_para_t)*config->eq_cal_para.filters);
    os_memcpy(&eq_alg->eq_load, &config->eq_cal_para.eq_load, sizeof(app_eq_load_t));
    if (eq_alg->eq_mode == EQ_MODE_HARDWARE)
    {
        eq_hw_apply(eq_alg);
    }

    audio_element_setdata(el, eq_alg);

    EQ_DATA_DUMP_OPEN();

    return el;
_eq_algorithm_init_exit:
    if (eq_alg->cfg_lock)
    {
        rtos_deinit_mutex(&eq_alg->cfg_lock);
    }
    audio_free(eq_alg);
    return NULL;
}

bk_err_t eq_algorithm_set_config(audio_element_handle_t eq_algorithm, void * eq_config)
{
    if (eq_algorithm == NULL || eq_config == NULL)
    {
        BK_LOGE(TAG, "%s, %d, NULL param (handle:%p, config:%p)\n", __func__, __LINE__, (void *)eq_algorithm, eq_config);
        return BK_FAIL;
    }

    eq_algorithm_t *eq = (eq_algorithm_t *)audio_element_getdata(eq_algorithm);
    app_aud_eq_config_t *eq_cfg = (app_aud_eq_config_t *)eq_config;

    if (eq == NULL)
    {
        BK_LOGE(TAG, "%s, %d, eq is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* clamp the requested band count to eq_para[] capacity; src and dst arrays are
     * both CON_AUD_EQ_BANDS deep, so a larger filters would overrun both buffers */
    uint32_t filters = eq_cfg->filters;
    if (filters > CON_AUD_EQ_BANDS)
    {
        BK_LOGW(TAG, "%s, %d, filters(%u) > max(%d), clamp\n", __func__, __LINE__, (unsigned int)filters, CON_AUD_EQ_BANDS);
        filters = CON_AUD_EQ_BANDS;
    }

    /* serialize live cfg/handle update against the element task's eq_process()/use */
    rtos_lock_mutex(&eq->cfg_lock);
    eq->eq_cfg.eq_gain      = eq_cfg->globle_gain;
    eq->eq_cfg.eq_valid_num = filters;

    os_memcpy(&eq->eq_cfg.eq_para, &eq_cfg->eq_para, sizeof(eq_para_t)*filters);
    os_memcpy(&eq->eq_load, &eq_cfg->eq_load, sizeof(app_eq_load_t));
    if (eq->eq_mode == EQ_MODE_SOFTWARE)
    {
        eq_destroy(eq->eq_handle);
        eq->eq_handle = eq_create(&eq->eq_cfg);
        if (!eq->eq_handle)
        {
            BK_LOGE(TAG, "%s, %d, eq element create fail\n", __func__, __LINE__);
            rtos_unlock_mutex(&eq->cfg_lock);
            return BK_FAIL;
        }
    } else
    {
        eq_hw_apply(eq);
    }

    audio_element_setdata(eq_algorithm, eq);
    rtos_unlock_mutex(&eq->cfg_lock);
    return BK_OK;
}

bk_err_t eq_algorithm_get_config(audio_element_handle_t eq_algorithm, void * eq_load)
{
    if (eq_algorithm == NULL || eq_load == NULL)
    {
        BK_LOGE(TAG, "%s, %d, NULL param (handle:%p, load:%p)\n", __func__, __LINE__, (void *)eq_algorithm, eq_load);
        return BK_FAIL;
    }

    eq_algorithm_t *eq = (eq_algorithm_t *)audio_element_getdata(eq_algorithm);
    if (eq == NULL)
    {
        BK_LOGE(TAG, "%s, %d, eq is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    rtos_lock_mutex(&eq->cfg_lock);
    os_memcpy(eq_load, &eq->eq_load, sizeof(app_eq_load_t));
    rtos_unlock_mutex(&eq->cfg_lock);
    return BK_OK;
}
