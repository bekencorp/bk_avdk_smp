#include <os/os.h>

#include <common/bk_include.h>
#include <components/bk_audio_asr_service.h>
#include <components/bk_asr_service_types.h>
#include <components/bk_asr_service.h>

#define TAG "aud_asr"

#define AUD_ASR_CHECK_NULL(ptr, act) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "%s, %d, AUD_ASR_CHECK_NULL fail \n", __func__, __LINE__);\
            {act;};\
        }\
    } while(0)

#define AUD_ASR_RAW_READ_SIZE    (480)

typedef enum
{
    AUD_ASR_IDLE = 0,
    AUD_ASR_START,
    AUD_ASR_EXIT
} aud_asr_op_t;

typedef struct
{
    aud_asr_op_t op;
    void *param;
} aud_asr_msg_t;

struct aud_asr
{
	asr_handle_t asr_handle;                                                        /**< asr handle */
	void *args;                                                                     /**< the pravate parameter of callback */
	int task_stack;                                                                 /**< Task stack size */
	int task_core;                                                                  /**< Task running in core (0 or 1) */
	int task_prio;                                                                  /**< Task priority (based on freeRTOS priority) */
	audio_mem_type_t mem_type;                                                      /**< memory type used, sram, psram, audio_heap */
	beken_thread_t aud_asr_task_hdl;
	beken_queue_t aud_asr_msg_que;
	beken_semaphore_t sem;
	uint8_t *read_buff;
	bool running;
	uint32_t max_read_size;                                                         /**< the max size of data read from asr handle, used in asr_read_callback */
	void (*aud_asr_result_handle)(uint32_t param);
	int (*aud_asr_init)(void);
	int (*aud_asr_recog)(void *read_buf, uint32_t read_size, void *p1, void *p2);
	void (*aud_asr_deinit)(void);
};

#define UAC_MIC_DEBUG (0)
#if UAC_MIC_DEBUG
#define ASR_INPUT_START()    do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define ASR_INPUT_END()      do { GPIO_DOWN(34); } while (0)
#else
#define ASR_INPUT_START()
#define ASR_INPUT_END()
#endif

#if CONFIG_ADK_UTILS
#define ASR_DATA_DUMP_BY_UART (0)
#if ASR_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_asr_util = {0};
#define ASR_DATA_DUMP_UART_ID            (2)
#define ASR_DATA_DUMP_UART_BAUD_RATE     (1000000)

#define ASR_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_asr_util, ASR_DATA_DUMP_UART_ID, ASR_DATA_DUMP_UART_BAUD_RATE)
#define ASR_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_asr_util)
#define ASR_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_asr_util, data_buf, len)
#else

#define ASR_DATA_DUMP_BY_UART_OPEN()
#define ASR_DATA_DUMP_BY_UART_CLOSE()
#define ASR_DATA_DUMP_BY_UART_DATA(data_buf, len)
#endif  //ASR_DATA_DUMP_BY_UART
#endif

const static char *text;
static float score;

static bk_err_t aud_asr_send_msg(beken_queue_t queue, aud_asr_op_t op, void *param)
{
	bk_err_t ret = BK_FAIL;
	aud_asr_msg_t msg;

	if (!queue)
	{
		BK_LOGE(TAG, "%s, %d, queue: %p \n", __func__, __LINE__, queue);
		return ret;
	}

	msg.op = op;
	msg.param = param;
	ret = rtos_push_to_queue(&queue, &msg, BEKEN_NO_WAIT);
	if (kNoErr != ret)
	{
		BK_LOGE(TAG, "%s, %d, send message: %d fail, ret: %d\n", __func__, __LINE__, op, ret);
		return ret;
	}

	return ret;
}

static void aud_asr_result_handle(uint32_t param)
{
	BK_LOGI(TAG, "asr_result : %s\r\n", (char *)param);
}

static void aud_asr_task_main(beken_thread_arg_t param_data)
{
	int result = 0;
	int read_size = 0;
	bk_err_t ret = BK_OK;

	aud_asr_handle_t aud_asr_handle = (aud_asr_handle_t)param_data;
	ASR_DATA_DUMP_BY_UART_OPEN();

	aud_asr_handle->running = false;
	long unsigned int wait_time = BEKEN_WAIT_FOREVER;

	if (aud_asr_handle->aud_asr_init) {
		if (aud_asr_handle->aud_asr_init() < 0)
		{
			os_printf("Wanson_ASR_Init Failed!\n");
			goto aud_asr_exit;
		}
	}
	rtos_set_semaphore(&aud_asr_handle->sem);

	while (1)
	{
		aud_asr_msg_t msg;
		ret = rtos_pop_from_queue(&aud_asr_handle->aud_asr_msg_que, &msg, wait_time);
		if (kNoErr == ret)
		{
			switch (msg.op)
			{
				case AUD_ASR_IDLE:
					aud_asr_handle->running = false;
					wait_time = BEKEN_WAIT_FOREVER;
					break;
				case AUD_ASR_EXIT:
					goto aud_asr_exit;
					break;
				case AUD_ASR_START:
					aud_asr_handle->running = true;
					wait_time = 0;
					break;
				default:
					break;
			}
		}

	/* read mic data and send */
	if (aud_asr_handle->running)
	{
			/* read mic data and send */
			extern int bk_aud_asr_get_size(asr_handle_t asr_handle);
			extern int bk_aud_asr_get_filled_size(asr_handle_t asr_handle);

		//	int __maybe_unused ss = bk_aud_asr_get_filled_size(aud_asr_handle->asr_handle);
		//	int __maybe_unused sss = bk_aud_asr_get_size(aud_asr_handle->asr_handle);

			{
				read_size = bk_aud_asr_read_mic_data(aud_asr_handle->asr_handle, (char *)aud_asr_handle->read_buff, aud_asr_handle->max_read_size);
				if (read_size == aud_asr_handle->max_read_size)
				{
					uint64_t __maybe_unused start_time = rtos_get_time();
					ASR_INPUT_START();
					if (aud_asr_handle->aud_asr_recog) {
						result = aud_asr_handle->aud_asr_recog((void*)aud_asr_handle->read_buff, aud_asr_handle->max_read_size, (void*)&text, (void*)&score);
					}
					ASR_INPUT_END();
					uint64_t __maybe_unused stop_time = rtos_get_time();
					ASR_DATA_DUMP_BY_UART_DATA(aud_asr_handle->read_buff, read_size);

					if ((uint32_t)(stop_time-start_time) >= 30)
					{
						BK_LOGV(TAG, "Recogn:%d---%d\n", (uint32_t)(stop_time-start_time), result);
					} else if ((uint32_t)(stop_time-start_time) < 0)
					{
						BK_LOGV(TAG, "Error excute--%d\n", (uint32_t)(stop_time-start_time));
					} else {
						;
					}

					if (result == 1) {
						if (aud_asr_handle->aud_asr_result_handle) {
							aud_asr_handle->aud_asr_result_handle((uint32_t)text);
						} else
						{
							BK_LOGE(TAG, "aud_asr_handle->aud_asr_result_handle is NULL\n");
							aud_asr_result_handle((uint32_t)text);
						}
					} else {
						;
					}
				}
				else {
					continue;
				}
			}
		}
	}

aud_asr_exit:

	aud_asr_handle->running = false;
	if (aud_asr_handle->aud_asr_deinit) {
		aud_asr_handle->aud_asr_deinit();
	}

	/* delete msg queue */
	ret = rtos_deinit_queue(&aud_asr_handle->aud_asr_msg_que);
	if (ret != kNoErr)
	{
		BK_LOGE(TAG, "%s, %d, delete message queue fail\n", __func__, __LINE__);
	}
	aud_asr_handle->aud_asr_msg_que = NULL;

	/* delete task */
	aud_asr_handle->aud_asr_task_hdl = NULL;
	rtos_set_semaphore(&aud_asr_handle->sem);
	rtos_delete_thread(NULL);
}

aud_asr_handle_t bk_aud_asr_init(aud_asr_cfg_t *cfg)
{
    bk_err_t ret = BK_OK;
    aud_asr_handle_t aud_asr_handle = NULL;

    AUD_ASR_CHECK_NULL(cfg, return NULL);

    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        aud_asr_handle = psram_malloc(sizeof(struct aud_asr));
    }
    else
    {
        aud_asr_handle = os_malloc(sizeof(struct aud_asr));
    }
    AUD_ASR_CHECK_NULL(aud_asr_handle, return NULL);

    os_memset(aud_asr_handle, 0x00, sizeof(struct aud_asr));

    /* copy config */
    aud_asr_handle->asr_handle    = cfg->asr_handle;
    aud_asr_handle->args          = cfg->args;
    aud_asr_handle->task_stack    = cfg->task_stack;
    aud_asr_handle->task_core     = cfg->task_core;
    aud_asr_handle->task_prio     = cfg->task_prio;
    aud_asr_handle->mem_type      = cfg->mem_type;

    aud_asr_handle->max_read_size         = cfg->max_read_size;
    aud_asr_handle->aud_asr_result_handle = cfg->aud_asr_result_handle;
    aud_asr_handle->aud_asr_init          = cfg->aud_asr_init;
    aud_asr_handle->aud_asr_recog         = cfg->aud_asr_recog;
    aud_asr_handle->aud_asr_deinit        = cfg->aud_asr_deinit;

    /* malloc read buffer */
    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        aud_asr_handle->read_buff = psram_malloc(aud_asr_handle->max_read_size);
    }
    else
    {
        aud_asr_handle->read_buff = os_malloc(aud_asr_handle->max_read_size);
    }
    AUD_ASR_CHECK_NULL(aud_asr_handle->read_buff, goto fail);

    os_memset(aud_asr_handle->read_buff, 0x00, aud_asr_handle->max_read_size);

    ret = rtos_init_semaphore(&aud_asr_handle->sem, 1);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate semaphore fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = rtos_init_queue(&aud_asr_handle->aud_asr_msg_que,
                          "aud_asr_que",
                          sizeof(aud_asr_msg_t),
                          32);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, ceate aud asr message queue fail\n", __func__, __LINE__);
        goto fail;
    }

    #if CONFIG_ASR_SERVICE_THREAD_BIND_CPU
    #if CONFIG_ASR_SERVICE_THREAD_BIND_CPU_ID == 0
    ret = rtos_core0_create_thread(&aud_asr_handle->aud_asr_task_hdl,
                             aud_asr_handle->task_prio,
                             "aud_asr",
                             (beken_thread_function_t)aud_asr_task_main,
                             aud_asr_handle->task_stack,
                             (beken_thread_arg_t)aud_asr_handle);
    #elif CONFIG_ASR_SERVICE_THREAD_BIND_CPU_ID == 1
    ret = rtos_core1_create_thread(&aud_asr_handle->aud_asr_task_hdl,
                         aud_asr_handle->task_prio,
                         "aud_asr",
                         (beken_thread_function_t)aud_asr_task_main,
                         aud_asr_handle->task_stack,
                         (beken_thread_arg_t)aud_asr_handle);
    #endif
    #else
    ret = rtos_create_thread(&aud_asr_handle->aud_asr_task_hdl,
                         aud_asr_handle->task_prio,
                         "aud_asr",
                         (beken_thread_function_t)aud_asr_task_main,
                         aud_asr_handle->task_stack,
                         (beken_thread_arg_t)aud_asr_handle);
    #endif
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "%s, %d, create aud asr task fail\n", __func__, __LINE__);
        goto fail;
    }

    rtos_get_semaphore(&aud_asr_handle->sem, BEKEN_NEVER_TIMEOUT);
    BK_LOGD(TAG, "init aud asr task complete\n");
    return aud_asr_handle;

fail:
    if (aud_asr_handle->sem)
    {
        rtos_deinit_semaphore(&aud_asr_handle->sem);
        aud_asr_handle->sem = NULL;
    }

    if (aud_asr_handle->aud_asr_msg_que)
    {
        rtos_deinit_queue(&aud_asr_handle->aud_asr_msg_que);
        aud_asr_handle->aud_asr_msg_que = NULL;
    }

    if (aud_asr_handle->read_buff)
    {
        if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
        {
            psram_free(aud_asr_handle->read_buff);
        }
        else
        {
            os_free(aud_asr_handle->read_buff);
        }
    }

    if (cfg->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(aud_asr_handle);
    }
    else
    {
        os_free(aud_asr_handle);
    }

    return NULL;
}

bk_err_t bk_aud_asr_deinit(aud_asr_handle_t aud_asr_handle)
{
    bk_err_t ret = BK_FAIL;

    AUD_ASR_CHECK_NULL(aud_asr_handle, return ret);

    BK_LOGD(TAG, "%s\n", __func__);

    ret = aud_asr_send_msg(aud_asr_handle->aud_asr_msg_que, AUD_ASR_EXIT, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: AUD_ASR_EXIT fail\n", __func__, __LINE__);
        return ret;
    }

    rtos_get_semaphore(&aud_asr_handle->sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&aud_asr_handle->sem);
    aud_asr_handle->sem = NULL;

    if (aud_asr_handle->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(aud_asr_handle->read_buff);
    }
    else
    {
        os_free(aud_asr_handle->read_buff);
    }

    if (aud_asr_handle->mem_type == AUDIO_MEM_TYPE_PSRAM)
    {
        psram_free(aud_asr_handle);
    }
    else
    {
        os_free(aud_asr_handle);
    }

    BK_LOGD(TAG, "deinit aud asr complete\n");
    return ret;
}

bk_err_t bk_aud_asr_start(aud_asr_handle_t aud_asr_handle)
{
    bk_err_t ret = BK_FAIL;

#if (CONFIG_WANSON_NEW_LIB_SEG)
    extern void asr_func_null(void);
    asr_func_null();
#endif

    AUD_ASR_CHECK_NULL(aud_asr_handle, return ret);
    BK_LOGD(TAG, "%s, Line:%d\n", __func__, __LINE__);

    ret = aud_asr_send_msg(aud_asr_handle->aud_asr_msg_que, AUD_ASR_START, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: AUD_ASR_START fail\n", __func__, __LINE__);
        return ret;
    }
    return ret;
}

bk_err_t bk_aud_asr_stop(aud_asr_handle_t aud_asr_handle)
{
    bk_err_t ret = BK_FAIL;
    AUD_ASR_CHECK_NULL(aud_asr_handle, return ret);
    BK_LOGD(TAG, "%s, Line:%d\n", __func__, __LINE__);

    ret = aud_asr_send_msg(aud_asr_handle->aud_asr_msg_que, AUD_ASR_IDLE, NULL);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, send message: AUD_ASR_IDLE fail\n", __func__, __LINE__);
        return ret;
    }
    return ret;
}

