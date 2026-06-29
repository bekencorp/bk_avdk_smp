#pragma once

/**
 * @file dm_gattc.h
 *
 * @brief BLE GATTC service wrapper APIs.
 */

#include "components/bluetooth/bk_dm_gattc.h"
#include "components/bluetooth/bk_dm_gatt_common.h"
#include "dm_gatt.h"

/**
 * @brief GATTC application event callback.
 *
 * @param event GATTC callback event.
 * @param gattc_if GATTC interface.
 * @param comm_param Event parameter.
 *
 * @return 0 on success, otherwise error code.
 */
typedef int32_t (* dm_ble_gattc_app_cb)(bk_gattc_cb_event_t event, bk_gatt_if_t gattc_if, bk_ble_gattc_cb_param_t *comm_param);

/**
 * @brief Initialize the GATTC wrapper.
 *
 * @param param Optional GATT initialization parameters.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_gattc_main(cli_gatt_param_t *param);

/**
 * @brief Deinitialize the GATTC wrapper.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_gattc_deinit(void);

/**
 * @brief Connect to a GATT server.
 *
 * @param addr Peer BLE address.
 * @param addr_type Peer BLE address type.
 * @param s_timeout Connection timeout in seconds.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gattc_connect(uint8_t *addr, uint32_t addr_type, uint32_t s_timeout);

/**
 * @brief Connect to a GATT server with custom connection parameters.
 *
 * When pm is NULL, this function uses the default parameters used by
 * dm_gattc_connect(addr, addr_type, 500).
 *
 * @param addr Peer BLE address.
 * @param addr_type Peer BLE address type.
 * @param pm Custom connection parameters, or NULL to use defaults.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gattc_connect_ext(uint8_t *addr, uint32_t addr_type, bk_gap_create_conn_params_t *pm);

/**
 * @brief Disconnect from a GATT server.
 *
 * @param addr Peer BLE address.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gattc_disconnect(uint8_t *addr);

/**
 * @brief Cancel an ongoing GATTC connection attempt.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gattc_connect_cancel(void);

/**
 * @brief Discover services on a GATT server.
 *
 * @param conn_id GATT connection ID.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gattc_discover(uint16_t conn_id);

/**
 * @brief Write an attribute on a GATT server.
 *
 * @param conn_id GATT connection ID.
 * @param attr_handle Attribute handle.
 * @param data Data buffer.
 * @param len Data length in bytes.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gattc_write(uint16_t conn_id, uint16_t attr_handle, uint8_t *data, uint32_t len);

/**
 * @brief Write an attribute with selectable write mode.
 *
 * @param gatt_conn_id GATT connection ID.
 * @param attr_handle Attribute handle.
 * @param data Data buffer.
 * @param len Data length in bytes.
 * @param write_req Non-zero to use write request, zero to use write command.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gattc_write_ext(uint16_t gatt_conn_id, uint16_t attr_handle, uint8_t *data, uint32_t len, uint8_t write_req);

/**
 * @brief Read an attribute from a GATT server.
 *
 * @param gatt_conn_id GATT connection ID.
 * @param attr_handle Attribute handle.
 * @param data Output buffer.
 * @param len Output buffer length in bytes.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gattc_read(uint16_t gatt_conn_id, uint16_t attr_handle, uint8_t *data, uint32_t len);

/**
 * @brief Send an MTU exchange request.
 *
 * @param mac Peer BLE address.
 * @param gatt_conn_id GATT connection ID.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gattc_send_mtu_req(uint8_t *mac, uint8_t gatt_conn_id);

/**
 * @brief Register a GATTC application callback.
 *
 * @param param Callback pointer.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_gattc_add_gattc_callback(void *param);
