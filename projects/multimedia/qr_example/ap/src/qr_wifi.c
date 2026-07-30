#include "qr_wifi.h"

#include <common/bk_include.h>
#include <components/event.h>
#include <components/log.h>
#include <components/netif_types.h>
#include <os/mem.h>
#include <os/str.h>

#include "cJSON.h"
#include "wifi_api.h"

#define TAG "qr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define QR_WIFI_PAYLOAD_MAX_LEN 256
#define QR_WIFI_SSID_MAX_LEN    32
#define QR_WIFI_PASSWORD_MIN_LEN 8
#define QR_WIFI_PASSWORD_MAX_LEN 63

typedef struct {
    uint8_t events_registered;
    uint8_t sta_started;
    qr_wifi_state_t state;
    char ip[NETIF_IP4_STR_LEN];
    char last_payload[QR_WIFI_PAYLOAD_MAX_LEN];
    uint16_t last_payload_len;
} qr_wifi_ctx_t;

static qr_wifi_ctx_t s_qr_wifi;

static const char *qr_wifi_state_name(qr_wifi_state_t state)
{
    switch (state) {
    case QR_WIFI_STATE_IDLE:
        return "IDLE";
    case QR_WIFI_STATE_CONNECTING:
        return "CONNECTING";
    case QR_WIFI_STATE_CONNECTED:
        return "CONNECTED";
    case QR_WIFI_STATE_GOT_IP:
        return "GOT_IP";
    case QR_WIFI_STATE_FAILED:
        return "FAILED";
    default:
        return "UNKNOWN";
    }
}

static void qr_wifi_set_state(qr_wifi_state_t state)
{
    if (s_qr_wifi.state != state) {
        LOGI("QR Wi-Fi state: %s -> %s\r\n",
             qr_wifi_state_name(s_qr_wifi.state),
             qr_wifi_state_name(state));
    }
    s_qr_wifi.state = state;
}

static int qr_wifi_event_cb(void *arg, event_module_t module, int event_id, void *event_data)
{
    (void)arg;

    if (module == EVENT_MOD_WIFI) {
        switch (event_id) {
        case EVENT_WIFI_STA_CONNECTED: {
            wifi_event_sta_connected_t *info = (wifi_event_sta_connected_t *)event_data;
            LOGI("QR Wi-Fi connected: ssid=%s\r\n", info ? info->ssid : "");
            qr_wifi_set_state(QR_WIFI_STATE_CONNECTED);
            break;
        }
        case EVENT_WIFI_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t *info = (wifi_event_sta_disconnected_t *)event_data;
            int reason = info ? info->disconnect_reason : -1;
            int local = info ? info->local_generated : -1;
            LOGW("QR Wi-Fi disconnected: reason=%d local=%d\r\n", reason, local);
            if (s_qr_wifi.state == QR_WIFI_STATE_CONNECTING &&
                reason == WIFI_REASON_NO_AP_FOUND) {
                LOGW("QR Wi-Fi scan miss, keep connecting\r\n");
                break;
            }
            if (s_qr_wifi.sta_started != 0U) {
                qr_wifi_set_state(QR_WIFI_STATE_FAILED);
            }
            break;
        }
        default:
            break;
        }
    } else if (module == EVENT_MOD_NETIF && event_id == EVENT_NETIF_GOT_IP4) {
        netif_event_got_ip4_t *got_ip = (netif_event_got_ip4_t *)event_data;
        if (got_ip != NULL) {
            os_strlcpy(s_qr_wifi.ip, got_ip->ip, sizeof(s_qr_wifi.ip));
            LOGI("QR Wi-Fi got IP: if=%d ip=%s\r\n", got_ip->netif_if, s_qr_wifi.ip);
        } else {
            s_qr_wifi.ip[0] = '\0';
            LOGI("QR Wi-Fi got IP\r\n");
        }
        qr_wifi_set_state(QR_WIFI_STATE_GOT_IP);
    }

    return BK_OK;
}

static avdk_err_t qr_wifi_register_events(void)
{
    if (s_qr_wifi.events_registered != 0U) {
        return AVDK_ERR_OK;
    }

    bk_err_t ret = bk_event_register_cb(EVENT_MOD_WIFI,
                                        EVENT_WIFI_STA_CONNECTED,
                                        qr_wifi_event_cb,
                                        NULL);
    if (ret != BK_OK) {
        LOGE("register Wi-Fi connected event failed=%d\r\n", ret);
        return AVDK_ERR_GENERIC;
    }

    ret = bk_event_register_cb(EVENT_MOD_WIFI,
                               EVENT_WIFI_STA_DISCONNECTED,
                               qr_wifi_event_cb,
                               NULL);
    if (ret != BK_OK) {
        LOGE("register Wi-Fi disconnected event failed=%d\r\n", ret);
        (void)bk_event_unregister_cb(EVENT_MOD_WIFI,
                                     EVENT_WIFI_STA_CONNECTED,
                                     qr_wifi_event_cb);
        return AVDK_ERR_GENERIC;
    }

    ret = bk_event_register_cb(EVENT_MOD_NETIF,
                               EVENT_NETIF_GOT_IP4,
                               qr_wifi_event_cb,
                               NULL);
    if (ret != BK_OK) {
        LOGE("register netif got-ip event failed=%d\r\n", ret);
        (void)bk_event_unregister_cb(EVENT_MOD_WIFI,
                                     EVENT_WIFI_STA_DISCONNECTED,
                                     qr_wifi_event_cb);
        (void)bk_event_unregister_cb(EVENT_MOD_WIFI,
                                     EVENT_WIFI_STA_CONNECTED,
                                     qr_wifi_event_cb);
        return AVDK_ERR_GENERIC;
    }

    s_qr_wifi.events_registered = 1U;
    return AVDK_ERR_OK;
}

static void qr_wifi_unregister_events(void)
{
    if (s_qr_wifi.events_registered == 0U) {
        return;
    }

    (void)bk_event_unregister_cb(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4, qr_wifi_event_cb);
    (void)bk_event_unregister_cb(EVENT_MOD_WIFI, EVENT_WIFI_STA_DISCONNECTED, qr_wifi_event_cb);
    (void)bk_event_unregister_cb(EVENT_MOD_WIFI, EVENT_WIFI_STA_CONNECTED, qr_wifi_event_cb);
    s_qr_wifi.events_registered = 0U;
}

static avdk_err_t qr_wifi_stop_sta(void)
{
    bk_err_t ret = bk_wifi_sta_stop();
    if (ret != BK_OK) {
        LOGE("bk_wifi_sta_stop failed=%d\r\n", ret);
        return AVDK_ERR_GENERIC;
    }

    s_qr_wifi.sta_started = 0U;
    s_qr_wifi.ip[0] = '\0';
    s_qr_wifi.last_payload_len = 0U;
    s_qr_wifi.last_payload[0] = '\0';
    qr_wifi_set_state(QR_WIFI_STATE_IDLE);
    return AVDK_ERR_OK;
}

static int qr_wifi_payload_is_last(const uint8_t *payload, uint32_t len)
{
    return (s_qr_wifi.last_payload_len == len &&
            len < QR_WIFI_PAYLOAD_MAX_LEN &&
            os_memcmp(s_qr_wifi.last_payload, payload, len) == 0);
}

static void qr_wifi_save_last_payload(const uint8_t *payload, uint32_t len)
{
    os_memset(s_qr_wifi.last_payload, 0, sizeof(s_qr_wifi.last_payload));
    os_memcpy(s_qr_wifi.last_payload, payload, len);
    s_qr_wifi.last_payload_len = (uint16_t)len;
}

static avdk_err_t qr_wifi_connect(const char *ssid, const char *password)
{
    wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();

    avdk_err_t err = qr_wifi_register_events();
    if (err != AVDK_ERR_OK) {
        return err;
    }

    qr_wifi_set_state(QR_WIFI_STATE_CONNECTING);

    bk_err_t ret = bk_wifi_sta_stop();
    if (ret != BK_OK) {
        LOGE("bk_wifi_sta_stop before connect failed=%d\r\n", ret);
        qr_wifi_set_state(QR_WIFI_STATE_FAILED);
        return AVDK_ERR_GENERIC;
    }

    os_strlcpy(sta_config.ssid, ssid, sizeof(sta_config.ssid));
    os_strlcpy(sta_config.password, password, sizeof(sta_config.password));

    LOGI("QR Wi-Fi provisioning connect ssid=%s password_len=%u\r\n",
         sta_config.ssid, (unsigned)os_strlen(sta_config.password));

    ret = bk_wifi_sta_set_config(&sta_config);
    if (ret != BK_OK) {
        LOGE("bk_wifi_sta_set_config failed=%d\r\n", ret);
        qr_wifi_set_state(QR_WIFI_STATE_FAILED);
        return AVDK_ERR_GENERIC;
    }

    ret = bk_wifi_sta_start();
    if (ret != BK_OK) {
        LOGE("bk_wifi_sta_start failed=%d\r\n", ret);
        qr_wifi_set_state(QR_WIFI_STATE_FAILED);
        return AVDK_ERR_GENERIC;
    }

    s_qr_wifi.sta_started = 1U;
    return AVDK_ERR_OK;
}

avdk_err_t qr_wifi_start(void)
{
    os_memset(&s_qr_wifi, 0, sizeof(s_qr_wifi));
    s_qr_wifi.state = QR_WIFI_STATE_IDLE;
    return AVDK_ERR_OK;
}

avdk_err_t qr_wifi_stop(void)
{
    qr_wifi_unregister_events();
    return qr_wifi_stop_sta();
}

avdk_err_t qr_wifi_disconnect(void)
{
    return qr_wifi_stop();
}

avdk_err_t qr_wifi_provisioning(const uint8_t *payload, uint32_t len)
{
    if (payload == NULL || len == 0U) {
        return AVDK_ERR_OK;
    }

    if (s_qr_wifi.state == QR_WIFI_STATE_CONNECTING ||
        s_qr_wifi.state == QR_WIFI_STATE_CONNECTED ||
        s_qr_wifi.state == QR_WIFI_STATE_GOT_IP) {
        return AVDK_ERR_BUSY;
    }

    if (len >= QR_WIFI_PAYLOAD_MAX_LEN) {
        LOGW("QR Wi-Fi payload too long=%u\r\n", (unsigned)len);
        return AVDK_ERR_INVAL;
    }

    if (s_qr_wifi.state == QR_WIFI_STATE_FAILED &&
        qr_wifi_payload_is_last(payload, len)) {
        return AVDK_ERR_BUSY;
    }

    cJSON *root = cJSON_ParseWithLength((const char *)payload, len);
    if (root == NULL) {
        return AVDK_ERR_INVAL;
    }

    avdk_err_t ret = AVDK_ERR_INVAL;
    if (!cJSON_IsObject(root) || cJSON_GetArraySize(root) != 3) {
        goto exit;
    }

    cJSON *password_item = cJSON_GetObjectItemCaseSensitive(root, "p");
    cJSON *ssid_item = cJSON_GetObjectItemCaseSensitive(root, "s");
    cJSON *token_item = cJSON_GetObjectItemCaseSensitive(root, "t");
    if (!cJSON_IsString(password_item) ||
        !cJSON_IsString(ssid_item) ||
        !cJSON_IsString(token_item)) {
        goto exit;
    }

    const char *password = password_item->valuestring ? password_item->valuestring : "";
    const char *ssid = ssid_item->valuestring ? ssid_item->valuestring : "";
    const char *token = token_item->valuestring ? token_item->valuestring : "";
    uint32_t ssid_len = os_strlen(ssid);
    uint32_t password_len = os_strlen(password);

    if (ssid_len == 0U || ssid_len > QR_WIFI_SSID_MAX_LEN) {
        LOGW("invalid QR Wi-Fi ssid length=%u\r\n", (unsigned)ssid_len);
        goto exit;
    }
    if (password_len != 0U &&
        (password_len < QR_WIFI_PASSWORD_MIN_LEN ||
         password_len > QR_WIFI_PASSWORD_MAX_LEN)) {
        LOGW("invalid QR Wi-Fi password length=%u\r\n", (unsigned)password_len);
        goto exit;
    }

    LOGI("valid QR Wi-Fi payload: ssid=%s token_len=%u\r\n",
         ssid, (unsigned)os_strlen(token));
    qr_wifi_save_last_payload(payload, len);
    ret = qr_wifi_connect(ssid, password);

exit:
    cJSON_Delete(root);
    return ret;
}

void qr_wifi_print_status(void)
{
    wifi_link_status_t link_status = {0};
    bk_err_t ret = bk_wifi_sta_get_link_status(&link_status);

    LOGI("QR Wi-Fi local state=%s started=%u events=%u\r\n",
         qr_wifi_state_name(s_qr_wifi.state),
         s_qr_wifi.sta_started,
         s_qr_wifi.events_registered);
    if (ret != BK_OK) {
        LOGW("bk_wifi_sta_get_link_status failed=%d\r\n", ret);
        return;
    }

    LOGI("QR Wi-Fi link state=%d ssid=%s rssi=%d ip=%s\r\n",
         link_status.state,
         link_status.ssid,
         link_status.rssi,
         s_qr_wifi.ip);
}

qr_wifi_state_t qr_wifi_get_state(void)
{
    return s_qr_wifi.state;
}

int qr_wifi_is_active(void)
{
    return (s_qr_wifi.sta_started != 0U || s_qr_wifi.events_registered != 0U);
}
