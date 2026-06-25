#pragma once

/*
 * Compatibility shim. The real implementation moved to
 *   ap/components/bk_bluetooth/service/dm/ble/gatt/dm_gattc.{c,h}
 *
 * Note: the new dm_gattc_connect takes an additional s_timeout argument.
 * bk_at_dm_gattc_connect is provided as a static inline wrapper that
 * delegates to dm_gattc_connect_ext(NULL) (which itself falls back to
 * dm_gattc_connect with the historical 500ms supervision timeout).
 */
#include "../../service/dm/ble/gatt/dm_gattc.h"


#if CONFIG_BT && CONFIG_BLE

#define bk_at_dm_gattc_main                  dm_gattc_main
#define bk_at_dm_gattc_deinit                dm_gattc_deinit
#define bk_at_dm_gattc_connect_ext           dm_gattc_connect_ext
#define bk_at_dm_gattc_disconnect            dm_gattc_disconnect
#define bk_at_dm_gattc_connect_cancel        dm_gattc_connect_cancel
#define bk_at_dm_gattc_discover              dm_gattc_discover
#define bk_at_dm_gattc_write                 dm_gattc_write
#define bk_at_dm_gattc_write_ext             dm_gattc_write_ext
#define bk_at_dm_gattc_read                  dm_gattc_read
#define bk_at_dm_gattc_send_mtu_req          dm_gattc_send_mtu_req
#define bk_at_dm_gattc_add_gattc_callback    dm_gattc_add_gattc_callback

#else

#define bk_at_dm_gattc_main(...) 0
#define bk_at_dm_gattc_deinit(...) 0
#define bk_at_dm_gattc_connect_ext(...) 0
#define bk_at_dm_gattc_disconnect(...) 0
#define bk_at_dm_gattc_connect_cancel(...) 0
#define bk_at_dm_gattc_discover(...) 0
#define bk_at_dm_gattc_write(...) 0
#define bk_at_dm_gattc_write_ext(...) 0
#define bk_at_dm_gattc_read(...) 0
#define bk_at_dm_gattc_send_mtu_req(...) 0
#define bk_at_dm_gattc_add_gattc_callback(...) 0

#endif

static inline int32_t bk_at_dm_gattc_connect(uint8_t *addr, uint32_t addr_type)
{
    return dm_gattc_connect_ext(addr, addr_type, NULL);
}
