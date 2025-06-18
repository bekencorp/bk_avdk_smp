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
        default:
        {
            ret = BK_FAIL;
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
