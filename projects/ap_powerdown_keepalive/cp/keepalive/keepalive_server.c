#include "keepalive_server.h"


#define KA_SERVER_TAG "KA_SERVER"

#define LOGI(...)   BK_LOGI(KA_SERVER_TAG, ##__VA_ARGS__)
#define LOGW(...)   BK_LOGW(KA_SERVER_TAG, ##__VA_ARGS__)
#define LOGE(...)   BK_LOGE(KA_SERVER_TAG, ##__VA_ARGS__)
#define LOGD(...)   BK_LOGD(KA_SERVER_TAG, ##__VA_ARGS__)
#define LOGV(...)   BK_LOGV(KA_SERVER_TAG, ##__VA_ARGS__)

ka_server_env_t ka_server_env;

bk_err_t ka_server_deinit();

void ka_server_handle_rx_msg(ka_msg_id_e rx_msg_id)
{

}

void ka_server_handler(void *arg)
{
    uint32_t sin_size;
    int sock = -1, retry, rx_msg_id;
    int connected = -1;
    struct sockaddr_in server_addr, client_addr;

    ka_server_env.rx_msg_info.rx_buffer = (uint8_t *) os_malloc(KEEPALIVE_RX_DATA_SIZE);
    if (ka_server_env.rx_msg_info.rx_buffer == NULL) {
        LOGE("%s %d: no memory\n", __func__, __LINE__);
        goto __exit;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOGE("%s %d: socket error\n",__func__,__LINE__);
        goto __exit;
    }

    // Set SO_REUSEADDR to allow binding to a port that's still in TIME_WAIT state
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        LOGE("%s %d: setsockopt failed, err=%d\n", __func__, __LINE__, errno);
        goto __exit;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ka_server_env.port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    os_memset(&(server_addr.sin_zero), 0x0, sizeof(server_addr.sin_zero));

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        LOGE("%s %d: unable to bind, err=%d\n", __func__, __LINE__, errno);
        goto __exit;
    }

    if (listen(sock, 5) == -1) {
        LOGE("%s %d: listen error, err=%d\n", __func__, __LINE__, errno);
        goto __exit;
    }
    ka_set_sock_opt(sock);

    while ((ka_server_env.keepalive_ongoing) && (retry < KA_SOCKET_MAX_RETRY_CNT)) {
        sin_size = sizeof(struct sockaddr_in);
_accept_retry:
        connected = accept(sock, (struct sockaddr *)&client_addr, &sin_size);
        if ((connected == -1) && (errno == EWOULDBLOCK)) {
            retry++;
            goto _accept_retry;
        }
        retry = 0;

        LOGI("%s %d: new client connected from (%s, %u)\n", __func__, __LINE__,
                  inet_ntoa(client_addr.sin_addr),
                  ntohs(client_addr.sin_port));

        ka_set_sock_opt(connected);
        ka_server_env.client_sock = connected;
        while (ka_server_env.keepalive_ongoing)
        {
            rx_msg_id = ka_recv_msg_handle(connected, &ka_server_env.rx_msg_info);
            if (rx_msg_id < 0)
            {
                LOGE("RX failed\n");
                goto __exit;
            }
            else if (rx_msg_id < KA_MSG_BUTT)
            {
                ka_server_handle_rx_msg(rx_msg_id);
            }
        }
        if (connected >= 0)
            closesocket(connected);
        connected = -1;
    }

__exit:
    if (sock >= 0)
        closesocket(sock);

    if (ka_server_env.rx_msg_info.rx_buffer) {
        os_free(ka_server_env.rx_msg_info.rx_buffer);
        ka_server_env.rx_msg_info.rx_buffer = NULL;
    }

    LOGE("%s %d: task exit\n", __func__, __LINE__);
    ka_server_env.keepalive_ongoing = false;
    ka_server_deinit();
}

bk_err_t ka_server_deinit()
{
    LOGI("%s %d\n", __func__, __LINE__);

    if (ka_server_env.handle) {
        rtos_delete_thread(&ka_server_env.handle);
        ka_server_env.handle = NULL;
    }

    if (ka_server_env.timer.handle) {
        rtos_deinit_timer(&ka_server_env.timer);
        ka_server_env.timer.handle = NULL;
    }

    os_memset(&ka_server_env, 0x0, sizeof(ka_server_env));
    return BK_OK;
}

bk_err_t ka_server_init(uint16_t port)
{
    bk_err_t ret = BK_OK;

    if (ka_server_env.keepalive_ongoing)
    {
        LOGI("%s %d keepalive is ongoing\n", __func__, __LINE__);
    }

    ka_server_env.keepalive_ongoing = true;
    ka_server_env.port = port;

    ret = rtos_create_thread(&ka_server_env.handle,
                                KA_TASK_PRIO,
                                "ka_server",
                                (beken_thread_function_t)ka_server_handler,
                                2048,
                                (beken_thread_arg_t)0);
    if (ret != BK_OK)
    {
        LOGE("%s %d create task failed:%d\n", __func__, __LINE__, ret);
        goto ka_init_failed;
    }

    LOGI("%s %d success\n", __func__, __LINE__);

    return ret;

ka_init_failed:
    ka_server_deinit();
    return ret;
}

bk_err_t ka_server_demo_start(uint16_t port)
{
    bk_err_t ret;

    ret = ka_server_init(port);
    if (ret != BK_OK)
    {
        LOGE("%s %d: failed[%d]\n", __func__, __LINE__, ret);
        return ret;
    }

    return BK_OK;
}

void ka_server_cli_usage(void)
{
    BK_LOG_RAW("Usage: ka server [option]\n");
    BK_LOG_RAW("\n");
    BK_LOG_RAW("Option:\n");
    BK_LOG_RAW("  -p  keepalive port\n");
    BK_LOG_RAW("  -m  message string to client\n");
    BK_LOG_RAW("      -m wakeup_host - send message to client to wakeup host\n");
    BK_LOG_RAW("      -m exit_lvsleep - send message to client to exit lv sleep\n");
    BK_LOG_RAW("      -m enter_lvsleep - send message to client to enter lv sleep\n");
    BK_LOG_RAW("  -h  usage\n");
    BK_LOG_RAW("  stop  stop demo program\n");
}

void ka_server_hanlde_cli(int argc, char **argv)
{
    uint16_t port = 0;

    if (argc == 2)
    {
        ka_server_demo_start(KA_DEFAULT_SOCKET_PORT);
    }
    else if ((argc == 3) && (os_strcmp(argv[2], "stop") == 0))
    {
        ka_server_env.keepalive_ongoing = false;
        ka_server_deinit();
    }
    else if ((argc == 4) && (os_strcmp(argv[2], "-p") == 0))
    {
        port = os_strtoul(argv[3], NULL, 10);
        ka_server_demo_start(port);
    }
    else if ((argc == 4) && (os_strcmp(argv[2], "-m") == 0))
    {
        ka_msg_id_e msg_id;

        if (os_strcmp(argv[3], "wakeup_host") == 0)
        {
            msg_id = KA_MSG_WAKEUP_HOST_REQ;
        }
        else if (os_strcmp(argv[3], "exit_lvsleep") == 0)
        {
            msg_id = KA_MSG_EXIT_LVSLEEP_REQ;
        }
        else if (os_strcmp(argv[3], "enter_lvsleep") == 0)
        {
            msg_id = KA_MSG_ENTER_LVSLEEP_REQ;
        }
        else
        {
            LOGE("Invalid MSG type\n");
            goto __usage;
        }

        if (ka_server_env.client_sock < 0)
        {
            LOGI("No valid client connected\n");
        }
        else
        {
            ka_send_msg(ka_server_env.client_sock, msg_id);
        }
    }
    else if ((argc == 3) && (os_strcmp(argv[2], "-h") == 0))
    {
        ka_server_cli_usage();
    }
    else
    {
        goto __usage;
    }

    return;
__usage:
    LOGE("Invalid server CLI command\n");
    ka_server_cli_usage();
}
