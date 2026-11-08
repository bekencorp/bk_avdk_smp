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

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_types.h"
#include "uvc_uac_api_common.h"

/*******************************************************************************
*                      Function Declarations
*******************************************************************************/

/**
 * @brief     USB Driver initialization
 *
 * Register the adaptation layer and pass the function pointers used by 
 * the closed source library. Register the current USB Class type to be 
 * supported, and it can be uninstalled and reinitialized during execution.
 * But be sure to do so before calling other interfaces.
 *
 *   - Configure Interrupt handling function registration
 *   - Configure Class Driver Registration
 *
 * This API should be called before any other USB APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usb_driver_init(void);

/**
 * @brief     USB Driver uninstallation
 *
 * Turn off the USB main interrupt switch
 * Unregister Total Interrupt Handling Function
 * Unregistered Class Driver
 *
 * This API should be called after any other USB APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usb_driver_deinit(void);

/**
 * @brief     Pin controlled USB Vbus
 *
 * This API controls VBUS for USB:
 *   - Determine whether to output voltage through the 
 *    high and low levels of a GPIO output
 *  parameter: gpio_id 
 *               Select the corresponding pin number
 *             ops
 *               Choose to power on or off
 *
 *   - Configure whether USB is power on
 *
 * When the fusion device is working, if there is a device voting for use,
 * it cannot be directly turned off.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usb_power_ops(uint32_t gpio_id, bool ops);

/**
 * @brief     open the USB
 *
 * This API configure the resoure common to USB:
 *   - Select the identity as host or slave
 *  parameter: usb_mode
 *            USB_HOST_MODE   = 0
 *            USB_DEVICE_MODE = 1
 *
 *   - Configure USB common clock
 *   - Configure USB common registers
 *
 * This API should be called before any other USB APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usb_open (uint32_t usb_mode);

/**
 * @brief     close the USB
 *
 * This API release USB resources:
 *   - Restore register
 *   - Turn off the power and clock
 *
 * This API should be called after any other USB APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usb_close (void);

/**
 * @brief     open the FS/LS USB IP (dual-IP mode)
 *
 * Bring the second BK7259 USB IP (SOC_USB_FS_BASE) up as a CherryUSB
 * host on bus 1 while the HS IP keeps bus 0. Compiles only with
 * CONFIG_USB_FS_HOST=y; returns BK_FAIL otherwise.
 *
 * Shared-resource policy with the HS port:
 *   - VBUS LDO (bk_usb_power_ops) is turned on automatically if nobody
 *     has enabled it yet, and released by whichever USB port closes last
 *     (the HS close path picks the ownership up if it closes after this
 *     one).
 *   - The PM sleep vote (PM_SLEEP_MODULE_NAME_USB_1) is shared: sleep is
 *     re-allowed only after both ports are closed.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usb_fs_open (uint32_t usb_mode);

/**
 * @brief     close the FS/LS USB IP
 *
 * Counterpart of bk_usb_fs_open(). Shared resources are released only
 * when the HS port is closed as well.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_usb_fs_close (void);

/**
 * @brief     Obtain whether a device is attached to the FS/LS port
 *
 * @return
 *    - TRUE: connect
 *    - FALSE: disconnect (also FALSE when the FS bus is closed or
 *      CONFIG_USB_FS_HOST is disabled)
*/
bool bk_usb_fs_get_device_connect_status(void);

/**
 * @brief     Obtain whether there is currently a device connection
 *
 *
 * @return
 *    - TRUE: connect
 *    - FALSE: disconnect
*/
bool bk_usb_get_device_connect_status(void);

#ifdef __cplusplus
}
#endif