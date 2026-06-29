#pragma once

/**
 * @file dm_gatt_connection.h
 *
 * @brief BLE GATT connection context management APIs.
 */

#include "os/os.h"

/** Maximum number of GATT connections tracked by the framework. */
#define GATT_MAX_CONNECTION_COUNT 7

/** Maximum number of profile-specific data blocks per connection. */
#define GATT_MAX_PROFILE_COUNT 6

/** BLE GAP connection states used by dm_gatt_app_env_t::status. */
enum
{
    GAP_CONNECT_STATUS_IDLE,          /**< Connection is idle. */
    GAP_CONNECT_STATUS_CONNECTING,    /**< Connection is being established. */
    GAP_CONNECT_STATUS_CONNECTED,     /**< Connection is established. */
    GAP_CONNECT_STATUS_DISCONNECTING, /**< Connection is being disconnected. */
};

/** GATTC MTU request states. */
enum
{
    GATTC_MTU_REQ_STATUS_IDLE,      /**< No MTU request is running. */
    GATTC_MTU_REQ_STATUS_ING,       /**< MTU request is in progress. */
    GATTC_MTU_REQ_STATUS_COMPLETED, /**< MTU request is complete. */
};

/** GATTC service discovery states. */
enum
{
    GATTC_DISCOVER_STATUS_IDLE,      /**< No discovery is running. */
    GATTC_DISCOVER_STATUS_ING,       /**< Discovery is in progress. */
    GATTC_DISCOVER_STATUS_COMPLETED, /**< Discovery is complete. */
};

/** BLE GATT per-connection context. */
typedef struct
{
    bk_bd_addr_t addr;          /**< Peer BLE address. */
    bk_ble_addr_type_t addr_type; /**< Peer BLE address type. */
    uint16_t conn_id;           /**< GATT connection ID. */
    uint8_t status;             /**< Connection state, see GAP_CONNECT_STATUS_*. */
    uint8_t local_is_master;    /**< Non-zero when the local device is master/central. */
    uint8_t is_authen;          /**< Non-zero when the link is authenticated. */
    beken_semaphore_t server_sem; /**< Server operation semaphore. */
    beken_semaphore_t client_sem; /**< Client operation semaphore. */

    uint32_t data_len; /**< Length of connection-level additional data. */
    uint8_t *data;     /**< Connection-level additional data. */

    /** Profile-specific data entries attached to this connection. */
    struct
    {
        uint32_t id;       /**< Profile ID. */
        uint32_t data_len; /**< Profile data length in bytes. */
        uint8_t *data;     /**< Profile data buffer. */
    }profile_array[GATT_MAX_PROFILE_COUNT];

} dm_gatt_app_env_t;

/** Demo GATT application context shared by GATTS and GATTC demo flows. */
typedef struct
{
    uint8_t notify_status; /**< Server notify status: 0 disabled, 1 notify, 2 indicate. */
    uint16_t server_mtu; /**< Server MTU. */
    uint16_t send_notify_status; /**< Notify/indicate send status. */
    uint16_t send_read_rsp_status; /**< Read response send status. */

    uint8_t job_status; /**< Client job status. */
    uint8_t mtu_req_status; /**< MTU request status, see GATTC_MTU_REQ_STATUS_*. */
    uint8_t discover_status; /**< Discovery status, see GATTC_DISCOVER_STATUS_*. */
    uint16_t client_mtu; /**< Client MTU. */
    uint8_t noti_indica_switch; /**< Notification/indication subscription state. */
    uint8_t noti_indicate_recv_count; /**< Number of notifications/indications received. */
    uint16_t write_read_status; /**< Write/read operation status. */

    uint8_t *read_buff; /**< Read result buffer. */
    uint32_t read_buff_len; /**< Read result buffer length. */
    uint32_t read_offset; /**< Read offset. */

    uint16_t peer_interest_service_start_handle; /**< Peer interested service start handle. */
    uint16_t peer_interest_service_end_handle; /**< Peer interested service end handle. */
    uint16_t peer_interest_char_handle; /**< Peer interested characteristic handle. */
    uint16_t peer_interest_char_desc_handle; /**< Peer interested descriptor handle. */

    uint16_t peer_gap_service_start_handle; /**< Peer GAP service start handle. */
    uint16_t peer_gap_service_end_handle; /**< Peer GAP service end handle. */
} dm_gatt_demo_app_env_t;

/**
 * @brief Initialize the BLE GATT connection context pool.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_ble_app_env_init(void);

/**
 * @brief Deinitialize the BLE GATT connection context pool.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t dm_ble_app_env_deinit(void);

/**
 * @brief Allocate a connection context by peer address.
 *
 * @param addr Peer BLE address.
 * @param data_len Length of additional data to allocate for this connection.
 *
 * @return Pointer to the context on success, otherwise NULL.
 */
dm_gatt_app_env_t *dm_ble_alloc_app_env_by_addr(uint8_t *addr, uint32_t data_len);

/**
 * @brief Find a connection context by peer address.
 *
 * @param addr Peer BLE address.
 *
 * @return Pointer to the context on success, otherwise NULL.
 */
dm_gatt_app_env_t *dm_ble_find_app_env_by_addr(uint8_t *addr);

/**
 * @brief Find a connection context by connection ID.
 *
 * @param conn_id GATT connection ID.
 *
 * @return Pointer to the context on success, otherwise NULL.
 */
dm_gatt_app_env_t *dm_ble_find_app_env_by_conn_id(uint16_t conn_id);

/**
 * @brief Delete a connection context by peer address.
 *
 * @param addr Peer BLE address.
 *
 * @return Non-zero if a context was deleted, otherwise zero.
 */
uint8_t dm_ble_del_app_env_by_addr(uint8_t *addr);

/**
 * @brief Free all connection contexts.
 *
 * @return Non-zero on success, otherwise zero.
 */
uint8_t dm_ble_free_all_app_env(void);

/**
 * @brief Allocate or resize additional data for a connection.
 *
 * @param addr Peer BLE address.
 * @param data_len Additional data length in bytes.
 *
 * @return Pointer to the context on success, otherwise NULL.
 */
dm_gatt_app_env_t *dm_ble_alloc_addition_data_by_addr(uint8_t *addr, uint32_t data_len);

/**
 * @brief Allocate profile-specific data for a connection.
 *
 * @param profile_id Profile ID.
 * @param addr Peer BLE address.
 * @param data_len Profile data length in bytes.
 * @param output_param Output pointer to the allocated profile data buffer.
 *
 * @return Pointer to the context on success, otherwise NULL.
 */
dm_gatt_app_env_t *dm_ble_alloc_profile_data_by_addr(uint32_t profile_id, uint8_t *addr, uint32_t data_len, uint8_t **output_param);

/**
 * @brief Find profile-specific data by profile ID.
 *
 * @param env Connection context.
 * @param profile_id Profile ID.
 *
 * @return Pointer to profile data on success, otherwise NULL.
 */
uint8_t *dm_ble_find_profile_data_by_profile_id(dm_gatt_app_env_t *env, uint32_t profile_id);

/**
 * @brief Iterate over active connection contexts.
 *
 * @param func Callback invoked for each context.
 * @param arg User argument passed to the callback.
 *
 * @return Number of visited contexts.
 */
uint8_t dm_ble_app_env_foreach( int32_t (*func) (dm_gatt_app_env_t *env, void *arg), void *arg );
