#pragma once

/**
 * @file bluetooth_storage.h
 *
 * @brief Bluetooth service storage APIs.
 *
 * This module stores BT link keys, per-device volume values, BLE bond
 * information, local BLE keys, and address hash mappings used by Bluetooth
 * service modules.
 */

#include <stdint.h>

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"


/** Persistent storage key used for Bluetooth user information. */
#define BT_STORAGE_KEY "bluetooth_user_info"

/** Maximum number of BT link key or BLE bond records saved by this module. */
#define BT_LINKKEY_MAX_SAVE_COUNT 3

/** Per-device BT link key and profile volume record. */
typedef struct __attribute__((packed))
{
    //    char addr[6 * 2 + 5 + 1];
    //    char link_key[16 * 2 + 1];
    uint8_t addr[6];      /**< Remote Bluetooth device address. */
    uint8_t link_key[16]; /**< BT link key. */
    uint8_t a2dp_volume;  /**< Saved A2DP volume. */
    uint32_t hash;        /**< Address hash used for compact lookup. */
    uint8_t hfp_mic_vol;  /**< Saved HFP microphone volume. */
    uint8_t hfp_spk_vol;  /**< Saved HFP speaker volume. */
} bt_user_storage_elem_linkkey_t;


/** Bluetooth user storage data persisted by the service layer. */
typedef struct __attribute__((packed))
{
    bt_user_storage_elem_linkkey_t linkkey[BT_LINKKEY_MAX_SAVE_COUNT]; /**< Saved BT link key records. */
#if CONFIG_BLE
    bk_ble_bond_dev_t ble_key[BT_LINKKEY_MAX_SAVE_COUNT]; /**< Saved BLE bond records. */
    bk_ble_local_keys_t local_keys; /**< Saved local BLE keys. */
#endif
} bt_user_storage_t;


/**
 * @brief Initialize Bluetooth storage.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_init(void);

/**
 * @brief Deinitialize Bluetooth storage.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_deinit(void);

/**
 * @brief Find a BT link key record by address.
 *
 * @param addr Remote Bluetooth device address.
 * @param key Output buffer for the link key. Pass NULL when only the index is needed.
 *
 * @return Record index on success, otherwise negative error code.
 */
int32_t bluetooth_storage_find_linkkey_info_index(uint8_t *addr, uint8_t *key);

/**
 * @brief Save or update a BT link key record.
 *
 * @param addr Remote Bluetooth device address.
 * @param key BT link key buffer. The buffer length must be 16 bytes.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_save_linkkey_info(uint8_t *addr, uint8_t *key);

/**
 * @brief Delete a BT link key record.
 *
 * @param addr Remote Bluetooth device address.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_del_linkkey_info(uint8_t *addr);

/**
 * @brief Move a BT link key record to the newest position.
 *
 * @param addr Remote Bluetooth device address.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_update_to_newest(uint8_t *addr);

/**
 * @brief Get the newest valid BT link key record.
 *
 * @param addr Output buffer for the remote Bluetooth device address.
 * @param key Output buffer for the link key. Pass NULL when only the address is needed.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_get_newest_linkkey_info(uint8_t *addr, uint8_t *key);

/**
 * @brief Clear all BT link key records.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_clean_linkkey_info(void);

/**
 * @brief Synchronize Bluetooth storage data to flash.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_sync_to_flash(void);

/**
 * @brief Print BT link key records for debugging.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_linkkey_debug(void);

/**
 * @brief Find saved A2DP volume by address.
 *
 * @param addr Remote Bluetooth device address.
 * @param volume Output A2DP volume.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_find_volume_by_addr(uint8_t *addr, uint8_t *volume);

/**
 * @brief Find saved HFP microphone and speaker volume by address.
 *
 * @param addr Remote Bluetooth device address.
 * @param mic_vol Output HFP microphone volume.
 * @param spk_vol Output HFP speaker volume.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_find_hfp_volume_by_addr(uint8_t *addr, uint8_t *mic_vol, uint8_t *spk_vol);

/**
 * @brief Save A2DP volume for a remote device.
 *
 * @param addr Remote Bluetooth device address.
 * @param volume A2DP volume.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_save_volume(uint8_t *addr, uint8_t volume);

/**
 * @brief Save HFP volume for a remote device.
 *
 * @param addr Remote Bluetooth device address.
 * @param type Volume target. 0 indicates microphone and 1 indicates speaker.
 * @param volume HFP volume.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_save_hfp_volume(uint8_t *addr, uint8_t type, uint8_t volume);

#if CONFIG_BLE
/**
 * @brief Save BLE bond information.
 *
 * @param list BLE bond record list.
 * @param count Number of records in the list.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_save_ble_key_info(bk_ble_bond_dev_t *list, uint32_t count);

/**
 * @brief Clear all BLE bond information.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_clean_ble_key_info(void);

/**
 * @brief Read BLE bond information.
 *
 * @param list Output BLE bond record list.
 * @param count Input capacity and output record count.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_read_ble_key_info(bk_ble_bond_dev_t *list, uint32_t *count);

/**
 * @brief Save local BLE keys.
 *
 * @param key Local BLE key data.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_save_local_key(bk_ble_local_keys_t *key);

/**
 * @brief Clear local BLE keys.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_clean_local_key(void);

/**
 * @brief Read local BLE keys.
 *
 * @param key Output local BLE key data.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_read_local_key(bk_ble_local_keys_t *key);
#endif

/**
 * @brief Save an address hash for a remote device.
 *
 * @param addr Remote Bluetooth device address.
 * @param hash Address hash value.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_save_hash(uint8_t *addr, uint32_t hash);

/**
 * @brief Find a remote device address by hash.
 *
 * @param addr Output remote Bluetooth device address.
 * @param hash Address hash value.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t bluetooth_storage_find_addr_by_hash(uint8_t *addr, uint32_t hash);

/**
 * @brief Get the number of bonded BT devices.
 *
 * @return Number of bonded devices.
 */
uint8_t bluetooth_storage_get_bond_device_num(void);

/**
 * @brief Get compact bond hash values.
 *
 * @param hasharray Output hash array.
 * @param arraylen Number of elements in hasharray.
 *
 * @return Number of hash values written to hasharray.
 */
uint32_t bluetooth_storage_get_bond_hash(uint16_t hasharray[], uint32_t arraylen );
