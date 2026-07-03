#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "bta_manager.h"
#include "bluetooth_config.h"
#include "bta_event.h"

#define TAG "bta_event"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct
{
    beken_queue_t queue;
    beken_thread_t thread;
    uint8_t running;
} bta_event_info_t;


static bta_event_info_t info = {0};


void bta_core_event_dispather(uint32_t event, uint32_t param, uint32_t extra)
{
    switch (event)
    {
        case BTA_EVT_CORE_WAIT4START:
        {
            beken_semaphore_t *init_semaphore = (beken_semaphore_t *)param;
            LOGI("%s BTA_EVT_CORE_WAIT4START\n", __func__);
            rtos_set_semaphore(init_semaphore);
        }
        break;
    }
}

void bta_event_thread(void *arg)
{
    info.running = true;

    while (info.running)
    {
        bk_err_t ret;
        bta_msg_t msg;

        ret = rtos_pop_from_queue(&info.queue, &msg, BEKEN_WAIT_FOREVER);

        if (ret != BK_OK)
        {
            LOGE("%s wait queue failed\n", __func__);
            continue;
        }

        switch (msg.event >> MODULE_BIT)
        {
            case BTA_MOD_CORE:
                bta_core_event_dispather(msg.event, msg.param, msg.extra);
                break;

            case BTA_MOD_AURACAST:
                bta_auracast_event_dispather(msg.event, msg.param, msg.extra);
                break;

            case BTA_MOD_AUDIO:
                bta_audio_event_dispather(msg.event, msg.param, msg.extra);
                break;

            case BTA_MOD_MANAGER:
                bta_manager_event_dispather(msg.event, msg.param, msg.extra);
                break;

        }
    }

    rtos_deinit_queue(&info.queue);
    info.queue = NULL;
    info.thread = NULL;
    rtos_delete_thread(NULL);
}

bk_err_t bta_event_send(uint32_t event, uint32_t param, uint32_t extra)
{
    bta_msg_t msg;
    bk_err_t ret = BK_FAIL;

    msg.event = event;
    msg.param = param;
    msg.extra = extra;

    ret = rtos_push_to_queue(&info.queue, &msg, BEKEN_WAIT_FOREVER);

    if (BK_OK != ret)
    {
        LOGE("%s, send queue failed\n", __func__);
    }

    return ret;
}

int bta_event_init(void)
{
    bk_err_t ret = BK_FAIL;
    beken_semaphore_t init_sem;

    ret = rtos_init_queue(&info.queue,
                          "bta_queue",
                          sizeof(bta_msg_t),
                          BTA_EVT_MSG_COUNT);

    if (ret != BK_OK)
    {
        LOGE("%s init bta_queue failed\n", __func__);
        return BK_FAIL;
    }

#if SMP_THREAD_USED
    ret = rtos_core1_create_thread(&info.thread,
                                   4,
                                   "bta_thread",
                                   (beken_thread_function_t)bta_event_thread,
                                   1024 * 5,
                                   (beken_thread_arg_t)0);

#else
    ret = rtos_create_thread(&info.thread,
                             4,
                             "bta_thread",
                             (beken_thread_function_t)bta_event_thread,
                             1024 * 5,
                             (beken_thread_arg_t)0);
#endif

    if (ret != BK_OK)
    {
        LOGE("%s init bta_thread failed\n", __func__);
        return BK_FAIL;
    }

    rtos_init_semaphore(&init_sem, 1);
    bta_event_send(BTA_EVT_CORE_WAIT4START, (uint32)&init_sem, 0);
    rtos_get_semaphore(&init_sem, BEKEN_NEVER_TIMEOUT);
    rtos_deinit_semaphore(&init_sem);

    LOGI("%s success\n", __func__);

    return ret;
}
