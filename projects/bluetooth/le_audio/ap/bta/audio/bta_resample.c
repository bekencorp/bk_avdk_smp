#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <driver/timer.h>

#include "FreeRTOS.h"
#include "event_groups.h"


#include <driver/uart.h>
#include "gpio_driver.h"

#include "bk_ring_buffer_node.h"
#include "bta_audio.h"
#include "bluetooth_config.h"
#include "bluetooth_config.h"
#include "bta_resample.h"

#if BTA_RESAMPLE_USE_AUDIO_RSP
#define RESAMPLE_V1 (1)
#else
#define RESAMPLE_V1 (0)
#endif


#if UART_DUMP
#include <driver/uart.h>
#include "gpio_driver.h"
#endif

#if BTA_RESAMPLE_USE_AUDIO_RSP
#include <modules/audio_rsp_types.h>
#include <modules/audio_rsp.h>
#else
#include <modules/src.h>
#endif



#define TAG "resample"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define EVT_EXIT_IND        (1 << 0)
#define EVT_PCM_DATA_IND    (1 << 8)
#define WAITTING_EVENTS     (EVT_PCM_DATA_IND)

typedef struct
{
    uint8_t *ring_buffer;
    bk_ring_buffer_octets_context_t context;
    uint32_t input_size;
    uint32_t output_size;
    uint8_t *resample_buffer;
    uint8_t *input_buffer;
    uint8_t *output_buffer;
    EventGroupHandle_t event;
    beken_thread_t hanndle;
    beken_mutex_t ctx_lock;
    uint8_t running;
    bta_resample_data_cb cb;
    uint32_t channels;
} bta_resample_info_t;


static bta_resample_info_t resample_info = {0};

static void bta_resample_task_entry(void *arg)
{
    bk_err_t ret;
    int size;

    xEventGroupClearBits(resample_info.event, EVT_EXIT_IND);

    while (resample_info.running == true)
    {
        EventBits_t bits = xEventGroupWaitBits(resample_info.event, WAITTING_EVENTS, true, false, 300 / bk_get_ms_per_tick());

        if (bits & EVT_PCM_DATA_IND)
        {
#if RESAMPLE_V1
            uint32_t input_samples = resample_info.input_size / 2;
#else
            uint32_t input_samples = resample_info.input_size / resample_info.channels / 2;
#endif
            uint32_t output_samples = resample_info.output_size / 2;

            rtos_lock_mutex(&resample_info.ctx_lock);
            size = bk_ring_buffer_octets_len(&resample_info.context);
            rtos_unlock_mutex(&resample_info.ctx_lock);

            while (size >= resample_info.input_size && resample_info.running == true)
            {
                rtos_lock_mutex(&resample_info.ctx_lock);
                ret = bk_ring_buffer_octets_read(&resample_info.context, resample_info.input_buffer, resample_info.input_size);
                rtos_unlock_mutex(&resample_info.ctx_lock);

                if (ret != resample_info.input_size)
                {
                    LOGE("error reald length: %d, %d\n", ret, resample_info.input_size);
                    break;
                }

                //GPIO_DOWN(34); GPIO_UP(34);

#if RESAMPLE_V1
                ret = bk_aud_rsp_process((int16_t *)resample_info.input_buffer, &input_samples, (int16_t *)resample_info.output_buffer, &output_samples);

                if (ret)
                {
                    LOGE("bk_aud_rsp_process err %d !!", ret);
                    break;
                }

                //LOGI("resample success length: %d->%d\n", input_len, output_len);
#else
                ret = src_exec_ex((SRCContext *)resample_info.resample_buffer,
                                  (void *)resample_info.input_buffer,
                                  16,
                                  (void *)resample_info.output_buffer,
                                  16,
                                  input_samples);

                if (ret <= 0 || ret > resample_info.output_size)
                {
                    LOGE("src_exec_ex failed, ret: %d\n", ret);
                    break;
                }

                output_samples = ret * resample_info.channels;
                //LOGI("resample success length: %d->%d\n", input_samples * 4, output_samples * 4);
#endif

#if UART_DUMP
                bk_uart_write_bytes(UART_ID_2, resample_info.output_buffer, ret);
#endif

                if (resample_info.cb)
                {
                    resample_info.cb(resample_info.output_buffer, output_samples * 2);
                }

                //GPIO_DOWN(34);
                size -= resample_info.input_size;
            }
        }
        else
        {
            //TODO
        }
    }

    xEventGroupSetBits(resample_info.event, EVT_EXIT_IND);

    rtos_delete_thread(NULL);

}

void bta_resample_write(uint8 *data, uint32_t length)
{
    int ret;

    rtos_lock_mutex(&resample_info.ctx_lock);
    ret = bk_ring_buffer_octets_write(&resample_info.context, data, length);
    rtos_unlock_mutex(&resample_info.ctx_lock);

    if (ret < 0)
    {
        LOGE("%s error\n", __func__);
        return;
    }

#if UART_DUMP
    //bk_uart_write_bytes(UART_ID_2, data, length);
#endif

    xEventGroupSetBits(resample_info.event, EVT_PCM_DATA_IND);
}

#if UART_DUMP
static void uart_dump_init(uart_id_t id, uint32_t baud_rate)
{
    uart_config_t config = {0};

    os_memset(&config, 0, sizeof(uart_config_t));

    if (id == 0)
    {
        gpio_dev_unmap(GPIO_10);
        gpio_dev_map(GPIO_10, GPIO_DEV_UART1_RXD);
        gpio_dev_unmap(GPIO_11);
        gpio_dev_map(GPIO_11, GPIO_DEV_UART1_TXD);
    }
    else if (id == 2)
    {
        gpio_dev_unmap(GPIO_40);
        gpio_dev_map(GPIO_40, GPIO_DEV_UART3_RXD);
        gpio_dev_unmap(GPIO_41);
        gpio_dev_map(GPIO_41, GPIO_DEV_UART3_TXD);
    }
    else
    {
        gpio_dev_unmap(GPIO_0);
        gpio_dev_map(GPIO_0, GPIO_DEV_UART2_TXD);
        gpio_dev_unmap(GPIO_1);
        gpio_dev_map(GPIO_1, GPIO_DEV_UART2_RXD);
    }

    config.baud_rate = baud_rate;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_NONE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_FLOWCTRL_DISABLE;
    config.src_clk = UART_SCLK_XTAL_26M;

    if (bk_uart_init(id, &config) != BK_OK)
    {
        LOGE("init uart fail \r\n");
    }
    else
    {
        LOGE("init uart ok \r\n");
    }
}
#endif

void bta_resample_buffer_init(bta_resample_config_t *config)
{
    int ret;

    resample_info.input_size = ((config->src_sample * config->duration * config->channels * 2) / 1000) / 1000;
    resample_info.output_size = ((config->dst_sample * config->duration * config->channels * 2) / 1000) / 1000;

    resample_info.input_buffer = os_malloc(resample_info.input_size);
    resample_info.output_buffer = os_malloc(resample_info.output_size);

    resample_info.channels = config->channels;

#if RESAMPLE_V1
    const aud_rsp_cfg_t cfg =
    {
        .src_rate = config->src_sample,
        .src_ch = config->channels,
        .src_bits = 16,
        .dest_rate = config->dst_sample,
        .dest_ch = config->channels,
        .dest_bits = 16,
        .complexity = 0,
        .down_ch_idx = 0,
    };

    ret = bk_aud_rsp_init(cfg);

    if (ret != BK_OK)
    {
        LOGE("bk_aud_rsp_init failed\n");
    }

#else
#define BLOCK_SIZE    (128)

    int id = 0, buffer_size;

    if (config->src_sample == 44100 && config->dst_sample == 48000)
    {
        id = SRC_ID_U160D147;
    }
    else
    {
        LOGE("%s invalid config\n", __func__);
    }

    buffer_size = src_size(id, BLOCK_SIZE, config->channels);

    LOGI("resample buffer: %d, input: %d, output: %d\n", buffer_size, resample_info.input_size, resample_info.output_size);

    resample_info.resample_buffer = os_malloc(buffer_size);

    if (resample_info.resample_buffer == NULL)
    {
        LOGE("resample_buffer malloc failed\n");
        return;
    }

    ret = src_init((SRCContext *)resample_info.resample_buffer, id, BLOCK_SIZE, config->channels);

    if (ret != BK_OK)
    {
        LOGE("src_init failed\n");
    }
#endif

    resample_info.ring_buffer = os_malloc(40 * 1024);
    bk_ring_buffer_octets_init(&resample_info.context, resample_info.ring_buffer, 40 * 1024);

#if UART_DUMP
    uart_dump_init(UART_ID_2, 2000000);
#endif
}

void bta_resample_buffer_deinit(void)
{
    os_free(resample_info.input_buffer);
    resample_info.input_buffer = NULL;

    os_free(resample_info.output_buffer);
    resample_info.output_buffer = NULL;

    os_free(resample_info.resample_buffer);
    resample_info.resample_buffer = NULL;

    bk_ring_buffer_octets_deinit(&resample_info.context);

    os_free(resample_info.ring_buffer);
    resample_info.ring_buffer = NULL;
}

void bta_resample_start(bta_resample_config_t *config)
{
    bk_err_t ret = BK_OK;

    LOGI("%s\n", __func__);

    bta_resample_buffer_init(config);

    rtos_init_mutex(&resample_info.ctx_lock);

    resample_info.event = xEventGroupCreate();
    resample_info.cb = config->cb;

    if (resample_info.event == NULL)
    {
        LOGE("resample_info.event null!\n");
    }

    resample_info.running = true;

#if SMP_THREAD_USED
    ret = rtos_core1_create_thread(&resample_info.hanndle,
                                   BEKEN_DEFAULT_WORKER_PRIORITY - 2,
                                   "resample",
                                   (beken_thread_function_t)bta_resample_task_entry,
                                   1024 * 4,
                                   (beken_thread_arg_t)0);
#else
    ret = rtos_create_thread(&resample_info.hanndle,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 2,
                             "resample",
                             (beken_thread_function_t)bta_resample_task_entry,
                             1024 * 4,
                             (beken_thread_arg_t)0);
#endif
    if (ret != BK_OK)
    {
        LOGE("%s fail \n", __func__);
    }
}

void bta_resample_stop(void)
{
    LOGI("%s +++\n", __func__);

    resample_info.running = false;

    xEventGroupWaitBits(resample_info.event, EVT_EXIT_IND, true, false, BEKEN_WAIT_FOREVER);
    vEventGroupDelete(resample_info.event);

    bta_resample_buffer_deinit();

    LOGI("%s ---\n", __func__);

}

uint8_t bta_resample_is_enabled(void)
{
    return resample_info.running;
}
