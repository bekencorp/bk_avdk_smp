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
#include <stdbool.h>
#include <avdk_types.h>
#include <avdk_error.h>
#include <avdk_check.h>
#include <components/bk_camera_bus.h>

/**
 * @brief Camera sensor configuration structure
 */
typedef struct
{
    uint8_t pin_reset;        /**< Reset pin number */
    uint8_t pin_pwdn;         /**< Power-down pin number */
    uint8_t pin_xclk;         /**< Clock pin number */
    bk_camera_bus_t *bus;     /**< Pointer to camera bus interface */
} bk_camera_sensor_config_t;

/**
 * @brief Camera sensor format configuration structure
 *
 * Each entry describes one operating mode that the sensor natively supports.
 * `output_pixel_fmt` declares the raw Bayer/pixel format the sensor emits in
 * this mode (e.g. RGGB8 for DVP sensors, RGGB10 for MIPI raw sensors). It is
 * a property of the sensor/mode itself, not an application-time choice, so
 * device drivers must populate it inside their `xxx_format_array[]`.
 *
 * On the downstream side, applications should copy this value into
 * `bk_isp_camera_ctlr_config_t.input_pixel_fmt` (sensor output == ISP input)
 * after locating the matched mode via `bk_camera_sensor_query_support_formats()`.
 */
typedef struct
{
    uint16_t width;                     /**< Image width (pixels) */
    uint16_t height;                    /**< Image height (pixels) */
    uint16_t fps;                       /**< Frame rate (frames/second) */
    uint32_t xclk;                      /**< External clock frequency (Hz) */
    bk_pixel_format_t output_pixel_fmt; /**< Sensor output pixel format (e.g. RGGB8 / RGGB10) */
} bk_camera_sensor_format_t;

/**
 * @brief Supported format array structure for camera sensor
 */
typedef struct
{
    uint32_t size;                              /**< Size of the format array */
    const bk_camera_sensor_format_t *format_array;  /**< Pointer to the format array */
} bk_camera_sensor_format_array_t;

typedef struct bk_camera_sensor_ctlr_t *bk_camera_sensor_handle_t;
typedef struct bk_camera_sensor_ctlr_t bk_camera_sensor_ctlr_t;

/**
 * @brief Camera sensor controller operations structure
 */
struct bk_camera_sensor_ctlr_t
{
    int (*init)(bk_camera_sensor_ctlr_t *controller);  /**< Initialize CSI sensor */
    int (*set_format)(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format);  /**< Set sensor resolution */
    int (*power_down)(bk_camera_sensor_ctlr_t *controller);  /**< Power down or reset the sensor */
    int (*reg_ctrl)(bk_camera_sensor_ctlr_t *controller, uint8_t cmd, uint16_t addr, uint8_t val);  /**< Register control function */
    avdk_err_t (*set_hmirror)(bk_camera_sensor_ctlr_t *controller, bool enable);  /**< Horizontal mirror */
    avdk_err_t (*set_vflip)(bk_camera_sensor_ctlr_t *controller, bool enable);    /**< Vertical flip */
    void *(*get_sensor_object)(bk_camera_sensor_ctlr_t *controller);  /**< Get pointer to sensor object */
    void *(*get_sensor_cfg)(bk_camera_sensor_ctlr_t *controller);   /**< Get pointer to sensor configuration */
    avdk_err_t (*query_support_formats)(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array);  /**< Query supported formats */
} ;


/**
 * @brief CSI camera sensor structure
 */
typedef struct
{
    const void *isp_pub_attr;          /**< Pointer to ISP public attributes */
    const void *sensor_config;         /**< Pointer to sensor-specific configuration */
    bk_camera_sensor_config_t config; /**< Sensor configuration */
    bk_camera_sensor_ctlr_t ops;       /**< Sensor controller operations */
} bk_camera_csi_sensor_t;

/**
 * @brief DVP camera sensor structure
 */
typedef struct
{
    const void *isp_pub_attr;          /**< Pointer to ISP public attributes */
    const void *sensor_config;         /**< Pointer to sensor-specific configuration */
    bk_camera_sensor_config_t config; /**< Sensor configuration */
    bk_camera_sensor_ctlr_t ops;       /**< Sensor controller operations */
} bk_camera_dvp_sensor_t;


/**
 * @brief Initialize the camera sensor
 * @param handle Sensor handle
 * @return Error code
 */
avdk_err_t bk_camera_sensor_init(bk_camera_sensor_handle_t handle);

/**
 * @brief Get pointer to the sensor object
 * @param handle Sensor handle
 * @return Pointer to the sensor object
 */
void *bk_camera_sensor_get_sensor_object(bk_camera_sensor_handle_t handle);

/**
 * @brief Get pointer to the sensor configuration
 * @param handle Sensor handle
 * @return Pointer to the sensor configuration
 */
void *bk_camera_sensor_get_sensor_cfg(bk_camera_sensor_handle_t handle);

/**
 * @brief Set sensor format
 * @param handle Sensor handle
 * @param format Format configuration
 * @return Error code
 */
avdk_err_t bk_camera_sensor_set_format(bk_camera_sensor_handle_t handle, bk_camera_sensor_format_t *format);

/**
 * @brief Set horizontal mirror on the sensor output.
 * @return AVDK_ERR_OK on success, AVDK_ERR_UNSUPPORTED if the sensor has no hook.
 */
avdk_err_t bk_camera_sensor_set_hmirror(bk_camera_sensor_handle_t handle, bool enable);

/**
 * @brief Set vertical flip on the sensor output.
 * @return AVDK_ERR_OK on success, AVDK_ERR_UNSUPPORTED if the sensor has no hook.
 */
avdk_err_t bk_camera_sensor_set_vflip(bk_camera_sensor_handle_t handle, bool enable);

/**
 * @brief Query supported sensor formats
 * @param handle Sensor handle
 * @param format_array Output parameter for the format array
 * @return Error code
 */
avdk_err_t bk_camera_sensor_query_support_formats(bk_camera_sensor_handle_t handle, bk_camera_sensor_format_array_t *format_array);

/**
 * @brief Auto-detect the camera sensor
 * @param config Sensor configuration
 * @param port Camera port
 * @return Sensor handle, or NULL on failure
 */
bk_camera_sensor_handle_t bk_camera_sensor_auto_detect(bk_camera_sensor_config_t *config, bk_camera_port_t port);

/**
 * @brief Release heap allocated by auto-detect (bk_camera_csi_sensor_t / bk_camera_dvp_sensor_t wrapper).
 * @param handle Handle from bk_camera_sensor_auto_detect or equivalent; NULL is a no-op.
 */
void bk_camera_sensor_destroy(bk_camera_sensor_handle_t handle);

/**
 * @brief Camera sensor detection function structure
 */
typedef struct
{
    bk_camera_port_t port;     /**< Camera port */
    avdk_err_t (*detect)(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);  /**< Detection function pointer */
} bk_camera_sensor_detect_func_t;

/**
 * @brief Section attribute macro implementation
 * @param SECTION Section name
 * @param COUNTER Counter
 */
#define _SECTION_ATTR_IMPL(SECTION, COUNTER)    __attribute__((section(SECTION "." _CPIMTER_STRINGIFY(COUNTER))))

/**
 * @brief Stringify macro
 * @param COUNTER Counter
 */
#define _CPIMTER_STRINGIFY(COUNTER) #COUNTER

/**
 * @brief Camera sensor detection function section registration macro
 * @param f Detection function name
 * @param p Port type
 *
 * This macro registers a sensor detection function into a dedicated section,
 * enabling the system to automatically discover and run detection routines.
 */
#define BK_CAMERA_SENSOR_DETECT_SECTION(f, p)                                                                   \
    avdk_err_t __bk_sensor_detect_##f(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);    \
    __attribute__((used)) _SECTION_ATTR_IMPL(".camera_sensor_detect_function_list", __COUNTER__)                \
    const bk_camera_sensor_detect_func_t bk_sensor_detect_##f = {                                               \
        .detect = __bk_sensor_detect_##f,                                                                       \
        .port = p,                                                                                              \
    } ;                                                                                                         \
    avdk_err_t __bk_sensor_detect_##f(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)     \
    { return f(handle, config); }

extern bk_camera_sensor_detect_func_t __camera_sensor_detect_array_start;
extern bk_camera_sensor_detect_func_t __camera_sensor_detect_array_end;

#ifdef __cplusplus
}
#endif

