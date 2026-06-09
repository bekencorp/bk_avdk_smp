// Copyright 2024
//
// Software I2C Configuration Header
// This header provides the configuration structure for software I2C
// with configurable GPIO pins.

#pragma once

#include <common/bk_err.h>
#include <driver/gpio_types.h>
#include <driver/i2c_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Software I2C Handle (Opaque)
 * 
 * Handle for software I2C instance. Caller receives this from sw_i2c_init()
 * and passes it to all subsequent I2C operations.
 */
typedef struct {
	gpio_id_t sda_pin;
	gpio_id_t scl_pin;
} sw_i2c_handle_t;



/**
 * @brief Software I2C Configuration Structure
 * 
 * Configuration for software I2C with GPIO pin specification.
 * Only contains the necessary fields for software I2C implementation.
 * 
 * Features:
 *  - No global variables
 *  - Caller manages handle pointer
 *  - Multiple instances supported (limited by memory)
 *  - Each instance with independent GPIO pins
 * 
 * Usage:
 * @code
 * // Configure I2C with specific GPIO pins
 * sw_i2c_config_t i2c_cfg = {
 *     .sda_pin = GPIO_34,  // SDA GPIO pin
 *     .scl_pin = GPIO_35   // SCL GPIO pin
 * };
 * 
 * // Initialize - returns handle (caller saves it)
 * sw_i2c_handle_t *handle = sw_i2c_init(&i2c_cfg);
 * if (handle == NULL) {
 *     // Initialization failed
 * }
 * 
 * // Use - pass handle to operations
 * sw_i2c_master_write(handle, dev_addr, data, size, timeout);
 * sw_i2c_master_read(handle, dev_addr, buffer, size, timeout);
 * 
 * // Deinitialize - frees handle
 * sw_i2c_deinit(handle);
 * @endcode
 * 
 * @note Thread Safety: Software I2C transactions are serialized in the
 *       driver, including transactions from different handles.
 */
typedef struct {
	gpio_id_t sda_pin;         /**< SDA GPIO pin number */
	gpio_id_t scl_pin;         /**< SCL GPIO pin number */
} sw_i2c_config_t;

/**
 * @brief Initialize software I2C
 * 
 * @param cfg Configuration pointer with GPIO pins
 * @return Handle pointer on success, NULL on failure
 * 
 * @note Caller must save the returned handle and use it for all operations
 */
sw_i2c_handle_t* sw_i2c_init(const sw_i2c_config_t *cfg);

/**
 * @brief Deinitialize software I2C
 * 
 * @param handle Handle returned by sw_i2c_init()
 * @return BK_OK on success, error code otherwise
 * 
 * @note After calling this, the handle pointer is invalid
 */
bk_err_t sw_i2c_deinit(sw_i2c_handle_t *handle);

/**
 * @brief Write data to I2C device
 * 
 * @param handle Handle returned by sw_i2c_init()
 * @param dev_addr Device address
 * @param data Data buffer
 * @param size Data size
 * @param timeout_ms Timeout in milliseconds
 * @return BK_OK on success, error code otherwise
 */
bk_err_t sw_i2c_master_write(sw_i2c_handle_t *handle, uint32_t dev_addr, 
                              const uint8_t *data, uint32_t size, uintptr_t timeout_ms);

/**
 * @brief Read data from I2C device
 * 
 * @param handle Handle returned by sw_i2c_init()
 * @param dev_addr Device address
 * @param data Data buffer
 * @param size Data size
 * @param timeout_ms Timeout in milliseconds
 * @return BK_OK on success, error code otherwise
 */
bk_err_t sw_i2c_master_read(sw_i2c_handle_t *handle, uint32_t dev_addr, 
                             uint8_t *data, uint32_t size, uintptr_t timeout_ms);

/**
 * @brief Write to I2C device memory
 * 
 * @param handle Handle returned by sw_i2c_init()
 * @param mem_param Memory parameters including device address, memory address, data, etc.
 * @return BK_OK on success, error code otherwise
 * 
 * Example:
 * @code
 * i2c_mem_param_t mem_param = {
 *     .dev_addr = 0x50,              // EEPROM address
 *     .mem_addr = 0x0100,            // Memory address
 *     .mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT,
 *     .data = write_buffer,
 *     .data_size = sizeof(write_buffer),
 *     .timeout_ms = 1000             // Not used in software I2C
 * };
 * sw_i2c_memory_write(handle, &mem_param);
 * @endcode
 */
bk_err_t sw_i2c_memory_write(sw_i2c_handle_t *handle, i2c_mem_param_t *mem_param);

/**
 * @brief Read from I2C device memory
 * 
 * @param handle Handle returned by sw_i2c_init()
 * @param mem_param Memory parameters including device address, memory address, data buffer, etc.
 * @return BK_OK on success, error code otherwise
 * 
 * Example:
 * @code
 * i2c_mem_param_t mem_param = {
 *     .dev_addr = 0x50,              // EEPROM address
 *     .mem_addr = 0x0100,            // Memory address
 *     .mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT,
 *     .data = read_buffer,
 *     .data_size = sizeof(read_buffer),
 *     .timeout_ms = 1000             // Not used in software I2C
 * };
 * sw_i2c_memory_read(handle, &mem_param);
 * @endcode
 */
bk_err_t sw_i2c_memory_read(sw_i2c_handle_t *handle, i2c_mem_param_t *mem_param);

#ifdef __cplusplus
}
#endif

