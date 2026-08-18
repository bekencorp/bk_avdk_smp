// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDE_MODULES_BK_BLE_API_H_
#define INCLUDE_MODULES_BK_BLE_API_H_


#include "components/bluetooth/bk_ble_types.h"

#ifdef __cplusplus
extern"C" {
#endif

/**
 * @brief ble APIs Version 1.0
 * @defgroup bk_ble_api_v1 New ble api group
 * @{
 */

/**
 * @brief     Register a gatt service
 * @param
 *     - ble_db_cfg: service param
 *
 * User example:
 *     First we must build test_service_db
 *     test_service_db is a database for att, which used in ble discovery. reading writing and other operation is used on a att database.
 *
 *
 * @code
 *     enum {
 *         TEST_IDX_SVC,
 *         TEST_IDX_CHAR_DECL,
 *         TEST_IDX_CHAR_VALUE,
 *         TEST_IDX_CHAR_DESC,
 *
 *         TEST_IDX_CHAR_DATALEN_DECL,
 *         TEST_IDX_CHAR_DATALEN_VALUE,
 *
 *         TEST_IDX_CHAR_INTER_DECL,
 *         TEST_IDX_CHAR_INTER_VALUE,
 *
 *         TEST_IDX_NB,
 *     };
 *
 *     //att records database.
 *
 *     ble_attm_desc_t test_service_db[TEST_IDX_NB] = {
 *        //  Service Declaration
 *        [TEST_IDX_SVC]              = {DECL_PRIMARY_SERVICE_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
 *        // Characteristic declare
 *        [TEST_IDX_CHAR_DECL]    = {DECL_CHARACTERISTIC_128,  BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
 *        // Characteristic Value
 *        [TEST_IDX_CHAR_VALUE]   = {{0x34, 0x12, 0},     BK_BLE_PERM_SET(NTF, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},
 *        //Client Characteristic Configuration Descriptor
 *        [TEST_IDX_CHAR_DESC] = {DESC_CLIENT_CHAR_CFG_128, BK_BLE_PERM_SET(RD, ENABLE) | BK_BLE_PERM_SET(WRITE_REQ, ENABLE), 0, 0},
 *     };
 * @endcode
 * TEST_IDX_SVC is nessecery, is declare a primary att service. The macro define is:
 *
 * @code
 *     #define DECL_PRIMARY_SERVICE_128     {0x00,0x28,0}
 * @endcode
 *
 * which is an UUID say it is a "primary service"
 * BK_BLE_PERM_SET(RD, ENABLE) means it can be read, and must be read, so it can be discove by peer master.
 *
 * TEST_IDX_CHAR_DECL declare a characteristic as a element in service, it must be BK_BLE_PERM_SET(RD, ENABLE)
 *
 * @code
 * #define DECL_CHARACTERISTIC_128      {0x03,0x28,0}
 * @endcode
 * show it's a "characteristic"
 *
 * BK_BLE_PERM_SET(RD, ENABLE) means it can be read, and must be read, so it can be discove by peer master.
 *
 *
 * TEST_IDX_CHAR_VALUE is the real value of TEST_IDX_CHAR_DECL,
 * {0x34, 0x12, 0} means it's type is 0x1234, you can determine by you self
 * BK_BLE_PERM_SET(NTF, ENABLE) means it cant notify peer, such as value change. For exzample, BLE mouse report pos by "notify" peer.
 * BK_BLE_PERM_SET(RI, ENABLE) means if peer read this att record, it will enable notification.
 * BK_BLE_PERM_SET(UUID_LEN, UUID_16) means the first elem's max len of TEST_IDX_CHAR_VALUE.
 *
 * TEST_IDX_CHAR_DESC is a Client Characteristic Configuration Descriptor for TEST_IDX_CHAR_VALUE, it used by peer master as know as a client.
 * As common usage, it config TEST_IDX_CHAR_VALUE indication or notification. Peer can write this att handle to triggle it.
 * Must be BK_BLE_PERM_SET(RD, ENABLE) | BK_BLE_PERM_SET(WRITE_REQ, ENABLE)
 *
 * Now, you have a basic database for peer, in this case, peer write TEST_IDX_CHAR_DESC or read TEST_IDX_CHAR_VALUE to enable notification, and then we notify peer by TEST_IDX_CHAR_VALUE
 *
 *
 * Secondlly, we build ble_db_cfg
 * @code
 *     struct bk_ble_db_cfg ble_db_cfg;
 *
 *     ble_db_cfg.att_db = (ble_attm_desc_t *)test_service_db;
 *     ble_db_cfg.att_db_nb = TEST_IDX_NB;
 *     ble_db_cfg.prf_task_id = g_test_prf_task_id;
 *     ble_db_cfg.start_hdl = 0;
 *     ble_db_cfg.svc_perm = BK_BLE_PERM_SET(SVC_UUID_LEN, UUID_16);
 * @endcode
 * prf_task_id is app handle. If you have multi att service, used prf_task_id to distinguish it.
 * svc_perm show TEST_IDX_SVC UUID type's len.
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_create_db (struct bk_ble_db_cfg* ble_db_cfg);

/**
 * @brief     Register ble event notification callback
 *
 * @param
 *    - func: event callback
 *
 * @attention 1. you must regist it, otherwise you cant get any event !
 * @attention 2. you must regist it before bk_ble_create_db, otherwise you cant get BLE_5_CREATE_DB event
 *
 * User example:
 * @code
 * void ble_at_notice_cb(ble_notice_t notice, void *param)
{
    switch (notice) {

    case BLE_5_WRITE_EVENT: {

        if (w_req->prf_id == g_test_prf_task_id)
        {
            switch(w_req->att_idx)
            {
            case TEST_IDX_CHAR_DECL:
                break;
            case TEST_IDX_CHAR_VALUE:
                break;
            case TEST_IDX_CHAR_DESC:
                //when peer enable notification, we create time and notify peer, such as
                //write_buffer = (uint8_t *)os_malloc(s_test_data_len);
                //bk_ble_send_noti_value(s_test_data_len, write_buffer, g_test_prf_task_id, TEST_IDX_CHAR_VALUE);
                break;

            default:
                break;
            }
        }
        break;
    }
    case BLE_5_CREATE_DB:
    //bk_ble_create_db success here
    break;
    }
}

bk_ble_set_notice_cb(ble_at_notice_cb);
 * @endcode
 * @return
 *    - void
 */
void bk_ble_set_notice_cb(ble_notice_cb_t func);

/**
 * @brief     Get device name
 *
 * @param
 *    - name: store the device name
 *    - buf_len: the length of buf to store the device name
 *
 * @return
 *    - length: the length of device name
 */
uint8_t bk_ble_appm_get_dev_name(uint8_t* name, uint32_t buf_len);

/**
 * @brief     Set device name
 *
 * @param
 *    - len: the length of device name
 *    - name: the device name to be set
 *
 * @return
 *    - length: the length of device name
 */
uint8_t bk_ble_appm_set_dev_name(uint8_t len, uint8_t* name);

/**
 * @brief     Create a ble advertising activity
 *
 * @param
 *    - actv_idx: the index of activity
 *    - adv_param: the advertising parameter
 *    - callback: register a callback for this action, ble_cmd_t: BLE_CREATE_ADV
 * @attention 1.you must wait callback status, 0 mean success.
 *
 * User example:
 * @code
 *     ble_adv_param_t adv_param;
 *
 *     adv_param.own_addr_type = 0;//BLE_STATIC_ADDR
 *     adv_param.adv_type = 0; //ADV_IND
 *     adv_param.chnl_map = 7;
 *     adv_param.adv_prop = 3;
 *     adv_param.adv_intv_min = 0x120; //min
 *     adv_param.adv_intv_max = 0x160; //max
 *     adv_param.prim_phy = 1;// 1M
 *     adv_param.second_phy = 1;// 1M
 *     actv_idx = bk_ble_get_idle_actv_idx_handle();
 *     if (actv_idx != UNKNOW_ACT_IDX) {
 *         bk_ble_create_advertising(actv_idx, &adv_param, ble_at_cmd_cb);
 *     }
 * @endcode
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_create_advertising(uint8_t actv_idx, ble_adv_param_t *adv_param, ble_cmd_cb_t callback);

/**
 * @brief     modify a ble advertising activity
 *
 * @param
 *    - actv_idx: the index of activity
 *    - adv_param: the advertising parameter
 *    - callback: register a callback for this action, ble_cmd_t: BLE_MODIFY_ADV
 * @attention 1.you must wait callback status, 0 mean success. 2.This function only be called when adv created and not started.
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_modify_advertising(uint8_t actv_idx, ble_adv_param_t *adv_param, ble_cmd_cb_t callback);

/**
 * @brief     Start a ble advertising
 *
 * @param
 *    - actv_idx: the index of activity
 *    - duration: Advertising duration (in unit of 10ms). 0 means that advertising continues
 *    - callback: register a callback for this action, ble_cmd_t: BLE_START_ADV
 *
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_create_advertising
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_start_advertising(uint8_t actv_idx, uint16 duration, ble_cmd_cb_t callback);

/**
 * @brief     Start a ble advertising with a maximum number of advertising events
 *
 * @param
 *    - actv_idx: the index of activity
 *    - duration: Advertising duration (in unit of 10ms). 0 means that advertising continues
 *    - max_evt: maximum number of advertising events to send. 0 means no limit
 *    - callback: register a callback for this action, ble_cmd_t: BLE_START_ADV
 *
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_create_advertising
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_start_advertising_ext(uint8_t actv_idx, uint16 duration, uint8_t max_evt, ble_cmd_cb_t callback);

/**
 * @brief     Stop the advertising that has been started
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action, ble_cmd_t: BLE_STOP_ADV
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_start_advertising
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_stop_advertising(uint8_t actv_idx, ble_cmd_cb_t callback);

/**
 * @brief     Delete the advertising that has been created
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action, ble_cmd_t: BLE_DELETE_ADV
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_create_advertising
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_delete_advertising(uint8_t actv_idx, ble_cmd_cb_t callback);

/**
 * @brief     Set the advertising data
 *
 * @param
 *    - actv_idx: the index of activity
 *    - adv_buff: advertising data
 *    - adv_len: the length of advertising data
 *    - callback: register a callback for this action, ble_cmd_t: BLE_SET_ADV_DATA
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_create_advertising
 *
 *
 * User example:
 * @code
 *     const uint8_t adv_data[] = {0x02, 0x01, 0x06, 0x0A, 0x09, 0x37, 0x32, 0x33, 0x31, 0x4e, 0x5f, 0x42, 0x4c, 0x45};
 *     bk_ble_set_adv_data(actv_idx, adv_data, sizeof(adv_data), ble_at_cmd_cb);
 * @endcode
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_set_adv_data(uint8_t actv_idx, uint8_t* adv_buff, uint8_t adv_len, ble_cmd_cb_t callback);

/**
 * @brief     Set the scan response data
 *
 * @param
 *    - actv_idx: the index of activity
 *    - scan_buff: scan response data
 *    - scan_len: the length of scan response data
 *    - callback: register a callback for this action, ble_cmd_t: BLE_SET_RSP_DATA
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.scan rsp data similaly to adv data
 * @attention 3.must used after bk_ble_create_advertising
 *
 *
 * User example:
 * @code
 *     const uint8_t scan_data[] = {0x02, 0x01, 0x06, 0x0A, 0x09, 0x37, 0x32, 0x33, 0x31, 0x4e, 0x5f, 0x42, 0x4c, 0x45};
 *     bk_ble_set_scan_rsp_data(actv_idx, scan_data, sizeof(scan_data), ble_at_cmd_cb);
 * @endcode
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_set_scan_rsp_data(uint8_t actv_idx, uint8_t* scan_buff, uint8_t scan_len, ble_cmd_cb_t callback);

/**
 * @brief     Set the periodic advertising data
 *
 * @param
 *    - actv_idx: the index of activity
 *    - per_adv_buff: periodic advertising data
 *    - per_adv_len: the length of periodic advertising data
 *    - callback: register a callback for this action, ble_cmd_t: BLE_SET_ADV_DATA
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_create_advertising
 *
 * User example:
 * @code
 *     const uint8_t adv_data[] = {0x02, 0x01, 0x06, 0x0A, 0x09, 0x37, 0x32, 0x33, 0x31, 0x4e, 0x5f, 0x42, 0x4c, 0x45};
 *     bk_ble_set_per_adv_data(actv_idx, adv_data, sizeof(adv_data), ble_at_cmd_cb);
 * @endcode
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_set_per_adv_data(uint8_t actv_idx, uint8_t* per_adv_buff, uint8_t per_adv_len, ble_cmd_cb_t callback);

/**
 * @brief     Set the adv random addr
 *
 * @param
 *    - actv_idx: the index of activity
 *    - addr: random address
 *    - callback: register a callback for this action, ble_cmd_t: BLE_SET_ADV_RANDOM_ADDR
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_create_advertising
 *
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_set_adv_random_addr(uint8_t actv_idx, uint8_t* addr, ble_cmd_cb_t callback);

/**
 * @brief     Read the phy of connection device
 *
 * @param
 *    - conn_idx: the index of connection device
 * @attention 1.must used after after connected
 *
 * User example:
 * @code
 *     bk_ble_read_phy(conn_idx);
 * @endcode
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_read_phy(uint8_t conn_idx);


/**
 * @brief     Set the phy of connection device
 *
 * @param
 *    - conn_idx: the index of connection device
 *    - phy_info: phy parameters
 * @attention 1.must used after after connected
 *
 * User example:
 * @code
 *     ble_set_phy_t * phy = {0x04, 0x01, 0x01};
 *     //set tx phy to s2 coded phy, and set rx phy to 1M phy
 *     bk_ble_set_phy(1, phy);
 * @endcode
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_set_phy(uint8_t conn_idx, ble_set_phy_t * phy_info);

/**
 * @brief     Update connection parameters
 *
 * @param
 *    - conn_idx: the index of connection
 *    - conn_param: connection parameters
 * @attention 1.must used after connected
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_update_param(uint8_t conn_idx, ble_conn_param_t *conn_param);

/**
 * @brief     Disconnect a ble connection
 *
 * @param
 *    - conn_idx: the index of connection
 * @attention 1.must used after connected
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_disconnect(uint8_t conn_idx);

/**
 * @brief     Exchange MTU, negotiating with the local maximum MTU
 *
 * This API always requests the local maximum MTU. If you want to negotiate a
 * specified MTU value, use bk_ble_gatt_mtu_change_ex() instead.
 *
 * @param
 *    - conn_idx: the index of connection
 * @attention 1.must used after connected
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_gatt_mtu_change(uint8_t conn_idx);

/**
 * @brief     Exchange MTU with a specified value
 *
 * @param
 *    - conn_idx: the index of connection
 *    - mtu: the MTU value the client requests. The finally negotiated MTU is
 *           the minimum of this value, the peer's MTU and the local maximum MTU,
 *           and is also clamped to the minimum legal ATT MTU. Passing 0 means
 *           using the local maximum MTU (same as bk_ble_gatt_mtu_change).
 * @attention 1.must used after connected
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_gatt_mtu_change_ex(uint8_t conn_idx, uint16_t mtu);

/**
 * @brief     Set maximal Exchange MTU
 *
 * @param
 *    - max_mtu: the value to set
 * @attention 1.must used before connected
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_set_max_mtu(uint16_t max_mtu);

/**
 * @brief     Create a ble scan activity
 *
 * @param
 *    - actv_idx: the index of activity
 *    - scan_param: the scan parameter
 *    - callback: register a callback for this action, ble_cmd_t: BLE_CREATE_SCAN
 *
 * @attention 1.you must wait callback status, 0 mean success.
 *
 * User example:
 * @code
 *     ble_scan_param_t scan_param;

 *     scan_param.own_addr_type = 0;//BLE_STATIC_ADDR
 *     scan_param.scan_phy = 5;
 *     scan_param.scan_intv = 0x64; //interval
 *     scan_param.scan_wd = 0x1e; //windows
 *     bk_ble_create_scaning(actv_idx, &scan_param, ble_at_cmd);
 *
 * @endcode
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_create_scaning(uint8_t actv_idx, ble_scan_param_t *scan_param, ble_cmd_cb_t callback);

/**
 * @brief     Start a ble scan
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action, ble_cmd_t: BLE_START_SCAN
 *
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_create_scaning
 * @attention 3.adv will report in ble_notice_cb_t as BLE_5_REPORT_ADV
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_start_scaning(uint8_t actv_idx, ble_cmd_cb_t callback);

ble_err_t bk_ble_start_scaning_ex(uint8_t actv_idx, uint8_t filt_duplicate, uint16_t duration, uint16_t period, ble_cmd_cb_t callback);


/**
 * @brief     Stop the scan that has been started
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action, ble_cmd_t: BLE_STOP_SCAN
 *
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_start_scaning
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_stop_scaning(uint8_t actv_idx, ble_cmd_cb_t callback);

/**
 * @brief     Delete the scan that has been created
 *
 * @param
 *    - actv_idx: the index of activity
 *    - callback: register a callback for this action, ble_cmd_t: BLE_DELETE_SCAN
 *
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_create_scaning
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_delete_scaning(uint8_t actv_idx, ble_cmd_cb_t callback);

/**
 * @brief     Create a activity for initiating a connection
 *
 * @param
 *    - con_idx: the index of connection
 *    - conn_param: the connection parameter
 *    - callback: register a callback for this action, ble_cmd_t: BLE_INIT_CREATE
 *
 * @attention 1.you must wait callback status, 0 mean success.
 *
 * User example:
 * @code
 *
 *  ble_conn_param_t conn_param;
    conn_param.intv_min = 0x40; //interval
    conn_param.intv_max = 0x40; //interval
    conn_param.con_latency = 0;
    conn_param.sup_to = 0x200;//supervision timeout
    conn_param.init_phys = INIT_PHY_TYPE_LE_1M;// 1M
    conn_param.filter_policy = INIT_TYPE_LE_DIRECT_CONN_EST;// initiating type, see enum \ref initiating_filter_type_le (DIRECT: connect the indicated peer address; AUTO: connect devices in the white list)
    bk_ble_create_init(con_idx, &conn_param, ble_at_cmd);
 * @endcode
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_create_init(uint8_t con_idx, ble_conn_param_t *conn_param, ble_cmd_cb_t callback);

/**
 * @brief     Initiate a connection
 *
 * @param
 *    - con_idx: the index of connection
 *    - callback: register a callback for this action, ble_cmd_t: BLE_INIT_START_CONN
 *
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_create_init
 * @attention 3.when connect result, will recv BLE_5_INIT_CONNECT_EVENT in ble_notice_cb_t
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_init_start_conn(uint8_t con_idx, ble_cmd_cb_t callback);

/**
 * @brief     Stop a connection
 *
 * @param
 *    - con_idx: the index of connection
 *    - callback: register a callback for this action, ble_cmd_t: BLE_INIT_STOP_CONN
 *
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_init_start_conn
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_init_stop_conn(uint8_t con_idx,ble_cmd_cb_t callback);//todo: upper feature

/**
 * @brief     Set the address of the device to be connected
 *
 * @param
 *    - connidx: the index of connection
 *    - bdaddr: the address of the device to be connected
 *    - addr_type: the address type of the device to be connected, 0: public 1: random
 *
 *
 * @attention 1.you must wait callback status, 0 mean success.
 * @attention 2.must used after bk_ble_create_init
 * @attention 3.addr_type must right, if wrong, cant connect
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_init_set_connect_dev_addr(uint8_t connidx, bd_addr_t *bdaddr, uint8_t addr_type);



ble_err_t bk_ble_create_periodic_sync(uint8_t actv_idx, ble_cmd_cb_t callback);
ble_err_t bk_ble_start_periodic_sync(uint8_t actv_idx, ble_periodic_param_t *param, ble_cmd_cb_t callback);
ble_err_t bk_ble_stop_periodic_sync(uint8_t actv_idx, ble_cmd_cb_t callback);
ble_err_t bk_ble_delete_periodic_sync(uint8_t actv_idx, ble_cmd_cb_t callback);

/**
 * @brief     Get an idle activity
 *
 * @return
 *    - xx: the idle activity's index
 */
uint8_t bk_ble_get_idle_actv_idx_handle(void);

/**
 * @brief     Get the maximum count of activities
 *
 * @return
 *    - xx: the maximum count of activities
 */
uint8_t bk_ble_get_max_actv_idx_count(void);

/**
 * @brief     Get the maximum count of supported connections
 *
 * @return
 *    - xx: the maximum count of supported connections
 */
uint8_t bk_ble_get_max_conn_idx_count(void);


/**
 * @brief     Get an idle connection activity
 *
 * @return
 *    - xx: the idle connection activity's index
 */
uint8_t bk_ble_get_idle_conn_idx_handle(void);


/**
 * @brief     Find the specific connection activity by address
 *
 * @param
 *    - connt_addr: the address of the connected device
 *
 * @return
 *    - xx: the index of the connection activity meeting the address
 */
uint8_t bk_ble_find_conn_idx_from_addr(bd_addr_t *connt_addr);


/**
 * @brief     Get the connection state of the specific device
 *
 * @param
 *    - connt_addr: the device's address
 *
 * @return
 *    - 1: this device is connected
 *    - 0: this device is disconnected
 */
uint8_t bk_ble_get_connect_state(bd_addr_t * connt_addr);

/**
 * @brief     get ble mac addr,this api is deprecated,please use bk_bluetooth_get_address
 *
 * @attention 1. return mac is 6 bytes.
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_get_mac(uint8_t *mac);

/**
 * @brief As slaver, send a notification of an attribute's value
 *
 * @param
 *    - con_idx: the index of connection
 *    - len: the length of attribute's value
 *    - buf: attribute's value
 *    - prf_id: The id of the profile
 *    - att_idx: The index of the attribute
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_send_noti_value(uint8_t con_idx,uint32_t len, uint8_t *buf, uint16_t prf_id, uint16_t att_idx);

/**
 * @brief As slaver, send an indication of an attribute's value
 *
 * @param
 *    - con_idx: the index of connection
 *    - len: the length of attribute's value
 *    - buf: attribute's value
 *    - prf_id: The id of the profile
 *    - att_idx: The index of the attribute
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_send_ind_value(uint8_t con_idx,uint32_t len, uint8_t *buf, uint16_t prf_id, uint16_t att_idx);

/**
 * @brief     reg hci recv callback
 *
 * @param
 *    - evt_cb: evt callback function
 *    - acl_cb: acl callback function
 *
 * @attention 1. you must call this after recv BLE_5_STACK_OK evt !
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_reg_hci_recv_callback(ble_hci_to_host_cb evt_cb, ble_hci_to_host_cb acl_cb);

/**
 * @brief     reg sco hci recv callback
 *
 * @param
 *    - sco_cb: sco callback function
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_reg_sco_hci_recv_callback(ble_hci_to_host_cb sco_cb);

/**
 * @brief register iso hci recv callback
 *
 * @param
 *    - iso_cb: iso callback function
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_reg_iso_hci_recv_callback(ble_hci_to_host_cb iso_cb);

/**
 * @brief send hci to controller.
 *
 *
 * @param
 *   - type: see \ref BK_BLE_HCI_TYPE.
 * - buf: payload
 * - len: buf's len
 *
 * @attention 1. you must call this after bk_ble_reg_hci_recv_callback !
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
**/
ble_err_t bk_ble_hci_to_controller(uint8_t type, uint8_t *buf, uint16_t len);



/**
 * @brief send hci cmd to controller.
 *
 *
 * @param
 * - buf: payload
 * - len: buf's len
 *
 * @attention 1. you must call this after bk_ble_reg_hci_recv_callback !
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
**/
ble_err_t bk_ble_hci_cmd_to_controller(uint8_t *buf, uint16_t len);

/**
 * @brief send hci acl to controller.
 *
 *
 * @param
 * - buf: payload
 * - len: buf's len
 *
 * @attention 1. you must call this after bk_ble_reg_hci_recv_callback !
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
**/
ble_err_t bk_ble_hci_acl_to_controller(uint8_t *buf, uint16_t len);

/**
 * @brief send hci iso data to controller (host -> controller, BIS/CIS TX, e.g. Auracast source).
 *
 *
 * @param
 * - buf: payload (HCI ISO packet without the type byte)
 * - len: buf's len
 *
 * @attention 1. you must call this after bk_ble_reg_hci_recv_callback !
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
**/
ble_err_t bk_ble_hci_iso_to_controller(uint8_t *buf, uint16_t len);



/*
 * @brief get if stack support central and link count
 *
 * @param
 * - count: if return true, show how many central link can be support, otherwise not used.
 *
 * @return
 *    - 1: support
 *    - 0: not support.
 */
uint8_t bk_ble_if_support_central(uint8_t *count);

/*
 * @brief get controller stack type
 *
 * @return
 *    enum BK_BLE_CONTROLLER_STACK_TYPE
 */

BK_BLE_CONTROLLER_STACK_TYPE bk_ble_get_controller_stack_type(void);


/*
 * @brief get host stack type
 *
 * @return
 *    enum BK_BLE_HOST_STACK_TYPE
 */
BK_BLE_HOST_STACK_TYPE bk_ble_get_host_stack_type(void);


/*
 * @brief get ble environment state,this api is deprecated,please use bk_bluetooth_get_status.
 *
 * @return
 *    - 1: ready
 *    - 0: not ready
 */
uint8_t bk_ble_get_env_state(void);

/*
 * @brief set ble task stack size, default is 3072
 *
 * @param
 *    - size: stack size
 *
 *
 * @attention 1.you must call it before app_ble_init !!!!
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - BK_ERR_BLE_FAIL: fail, because you call this func before app_ble_init !!!
 */
ble_err_t bk_ble_set_task_stack_size(uint16_t size);

/*
 * @brief register a callback that will report the action of notification/indication/read/write
 *
 * @param
 *    - cb: callback
 *
 * @return
 * - void
 */
void bk_ble_register_app_sdp_charac_callback(app_sdp_charac_callback cb);

/*
 * @brief register a callback that will report the result of gatt operation
 *
 * @param
 *    - cb: callback
 *
 * @return
 * - void
 */
void bk_ble_register_app_sdp_common_callback(app_sdp_comm_callback cb);

/**
 * @brief As master, enable notification or indication
 *
 * @param
 *    - con_idx: the index of connection
 *    - ccc_handle: the handle of Client Characteristic Configuration descriptor
 *    - ccc_value: descriptor value, 0x01 means notification ,0x02 means indication
 *
 * @return
 *    - 0: succeed
 *    - others: errors.
 */
uint8_t bk_ble_gatt_write_ccc(uint8_t con_idx,uint16_t ccc_handle,uint16_t ccc_value);

/**
 * @brief As master, write attribute value
 *
 * @param
 *    - con_idx: the index of connection
 *    - att_handle: the handle of attribute value
 *    - data: value data
 *    - len: the length of attribute value
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_gatt_write_value(uint8_t con_idx, uint16_t att_handle, uint16_t len, uint8_t *data);

/**
 * @brief As slaver, send response value
 *
 * @param
 *    - con_idx: the idx of app connections
 *    - len: the length of attribute's value
 *    - buf: attribute's value
 *    - prf_id: The id of the profile
 *    - att_idx: The index of the attribute
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_read_response_value(uint8_t con_idx, uint32_t len, uint8_t *buf, uint16_t prf_id, uint16_t att_idx);

/**
 * @brief As slaver, send read response value with a return status
 *
 * Same as bk_ble_read_response_value(), but allows the application to report a
 * specific ATT error status to the peer instead of always returning success.
 *
 * @param
 *    - con_idx: the idx of app connections
 *    - len: the length of attribute's value
 *    - buf: attribute's value
 *    - prf_id: The id of the profile
 *    - att_idx: The index of the attribute
 *    - ret_status: ATT return status (0 means success, non-zero means the ATT error code reported to peer)
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_read_response_value_ext(uint8_t con_idx, uint32_t len, uint8_t *buf, uint16_t prf_id, uint16_t att_idx, uint8_t ret_status);

/**
 * @brief As slaver, send write response (confirm a Write Request) to the peer
 *
 * Sends the ATT Write Response for a Write Request that was reported to the
 * application via BLE_5_WRITE_EVENT, optionally carrying an ATT error code.
 *
 * @param
 *    - con_idx: the idx of app connections
 *    - prf_id: The id of the profile
 *    - att_idx: The index of the attribute
 *    - ret_status: ATT return status (0 means success, non-zero means the ATT error code reported to peer)
 *
 * @attention
 * - This API MUST ONLY be called when the auto write-response feature (_auto_rsp_write_req)
 *   is DISABLED. When _auto_rsp_write_req is enabled, the stack automatically confirms every
 *   Write Request with a success status, so calling this API would send a duplicate
 *   GATTC_WRITE_CFM and may cause a mismatched/incorrect write response. In that mode the
 *   application should only process the data reported by BLE_5_WRITE_EVENT and MUST NOT call
 *   this API.
 * - When _auto_rsp_write_req is disabled, call this API after receiving BLE_5_WRITE_EVENT and
 *   before the peer's request times out, to confirm the Write Request or return a specific
 *   ATT error to the peer (e.g. invalid length, out-of-range value, insufficient authorization).
 * - Only applies to a Write Request (ble_write_req_t.is_cmd == 0). Do NOT call it for a
 *   Write Command (is_cmd == 1), which expects no response.
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_write_response(uint8_t con_idx, uint16_t prf_id, uint16_t att_idx, uint8_t ret_status);

/**
 * @brief As slave, accept a pairing request from peer with the specified security features
 *
 * Used by the SLAVE (pairing responder) inside the BLE_5_PAIRING_REQ event handler to accept
 * the pairing initiated by the peer, replying the Pairing Response with the given features.
 * To reject instead, use bk_ble_reject_pairing(). Not used by the master; the master sets its
 * own features when it initiates pairing via bk_ble_create_bond()/bk_ble_create_bond_ext().
 *
 * @param
 *    - con_idx: the index of connection
 *    - mode: authentication features (see enum gap_auth)
 *    - iocap: IO Capability Values (see enum bk_ble_gap_io_cap)
 *    - sec_req: Security Defines (see enum gap_sec_req)
 *    - oob: OOB Data Present Flag Values (see enum gap_oob)
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_sec_send_auth_mode(uint8_t con_idx, uint8_t mode, uint8_t iocap, uint8_t sec_req, uint8_t oob);

/**
 * @brief As slave, accept a pairing request from peer with security features and key distribution
 *
 * Same as bk_ble_sec_send_auth_mode() but also specifies the initiator/responder key distribution.
 * Used by the SLAVE (pairing responder) inside the BLE_5_PAIRING_REQ event handler to accept the
 * pairing initiated by the peer. To reject instead, use bk_ble_reject_pairing().
 *
 * @param
 *    - con_idx: the index of connection
 *    - mode: authentication features (see enum gap_auth)
 *    - iocap: IO Capability Values (see enum bk_ble_gap_io_cap)
 *    - sec_req: Security Defines (see enum gap_sec_req)
 *    - oob: OOB Data Present Flag Values (see enum gap_oob)
 *    - initiator_key_distr: init key distr, see gap_key_distr
 *    - responder_key_distr: resp key distr, see gap_key_distr
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_sec_send_auth_mode_ext(uint8_t con_idx, uint8_t mode, uint8_t iocap, uint8_t sec_req, uint8_t oob, uint8_t initiator_key_distr, uint8_t responder_key_distr);

/**
 * @brief As slave, reject a pairing request from peer
 *
 * @param
 *    - con_idx: the index of connection
 *
 * @attention Used by the SLAVE (pairing responder) in the BLE_5_PAIRING_REQ event handler when the
 *            local side wants to reject the pairing request initiated by the peer. Only valid for
 *            the slave role; to accept instead, use bk_ble_sec_send_auth_mode()/_ext().
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_reject_pairing(uint8_t con_idx);

/**
 * @brief ble init function,this api is deprecated,please use bk_bluetooth_init.
 *
 * @param
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_init(void);

/**
 * @brief ble deinit function,this api is deprecated,please use bk_bluetooth_deinit.
 *
 * @param
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_deinit(void);

/**
 * @brief     Unregister a gatt service
 * @param
 *     - ble_db_cfg: service param
 *
 * @attention 1.you must set the uuid of service and the len of uuid.
 *
 * User example:
 * @code
 *     struct bk_ble_db_cfg ble_db_cfg;
 *     uint16 service_uuid = 0x1800;
 *
 *     os_memcpy(&(ble_db_cfg.uuid[0]), &service_uuid, 2);
 *     ble_db_cfg.svc_perm = BK_BLE_PERM_SET(SVC_UUID_LEN, UUID_16);
 * @endcode
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_delete_service(struct bk_ble_db_cfg* ble_db_cfg);

/**
 * @brief As master, read attribute value, the result is reported in the callback registered through bk_ble_register_app_sdp_charac_callback
 *
 * @param
 *    - con_idx: the index of connection
 *    - att_handle: the handle of attribute value
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_att_read(uint8_t con_idx, uint16_t att_handle);

/**
 * @brief Start the security procedure on the link (usable by both master and slave)
 *
 * - As MASTER: if the link is not bonded it starts pairing (sends a Pairing Request);
 *              if already bonded it starts encryption using the stored LTK.
 * - As SLAVE : it sends a Security Request asking the master to start pairing/encryption.
 *
 * @param
 *    - con_idx: the index of connection
 *    - auth: authentication features(see enum \ref gap_auth)
 *    - iocap: IO Capability Values(see enum \ref bk_ble_gap_io_cap)
 *    - sec_req: Security Defines(see enum \ref gap_sec_req)
 *    - oob: OOB Data Present Flag Values(see enum \ref gap_oob)
 *
 * @attention if the link has not been bonded, it will trigger pairing, otherwise it will trigger the link encryption.
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_create_bond(uint8_t con_idx, uint8_t auth, uint8_t iocap, uint8_t sec_req, uint8_t oob);

/**
 * @brief Start the security procedure on the link with key distribution (usable by both master and slave)
 *
 * Same as bk_ble_create_bond() but also specifies the initiator/responder key distribution.
 * - As MASTER: if not bonded it starts pairing; if already bonded it starts encryption.
 * - As SLAVE : it sends a Security Request asking the master to start pairing/encryption.
 *
 * @param
 *    - con_idx: the index of connection
 *    - auth: authentication features(see enum \ref gap_auth)
 *    - iocap: IO Capability Values(see enum \ref bk_ble_gap_io_cap)
 *    - sec_req: Security Defines(see enum \ref gap_sec_req)
 *    - oob: OOB Data Present Flag Values(see enum \ref gap_oob)
 *    - initiator_key_distr: init key distr, see gap_key_distr
 *    - responder_key_distr: resp key distr, see gap_key_distr
 *
 * @attention if the link has not been bonded, it will trigger pairing, otherwise it will trigger the link encryption.
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_create_bond_ext(uint8_t con_idx, uint8_t auth, uint8_t iocap, uint8_t sec_req, uint8_t oob, uint8_t initiator_key_distr, uint8_t responder_key_distr);

/**
 * @brief Reply the passkey during pairing (Passkey Entry model; usable by both master and slave)
 *
 * @attention Call this in response to the BLE_5_PARING_PASSKEY_INPUT_REQ event, when the local side
 *            needs to input the passkey displayed on the peer. Applies to whichever role received
 *            the request (master or slave).
 *
 * @param
 *    - con_idx: the index of connection
 *    - accept: accept(true) / reject(false) the pairing
 *    - passkey: the num that peer need to input or local need to input
 *
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_passkey_send(uint8_t con_idx, uint8_t accept, uint32_t passkey);

/**
 * @brief Reply the numeric comparison result during LE Secure Connections pairing
 *        (Numeric Comparison model; usable by both master and slave)
 *
 * @attention Call this in response to the BLE_5_PARING_NUMBER_COMPARE_REQ_EVENT event after the user
 *            confirms whether the two displayed 6-digit values match. Applies to whichever role
 *            received the request (master or slave).
 *
 * @param
 *    - con_idx: the index of connection
 *    - accept: accept(true) / reject(false) the pairing
 *
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_number_compare_send(uint8_t con_idx, uint8_t accept);

/**
 * @brief     Read the rssi of connection device. The result will report in callback registered through bk_ble_set_notice_cb with evt BLE_5_READ_RSSI_CMPL_EVENT, and param is ble_read_rssi_rsp_t *
 *
 * @param
 *    - conn_idx: the index of connection device
 * @attention 1.must used after after connected
 *
 * User example:
 * @code
 *     bk_ble_read_rssi(conn_idx);
 * @endcode
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_read_rssi(uint8_t conn_idx);

/**
 * @brief           set local gap appearance
 *
 *
 * @param[in]       appearance   - External appearance value, these values are defined by the Bluetooth SIG, please refer to
 *                  https://specificationrefs.bluetooth.com/assigned-values/Appearance%20Values.pdf
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_config_local_appearance(uint16_t appearance);


/**
 * @brief Discover peer primary service. The result is report in the callback registered through bk_ble_register_app_sdp_common_callback, and evt is MST_TYPE_DISCOVER_PRI_SERVICE_RSP.
 *        When completed, callback will report MST_TYPE_DISCOVER_COMPLETED with ble_descover_complete_inf and MST_TYPE_DISCOVER_PRI_SERVICE_RSP.
 *
 *
 * @param
 *    - sh: att start handle
 *    - eh: att end handle
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_discover_primary_service(uint8_t conn_id, uint16_t sh, uint16_t eh);

/**
 * @brief Discover peer primary service. The result is report in the callback registered through bk_ble_register_app_sdp_common_callback, and evt is MST_TYPE_DISCOVER_PRI_SERVICE_BY_UUID_RSP.
 *        When completed, callback will report MST_TYPE_DISCOVER_COMPLETED with ble_descover_complete_inf and MST_TYPE_DISCOVER_PRI_SERVICE_BY_UUID_RSP.
 *
 *
 * @param
 *    - sh: att start handle
 *    - eh: att end handle
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 */
ble_err_t bk_ble_discover_primary_service_by_uuid(uint8_t conn_id, uint16_t sh, uint16_t eh, uint16_t uuid);

/**
 * @brief Discover peer primary service. The result is report in the callback registered through bk_ble_register_app_sdp_common_callback, and evt is MST_TYPE_DISCOVER_PRI_SERVICE_BY_128_UUID_RSP.
 *        When completed, callback will report MST_TYPE_DISCOVER_COMPLETED with ble_descover_complete_inf and MST_TYPE_DISCOVER_PRI_SERVICE_BY_128_UUID_RSP.
 *     bk_ble_read_rssi(conn_idx);
 * @param
 *    - conn_id: the index of connection
 *    - sh: att start handle
 *    - eh: att end handle
 *    - uuid: uuid that service need to find in 128bits
 *
 * @return
 * - others: fail
 */
ble_err_t bk_ble_discover_primary_service_by_128uuid(uint8_t conn_id, uint16_t sh, uint16_t eh, uint8_t *uuid);

/**
 * @brief Discover peer characteristic. The result is report in the callback registered through bk_ble_register_app_sdp_common_callback, and evt is MST_TYPE_DISCOVER_CHAR_RSP.
 *        When completed, callback will report MST_TYPE_DISCOVER_COMPLETED with ble_descover_complete_inf and MST_TYPE_DISCOVER_CHAR_RSP.
 *
 *
 * @param
 *    - conn_id: the index of connection
 *    - sh: att start handle
 *    - eh: att end handle
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_discover_characteristic(uint8_t conn_id, uint16_t sh, uint16_t eh);

/**
 * @brief Discover peer characteristic. The result is report in the callback registered through bk_ble_register_app_sdp_common_callback, and evt is MST_TYPE_DISCOVER_CHAR_BY_UUID_RSP.
 *        When completed, callback will report MST_TYPE_DISCOVER_COMPLETED with ble_descover_complete_inf and MST_TYPE_DISCOVER_CHAR_BY_UUID_RSP.
 *
 *
 * @param
 *    - conn_id: the index of connection
 *    - sh: att start handle
 *    - eh: att end handle
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_discover_characteristic_by_uuid(uint8_t conn_id, uint16_t sh, uint16_t eh, uint16_t uuid);

/**
 * @brief Discover peer characteristic. The result is report in the callback registered through bk_ble_register_app_sdp_common_callback, and evt is MST_TYPE_DISCOVER_CHAR_BY_128_UUID_RSP.
 *        When completed, callback will report MST_TYPE_DISCOVER_COMPLETED with ble_descover_complete_inf and MST_TYPE_DISCOVER_CHAR_BY_128_UUID_RSP.
 *
 * @param
 *    - conn_id: the index of connection
 *    - sh: att start handle
 *    - eh: att end handle
 *
 * @return
 * - others: fail
 */
ble_err_t bk_ble_discover_characteristic_by_128uuid(uint8_t conn_id, uint16_t sh, uint16_t eh, uint8_t *uuid);

/**
 * @brief Discover peer characteristic descriptor. The result is report in the callback registered through bk_ble_register_app_sdp_common_callback, and evt is MST_TYPE_DISCOVER_CHAR_DESC.
 *        When completed, callback will report MST_TYPE_DISCOVER_COMPLETED with ble_descover_complete_inf and MST_TYPE_DISCOVER_CHAR_DESC.
 *
 *
 * @param
 *    - conn_id: the index of connection
 *    - sh: att start handle
 *    - eh: att end handle
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_discover_characteristic_descriptor(uint8_t conn_id, uint16_t sh, uint16_t eh);

/**
 * @brief As master, read attribute value, the result is reported in the callback registered through bk_ble_register_app_sdp_charac_callback
 *
 * @param
 *    - con_idx: the index of connection
 *    - att_handle: the handle of attribute value
 *    - offset: the offset of attribute value
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_gattc_read(uint8_t con_idx, uint16_t att_handle, uint16_t offset);

/**
 * @brief As master, read attribute value, the result is reported in the callback registered through bk_ble_register_app_sdp_charac_callback
 *
 * @param
 *    - con_idx: the index of connection
 *    - sh: the start handle of attribute
 *    - eh: the end handle of attribute
 *    - uuid: uuid
 *    - uuid_len: uuid len, may be 2 or 16
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_gattc_read_by_uuid(uint8_t conn_id, uint16_t sh, uint16_t eh, uint8_t *uuid, uint8_t uuid_len);

/**
 * @brief As master, write attribute value
 *
 * @param
 *    - con_idx: the index of connection
 *    - att_handle: the handle of attribute value
 *    - data: value data
 *    - len: the length of attribute value
 *    - is_write_cmd: when true, will trigger write cmd, otherwise write req.
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_gattc_write(uint8_t con_idx, uint16_t att_handle, uint8_t *data, uint16_t len, uint8_t is_write_cmd);

/**
 * @brief clear the white list.
 *
 * @param
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_clear_white_list(void);

/**
 * @brief Add a device to the while list.
 *
 *
 * @param
 *    - addr: the address of the device.
 *    - addr_type: the type of the address, 0 is public address, 1 is random address.
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_add_devices_to_while_list(bd_addr_t *addr, uint8_t addr_type);

/**
 * @brief Remove a device from the while list.
 *
 *
 * @param
 *    - addr: the address of the device.
 *    - addr_type: the type of the address, 0 is public address, 1 is random address.
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_remove_devices_from_while_list(bd_addr_t *addr, uint8_t addr_type);

/**
 * @brief Set tx power.
 *
 *
 * @param
 *    - pwr_gain: tx power gain
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_tx_power_set(float pwr_gain);

/**
 * @brief Provide the Legacy-OOB Temporary Key for SMP, in response to BLE_5_OOB_REQ_EVENT
 *        (LE legacy pairing OOB; usable by both master and slave)
 *
 * @attention Applies to whichever role received the request. When accept is true, tk must be a
 *            128-bit (GAP_KEY_LEN) random number; accept=false rejects the pairing.
 *
 * @param
 *    - con_idx: the index of connection
 *    - accept: Accept or Reject the OOB
 *    - tk: Temporary Key value, the TK value shall be a 128-bit random number
 *    - len: length of temporary key, should always be 128-bit
 *
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_oob_req_reply(uint8_t con_idx, uint8_t accept, uint8_t *tk, uint8_t len);

/**
* @brief Provide the LE Secure Connections OOB data (conf/rand) for SMP, in response to
*        BLE_5_SC_OOB_REQ_EVENT (LE Secure Connections OOB; usable by both master and slave)
*
* @attention Applies to whichever role received the request. The local OOB conf/rand to be
*            transferred to the peer are delivered earlier via the BLE_5_SC_LOC_OOB_IND event.
*            accept=false rejects the pairing.
*
* @param
*    - con_idx: the index of connection
*    - accept: Accept or Reject the OOB
*    - conf: Confirmation value, it shall be a 128-bit random number
*    - rand: Randomizer value, it should be a 128-bit random number
*
*
* @return
* - BK_ERR_BLE_SUCCESS: succeed
* - others: fail
*/
ble_err_t bk_ble_sc_oob_req_reply(uint8_t con_idx, uint8_t accept, uint8_t conf[16], uint8_t rand[16]);

/**
 * @brief Register a Protocol/Service Multiplexer (PSM) for BLE COC
 *
 * Registers a PSM value to enable BLE Connection Oriented Channel (COC) services.
 * PSM is used to identify different services or protocols over a single BLE connection.
 *
 * @param
 *    - psm: Protocol/Service Multiplexer value to register
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_coc_reg(uint16_t psm);

/**
 * @brief Unregister a Protocol/Service Multiplexer (PSM) for BLE COC
 *
 * Unregisters a previously registered PSM value, disabling the associated
 * COC service. All active connections on this PSM should be closed before unregistering.
 *
 * @param
 *    - psm: Protocol/Service Multiplexer value to unregister
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_coc_unreg(uint16_t psm);

/**
 * @brief config the security level of a BLE COC channel
 *
 * Configures the security level required for a previously registered PSM.
 * Incoming/outgoing connections on this PSM must satisfy the configured
 * security level before the channel can be established.
 *
 * @param
 *    - psm: Protocol/Service Multiplexer value to configure
 *    - sec_lvl: required security level, of type enum coc_security:
 *               - BK_BLE_COC_SEC_NONE: no security
 *               - BK_BLE_COC_SEC_UNAUTH_ENCRYPT: unauthenticated encryption
 *               - BK_BLE_COC_SEC_AUTH_ENCRYPT: authenticated encryption
 *               - BK_BLE_COC_SEC_SECURE_CONNECTION: LE Secure Connections
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_coc_config(uint16_t psm, uint8_t sec_lvl);

/**
 * @brief Get current information of a BLE COC channel
 *
 * Retrieves the current information of a Connection Oriented Channel (COC)
 * based on the specified information type. This function can be used to query
 * various channel parameters such as current credits, maximum credits, MTU, MPS, etc.
 *
 * @param
 *    - conn_idx: the index of BLE connection to the remote device
 *    - cid: Channel ID of the COC connection to query
 *    - type: Information type to retrieve, of type enum coc_info. Possible values:
 *            - BK_BLE_COC_PEER_CURRENT_CREDIT: Peer's current available credits
 *            - BK_BLE_COC_PEER_MAX_CREDIT: Peer's maximum credits
 *            - BK_BLE_COC_PEER_MTU: Peer's Maximum Transmission Unit size
 *            - BK_BLE_COC_PEER_MPS: Peer's Maximum PDU Payload Size
 *            - BK_BLE_COC_LOCAL_CURRENT_CREDIT: Local current available credits
 *            - BK_BLE_COC_LOCAL_MAX_CREDIT: Local maximum credits
 *            - BK_BLE_COC_LOCAL_MTU: Local Maximum Transmission Unit size
 *            - BK_BLE_COC_LOCAL_MPS: Local Maximum PDU Payload Size
 *    - output: Pointer to a uint32_t variable to store the retrieved information.
 *              The value will be written to this location upon successful retrieval.
 *              Must not be NULL.
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed, information retrieved and stored in output
 * - BK_ERR_BLE_FAIL: fail, invalid parameters or channel not found
 * - BK_ERR_BLE_UNKNOW_IDX: invalid connection index or channel ID
 * - others: fail
 *
 * @note
 * - The function must be called after the COC channel is established
 * - The output parameter must point to a valid memory location
 * - The meaning of the output value depends on the type parameter
 */
ble_err_t bk_ble_coc_get_current_info(uint8_t conn_idx, uint16_t cid, uint8_t type, uint32_t *output);

/**
 * @brief Request to establish a BLE COC connection
 *
 * Initiates a connection request to establish a Connection Oriented Channel
 * with a remote device on the specified connection using the given PSM.
 *
 * @param
 *    - conn_idx: the index of BLE connection to the remote device
 *    - psm: Protocol/Service Multiplexer value to connect to
 *    - mtu: max transmision unit size, use default value if 0
 *    - mps: max pdu payload size, use default value if 0
 *    - credit: init credit, use default value if 0
 *
 * @attention the connection result is reported in ble_notice_cb_t as BLE_5_COC_CONNECTION_COMPL_EVENT
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_coc_connection_req(uint8_t conn_idx, uint16_t psm, uint16_t mtu, uint16_t mps, uint16_t credit);

/**
 * @brief Request to disconnect a BLE COC channel
 *
 * Initiates a disconnection request to close an established Connection
 * Oriented Channel identified by the channel ID.
 *
 * @param
 *    - conn_idx: the index of BLE connection
 *    - cid: Channel ID of the COC connection to disconnect
 *
 * @attention the disconnection result is reported in ble_notice_cb_t as BLE_5_COC_DISCCONNECT_COMPL_EVENT
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_coc_disconnection_req(uint8_t conn_idx, uint16_t cid);

/**
 * @brief Accept or reject a BLE COC connection request from a peer device
 *
 * Responds to an incoming Connection Oriented Channel (COC) connection request
 * from a remote device. This function is typically called by the server side
 * after receiving a connection request event. It allows the application to
 * accept or reject the connection and specify the channel parameters.
 *
 * @param
 *    - conn_idx: the index of BLE connection to the remote device
 *    - accept: Flag to accept or reject the connection request.
 *              - 0: Reject the connection request
 *              - 1 (non-zero): Accept the connection request
 *    - peer_cid: Peer's Channel ID from the connection request event
 *    - mtu: Maximum Transmission Unit size for the channel.
 *           Use default value if 0. This value will be negotiated with the peer.
 *    - mps: Maximum PDU Payload Size for the channel.
 *           Use default value if 0. This value will be negotiated with the peer.
 *    - credit: Initial credit value for the channel.
 *              Use default value if 0. Credits are used for flow control.
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed, connection request accepted or rejected
 * - BK_ERR_BLE_FAIL: fail, invalid parameters or connection request not found
 * - BK_ERR_BLE_UNKNOW_IDX: invalid connection index or peer channel ID
 * - others: fail
 *
 * @note
 * - This function should be called in response to a COC connection request event (BLE_5_COC_CONNECT_REQ_EVENT); the result is reported as BLE_5_COC_CONNECTION_COMPL_EVENT
 * - If accept is 0, the connection will be rejected and mtu/mps/credit parameters are ignored
 * - If accept is non-zero, the connection will be accepted with the specified parameters
 * - The actual MTU and MPS used will be the minimum of local and peer values after negotiation
 * - The function must be called before the connection request timeout expires
 */
ble_err_t bk_ble_coc_accept_connect_req(uint8_t conn_idx, uint8_t accept, uint16_t peer_cid, uint16_t mtu, uint16_t mps, uint16_t credit);

/**
 * @brief Send data over a BLE COC channel
 *
 * Transmits data through an established Connection Oriented Channel.
 * The data will be sent to the remote device on the specified channel.
 *
 * @param
 *    - conn_idx: the index of BLE connection
 *    - cid: Channel ID of the COC connection
 *    - data: Pointer to the data buffer to send
 *    - len: Length of data to send in bytes
 *
 * @attention the transmission completion is reported in ble_notice_cb_t as BLE_5_COC_TX_DONE
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
 * - others: fail
 */
ble_err_t bk_ble_coc_send_req(uint8_t conn_idx, uint16_t cid, uint8_t *data, uint32_t len);

/**
 * @brief  register hci callback for host only
 *
 * @param
 *    - cb: hci callback function used to recv hci data from host
 *
 * @attention used for host only
 *
 * @return
 *    - BK_ERR_BLE_SUCCESS: succeed
 *    - others: other errors.
 */
ble_err_t bk_ble_host_register_hci_callback(ble_hci_to_cp_cb cb);

/**
 * @brief send hci data to host.
 *
 * @param
 * - buf: payload
 * - len: buf's len
 *
 * @attention used for host only
 *
 * @return
 * - BK_ERR_BLE_SUCCESS: succeed
**/
bk_err_t bk_ble_hci_send_to_host(uint8_t *buf, uint32_t len);

/*
 * @}
 */
#ifdef __cplusplus
}
#endif

//#endif

#endif
