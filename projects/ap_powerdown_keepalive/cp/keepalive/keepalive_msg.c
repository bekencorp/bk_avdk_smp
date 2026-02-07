#include "keepalive_msg.h"


#define KA_MSG_TAG "KA_MSG"

#define LOGI(...)   BK_LOGI(KA_MSG_TAG, ##__VA_ARGS__)
#define LOGW(...)   BK_LOGW(KA_MSG_TAG, ##__VA_ARGS__)
#define LOGE(...)   BK_LOGE(KA_MSG_TAG, ##__VA_ARGS__)
#define LOGD(...)   BK_LOGD(KA_MSG_TAG, ##__VA_ARGS__)
#define LOGV(...)   BK_LOGV(KA_MSG_TAG, ##__VA_ARGS__)

const char *ka_msg_str[] =
{
    "KEEPALIVE_REQ",        // KA_MSG_KEEPALIVE_REQ = 0,
    "KEEPALIVE_RSP",        // KA_MSG_KEEPALIVE_RSP = 1,
    "WAKEUP_HOST_REQ",      // KA_MSG_WAKEUP_HOST_REQ = 2,
    "WAKEUP_HOST_RSP",      // KA_MSG_WAKEUP_HOST_RSP = 3,
    "PWRDOWN_HOST_REQ",     // KA_MSG_PWRDOWN_HOST_REQ = 4,
    "PWRDOWN_HOST_RSP",     // KA_MSG_PWRDOWN_HOST_RSP = 5,
    "ENTER_LVSLEEP_REQ",    // KA_MSG_ENTER_LVSLEEP_REQ = 6,
    "ENTER_LVSLEEP_RSP",    // KA_MSG_ENTER_LVSLEEP_RSP = 7,
    "EXIT_LVSLEEP_REQ",     // KA_MSG_EXIT_LVSLEEP_REQ = 8,
    "EXIT_LVSLEEP_RSP",     // KA_MSG_EXIT_LVSLEEP_RSP = 9,
    "ENTER_DEEPSLEEP_REQ",  // KA_MSG_ENTER_DEEPSLEEP_REQ = 10,
    "ENTER_DEEPSLEEP_RSP",  // KA_MSG_ENTER_DEEPSLEEP_RSP = 11,
};

ka_common_t ka_common = {
    .queue_item_count = 16,
    .msg_queue        = NULL,
    .handle           = NULL,
    .stack_size       = 2048,
    .timer            = {0},
    .host_keepalive_cnt   = KA_INVALID_KEEPALIVE_CNT,
    .server_keepalive_cnt = KA_INVALID_KEEPALIVE_CNT,
};

int ka_param_find_id(int argc, char **argv, char *param)
{
    int i;
    int index;

    index = KA_INVALID_INDEX;
    if (NULL == param)
        goto find_over;

    for (i = 1; i < argc; i ++) {
        if (os_strcmp(argv[i], param) == 0) {
            index = i;
            break;
        }
    }

find_over:
    return index;
}

int ka_param_find(int argc, char **argv, char *param)
{
    int id;
    int find_flag = 0;

    id = ka_param_find_id(argc, argv, param);
    if (KA_INVALID_INDEX != id)
        find_flag = 1;

    return find_flag;
}

void ka_set_sock_opt(int sock)
{
    struct timeval tv;
    int flag = 1;

    setsockopt(sock, IPPROTO_TCP,   /* set option at TCP level */
               TCP_NODELAY, /* name of option */
               (void *)&flag,       /* the cast is historical cruft */
               sizeof(int));        /* length of option value */

    tv.tv_sec = KEEPALIVE_TX_TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    tv.tv_sec = KEEPALIVE_RX_TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

int ka_send_data(int sock, uint8_t *data, uint16_t len)
{
    int ret;

    if (sock < 0)
    {
        LOGI("%s %d: sock invalid\n", __func__, __LINE__);
        return -1;
    }

    if (!data)
    {
        LOGI("%s %d: data is null\n", __func__, __LINE__);
        return -1;
    }

    if (len > KA_MSG_MAX_LEN)
    {
        LOGI("%s %d: len[%d] is greater than %d\n", __func__, __LINE__, len, KA_MSG_MAX_LEN);
        return -1;
    }

    ret = send(sock, data, len, 0);
    if (ret <= 0) 
    {
        LOGI("%s %d: send data failed errno:%d ret:%d\n", __func__, __LINE__, errno, ret);
    }

    return ret;
}

int ka_send_msg(int sock, ka_msg_id_e msg_id)
{
    int ret;
    ka_msg_t msg;

    msg.len = sizeof(msg.data);
    msg.msg_id = msg_id;
    msg.data = 0xABCD;

    ret = ka_send_data(sock, (uint8_t *)&msg, sizeof(ka_msg_t));
    if (ret == sizeof(ka_msg_t)) 
    {
        LOGI("send msg[%d]:%s success\n", msg_id, ka_msg_str[msg_id]);
    }
    else
    {
        LOGE("send msg:%s failed, ret:%d ERROR:%d\n", ka_msg_str[msg_id], ret, errno);
        ret = -1;
    }
    return ret;
}

ka_msg_id_e ka_parse_rx_msg(int sock, ka_rx_msg_info_t *rx_msg_info)
{
    ka_msg_t *msg = NULL;
    int ret = 0;
    BK_ASSERT(rx_msg_info->rx_size == KA_MSG_MAX_LEN);
    msg = (ka_msg_t *)(rx_msg_info->rx_buffer);
    ka_msg_id_e msg_id = msg->msg_id;

    if (msg_id < KA_MSG_BUTT)
    {
        LOGI("RX msg[%d]:%s\n", msg_id, ka_msg_str[msg_id]);
    }
    else
    {
        LOGI("RX Unknowed msg len:%x id:%x\n", msg->len, msg->msg_id,msg->data);
    }

    switch(msg_id)
    {
        case KA_MSG_KEEPALIVE_REQ:
            ret = ka_send_msg(sock, KA_MSG_KEEPALIVE_RSP);
            break;
        case KA_MSG_KEEPALIVE_RSP:
            break;
        case KA_MSG_WAKEUP_HOST_REQ:
            ret = ka_send_msg(sock, KA_MSG_WAKEUP_HOST_RSP);
            break;
        case KA_MSG_WAKEUP_HOST_RSP:
            break;
        case KA_MSG_PWRDOWN_HOST_REQ:
            ret = ka_send_msg(sock, KA_MSG_PWRDOWN_HOST_RSP);
            break;
        case KA_MSG_PWRDOWN_HOST_RSP:
            break;
        case KA_MSG_ENTER_LVSLEEP_REQ:
            ret = ka_send_msg(sock, KA_MSG_ENTER_LVSLEEP_RSP);
            break;
        case KA_MSG_ENTER_LVSLEEP_RSP:
            break;
        case KA_MSG_EXIT_LVSLEEP_REQ:
            ret = ka_send_msg(sock, KA_MSG_EXIT_LVSLEEP_RSP);
            break;
        case KA_MSG_EXIT_LVSLEEP_RSP:
            break;
        case KA_MSG_ENTER_DEEPSLEEP_REQ:
            ret = ka_send_msg(sock, KA_MSG_ENTER_DEEPSLEEP_RSP);
            break;
        case KA_MSG_ENTER_DEEPSLEEP_RSP:
            break;
        default:
            ret = -1;
            break;
    }
    rx_msg_info->rx_size = 0;
    os_memset(rx_msg_info->rx_buffer, 0x0, KEEPALIVE_RX_DATA_SIZE);

    if (ret < 0)
    {
        return KA_MSG_BUTT;
    }
    return msg_id;
}

int ka_recv_msg_handle(int sock, ka_rx_msg_info_t *rx_msg_info)
{
    int rx_size = 0;
    uint8_t rx_retry_cnt = 0;
rx_retry:
    rx_size = recv(sock, rx_msg_info->rx_buffer + rx_msg_info->rx_size, KA_MSG_MAX_LEN - rx_msg_info->rx_size, 0);
    if (rx_size <= 0)
    {
        //LOGI("RX msg failed rx_size:%d, err=%d!\n", rx_size, errno);
        if (errno == EWOULDBLOCK)
            rx_retry_cnt++;
        else
        {
            LOGI("RX msg failed, size:%d, err=%d!\n", rx_size, errno);
            return -1;
        }

        if (rx_retry_cnt > KEEPALIVE_RX_MAX_RETRY_CNT)
        {
            LOGI("RX msg reaches MAX retry, size:%d, err=%d!\n", rx_size, errno);
            return -1;
        }
        else
            goto rx_retry;
    }
    else if (rx_size + rx_msg_info->rx_size == KA_MSG_MAX_LEN)
    {
        rx_msg_info->rx_size = KA_MSG_MAX_LEN;
        return ka_parse_rx_msg(sock, rx_msg_info);
    }
    else
    {
        LOGI("rx_size:%d msg_rx_size:%d err=%d!\n", rx_size, rx_msg_info->rx_size, errno);
    }

    return KA_MSG_BUTT;
}


void ka_co_send_msg(ka_common_e new_msg)
{
    bk_err_t ret;

    if (ka_common.msg_queue) {

        ret = rtos_push_to_queue(&ka_common.msg_queue, &new_msg, BEKEN_NO_WAIT);
        if (kNoErr != ret) {
            LOGE("%s,%d,push queue fail\n",__func__,__LINE__);
        }
    }
}

static void ka_common_handler(void *data1,void *data2)
{
    static uint16_t time_cnt = 0;
    bk_err_t err;
    time_cnt++;

    if(ka_common.host_keepalive_cnt != KA_INVALID_KEEPALIVE_CNT)
    {
        ka_common.host_keepalive_cnt++;
    }
    //CASE0: KEEP ALIVE WITH HOST
    if(ka_common.host_keepalive_cnt == ((KEEPALIVE_WITH_HOST_TIME)/(KA_CO_TIMER_INTVL)))
    {
        LOGI("keepalive with host timeout:%d\n", ka_common.host_keepalive_cnt);
        ka_co_send_msg(KEEPALIVE_WITH_HOSTS);
        ka_common.host_keepalive_cnt = 0;
    }
    //CASE2: KEEP ALIVE WITH SERVER
    if(ka_common.server_keepalive_cnt != KA_INVALID_KEEPALIVE_CNT)
    {
        //os_printf("keepalive with server failure cnt:%d\n", ka_common.server_keepalive_cnt);
        ka_common.server_keepalive_cnt++;
    }
    if(ka_common.server_keepalive_cnt == ((KEEPALIVE_WITH_SERVER_FAIL_TIME)/(KA_CO_TIMER_INTVL)))
    {
        LOGI("keepalive with server timeout:%d\n", ka_common.server_keepalive_cnt);
        ka_co_send_msg(KEEPALIVE_WITH_SERVER_FAILED);
        ka_common.server_keepalive_cnt = 0;
    }

    err = rtos_oneshot_reload_timer(&ka_common.timer);
    if(err != kNoErr) {
        LOGE("%s,%d,reload timer fail\n",__func__,__LINE__);
    }
}

static void ka_common_main(beken_thread_arg_t data)
{
    bk_err_t err;

    err = rtos_init_oneshot_timer(&ka_common.timer,
                          KA_CO_TIMER_INTVL,
                          ka_common_handler,
                          (void *)0,
                          (void *)0);
    if(err != kNoErr) {
        LOGE("%s,%d, fail\n",__func__,__LINE__);
    }

    err = rtos_start_oneshot_timer(&ka_common.timer);
    if(err != kNoErr) {
        LOGE("%s,%d, fail\n",__func__,__LINE__);
    }

    while (1) {
        ka_common_e msg;

        err = rtos_pop_from_queue(&ka_common.msg_queue, &msg, BEKEN_WAIT_FOREVER);
        if (kNoErr == err) {
            switch (msg) {
            case KEEPALIVE_WITH_HOSTS:
            {
                ka_disable_keepalive_with_host();
                break;
            }
            case KEEPALIVE_WITH_SERVER_FAILED:
            {
                //pl_wakeup_host(POWERUP_KEEPALIVE_DISCONNECTION);
                ka_disable_keepalive_with_server();
            }
            default:
                break;
            }
        }
    }
}

unsigned int ka_common_init(void)
{
    int ret;
    LOGI("%s\r\n",__func__);

    if ((ka_common.handle == NULL)&& (ka_common.msg_queue == NULL)) {
    
        ret = rtos_init_queue(&ka_common.msg_queue,
                              "ka_common_queue",
                              sizeof(ka_common_e),
                              ka_common.queue_item_count);
        if (kNoErr != ret) {
            LOGE("%s %d: create queue failed\r\n", __func__, __LINE__);
            return kGeneralErr;
        }

        ret = rtos_create_thread(&ka_common.handle,
                                 BEKEN_DEFAULT_WORKER_PRIORITY,
                                 "ka_common",
                                 (beken_thread_function_t)ka_common_main,
                                 ka_common.stack_size,
                                 NULL);
        if (ret != kNoErr) {
            rtos_deinit_queue(&ka_common.msg_queue);
            ka_common.msg_queue = NULL;
            LOGE("%s,%d,create thread fail\n",__func__,__LINE__);
            return kGeneralErr;
        }
    }

    return kNoErr;
}


