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
#include "components/dvp_camera_types.h"
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Read sensor register value
 *
 * This API will called after bk_i2c_init
 *
 * @param addr sensor read address
 *
 * @param reg sensor register address
 *
 * @param value sensor register value
 *
 * @return
 *    - 0 : succeed
 *    - other: other errors.
 */
int dvp_camera_i2c_read_uint8(uint8_t addr, uint8_t reg, uint8_t *value);

/**
 * @brief     Read sensor register value
 *
 * This API will called after bk_i2c_init
 *
 * @param addr sensor read address
 *
 * @param reg sensor register address
 *
 * @param value sensor register value
 *
 * @return
 *    - 0 : succeed
 *    - other: other errors.
 */
int dvp_camera_i2c_read_uint16(uint8_t addr, uint16_t reg, uint8_t *value);

/**
 * @brief     Write sensor register value
 *
 * This API will called after bk_i2c_init
 *
 * @param addr sensor read address
 *
 * @param reg sensor register address
 *
 * @param value sensor register value
 *
 * @return
 *    - 0 : succeed
 *    - other: other errors.
 */
int dvp_camera_i2c_write_uint8(uint8_t addr, uint8_t reg, uint8_t value);

/**
 * @brief     Write sensor register value
 *
 * This API will called after bk_i2c_init
 *
 * @param addr sensor write address
 *
 * @param reg sensor register address
 *
 * @param value sensor register value
 *
 * @return
 *    - 0 : succeed
 *    - other: other errors.
 */
int dvp_camera_i2c_write_uint16(uint8_t addr, uint16_t reg, uint8_t value);

/**
 * @brief     set the I2C bus id used for DVP sensor register read/write
 *
 * bk_dvp_detect() calls this with bk_dvp_config_t.i2c_config.id so the sensor
 * register read/write follows the same bus that bk_i2c_init() was configured
 * with. This makes i2c_config.id the single source of truth and removes the
 * need to also define CONFIG_DVP_CAMERA_I2C_ID.
 *
 * @param id I2C bus id (the same value passed as i2c_config.id to bk_dvp_open)
 */
void dvp_camera_i2c_set_id(uint8_t id);


#ifdef __cplusplus
}
#endif
