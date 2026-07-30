#include "cif_cntrl.h"
#include "net.h"
#include "bk_wifi_types.h"
#include "modules/wifi.h"
#include "bk_wifi.h"
#include <stdlib.h>
#include <string.h>
//#include "time/time.h"
//#include "utils_httpc.h"
#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "cif_ipc.h"
#include "cif_wifi_api.h"
#include "wifi_config.h"
#include "lwip/stats.h"
#if CONFIG_CONTROLLER_AP_BUFFER_COPY
#include <sys_sw_regs.h>
#endif
#ifdef CONFIG_IPV6
#include "lwip/netif.h"
#include "lwip/ip6_addr.h"
#endif

extern int bmsg_tx_sender(struct pbuf *p, uint32_t vif_idx);
extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
wifi_sta_config_t cif_sta_config = {0};

int cif_sta_app_init(char *oob_ssid, char *connect_key)
{
    int len;

    len = os_strlen(oob_ssid);
    if (32 < len) {
        CTRL_IF_CMD("ssid name more than 32 Bytes\r\n");
        return BK_FAIL;
    }
    os_strcpy(cif_sta_config.ssid, oob_ssid);
    os_strcpy(cif_sta_config.password, connect_key);

#ifdef CONFIG_CONT_LP_RECONNECT
    cif_sta_config.auto_reconnect_count = CONFIG_CONT_LP_RECONNECT_COUNT;
#endif

    CTRL_IF_CMD("ssid:%s key:%s\r\n", cif_sta_config.ssid, cif_sta_config.password);
    bk_wifi_sta_set_config(&cif_sta_config);
    bk_wifi_sta_start();
    return BK_OK;
}
bk_err_t cif_bk_cmd_confirm(struct bk_msg_hdr *rx_msg, uint8_t *cfm_data, uint16_t cfm_len)
{
    uint32_t buf_len = sizeof(struct cpdu_t) + sizeof(struct bk_rx_msg_hdr) + cfm_len;
    struct ctrl_cmd_hdr * buf = NULL;
    bk_err_t ret = BK_OK;
    //CTRL_IF_CMD("%s\n",__func__);

    if (!cif_env.host_powerup)
    {
        CIF_LOGD("%s skip cfm 0x%x, host not power up\n", __func__,
                 rx_msg ? (rx_msg->cmd_id + BK_CMD_CFM_OFFSET) : 0);
        return BK_FAIL;
    }

    if (cfm_len > CIF_MAX_CFM_DATA_LEN)
    {
        CTRL_IF_CMD("cif_bk_cmd_confirm data len[%d] is greater than %d, return\n", cfm_len, CIF_MAX_CFM_DATA_LEN);
        return BK_FAIL;
    }

    buf = (struct ctrl_cmd_hdr *)cif_get_event_buffer(buf_len);
    if (buf == NULL)
    {
        CTRL_IF_CMD("Failed to allocate memory\n");
        return BK_FAIL;
    }
    buf->co_hdr.length = buf_len;
    buf->co_hdr.type = RX_BK_CMD_DATA;
    buf->co_hdr.special_type = 0;
    buf->msg_hdr.id = rx_msg->cmd_id + BK_CMD_CFM_OFFSET;
    buf->msg_hdr.cfm_sn = rx_msg ? rx_msg->cmd_sn:0;
    buf->msg_hdr.param_len = cfm_len;

    if (cfm_data && cfm_len)
    {
        memcpy(buf->msg_hdr.param, cfm_data, cfm_len);
    }

    ret = cif_msg_sender(buf,CIF_TASK_MSG_EVT,0);
    
    if (ret != BK_OK)
    {
        CIF_LOGE("%s,%d,error type:%d\n",__func__,__LINE__,ret);
        cif_free_cmd_buffer((uint8_t*)buf);
        return BK_FAIL;
    }


    return BK_OK;
}
bk_err_t cif_bk_send_event(uint16_t event_id, uint8_t *event_data, uint16_t event_len)
{
    uint32_t buf_len = sizeof(struct cpdu_t) + sizeof(struct bk_rx_msg_hdr) + event_len;
    struct ctrl_cmd_hdr * buf = NULL;
    bk_err_t ret = BK_OK;
    CTRL_IF_CMD("%s\n",__func__);

    if (!cif_env.host_powerup)
    {
        CIF_LOGD("%s skip event 0x%x, host not power up\n", __func__, event_id);
        return BK_FAIL;
    }

    // if (!cif_env.host_wifi_init)
    // {
    //     CIF_LOGV("AP does not init, cif_bk_send_event skip\n");
    //     return BK_FAIL;
    // }

    if (event_len > CIF_MAX_CFM_DATA_LEN)
    {
        CTRL_IF_CMD("ctrl_if_bk_send_event data len[%d] is greater than %d, return\n", event_len, CIF_MAX_CFM_DATA_LEN);
        return BK_FAIL;
    }

    buf = (struct ctrl_cmd_hdr *)cif_get_event_buffer(buf_len);
    if (buf == NULL)
    {
        CTRL_IF_CMD("Failed to allocate memory\n");
        return BK_FAIL;
    }
    buf->co_hdr.length = buf_len;
    buf->co_hdr.type = RX_BK_CMD_DATA;
    buf->co_hdr.special_type = 0;
    buf->msg_hdr.id = event_id;
    buf->msg_hdr.param_len = event_len;

    if (event_data && event_len)
    {
        memcpy(buf->msg_hdr.param, event_data, event_len);
    }

    ret = cif_msg_sender(buf,CIF_TASK_MSG_EVT,0);
    
    if (ret != BK_OK)
    {
        CIF_LOGE("%s,%d,error type:%d\n",__func__,__LINE__,ret);
        cif_free_cmd_buffer((uint8_t*)buf);
        return BK_FAIL;
    }

    return BK_OK;
}
bk_err_t cif_handle_bk_cmd_connect_req(struct bk_msg_hdr *msg)
{
    struct bk_msg_connect_req *req = (struct bk_msg_connect_req*) (msg + 1);
    CTRL_IF_CMD("%s,ssid = %s,password= %s\n",__func__,req->ssid,req->pw);

    cif_bk_cmd_confirm(msg, NULL, 0);

    cif_sta_app_init((char *)req->ssid, (char *)req->pw);
    return BK_OK;
}
bk_err_t cif_handle_bk_cmd_connect_ind(char *ssid, uint8_t rssi, uint32_t ip, uint32_t gw, uint32_t mk, uint32_t dns, uint8_t vif_idx)
{
    struct bk_msg_connect_ind ind = {0};

    os_strcpy((char *)ind.ussid, ssid);
    ind.rssi = rssi;
    ind.ip = ip;
    ind.gw = gw;
    ind.mk = mk;
    ind.dns = dns;
    ind.vif_idx = vif_idx;
    return cif_bk_send_event(BK_EVT_IPV4_IND, (uint8_t *)&ind, sizeof(ind));
}

#ifdef CONFIG_IPV6
bk_err_t cif_handle_bk_cmd_ipv6_ind(void *n)
{
    struct bk_msg_ipv6_ind ind = {0};
    int i;
    u8 *ipv6_addr;
    int valid_count = 0;
    struct netif *netif = (struct netif *)n;
    for (i = 0; i < MAX_IPV6_ADDRESSES_IN_MSG; i++) {
        if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
            ipv6_addr = (u8 *)(ip_2_ip6(&netif->ip6_addr[i]))->addr;
            os_memcpy(ind.ipv6_addr[valid_count].address, ipv6_addr, 16);
            ind.ipv6_addr[valid_count].addr_state = netif->ip6_addr_state[i];
            ind.ipv6_addr[valid_count].addr_type = netif->ip6_addr[i].type;
            valid_count++;
        }
    }
    ind.addr_count = valid_count;

    if (valid_count > 0) {
        return cif_bk_send_event(BK_EVT_IPV6_IND, (uint8_t *)&ind, sizeof(ind));
    }
    return BK_OK;
}
#endif

bk_err_t cif_handle_bk_cmd_disconnect_req(struct bk_msg_hdr *msg)
{
    CTRL_IF_CMD("%s\n",__func__);
    cif_bk_cmd_confirm(msg, NULL, 0);
    bk_wifi_sta_stop();

    return BK_OK;
}

bk_err_t cif_handle_bk_cmd_wifi_event_ind(cif_wifi_event_id_t event_id,
		const void *data, uint16_t data_len)
{
	cif_wifi_event_ind_t ind = {0};

	if (event_id >= CIF_WIFI_EVT_COUNT || !data ||
	    data_len > CIF_WIFI_EVENT_IND_MAX_DATA)
		return BK_ERR_PARAM;

	ind.event_id = event_id;
	ind.data_len = data_len;
	os_memcpy(ind.data, data, data_len);
	return cif_bk_send_event(BK_EVT_WIFI_EVENT_IND, (uint8_t *)&ind,
				 sizeof(ind));
}
bk_err_t cif_handle_bk_cmd_set_mac_addr_req(struct bk_msg_hdr *msg)
{
    uint8_t *base_mac = (uint8_t*)(msg + 1);
    CTRL_IF_CMD("%s\n",__func__);
    BK_LOG_ON_ERR(bk_set_base_mac(base_mac));

    return BK_OK;
}
bk_err_t cif_handle_bk_cmd_get_mac_addr_req(struct bk_msg_hdr *msg)
{
    uint8_t base_mac[BK_MAC_ADDR_LEN] = {0};

    CTRL_IF_CMD("%s\n",__func__);
    BK_LOG_ON_ERR(bk_get_mac(base_mac, MAC_TYPE_BASE));

    CTRL_IF_CMD("base mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(base_mac));

    cif_bk_cmd_confirm(msg, base_mac, BK_MAC_ADDR_LEN);
    return BK_OK;
}

#if (CONFIG_NTP_SYNC_RTC)
bk_err_t cif_handle_bk_cmd_set_time_req(struct bk_msg_hdr *msg)
{
    uint32_t *set_time = (uint32_t*)(msg + 1);
    CIF_LOGV("%s, set time: %d\n",__func__, (*set_time));
    datetime_set((time_t) (*set_time));
    cif_bk_cmd_confirm(msg, NULL, 0);
    return BK_OK;
}
bk_err_t cif_handle_bk_cmd_get_time_req(struct bk_msg_hdr *msg)
{
    uint32_t time_get = (uint32_t) timestamp_get();
    CTRL_IF_CMD("%s, time_get = %d.\n",__func__, time_get);
    cif_bk_cmd_confirm(msg, (uint8_t *)&time_get, sizeof(time_get));
    return BK_OK;
}
#endif

bk_err_t cif_handle_bk_cmd_get_wlan_status_req(struct bk_msg_hdr *msg)
{
    struct bk_msg_wlan_status_cfm cfm = {0};
    wifi_link_status_t link_status = {0};
    netif_ip4_config_t config = {0};
    char ssid[33] = {0};
    int ctrl_rssi = 0;

    CIF_LOGV("%s\n",__func__);

    if ((wifi_netif_sta_is_connected() || wifi_netif_sta_is_got_ip())) {
            os_memset(&link_status, 0x0, sizeof(link_status));
            bk_wifi_sta_get_link_status(&link_status);
            link_status.state = WIFI_LINKSTATE_STA_CONNECTED;
            os_memcpy(ssid, link_status.ssid, 32);
            ctrl_rssi = link_status.rssi;
    }
    else
        link_status.state = WIFI_LINKSTATE_STA_DISCONNECTED;

    BK_LOG_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_STA, &config));
    os_strcpy((char *)cfm.ussid, ssid);
    cfm.rssi = ctrl_rssi;
    cfm.status = link_status.state;
    os_strcpy(cfm.ip, config.ip);
    os_strcpy(cfm.gateway, config.gateway);
    os_strcpy(cfm.mask, config.mask);
    os_strcpy(cfm.dns, config.dns);
    //BK_LOGD(NULL,"link state:%d\n", link_status.state);
    cif_bk_cmd_confirm(msg, (uint8_t *)&cfm, sizeof(cfm));
    return BK_OK;
}

bk_err_t cif_handle_bk_cmd_start_ap_req(struct bk_msg_hdr *msg)
{
    struct bk_msg_start_ap_req * buf = (struct bk_msg_start_ap_req*)(msg + 1);
    CTRL_IF_CMD("%s\n",__func__);
    if(buf->band == 1)//5G Band don't support
    {
        CTRL_IF_CMD("%s, 5G Band don't support\n",__func__);
    }
    else
    {
        cif_bk_cmd_confirm(msg, NULL, 0);
        char channel[3];
        itoa(buf->channel, channel, 10);
        uint8_t hidden_flag = buf->hidden;
        if (hidden_flag) {
            if(BK_OK == demo_softap_hidden_init((char *)buf->ssid, (char *)buf->pw, (char *)channel))
            {
                cif_handle_bk_cmd_start_ap_ind(CONTROLLER_AP_START);
            }
            else
            {
                cif_handle_bk_cmd_start_ap_ind(CONTROLLER_AP_CLOSE);
            }
        } else {
            if(BK_OK == demo_softap_app_init((char *)buf->ssid, (char *)buf->pw, (char *)channel))
            {
                cif_handle_bk_cmd_start_ap_ind(CONTROLLER_AP_START);
            }
            else
            {
                cif_handle_bk_cmd_start_ap_ind(CONTROLLER_AP_CLOSE);
            }
        }
    }

    return BK_OK;
}

bk_err_t cif_handle_bk_cmd_start_ap_ind(uint8_t status)
{
    struct bk_msg_ap_status_cfm cfm = {0};
    char ip[18] = {WLAN_DEFAULT_IP};
    char gw[18] = {WLAN_DEFAULT_GW};
    char mk[18] = {WLAN_DEFAULT_MASK};
    CTRL_IF_CMD("%s\n",__func__);

    sscanf(ip, "%d.%d.%d.%d",
        (uint8_t*)(&cfm.ip), (uint8_t*)(&cfm.ip)+1, (uint8_t*)(&cfm.ip)+2,(uint8_t*)(&cfm.ip)+3);
    sscanf(gw, "%d.%d.%d.%d",
        (uint8_t*)(&cfm.gw), (uint8_t*)(&cfm.gw)+1, (uint8_t*)(&cfm.gw)+2,(uint8_t*)(&cfm.gw)+3);
    sscanf(mk, "%d.%d.%d.%d",
        (uint8_t*)(&cfm.mk), (uint8_t*)(&cfm.mk)+1, (uint8_t*)(&cfm.mk)+2,(uint8_t*)(&cfm.mk)+3);

    CTRL_IF_CMD("%s,ip=%x,gw=%x,mk=%x\n",__func__,cfm.ip,cfm.gw,cfm.mk);
    CTRL_IF_CMD("%s,ip char=%s,gw=%s,mk=%s\n",__func__,ip,gw,mk);

    cfm.status = status;

    return cif_bk_send_event(BK_EVT_START_AP_IND, (uint8_t *)&cfm, sizeof(cfm));
}
bk_err_t cif_handle_bk_cmd_stop_ap_req(struct bk_msg_hdr *msg)
{
    CTRL_IF_CMD("%s\n",__func__);
    cif_bk_cmd_confirm(msg, NULL, 0);
    bk_wifi_ap_stop();
    cif_handle_bk_cmd_stop_ap_ind(CONTROLLER_AP_CLOSE);
    return BK_OK;
}

bk_err_t cif_handle_bk_cmd_stop_ap_ind(uint8_t status)
{
    struct bk_msg_ap_status_cfm cfm = {0};
    char ip[18] = {WLAN_DEFAULT_IP};
    char gw[18] = {WLAN_DEFAULT_GW};
    char mk[18] = {WLAN_DEFAULT_MASK};

    CTRL_IF_CMD("%s\n",__func__);

    sscanf(ip, "%d.%d.%d.%d",
        (uint8_t*)(&cfm.ip), (uint8_t*)(&cfm.ip)+1, (uint8_t*)(&cfm.ip)+2,(uint8_t*)(&cfm.ip)+3);
    sscanf(gw, "%d.%d.%d.%d",
        (uint8_t*)(&cfm.gw), (uint8_t*)(&cfm.gw)+1, (uint8_t*)(&cfm.gw)+2,(uint8_t*)(&cfm.gw)+3);
    sscanf(mk, "%d.%d.%d.%d",
        (uint8_t*)(&cfm.mk), (uint8_t*)(&cfm.mk)+1, (uint8_t*)(&cfm.mk)+2,(uint8_t*)(&cfm.mk)+3);

    CTRL_IF_CMD("%s,ip=%x,gw=%x,mk=%x\n",__func__,cfm.ip,cfm.gw,cfm.mk);
    CTRL_IF_CMD("%s,ip char=%s,gw=%s,mk=%s\n",__func__,ip,gw,mk);

    cfm.status = status;

    return cif_bk_send_event(BK_EVT_STOP_AP_IND, (uint8_t *)&cfm, sizeof(cfm));
}

bk_err_t cif_handle_bk_cmd_scan_wifi_req(struct bk_msg_hdr *msg)
{
    struct bk_msg_scan_start_req *req = (struct bk_msg_scan_start_req*) (msg + 1);

    cif_bk_cmd_confirm(msg, NULL, 0);

    wifi_scan_config_t scan_config = {0};
    int len = os_strlen((const char *)req->ssid);

    if(0 == len)
    {
        CTRL_IF_CMD("%s,len 0\n",__func__);
        BK_LOG_ON_ERR(bk_wifi_scan_start(NULL));
    }
    else
    {
        CTRL_IF_CMD("%s,ssid %s\n",__func__,req->ssid);
        os_strncpy(scan_config.ssid, (char *)req->ssid, WIFI_SSID_STR_LEN);
        BK_LOG_ON_ERR(bk_wifi_scan_start(&scan_config));
    }

    return BK_OK;

}

bk_err_t cif_handle_bk_cmd_bcn_cc_ind(uint8_t *cc, uint8_t cc_len)
{
    bk_err_t ret = BK_OK;

    ret = cif_bk_send_event(BK_EVT_BCN_CC_RXED, cc, cc_len);

    return ret;
}


bk_err_t cif_handle_bk_cmd_csi_info_ind(void *data)
{
    bk_err_t ret = BK_OK;

    ret = cif_bk_send_event(BK_EVT_CSI_INFO_IND, (uint8_t *)(data), sizeof(struct wifi_csi_info_t));

    return ret;
}


bk_err_t cif_send_exit_sleep_cfm(void)
{
    return BK_OK;
}
bk_err_t cif_handle_bk_cmd_enter_sleep_cfm(void)
{
    return BK_OK;
}
bk_err_t cif_handle_bk_cmd_enter_sleep_req(struct bk_msg_hdr *msg)
{
    CTRL_IF_CMD("%s\n",__func__);
    rtos_start_oneshot_timer(&(cif_env.enter_lv_timer));
    return BK_OK;
}

bk_err_t cif_handle_bk_cmd_exit_sleep_req(struct bk_msg_hdr *msg)
{
    return BK_OK;
}
bk_err_t cif_handle_bk_cmd_at_req(struct bk_msg_hdr *msg)
{
    CTRL_IF_CMD("%s\n",__func__);
    //need copy this buffer
#if CONFIG_AT_SUPPORT_SDIO
    extern void atsvr_rx_indicate(u8* pbuf,u16 buf_len);
    atsvr_rx_indicate((u8*)(msg+1)+1,(u16)*((u8*)(msg+1)));
#endif
    return BK_OK;

}
bk_err_t cif_handle_bk_cmd_at_rsp(void *payload, uint16_t len, struct bk_msg_hdr *msg)
{
    return cif_bk_cmd_confirm(msg, payload, len);
}
bk_err_t cif_handle_bk_cmd_at_ind(void *payload, uint16_t len)
{
    uint32_t buf_len = sizeof(struct cpdu_t) + sizeof(struct bk_rx_msg_hdr) + len;
    struct ctrl_cmd_hdr * buf = NULL;

    CTRL_IF_CMD("%s\n",__func__);
    buf = os_malloc(buf_len);
    if (buf == NULL)
    {
        CTRL_IF_CMD("Failed to allocate memory\n");
        return BK_FAIL;
    }
    buf->co_hdr.length = buf_len;
    buf->co_hdr.type = RX_BK_CMD_DATA;
    buf->msg_hdr.id = BK_EVT_CONTROLLER_AT_IND;
    memcpy(buf->msg_hdr.param,payload,len);
    
    if(!cif_msg_sender((struct common_header*)&buf->co_hdr,CIF_TASK_MSG_EVT,0))
    {
        os_free(buf);
        return BK_FAIL;
    }

    return BK_OK;
}
bk_err_t cif_handle_bk_cmd_keepalive_cfg_req(struct bk_msg_hdr *msg)
{
    struct bk_msg_keepalive_cfg_req *req = (struct bk_msg_keepalive_cfg_req*) (msg + 1);
    CTRL_IF_CMD("%s, ip = %s, port= %d\n",__func__, req->server,req->port);
    cif_bk_cmd_confirm(msg, NULL, 0);
    if (cif_env.keepalive_handler)
        cif_env.keepalive_handler(req);
    return BK_OK;

}
bk_err_t cif_send_customer_event(uint8_t *data, uint16_t len)
{
    return cif_bk_send_event(BK_EVT_CUSTOMER_IND, data, len);
}
bk_err_t cif_send_customer_cmd_cfm(uint8_t *data, uint16_t len, struct bk_msg_hdr *msg)
{
    return cif_bk_cmd_confirm(msg, data, len);
}

bk_err_t cif_handle_bk_cmd_customer_data_req(struct bk_msg_hdr *msg)
{
    bk_err_t ret = BK_OK;
    //CTRL_IF_CMD("%s, len:%d\n",__func__, msg->len);

    //stack_mem_dump((uint32_t)data, (uint32_t)data + msg->len);
    if (cif_env.customer_msg_cb)
        ret = cif_env.customer_msg_cb(msg);

    return ret;

}

bk_err_t cif_handle_bk_cmd_wifi_mmd_cfg_req(struct bk_msg_hdr *msg)
{
    int32_t ret = 0;
    bool wifi_mmd_en = false;

    wifi_mmd_en = *(bool *)(msg + 1);
    CTRL_IF_CMD("%s wifi_mmd_en:%d\n",__func__, wifi_mmd_en);
    if (wifi_mmd_en)
    {
        bk_wifi_set_wifi_media_mode(true);
    }
    else
    {
        bk_wifi_set_wifi_media_mode(false);
    }
    cif_bk_cmd_confirm(msg, NULL, 0);
    return ret;
}

bk_err_t cif_handle_bk_cmd_set_netinfo_req(struct bk_msg_hdr *msg)
{
    int32_t ret = 0;
    struct bk_msg_net_info_req *req = (struct bk_msg_net_info_req *)(msg + 1);
    netif_ip4_config_t config = {0};

    CTRL_IF_CMD("config ip:%s mask:%s\n", req->ip, req->mask);
    CTRL_IF_CMD("config gw:%s dns:%s\n", req->gw, req->dns);

    os_strncpy(config.ip, req->ip, NETIF_IP4_STR_LEN);
    os_strncpy(config.mask, req->mask, NETIF_IP4_STR_LEN);
    os_strncpy(config.gateway, req->gw, NETIF_IP4_STR_LEN);
    os_strncpy(config.dns, req->dns, NETIF_IP4_STR_LEN);

    if (sta_ip_is_start())
    {
        sta_ip_down();
        sta_ip_mode_set(0);
        bk_netif_set_ip4_config(NETIF_IF_STA, &config);
        sta_ip_start();
    }
    else
    {
        bk_netif_set_ip4_config(NETIF_IF_STA, &config);
    }

    cif_bk_cmd_confirm(msg, NULL, 0);
    return ret;
}

bk_err_t cif_handle_bk_cmd_set_media_mode_req(struct bk_msg_hdr *msg)
{
    bool media_mode = false;

    media_mode = *(bool *)(msg + 1);
    CIF_LOGV("%s media_mode:%d\n",__func__, media_mode);
    bk_wifi_set_wifi_media_mode(media_mode);

    cif_bk_cmd_confirm(msg, NULL, 0);

    return BK_OK;
}

bk_err_t cif_handle_bk_cmd_set_media_quality_req(struct bk_msg_hdr *msg)
{
    uint8_t media_quality = 0xFF;

    media_quality = *(bool *)(msg + 1);
    CIF_LOGV("%s media_quality:%d\n",__func__, media_quality);

    if (media_quality != 0xFF)
        bk_wifi_set_video_quality(media_quality);

    cif_bk_cmd_confirm(msg, NULL, 0);

    return BK_OK;
}

bk_err_t cif_handle_bk_cmd_set_autoconnect_req(struct bk_msg_hdr *msg)
{
    int32_t ret = 0;
    bool ar_en = false;

    ar_en = *(bool *)(msg + 1);
    CTRL_IF_CMD("%s auto reconnect:%d\n",__func__, ar_en);

    cif_sta_config.disable_auto_reconnect_after_disconnect = !ar_en;
    //#if CONFIG_STA_AUTO_RECONNECT
    wlan_auto_reconnect_t ar;
    /* set auto reconnect parameters */
    ar.max_count = cif_sta_config.auto_reconnect_count;
    ar.timeout = cif_sta_config.auto_reconnect_timeout;
    ar.disable_reconnect_when_disconnect = cif_sta_config.disable_auto_reconnect_after_disconnect;

    wlan_sta_set_autoreconnect(&ar);
    //#endif

    cif_bk_cmd_confirm(msg, NULL, 0);
    return ret;
}


bk_err_t cif_handle_bk_cmd_set_coex_csa_req(struct bk_msg_hdr *msg)
{
    int32_t ret = 0;
    bool close_coex_csa = *(bool *)(msg + 1);

    CIF_LOGV("%s close coex csa:%d\n",__func__, close_coex_csa);

    ret = bk_wifi_set_csa_coexist_mode_flag(close_coex_csa);

    cif_bk_cmd_confirm(msg, NULL, 0);

    return ret;
}
bk_err_t cif_handle_bk_cmd_interface_debug(struct bk_msg_hdr *msg)
{
    bk_err_t ret = BK_OK;
    cif_print_debug_info();
    return ret;
}
#if CONFIG_CONTROLLER_AP_BUFFER_COPY
/* CP-side static snapshot of lwIP/heap addresses. It lives in CP SRAM (mapped
 * non-cacheable on the AP side), so AP reads it coherently via the pointer
 * published in sys_sw_regs. heap_free_addr is intentionally left 0 here: it is
 * published separately by the heap itself (cp_heap_size_ptr) to keep the OS
 * heap internals decoupled from the controller. */
static cp_mem_addr_info_t s_cp_mem_addr_info = {0};

void cif_publish_mem_addr(void)
{
    s_cp_mem_addr_info.magic = CP_MEM_SNAPSHOT_MAGIC;
    s_cp_mem_addr_info.version = CP_MEM_SNAPSHOT_VERSION;
    s_cp_mem_addr_info.size = sizeof(cp_mem_addr_info_t);
    s_cp_mem_addr_info.heap_value_size = sizeof(size_t);
    s_cp_mem_addr_info.heap_free_addr = 0; /* filled by AP from cp_heap_size_ptr */
    s_cp_mem_addr_info.heap_total = rtos_get_total_heap_size();
    s_cp_mem_addr_info.heap_min_rsv_addr = (uint32_t)&g_wifi_mac_config.min_rsv_mem;
    s_cp_mem_addr_info.heap_min_rsv_value_size = sizeof(g_wifi_mac_config.min_rsv_mem);
#if MEM_STATS
    s_cp_mem_addr_info.lwip_mem_value_size = sizeof(lwip_stats.mem.used);
    s_cp_mem_addr_info.lwip_used_addr = (uint32_t)&lwip_stats.mem.used;
    s_cp_mem_addr_info.lwip_avail_addr = (uint32_t)&lwip_stats.mem.avail;
#if MEM_TRX_DYNAMIC_EN
    s_cp_mem_addr_info.lwip_tx_used_addr = (uint32_t)&lwip_stats.mem.tx_used;
    s_cp_mem_addr_info.lwip_tx_avail_addr = (uint32_t)&lwip_stats.mem.tx_avail;
#else
    s_cp_mem_addr_info.lwip_tx_used_addr = (uint32_t)&lwip_stats.mem.used;
    s_cp_mem_addr_info.lwip_tx_avail_addr = (uint32_t)&lwip_stats.mem.avail;
#endif
#endif
    /* Publish the snapshot address; AP reads it from shared memory at init,
     * so no IPC command exchange is needed at boot. */
    bk_sys_sw_regs_set_cp_lwip_mem_info_ptr((uint32_t)&s_cp_mem_addr_info);

    CIF_LOGI("[cp_mem_addr] publish @0x%x magic:0x%x ver:%u size:%u (MEM_STATS=%d TRX_DYN=%d)\n",
             (uint32_t)&s_cp_mem_addr_info, s_cp_mem_addr_info.magic,
             s_cp_mem_addr_info.version, s_cp_mem_addr_info.size, MEM_STATS, MEM_TRX_DYNAMIC_EN);
    CIF_LOGI("[cp_mem_addr] lwip val_sz:%u used:0x%x avail:0x%x tx_used:0x%x tx_avail:0x%x\n",
             s_cp_mem_addr_info.lwip_mem_value_size, s_cp_mem_addr_info.lwip_used_addr,
             s_cp_mem_addr_info.lwip_avail_addr, s_cp_mem_addr_info.lwip_tx_used_addr,
             s_cp_mem_addr_info.lwip_tx_avail_addr);
    CIF_LOGI("[cp_mem_addr] heap val_sz:%u total:%u min_rsv:0x%x rsv_sz:%u\n",
             s_cp_mem_addr_info.heap_value_size, s_cp_mem_addr_info.heap_total,
             s_cp_mem_addr_info.heap_min_rsv_addr, s_cp_mem_addr_info.heap_min_rsv_value_size);
}
#endif
bk_err_t cif_handle_wifi_ctrnl_cmd(struct bk_msg_hdr *msg)
{
    bk_err_t ret = BK_OK;

    switch(msg->cmd_id)
    {
        case BK_CMD_CONNECT:
        {
            cif_env.host_wifi_init = true;
            ret = cif_handle_bk_cmd_connect_req(msg);
            break;
        }
        case BK_CMD_DISCONNECT:
        {
            ret = cif_handle_bk_cmd_disconnect_req(msg);
            break;
        }
        case BK_CMD_SET_MAC_ADDR:
        {
            ret = cif_handle_bk_cmd_set_mac_addr_req(msg);
            break;
        }
        case BK_CMD_GET_MAC_ADDR:
        {
            ret = cif_handle_bk_cmd_get_mac_addr_req(msg);
            break;
        }
        case BK_CMD_START_AP:
        {
            cif_env.host_wifi_init = true;
            ret = cif_handle_bk_cmd_start_ap_req(msg);
            break;
        }
        case BK_CMD_STOP_AP:
        {
            ret = cif_handle_bk_cmd_stop_ap_req(msg);
            break;
        }
        case BK_CMD_ENTER_SLEEP:
        {
            ret = cif_handle_bk_cmd_enter_sleep_req(msg);
            break;
        }
        case BK_CMD_EXIT_SLEEP:
        {
            ret = cif_handle_bk_cmd_exit_sleep_req(msg);
            break;
        }
        case BK_CMD_GET_WLAN_STATUS:
        {
            ret = cif_handle_bk_cmd_get_wlan_status_req(msg);
            break;
        }

        case BK_CMD_SCAN_WIFI:
        {
            cif_env.host_wifi_init = true;
            ret = cif_handle_bk_cmd_scan_wifi_req(msg);
            break;
        }

        case BK_CMD_CONTROLLER_AT:
        {
            ret = cif_handle_bk_cmd_at_req(msg);
            break;
        }
        case BK_CMD_KEEPALIVE_CFG:
        {
            ret = cif_handle_bk_cmd_keepalive_cfg_req(msg);
            break;
        }
#if (CONFIG_NTP_SYNC_RTC)
        case BK_CMD_SET_TIME:
        {
            ret = cif_handle_bk_cmd_set_time_req(msg);
            break;
        }
        case BK_CMD_GET_TIME:
        {
            ret = cif_handle_bk_cmd_get_time_req(msg);
            break;
        }
#endif
        case BK_CMD_CUSTOMER_DATA:
        {
            ret = cif_handle_bk_cmd_customer_data_req(msg);
            break;
        }
        case BK_CMD_WIFI_MMD_CONFIG:
        {
            ret = cif_handle_bk_cmd_wifi_mmd_cfg_req(msg);
            break;
        }
        case BK_CMD_SET_NET_INFO:
        {
            ret = cif_handle_bk_cmd_set_netinfo_req(msg);
            break;
        }
        case BK_CMD_SET_AUTOCONNECT:
        {
            ret = cif_handle_bk_cmd_set_autoconnect_req(msg);
            break;
        }
        case BK_CMD_SET_MEDIA_MODE:
        {
            ret = cif_handle_bk_cmd_set_media_mode_req(msg);
            break;
        }
        case BK_CMD_SET_MEDIA_QUALITY:
        {
            ret = cif_handle_bk_cmd_set_media_quality_req(msg);
            break;
        }

        case BK_CMD_SET_COEX_CSA:
        {
            ret = cif_handle_bk_cmd_set_coex_csa_req(msg);
            break;
        }
        case BK_INTERFACE_DEBUG_CMD:
        {
            ret = cif_handle_bk_cmd_interface_debug(msg);
            break;
        }
#if CONFIG_P2P
		case BK_CMD_MODEXP_RESULT:
		{
			extern void crypto_mod_exp_on_ipc_result(const uint8_t *data, uint16_t len);
			crypto_mod_exp_on_ipc_result((const uint8_t *)(msg + 1), msg->len);
			break;
		}
#endif

        default:
        {
            CIF_LOGE("%s,error CMD type %x\n",__func__, msg->cmd_id);
            ret = BK_FAIL;
            break;
        }
    }

    return ret;
}
bk_err_t cif_handle_bk_cmd(void *head)
{
    bk_err_t ret = BK_OK;
    struct bk_msg_hdr *msg = NULL;
    cpdu_t* hdr = (cpdu_t*)head;
    void* cmd = (void *)((struct cpdu_t*)head + 1);
    CIF_LOGV("%s,TX_BK_CMD_DATA \n",__func__);

    if(hdr->co_hdr.is_buf_bank)
    {
        cif_save_buffer_addr(head);
        return BK_OK;
    }


    if (cmd == NULL)
    {
        CIF_LOGE("cif_handle_bk_cmd INVALID paramter cmd:%x\n", cmd);
        BK_ASSERT(0);
    }

    msg = (struct bk_msg_hdr *)cmd;

    CIF_LOGV("cif_handle_bk_cmd cmd_id:%x\n", msg->cmd_id);
    cif_env.no_host = false;
    cif_env.host_powerup = true;
    cif_env.host_wifi_init = true;

    if ((msg->cmd_id >= BK_CMD_WIFI_API_START) && (msg->cmd_id < BK_CMD_WIFI_API_END))
    {
        ret = cif_handle_wifi_api_cmd(msg);
    }
    else
    {
        ret = cif_handle_wifi_ctrnl_cmd(msg);
    }

    //Free AP cmd buffer
    cif_free_cmd_buffer(head);
    return ret;
}

#if CONFIG_P2P
bk_err_t cif_handle_bk_cmd_p2p_go_start_ind(uint8_t *mac_addr)
{
    return cif_bk_send_event(BK_EVT_P2P_GO_START_IND, mac_addr, mac_addr ? 6 : 0);
}

bk_err_t cif_handle_bk_cmd_p2p_go_stop_ind(void)
{
    return cif_bk_send_event(BK_EVT_P2P_GO_STOP_IND, NULL, 0);
}

bk_err_t cif_send_modexp_req(const uint8_t *base, uint16_t base_len,
                              const uint8_t *exp,  uint16_t exp_len,
                              const uint8_t *mod,  uint16_t mod_len,
                              uint16_t result_max_len)
{
    cif_modexp_req_t req;
    uint8_t *p = req.data;

    req.base_len       = base_len;
    req.exp_len        = exp_len;
    req.mod_len        = mod_len;
    req.result_max_len = result_max_len;

    os_memcpy(p, base, base_len); p += base_len;
    os_memcpy(p, exp,  exp_len);  p += exp_len;
    os_memcpy(p, mod,  mod_len);

    return cif_bk_send_event(BK_EVT_MODEXP_REQ,
                             (uint8_t *)&req, sizeof(cif_modexp_req_t));
}

#endif
