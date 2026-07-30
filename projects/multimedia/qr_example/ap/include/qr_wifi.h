#pragma once

#include <stdint.h>

#include <avdk_error.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QR_WIFI_STATE_IDLE = 0,
    QR_WIFI_STATE_CONNECTING,
    QR_WIFI_STATE_CONNECTED,
    QR_WIFI_STATE_GOT_IP,
    QR_WIFI_STATE_FAILED,
} qr_wifi_state_t;

avdk_err_t qr_wifi_start(void);
avdk_err_t qr_wifi_stop(void);
avdk_err_t qr_wifi_disconnect(void);
avdk_err_t qr_wifi_provisioning(const uint8_t *payload, uint32_t len);
void qr_wifi_print_status(void);
qr_wifi_state_t qr_wifi_get_state(void);
int qr_wifi_is_active(void);

#ifdef __cplusplus
}
#endif
