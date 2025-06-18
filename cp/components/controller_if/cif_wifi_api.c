#include "net.h"
#include "bk_wifi_types.h"
#include "wifi.h"
#include "bk_wifi.h"
#include <stdlib.h>
#include <string.h>
#include "cif_wifi_api.h"
#include "cif_main.h"

bk_err_t cif_handle_wifi_api_cmd(struct bk_msg_hdr *msg)
{
    bk_err_t ret = BK_OK;
    wifi_api_arg_info_t *arg_info = (wifi_api_arg_info_t *)(msg + 1);

    CIF_LOGI("cif_handle_wifi_api_cmd cmd:%x agrc:%d\n", msg->cmd_id, arg_info->argc);

    if (arg_info->argc)
    {
        BK_ASSERT(arg_info->argc < WIFI_API_IPC_COM_REQ_MAX_ARGC);
        // CIF_LOGI("arg[0]:%x arg[1]:%x arg[2]:%x arg[3]:%x arg[4]:%x arg[5]:%x\n", 
        //     arg_info->args[0], arg_info->args[1], arg_info->args[2],
        //     arg_info->args[3], arg_info->args[4], arg_info->args[5]);
    }

    switch(msg->cmd_id)
    {
        case STA_PM_ENABLE:
        {
            ret = bk_wifi_sta_pm_enable();
            break;
        }
        case STA_PM_DISABLE:
        {
            ret = bk_wifi_sta_pm_disable();
            break;
        }
        case STA_SET_CONFIG:
        {
            wifi_sta_config_t *config = (wifi_sta_config_t *)arg_info->args[0];
            //CIF_LOGI("sizeof(wifi_sta_config_t)=%d\r\n", sizeof(wifi_sta_config_t));
            ret = bk_wifi_sta_set_config(config);
            break;
        }
        case STA_GET_CONFIG:
        {
            wifi_sta_config_t *config = (wifi_sta_config_t *)arg_info->args[0];
            //CIF_LOGI("sizeof(wifi_sta_config_t)=%d\r\n", sizeof(wifi_sta_config_t));
            ret = bk_wifi_sta_get_config(config);
            break;
        }
        case STA_START:
        {
            ret = bk_wifi_sta_start();
            break;
        }

        case STA_GET_LINK_STATUS:
        {
            wifi_link_status_t *link_status = (wifi_link_status_t *)arg_info->args[0];
            if ((wifi_netif_sta_is_connected() || wifi_netif_sta_is_got_ip()))
            {
                    bk_wifi_sta_get_link_status(link_status);
                    link_status->state = WIFI_LINKSTATE_STA_CONNECTED;
            }
            else
                link_status->state = WIFI_LINKSTATE_STA_DISCONNECTED;

            break;
        }

        case WIFI_GET_CHANNEL:
        {
            uint8_t *channel = (uint8_t *)(arg_info->args[0]);
            *channel = bk_wifi_get_channel();
            break;
        }

        case WIFI_SET_COUNTRY:
        {
            wifi_country_t *country = (wifi_country_t *)(arg_info->args[0]);
            ret = bk_wifi_set_country(country);
            break;
        }

        case STA_GET_LISTEN_INTERVAL:
        {
            uint8_t *listen_interval = (uint8_t *)(arg_info->args[0]);
            ret = bk_wifi_get_listen_interval(listen_interval);
            break;
        }

        case STA_SET_LISTEN_INTERVAL:
        {
            uint8_t listen_interval = (uint8_t)(arg_info->args[0]);
            ret = bk_wifi_send_listen_interval_req(listen_interval);
            break;
        }

        case STA_SET_BCN_LOSS_INT:
        {
            uint8_t interval = (uint8_t)(arg_info->args[0]);
            uint8_t repeat_num = (uint8_t)(arg_info->args[1]);
            ret = bk_wifi_send_bcn_loss_int_req(interval, repeat_num);
            break;
        }

        case STA_SET_BCN_RECV_WIN:
        {
            uint8_t default_win = (uint8_t)(arg_info->args[0]);
            uint8_t max_win = (uint8_t)(arg_info->args[1]);
            uint8_t step = (uint8_t)(arg_info->args[2]);
            ret = bk_wifi_set_bcn_recv_win(default_win, max_win, step);
            break;
        }

        case STA_SET_BCN_LOSS_TIME:
        {
            uint8_t wait_cnt = (uint8_t)(arg_info->args[0]);
            uint8_t wake_cnt = (uint8_t)(arg_info->args[1]);
            ret = bk_wifi_set_bcn_loss_time(wait_cnt, wake_cnt);
            break;
        }

        case STA_GET_LINK_STATE_WITH_REASON:
        {
            wifi_linkstate_reason_t *info = (wifi_linkstate_reason_t *)(arg_info->args[0]);
            ret = bk_wifi_sta_get_linkstate_with_reason(info);
            break;
        }

        default:
        {
            ret = BK_ERR_NOT_FOUND;
            break;
        }
    }

    if (cif_bk_cmd_confirm(msg, (uint8_t *)&ret, sizeof(ret)) != BK_OK)
    {
        CIF_LOGE("wifi api confirm FAILED\n",__func__,__LINE__);
        return BK_FAIL;
    }

    //CIF_LOGI("cif_handle_wifi_api_cmd ret:%d\n", ret);
    return BK_OK;
}

bk_err_t cif_send_wifi_api_evt(uint32_t cmd_id, uint32_t argc, ...)
{
     bk_err_t ret = BK_OK;
    wifi_api_arg_info_t arg_info = { 0 };

    if (argc)
    {
        BK_ASSERT (argc <= WIFI_API_IPC_COM_REQ_MAX_ARGC);
        arg_info.argc = argc;

        va_list args;
        va_start(args, argc);
        for (int i = 0; i < argc; i++)
        {
            arg_info.args[i] = va_arg(args, uint32_t);
            //WIFI_LOGI("arg[%d]:%x\n", i, com_req.arg_info.args[i]);
        }
        va_end(args);
    }
    ret = cif_bk_send_event(cmd_id, (uint8_t *)&arg_info, sizeof(wifi_api_arg_info_t));

    if (ret < 0)
    {
        CIF_LOGE("cif_send_wifi_api_evt FAILED, cmd_id:%x argc:%d ret:%d\n", cmd_id, arg_info.argc, ret);
        return BK_ERR_TIMEOUT;
    }

    return ret;
}
