/*
 * Copyright 2020-2025 Beken
 *
 * @file wdrv_api.c 
 * 
 * @brief Beken Wi-Fi Driver Command Control Entry
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "wdrv_api.h"
#include "wdrv_main.h"
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <stdint.h>
#include <stdbool.h>
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>
#include "net.h"
#include "wdrv_cntrl.h"
#include "wdrv_co_list.h"
#include "wdrv_tx.h"

general_param_t *g_wlan_general_param = NULL;
ap_param_t *g_ap_param_ptr = NULL;
sta_param_t *g_sta_param_ptr = NULL;
struct scan_cfg_scan_param_tag scan_param_env = {0};

/* State Indication */
static uint16_t s_wifi_state_bits = 0;
static inline void wifi_set_state_bit(uint16_t state_bit)
{
    wifi_lock();
    s_wifi_state_bits |= state_bit;
    wifi_unlock();
}

static inline void wifi_clear_state_bit(uint16_t state_bit)
{
    wifi_lock();
    s_wifi_state_bits &= ~state_bit;
    wifi_unlock();
}

static inline bool wifi_is_inited(void)
{
    return (s_wifi_state_bits & WIFI_INIT_BIT);
}

bool wifi_sta_is_started(void)
{
    return (s_wifi_state_bits & WIFI_STA_STARTED_BIT);
}

static inline bool wifi_sta_is_connected(void)
{
    return (s_wifi_state_bits & WIFI_STA_CONNECTED_BIT);
}

bool wifi_ap_is_started(void)
{
    return (s_wifi_state_bits & WIFI_AP_STARTED_BIT);
}

static inline bool wifi_sta_is_configured(void)
{
    return (s_wifi_state_bits & WIFI_STA_CONFIGURED_BIT);
}

static inline bool wifi_ap_is_configured(void)
{
    return (s_wifi_state_bits & WIFI_AP_CONFIGURED_BIT);
}

/* API Reference */
static int wifi_sta_validate_config(const wifi_sta_config_t *config)
{
    if (!config) {
        WDRV_LOGD("sta config fail, null config\n");
        return BK_ERR_NULL_PARAM;
    }

    //TODO more check
    return BK_OK;
}


//TODO optimize param_config.c
//Init global STA configurations
static int wifi_sta_init_global_config(void)
{
    BK_ASSERT(g_sta_param_ptr); /* ASSERT VERIFIED */
    BK_ASSERT(g_wlan_general_param); /* ASSERT VERIFIED */

    bk_wifi_sta_get_mac((uint8_t *)(&g_sta_param_ptr->own_mac));
    g_wlan_general_param->role = CONFIG_ROLE_STA;
    WDRV_LOGI("wdrv mac addr:"BK_MAC_FORMAT"\r\n", BK_MAC_STR(g_sta_param_ptr->own_mac) );

    return BK_OK;
}

static int wifi_scan_init_global_config(void)
{
    BK_ASSERT(g_sta_param_ptr); /* ASSERT VERIFIED */
    BK_ASSERT(g_wlan_general_param); /* ASSERT VERIFIED */

    bk_wifi_sta_get_mac((uint8_t *)(&g_sta_param_ptr->own_mac));

    return BK_OK;
}

//Set STA configuration to global configuration
static int wifi_sta_set_global_config(const wifi_sta_config_t *config)
{
    g_sta_param_ptr->ssid.length = MIN(SSID_MAX_LEN, os_strlen(config->ssid));
    memcpy(g_sta_param_ptr->ssid.array, config->ssid, g_sta_param_ptr->ssid.length);

    g_sta_param_ptr->cipher_suite = config->security;

    g_sta_param_ptr->key_len = os_strlen(config->password);
    os_memcpy(g_sta_param_ptr->key, config->password, g_sta_param_ptr->key_len);
    g_sta_param_ptr->key[g_sta_param_ptr->key_len] = 0;

    WDRV_LOGI("sta config, ssid=%s password=%s security=%d\n",
                      config->ssid, config->password, config->security);
    return BK_OK;
}

static int wifi_sta_get_global_config(wifi_sta_config_t *sta_config)
{
    if (!sta_config)
        return BK_ERR_NULL_PARAM;

    os_memset(sta_config, 0, sizeof(sta_config));
    os_memcpy(sta_config->ssid, g_sta_param_ptr->ssid.array, g_sta_param_ptr->ssid.length);

    os_memcpy(sta_config->password, g_sta_param_ptr->key, g_sta_param_ptr->key_len);

    WDRV_LOGD("sta get sta_config, ssid=%s password=%s security=%d\n",
                  sta_config->ssid, sta_config->password, sta_config->security);

    return BK_OK;
}

bk_err_t bk_wifi_sta_set_config(const wifi_sta_config_t *config)
{
    int ret = BK_OK;

    WDRV_LOGI("sta configuring\n");

//    if (!wifi_is_inited()) {
//        WDRV_LOGD("set sta config fail, wifi not init\n");
//        return BK_ERR_WIFI_NOT_INIT;
//    }

    ret = wifi_sta_validate_config(config);
    if (ret != BK_OK) {
        WDRV_LOGI("set config fail, invalid param\n");
        return ret;
    }

    wifi_sta_set_global_config(config);

    wifi_set_state_bit(WIFI_STA_CONFIGURED_BIT);
    WDRV_LOGI("sta configured(%x)\n", s_wifi_state_bits);

    return BK_OK;
}


bk_err_t bk_wifi_sta_get_config(wifi_sta_config_t *config)
{
    if (!config) {
        WDRV_LOGD("get sta config fail, null config");
        return BK_ERR_NULL_PARAM;
    }

    os_memset(config, 0, sizeof(config));

    if (!wifi_sta_is_configured())
        return BK_ERR_WIFI_STA_NOT_CONFIG;

    os_memcpy(config->ssid, g_sta_param_ptr->ssid.array, g_sta_param_ptr->ssid.length);

    os_memcpy(config->password, g_sta_param_ptr->key, g_sta_param_ptr->key_len);
    config->password[g_sta_param_ptr->key_len] = 0;

    //TODO get channel and security type
    return BK_OK;
}

void bk_wifi_init(void)
{
    WDRV_LOGI("%s, %d\r\n", __func__, __LINE__);
    uint8_t mac[ETH_ALEN];

    if (wifi_is_inited())
        WDRV_LOGI("wifi already init, reinit anyway!\n");
    wdrv_host_env.wlan_mode                         = WIFI_MODE_IDLE;
    wdrv_host_env.wlan_link_sta_status              = WIFI_LINKSTATE_STA_DISCONNECTED;
    wdrv_host_env.ap_status_cfm.status              = CP_SOFTAP_CLOSE;
    bk_wifi_sta_get_mac((uint8_t *)mac);
    host_wlan_add_netif(mac);
    //ToDo: AP Netif should add separated
    //bk_wifi_ap_get_mac((uint8_t *)mac);
    //host_wlan_add_netif(mac);

    wifi_set_state_bit(WIFI_INIT_BIT);
    WDRV_LOGI("wifi inited(%x)\n", s_wifi_state_bits);
}

bk_err_t bk_wifi_sta_start(void)
{
    WDRV_LOGI("sta starting\n");

    if (!wifi_sta_is_configured()) {
        WDRV_LOGI("sta start fail, sta not configured\n");
        return BK_ERR_WIFI_STA_NOT_CONFIG;
    }

    wifi_sta_init_global_config();

    if (wifi_sta_is_started()) {
        WDRV_LOGI("sta already started, ignored!\n");
        return BK_OK;
    }

    bk_wifi_init();

    wifi_set_state_bit(WIFI_STA_STARTED_BIT);
    WDRV_LOGD("sta started(%x)\n", s_wifi_state_bits);

    /* always connect the AP automatically */
    bk_wifi_sta_connect();

    return BK_OK;
}


bk_err_t bk_wifi_sta_stop(void)
{
    WDRV_LOGI("sta stopping\n");

    if (!wifi_sta_is_started()) {
        WDRV_LOGD("sta stop, already stopped\n");
        return BK_OK;
    }

    bk_wifi_sta_disconnect();

#if CONFIG_LWIP
    host_wlan_remove_netif();
#endif

    wifi_clear_state_bit(WIFI_STA_STARTED_BIT);
    WDRV_LOGI("sta stopped(%x)\n", s_wifi_state_bits);

    return BK_OK;
}

bk_err_t bk_wifi_scan_start(const wifi_scan_config_t *config)
{
    u8 ssid_len = 0;

    wdrv_cmd_hdr req;
    wdrv_cmd_cfm cmd_cfm;
    req.cmd_id = BK_CMD_SCAN_WIFI;
    cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    cmd_cfm.cfm_id = 0;

    WDRV_LOGI("scaning\n");

    if (!wifi_is_inited()) {
        WDRV_LOGD("start scan fail, wifi not init\n");
        return BK_ERR_WIFI_NOT_INIT;
    }

    wifi_scan_init_global_config();

    wifi_set_state_bit(WIFI_PURE_SCAN_STARTED_BIT);

    if (config ) {
        ssid_len = MIN(SSID_MAX_LEN, os_strlen((char *)config->ssid));

        if((0 != config->scan_type) ||(0 != config->chan_cnt) ||(0 != config->duration)) {
            scan_param_env.set_param = 1;
            scan_param_env.scan_type = config->scan_type;
            scan_param_env.chan_cnt = config->chan_cnt;
            if(WIFI_MAX_SCAN_CHAN_DUR < config->duration) {
                WDRV_LOGW("scan duration is too long %dus,need less than 200ms\r\n",config->duration);
                scan_param_env.duration = WIFI_MAX_SCAN_CHAN_DUR * 1000;
            } else
                scan_param_env.duration = config->duration * 1000;
            os_memcpy(scan_param_env.chan_nb, config->chan_nb, config->chan_cnt);
        }
    }

    if (0 == ssid_len) {
        WDRV_LOGI("scan all APs\n");
        wdrv_tx_msg((uint8_t *)&req, sizeof(req), &cmd_cfm, NULL);
    } else {
        wlan_sta_scan_param_t scan_param = {0};

        WDRV_LOGI("scan %s\n", config->ssid);
        scan_param.num_ssids = 1;
        scan_param.ssids[0].ssid_len = MIN(SSID_MAX_LEN, os_strlen((char *)config->ssid));
        os_memcpy(scan_param.ssids[0].ssid, config->ssid, scan_param.ssids[0].ssid_len);
        //ret = wlan_sta_scan(&scan_param);
        //mailbox_send_cmd(BK_CMD_SCAN_WIFI);
    }

    return BK_OK;
}

bk_err_t bk_wifi_sta_connect(void)
{
    wifi_sta_config_t sta_config = { 0 };

    struct wdrv_connect_req connect_req = {0};

    WDRV_LOGI("sta connecting\n");

    wifi_sta_get_global_config(&sta_config);

    if (!wifi_sta_is_started()) {
        WDRV_LOGD("sta connect fail, sta not start\n");
        return BK_ERR_WIFI_STA_NOT_STARTED;
    }

    bk_wifi_sta_disconnect();

    /* Pass sta_config to CP */
    os_memcpy(connect_req.ssid, sta_config.ssid, sizeof(sta_config.ssid));
    os_memcpy(connect_req.pw, sta_config.password, sizeof(sta_config.password));

    connect_req.cmd_hdr.cmd_id = BK_CMD_CONNECT;
    connect_req.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    connect_req.cmd_cfm.cfm_id = 0;

    wdrv_tx_msg((uint8_t *)&connect_req, sizeof(connect_req), &connect_req.cmd_cfm, NULL);

    wifi_set_state_bit(WIFI_STA_CONNECTED_BIT);
    WDRV_LOGI("sta connected(%x)\n", s_wifi_state_bits);

    return BK_OK;
}

bk_err_t bk_wifi_sta_disconnect(void)
{
    wdrv_cmd_hdr req;
    wdrv_cmd_cfm cmd_cfm;
    req.cmd_id = BK_CMD_DISCONNECT;
    cmd_cfm.waitcfm = WDRV_CMD_NOWAITCFM;
    cmd_cfm.cfm_id = 0;

    WDRV_LOGI("sta disconnecting\n");

    if (wifi_sta_is_connected()) {
#if CONFIG_LWIP
        sta_ip_down();
#endif
        /* Post CMD to CIF */
        wdrv_tx_msg((uint8_t *)&req, sizeof(req), &cmd_cfm, NULL);

        //TODO do we need to post the disconnect event?
        wifi_clear_state_bit(WIFI_STA_CONNECTED_BIT);
    }

    WDRV_LOGI("sta disconnected(%x)\n", s_wifi_state_bits);
    return BK_OK;
}

bk_err_t  bk_wifi_sta_get_mac(uint8_t *mac)
{
    if (!mac)
        return BK_ERR_NULL_PARAM;

    bk_wdrv_get_mac(mac, MAC_TYPE_STA);
    return BK_OK;
}

bk_err_t bk_wifi_ap_get_mac(uint8_t *mac)
{
    if (!mac)
        return BK_ERR_NULL_PARAM;

    bk_wdrv_get_mac(mac, MAC_TYPE_AP);
    return BK_OK;
}

bk_err_t bk_wifi_set_wifi_media_mode(bool flag)
{
    struct wdrv_media_mode_req req = {0};

    req.media_flag = flag;
    req.cmd_hdr.cmd_id = BK_CMD_SET_MEDIA_MODE;
    req.cmd_cfm.waitcfm = WDRV_CMD_NOWAITCFM;
    req.cmd_cfm.cfm_id = 0;
    WDRV_LOGI("%s flag %x\n",__func__,flag);

    wdrv_tx_msg((uint8_t *)&req, sizeof(req), &req.cmd_cfm, NULL);

    return BK_OK;
}

bk_err_t bk_wifi_set_video_quality(uint8_t quality)
{
    struct wdrv_media_quality_req req = {0};

    req.media_quality = quality;
    req.cmd_hdr.cmd_id = BK_CMD_SET_MEDIA_QUALITY;
    req.cmd_cfm.waitcfm = WDRV_CMD_NOWAITCFM;
    req.cmd_cfm.cfm_id = 0;
    WDRV_LOGI("%s quality %d\n",__func__,quality);

    wdrv_tx_msg((uint8_t *)&req, sizeof(req), &req.cmd_cfm, NULL);

    return BK_OK;
}

#if CONFIG_WIFI_SOFTAP

static inline int is_zero_ether_addr(const u8 *a)
{
    return !(a[0] | a[1] | a[2] | a[3] | a[4] | a[5]);
}

void bk_wifi_ap_init(void)
{
    WDRV_LOGI("%s, %d\r\n", __func__, __LINE__);
    uint8_t mac[ETH_ALEN];

    if (wifi_is_inited())
        WDRV_LOGI("wifi already init, reinit anyway!\n");
    //ToDo
    bk_wifi_ap_get_mac((uint8_t *)mac);
    host_wlan_add_netif(mac);

    wifi_set_state_bit(WIFI_INIT_BIT);
    WDRV_LOGI("wifi inited(%x)\n", s_wifi_state_bits);

}

bk_err_t bk_wifi_ap_start(void)
{
    struct wdrv_start_ap_req start_ap_req = {0};

    WDRV_LOGD("ap starting\n");

    if (!wifi_ap_is_configured()) {
        WDRV_LOGD("start ap failed, ap not configured\n");
        return BK_ERR_WIFI_AP_NOT_CONFIG;
    }

    if (wifi_ap_is_started()) {
        WDRV_LOGD("start ap, already started, ignored\n");
        return BK_OK;
    }

#if CONFIG_LWIP
    WDRV_LOGD("ap start, ip down\n");
    //TODO move to event handler
    uap_ip_down();
#endif

    WDRV_LOGD("ap start, ap start rf\n");
#if 0
    if(wifi_ap_init_rw_driver()) {
        WDRV_LOGE("ap start fail,ap init rw driver fail!\n");
        return BK_ERR_WIFI_AP_NOT_STARTED;
    }
    //TODO return value
    if(wlan_ap_enable()) {
        WDRV_LOGE("ap start fail,ap enable fail!\n");
        return BK_ERR_WIFI_AP_NOT_STARTED;
    }
    if(wlan_ap_reload()) {
        WDRV_LOGE("ap start fail,ap reload fail!\n");
        return BK_ERR_WIFI_AP_NOT_STARTED;
    }
#endif

    bk_wifi_ap_init();

    os_memcpy(start_ap_req.ssid, g_ap_param_ptr->ssid.array, g_ap_param_ptr->ssid.length);
    os_memcpy(start_ap_req.pw, g_ap_param_ptr->key, g_ap_param_ptr->key_len);

    start_ap_req.cmd_hdr.cmd_id = BK_CMD_START_AP;
    start_ap_req.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    start_ap_req.cmd_cfm.cfm_id = 0;

    wdrv_tx_msg((uint8_t *)&start_ap_req, sizeof(start_ap_req), &start_ap_req.cmd_cfm, NULL);

    WDRV_LOGI("ap started\n");

#if CONFIG_LWIP
    //TODO move to event handler
    uap_ip_start();
#endif


    wifi_set_state_bit(WIFI_AP_STARTED_BIT);
    return BK_OK;
}

bk_err_t wifi_ap_validate_config(const wifi_ap_config_t *ap_config)
{
    if (!ap_config)
        return BK_ERR_NULL_PARAM;
#if (CONFIG_SOC_BK7239XX) && CONFIG_WIFI_BAND_5G
    if (ap_config->channel >= 36 && ap_config->channel <=165) {

        //check if configured channel is avaliable channel and no need for radat detection
        int selected_channels_size = 0;
        extern int* rw_select_5g_non_radar_avaliable_channels(int *selected_channels_size);
        int *non_radar_avaliable_channels = rw_select_5g_non_radar_avaliable_channels(&selected_channels_size);

        for (int i = 0; i < selected_channels_size; i++) {
            if (non_radar_avaliable_channels[i] == ap_config->channel)
                return BK_OK;
        }

        //TODO more parameter checking
        WDRV_LOGE("[%s]configured unavaliable or dfs channel\r\n",__FUNCTION__);
        return BK_ERR_NOT_FOUND;
    }
#endif
    return BK_OK;
}

static bk_err_t wifi_ap_set_config(const wifi_ap_config_t *ap_config)
{
    BK_ASSERT(g_ap_param_ptr); /* ASSERT VERIFIED */
    BK_ASSERT(g_wlan_general_param); /* ASSERT VERIFIED */

    if (is_zero_ether_addr((u8 *)&g_ap_param_ptr->bssid))
        bk_wifi_ap_get_mac((uint8_t *)(&g_ap_param_ptr->bssid));

    //TODO
    if ((ap_config->channel >= 1 && ap_config->channel <=14)
#if CONFIG_SOC_BK7239XX
        || (ap_config->channel >= 36 && ap_config->channel <=165)
#endif
        ) {
        g_ap_param_ptr->chann = ap_config->channel;
    } else if (ap_config->channel == 0){
        g_ap_param_ptr->chann = DEFAULT_CHANNEL_AP;
    } else {
        WDRV_LOGE("error:invalid channel\r\n");
        return BK_FAIL;
    }

    if(ap_config->max_con == 0 || ap_config->max_con > 2) {
        if(ap_config->max_con == 0)
            WDRV_LOGW("the max conn num is zero, set it is default\n");
        if(ap_config->max_con > 2)
            WDRV_LOGW("the max conn num is more than SUPPORTED_MAX_STA_NUM, set it is default\n");

        g_ap_param_ptr->max_con = 2;
    } else {
        g_ap_param_ptr->max_con = ap_config->max_con;
    }
    g_wlan_general_param->role = CONFIG_ROLE_AP;
    //TODO why need this???
    //bk_wlan_set_coexist_at_init_phase(CONFIG_ROLE_AP);

    g_ap_param_ptr->ssid.length = MIN(SSID_MAX_LEN, os_strlen(ap_config->ssid));
    os_memcpy(g_ap_param_ptr->ssid.array, ap_config->ssid, g_ap_param_ptr->ssid.length);
    g_ap_param_ptr->key_len = os_strlen(ap_config->password);
    g_ap_param_ptr->hidden_ssid = ap_config->hidden;
    if (g_ap_param_ptr->key_len < 8) {
        g_ap_param_ptr->cipher_suite = WIFI_SECURITY_NONE;
    } else {
#if CONFIG_SOFTAP_WPA3
        g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA3_WPA2_MIXED;
#else
        g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA2_AES;
#endif
        os_memset(g_ap_param_ptr->key, 0, sizeof(g_ap_param_ptr->key));
        os_memcpy(g_ap_param_ptr->key, ap_config->password, g_ap_param_ptr->key_len);
    }
#if CONFIG_AP_VSIE
    g_ap_param_ptr->vsie_len = ap_config->vsie_len;
    if (ap_config->vsie_len)
        os_memcpy(g_ap_param_ptr->vsie, ap_config->vsie, g_ap_param_ptr->vsie_len);
#endif

    g_wlan_general_param->dhcp_enable = 1;
    g_ap_param_ptr->hidden_ssid = ap_config->hidden;

    return BK_OK;
}

bk_err_t bk_wifi_ap_set_config(const wifi_ap_config_t *ap_config)
{
    int ret = BK_OK;

    WDRV_LOGI("ap configuring\n");
#if 0
    if (!wifi_is_inited()) {
        WDRV_LOGI("set ap config fail, wifi not init\n");
        return BK_ERR_WIFI_NOT_INIT;
    }
#endif
    ret = wifi_ap_validate_config(ap_config);
    if (ret != BK_OK)
        return ret;

    ret = wifi_ap_set_config(ap_config);
    if (ret != BK_OK)
        return ret;

    wifi_set_state_bit(WIFI_AP_CONFIGURED_BIT);
    WDRV_LOGI("ap configured\n");

    if (wifi_ap_is_started()) {
        BK_LOG_ON_ERR(bk_wifi_ap_stop());
        BK_LOG_ON_ERR(bk_wifi_ap_start());
    }
    return BK_OK;
}

bk_err_t bk_wifi_ap_stop(void)
{
    struct wdrv_stop_ap_req stop_ap_req = {0};
    if (!wifi_ap_is_started()) {
        WDRV_LOGI("ap stop: already stopped\n");
        return BK_OK;
    }

    stop_ap_req.cmd_hdr.cmd_id = BK_CMD_STOP_AP;
    stop_ap_req.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    stop_ap_req.cmd_cfm.cfm_id = 0;

    wdrv_tx_msg((uint8_t *)&stop_ap_req, sizeof(stop_ap_req), &stop_ap_req.cmd_cfm, NULL);

    WDRV_LOGI("ap stopped\n");
    wifi_clear_state_bit(WIFI_AP_STARTED_BIT);
    return BK_OK;
}
#endif

