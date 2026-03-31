#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>

#include <modules/wifi.h>
#include <modules/wifi_types.h>
#include <modules/raw_link.h>
#include "rlk_demo_cli.h"
#include "rlk_common.h"
#include "bk_misc.h"


RLK_IPERF_PARAM rlk_iperf_env = {0};

static uint32_t s_tick_last = 0;
static uint32_t s_tick_delta = 0;
static uint32_t s_pkt_delta = 0;

static uint32_t rlk_iperf_report_priority = RLK_IPERF_REPORT_TASK_PRIORITY;

static beken_thread_t rlk_iperf_report_thread_hdl = NULL;
static beken_thread_t rlk_iperf_client_thread_hdl = NULL;
static beken_thread_t rlk_iperf_server_thread_hdl = NULL;

beken_semaphore_t rlk_iperf_server_sem;
beken_semaphore_t rlk_iperf_client_sem;


#if (CONFIG_TASK_WDT)
extern void bk_task_wdt_feed(void);
#endif

void rlk_iperf_defaults_set(void)
{
    if(rlk_iperf_env.size == 0)
        rlk_iperf_env.size = RLK_IPERF_BUFSZ;

    if(rlk_iperf_env.s_time == 0)
        rlk_iperf_env.s_time = 30;

    if(rlk_iperf_env.interval == 0)
        rlk_iperf_env.interval = 1;

    if(rlk_iperf_env.speed_limit == 0)
        rlk_iperf_env.speed_limit = RLK_IPERF_DEFAULT_SPEED_LIMIT;
}

void rlk_iperf_usage(void)
{
    RLK_LOGD("Usage: rlk_iperf [-s] [-c] [peer_mac_address] [-i] [-n] [-t] [-b] [-h][--stop]\r\n");
    RLK_LOGD("rlk_iperf param:\r\n");
    RLK_LOGD("-s: run in rlk_server mode\n");
    RLK_LOGD("-c: run in rlk_client mode\n");
    RLK_LOGD("peer_mac_address: peer device mac address\r\n");
    RLK_LOGD("-i: set the intervel time in secondes(default 1 secs)\r\n");
    RLK_LOGD("-t: time in seconds to transmit for (default 30 secs)\r\n");
    RLK_LOGD("-b: set target bandwidth in bits/sec\r\n");
    RLK_LOGD("-h: print help infomation and quit\r\n");
    RLK_LOGD("--stop stop rlk_iperf program\r\n");
    RLK_LOGD("For example rlk client: \r\n");
    RLK_LOGD("rlk_iperf -c ff:ff:ff:ff:ff:ff -i 1 -t 60 -b 20M \r\n");
    RLK_LOGD("rlk server:\r\n");
    RLK_LOGD("rlk_iperf -s -i 1\r\n");

    return;
}

void rlk_iperf_report_avg_bandwidth(uint64_t pkt_len)
{
    if (pkt_len > 0)
    {
        double total_f;
        total_f = (double)pkt_len * 8;
        total_f /= (double)(RLK_IPERF_MILLION_UNIT * s_tick_delta);
        RLK_LOGD("[%d-%d] sec bandwidth: %.2f  Mbits/sec.\r\n",
                0, s_tick_delta , total_f);
    }
    RLK_LOGD("rlk_iperf: rlk iperf is stopped\r\n");
}

void rlk_iperf_stop(void)
{
    if (rlk_iperf_env.state == RLK_IPERF_STATE_STARTED)
    {
        rlk_iperf_env.state = RLK_IPERF_STATE_STOPPING;
        RLK_LOGD("rlk_iperf: rlk iperf is stopping...\n");
    }

    if ((rlk_iperf_env.mode == RLK_IPERF_MODE_CLIENT) && (rlk_iperf_env.connected == 1))
    {
        rlk_iperf_send_disconnection();
    }
    else
    {
        if (rlk_iperf_env.connected == 1)
        {
            rlk_iperf_env.connected = 0;
        }
    }
}

void rlk_iperf_reset(void)
{
    rlk_iperf_env.mode = RLK_IPERF_MODE_NONE;
    rlk_iperf_env.state = RLK_IPERF_STATE_STOPPED;
}

int rlk_iperf_bw_delay(int send_size)
{
    int period_us = 0;
    float pkts_per_tick = 0;

    if (rlk_iperf_env.speed_limit > 0) 
    {
        pkts_per_tick = rlk_iperf_env.speed_limit * 1.0 / (send_size * 8) / 500;
        period_us = 2000 / pkts_per_tick;
        RLK_LOGD("rlk_iperf_size:%d, speed_limit:%d, period_us:%d pkts_per_tick:%d\n",
            send_size, rlk_iperf_env.speed_limit, period_us, pkts_per_tick);
    }
    return period_us;
}

void rlk_iperf_disconnection_cb(void *mgmt_args, bool success)
{
    if (success)
    {
        rlk_iperf_env.connected = 0;
    }
    else
    {
        if (rlk_iperf_env.connected_cnt > RLK_IPERF_MAX_TX_RETRY)
        {
             rlk_iperf_env.connected_cnt = 0;
             RLK_LOGE("rlk_iperf: tx reaches max retry(%u)\n", rlk_iperf_env.connected_cnt);
        }
        else
        {
            rlk_iperf_env.connected_cnt++;
            rlk_iperf_send_disconnection();
        }
    }
}

bk_err_t rlk_iperf_send_disconnection(void)
{
    RLK_APP_HDR* app_ptr;
    bk_rlk_config_info_t rlk_cntrl;
    bk_err_t ret;

    app_ptr = (RLK_APP_HDR*)os_malloc(RLK_IPERF_CONNECITON_SIZE);

    if (app_ptr == NULL)
    {
        RLK_LOGD("%s mem failed!!\n",__func__);
        return BK_FAIL;
    }
    memset((void*)app_ptr,0,RLK_IPERF_CONNECITON_SIZE);

    app_ptr->type = RLK_APP_TYPE_DATA;
    app_ptr->subtype = RLK_APP_IPERF_DISCONNECTION;
    app_ptr->len = RLK_IPERF_CONNECITON_SIZE;
    app_ptr->seq = 0;
    app_ptr->end = 1;

    os_memset(&rlk_cntrl, 0, sizeof(bk_rlk_config_info_t));

    rlk_cntrl.data = (uint8_t *)(app_ptr);
    rlk_cntrl.len = RLK_IPERF_CONNECITON_SIZE;
    rlk_cntrl.cb = rlk_iperf_disconnection_cb;
    rlk_cntrl.args = NULL;

    ret = bk_rlk_send_ex(rlk_iperf_env.mac_addr, &rlk_cntrl);

    if (ret < 0)
    {
        RLK_LOGD("rlk_iperf_send failed ret:%d\n", ret);

        os_free(app_ptr);
    }
    return ret;
}

void rlk_iperf_connection_req_cb(void *mgmt_args, bool success)
{
    if (success)
    {
        rlk_iperf_env.connected = 1;
        rtos_set_semaphore(&rlk_iperf_client_sem);
    }
    else
    {
        if (rlk_iperf_env.connected_cnt > RLK_IPERF_MAX_TX_RETRY)
        {
             rlk_iperf_env.connected_cnt = 0;
             RLK_LOGE("rlk_iperf: tx reaches max retry(%u)\n", rlk_iperf_env.connected_cnt);
        }
        else
        {
            rlk_iperf_env.connected_cnt++;
            rlk_iperf_send_connection_req();
        }
    }
}

bk_err_t rlk_iperf_send_connection_req(void)
{
    RLK_APP_HDR* app_ptr;
    bk_rlk_config_info_t rlk_cntrl;
    bk_err_t ret;

    app_ptr = (RLK_APP_HDR*)os_malloc(RLK_IPERF_CONNECITON_SIZE);

    if (app_ptr == NULL)
    {
        RLK_LOGD("%s malloc failed\n",__func__);
        return BK_FAIL;
    }
    memset((void*)app_ptr,0,RLK_IPERF_CONNECITON_SIZE);

    app_ptr->type = RLK_APP_TYPE_DATA;
    app_ptr->subtype = RLK_APP_IPERF_CONNECTION_REQ;
    app_ptr->len = RLK_IPERF_CONNECITON_SIZE;
    app_ptr->seq = rlk_iperf_env.connected_seq;
    app_ptr->end = 0;

    os_memset(&rlk_cntrl, 0, sizeof(bk_rlk_config_info_t));

    rlk_cntrl.data = (uint8_t *)(app_ptr);
    rlk_cntrl.len = RLK_IPERF_CONNECITON_SIZE;
    rlk_cntrl.cb = rlk_iperf_connection_req_cb;
    rlk_cntrl.args = NULL;

    ret = bk_rlk_send_ex(rlk_iperf_env.mac_addr, &rlk_cntrl);

    if (ret < 0)
    {
        RLK_LOGD("rlk_iperf_send failed ret:%d\n", ret);
        os_free(app_ptr);
    }
    return ret;
}

bk_err_t rlk_iperf_disconnection_handler(bk_rlk_recv_info_t *rx_info)
{
    bk_err_t ret = BK_OK;

    if (0 != memcmp(rlk_iperf_env.local_mac_addr, rx_info->des_addr, RLK_WIFI_MAC_ADDR_LEN))
    {
        return BK_FAIL;
    }

    rlk_iperf_env.connected = 0;

    return ret;
}

bk_err_t rlk_iperf_connection_req_handler(bk_rlk_recv_info_t *rx_info)
{
    RLK_APP_HDR *app_recv_ptr;
    bk_err_t ret = BK_OK;

    if (0 != memcmp(rlk_iperf_env.local_mac_addr, rx_info->des_addr, RLK_WIFI_MAC_ADDR_LEN))
    {
        return BK_FAIL;
    }

    app_recv_ptr = (RLK_APP_HDR*)rx_info->data;

    os_memcpy(rlk_iperf_env.mac_addr,rx_info->src_addr,RLK_WIFI_MAC_ADDR_LEN);

    rlk_iperf_env.connected_seq = app_recv_ptr->seq;
    rlk_iperf_env.connected = 1;
    rtos_set_semaphore(&rlk_iperf_server_sem);

    return ret;
}

void rlk_iperf_report_task_handler(void *arg)
{
    uint32_t delay_interval = (rlk_iperf_env.interval * 1000);
    beken_time_get_time(&s_tick_last);
    s_tick_delta = 0;
    s_pkt_delta = 0;
    uint32_t tick_now = 0;
    uint32_t count = 0;
    uint64_t total_len = 0;

    while (rlk_iperf_env.state == RLK_IPERF_STATE_STARTED)
    {
        beken_time_get_time(&tick_now);
        rtos_delay_milliseconds(delay_interval / 5);

        int f;
        f = s_pkt_delta / RLK_IPERF_KILO_UNIT * 8;

        if (++count >= RLK_IPERF_REPORT_INTERVAL * 5)  /* 5: schedule every 200ms */
        {
            count = 0;

            if (s_pkt_delta >= 0)
            {
                RLK_LOGD("[%d-%d] sec bandwidth: %d Kbits/sec.\r\n",
                          s_tick_delta, s_tick_delta + RLK_IPERF_REPORT_INTERVAL, f);
            }
            s_tick_delta = s_tick_delta + RLK_IPERF_REPORT_INTERVAL;

            total_len += s_pkt_delta;
            s_tick_last = tick_now;
            s_pkt_delta = 0;
        }

        if (rlk_iperf_env.mode == RLK_IPERF_MODE_CLIENT)
        {
            if (s_tick_delta >= rlk_iperf_env.s_time)
            {
                rlk_iperf_send_disconnection();
                break;
            }
        }

        if (rlk_iperf_env.connected == 0)
        {
            break;
        }
    }

    rlk_iperf_report_avg_bandwidth(total_len);
    total_len = 0;

    if (rlk_iperf_env.state == RLK_IPERF_STATE_STARTED)
    {
        rlk_iperf_env.state = RLK_IPERF_STATE_STOPPING;
    }

    if (rlk_iperf_report_thread_hdl != NULL)
    {
        rtos_delete_thread(&rlk_iperf_report_thread_hdl);
        rlk_iperf_report_thread_hdl = NULL;
    }
}

bk_err_t rlk_iperf_report_task_start(void)
{
    int ret;
    ret = rtos_create_thread(&rlk_iperf_report_thread_hdl, rlk_iperf_report_priority, RLK_IPERF_REPORT_TASK_NAME,
                        rlk_iperf_report_task_handler, RLK_IPERF_REPORT_TASK_STACK,
                        (beken_thread_arg_t) 0);

    if (ret != kNoErr)
    {
        RLK_LOGE("create task %s failed", RLK_IPERF_REPORT_TASK_NAME);
        return BK_FAIL;
    }

    return BK_OK;
}

int rlk_iperf_recv(void)
{
    RLK_APP_HDR *app_ptr = NULL;
    int len = 0;
    uint32_t buf = 0;

    while(kNoErr == rtos_pop_from_queue(&rlk_iperf_env.conn.recvmbox, &buf,BEKEN_WAIT_FOREVER))
    {
        app_ptr = (RLK_APP_HDR *)buf;
        if((app_ptr->type == RLK_APP_TYPE_DATA) && (app_ptr->subtype == RLK_APP_IPERF_DATA))
        {
            if (app_ptr->end == 1)
            {
                len = -1;
            }
            else
            {
                len = app_ptr->len;
            }
        }
        else
        {
            len = 0;
        }

        if (buf)
            os_free((void*)buf);

        if(len != 0)
        {
            break;
        }
    }

    return len;
}

static void rlk_iperf_client(void *thread_param)
{
    uint32_t retry_cnt = 0;
    bk_err_t ret;
    RLK_APP_HDR* app_ptr;
    int period_us = 0;
    int fdelay_us = 0;
    int64_t prev_time = 0;
    int64_t send_time = 0;
    uint32_t now_time = 0;

    app_ptr = (RLK_APP_HDR*)os_malloc(rlk_iperf_env.size);

    if (!app_ptr)
        goto _exit;

    os_memset((void*)app_ptr,0,sizeof(RLK_APP_HDR));

    os_memset((void*)(app_ptr + 1),0xff,(rlk_iperf_env.size - sizeof(RLK_APP_HDR)));

    app_ptr->type = RLK_APP_TYPE_DATA;
    app_ptr->subtype = RLK_APP_IPERF_DATA;
    app_ptr->len = rlk_iperf_env.size;
    app_ptr->seq = 0;
    app_ptr->end = 0;

    period_us = rlk_iperf_bw_delay(rlk_iperf_env.size);
    while (rlk_iperf_env.state == RLK_IPERF_STATE_STARTED)
    {
        rlk_iperf_send_connection_req();
_accept_retry:
        if (rlk_iperf_env.connected == 0)
        {
            ret = rtos_get_semaphore(&rlk_iperf_client_sem, BEKEN_WAIT_FOREVER);

            if (BK_OK != ret)
            {
                break;
            }

            goto _accept_retry;
        }

        RLK_LOGD("rlk_iperf: connect to rlk_iperf server successful!\n");

        rlk_iperf_report_task_start();

        prev_time = rtos_get_time();
        while (rlk_iperf_env.state == RLK_IPERF_STATE_STARTED) 
        {
            if (rlk_iperf_env.speed_limit > 0)
            {
                send_time = rtos_get_time();
                fdelay_us = period_us + (int32_t)(prev_time - send_time);
                prev_time = send_time;
            }
            else if (rlk_iperf_env.speed_limit == 0)
            {
                now_time = rtos_get_time();
                if ((now_time - prev_time) / 1000 > 0)
                {
                    prev_time = now_time;
                    rtos_delay_milliseconds(4);
                }
            }

            retry_cnt = 0;
_tx_retry:
            ret = bk_rlk_send(rlk_iperf_env.mac_addr,app_ptr,rlk_iperf_env.size);

            if (rlk_iperf_env.connected == 0)
            {
                rlk_iperf_env.state = RLK_IPERF_STATE_STOPPING;
                break;
            }

            if (rlk_iperf_env.state != RLK_IPERF_STATE_STARTED)
           {
                break;
            }

            if (ret) {
                if (ret >= 0)
                {
                    s_pkt_delta +=ret;
                    if (fdelay_us > 0)
                    {
                        bk_delay_us(fdelay_us);
                    }
                }
            }
            else
            {
                if (rlk_iperf_env.state != RLK_IPERF_STATE_STARTED)
                {
                    break;
                }

                if (ret < 0)
                {
                    retry_cnt ++;
                    if (retry_cnt >= RLK_IPERF_MAX_TX_RETRY) 
                    {
                        RLK_LOGE("rlk_iperf: tx reaches max retry(%u)\n", retry_cnt);
                        break;
                    } else
                    {
                        goto _tx_retry;
                    }
                }
                break;
            }

           #if (CONFIG_TASK_WDT)
            bk_task_wdt_feed();
            #endif
        }

        if (rlk_iperf_env.state != RLK_IPERF_STATE_STARTED)
            break;
        rtos_delay_milliseconds(1000 * 2);
    }

_exit:
    if (app_ptr)
        os_free(app_ptr);
    rlk_iperf_reset();
    RLK_LOGD("rlk_iperf: rlk iperf is stopped\r\n");
}

void rlk_iperf_server(void *thread_param)
{
    int bytes_received;
    bk_err_t ret;

    while (rlk_iperf_env.state == RLK_IPERF_STATE_STARTED)
    {
_accept_retry:
        if (rlk_iperf_env.connected == 0)
        {
            ret = rtos_get_semaphore(&rlk_iperf_server_sem, BEKEN_WAIT_FOREVER);

            if (BK_OK != ret)
            {
                break;
            }

            goto _accept_retry;
        }

        rlk_iperf_report_task_start();

        while (rlk_iperf_env.state == RLK_IPERF_STATE_STARTED)
        {
            bytes_received = rlk_iperf_recv();

            if (rlk_iperf_env.connected == 0)
            {
                rlk_iperf_env.state = RLK_IPERF_STATE_STOPPING;
                break;
            }

            if (bytes_received == -1)
            {
                rlk_iperf_env.connected = 0;
                rlk_iperf_env.state = RLK_IPERF_STATE_STOPPING;
                break;
            }
            else
            {
                RLK_LOGV("s_pkt_delta %d bytes_received %d\r\n",s_pkt_delta,bytes_received);
                s_pkt_delta += bytes_received;
            }
        }
    }

//__exit:
    rlk_iperf_reset();
    //RLK_LOGD("rlk_iperf: rlk iperf is stopped\r\n");
}

void rlk_iperf_queue_init(void)
{
    rtos_init_queue(&rlk_iperf_env.conn.recvmbox, "rlk_iperf_recv", sizeof(uint32), RLK_IPERF_RECV_MSG_NUM);
}

void rlk_iperf_queue_deinit()
{
    rlk_iperf_env.state = RLK_IPERF_STATE_STOPPED;
    rtos_deinit_queue(&rlk_iperf_env.conn.recvmbox);
}

void rlk_iperf_client_thread(void *thread_param)
{
    rlk_iperf_queue_init();

    rlk_iperf_client(thread_param);
    rlk_iperf_queue_deinit();

    if (rlk_iperf_client_thread_hdl)
    {
        rtos_delete_thread(&rlk_iperf_client_thread_hdl);
        rlk_iperf_client_thread_hdl = NULL;
    }

    if (rlk_iperf_client_sem)
    {
        rtos_deinit_semaphore(&rlk_iperf_client_sem);
    }

    RLK_LOGD("rlk_iperf: rlk_iperf is stopped\n");
}

void rlk_iperf_server_thread(void *thread_param)
{
    rlk_iperf_queue_init();
    rlk_iperf_server(thread_param);
    rlk_iperf_queue_deinit();

    if (rlk_iperf_server_thread_hdl)
    {
        rtos_delete_thread(&rlk_iperf_server_thread_hdl);
        rlk_iperf_server_thread_hdl = NULL;
    }

    if (rlk_iperf_server_sem)
    {
        rtos_deinit_semaphore(&rlk_iperf_server_sem);
    }

    RLK_LOGD("rlk_iperf: rlk_iperf is stopped\n");
}

void rlk_iperf_start(int mode)
{
    int ret;

    if (rlk_iperf_env.state == RLK_IPERF_STATE_STOPPED)
    {
        if ((mode == RLK_IPERF_MODE_CLIENT) && (!rlk_iperf_env.mac_addr))
        {
            RLK_LOGD("rlk_iperf: Please input <peer mac address >\r\n");
            return;
        }

        if (mode == RLK_IPERF_MODE_CLIENT)
        {
            ret = rtos_init_semaphore(&rlk_iperf_client_sem, 1);
            
            if (BK_OK != ret)
            {
                RLK_LOGE("%s semaphore init failed\n", __func__);
                return;
            }
        }
        else
        {
            ret = rtos_init_semaphore(&rlk_iperf_server_sem, 1);
            
            if (BK_OK != ret)
            {
                RLK_LOGE("%s semaphore init failed\n", __func__);
                return;
            }
        }

        rlk_iperf_env.state = RLK_IPERF_STATE_STARTED;
        rlk_iperf_env.mode = mode;
        rlk_iperf_env.connected = 0;

        bk_wifi_sta_get_mac((uint8_t *)(&rlk_iperf_env.local_mac_addr));

        if (mode == RLK_IPERF_MODE_CLIENT)
        {
            #ifdef CONFIG_FREERTOS_SMP
            rtos_smp_create_thread(&rlk_iperf_client_thread_hdl, RLK_IPERF_PRIORITY, "rlk_iperf_c",
                               rlk_iperf_client_thread, RLK_IPERF_THREAD_SIZ,
                               (beken_thread_arg_t) 0);
            #else
            rtos_create_thread(&rlk_iperf_client_thread_hdl, RLK_IPERF_PRIORITY, "rlk_iperf_c",
                               rlk_iperf_client_thread, RLK_IPERF_THREAD_SIZ,
                               (beken_thread_arg_t) 0); 
            #endif
        }
        else if (mode == RLK_IPERF_MODE_SERVER)
        {
            #ifdef CONFIG_FREERTOS_SMP
            rtos_smp_create_thread(&rlk_iperf_server_thread_hdl, RLK_IPERF_PRIORITY, "rlk_iperf_s",
                               rlk_iperf_server_thread, RLK_IPERF_THREAD_SIZ,
                               (beken_thread_arg_t) 0);
            #else
            rtos_create_thread(&rlk_iperf_server_thread_hdl, RLK_IPERF_PRIORITY, "rlk_iperf_s",
                               rlk_iperf_server_thread, RLK_IPERF_THREAD_SIZ,
                               (beken_thread_arg_t) 0); 
            #endif
        }
        else
        {
            RLK_LOGD("rlk_iperf: invalid rlk_iperf mode=%d\r\n", mode);
        }
    }
    else if (rlk_iperf_env.state == RLK_IPERF_STATE_STOPPING)
    {
        RLK_LOGD("rlk_iperf: rlk_iperf is stopping, try again later!\n");
    }
    else
    {
        RLK_LOGD("rlk_iperf: rlk_iperf is running, stop first!\n");
    }
}


