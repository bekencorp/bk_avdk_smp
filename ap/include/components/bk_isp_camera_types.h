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

#include <avdk_error.h>


typedef enum
{
    ISP_CHANNEL_STATE_TURN_OFF = 0,
    ISP_CHANNEL_STATE_TURNING_ON,
    ISP_CHANNEL_STATE_TURN_ON,
    ISP_CHANNEL_STATE_TURNING_OFF,
} bk_isp_camera_channel_state_t;

typedef enum {
    ISR_TYPE_16LINE_DONE = 0,
    ISR_TYPE_FRAME_COMPLETE,
    ISR_TYPE_MAX,
} bk_camera_isr_type_t;

typedef void (*bk_camera_isr_t)(uint32_t seqence, uint32_t line, uint8_t chnl, uint8_t error, void *param);

/**
 * @brief Enumeration of MCLK clock frequencies
 */
typedef enum
{
    MCLK_15M,     /**< 15 MHz clock frequency */
    MCLK_16M,     /**< 16 MHz clock frequency */
    MCLK_20M,     /**< 20 MHz clock frequency */
    MCLK_24M,     /**< 24 MHz clock frequency */
    MCLK_30M,     /**< 30 MHz clock frequency */
    MCLK_32M,     /**< 32 MHz clock frequency */
    MCLK_40M,     /**< 40 MHz clock frequency */
    MCLK_48M,     /**< 48 MHz clock frequency */
    MCLK_UNKNOW,  /**< Unknown clock frequency */
} mclk_freq_t;

/**
 * @brief Enumeration of sync signal levels
 */
typedef enum
{
   SYNC_LOW_LEVEL,   /**< Synchronize on low level */
   SYNC_HIGH_LEVEL,  /**< Synchronize on high level */
} sync_level_t;


/**
 * @brief Enumeration of sensor identifiers
 */
typedef enum
{

    ID_UNKNOW = 0,   /**< Unknown sensor */
    ID_PAS6329,      /**< PAS6329 sensor */
    ID_OV7670,       /**< OV7670 sensor */
    ID_PAS6375,      /**< PAS6375 sensor */
    ID_GC0328C,      /**< GC0328C sensor */
    ID_BF2013,       /**< BF2013 sensor */
    ID_GC0308C,      /**< GC0308C sensor */
    ID_HM1055,       /**< HM1055 sensor */
    ID_GC2145,       /**< GC2145 sensor */
    ID_OV2640,       /**< OV2640 sensor */
    ID_GC0308,       /**< GC0308 sensor */
    ID_TVP5150,      /**< TVP5150 sensor */
    ID_SC101,        /**< SC101 sensor */
    ID_GC2053,       /**< GC2053 sensor */
    ID_GC4653,       /**< GC4653 sensor */
    ID_OV2775,       /**< OV2775 sensor */
    ID_GC2053D,      /**< GC2053D sensor */
} sensor_id_t;

/**
 * @brief Camera handle type definition
 */
typedef void *bk_cam_handle_t;

/**
 * @brief Enumeration of camera interface IOCTL commands
 */
typedef enum
{
    BK_CAM_IOCTL_UNKNOW = 0,  /**< Unknown IOCTL command */
    BK_CAM_IOCTL_SOFTRESET,  /**< Soft reset the ISP controller */
} bk_cam_interface_ioctl_t;

/**
 * @brief Camera ISP instance configuration structure
 */
typedef struct
{
    uint8_t buf_cnt;        /**< Number of frame buffers */
    uint8_t port_id;        /**< Port identifier */
    uint8_t enable_flexa;   /**< Enable Flexa: 0 - disable, 1 - enable */
    uint8_t work_mode;      /**< Operating mode: 0 - frame mode, 1 - SFW Flexa mode */
    uint16_t width;         /**< Frame width in pixels */
    uint16_t height;        /**< Frame height in pixels */
    uint16_t format;        /**< Pixel format */
} bk_isp_camera_channel_config_t;


/**
 * @brief Camera controller handle type definition
 */
typedef struct bk_camera_ctlr_t *bk_isp_camera_ctlr_handle_t;

/**
 * @brief Camera controller structure type definition
 */
typedef struct bk_camera_ctlr_t bk_camera_ctlr_t;

/**
 * @brief Camera controller operation table
 */
struct bk_camera_ctlr_t
{
    avdk_err_t (*dev_init)(bk_camera_ctlr_t *controller);   /**< Initialize controller level resources (clock, GPIO, etc.) */
    avdk_err_t (*port_init)(bk_camera_ctlr_t *controller, void *config); /**< Configure a physical port according to the supplied configuration */
    avdk_err_t (*port_change)(bk_camera_ctlr_t *controller); /**< Switch the active port or update link configuration dynamically */
    avdk_err_t (*open)(bk_camera_ctlr_t *controller, void *parameter); /**< Open the controller for streaming with the specified parameters */
    avdk_err_t (*read)(bk_camera_ctlr_t *controller, uint16_t id, uint8_t *frame, uint32_t size, uint32_t timeout); /**< Read a frame */
    avdk_err_t (*close)(bk_camera_ctlr_t *controller); /**< Stop streaming and release transient resources */
    avdk_err_t (*deinit)(bk_camera_ctlr_t *controller); /**< Deinitialize controller level resources */
    avdk_err_t (*suspend)(bk_camera_ctlr_t *controller); /**< Suspend the controller for low-power operation */
    avdk_err_t (*resume)(bk_camera_ctlr_t *controller); /**< Resume the controller after suspension */
    avdk_err_t (*ioctl)(bk_camera_ctlr_t *controller, bk_cam_interface_ioctl_t ioctl, void *arg); /**< Issue controller specific IOCTL commands */
    avdk_err_t (*del)(bk_camera_ctlr_t *controller); /**< Destroy the controller object and free memory */

    avdk_err_t (*register_isr_callback)(bk_camera_ctlr_t *controller, bk_camera_isr_type_t type, bk_camera_isr_t cb, void *arg); /**< Register ISP interrupt service routine callback */
    avdk_err_t (*deregister_isr_callback)(bk_camera_ctlr_t *controller, bk_camera_isr_type_t type, void *arg); /**< Deregister ISP interrupt service routine callback */

    avdk_err_t (*channel_open)(bk_camera_ctlr_t *controller, uint8_t channel, bk_isp_camera_channel_config_t *config); /**< Open a channel */
    avdk_err_t (*channel_close)(bk_camera_ctlr_t *controller, uint8_t channel); /**< Close a channel */
    bk_isp_camera_channel_state_t (*channel_state_get)(bk_camera_ctlr_t *controller, uint8_t channel); /**< Get a channel state */
} ;



#ifdef __cplusplus
}
#endif

