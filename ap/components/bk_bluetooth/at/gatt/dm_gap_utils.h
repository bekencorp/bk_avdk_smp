#pragma once

/*
 * Compatibility shim. The real implementation moved to
 *   ap/components/bk_bluetooth/service/dm/ble/gatt/dm_gap_utils.{c,h}
 * Existing AT-layer code that still calls bk_at_dm_gap_* keeps working
 * through the macros below.
 */
#include "../../service/dm/ble/gatt/dm_gap_utils.h"

#define bk_at_dm_gap_is_addr_valid  bk_dm_prf_gap_is_addr_valid
#define bk_at_dm_gap_is_data_valid  bk_dm_prf_gap_is_data_valid
