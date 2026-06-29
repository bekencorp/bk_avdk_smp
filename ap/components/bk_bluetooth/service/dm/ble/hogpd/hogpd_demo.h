#pragma once

/**
 * @file hogpd_demo.h
 *
 * @brief HID-over-GATT keyboard demo service APIs.
 */

/** Enable the HOGPD demo implementation. */
#define HOGPD_DEMO_ENABLE 1

/**
 * @brief Initialize the HOGPD demo.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t hogpd_demo_init(void);

/**
 * @brief Deinitialize the HOGPD demo.
 *
 * @param deinit_bluetooth_future Non-zero when called as part of Bluetooth
 * deinitialization flow.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t hogpd_demo_deinit(uint8_t deinit_bluetooth_future);

/**
 * @brief Deinitialize HOGPD during Bluetooth deinitialization.
 *
 * @return 0 on success, otherwise error code.
 */
int32_t hogpd_demo_deinit_because_bluetooth_deinit_future(void);
