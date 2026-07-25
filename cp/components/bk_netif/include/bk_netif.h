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

#pragma once

#include <common/bk_err.h>
#include <components/netif_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BK Netif internal API
 * @defgroup bk_api_netif_internal BK Netif Internal API
 * @{
 */

/**
 * @brief  Initialize netif component (SDK internal)
 *
 * Registers the Wi-Fi event callback used to bring up STA/AP lwIP netifs.
 * Called automatically from ``bk_init()`` via ``app_wifi_init()``; applications
 * do not need to call this API.
 *
 * @return
 *   - BK_OK: success
 *   - others: failure
 */
bk_err_t bk_netif_init(void);

/**
 * @brief  Switch STA netif IP mode to DHCP (internal / debug use)
 *
 * This API only updates the STA IP mode setting to DHCP via ip_address_set().
 * It does not start the lwIP DHCP client; STA normally obtains an IP automatically
 * via sta_ip_start() when Wi-Fi connects.
 *
 * @attention Typical applications do not need to call this API. It is mainly used
 *            by the ``dhcpc`` CLI command to switch back to DHCP after a static IP
 *            was configured. If the interface is already up with a static IP, stop
 *            or restart the STA interface after calling this API.
 *
 * @param ifx  netif interface ID, currently only NETIF_IF_STA is supported.
 *
 * @return
 *   - BK_OK: succeed
 *   - BK_ERR_NETIF_IF: invalid netif interface ID
 */
bk_err_t bk_netif_dhcpc_start(netif_if_t ifx);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
