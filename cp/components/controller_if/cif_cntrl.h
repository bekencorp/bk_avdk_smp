#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cif_main.h"

#define CTRL_IF_CMD     os_printf

struct bk_msg_hdr;

bk_err_t cif_handle_bk_cmd_connect_ind(char *ssid, uint8_t rssi, uint32_t ip, uint32_t gw, uint32_t mk, uint32_t dns);
bk_err_t cif_handle_bk_cmd_disconnect_ind(void);
bk_err_t cif_send_exit_sleep_cfm(void);
bk_err_t cif_handle_bk_cmd(void *cmd);
bk_err_t cif_handle_bk_cmd_enter_sleep_cfm(void);
bk_err_t cif_handle_bk_cmd_get_wlan_status_cfm(char *ssid, uint8_t rssi, uint8_t status, char *ip, char *gw, char *mk, char *dns);
bk_err_t cif_handle_bk_cmd_start_ap_cfm(void);
bk_err_t cif_handle_bk_cmd_start_ap_ind(uint8_t status);
bk_err_t cif_handle_bk_cmd_assoc_ap_ind(uint8_t* mac_addr);
bk_err_t cif_handle_bk_cmd_disassoc_ap_ind(uint8_t* mac_addr);
bk_err_t cif_handle_bk_cmd_stop_ap_ind(uint8_t status);
bk_err_t cif_handle_bk_cmd_scan_wifi_cfm(void);
bk_err_t cif_handle_bk_cmd_scan_wifi_ind(wifi_scan_result_t *scan_result);
bk_err_t cif_send_customer_cmd_cfm(uint8_t *data, uint16_t len, struct bk_msg_hdr *msg);
bk_err_t cif_send_customer_event(uint8_t *data, uint16_t len);
int32_t bluetooth_controller_deinit_api(void);
#ifdef __cplusplus
}
#endif
