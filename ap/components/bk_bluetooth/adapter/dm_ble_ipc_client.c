#include <stdbool.h>
#include <stdint.h>

#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_ble.h"
#include "components/bluetooth/bk_dm_ble_types.h"
#include "components/bluetooth/bk_dm_gatt_common.h"
#include "components/bluetooth/bk_dm_gattc.h"
#include "components/bluetooth/bk_dm_gatts.h"
#include "../include/private/ble_api_5_x.h"

ble_err_t bk_ble_get_att_handle_from_device_handle(ATT_HANDLE *att_handle, DEVICE_HANDLE *device_handle)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_create_connection(ble_conn_param_normal_t *conn_param, ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_create_connection_ex(ble_conn_param_ex_t *conn_param, ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_send_notify(uint8_t conn_handle,
                             uint16_t service_handle,
                             uint16_t char_handle,
                             uint8_t *data,
                             uint16_t len)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatt_db_add_service(GATT_DB_SERVICE_INFO *service_info,
                                     uint16_t num_attr_handles,
                                     uint16_t *service_handle)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatt_db_add_characteristic(uint16_t service_handle,
                                            GATT_DB_UUID_TYPE *char_uuid,
                                            uint16_t perm,
                                            uint16_t property,
                                            ATT_VALUE *char_value,
                                            uint16_t *char_handle)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatt_db_add_characteristic_descriptor(uint16_t service_handle,
                                                       uint16_t char_handle,
                                                       GATT_DB_UUID_TYPE *desc_uuid,
                                                       uint16_t perm,
                                                       ATT_VALUE *desc_value)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatt_db_set_callback(ble_gatt_db_callback_t hndlr_cb)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatt_db_add_completed(void)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_att_write(uint8_t conn_handle, ATT_ATTR_HANDLE hdl, uint8_t *value, uint16_t length)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_advertising_params_extended(uint8_t adv_handle,
                                                 uint16_t adv_event_properties,
                                                 uint32_t primary_advertising_interval_min,
                                                 uint32_t primary_advertising_interval_max,
                                                 uint8_t primary_advertising_channel_map,
                                                 uint8_t own_address_type,
                                                 uint8_t peer_address_type,
                                                 uint8_t *peer_address,
                                                 uint8_t advertising_filter_policy,
                                                 int8_t advertising_tx_power,
                                                 uint8_t primary_advertising_phy,
                                                 uint8_t secondary_adv_max_skip,
                                                 uint8_t secondary_advertising_phy,
                                                 uint8_t advertising_set_id,
                                                 uint8_t scan_req_nfy_enable,
                                                 ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_advertising_params(ble_adv_parameter_t *param, ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_advertising_data_extended(uint8_t advertising_handle,
                                               uint8_t operation,
                                               uint8_t frag_pref,
                                               uint8_t *adv_buff,
                                               uint8_t adv_len,
                                               ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_advertising_data(uint8_t adv_len, uint8_t *adv_buff, ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_scan_response_data_extended(uint8_t advertising_handle,
                                                 uint8_t operation,
                                                 uint8_t fragment_pref,
                                                 uint8_t scan_response_data_length,
                                                 uint8_t *scan_response_data,
                                                 ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_scan_response_data(uint8_t scan_response_data_length,
                                        uint8_t *scan_response_data,
                                        ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_advertising_enable_extended(uint8_t enable,
                                                 uint8_t number_of_sets,
                                                 uint8_t *advertising_handle,
                                                 uint16_t *duration,
                                                 uint8_t *max_extd_adv_evts,
                                                 ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_advertising_enable(uint8_t enable, ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_scan_parameters_extended(uint8_t own_address_type,
                                              uint8_t scanning_filter_policy,
                                              uint8_t scanning_phy,
                                              uint16_t *scan_interval,
                                              uint16_t *scan_window,
                                              ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_scan_parameters(uint8_t own_address_type,
                                     uint8_t scanning_filter_policy,
                                     uint8_t scanning_phy,
                                     uint16_t scan_interval,
                                     uint16_t scan_window,
                                     ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_scan_enable_extended(uint8_t enable,
                                          uint8_t filter_duplicates,
                                          uint16_t duration,
                                          uint16_t period,
                                          ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_disconnect_connection(bd_addr_t *addr, ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_cancel_connect(ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_update_connection_params(ble_update_conn_param_t *conn_param)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_hci_read_phy(bd_addr_t *peer_addr, uint8_t peer_addr_type, ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_hci_set_phy(bd_addr_t *peer_addr,
                             uint8_t peer_addr_type,
                             ble_set_phy_t *le_set_phy,
                             ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_local_name(uint8_t *name, uint8_t name_len, ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_get_local_name(ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_set_gatt_mtu(ATT_HANDLE *att_handle, uint16_t mtu)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatt_read_resp(uint8_t conn_handle, uint8_t *value, uint16_t length)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatt_get_char_val(GATT_DB_HANDLE *handle, ATT_VALUE *attr_value)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_get_conn_handle_from_device_handle(uint8 *conn_handle, DEVICE_HANDLE *device_handle)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ATT_ATTR_HANDLE bk_ble_get_current_gatt_db_attr_handle(void)
{
    return (ATT_ATTR_HANDLE)BK_ERR_BLE_CMD_NOT_SUPPORT;
}

void bk_ble_set_event_callback(ble_event_cb_t func)
{
}

ble_err_t bk_ble_set_random_addr(bd_addr_t *addr, ble_cmd_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_register_callback(bk_gatts_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_create_attr_tab(const bk_gatts_attr_db_t *db,
                                       bk_gatt_if_t gatts_if,
                                       uint16_t count,
                                       uint32_t max_attr_count)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_create_service(bk_gatt_if_t gatts_if,
                                      bk_gatt_srvc_id_t *service_id,
                                      uint16_t num_handle)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_add_char(uint16_t service_attr_handle,
                                bk_bt_uuid_t *char_uuid,
                                bk_gatt_perm_t perm,
                                bk_gatt_char_prop_t property,
                                bk_attr_value_t *char_val,
                                bk_attr_control_t *control)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_add_char_descr(uint16_t service_attr_handle,
                                      uint16_t char_attr_handle,
                                      bk_bt_uuid_t *descr_uuid,
                                      bk_gatt_perm_t perm,
                                      bk_attr_value_t *char_descr_val,
                                      bk_attr_control_t *control)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_send_indicate(bk_gatt_if_t gatts_if,
                                     uint16_t conn_handle,
                                     uint16_t attr_handle,
                                     uint16_t value_len,
                                     uint8_t *value,
                                     bool need_confirm)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_send_response(bk_gatt_if_t gatts_if,
                                     uint16_t conn_id,
                                     uint32_t trans_id,
                                     bk_gatt_status_t status,
                                     bk_gatt_rsp_t *rsp)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_set_attr_value(uint16_t attr_handle, uint16_t length, const uint8_t *value)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_get_attr_value(uint16_t attr_handle, uint16_t *length, uint8_t **value)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_app_register(uint16_t app_id)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_app_unregister(bk_gatt_if_t gatts_if)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_start_service(uint16_t service_handle)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_stop_service(uint16_t service_attr_handle)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatts_send_service_change_indicate(bk_gatt_if_t gatts_if,
                                                    uint16_t conn_id,
                                                    uint8_t all_connected)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_register_callback(bk_gattc_cb_t callback)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_app_register(uint16_t app_id)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_app_unregister(bk_gatt_if_t gattc_if)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_send_mtu_req(bk_gatt_if_t gattc_if, uint16_t conn_id)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_read_char(bk_gatt_if_t gattc_if,
                                 uint16_t conn_id,
                                 uint16_t handle,
                                 bk_gatt_auth_req_t auth_req)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_read_by_type(bk_gatt_if_t gattc_if,
                                    uint16_t conn_id,
                                    uint16_t start_handle,
                                    uint16_t end_handle,
                                    bk_bt_uuid_t *uuid,
                                    bk_gatt_auth_req_t auth_req)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_read_multiple(bk_gatt_if_t gattc_if,
                                     uint16_t conn_id,
                                     bk_gattc_multi_t *read_multi,
                                     bk_gatt_auth_req_t auth_req)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_read_char_descr(bk_gatt_if_t gattc_if,
                                       uint16_t conn_id,
                                       uint16_t handle,
                                       bk_gatt_auth_req_t auth_req)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_write_char(bk_gatt_if_t gattc_if,
                                  uint16_t conn_id,
                                  uint16_t handle,
                                  uint16_t value_len,
                                  uint8_t *value,
                                  bk_gatt_write_type_t write_type,
                                  bk_gatt_auth_req_t auth_req)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_write_char_descr(bk_gatt_if_t gattc_if,
                                        uint16_t conn_id,
                                        uint16_t handle,
                                        uint16_t value_len,
                                        uint8_t *value,
                                        bk_gatt_write_type_t write_type,
                                        bk_gatt_auth_req_t auth_req)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_prepare_write(bk_gatt_if_t gattc_if,
                                     uint16_t conn_id,
                                     uint16_t handle,
                                     uint16_t offset,
                                     uint16_t value_len,
                                     uint8_t *value,
                                     bk_gatt_auth_req_t auth_req)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_prepare_write_char_descr(bk_gatt_if_t gattc_if,
                                                uint16_t conn_id,
                                                uint16_t handle,
                                                uint16_t offset,
                                                uint16_t value_len,
                                                uint8_t *value,
                                                bk_gatt_auth_req_t auth_req)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_execute_write(bk_gatt_if_t gattc_if, uint16_t conn_id, bool is_execute)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gattc_discover(bk_gatt_if_t gattc_if, uint16_t conn_id, bk_gatt_auth_req_t auth_req)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_gatt_set_local_mtu(uint16_t mtu)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

int32_t ble_ethermind_post_msg(uint32_t msg_id, uint32_t sub_msg_id, void *data, uint32_t len, void *cb)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t set_cmd_type(int type, int cmd_type)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

void bk_ble_gap_set_callback(void *cb)
{
}

uint32_t bk_ble_gap_get_scan_duplicate(void)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

void bk_ble_gap_set_scan_duplicate(uint32_t scan_duplicate)
{
}

ble_err_t bk_ble_get_gatt_conn_id_from_hci_handle(uint16_t hci_handle, uint16_t *gatt_conn_id)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

ble_err_t bk_ble_get_hci_handle_from_gatt_conn_id(uint16_t gatt_conn_id, uint16_t *hci_handle)
{
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}
