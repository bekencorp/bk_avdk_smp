// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bk_wifi.h"
#include "os/os.h"

#define wifi_lock() \
	do{\
		GLOBAL_INT_DECLARATION();\
		GLOBAL_INT_DISABLE();
#define wifi_unlock() \
		GLOBAL_INT_RESTORE();\
	}while(0);

// bk_wifi_init() set the bit, bk_wifi_deinit() clear the bit
#define WIFI_INIT_BIT               (1)

// bk_wifi_sta_start() set the bit, bk_wifi_sta_stop() clear the bit
#define WIFI_STA_STARTED_BIT        (1<<1)
#define WIFI_STA_CONFIGURED_BIT     (1<<2)

// bk_wifi_sta_connect() set the bit, bk_wifi_sta_disconnect() clear the bit
// It doesn't indicate the STA is connected, it only means the API is called
// or NOT.
#define WIFI_STA_CONNECTED_BIT      (1<<3)

// bk_wifi_ap_start() set the bit, bk_wifi_ap_stop() clear the bit
#define WIFI_AP_STARTED_BIT         (1<<4)
#define WIFI_AP_CONFIGURED_BIT      (1<<5)
#define WIFI_MONITOR_STARTED_BIT    (1<<6)
#define WIFI_PURE_SCAN_STARTED_BIT  (1<<7)
#define WIFI_VIDEO_TRANSFER_STARTED_BIT  (1<<8)

#define WIFI_RESERVED_BYTE_VALUE    0

#define ENC_METHOD_NULL             1
#define ENC_METHOD_XOR              2
#define ENC_METHOD_AES              3

#define WIFI_TAG "wifi"
#define WIFI_LOGI(...) BK_LOGI(WIFI_TAG, ##__VA_ARGS__)
#define WIFI_LOGW(...) BK_LOGW(WIFI_TAG, ##__VA_ARGS__)
#define WIFI_LOGE(...) BK_LOGE(WIFI_TAG, ##__VA_ARGS__)
#define WIFI_LOGD(...) BK_LOGD(WIFI_TAG, ##__VA_ARGS__)
#define WIFI_LOGV(...) BK_LOGV(WIFI_TAG, ##__VA_ARGS__)
#define WIFI_LOG_RAW(...) BK_LOG_RAW(WIFI_TAG, ##__VA_ARGS__)

#define WIFI_VIDEO_TRANSFER_TCP_RTO     1

const char *wifi_sec_type_string(wifi_security_t security);
void wifi_sta_reg_bcn_cb(void);
void bk_wlan_set_coexist_at_init_phase(uint8_t current_role);
void bk_wifi_media_dtim(void);
bool wifi_sta_is_started(void);
bool wifi_ap_is_started(void);
void bk_wifi_ota_dtim(bool is_open);
bk_err_t bk_wifi_free_get_sta_list_memory(wlan_ap_stas_t *stas);
bk_err_t bk_wifi_p2p_get_mac(uint8_t *mac);
#if CONFIG_P2P
/* Supplicant vif: 0=infra STA MAC, 1=P2P device MAC at wpa init */
void bk_wifi_p2p_set_init_role(int p2p_device);
int bk_wifi_p2p_get_init_role(void);
struct wpa_supplicant;
int bk_wifi_p2p_ensure_supplicant_vif(struct wpa_supplicant *wpa_s, int p2p_mac);

/* Role and runtime channel (active LMAC vif; GC has no separate ap_param) */
bk_err_t bk_wifi_p2p_get_role(int *role);
uint8_t bk_wifi_p2p_go_get_channel(void);
uint8_t bk_wifi_p2p_gc_get_channel(void);
uint8_t bk_wifi_p2p_get_group_channel(void);

/* Fixed P2P device MAC (MAC_TYPE_P2P); unlike bk_wifi_p2p_get_mac() */
void bk_wifi_p2p_get_device_mac(uint8_t *mac);

/* P2P GO hostapd config (ap_param); GC reuses infra STA path */
ap_param_t *bk_wifi_p2p_go_ap_param_ensure(void);
uint8_t bk_wifi_p2p_go_get_channel_config(void);
void bk_wifi_p2p_go_set_channel_config(uint8_t channel);

void bk_wifi_p2p_shutdown_before_sleep(void);

#if CONFIG_P2P_SOFTAP_CHAN_ALIGN
struct p2p_data;
struct p2p_channels;
uint8_t bk_wifi_p2p_go_get_planned_channel(void);
uint8_t bk_wifi_p2p_pick_go_startup_channel(struct wpa_supplicant *wpa_s);
uint8_t bk_wifi_p2p_get_coexist_anchor_channel(void);
int bk_wifi_p2p_get_coexist_anchor_freq(void);
int bk_wifi_p2p_coexist_force_op_channel(struct p2p_data *p2p,
					 struct p2p_channels *intersection);
void bk_wifi_p2p_softap_csa_to_group(void);
void bk_wifi_p2p_softap_csa_to_group_deferred(void);
bool bk_wifi_infra_sta_vif_active(void);
bool bk_wifi_infra_ap_vif_active(void);
#endif
#endif
#ifdef __cplusplus
}
#endif
