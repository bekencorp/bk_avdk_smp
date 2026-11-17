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

#include <os/os.h>
#include <avdk_types.h>
#include <avdk_error.h>
#include <avdk_check.h>

#define BK_CAMERA_PIN_INVALID 0xFF

typedef enum
{
    DVP_CAMERA_PORT = 1,     /**< dvp port */
    CSI_CAMERA_PORT,         /**< csi port */
} bk_camera_port_t;

typedef struct bk_camera_bus_ops_t bk_camera_bus_ops_t;
typedef struct bk_camera_bus_config_t bk_camera_bus_config_t;

struct bk_camera_bus_ops_t
{
    avdk_err_t (*read8)(bk_camera_bus_config_t *config, uint8_t reg, uint8_t *value);
    avdk_err_t (*read16)(bk_camera_bus_config_t *config, uint16_t reg, uint8_t *value);
    avdk_err_t (*write8)(bk_camera_bus_config_t *config, uint8_t reg, uint8_t value);
    avdk_err_t (*write16)(bk_camera_bus_config_t *config, uint16_t reg, uint8_t value);
};

struct bk_camera_bus_config_t
{
    uint8_t pin_scl;
    uint8_t pin_sda;
    uint8_t i2c_id;
    uint16_t write_address;
    uint32_t data_size;
    uint32_t timeout_ms;
    uint8_t mipi_port_en;
    uint8_t dvp_port_en;
    uint8_t pin_xclk;         /**< Sensor MCLK GPIO; BK_CAMERA_PIN_INVALID = no SoC MCLK */
};

typedef struct bk_camera_bus_t bk_camera_bus_t;
struct bk_camera_bus_t
{
    uint8_t pin_scl;
    uint8_t pin_sda;
    uint8_t i2c_id;
    uint16_t write_address;
    uint32_t data_size;
    uint32_t timeout_ms;
    void *i2c_handle;
    uint8_t mipi_port_en;
    uint8_t dvp_port_en;
    uint8_t pin_xclk;
    avdk_err_t (*read8)(bk_camera_bus_t *bus, uint8_t reg, uint8_t *value);
    avdk_err_t (*read16)(bk_camera_bus_t *bus, uint16_t reg, uint8_t *value);
    avdk_err_t (*write8)(bk_camera_bus_t *bus, uint8_t reg, uint8_t value);
    avdk_err_t (*write16)(bk_camera_bus_t *bus, uint16_t reg, uint8_t value);
};


/**
 * @brief Create a camera I2C bus instance
 * @param config Bus configuration (pins, I2C ID, sensor address, etc.)
 * @return Pointer to the bus instance, or NULL on failure
 */
bk_camera_bus_t *bk_camera_bus_new(bk_camera_bus_config_t *config);

/**
 * @brief Enable the camera I2C bus
 * @param bus Bus instance from bk_camera_bus_new
 * @return AVDK error code
 */
avdk_err_t bk_camera_bus_enable(bk_camera_bus_t *bus);

/**
 * @brief Suspend the camera bus: drop the AP-internal clocks and domains
 *
 * Releases the CISP clock, MIPI CSI power domain and MCLK votes while keeping
 * the I2C bus and the sensor rails enabled. Used by keep-alive paths that leave
 * the sensor powered across AP power-off; retained votes would otherwise turn
 * the next wake-up into a no-op. The matching call is bk_camera_bus_resume().
 *
 * @param bus Bus instance
 * @return AVDK error code
 */
avdk_err_t bk_camera_bus_suspend(bk_camera_bus_t *bus);

/**
 * @brief Resume the camera bus: re-acquire the AP-internal clocks and domains
 *
 * Mirror of bk_camera_bus_suspend(): re-acquires the CISP clock, MIPI CSI power
 * domain and MCLK votes without touching the I2C bus or the sensor rails. Used
 * on the keep-alive wake-up path, where a full bk_camera_bus_enable() would
 * only re-vote rails that were never released.
 *
 * @param bus Bus instance
 * @return AVDK error code
 */
avdk_err_t bk_camera_bus_resume(bk_camera_bus_t *bus);

/**
 * @brief Disable the camera I2C bus
 * @param bus Bus instance
 * @return AVDK error code
 */
avdk_err_t bk_camera_bus_disable(bk_camera_bus_t *bus);

/**
 * @brief Delete the camera I2C bus instance
 * @param bus Bus instance
 * @return AVDK error code
 */
avdk_err_t bk_camera_bus_delete(bk_camera_bus_t *bus);

/**
 * @brief Get the global camera bus instance
 * @return Pointer to the bus instance, or NULL if not created
 */
bk_camera_bus_t *bk_camera_bus_get(void);

#ifdef __cplusplus
}
#endif