#pragma once

/*
 * Compatibility shim. The real implementation moved to
 *   ap/components/bk_bluetooth/service/dm/ble/gatt/dm_gatts.{c,h}
 * Existing AT-layer code that still calls bk_at_dm_gatts_* keeps working
 * through the macros below.
 */
#include "../../service/dm/ble/gatt/dm_gatts.h"

#if CONFIG_BT && CONFIG_BLE

#define bk_at_dm_gatts_is_init                      bk_dm_prf_gatts_is_init
#define bk_at_dm_gatts_main                         bk_dm_prf_gatts_main
#define bk_at_dm_gatts_deinit                       bk_dm_prf_gatts_deinit
#define bk_at_dm_gatts_deinit_because_bluetooth_deinit_future bk_dm_prf_gatts_deinit_because_bluetooth_deinit_future
#define bk_at_dm_gatts_disconnect                   bk_dm_prf_gatts_disconnect
#define bk_at_dm_gatts_enable_adv                   bk_dm_prf_gatts_enable_adv
#define bk_at_dm_gatts_enable_service               bk_dm_prf_gatts_enable_service
#define bk_at_dm_gatts_reg_db                       bk_dm_prf_gatts_reg_db
#define bk_at_dm_gatts_unreg_db                     bk_dm_prf_gatts_unreg_db
#define bk_at_dm_gatts_get_buff_from_attr_handle    bk_dm_prf_gatts_get_buff_from_attr_handle
#define bk_at_dm_gatts_get_current_if               bk_dm_prf_gatts_get_current_if
#define bk_at_dm_gatts_add_gatts_callback           bk_dm_prf_gatts_add_gatts_callback
#define bk_at_dm_gatts_send_service_change_indicate bk_dm_prf_gatts_send_service_change_indicate
#define bk_at_dm_gatts_send_notify                  bk_dm_prf_gatts_send_notify

#else

#define bk_at_dm_gatts_is_init(...) 0
#define bk_at_dm_gatts_main(...) 0
#define bk_at_dm_gatts_deinit(...) 0
#define bk_at_dm_gatts_deinit_because_bluetooth_deinit_future(...) 0
#define bk_at_dm_gatts_disconnect(...) 0
#define bk_at_dm_gatts_enable_adv(...) 0
#define bk_at_dm_gatts_enable_service(...) 0
#define bk_at_dm_gatts_reg_db(...) 0
#define bk_at_dm_gatts_unreg_db(...) 0
#define bk_at_dm_gatts_get_buff_from_attr_handle(...) 0
#define bk_at_dm_gatts_get_current_if(...) 0
#define bk_at_dm_gatts_add_gatts_callback(...) 0
#define bk_at_dm_gatts_send_service_change_indicate(...) 0
#define bk_at_dm_gatts_send_notify(...) 0

#endif
