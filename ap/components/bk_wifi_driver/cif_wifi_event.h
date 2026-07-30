#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CIF_WIFI_EVT_SCAN_DONE = 0,
	CIF_WIFI_EVT_STA_CONNECTED,
	CIF_WIFI_EVT_STA_DISCONNECTED,
	CIF_WIFI_EVT_AP_CONNECTED,
	CIF_WIFI_EVT_AP_DISCONNECTED,
	CIF_WIFI_EVT_GO_CONNECTED,
	CIF_WIFI_EVT_GO_DISCONNECTED,
	CIF_WIFI_EVT_GC_CONNECTED,
	CIF_WIFI_EVT_GC_DISCONNECTED,
	CIF_WIFI_EVT_COUNT,
} cif_wifi_event_id_t;

#define CIF_WIFI_EVENT_IND_MAX_DATA      64

typedef struct {
	uint16_t event_id;
	uint16_t data_len;
	uint8_t data[CIF_WIFI_EVENT_IND_MAX_DATA];
} cif_wifi_event_ind_t;

#ifdef __cplusplus
}
#endif
