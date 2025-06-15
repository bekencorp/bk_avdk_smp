/*
 * Copyright 2020-2025 Beken

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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/netif.h"
#include "wdrv_co_list.h"

#define MAX_SCAN_AP_NUM             48
#define SSID_MAX_LEN                33     /**< Maximum **NULL-terminated** WiFi SSID length */
#define PASSWORD_MAX_LEN            64     /**< Maximum **NULL-terminated** WiFi password length */
#define NETIF_IP4_STR_LEN           16
#define WIFI_BSSID_LEN              6      /**< Length of BSSID */
#define ETH_ALEN                    6
#define WIFI_MAX_SCAN_CHAN_DUR      200    /**< scan duration param, need less than 200ms */
#define WIFI_CHANNEL_NUM_2G4        14    /**< Maximum supported 2.4G channel number */
#define WIFI_MIN_CHAN_NUM           1      /**< Minimum supported channel number */
#define WIFI_MAX_CHAN_NUM           14     /**< Maximum supported channel number */
#define WIFI_CHANNEL_NUM_5G         28    /**< Maximum supported 5G channel numbe*/
#define WIFI_2BAND_MAX_CHAN_NUM     (WIFI_CHANNEL_NUM_2G4 + WIFI_CHANNEL_NUM_5G)
#define DEFAULT_CHANNEL_AP          1     /**< Default Channel of SoftAP */
#define WIFI_MAC_LEN                6      /**< Length of MAC */

/* host cmd setting */
#define WDRV_CMD_WAITCFM           1
#define WDRV_CMD_NOWAITCFM         0
#define WDRV_CMDCFM_TIMEOUT        2000
#define WDRV_MAX_MSG_CNT                      (0x800)
#define WDRV_CMD_CFM_OFFSET                   (0x8000)

#define BK_CFM_GET_CMD_ID(cfm_id)           (cfm_id - WDRV_CMD_CFM_OFFSET)

struct wifi_ssid {
    /// Actual length of the SSID.
    uint8_t length;
    /// Array containing the SSID name.
    uint8_t array[32];
};

struct wifi_bssid {
    uint8_t bssid[6];
};

typedef struct fast_connect_param {
    uint8_t bssid[6];
    uint8_t chann;
} fast_connect_param_t;

typedef enum {
    WIFI_SECURITY_NONE,            /**< Open system. */
    WIFI_SECURITY_WEP,             /**< WEP security, **it's unsafe security, please don't use it** */
    WIFI_SECURITY_WPA_TKIP,        /**< WPA TKIP */
    WIFI_SECURITY_WPA_AES,         /**< WPA AES */
    WIFI_SECURITY_WPA_MIXED,       /**< WPA AES or TKIP */
    WIFI_SECURITY_WPA2_TKIP,       /**< WPA2 TKIP */
    WIFI_SECURITY_WPA2_AES,        /**< WPA2 AES */
    WIFI_SECURITY_WPA2_MIXED,      /**< WPA2 AES or TKIP */
    WIFI_SECURITY_WPA3_SAE,        /**< WPA3 SAE */
    WIFI_SECURITY_WPA3_WPA2_MIXED, /**< WPA3 SAE or WPA2 AES */
    WIFI_SECURITY_EAP,             /**< EAP */
    WIFI_SECURITY_OWE,             /**< OWE */
    WIFI_SECURITY_AUTO,            /**< WiFi automatically detect the security type */
} wifi_security_t;

typedef enum {
    /* for STA mode */
    WIFI_LINKSTATE_STA_IDLE = 0,      /**< sta mode is idle */
    WIFI_LINKSTATE_STA_CONNECTING,    /**< sta mode is connecting */
    WIFI_LINKSTATE_STA_DISCONNECTED,  /**< sta mode is disconnected */
    WIFI_LINKSTATE_STA_CONNECTED,     /**< sta mode is connected */
    WIFI_LINKSTATE_STA_CONNECT_FAILED,     /**< sta mode is connec fail */
    WIFI_LINKSTATE_STA_GOT_IP,        /**< sta mode got ip */
    WIFI_LINKSTATE_STA_SCAN_DONE,
    /* for AP mode */
    WIFI_LINKSTATE_AP_CONNECTED,      /**< softap mode, a client association success */
    WIFI_LINKSTATE_AP_DISCONNECTED,   /**< softap mode, a client disconnect */
    WIFI_LINKSTATE_AP_CONNECT_FAILED, /**< softap mode, a client association failed */
    WIFI_LINKSTATE_MAX,               /**< reserved */
    //TODO maybe we can provide more precise link status
} wifi_link_state_t;

typedef enum {
    WIFI_REASON_RESERVED = 0,
    WIFI_REASON_UNSPECIFIED = 1,
    WIFI_REASON_PREV_AUTH_NOT_VALID = 2,
    WIFI_REASON_DEAUTH_LEAVING = 3,
    WIFI_REASON_DISASSOC_DUE_TO_INACTIVITY = 4,
    WIFI_REASON_DISASSOC_AP_BUSY = 5,
    WIFI_REASON_CLASS2_FRAME_FROM_NONAUTH_STA = 6,
    WIFI_REASON_CLASS3_FRAME_FROM_NONASSOC_STA = 7,
    WIFI_REASON_DISASSOC_STA_HAS_LEFT = 8,
    WIFI_REASON_STA_REQ_ASSOC_WITHOUT_AUTH = 9,
    WIFI_REASON_PWR_CAPABILITY_NOT_VALID = 10,
    WIFI_REASON_SUPPORTED_CHANNEL_NOT_VALID = 11,
    WIFI_REASON_BSS_TRANSITION_DISASSOC = 12,
    WIFI_REASON_INVALID_IE = 13,
    WIFI_REASON_MICHAEL_MIC_FAILURE = 14,
    WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT = 15,
    WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT = 16,
    WIFI_REASON_IE_IN_4WAY_DIFFERS = 17,
    WIFI_REASON_GROUP_CIPHER_NOT_VALID = 18,
    WIFI_REASON_PAIRWISE_CIPHER_NOT_VALID = 19,
    WIFI_REASON_AKMP_NOT_VALID = 20,
    WIFI_REASON_UNSUPPORTED_RSN_IE_VERSION = 21,
    WIFI_REASON_INVALID_RSN_IE_CAPAB = 22,
    WIFI_REASON_IEEE_802_1X_AUTH_FAILED = 23,
    WIFI_REASON_CIPHER_SUITE_REJECTED = 24,
    WIFI_REASON_TDLS_TEARDOWN_UNREACHABLE = 25,
    WIFI_REASON_TDLS_TEARDOWN_UNSPECIFIED = 26,
    WIFI_REASON_SSP_REQUESTED_DISASSOC = 27,
    WIFI_REASON_NO_SSP_ROAMING_AGREEMENT = 28,
    WIFI_REASON_BAD_CIPHER_OR_AKM = 29,
    WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION = 30,
    WIFI_REASON_SERVICE_CHANGE_PRECLUDES_TS = 31,
    WIFI_REASON_UNSPECIFIED_QOS_REASON = 32,
    WIFI_REASON_NOT_ENOUGH_BANDWIDTH = 33,
    WIFI_REASON_DISASSOC_LOW_ACK = 34,
    WIFI_REASON_EXCEEDED_TXOP = 35,
    WIFI_REASON_STA_LEAVING = 36,
    WIFI_REASON_END_TS_BA_DLS = 37,
    WIFI_REASON_UNKNOWN_TS_BA = 38,
    WIFI_REASON_TIMEOUT = 39,
    WIFI_REASON_PEERKEY_MISMATCH = 45,
    WIFI_REASON_AUTHORIZED_ACCESS_LIMIT_REACHED = 46,
    WIFI_REASON_EXTERNAL_SERVICE_REQUIREMENTS = 47,
    WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT = 48,
    WIFI_REASON_INVALID_PMKID = 49,
    WIFI_REASON_INVALID_MDE = 50,
    WIFI_REASON_INVALID_FTE = 51,
    WIFI_REASON_MESH_PEERING_CANCELLED = 52,
    WIFI_REASON_MESH_MAX_PEERS = 53,
    WIFI_REASON_MESH_CONFIG_POLICY_VIOLATION = 54,
    WIFI_REASON_MESH_CLOSE_RCVD = 55,
    WIFI_REASON_MESH_MAX_RETRIES = 56,
    WIFI_REASON_MESH_CONFIRM_TIMEOUT = 57,
    WIFI_REASON_MESH_INVALID_GTK = 58,
    WIFI_REASON_MESH_INCONSISTENT_PARAMS = 59,
    WIFI_REASON_MESH_INVALID_SECURITY_CAP = 60,
    WIFI_REASON_MESH_PATH_ERROR_NO_PROXY_INFO = 61,
    WIFI_REASON_MESH_PATH_ERROR_NO_FORWARDING_INFO = 62,
    WIFI_REASON_MESH_PATH_ERROR_DEST_UNREACHABLE = 63,
    WIFI_REASON_MAC_ADDRESS_ALREADY_EXISTS_IN_MBSS = 64,
    WIFI_REASON_MESH_CHANNEL_SWITCH_REGULATORY_REQ = 65,
    WIFI_REASON_MESH_CHANNEL_SWITCH_UNSPECIFIED = 66,

    WIFI_REASON_BEACON_LOST = 256,       /**< The BK STA can't detect the beacon of the connected AP */
    WIFI_REASON_NO_AP_FOUND = 257,       /**< Can't find the target AP */
    WIFI_REASON_WRONG_PASSWORD = 258,    /**< The password is wrong */
    WIFI_REASON_DISCONNECT_BY_APP = 259, /**< The BK STA disconnected by application */
    WIFI_REASON_DHCP_TIMEOUT = 260,      /**<The BK STA dhcp timeout, 20s**/
    WIFI_REASON_MAX,                     /**<The BK STA connect success*/
} wifi_err_reason_t;

typedef struct {
    wifi_link_state_t state;           /**<Wifi linkstate*/
    wifi_err_reason_t reason_code;     /**<Wifi disconnect reason code, success will be WIFI_REASON_MAX*/
} wifi_linkstate_reason_t;

typedef struct wlan_ssid {
    uint8_t ssid[SSID_MAX_LEN];
    uint8_t ssid_len;
} wlan_ssid_t;

/**
 * @brief Wlan station scan parameters definition
 */
typedef struct wlan_sta_scan_param {
    uint8_t scan_only;    /* do scan only */
    uint8_t scan_passive; /* passive scan */
    uint8_t scan_ssid;    /* Scan SSID of configured network with Probe Requests */
    uint8_t num_ssids;
    wlan_ssid_t ssids[SSID_MAX_LEN];
} wlan_sta_scan_param_t;

typedef struct general_param {
    uint8_t role;
    uint8_t dhcp_enable;
    uint32_t ip_addr;
    uint32_t ip_mask;
    uint32_t ip_gw;
} general_param_t;

typedef struct ap_param {
    struct wifi_bssid bssid;
    struct wifi_ssid ssid;

    uint8_t chann;
    uint8_t cipher_suite;
    uint8_t key[65];
    uint8_t key_len;
    bool hidden_ssid;
    u8 max_statype_num[4];
    u8 max_con;
} ap_param_t;

typedef struct sta_param {
    uint8_t own_mac[6];
    struct wifi_ssid ssid;
    uint8_t cipher_suite;
    uint8_t key[65];
    uint8_t key_len;
    uint8_t fast_connect_set;
    uint8_t ocv;
    fast_connect_param_t fast_connect;
    int auto_reconnect_count;          /**< auto reconnect max count, 0 for always reconnect */
    int auto_reconnect_timeout;        /**< auto reconnect timeout in secs, 0 for no timeout */
    bool disable_auto_reconnect_after_disconnect;  /**< disable auto reconnect if deauth/disassoc by AP when in connected state */
} sta_param_t;

struct wlan_fast_connect_info
{
    uint8_t ssid[33]; /**< SSID of AP */
    uint8_t bssid[6]; /**< BSSID of AP */
    uint8_t security; /**< Security of AP */
    uint8_t channel;  /**< Channel of AP */
    uint8_t psk[65];  /**< PSK of AP */
    uint8_t pwd[65];  /**< password of AP */
    uint8_t ip_addr[4];
    uint8_t netmask[4];
    uint8_t gw[4];
    uint8_t dns1[4];
    /* aes attention: sizeof(RL_BSSID_INFO_T) = 16 * n */
    uint8_t padding[0] __attribute__ ((aligned (16)));
};

typedef struct {
    char ssid[SSID_MAX_LEN];      /**< SSID of AP to be connected */
    uint8_t bssid[WIFI_BSSID_LEN];     /**< BSSID of AP to be connected, fast connect only */
    uint8_t channel;                   /**< Primary channel of AP to be connected, fast connect only */
    wifi_security_t security;          /**< Security of AP to be connected */
    char  password[PASSWORD_MAX_LEN]; /**< Security key or PMK of the wlan. */
    uint8_t psk[PASSWORD_MAX_LEN];    /**< PSK of AP */

    uint8_t ip_addr[4];                 /**< IP address */
    uint8_t netmask[4];                 /**< NET address */
    uint8_t gw[4];                      /**< GW address */
    uint8_t dns1[4];                    /**< DNS address */
    uint8_t is_not_support_auto_fci;    /**< is not support auto fast connect */
    uint8_t is_user_fast_connect;       /**< is use user fast connect */
    uint8_t pmf;                        /**< is support pmf */
    uint8_t tk[16];                     /**< WPA3 tk key */

    /* auto reconnect configuration */
    int auto_reconnect_count;          /**< auto reconnect max count, 0 for always reconnect */
    int auto_reconnect_timeout;        /**< auto reconnect timeout in secs, 0 for no timeout */
    bool disable_auto_reconnect_after_disconnect;  /**< disable auto reconnect if deauth/disassoc by AP when in connected state */

    uint8_t reserved[32];              /**< reserved, **must set to 0** */
} wifi_sta_config_t;

typedef struct {
    char ssid[SSID_MAX_LEN];     /**< SSID to be scaned */
    u8 scan_type;                /**< 0: active scan; 1: passive scan*/
    u8 chan_cnt;                 /**< scan channel cnt*/
    u8 chan_nb[WIFI_2BAND_MAX_CHAN_NUM];     /**< scan channel number 2.4g+5g*/
    u32 duration;                /**< scan duration,ms*/
} wifi_scan_config_t;

typedef struct {
    wifi_link_state_t state;           /**< The WiFi connection status */
    int aid;                           /**< STA AID */
    int rssi;                          /**< The RSSI of AP the BK STA is connected */
    char ssid[SSID_MAX_LEN];           /**< SSID of AP the BK STA is connected */
    uint8_t bssid[WIFI_BSSID_LEN];     /**< BSSID of AP the BK STA is connected */
    uint8_t channel;                   /**< Primary channel of AP the BK STA is connected */
    wifi_security_t security;          /**< Security of AP the BK STA is connected */
    char password[PASSWORD_MAX_LEN];   /**< Passord of AP the BK STA is connected */
} wifi_link_status_t;

typedef struct {
    char ssid[SSID_MAX_LEN];      /**< The SSID of BK AP */
    char password[PASSWORD_MAX_LEN];  /**< Password of BK AP, ignored in an open system.*/
    uint8_t channel;                   /**< The channel of BK AP, 0 indicates default TODO */
    wifi_security_t security;          /**< Security type of BK AP, default value TODO */
    uint8_t hidden: 1;                 /**< Whether the BK AP is hidden */
    uint8_t acs: 1;                    /**< Whether Auto Channel Selection is enabled */
    uint8_t vsie_len;                  /**< Beacon/ProbeResp Vendor Specific IE len */
    uint8_t vsie[255];                 /**< Beacon/ProbeResp Vendor Specific IE */
    uint8_t max_con;                   /**< Max number of stations allowed to connect to BK AP, TODO default value? */ 
    u8 max_statype_num[4];
    uint8_t reserved[32];              /**< Reserved, **must be zero** */
} wifi_ap_config_t;

typedef enum
{
    BK_SOFT_AP,  /**< Act as an access point, and other station can connect, 4 stations Max*/
    BK_STATION,   /**< Act as a station which can connect to an access point*/
    BK_P2P
} wlanInterfaceTypedef;

/**
 *  @brief  wlan local IP information structure definition.
 */
typedef struct
{
    uint8_t dhcp;       /**< DHCP mode: @ref DHCP_DISABLE, @ref DHCP_CLIENT, @ref DHCP_SERVER.*/
    char    ip[16];     /**< Local IP address on the target wlan interface: @ref wlanInterfaceTypedef.*/
    char    gate[16];   /**< Router IP address on the target wlan interface: @ref wlanInterfaceTypedef.*/
    char    mask[16];   /**< Netmask on the target wlan interface: @ref wlanInterfaceTypedef.*/
    char    dns[16];    /**< DNS server IP address.*/
    char    mac[16];    /**< MAC address, example: "C89346112233".*/
    char    broadcastip[16]; /**< Broadcast IP address */
} IPStatusTypedef;

#define WiFi_Interface  wlanInterfaceTypedef /**< WiFi interface */

#define DHCP_DISABLE  (0)   /**< Disable DHCP service. */
#define DHCP_CLIENT   (1)   /**< Enable DHCP client which get IP address from DHCP server automatically */
#define DHCP_SERVER   (2)   /**< Enable DHCP server, needs assign a static address as local address. */

#define BK_ERR_WIFI_NOT_INIT            (BK_ERR_WIFI_BASE - 1) /**< WiFi is not initialized, call bk_wifi_init() to init the WiFi */
#define BK_ERR_WIFI_STA_NOT_STARTED     (BK_ERR_WIFI_BASE - 2) /**< STA is not started, call bk_wifi_sta_start() to start the STA */
#define BK_ERR_WIFI_AP_NOT_STARTED      (BK_ERR_WIFI_BASE - 3) /**< AP is not initialized, call bk_wifi_ap_start() to start the AP */
#define BK_ERR_WIFI_CHAN_RANGE          (BK_ERR_WIFI_BASE - 4) /**< Invalid channel range */
#define BK_ERR_WIFI_COUNTRY_POLICY      (BK_ERR_WIFI_BASE - 5) /**< Invalid country policy */
#define BK_ERR_WIFI_RESERVED_FIELD      (BK_ERR_WIFI_BASE - 6) /**< Reserved fields not 0 */
#define BK_ERR_WIFI_MONITOR_IP          (BK_ERR_WIFI_BASE - 7) /**< Monitor is in progress */
#define BK_ERR_WIFI_STA_NOT_CONFIG      (BK_ERR_WIFI_BASE - 8) /**< STA is not configured, call bk_wifi_sta_config() to configure it */
#define BK_ERR_WIFI_AP_NOT_CONFIG       (BK_ERR_WIFI_BASE - 9) /**< AP is not configured, call bk_wifi_ap_config() to configure it */
#define BK_ERR_WIFI_DRIVER              (BK_ERR_WIFI_BASE - 10) /**< Internal WiFi driver error */
#define BK_ERR_WIFI_MONITOR_ENTER       (BK_ERR_WIFI_BASE - 11) /**< WiFi failed to enter monitor mode */
#define BK_ERR_WIFI_DRIVER_DEL_VIF      (BK_ERR_WIFI_BASE - 12) /**< WiFi driver failed to delete WiFi virtual interface */
#define BK_ERR_WIFI_DRIVER_AP_START     (BK_ERR_WIFI_BASE - 13) /**< WiFi driver failed to start BK AP */
#define BK_ERR_WIFI_CHAN_NUMBER         (BK_ERR_WIFI_BASE - 14) /**< Invalid channel number */

struct scan_cfg_scan_param_tag{
    u8 set_param;     /**< indicates set scan param*/
    u8 scan_type;     /**< passive scan:1, active scan:0*/
    u8 chan_cnt;     /**< scan channel cnt*/
    u8 chan_nb[WIFI_2BAND_MAX_CHAN_NUM];     /**< scan channel number 2.4g+5g*/
    u32 duration;     /**< scan duration,us*/
};

typedef struct _wdrv_cmd_cfm
{
    struct co_list_hdr list;
    uint32_t waitcfm;
    uint16_t cfm_id;
    uint16_t cfm_sn;
    uint8_t *cfm_buf;
    uint16_t cfm_len;
    beken_semaphore_t sema;
}wdrv_cmd_cfm;

/*CMD header from AP to CP */
typedef struct _wdrv_cmd_hdr
{
    uint32_t rsv0;
    uint16_t cmd_id;
    uint16_t cmd_sn;
    uint16_t rsv1;
    uint16_t len;
}wdrv_cmd_hdr;

/*Event or CMD-CFM header from CP to AP */
typedef struct _wdrv_rx_msg
{
    uint16_t id;                ///< Message id.
    uint16_t cfm_sn;            /// confirm msg sequence
    uint16_t rsv;               /// reserve
    uint16_t param_len;         ///< Parameter embedded struct length.
    uint32_t pattern;           ///< Used to stamp a valid MSG buffer
    uint32_t param[1];          ///< Parameter embedded struct. Must be word-aligned.
}wdrv_rx_msg;

struct wdrv_connect_req
{
    wdrv_cmd_hdr cmd_hdr;
    char ssid[SSID_MAX_LEN];
    char pw[PASSWORD_MAX_LEN];
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_connect_ind
{
    uint8_t ussid[SSID_MAX_LEN];
    int8_t  rssi;
    u32  ip;
    u32  mk;
    u32  gw;
    u32  dns;
};

struct wdrv_mac_addr_cfm
{
    uint8_t mac_addr[6];
};

struct wdrv_media_mode_req
{
    wdrv_cmd_hdr cmd_hdr;
    bool media_flag;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_media_quality_req
{
    wdrv_cmd_hdr cmd_hdr;
    uint8_t media_quality;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_start_ap_req
{
    wdrv_cmd_hdr cmd_hdr;
    uint8_t band;
    char ssid[SSID_MAX_LEN];
    char pw[PASSWORD_MAX_LEN];
    uint8_t channel;
    uint8_t hidden;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_stop_ap_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_start_scan_req
{
    wdrv_cmd_hdr cmd_hdr;
    char ssid[SSID_MAX_LEN];
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_get_interval_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_get_wifi_status_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};

struct wdrv_get_ap_config_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};
struct wdrv_get_ip4_config_req
{
    wdrv_cmd_hdr cmd_hdr;
	uint8_t flag;
    wdrv_cmd_cfm cmd_cfm;
};
struct wdrv_get_staipup_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};
struct wdrv_get_apipup_req
{
    wdrv_cmd_hdr cmd_hdr;
    wdrv_cmd_cfm cmd_cfm;
};


struct wdrv_set_csa_coexist_mode_flag_req
{
    wdrv_cmd_hdr cmd_hdr;
    bool is_close;
    wdrv_cmd_cfm cmd_cfm;
};

enum BK_CMD_TYPE
{
    // Wi-Fi command
    BK_CMD_SCAN_WIFI           = 0x1,
    BK_CMD_CONNECT             = 0x2,
    BK_CMD_DISCONNECT          = 0x3,
    BK_CMD_START_AP            = 0x4,
    BK_CMD_CHANGE_AP_MODE      = 0x5,
    BK_CMD_STOP_AP             = 0x6,
    BK_CMD_GET_WLAN_STATUS     = 0x7,
    BK_CMD_WIFI_MMD_CONFIG     = 0x8,
    BK_CMD_SET_NET_INFO        = 0x9,
    BK_CMD_SET_AUTO_RECONNECT  = 0xA,
    BK_CMD_SET_MEDIA_MODE      = 0xB,
    BK_CMD_SET_MEDIA_QUALITY   = 0xC,
    BK_CMD_GET_INTERVAL         = 0xD,
    BK_CMD_GET_WIFI_STATUS      = 0xE,
    BK_CMD_SET_COEX_CSA         = 0xF,
    BK_CMD_GET_AP_CONFIG       = 0x10,
    BK_CMD_GET_IP_CONFIG       = 0x11,
    BK_CMD_GET_STAIPUP         = 0x12,
    BK_CMD_GET_APIPUP          = 0x13,

    // BLE command
    BK_CMD_OPEN_BLE            = 0x101,
    BK_CMD_CLOSE_BLE           = 0x102,

    // system command
    BK_CMD_SET_MAC_ADDR        = 0x201,
    BK_CMD_GET_MAC_ADDR        = 0x202,
    BK_CMD_ENTER_SLEEP         = 0x203,
    BK_CMD_EXIT_SLEEP          = 0x204,
    BK_CMD_CONTROLLER_AT       = 0x205,
    BK_CMD_KEEPALIVE_CFG       = 0x206,
    BK_CMD_SET_TIME            = 0x207,
    BK_CMD_GET_TIME            = 0x208,
    BK_CMD_CUSTOMER_DATA       = 0x209,
    BK_CMD_START_OTA           = 0x20A,
    BK_CMD_SEND_OTA_PKT        = 0x20B,
    BK_CMD_STOP_OTA            = 0x20C,

    BK_CMD_BUTT                = WDRV_MAX_MSG_CNT - 1
};

/* CP Wi-Fi Mode */
enum WLAN_MODE
{
    WIFI_MODE_IDLE = 0,                 /* Wi-Fi Idle Mode */
    WIFI_MODE_STA,                      /* Wi-Fi Station Mode */
    WIFI_MODE_AP,                       /* Wi-Fi SoftAP Mode */
    WIFI_MODE_MAX
};

#define WLAN_DEFAULT_IP         "192.168.188.1"
#define WLAN_DEFAULT_GW         "192.168.188.1"
#define WLAN_DEFAULT_MASK       "255.255.255.0"

struct wdrv_ap_status_cfm
{
    uint8_t  status;
    uint32_t ip;
    uint32_t gw;
    uint32_t mk;
};

struct wdrv_ap_assoc_sta_ind
{
    uint8_t sub_sta_addr[6];
};

struct wdrv_wlan_status_cfm
{
    uint8_t status;
    int8_t  rssi;
    uint8_t ussid[SSID_MAX_LEN];
    char  ip[NETIF_IP4_STR_LEN];
    char  mk[NETIF_IP4_STR_LEN];
    char  gw[NETIF_IP4_STR_LEN];
    char  dns[NETIF_IP4_STR_LEN];
};

struct wdrv_scan_result_cfm
{
    uint8_t  scan_num;
    int8_t   rssi;
    uint8_t  bssid[6];
    uint8_t  ssid[SSID_MAX_LEN];
    uint32_t  akm;
    int      channal;
};


/* event-table from CP to AP */
enum BK_EVENT_TYPE
{
    BK_EVT_CONNECT_IND          = 0x1,
    BK_EVT_DISCONNECT_IND       = 0x2,
    BK_EVT_START_AP_IND         = 0x3,
    BK_EVT_ASSOC_AP_IND         = 0x4,
    BK_EVT_DISASSOC_AP_IND      = 0x5,
    BK_EVT_STOP_AP_IND          = 0x6,
    BK_EVT_SCAN_WIFI_IND        = 0x7,
    BK_EVT_WIFI_FAIL_IND        = 0x8,

    // BLE event
    // BK_EVT_BLE_XX            = 0x101

    // system event
    BK_EVT_CONTROLLER_AT_IND    = 0x201,
    BK_EVT_CUSTOMER_IND         = 0x202,

    // loopcheck event
    BK_EVT_LOOPCHECK            = 0x301,

    // throughput test
    BK_EVT_TP_TEST              = 0x401,


    BK_EVT_BUTT                 = WDRV_MAX_MSG_CNT - 1
};
/* cmd-table from app to netdrv */

typedef struct _wdrv_wlan {
    int8_t  wlan_mode;
    uint8_t wlan_link_sta_status;
    bool    comp_sign_get_mac_ready;

    beken_mutex_t cfm_lock;

    struct co_list cfm_pending_list;

    struct wdrv_mac_addr_cfm macaddr_cfm;
    struct wdrv_wlan_status_cfm get_wlan_cfm;
    struct wdrv_connect_ind connect_ind;
    struct wdrv_ap_status_cfm ap_status_cfm;
    struct wdrv_ap_assoc_sta_ind ap_assoc_sta_addr_ind;
    struct wdrv_scan_result_cfm *scan_wifi_cfm_ptr;
    struct wdrv_scan_result_cfm scan_wifi_cfm[MAX_SCAN_AP_NUM];

}wdrv_wlan;

/**
 * @brief WiFi public event type
 */
typedef enum {
    EVENT_WIFI_SCAN_DONE = 0,      /**< WiFi scan done event */
    EVENT_WIFI_STA_ASSOCIATED,     /**< WiFi associated event */
    EVENT_WIFI_STA_CONNECTED,      /**< The BK STA is connected */
    EVENT_WIFI_STA_DISCONNECTED,   /**< The BK STA is disconnected */

    EVENT_WIFI_AP_CONNECTED,       /**< A STA is connected to the BK AP */
    EVENT_WIFI_AP_DISCONNECTED,    /**< A STA is disconnected from the BK AP */

    EVENT_WIFI_NETWORK_FOUND,      /**< The BK STA find target AP */
    EVENT_WIFI_COUNT,              /**< WiFi event count */
} wifi_event_t;

typedef struct {
    char ssid[SSID_MAX_LEN];      /**< SSID of connected AP */
    uint8_t bssid[WIFI_BSSID_LEN];        /**< BSSID of connected AP*/
} wifi_event_sta_connected_t;

typedef struct {
    uint32_t scan_id; /**< Scan ID */
    uint32_t scan_use_time;/**< scan time. us */
} wifi_event_scan_done_t;

typedef struct {
    int disconnect_reason;                /**< Disconnect reason of BK STA */
    bool local_generated;                 /**< if disconnect is request by local */
} wifi_event_sta_disconnected_t;

typedef struct {
    uint8_t mac[WIFI_MAC_LEN];            /**< MAC of the STA connected to the BK AP */
} wifi_event_ap_connected_t;

typedef struct {
    uint8_t mac[WIFI_MAC_LEN];            /**< MAC of the STA disconnected from the BK AP */
} wifi_event_ap_disconnected_t;

/* A-Core Wi-Fi AP Mode State */
enum WLAN_LINK_AP_STATUS
{
    CONTROLLER_AP_START,                /* A-Core AP Mode Started */
    CONTROLLER_AP_CLOSE,                /* A-Core AP Mode Closed  */
};


#define CIFD_CUST_DEBUG_CODE_MAGIC                  (0xAABBCCDD)

#define CIFD_CUST_PATTERN  (0xa5a6)
#define MAX_CIFD_CUST_SIZE   256

typedef enum{
    CIFD_CMD_BLE_DATA_TO_APK             = 0x0001,

    CIFD_EVENT_BLE_DATA_TO_USER          = 0x1001,

}CIFD_CMD_EVENT;

typedef struct{
    uint16_t magic;
    uint16_t cid;
    uint16_t ctl;
    uint16_t seq;
    uint16_t checksum;
    uint16_t len;
}CIFD_PROTO_HDR;

typedef struct{
    CIFD_PROTO_HDR header;
    uint8_t data[MAX_CIFD_CUST_SIZE];
}CIFD_CUST_DATA;

typedef struct cifd_cust_msg_hdr
{
    uint16_t cmd_id;
    uint16_t len;
    uint8_t payload[0];
}cifd_cust_msg_hdr_t;

struct wdrv_customer_req
{
    wdrv_cmd_hdr cmd_hdr;
    uint8_t data[MAX_CIFD_CUST_SIZE];
};

#define OPCODE_LEN                      (2)
#define DATA_LEN_LEN                    (2)
#define SSID_LEN_LEN                    (2)
#define PW_LEN_LEN                      (2)


/*   API    */
void wdrv_host_init(void);
int wdrv_get_mac_addr();
bk_err_t bk_wdrv_get_mac(uint8_t *mac, mac_type_t type);
void wdrv_get_mac_ready(void);
uint32_t wdrv_param_init(void);
int bk_platform_get_wlan_status(void);
extern void wdrv_rx_handle_event(wdrv_rx_msg *msg);
extern void wdrv_rx_handle_cmd_confirm(wdrv_rx_msg *msg);
void wdrv_notify_sta_connected(void);
void wdrv_notify_sta_got_ip(void);
void bk_rx_handle_customer_event(void *data, uint16_t len);
int bk_wdrv_send_customer_data(uint8_t *data, uint16_t len);
int bk_wdrv_customer_transfer(uint16_t cmd_id, uint8_t * data, uint16_t len);
void wdrv_notify_sta_disconnected(void *data, uint16_t len);
void wdrv_notify_sap_sta_connected(void);
void wdrv_notify_sap_sta_disconnected(void);

FUNC_1PARAM_PTR bk_wlan_get_status_cb(void);
void wifi_netif_call_status_cb_when_sta_got_ip(void);
void mhdr_set_station_status(wifi_linkstate_reason_t info);
void bk_wlan_status_register_cb(FUNC_1PARAM_PTR cb);

typedef void (* rx_handle_customer_event_cb)(void *data, uint16_t len);
void bk_customer_event_register_callback(rx_handle_customer_event_cb callback);

extern wdrv_wlan wdrv_host_env;

extern general_param_t *g_wlan_general_param;
extern ap_param_t *g_ap_param_ptr;
extern sta_param_t *g_sta_param_ptr;

#ifdef __cplusplus
}
#endif
