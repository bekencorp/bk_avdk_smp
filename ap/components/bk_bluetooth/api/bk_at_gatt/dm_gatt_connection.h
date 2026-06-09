#pragma once

/*
 * Compatibility shim. The real implementation moved to
 *   ap/components/bk_bluetooth/bt_dm/ble/gatt/dm_gatt_connection.{c,h}
 * Existing AT-layer code that still calls bk_at_dm_ble_* keeps working
 * through the macros below.
 *
 * ABI note: bk_at_dm_ble_alloc_profile_data_by_addr's first parameter was
 * declared as uint8_t profile_id, while the new dm_ble_alloc_profile_data_by_addr
 * uses uint32_t profile_id. C implicit promotion handles the call sites safely.
 */
#include "../../bt_dm/ble/gatt/dm_gatt_connection.h"

#if CONFIG_BT && CONFIG_BLE

#define bk_at_dm_ble_app_env_init                   dm_ble_app_env_init
#define bk_at_dm_ble_app_env_deinit                 dm_ble_app_env_deinit
#define bk_at_dm_ble_alloc_app_env_by_addr          dm_ble_alloc_app_env_by_addr
#define bk_at_dm_ble_find_app_env_by_addr           dm_ble_find_app_env_by_addr
#define bk_at_dm_ble_find_app_env_by_conn_id        dm_ble_find_app_env_by_conn_id
#define bk_at_dm_ble_del_app_env_by_addr            dm_ble_del_app_env_by_addr
#define bk_at_dm_ble_free_all_app_env               dm_ble_free_all_app_env
#define bk_at_dm_ble_alloc_addition_data_by_addr    dm_ble_alloc_addition_data_by_addr
#define bk_at_dm_ble_alloc_profile_data_by_addr     dm_ble_alloc_profile_data_by_addr
#define bk_at_dm_ble_find_profile_data_by_profile_id dm_ble_find_profile_data_by_profile_id
#define bk_at_dm_ble_app_env_foreach                dm_ble_app_env_foreach

#else

#define bk_at_dm_ble_app_env_init(...) 0
#define bk_at_dm_ble_app_env_deinit(...) 0
#define bk_at_dm_ble_alloc_app_env_by_addr(...) 0
#define bk_at_dm_ble_find_app_env_by_addr(...) 0
#define bk_at_dm_ble_find_app_env_by_conn_id(...) 0
#define bk_at_dm_ble_del_app_env_by_addr(...) 0
#define bk_at_dm_ble_free_all_app_env(...) 0
#define bk_at_dm_ble_alloc_addition_data_by_addr(...) 0
#define bk_at_dm_ble_alloc_profile_data_by_addr(...) 0
#define bk_at_dm_ble_find_profile_data_by_profile_id(...) 0
#define bk_at_dm_ble_app_env_foreach(...) 0

#endif
