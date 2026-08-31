#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_bt.h"

/**
 * @brief Enumeration of Bluetooth manager operation modes.
 */
enum
{
    BT_MNG_MODE_PAIRING = 1,       /**< connectable and discoverable*/
    BT_MNG_MODE_RECONNECTING,      /**< no-connectable and no-discoverable*/
    BT_MNG_MODE_CONNECTEED,        /**< no-connectable and no-discoverable*/
    BT_MNG_MODE_CONNECTABLE,       /**< connectable and no-discoverable */
    BT_MNG_MODE_IDLE,              /**< no-connectable and no-discoverable*/
    BT_MNG_MODE_DISCOVERABLE_ONLY, /**< no-connectable and discoverable*/
    BT_MNG_MODE_ALL_OFF,           /**< no-connectable and no-discoverable*/
};

/**
 * @brief Enumeration of Bluetooth connection states.
 */
enum
{
    BT_STATE_IDLE = 0,             /**< Bluetooth is in idle state. */
    BT_STATE_WAIT_FOR_RECONNECT,   /**< Waiting to initiate a reconnection. */
    BT_STATE_RECONNECTING,         /**< Currently in the process of reconnecting. */
    BT_STATE_LINK_CONNECTED,       /**< The link layer is connected. */
    BT_STATE_PROFILE_CONNECTED,    /**< The profile is connected. */
    BT_STATE_KEY_MISSING,          /**< The encryption key is missing. */
};

enum
{
    PAIRING_STATE_IDLE = 0,
    PAIRING_STATE_PREPARATION = 1,
    PAIRING_STATE_WAIT_CFM = 2,
};

enum
{
    BT_MNG_AUTO_ACCEPT_CONNECTION_ACL,
    BT_MNG_AUTO_ACCEPT_CONNECTION_SCO,
};



/**
 * @brief Callback function type for GAP (Generic Access Profile) events.
 * @param event The GAP event type.
 * @param param Pointer to the GAP callback parameter structure.
 */
typedef void (*btm_gap_event_cb)(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param);

/**
 * @brief Callback function type for starting profile connection.
 * @param remote_addr Pointer to the remote device address.
 */
typedef void (*btm_start_profile_connect_cb)(uint8_t *remote_addr);

/**
 * @brief Callback function type for starting profile disconnection.
 * @param remote_addr Pointer to the remote device address.
 */
typedef void (*btm_start_profile_disconnect_cb)(uint8_t *remote_addr);

/**
 * @brief Callback function type for stopping profile connection.
 */
typedef void (*btm_stop_profile_connect_cb)(void);

/**
 * @brief Callback function type for reconnect failure.
 */
typedef void (*btm_reconnect_fail_cb)(void);

/**
 * @brief Structure for Bluetooth manager callbacks.
 */
typedef struct
{
    btm_gap_event_cb gap_cb;                   /**< GAP event callback function. */
    btm_start_profile_connect_cb start_connect_cb; /**< Callback for starting profile connection. */
    btm_stop_profile_connect_cb stop_connect_cb;   /**< Callback for stopping profile connection. */
    btm_start_profile_disconnect_cb start_disconnect_cb; /**< Callback for starting profile disconnection. */
    btm_reconnect_fail_cb reconnect_fail_cb;   /**< Callback for reconnect failure. */
} btm_callback_s;

/**
 * @brief Bluetooth manager initialization configuration.
 */
typedef struct
{
    const char *local_name;            /**< Complete local Bluetooth name. */
    uint32_t device_class;             /**< Bluetooth class of device. If 0, the component default is used. */
    uint16_t page_scan_interval;       /**< Page scan interval. If 0, the component default is used. */
    uint16_t page_scan_window;         /**< Page scan window. If 0, the component default is used. */
    uint16_t page_timeout;             /**< Page timeout in 0.625 ms units. If 0, the component default is used. */
    uint32_t reconnect_interval_ms;    /**< Reconnect interval in ms. If 0, the component default is used. */
    uint8_t max_reconnect_count;       /**< Max reconnect attempts. If 0, the component default is used. */
    uint8_t io_capability;             /**< GAP IO capability. If 0xff, the component default is used. */
    uint8_t authreq_mode;              /**< SSP authentication requirements (0x01~0x05). If 0, the component default is used. */
    uint8_t role;                      /**< Link role applied after ACL is up: 0 = keep stack role (slave for headset/sink, default), 1 = master (A2DP source), 2 = slave. */
} bt_manager_cfg_t;

/**
 * @brief Register callback functions for the Bluetooth manager.
 * @param cb Pointer to the callback structure.
 * @return 0 on success, otherwise error code.
 */
int bt_manager_register_callback(btm_callback_s *cb);

/**
 * @brief Start the reconnection process to a specified device.
 * @param addr Pointer to the address of the device to reconnect to.
 * @param immediate Whether to initiate the reconnection immediately.
 */
void bt_manager_start_reconnect(uint8_t *addr, uint8_t immediate);

/**
 * @brief Set the operation mode of the Bluetooth manager.
 * @param mode The mode to set.
 */
void bt_manager_set_mode(uint8_t mode);

/**
 * @brief Get the current operation mode of the Bluetooth manager.
 * @return The current operation mode.
 */
uint8_t bt_manager_get_mode(void);

/**
 * @brief Initialize the Bluetooth manager.
 * @return 0 on success, otherwise error code.
 */
int bt_manager_init(const bt_manager_cfg_t *cfg);

/**
 * @brief Deinitialize the Bluetooth manager.
 * @return 0 on success, otherwise error code.
 */
int bt_manager_deinit(void);

/**
 * @brief Get the current connection state of the Bluetooth manager.
 * @return The current connection state.
 */
uint8_t bt_manager_get_connect_state(void);

/**
 * @brief Set the current connection state of the Bluetooth manager.
 * @param state The connection state to set.
 */
void bt_manager_set_connect_state(uint8_t state);

/**
 * @brief Get the address of the device to reconnect to.
 * @param addr Pointer to store the address of the device to reconnect to.
 */
void bt_manager_get_reconnect_device(uint8_t *addr);

/**
 * @brief Get the address of the currently connected device.
 * @return Pointer to the address of the currently connected device.
 */
uint8_t* bt_manager_get_connected_device(void);

/**
 * @brief Unregister a callback function by index.
 * @param index The index of the callback to unregister.
 * @return 0 on success, otherwise error code.
 */
int bt_manager_unregister_callback(uint8_t index);

/**
 * @brief Enter the Bluetooth pairing mode.
 */
void bk_bt_enter_pairing_mode(uint8_t is_visible);

/**
 * @brief Clean the bonded devices.
 */
void bt_manager_clean_bond(void);

/**
 * @brief Save the CTKD linkkey.
 * @param addr Pointer to the address of the device.
 * @param linkkey Pointer to the linkkey.
 * @return 0 on success, otherwise error code.
 */
int bt_manager_save_ctkd_linkkey(uint8_t *addr, uint8_t *linkkey);

/**
 * @brief Set the auto accept connection.
 * @param type The type of the connection.
 * @param accept The accept state.
 */
void bt_manager_set_auto_accept_connection(uint8_t type, uint8_t accept); // type see BT_MNG_AUTO_ACCEPT_CONNECTION_ACL

/**
 * @brief Clear the reconnect information.
 */
void bt_manager_clear_reconnect_info(void);

/**
 * @brief Start Classic Bluetooth device discovery (inquiry).
 *
 * Inquiry results are delivered as BK_BT_GAP_DISC_RES_EVT to the GAP callbacks
 * registered via bt_manager_register_callback().
 *
 * @param sec Inquiry duration in seconds.
 * @param num_report Max number of responses to report (0 = unlimited; capped at 255).
 * @return 0 on success, otherwise error code.
 */
int bt_manager_discover_bt(uint32_t sec, uint32_t num_report);

/**
 * @brief Cancel an ongoing Classic Bluetooth device discovery.
 * @return 0 on success, otherwise error code.
 */
int bt_manager_cancel_discover_bt(void);
