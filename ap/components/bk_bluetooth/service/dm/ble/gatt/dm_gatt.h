#pragma once

/**
 * @file dm_gatt.h
 *
 * @brief BLE GATT framework common APIs.
 *
 * This header provides shared GAP, security, bond management, and connection
 * helper APIs used by the dm_gatts and dm_gattc service wrappers.
 */

#include "dm_gatt_connection.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"
#include <stdint.h>

/** Enable BLE bond/local-key storage in the GATT framework. */
#if CONFIG_BLUETOOTH_BTDM_COMPONENT_BLE_USE_STORAGE
#define BLE_USE_STORAGE 1
#endif

/** Enable the built-in GATTS test attribute table. */
#if CONFIG_BLUETOOTH_BTDM_COMPONENT_BLE_GATTS_TEST_ATTR
#define GATTS_TEST_ATTR_ENABLE 1
#endif

/** Select the legacy GAP API path. */
#define GAP_IS_OLD_API 0

/** Maximum number of BLE bonded devices tracked by the GATT framework. */
#define GATT_MAX_BOND_COUNT 7

/** GATT framework log levels. */
enum
{
    GATT_DEBUG_LEVEL_ERROR,   /**< Error log level. */
    GATT_DEBUG_LEVEL_WARNING, /**< Warning log level. */
    GATT_DEBUG_LEVEL_INFO,    /**< Informational log level. */
    GATT_DEBUG_LEVEL_DEBUG,   /**< Debug log level. */
    GATT_DEBUG_LEVEL_VERBOSE, /**< Verbose log level. */
};

/** Timeout in milliseconds for synchronous GATT helper commands. */
#define SYNC_CMD_TIMEOUT_MS 4000

#ifndef GATT_DEBUG_LEVEL
#ifdef CONFIG_BLUETOOTH_BTDM_COMPONENT_BLE_GATT_LOG_LEVEL
#define GATT_DEBUG_LEVEL CONFIG_BLUETOOTH_BTDM_COMPONENT_BLE_GATT_LOG_LEVEL
#else
#define GATT_DEBUG_LEVEL GATT_DEBUG_LEVEL_INFO
#endif
#endif

#define gatt_loge(format, ...) do{if(GATT_DEBUG_LEVEL >= GATT_DEBUG_LEVEL_ERROR)   BK_LOGE("dm_gatt", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define gatt_logw(format, ...) do{if(GATT_DEBUG_LEVEL >= GATT_DEBUG_LEVEL_WARNING) BK_LOGW("dm_gatt", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define gatt_logi(format, ...) do{if(GATT_DEBUG_LEVEL >= GATT_DEBUG_LEVEL_INFO)    BK_LOGI("dm_gatt", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define gatt_logd(format, ...) do{if(GATT_DEBUG_LEVEL >= GATT_DEBUG_LEVEL_DEBUG)   BK_LOGD("dm_gatt", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define gatt_logv(format, ...) do{if(GATT_DEBUG_LEVEL >= GATT_DEBUG_LEVEL_VERBOSE) BK_LOGV("dm_gatt", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)


/** Return values used by registered BLE GAP application callbacks. */
enum
{
    DM_BLE_GAP_APP_CB_RET_PROCESSED,      /**< Event was processed by the callback. */
    DM_BLE_GAP_APP_CB_RET_NO_INTERESTING, /**< Event was not handled by the callback. */
};


/** Demo service UUID used by the GATT sample flow. */
#define INTERESTING_SERIVCE_UUID 0x1234

/** Demo characteristic UUID used by the GATT sample flow. */
#define INTERESTING_CHAR_UUID 0x5678

/** Helper macro used to declare CLI GATT parameter fields. */
#define GATT_PARAM_MEMBER(type) \
                type rpa;           \
                type *p_rpa;        \
                type privacy;       \
                type *p_privacy;    \
                type iocap;         \
                type *p_iocap;      \
                type auth;          \
                type *p_auth;       \
                type ikd;           \
                type *p_ikd;        \
                type rkd;           \
                type *p_rkd;        \
                type pa;            \
                type *p_pa;         \
                type lrkd;          \
                type *p_lrkd;

/** GATT framework initialization parameters. */
typedef struct
{
    GATT_PARAM_MEMBER(uint8_t)
}__attribute__((packed)) cli_gatt_param_t;


/**
 * @brief Initialize the BLE GATT framework.
 *
 * @param param Optional initialization parameters. Pass NULL to use defaults.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_gatt_main(cli_gatt_param_t *param);

/**
 * @brief Deinitialize the BLE GATT framework.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_gatt_deinit(void);

/**
 * @brief Disable all GATT framework roles and activities.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatt_disable_all(void);

/**
 * @brief Register a BLE GAP callback.
 *
 * @param cb Callback pointer. The callback type is defined by the BLE GAP SDK.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_gatt_add_gap_callback(void * cb);

/**
 * @brief Get authentication address mapping information.
 *
 * @param nominal_addr Nominal address input buffer.
 * @param nominal_addr_type Nominal address type output buffer.
 * @param identity_addr Identity address output buffer.
 * @param identity_addr_type Identity address type output buffer.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatt_get_authen_status(uint8_t *nominal_addr, uint8_t *nominal_addr_type, uint8_t *identity_addr, uint8_t *identity_addr_type);

/**
 * @brief Find identity address information by nominal address information.
 *
 * @param nominal_addr Nominal address input buffer.
 * @param nominal_addr_type Nominal address type.
 * @param identity_addr Identity address output buffer.
 * @param identity_addr_type Identity address type output buffer.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatt_find_id_info_by_nominal_info(uint8_t *nominal_addr, uint8_t nominal_addr_type, uint8_t *identity_addr, uint8_t *identity_addr_type);

/**
 * @brief Reply to a BLE passkey request.
 *
 * @param accept Non-zero to accept the passkey, zero to reject.
 * @param passkey Passkey value.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_gatt_passkey_reply(uint8_t accept, uint32_t passkey);

/**
 * @brief Configure BLE security method.
 *
 * @param iocap IO capability.
 * @param auth_req Authentication request flags.
 * @param key_distr Key distribution flags.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_gatt_set_security_method(uint8_t iocap, uint8_t auth_req, uint8_t key_distr);

/**
 * @brief Check whether link keys are distributed from LTK information.
 *
 * @return true if link keys are distributed from LTK, otherwise false.
 */
bool dm_gatt_is_linkkey_distr_from_ltk(void);

/**
 * @brief Create a BLE bond with a peer device.
 *
 * @param addr Peer BLE address.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_ble_gap_create_bond(uint8_t *addr);

/**
 * @brief Remove BLE bond information for a peer device.
 *
 * @param addr Peer BLE address.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_ble_gap_remove_bond(uint8_t *addr);

/**
 * @brief Get the number of bonded BLE devices.
 *
 * @return Number of bonded devices.
 */
uint32_t dm_ble_gap_get_bonded_count(void);

/**
 * @brief Clear all BLE bond information.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_ble_gap_clean_bond(void);

/**
 * @brief Print the BLE bond list for debugging.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_ble_gap_show_bond_list(void);

/**
 * @brief Get BLE bond information by address.
 *
 * @param addr Peer BLE address.
 *
 * @return Pointer to bond information on success, otherwise NULL.
 */
bk_ble_bond_dev_t* dm_ble_gap_get_bond_info_by_addr(uint8_t *addr);

/**
 * @brief Iterate over BLE bond information.
 *
 * @param func Callback invoked for each bond record.
 * @param arg User argument passed to the callback.
 *
 * @return Number of visited bond records.
 */
uint8_t dm_ble_gap_bond_info_foreach(int32_t (*func) (bk_ble_bond_dev_t *info, void *arg), void *arg);

/**
 * @brief Clear local BLE security keys.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_ble_gap_clean_local_key(void);

/**
 * @brief Update BLE connection parameters.
 *
 * @param addr Peer BLE address.
 * @param interval Connection interval.
 * @param tout Supervision timeout.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_ble_gap_update_param(uint8_t *addr, uint16_t interval, uint16_t tout);

/**
 * @brief Get the current resolvable private address.
 *
 * @param rpa Output buffer for the address. The buffer length must be 6 bytes.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_ble_gap_get_rpa(uint8_t *rpa);

/**
 * @brief Get the local identity address.
 *
 * @param addr Output buffer for the address. The buffer length must be 6 bytes.
 */
void dm_ble_gap_get_identity_addr(uint8_t *addr);

/**
 * @brief Get the current BLE GATT connection ID.
 *
 * @return Current connection ID, or negative value when no connection is active.
 */
int16_t dm_ble_gap_get_current_conn_id(void);

/**
 * @brief Configure whether pairing requests are accepted automatically.
 *
 * @param accpet Non-zero to accept automatically, zero to require application handling.
 *
 * @return 0 on success, otherwise error code.
 */
int dm_ble_gap_set_auto_accept_pair_req(uint8_t accpet);

/**
 * @brief Disconnect a BLE GATT link by address.
 *
 * @param addr Peer BLE address.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatt_disconnect(uint8_t *addr);

/**
 * @brief Cancel an ongoing BLE connection attempt.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_gatt_connect_cancel(void);

/** Non-zero when the GATT framework uses RPA. */
extern uint8_t g_dm_gap_use_rpa;
