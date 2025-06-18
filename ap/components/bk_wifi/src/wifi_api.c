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

#include "wifi_api.h"
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
#include "wifi_api_ipc.h"
#include "wdrv_cntrl.h"

general_param_t *g_wlan_general_param = NULL;
ap_param_t *g_ap_param_ptr = NULL;
sta_param_t *g_sta_param_ptr = NULL;
struct scan_cfg_scan_param_tag scan_param_env = {0};

static wifi_monitor_cb_t s_monitor_ap_cb = NULL;
static wifi_filter_cb_t s_filter_ap_cb = NULL;
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

bk_err_t bk_wifi_init(void)
{
    WDRV_LOGI("%s, %d\r\n", __func__, __LINE__);
    uint8_t mac[ETH_ALEN];

#ifdef CONFIG_WIFI_VNET_CONTROLLER
    wdrv_init();
#endif

    if (wifi_is_inited())
    {
        WDRV_LOGI("wifi already init!\n");
        host_wlan_remove_netif();
    }

    bk_wifi_sta_get_mac((uint8_t *)mac);
    host_wlan_add_netif(mac);
    //ToDo: AP Netif should add separated
    //bk_wifi_ap_get_mac((uint8_t *)mac);
    //host_wlan_add_netif(mac);

    wifi_set_state_bit(WIFI_INIT_BIT);
    WDRV_LOGI("wifi inited(%x)\n", s_wifi_state_bits);

    return BK_OK;
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
        WDRV_LOGI("sta already started, need stop!\n");
        bk_wifi_sta_stop();
    }

    //bk_wifi_init();

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

    struct wdrv_start_scan_req req;
    wdrv_cmd_cfm cmd_cfm;
    os_memset(&req,0,sizeof(req));

    req.cmd_hdr.cmd_id = BK_CMD_SCAN_WIFI;
    cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    cmd_cfm.cfm_id = 0;

    WDRV_LOGI("scaning\n");

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
        //wlan_sta_scan_param_t scan_param = {0};

        WDRV_LOGI("scan %s\n", config->ssid);

        uint8_t ssid_len = MIN(SSID_MAX_LEN, os_strlen((char *)config->ssid));
        os_memcpy(req.ssid, config->ssid, ssid_len);
        wdrv_tx_msg((uint8_t *)&req, sizeof(req), &cmd_cfm, NULL);

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
        wifi_clear_state_bit(WIFI_STA_STARTED_BIT);
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

bk_err_t bk_wifi_set_csa_coexist_mode_flag(bool is_close)
{
    struct wdrv_set_csa_coexist_mode_flag_req set_coex_flag_req = {0};

    if (s_wifi_state_bits & WIFI_STA_STARTED_BIT)
    {
        WDRV_LOGW("Set csa coxist mode before starting station! state_bit=0x%x\r\n",s_wifi_state_bits);
        return BK_ERR_STATE;
    }

    set_coex_flag_req.is_close = is_close;

    set_coex_flag_req.cmd_hdr.cmd_id = BK_CMD_SET_COEX_CSA;
    set_coex_flag_req.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    set_coex_flag_req.cmd_cfm.cfm_id = 0;

    wdrv_tx_msg((uint8_t *)&set_coex_flag_req, sizeof(set_coex_flag_req), &set_coex_flag_req.cmd_cfm, NULL);
    return BK_OK;
}

#if 1 //CONFIG_WIFI_SOFTAP

static inline int is_zero_ether_addr(const u8 *a)
{
    return !(a[0] | a[1] | a[2] | a[3] | a[4] | a[5]);
}

void bk_wifi_ap_init(void)
{
    WDRV_LOGI("%s, %d\r\n", __func__, __LINE__);
    uint8_t mac[ETH_ALEN];

    if(wifi_is_inited())
    {
        WDRV_LOGI("wifi already init, reinit anyway!\n");
        uap_ip_down();
        host_wlan_remove_sap_netif();
    }

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
    start_ap_req.channel = g_ap_param_ptr->chann;
    start_ap_req.hidden = g_ap_param_ptr->hidden_ssid;

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
        g_ap_param_ptr->chann = 0;
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
    uap_ip_down();
    host_wlan_remove_sap_netif();
    wifi_clear_state_bit(WIFI_AP_STARTED_BIT);
    return BK_OK;
}


int demo_softap_app_init(char *ap_ssid, char *ap_key, char *ap_channel)
{
    wifi_ap_config_t ap_config = {0};//WIFI_DEFAULT_AP_CONFIG();
    int len, key_len = 0;
    len = os_strlen(ap_ssid);

    if (ap_key)
        key_len = os_strlen(ap_key);
    if (SSID_MAX_LEN < len) {
        WDRV_LOGE("ssid name more than 32 Bytes\r\n");
        return BK_FAIL;
    }
    if (0 == len) {
        WDRV_LOGE("ssid name must not be null\r\n");
        return BK_FAIL;
    }

    if (8 > key_len)
        WDRV_LOGE("key less than 8 Bytes, the security will be set NONE\r\n");

    if (64 < key_len) {
        WDRV_LOGE("key more than 64 Bytes\r\n");
        return BK_FAIL;
    }

    os_strcpy(ap_config.ssid, ap_ssid);
    if (ap_key)
        os_strcpy(ap_config.password, ap_key);

    if (ap_channel) {
        int channel;
        char *end;

        channel = strtol(ap_channel, &end, 0);
        if (*end) {
            WDRV_LOGE("Invalid number '%s'", ap_channel);
            return BK_FAIL;
        }
        ap_config.channel = channel;
    }

    WDRV_LOGI("ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
    BK_RETURN_ON_ERR(bk_wifi_ap_set_config(&ap_config));
    BK_RETURN_ON_ERR(bk_wifi_ap_start());
    return BK_OK;
}


#endif

int demo_sta_app_init(char *oob_ssid, char *connect_key)
{
    wifi_sta_config_t sta_config = {0};
    int len;

    len = os_strlen(oob_ssid);
    if (SSID_MAX_LEN < len) {
        WDRV_LOGI("ssid name more than 32 Bytes\r\n");
        return BK_FAIL;
    }
#ifdef CONFIG_CONNECT_THROUGH_PSK_OR_SAE_PASSWORD
    if (psk) {
        sta_config.psk_len = PMK_LEN * 2;
        sta_config.psk_calculated = true;
        os_strlcpy((char *)sta_config.psk, (char *)psk, sizeof(sta_config.psk));
    }
#endif
    os_strcpy(sta_config.ssid, oob_ssid);
    if (connect_key)
        os_strcpy(sta_config.password, connect_key);
#if CONFIG_BRIDGE
    extern uint8_t bridge_is_enabled;
    extern uint8_t bridge_open;
    void bk_bridge_stop(void);
    /* Before connecting the STA to the router, if the bridge is in the enabled state,
        disconnect the bridge first. After starting the STA, if the original bridge was
        in the enabled state, maintain the original state */
    if (bridge_open && bridge_is_enabled) {
        bk_bridge_stop();
        bridge_open = true;
    }
#endif
    WDRV_LOGI("ssid:%s key:%s\r\n", sta_config.ssid, sta_config.password);

    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    BK_LOG_ON_ERR(bk_wifi_sta_start());
    return BK_OK;
}

bk_err_t bk_wifi_ap_get_config(wifi_ap_config_t *ap_config)
{
    if (!ap_config)
        return BK_ERR_NULL_PARAM;
    struct wdrv_get_ap_config_req get_req = {0};
    wifi_ap_config_t ap_config_cfm = {0};

    WDRV_LOGI("getting ap_get_config\n");
    if(ap_config == NULL)
        return BK_FAIL;

    get_req.cmd_hdr.cmd_id = BK_CMD_GET_AP_CONFIG;
    get_req.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    get_req.cmd_cfm.cfm_id = 0;

    wdrv_tx_msg((uint8_t *)&get_req, sizeof(get_req), &get_req.cmd_cfm, (uint8_t *)(&ap_config_cfm));

    if (get_req.cmd_cfm.cfm_buf)
        os_memcpy(ap_config, get_req.cmd_cfm.cfm_buf, get_req.cmd_cfm.cfm_len);
    else
        WDRV_LOGI("invalid addr\n");

    WDRV_LOGI("got ap_get_config\n");
#if 0
    os_memcpy(ap_config->ssid, g_ap_param_ptr->ssid.array, g_ap_param_ptr->ssid.length);
    os_memcpy(ap_config->password, g_ap_param_ptr->key, g_ap_param_ptr->key_len);
    ap_config->channel = g_ap_param_ptr->chann;
    ap_config->security = g_ap_param_ptr->cipher_suite;
#endif
    return BK_OK;
}


bk_err_t bk_netif_get_ip4_config_api(uint8_t ifx, uint8_t *ip4_config)
{
    if (!ip4_config)
        return BK_ERR_NULL_PARAM;
    struct wdrv_get_ip4_config_req get_ip_req = {0};

    WDRV_LOGI("getting ip4_config\n");

    get_ip_req.cmd_hdr.cmd_id = BK_CMD_GET_IP_CONFIG;
    get_ip_req.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    get_ip_req.cmd_cfm.cfm_id = 0;

    get_ip_req.flag = ifx;

    wdrv_tx_msg((uint8_t *)&get_ip_req, sizeof(get_ip_req), &get_ip_req.cmd_cfm, ip4_config);

    WDRV_LOGI("got ip4_config\n");

    return BK_OK;
}


bool wifi_netif_sta_is_got_ip_api(void)
{
    bool is_sta_got_ip = false;

    struct wdrv_get_staipup_req get_ip_req = {0};

    WDRV_LOGI("getting ip4_config\n");

    get_ip_req.cmd_hdr.cmd_id = BK_CMD_GET_STAIPUP;
    get_ip_req.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    get_ip_req.cmd_cfm.cfm_id = 0;

    wdrv_tx_msg((uint8_t *)&get_ip_req, sizeof(get_ip_req), &get_ip_req.cmd_cfm, (uint8_t *)&is_sta_got_ip);
    

    return is_sta_got_ip;
}
bool uap_ip_is_start_api(void)
{
    bool is_ap_ip_up = false;

    struct wdrv_get_apipup_req get_ip_req = {0};

    WDRV_LOGI("getting ip4_config\n");

    get_ip_req.cmd_hdr.cmd_id = BK_CMD_GET_APIPUP;
    get_ip_req.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    get_ip_req.cmd_cfm.cfm_id = 0;

    wdrv_tx_msg((uint8_t *)&get_ip_req, sizeof(get_ip_req), &get_ip_req.cmd_cfm, (uint8_t *)&is_ap_ip_up);

    return is_ap_ip_up;
}

bk_err_t bk_wifi_sta_get_link_status(wifi_link_status_t *link_status)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_link_status_t);

    if (link_status == NULL) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(STA_GET_LINK_STATUS, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(link_status, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_get_channel(void)
{
    uint8_t channel = 0;
    void *buffer_to_ipc = NULL;

    buffer_to_ipc = os_malloc(1);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    wifi_send_com_api_cmd(WIFI_GET_CHANNEL, 1, (uint32_t)buffer_to_ipc);

    channel = *(uint8_t *)buffer_to_ipc;

    WIFI_LOGI("%s: %d \n", __func__, channel);

    return channel;
}

bk_err_t bk_wifi_set_country(const wifi_country_t *country)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_country_t);

    if (country == NULL) {
        WIFI_LOGE("%s failed, invalid input param\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, country, len);
    ret = wifi_send_com_api_cmd(WIFI_SET_COUNTRY, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_get_listen_interval(uint8_t *listen_interval)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;

    buffer_to_ipc = os_malloc(1);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(STA_GET_LISTEN_INTERVAL, 1, (uint32_t)buffer_to_ipc);

    *listen_interval = *(uint8_t *)buffer_to_ipc;

    WIFI_LOGI("%s: %d \n", __func__, *listen_interval);

    return ret;
}

bk_err_t bk_wifi_send_listen_interval_req(uint8_t interval)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_LISTEN_INTERVAL, 1, interval);

    return ret;
}

bk_err_t bk_wifi_send_bcn_loss_int_req(uint8_t interval,uint8_t repeat_num)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_BCN_LOSS_INT, 2, interval, repeat_num);

    return ret;
}

bk_err_t bk_wifi_set_bcn_recv_win(uint8_t default_win, uint8_t max_win, uint8_t step)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_BCN_RECV_WIN, 3, default_win, max_win, step);

    return ret;
}

bk_err_t bk_wifi_set_bcn_loss_time(uint8_t wait_cnt, uint8_t wake_cnt)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_BCN_LOSS_TIME, 2, wait_cnt, wake_cnt);

    return ret;
}

bk_err_t bk_wifi_sta_get_linkstate_with_reason(wifi_linkstate_reason_t *info)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_linkstate_reason_t);

    if (info == NULL) {
        WIFI_LOGE("%s failed, invalid input param\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(STA_GET_LINK_STATE_WITH_REASON, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(info, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;
}


bk_err_t bk_wifi_sta_start_ex(void)
{
    //bk_wifi_init();
    return wifi_send_com_api_cmd(STA_START, 0);
}
bk_err_t bk_wifi_sta_set_config_ex(const wifi_sta_config_t *config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_sta_config_t);

    //WIFI_LOGE("%s config len:%d\r\n", __func__, len);
    if (config == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, config, len);
    ret = wifi_send_com_api_cmd(STA_SET_CONFIG, 2, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;
}
bk_err_t bk_wifi_sta_get_config_ex(wifi_sta_config_t *config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_sta_config_t);

    //WIFI_LOGE("%s config len:%d\r\n", __func__, len);
    if (config == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(STA_GET_CONFIG, 2, (uint32_t)buffer_to_ipc);

    os_memcpy(config, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_sta_pm_enable(void)
{
    return wifi_send_com_api_cmd(STA_PM_ENABLE, 0);
}

bk_err_t bk_wifi_sta_pm_disable(void)
{
    return wifi_send_com_api_cmd(STA_PM_DISABLE, 0);
}

int demo_sta_app_init_ex(char *oob_ssid, char *connect_key)
{
	wifi_sta_config_t sta_config = {0};
	int len;

	len = os_strlen(oob_ssid);
	if (SSID_MAX_LEN < len) {
		WIFI_LOGI("ssid name more than 32 Bytes\r\n");
		return BK_FAIL;
	}

	os_strcpy(sta_config.ssid, oob_ssid);
	if (connect_key)
		os_strcpy(sta_config.password, connect_key);

	WIFI_LOGI("ssid:%s key:%s\r\n", sta_config.ssid, sta_config.password);
	BK_LOG_ON_ERR(bk_wifi_sta_set_config_ex(&sta_config));
	BK_LOG_ON_ERR(bk_wifi_sta_start_ex());
	return BK_OK;
}

bk_err_t bk_wifi_monitor_start(void)
{
    return wifi_send_com_api_cmd(MONITOR_START, 0);
}

bk_err_t bk_wifi_monitor_stop(void)
{
    return wifi_send_com_api_cmd(MONITOR_STOP, 0);
}

bk_err_t bk_wifi_monitor_set_channel(const wifi_channel_t *chan)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_channel_t);

    if (chan == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, chan, len);
    ret = wifi_send_com_api_cmd(MONITOR_SET_CHANNEL, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_send_raw(uint8_t *buffer, int len)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;

    if (buffer == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, buffer, len);
    ret = wifi_send_com_api_cmd(SEND_RAW, 2, (uint32_t)buffer_to_ipc,len);

    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_manual_cal_rfcali_status(void)
{
    return wifi_send_com_api_cmd(PHY_CAL_RFCALI, 0);
}

bk_err_t bk_wifi_capa_config(wifi_capability_t capa_id, uint32_t capa_val)
{
    return wifi_send_com_api_cmd(WIFI_CAPA_CONFIG, 2, (uint32_t)capa_id,capa_val);
}

bk_err_t bk_wifi_set_mac_address(char *mac)
{
    struct wdrv_set_mac_req set_mac_req;
    set_mac_req.cmd_hdr.cmd_id = BK_CMD_SET_MAC_ADDR;
    os_memcpy(set_mac_req.mac_addr, mac, 6);
    set_mac_req.cmd_cfm.waitcfm = WDRV_CMD_NOWAITCFM;
    set_mac_req.cmd_cfm.cfm_id = 0;
    WDRV_LOGD("set mac addr: %02X:%02X:%02X:%02X:%02X:%02X", set_mac_req.mac_addr[0], set_mac_req.mac_addr[1],
              set_mac_req.mac_addr[2], set_mac_req.mac_addr[3], set_mac_req.mac_addr[4], set_mac_req.mac_addr[5]);
    wdrv_tx_msg((uint8_t *)&set_mac_req, sizeof(set_mac_req), &set_mac_req.cmd_cfm, NULL);

    return BK_OK;
}

bk_err_t bk_wifi_scan_start_ex(const wifi_scan_config_t *scan_config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_scan_config_t);

    WDRV_LOGI("scaning\n");

    //WIFI_LOGE("%s config len:%d\r\n", __func__, len);
    if (scan_config == NULL) {
        return wifi_send_com_api_cmd(SCAN_START, 1, 0);
    } else {
        buffer_to_ipc = os_malloc(len);
        if (!buffer_to_ipc)
        {
            WIFI_LOGE("%s malloc failed\r\n", __func__);
            return BK_ERR_NO_MEM;
        }

        os_memcpy(buffer_to_ipc, scan_config, len);

        ret = wifi_send_com_api_cmd(SCAN_START, 1, (uint32_t)buffer_to_ipc);

        os_free(buffer_to_ipc);

        return ret;
    }
}

bk_err_t bk_wifi_scan_stop(void)
{
    return wifi_send_com_api_cmd(SCAN_STOP, 0);
}

bk_err_t bk_wifi_scan_get_result(wifi_scan_result_t *scan_result)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_scan_result_t);

    if (scan_result == NULL) {
        WIFI_LOGE("%s failed, invalid scan_result\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(SCAN_RESULT, 1, (uint32_t)buffer_to_ipc);
    os_memcpy(scan_result, buffer_to_ipc, len);

    os_free(buffer_to_ipc);

    return ret;
}

static const char *wifi_sec_type_string_api(wifi_security_t security)
{
	switch (security) {
	case WIFI_SECURITY_NONE:
		return "NONE";
	case WIFI_SECURITY_WEP:
		return "WEP";
	case WIFI_SECURITY_WPA_TKIP:
		return "WPA-TKIP";
	case WIFI_SECURITY_WPA_AES:
		return "WPA-AES";
	case WIFI_SECURITY_WPA_MIXED:
		return "WPA-MIX";
	case WIFI_SECURITY_WPA2_TKIP:
		return "WPA2-TKIP";
	case WIFI_SECURITY_WPA2_AES:
		return "WPA2-AES";
	case WIFI_SECURITY_WPA2_MIXED:
		return "WPA2-MIX";
	case WIFI_SECURITY_WPA3_SAE:
		return "WPA3-SAE";
	case WIFI_SECURITY_WPA3_WPA2_MIXED:
		return "WPA3-WPA2-MIX";
	case WIFI_SECURITY_EAP:
		return "EAP";
	case WIFI_SECURITY_OWE:
		return "OWE";
	case WIFI_SECURITY_AUTO:
		return "AUTO";
#ifdef CONFIG_WAPI_SUPPORT
	case WIFI_SECURITY_TYPE_WAPI_PSK:
		return "WAPI_PSK";
	case WIFI_SECURITY_TYPE_WAPI_CERT:
		return "WAPI_CERT";
#endif
	default:
		return "UNKNOWN";
	}
}

static void wifi_scan_dump_ap(const wifi_scan_ap_info_t *ap)
{
    const char *security_str = wifi_sec_type_string_api(ap->security);
#if (CONFIG_SHELL_ASYNCLOG)
    shell_cmd_ind_out("%-32s " BK_MAC_FORMAT "   %4d %2d %s\r\n",
               ap->ssid, BK_MAC_STR(ap->bssid), (int8_t)ap->rssi, ap->channel, security_str);
#else
    WIFI_LOG_RAW("%-32s " BK_MAC_FORMAT "   %4d %2d %s\n",
               ap->ssid, BK_MAC_STR(ap->bssid), (int8_t)ap->rssi, ap->channel, security_str);
#endif
}

bk_err_t bk_wifi_scan_dump_result(const wifi_scan_result_t *scan_result)
{
    int i;

    if (!scan_result) {
#if (CONFIG_SHELL_ASYNCLOG)
        shell_cmd_ind_out("scan doesn't found AP\n");
#else
        WIFI_LOGI("scan doesn't found AP\n");
#endif
        return BK_OK;
    }

    if ((scan_result->ap_num > 0) && (!scan_result->aps)) {
        WIFI_LOGE("scan number is %d, but AP info is NULL\n", scan_result->ap_num);
        return BK_ERR_PARAM;
    }
#if (CONFIG_SHELL_ASYNCLOG)
    shell_cmd_ind_out("scan found %d AP\r\n", scan_result->ap_num);
    shell_cmd_ind_out("%32s %17s   %4s %4s %s\r\n", "              SSID              ",
               "      BSSID      ", "RSSI", "chan", "security");
    shell_cmd_ind_out("%32s %17s   %4s %4s %s\r\n", "--------------------------------",
               "-----------------", "----", "----", "---------\n");
#else
    WIFI_LOGI("scan found %d AP\n", scan_result->ap_num);
    WIFI_LOG_RAW("%32s %17s   %4s %4s %s\n", "              SSID              ",
               "      BSSID      ", "RSSI", "chan", "security");
    WIFI_LOG_RAW("%32s %17s   %4s %4s %s\n", "--------------------------------",
               "-----------------", "----", "----", "---------\n");
#endif
    for (i = 0; i < scan_result->ap_num; i++) {
        wifi_scan_dump_ap(&scan_result->aps[i]);
        rtos_delay_milliseconds(10);
    }

    //WIFI_LOG_RAW("\n");

    return BK_OK;
}

void bk_wifi_scan_free_result(wifi_scan_result_t *scan_result)
{
    if (scan_result) {
        os_free(scan_result->aps);
        scan_result->aps = 0;
        scan_result->ap_num = 0;
    }
    WIFI_LOGD("scan free result\n");
}
//MONITOR
bk_err_t bk_wifi_monitor_register_cb(const wifi_monitor_cb_t monitor_cb)
{
    s_monitor_ap_cb = monitor_cb;
    return wifi_send_com_api_cmd(MONITOR_REGISTER_CB, 0);
}

wifi_monitor_cb_t bk_wifi_monitor_get_cb(void)
{
	return s_monitor_ap_cb;
}

bk_err_t bk_wifi_monitor_register_ind(uint8_t * msg_payload)
{
    struct monitor_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    bk_err_t ret = BK_OK;
    const uint8_t *frame = NULL;
    uint32_t len;
    const wifi_frame_info_t *frame_info;
    wifi_filter_cb_t cb = bk_wifi_monitor_get_cb();
    //uint8_t *payload_temp =  NULL;
    struct pbuf* pbuf = NULL;
    cpdu_t * cpdu = NULL;
    uint8_t vif_idx = 0;

    pbuf = (struct pbuf*)((uint8_t*)msg_payload + sizeof(uint32_t) - sizeof(cpdu_t) - sizeof(wdrv_rx_msg) - sizeof(struct pbuf));
    struct monitor_struct* mon_hdr = (struct monitor_struct*)(pbuf + 1);
    
    len = mon_hdr->para[0];
    frame = (const uint8_t*)mon_hdr->para[2];
    frame_info = (const wifi_frame_info_t *)mon_hdr->para[1];

    if(cb)
    {
        cb(frame,len,frame_info);//This payload will free in next line.
    }

    cpdu = (cpdu_t*)(pbuf + 1);
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(pbuf,vif_idx);//vif null, just for free this RXC pbuf
    return ret;
}
//Filter
bk_err_t bk_wifi_filter_register_cb(const wifi_filter_cb_t filter_cb)
{
    s_filter_ap_cb = filter_cb;
    return wifi_send_com_api_cmd(FILTER_REGISTER_CB, 0);
}

wifi_filter_cb_t bk_wifi_filter_get_cb(void)
{
	return s_filter_ap_cb;
}
bk_err_t bk_wifi_filter_register_ind(uint8_t * msg_payload)
{
    struct filter_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    bk_err_t ret = BK_OK;
    const uint8_t *frame = NULL;
    uint32_t len;
    const wifi_frame_info_t *frame_info;
    wifi_filter_cb_t cb = bk_wifi_filter_get_cb();
    //uint8_t *payload_temp =  NULL;
    struct pbuf* pbuf = NULL;
    cpdu_t * cpdu = NULL;
    uint8_t vif_idx = 0;

    pbuf = (struct pbuf*)((uint8_t*)msg_payload + sizeof(uint32_t) - sizeof(cpdu_t) - sizeof(wdrv_rx_msg) - sizeof(struct pbuf));
    struct filter_struct* filter_hdr = (struct filter_struct*)(pbuf + 1);
    
    len = filter_hdr->para[0];
    frame = (const uint8_t*)filter_hdr->para[2];
    frame_info = (const wifi_frame_info_t *)filter_hdr->para[1];
    //bk_mem_dump("ap",(uint32_t)pbuf,200);
    if(cb)
    {
        cb(frame,len,frame_info);//This payload will free in next line.
    }

    cpdu = (cpdu_t*)(pbuf + 1);
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(pbuf,vif_idx);//vif null, just for free this RXC pbuf
    return ret;
}
