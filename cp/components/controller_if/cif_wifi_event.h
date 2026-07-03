#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Wi-Fi event IDs for CP -> AP IPC (values match ap/include/modules/wifi_types.h).
 */
#define CIF_WIFI_EVT_STA_CONNECTED        2
#define CIF_WIFI_EVT_STA_DISCONNECTED     4
#define CIF_WIFI_EVT_GO_CONNECTED         7
#define CIF_WIFI_EVT_GO_DISCONNECTED      8
#define CIF_WIFI_EVT_GC_CONNECTED         9
#define CIF_WIFI_EVT_GC_DISCONNECTED     10

#define CIF_WIFI_EVENT_IND_MAX_DATA      64

typedef struct {
	uint16_t event_id;
	uint16_t data_len;
	uint8_t data[CIF_WIFI_EVENT_IND_MAX_DATA];
} cif_wifi_event_ind_t;

#ifdef __cplusplus
}
#endif
