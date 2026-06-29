/**
 * @file pan_service.h
 *
 * @brief Bluetooth PAN service wrapper APIs.
 *
 * This header provides helpers for initializing the PAN service, registering
 * PAN demo CLI commands, and controlling PAN reconnect flow.
 */

#ifndef PAN_SERVICE_H
#define PAN_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Initialize the PAN service.
 *
 * @return 0 on success, otherwise error code.
 */
int pan_service_init(void);

/**
 * @brief Register PAN demo CLI commands.
 *
 * @return 0 on success, otherwise error code.
 */
int cli_pan_demo_init(void);

/**
 * @brief Start PAN reconnect flow.
 */
void bt_start_pan_reconnect(void);

/** @cond */
void bk_bt_enter_pairing_mode(uint8_t is_visible);
/** @endcond */

/**
 * @brief Print the current PAN TX data cache count.
 */
void pan_show_tx_data_cache_count(void);

/**
 * @brief Deinitialize the PAN service.
 *
 * @return 0 on success, otherwise error code.
 */
int pan_service_deinit(void);

/**
 * @brief Unregister PAN demo CLI commands.
 *
 * @return 0 on success, otherwise error code.
 */
int cli_pan_demo_deinit(void);

/**
 * @brief Handle PAN reconnect failure.
 */
void bt_pan_reconnect_failure_handler(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*PAN_SERVICE_H*/
