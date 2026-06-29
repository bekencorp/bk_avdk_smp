#pragma once

/**
 * @file dm_gatts.h
 *
 * @brief BLE GATTS service wrapper APIs.
 */

#include "components/bluetooth/bk_dm_gatts.h"
#include "components/bluetooth/bk_dm_gatt_common.h"
#include "dm_gatt.h"

/** Build a 16-bit UUID attribute type initializer. */
#define BK_GATT_ATTR_TYPE(iuuid) {.len = BK_UUID_LEN_16, .uuid = {.uuid16 = iuuid}}

/** Build a 16-bit UUID attribute content initializer. */
#define BK_GATT_ATTR_CONTENT(iuuid) {.len = BK_UUID_LEN_16, .uuid = {.uuid16 = iuuid}}

/** Build an attribute value initializer. */
#define BK_GATT_ATTR_VALUE(ilen, ivalue) {.attr_max_len = ilen, .attr_len = ilen, .attr_value = ivalue}

/** Build a 128-bit UUID attribute type initializer. */
#define BK_GATT_ATTR_TYPE_128(iuuid) {.len = BK_UUID_LEN_128, .uuid = {.uuid128 = {iuuid[0], iuuid[1], iuuid[2], iuuid[3], iuuid[4], \
                iuuid[5], iuuid[6], iuuid[7], iuuid[8], iuuid[9], iuuid[10], iuuid[11], iuuid[12], iuuid[13], iuuid[14], iuuid[15]}}}

/** Build a 128-bit UUID attribute content initializer. */
#define BK_GATT_ATTR_CONTENT_128(iuuid) {.len = BK_UUID_LEN_128, .uuid = {.uuid128 = {iuuid[0], iuuid[1], iuuid[2], iuuid[3], iuuid[4], \
                iuuid[5], iuuid[6], iuuid[7], iuuid[8], iuuid[9], iuuid[10], iuuid[11], iuuid[12], iuuid[13], iuuid[14], iuuid[15]}}}

/** Build a primary service declaration with a 16-bit service UUID. */
#define BK_GATT_PRIMARY_SERVICE_DECL(iuuid) \
    .att_desc =\
               {\
                .attr_type = BK_GATT_ATTR_TYPE(BK_GATT_UUID_PRI_SERVICE),\
                .attr_content = BK_GATT_ATTR_CONTENT(iuuid),\
               }

/** Build a primary service declaration with a 128-bit service UUID. */
#define BK_GATT_PRIMARY_SERVICE_DECL_128(iuuid) \
    .att_desc =\
               {\
                .attr_type = BK_GATT_ATTR_TYPE(BK_GATT_UUID_PRI_SERVICE),\
                .attr_content = BK_GATT_ATTR_CONTENT_128(iuuid)\
               }

/** Build a characteristic declaration with a 16-bit characteristic UUID. */
#define BK_GATT_CHAR_DECL(iuuid, ilen, ivalue, iprop, iperm, irsp) \
    .att_desc = \
                {\
                 .attr_type = BK_GATT_ATTR_TYPE(BK_GATT_UUID_CHAR_DECLARE),\
                 .attr_content = BK_GATT_ATTR_CONTENT(iuuid),\
                 .value = BK_GATT_ATTR_VALUE(ilen, ivalue),\
                 .prop = iprop,\
                 .perm = iperm,\
                },\
                .attr_control = {.auto_rsp = irsp}

/** Build a characteristic declaration with a 128-bit characteristic UUID. */
#define BK_GATT_CHAR_DECL_128(iuuid, ilen, ivalue, iprop, iperm, irsp) \
    .att_desc = \
                {\
                 .attr_type = BK_GATT_ATTR_TYPE(BK_GATT_UUID_CHAR_DECLARE),\
                 .attr_content = BK_GATT_ATTR_CONTENT_128(iuuid),\
                 .value = BK_GATT_ATTR_VALUE(ilen, ivalue),\
                 .prop = iprop,\
                 .perm = iperm,\
                },\
                .attr_control = {.auto_rsp = irsp}

/** Build a characteristic descriptor declaration with a 16-bit descriptor UUID. */
#define BK_GATT_CHAR_DESC_DECL(iuuid, ilen, ivalue, iperm, irsp) \
    .att_desc = \
                {\
                 .attr_type = BK_GATT_ATTR_TYPE(iuuid),\
                 .value = BK_GATT_ATTR_VALUE(ilen, ivalue),\
                 .perm = iperm,\
                },\
                .attr_control = {.auto_rsp = irsp}

/** Build a characteristic descriptor declaration with a 128-bit descriptor UUID. */
#define BK_GATT_CHAR_DESC_DECL_128(iuuid, ilen, ivalue, iperm, irsp) \
    .att_desc = \
                {\
                 .attr_type = BK_GATT_ATTR_TYPE_128(iuuid),\
                 .value = BK_GATT_ATTR_VALUE(ilen, ivalue),\
                 .perm = iperm,\
                },\
                .attr_control = {.auto_rsp = irsp}

/**
 * @brief GATTS database event callback.
 *
 * @param event GATTS callback event.
 * @param gatts_if GATTS interface.
 * @param param Event parameter.
 *
 * @return 0 on success, otherwise error code.
 */
typedef int32_t (* dm_ble_gatts_db_cb)(bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if, bk_ble_gatts_cb_param_t *param);

/**
 * @brief GATTS application event callback.
 *
 * @param event GATTS callback event.
 * @param gatts_if GATTS interface.
 * @param comm_param Event parameter.
 *
 * @return 0 on success, otherwise error code.
 */
typedef int32_t (* dm_ble_gatts_app_cb)(bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if, bk_ble_gatts_cb_param_t *comm_param);

/**
 * @brief Check whether GATTS has been initialized.
 *
 * @return Non-zero if initialized, otherwise zero.
 */
int32_t dm_gatts_is_init(void);

/**
 * @brief Initialize the GATTS wrapper.
 *
 * @param param Optional GATT initialization parameters.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_gatts_main(cli_gatt_param_t *param);

/**
 * @brief Deinitialize the GATTS wrapper.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_gatts_deinit(void);

/**
 * @brief Deinitialize GATTS during Bluetooth deinitialization.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatts_deinit_because_bluetooth_deinit_future(void);

/**
 * @brief Disconnect a GATTS link by peer address.
 *
 * @param addr Peer BLE address.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatts_disconnect(uint8_t *addr);

/**
 * @brief Enable or disable advertising.
 *
 * @param enable Non-zero to enable advertising, zero to disable.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatts_enable_adv(uint8_t enable);

/**
 * @brief Enable or disable a registered GATTS service.
 *
 * @param index Service index.
 * @param enable Non-zero to enable the service, zero to disable.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatts_enable_service(uint32_t index, uint8_t enable);

/**
 * @brief Register a GATTS attribute database.
 *
 * @param list Attribute database list.
 * @param count Number of attributes in the list.
 * @param attr_handle_list Output attribute handle list.
 * @param cb Database event callback.
 * @param need_create_tab Non-zero to create the attribute table.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatts_reg_db(bk_gatts_attr_db_t *list, uint32_t count, uint16_t *attr_handle_list, dm_ble_gatts_db_cb cb, uint8_t need_create_tab);

/**
 * @brief Unregister a GATTS attribute database.
 *
 * @param list Attribute database list.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatts_unreg_db(bk_gatts_attr_db_t *list);

/**
 * @brief Find an attribute value buffer by attribute handle.
 *
 * @param attr_list Attribute database list.
 * @param attr_handle_list Attribute handle list.
 * @param size Number of attributes in the list.
 * @param attr_handle Attribute handle to find.
 * @param output_index Output index in the attribute list.
 * @param output_buff Output pointer to the attribute value buffer.
 * @param output_size Output attribute value buffer size.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatts_get_buff_from_attr_handle(bk_gatts_attr_db_t *attr_list, uint16_t *attr_handle_list, uint32_t size, uint16_t attr_handle, uint32_t *output_index, uint8_t **output_buff, uint32_t *output_size);

/**
 * @brief Get the current GATTS interface.
 *
 * @return Current GATT interface.
 */
bk_gatt_if_t dm_gatts_get_current_if(void);

/**
 * @brief Register a GATTS application callback.
 *
 * @param param Callback pointer.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_gatts_add_gatts_callback(void *param);

/**
 * @brief Send a Service Changed indication.
 *
 * @param conn_id GATT connection ID.
 * @param all_connected Non-zero to send to all connected peers.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatts_send_service_change_indicate(uint16_t conn_id, uint8_t all_connected);

/**
 * @brief Send notification or indication.
 *
 * @param gatt_conn_id GATT connection ID.
 * @param attr_handle Attribute handle.
 * @param data Data buffer.
 * @param len Data length in bytes.
 * @param is_notify Non-zero to send notification, zero to send indication.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatts_send_notify(uint16_t gatt_conn_id, uint16_t attr_handle, uint8_t *data, uint32_t len, uint8_t is_notify);
