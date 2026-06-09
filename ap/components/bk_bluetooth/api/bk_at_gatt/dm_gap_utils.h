#pragma once

/*
 * Compatibility shim. The real implementation moved to
 *   ap/components/bk_bluetooth/bt_dm/ble/gatt/dm_gap_utils.{c,h}
 * Existing AT-layer code that still calls bk_at_dm_gap_* keeps working
 * through the macros below.
 */
#include "../../bt_dm/ble/gatt/dm_gap_utils.h"

#define bk_at_dm_gap_is_addr_valid  dm_gap_is_addr_valid
#define bk_at_dm_gap_is_data_valid  dm_gap_is_data_valid
