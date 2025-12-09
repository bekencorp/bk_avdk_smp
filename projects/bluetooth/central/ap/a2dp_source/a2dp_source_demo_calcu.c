#include <os/os.h>
#include <components/log.h>
#include <os/mem.h>
#include <modules/audio_rsp_types.h>
#include <modules/audio_rsp.h>
#include <modules/sbc_encoder.h>
#include "a2dp_source_demo_calcu.h"

#define TAG "a2dp_calcu"

enum
{
    BT_AUDIO_DEBUG_LEVEL_ERROR,
    BT_AUDIO_DEBUG_LEVEL_WARNING,
    BT_AUDIO_DEBUG_LEVEL_INFO,
    BT_AUDIO_DEBUG_LEVEL_DEBUG,
    BT_AUDIO_DEBUG_LEVEL_VERBOSE,
};

#define BT_AUDIO_DEBUG_LEVEL BT_AUDIO_DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_ERROR)   BK_LOGE(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_WARNING) BK_LOGW(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_INFO)    BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_DEBUG)   BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_VERBOSE) BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)


typedef struct
{
    uint32_t ret;
    uint32_t in_len;
    uint32_t out_len;
} bt_audio_resample_result_t;

static uint8_t s_is_rsp_inited;


static beken_thread_t s_bt_audio_task = NULL;
static beken_queue_t s_msg_queue = NULL;

static aud_rsp_cfg_t s_rsp_cfg_final;

static void bt_audio_task(void *arg)
{
    int32_t ret = 0;
    a2dp_source_calcu_req_t msg = {0};

    while (1)
    {
        ret = rtos_pop_from_queue(&s_msg_queue, &msg, BEKEN_WAIT_FOREVER);

        if (ret)
        {
            LOGE("pop queue err %d", ret);
            continue;
        }

        switch (msg.type)
        {
        case EVENT_BT_PCM_RESAMPLE_INIT_REQ:
            do
            {
                if (s_is_rsp_inited)
                {
                    LOGE("resample already init");
                    break;
                }

                extern bk_err_t bk_audio_osi_funcs_init(void);
                bk_audio_osi_funcs_init();

                bt_audio_resample_init_req_t *cfg = &msg.rsp_init;

                os_memset(&s_rsp_cfg_final, 0, sizeof(s_rsp_cfg_final));
                s_rsp_cfg_final = cfg->rsp_cfg;

                LOGI("resample init %p %d %d %d %d %d %d %d %d", cfg,
                     s_rsp_cfg_final.src_rate,
                     s_rsp_cfg_final.src_ch,
                     s_rsp_cfg_final.src_bits,
                     s_rsp_cfg_final.dest_rate,
                     s_rsp_cfg_final.dest_ch,
                     s_rsp_cfg_final.dest_bits,
                     s_rsp_cfg_final.complexity,
                     s_rsp_cfg_final.down_ch_idx);

                ret = bk_aud_rsp_init(s_rsp_cfg_final);

                if (ret)
                {
                    LOGE("bk_aud_rsp_init err %d !!", ret);
                    ret = -1;
                    break;
                }

                s_is_rsp_inited = 1;

            }
            while (0);

            if (ret)
            {
                if (s_is_rsp_inited)
                {
                    bk_aud_rsp_deinit();
                    s_is_rsp_inited = 0;
                }
            }

            break;

        case EVENT_BT_PCM_RESAMPLE_DEINIT_REQ:
            do
            {
                if (!s_is_rsp_inited)
                {
                    LOGE("resample already deinit");
                    break;
                }

                ret = bk_aud_rsp_deinit();

                if (ret)
                {
                    LOGE("bk_aud_rsp_deinit err %d !!", ret);
                    ret = -1;
                    break;
                }

                s_is_rsp_inited = 0;

            }
            while (0);

            if (ret)
            {
                if (s_is_rsp_inited)
                {
                    bk_aud_rsp_deinit();
                    s_is_rsp_inited = 0;
                }
            }

            break;

        case EVENT_BT_PCM_RESAMPLE_REQ:
        {
            bt_audio_resample_req_t *param = &msg.rsp_req;

            uint32_t in_len = *(param->in_bytes_ptr) / (s_rsp_cfg_final.src_bits / 8);
            uint32_t out_len = *(param->out_bytes_ptr) / (s_rsp_cfg_final.dest_bits / 8);

            LOGD("resample start %p %p %p %d %d", param, param->in_addr, param->out_addr, in_len, out_len);

            ret = bk_aud_rsp_process((int16_t *)param->in_addr, &in_len, (int16_t *)param->out_addr, &out_len);

            if (ret)
            {
                LOGE("bk_aud_rsp_process err %d !!", ret);
            }
            else
            {
                *(param->in_bytes_ptr) = in_len * (s_rsp_cfg_final.src_bits / 8);
                *(param->out_bytes_ptr) = out_len * (s_rsp_cfg_final.dest_bits / 8);
            }

            LOGD("resample done %d %d", in_len, out_len);
        }
        break;

        case EVENT_BT_PCM_ENCODE_INIT_REQ:
            ret = 0;
            break;

        case EVENT_BT_PCM_ENCODE_DEINIT_REQ:
            ret = 0;
            break;

        case EVENT_BT_PCM_ENCODE_REQ:
        {
            bt_audio_encode_req_t *param = &msg.encode_req;

            if (!param || !param->handle || !param->in_addr || !param->out_len_ptr)
            {
                LOGE("encode req param err");
                ret = -1;
                break;
            }

            int32_t encode_len = 0;

            if (param->type != 0)
            {
                LOGE("type not match %d", param->type);
                ret = -1;
                break;
            }

            encode_len = sbc_encoder_encode((SbcEncoderContext *)param->handle, (const int16_t *)param->in_addr);

            if (!encode_len)
            {
                LOGE("encode err %d", encode_len);
                ret = -1;
                break;
            }

            *param->out_len_ptr = encode_len;
        }
        break;

        case EVENT_BT_PCM_EXIT:
            goto end;
            break;

        default:
            LOGE("unknow event 0x%x", msg.type);
            ret = -1;
            break;
        }

        if (msg.ret_sem)
        {
            rtos_set_semaphore(msg.ret_sem);
        }
    }

end:;

    LOGI("exit");
    rtos_delete_thread(NULL);
}

static bk_err_t bt_audio_init_handle(void)
{
    int ret = 0;

    LOGI("");

    if (s_bt_audio_task)
    {
        LOGE("already init");
        goto end;
    }

    ret = rtos_init_queue(&s_msg_queue,
                          "a2dp_source_calcu_queue",
                          sizeof(a2dp_source_calcu_req_t),
                          50);

    if (ret)
    {
        LOGE("queue init failed");
        ret = -1;
        goto end;
    }

    ret = rtos_create_thread(&s_bt_audio_task,
                             4,
                             "bt_audio_task",
                             (beken_thread_function_t)bt_audio_task,
                             1024 * 5,
                             (beken_thread_arg_t)NULL);

    if (ret)
    {
        LOGE("task init failed");
        ret = -1;
        goto end;
    }

end:

    if (ret)
    {
        if (s_bt_audio_task)
        {
            rtos_thread_join(&s_bt_audio_task);
            s_bt_audio_task = NULL;
        }
    }

    return ret;
}


static bk_err_t bt_audio_deinit_handle(void)
{
    int ret = 0;

    LOGI("");

    if (!s_bt_audio_task)
    {
        LOGE("already deinit");
        return 0;
    }

    if (s_msg_queue)
    {
        a2dp_source_calcu_req_t msg = {0};

        msg.type = EVENT_BT_PCM_EXIT;

        ret = rtos_push_to_queue(&s_msg_queue, &msg, BEKEN_WAIT_FOREVER);

        if (kNoErr != ret)
        {
            LOGE("send exit queue failed");
        }
    }

    rtos_thread_join(&s_bt_audio_task);
    s_bt_audio_task = NULL;

    if (s_msg_queue)
    {
        rtos_deinit_queue(&s_msg_queue);
        s_msg_queue = NULL;
    }

end:

    if (ret)
    {
        if (s_bt_audio_task)
        {
            rtos_thread_join(&s_bt_audio_task);
        }

        if (s_msg_queue)
        {
            rtos_deinit_queue(&s_msg_queue);
            s_msg_queue = NULL;
        }
    }

    return ret;
}

bk_err_t a2dp_source_demo_calcu_rsp_init_req(bt_audio_resample_init_req_t *req, uint8_t is_init)
{
    int32_t ret = 0;

    a2dp_source_calcu_req_t msg = {0};

    if (!s_msg_queue)
    {
        LOGE("queue not init");
        return -1;
    }

    LOGI("%d", is_init);

    msg.type = (is_init ? EVENT_BT_PCM_RESAMPLE_INIT_REQ : EVENT_BT_PCM_RESAMPLE_DEINIT_REQ);

    if (is_init)
    {
        msg.rsp_init = *req;
    }

    beken_semaphore_t sem_tmp = NULL;
    ret = rtos_init_semaphore(&sem_tmp, 1);

    if (ret)
    {
        LOGE("init sem err");
        return -1;
    }

    msg.ret_sem = &sem_tmp;

    ret = rtos_push_to_queue(&s_msg_queue, &msg, BEKEN_WAIT_FOREVER);

    if (kNoErr != ret)
    {
        LOGE("send queue failed");
        goto end;
    }

    ret = rtos_get_semaphore(&sem_tmp, BEKEN_WAIT_FOREVER);

end:;

    if (sem_tmp)
    {
        rtos_deinit_semaphore(&sem_tmp);
        sem_tmp = NULL;
    }

    return ret;
}

bk_err_t a2dp_source_demo_calcu_rsp_req(bt_audio_resample_req_t *req)
{
    int32_t ret = 0;

    a2dp_source_calcu_req_t msg = {0};

    if (!s_msg_queue)
    {
        LOGE("queue not init");
        return -1;
    }

    msg.type = EVENT_BT_PCM_RESAMPLE_REQ;
    msg.rsp_req = *req;

    beken_semaphore_t sem_tmp = NULL;
    ret = rtos_init_semaphore(&sem_tmp, 1);

    if (ret)
    {
        LOGE("init sem err");
        return -1;
    }

    msg.ret_sem = &sem_tmp;

    ret = rtos_push_to_queue(&s_msg_queue, &msg, BEKEN_WAIT_FOREVER);

    if (kNoErr != ret)
    {
        LOGE("send queue failed");
        goto end;
    }

    ret = rtos_get_semaphore(&sem_tmp, BEKEN_WAIT_FOREVER);

end:;

    if (sem_tmp)
    {
        rtos_deinit_semaphore(&sem_tmp);
        sem_tmp = NULL;
    }


    return ret;
}

bk_err_t a2dp_source_demo_calcu_encode_init_req(bt_audio_encode_req_t *req, uint8_t is_init)
{
    int32_t ret = 0;

    a2dp_source_calcu_req_t msg = {0};

    if (!s_msg_queue)
    {
        LOGE("queue not init");
        return -1;
    }

    LOGI("%d", is_init);

    msg.type = (is_init ? EVENT_BT_PCM_ENCODE_INIT_REQ : EVENT_BT_PCM_ENCODE_DEINIT_REQ);

    beken_semaphore_t sem_tmp = NULL;
    ret = rtos_init_semaphore(&sem_tmp, 1);

    if (ret)
    {
        LOGE("init sem err");
        return -1;
    }

    msg.ret_sem = &sem_tmp;

    ret = rtos_push_to_queue(&s_msg_queue, &msg, BEKEN_WAIT_FOREVER);

    if (kNoErr != ret)
    {
        LOGE("send queue failed");
        goto end;
    }

    ret = rtos_get_semaphore(&sem_tmp, BEKEN_WAIT_FOREVER);

end:;

    if (sem_tmp)
    {
        rtos_deinit_semaphore(&sem_tmp);
        sem_tmp = NULL;
    }

    return ret;
}

bk_err_t a2dp_source_demo_calcu_encode_req(bt_audio_encode_req_t *req)
{
    int32_t ret = 0;

    a2dp_source_calcu_req_t msg = {0};

    if (!s_msg_queue)
    {
        LOGE("queue not init");
        return -1;
    }

    msg.type = EVENT_BT_PCM_ENCODE_REQ;
    msg.encode_req = *req;

    beken_semaphore_t sem_tmp = NULL;
    ret = rtos_init_semaphore(&sem_tmp, 1);

    if (ret)
    {
        LOGE("init sem err");
        return -1;
    }

    msg.ret_sem = &sem_tmp;

    ret = rtos_push_to_queue(&s_msg_queue, &msg, BEKEN_WAIT_FOREVER);

    if (kNoErr != ret)
    {
        LOGE("send queue failed");
        goto end;
    }

    ret = rtos_get_semaphore(&sem_tmp, BEKEN_WAIT_FOREVER);

end:;

    if (sem_tmp)
    {
        rtos_deinit_semaphore(&sem_tmp);
        sem_tmp = NULL;
    }

    return ret;
}

bk_err_t a2dp_source_demo_calcu_init(void)
{
    int32_t ret = 0;
    LOGI("");
    ret = bt_audio_init_handle();

    return ret;
}

bk_err_t a2dp_source_demo_calcu_deinit(void)
{
    int32_t ret = 0;
    LOGI("");
    ret = bt_audio_deinit_handle();

    return ret;
}
