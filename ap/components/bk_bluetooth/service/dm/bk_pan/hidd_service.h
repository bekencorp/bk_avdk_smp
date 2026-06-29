/**
 * @file hidd_service.h
 *
 * @brief Bluetooth HID Device demo service APIs.
 */

#ifndef HIDD_SERVICE_H
#define HIDD_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Initialize the Bluetooth HID Device demo service.
 */
void bt_hidd_init(void);

/**
 * @brief Deinitialize the Bluetooth HID Device demo service.
 */
void bt_hidd_deinit(void);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*HIDD_SERVICE_H*/
