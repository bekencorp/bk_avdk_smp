#include <stdbool.h>
#include <stdint.h>

#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_gatt_common.h"
#include "components/bluetooth/bk_dm_gattc.h"
#include "components/bluetooth/bk_dm_gatts.h"
#include "../../include/private/ble_api_5_x.h"

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
