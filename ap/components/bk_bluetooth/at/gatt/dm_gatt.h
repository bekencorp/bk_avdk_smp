#pragma once

/*
 * Compatibility shim. The real implementation moved to
 *   ap/components/bk_bluetooth/service/dm/ble/gatt/dm_gatt.{c,h}
 * Existing AT-layer code that still calls bk_at_dm_gatt* / bk_at_dm_ble_gap_*
 * keeps working through the macros below.
 */
#include "../../service/dm/ble/gatt/dm_gatt.h"


#if CONFIG_BT && CONFIG_BLE

#define bk_at_dm_gatt_main                          dm_gatt_main
#define bk_at_dm_gatt_deinit                        dm_gatt_deinit
#define bk_at_dm_gatt_disable_all                   dm_gatt_disable_all
#define bk_at_dm_gatt_add_gap_callback              dm_gatt_add_gap_callback
#define bk_at_dm_gatt_get_authen_status             dm_gatt_get_authen_status
#define bk_at_dm_gatt_find_id_info_by_nominal_info  dm_gatt_find_id_info_by_nominal_info
#define bk_at_dm_gatt_passkey_reply                 dm_gatt_passkey_reply
#define bk_at_dm_gatt_set_security_method           dm_gatt_set_security_method
#define bk_at_dm_gatt_is_linkkey_distr_from_ltk     dm_gatt_is_linkkey_distr_from_ltk
#define bk_at_dm_ble_gap_create_bond                dm_ble_gap_create_bond
#define bk_at_dm_ble_gap_remove_bond                dm_ble_gap_remove_bond
#define bk_at_dm_ble_gap_get_bonded_count           dm_ble_gap_get_bonded_count
#define bk_at_dm_ble_gap_clean_bond                 dm_ble_gap_clean_bond
#define bk_at_dm_ble_gap_show_bond_list             dm_ble_gap_show_bond_list
#define bk_at_dm_ble_gap_get_bond_info_by_addr      dm_ble_gap_get_bond_info_by_addr
#define bk_at_dm_ble_gap_bond_info_foreach          dm_ble_gap_bond_info_foreach
#define bk_at_dm_ble_gap_clean_local_key            dm_ble_gap_clean_local_key
#define bk_at_dm_ble_gap_update_param               dm_ble_gap_update_param
#define bk_at_dm_ble_gap_get_rpa                    dm_ble_gap_get_rpa
#define bk_at_dm_ble_gap_get_identity_addr          dm_ble_gap_get_identity_addr
#define bk_at_dm_ble_gap_get_current_conn_id        dm_ble_gap_get_current_conn_id
#define bk_at_dm_ble_gap_set_auto_accept_pair_req   dm_ble_gap_set_auto_accept_pair_req
#define bk_at_dm_gatt_disconnect                    dm_gatt_disconnect
#define bk_at_dm_gatt_connect_cancel                dm_gatt_connect_cancel

#define g_bk_at_dm_gap_use_rpa                      g_dm_gap_use_rpa

#else

#define bk_at_dm_gatt_main(...) 0
#define bk_at_dm_gatt_deinit(...) 0
#define bk_at_dm_gatt_disable_all(...) 0
#define bk_at_dm_gatt_add_gap_callback(...) 0
#define bk_at_dm_gatt_get_authen_status(...) 0
#define bk_at_dm_gatt_find_id_info_by_nominal_info(...) 0
#define bk_at_dm_gatt_passkey_reply(...) 0
#define bk_at_dm_gatt_set_security_method(...) 0
#define bk_at_dm_gatt_is_linkkey_distr_from_ltk(...) 0
#define bk_at_dm_ble_gap_create_bond(...) 0
#define bk_at_dm_ble_gap_remove_bond(...) 0
#define bk_at_dm_ble_gap_get_bonded_count(...) 0
#define bk_at_dm_ble_gap_clean_bond(...) 0
#define bk_at_dm_ble_gap_show_bond_list(...) 0
#define bk_at_dm_ble_gap_get_bond_info_by_addr(...) 0
#define bk_at_dm_ble_gap_bond_info_foreach(...) 0
#define bk_at_dm_ble_gap_clean_local_key(...) 0
#define bk_at_dm_ble_gap_update_param(...) 0
#define bk_at_dm_ble_gap_get_rpa(...) 0
#define bk_at_dm_ble_gap_get_identity_addr(...) 0
#define bk_at_dm_ble_gap_get_current_conn_id(...) 0
#define bk_at_dm_ble_gap_set_auto_accept_pair_req(...) 0
#define bk_at_dm_gatt_disconnect(...) 0
#define bk_at_dm_gatt_connect_cancel(...) 0

#endif
