#include "keepalive_client.h"
#include <driver/pwr_clk.h>
#include "modules/cif_common.h"
#include "powerctrl.h"

#define KA_CLIENT_TAG "KA_CLIENT"

#define LOGI(...)   BK_LOGI(KA_CLIENT_TAG, ##__VA_ARGS__)
#define LOGW(...)   BK_LOGW(KA_CLIENT_TAG, ##__VA_ARGS__)
#define LOGE(...)   BK_LOGE(KA_CLIENT_TAG, ##__VA_ARGS__)
#define LOGD(...)   BK_LOGD(KA_CLIENT_TAG, ##__VA_ARGS__)
#define LOGV(...)   BK_LOGV(KA_CLIENT_TAG, ##__VA_ARGS__)

ka_client_env_t ka_client_env;

static void ka_client_rtc_timer_handler(aon_rtc_id_t id, uint8_t *name_p, void *param)
{
    //LOGI("RTC timeout\r\n");
    //bk_pm_sleep_mode_set(PM_MODE_NORMAL_SLEEP);
    if (!ka_client_env.keepalive_ongoing)
    {
        return;
    }
    rtos_set_semaphore(&ka_client_env.sema);
}

void ka_client_set_keepalive_rtc(void)
{
    alarm_info_t keepalive_rtc = {
                                    "ka_rtc",
                                    ka_client_env.tx_interval_s*1000*AON_RTC_MS_TICK_CNT,
                                    1,
                                    ka_client_rtc_timer_handler,
                                    NULL
                                };
    if (ka_client_env.tx_interval_s*1000 < KA_RTC_TIMER_THRESHOLD)
    {
        LOGE("timeout %d invalid ! must > %dms.\r\n", ka_client_env.tx_interval_s*1000, KA_RTC_TIMER_THRESHOLD);
        return;
    }
    //force unregister previous if doesn't finish.
    bk_alarm_unregister(AON_RTC_ID_1, keepalive_rtc.name);
    bk_alarm_register(AON_RTC_ID_1, &keepalive_rtc);
    bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
    bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);
    //LOGI("set keepalive RTC\r\n");
}

void ka_client_stop_keepalive_rtc(void)
{
    //force unregister previous if doesn't finish.
    bk_alarm_unregister(AON_RTC_ID_1, ka_client_env.keepalive_rtc.name);
    LOGI("stop keepalive RTC\r\n");
}


void ka_client_start_lv_sleep(void)
{
    LOGI("%s %d\n", __func__, __LINE__);
    if (ka_client_env.lp_state == PM_MODE_LOW_VOLTAGE)
    {
        LOGI("already in LVSLEEP state\n");
        return;
    }
    ka_client_env.lp_state = PM_MODE_LOW_VOLTAGE;
    pl_start_lv_sleep();
}

void ka_client_exit_lv_sleep(void)
{
    LOGI("%s %d\n", __func__, __LINE__);
    ka_client_env.lp_state = PM_MODE_NORMAL_SLEEP;
    pl_exit_lv_sleep();
}

bk_err_t ka_client_deinit_connection(ka_client_env_t *client)
{
    if (client->sock >= 0)
        closesocket(client->sock);
    return BK_OK;
}

bk_err_t ka_client_init_connection(ka_client_env_t *client)
{
    struct sockaddr_in addr;
    uint8_t retry_cnt = 0;
    int ret;

    while (retry_cnt < KA_SOCKET_MAX_RETRY_CNT)
    {
        retry_cnt++;
        client->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (client->sock < 0) {
            LOGI("create socket failed, err=%d!\n", errno);
            rtos_delay_milliseconds(1000);
            continue;
        }

        addr.sin_family = PF_INET;
        addr.sin_port = htons(client->keepalive_cfg.port);
        addr.sin_addr.s_addr = inet_addr((char *)(client->keepalive_cfg.server));

        ret = connect(client->sock, (const struct sockaddr *)&addr, sizeof(addr));
        if (ret == -1) {
            LOGI("connect failed, err=%d!\n", errno);
            closesocket(client->sock);
            client->sock = -1;
            rtos_delay_milliseconds(1000);
            continue;
        }

        LOGI("connect to server[%s] port[%d] successful!\n", 
                    client->keepalive_cfg.server, client->keepalive_cfg.port);
        ka_set_sock_opt(client->sock);
        return BK_OK;
    }

    LOGI("init connection failed!\n");
    client->sock = -1;
    return BK_FAIL;
}

void ka_client_handle_rx_msg(ka_msg_id_e rx_msg_id)
{
    switch(rx_msg_id)
    {
        case KA_MSG_WAKEUP_HOST_REQ:
        {
            pl_wakeup_host(POWERUP_POWER_WAKEUP_FLAG);
            break;
        }
        case KA_MSG_PWRDOWN_HOST_REQ:
        {
            pl_power_down_host();
            break;
        }
        case KA_MSG_ENTER_LVSLEEP_REQ:
        {
            ka_client_start_lv_sleep();
            break;
        }
        case KA_MSG_EXIT_LVSLEEP_REQ:
        {
            ka_client_exit_lv_sleep();
            break;
        }
        default:
        {
            //LOGI("RX msg:%d\n", rx_msg_id);
            break;
        }
    }
}

void ka_client_rx_handler(void *arg)
{
    int rx_msg_id;
    ka_client_env_t *client = (ka_client_env_t *)arg;

    LOGI("%s %d\n", __func__, __LINE__);

    client->rx_msg_info.rx_buffer = (uint8_t *) os_malloc(KEEPALIVE_RX_DATA_SIZE);
    if (client->rx_msg_info.rx_buffer == NULL) {
        LOGE("%s %d: no memory\n", __func__, __LINE__);
        goto _exit;
    }

    while (client->keepalive_ongoing) 
    {
        rx_msg_id = ka_recv_msg_handle(client->sock, &client->rx_msg_info);
        if (rx_msg_id < 0)
        {
            LOGE("%s %d: RX failed\n", __func__, __LINE__);
            goto _exit;
        }
        else if (rx_msg_id < KA_MSG_BUTT)
        {
            ka_client_handle_rx_msg(rx_msg_id);
        }
    }

_exit:
    LOGE("%s %d: RX task exit\n", __func__, __LINE__);
    ka_client_deinit();
}

bk_err_t ka_client_init_rx()
{
    bk_err_t ret;

    ret = rtos_create_thread(&ka_client_env.rx_handle,
                                KA_TASK_PRIO,
                                "cid_rx_demo",
                                (beken_thread_function_t)ka_client_rx_handler,
                                2048,
                                (beken_thread_arg_t)&ka_client_env);
    if (ret != BK_OK)
    {
        LOGE("%s %d: create task failed:%d\n", __func__, __LINE__, ret);
        ka_client_deinit();
    }

    return ret;
}

void ka_client_handler(void *arg)
{
    int ret;
    ka_client_env_t *client = (ka_client_env_t *)arg;

    LOGI("client start, keepalive server[%s] port[%d]\n", 
                    client->keepalive_cfg.server, client->keepalive_cfg.port);

    pl_power_down_host();
    cif_filter_add_customer_filter(inet_addr((char *)(client->keepalive_cfg.server)), client->keepalive_cfg.port);
    if (ka_client_init_connection(client) != BK_OK)
    {
        LOGI("exit with connection err\n");
        goto _exit;
    }

    ret = ka_client_init_rx();
    if (BK_OK != ret) 
    {
        LOGI("init RX failed\n");
        goto _exit;
    }

    ka_client_start_lv_sleep();

    while (client->keepalive_ongoing) 
    {
        ka_client_set_keepalive_rtc();
        ret = rtos_get_semaphore(&client->sema, BEKEN_WAIT_FOREVER);
        if (BK_OK != ret) 
        {
            LOGI("get sema failed\n");
            goto _exit;
        }

        ka_send_msg(client->sock, KA_MSG_KEEPALIVE_REQ);
    }

_exit:
    LOGI("TX task exit\n");
    ka_client_deinit();
}

void ka_client_timer_handler(void *arg)
{
    LOGI("%s %d\n", __func__, __LINE__);
    cif_start_lv_sleep();
    ka_client_set_keepalive_rtc();
}

bk_err_t ka_client_deinit()
{
    LOGI("%s %d\n", __func__, __LINE__);

    ka_client_env.keepalive_ongoing = false;

    ka_client_deinit_connection(&ka_client_env);
    cif_filter_add_customer_filter(0, 0);
    if (ka_client_env.rx_msg_info.rx_buffer)
    {
        os_free(ka_client_env.rx_msg_info.rx_buffer);
        ka_client_env.rx_msg_info.rx_buffer = NULL;
    }

    if (ka_client_env.tx_handle) {
        rtos_delete_thread(&ka_client_env.tx_handle);
        ka_client_env.tx_handle = NULL;
    }

    if (ka_client_env.rx_handle) {
        rtos_delete_thread(&ka_client_env.rx_handle);
        ka_client_env.rx_handle = NULL;
    }

    if (ka_client_env.timer.handle) {
        rtos_deinit_timer(&ka_client_env.timer);
        ka_client_env.timer.handle = NULL;
    }

    if (ka_client_env.sema) {
        rtos_deinit_semaphore(&ka_client_env.sema);
        ka_client_env.sema = NULL;
    }
    ka_client_stop_keepalive_rtc();
    os_memset(&ka_client_env, 0x0, sizeof(ka_client_env));
    return BK_OK;
}

bk_err_t ka_client_demo_init(void *arg)
{
    bk_err_t ret = BK_OK;
    struct bk_msg_keepalive_cfg_req *req = (struct bk_msg_keepalive_cfg_req *)arg;

    if (ka_client_env.keepalive_ongoing)
    {
        LOGI("%s %d keepalive is ongoing\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (ka_client_env.tx_interval_s == 0)
        ka_client_env.tx_interval_s = KEEPALIVE_TX_INTERVAL_SEC;
    ka_client_env.keepalive_ongoing = true;
    os_memcpy(&(ka_client_env.keepalive_cfg), req, sizeof(struct bk_msg_keepalive_cfg_req));

    ret = rtos_init_semaphore(&ka_client_env.sema, 1);
    if (ret != BK_OK) 
    {
        LOGE("%s %d: init semaphore failed:%d\n", __func__, __LINE__, ret);
        goto ka_init_failed;
    }

    ret = rtos_init_timer(&(ka_client_env.timer), KEEPALIVE_START_LVSLEEP_DELAY,
                    ka_client_timer_handler, 0);
    if (ret != BK_OK) 
    {
        LOGE("%s %d: init timer failed:%d\n", __func__, __LINE__, ret);
        goto ka_init_failed;
    }
    ret = rtos_create_thread(&ka_client_env.tx_handle,
                                KA_TASK_PRIO,
                                "cid_tx_demo",
                                (beken_thread_function_t)ka_client_handler,
                                2048,
                                (beken_thread_arg_t)&ka_client_env);
    if (ret != BK_OK)
    {
        LOGE("%s %d: create task failed:%d\n", __func__, __LINE__, ret);
        goto ka_init_failed;
    }

    LOGI("%s %d: success\n", __func__, __LINE__);

    return ret;
ka_init_failed:
    ka_client_deinit();
    return ret;
}

bk_err_t ka_client_demo_start(struct bk_msg_keepalive_cfg_req *req, uint16_t interval_s)
{
    ka_client_env.tx_interval_s = interval_s;
    return ka_client_demo_init(req);
}

void ka_client_cli_usage(void)
{
    BK_LOG_RAW("Usage: ka client [-s server_ip] [option]\n");
    BK_LOG_RAW("\n");
    BK_LOG_RAW("Option:\n");
    BK_LOG_RAW("  -p  keepalive port\n");
    BK_LOG_RAW("  -t  keepalive message TX interval\n");
    BK_LOG_RAW("  -h  usage\n");
    BK_LOG_RAW("  stop  stop demo program\n");
}

void ka_client_hanlde_cli(int argc, char **argv)
{
    int id;
    struct bk_msg_keepalive_cfg_req req;
    uint16_t keepalive_interval = KEEPALIVE_TX_INTERVAL_SEC;
    uint16_t port = KA_DEFAULT_SOCKET_PORT;

    /* keepalive protocol port*/
    id = ka_param_find_id(argc, argv, "-p");
    if (KA_INVALID_INDEX != id) {
        if (argc - 1 < id + 1)
            goto __usage;
        port = os_strtoul(argv[id + 1], NULL, 10);
    }

    /* keepalive tx interval*/
    id = ka_param_find_id(argc, argv, "-t");
    if (KA_INVALID_INDEX != id) {
        if (argc - 1 < id + 1)
            goto __usage;
        keepalive_interval = os_strtoul(argv[id + 1], NULL, 10);
    }

    id = ka_param_find_id(argc, argv, "-s");
    if (KA_INVALID_INDEX != id) {
        if (argc - 1 < id + 1)
            goto __usage;
        os_strcpy(req.server, argv[id + 1]);
        req.port = port;
        ka_client_demo_start(&req, keepalive_interval);
        return;
    }

    if ((argc == 3) && (os_strcmp(argv[2], "stop") == 0))
    {
        ka_client_env.keepalive_ongoing = false;
        ka_client_deinit();
    }
    else if ((argc == 3) && (os_strcmp(argv[2], "wakeup_host") == 0))
    {
        pl_wakeup_host(POWERUP_POWER_WAKEUP_FLAG);
    }
    else if ((argc == 3) && (os_strcmp(argv[2], "pwrdown_host") == 0))
    {
        pl_power_down_host();
    }
    else if ((argc == 3) && (os_strcmp(argv[2], "exit_lvsleep") == 0))
    {
        ka_client_exit_lv_sleep();
    }
    else if ((argc == 3) && (os_strcmp(argv[2], "enter_lvsleep") == 0))
    {
        ka_client_start_lv_sleep();
    }
    else if ((argc == 3) && (os_strcmp(argv[2], "-h") == 0))
    {
        ka_client_cli_usage();
    }
    else
    {
        goto __usage;
    }

    return;

__usage:
    LOGI("Invalid client CLI command\n");
    ka_client_cli_usage();
}

bk_err_t ka_client_init(void)
{
    //cif_register_keepalive_handler(ka_client_demo_init);
    return BK_OK;
}