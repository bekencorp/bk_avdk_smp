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

#include <avdk_error.h>
#include <components/usb_types.h>
#include <common/avdk_pixel_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of UVC ports
 * 
 * This macro defines the maximum number of UVC ports supported,
 * based on the USB host hub maximum external ports configuration.
 */
#ifdef CONFIG_USBHOST_HUB_MAX_EHPORTS
#define UVC_PORT_MAX                      (CONFIG_USBHOST_HUB_MAX_EHPORTS)
#else
#define UVC_PORT_MAX                      (1)
#endif

/**
 * @brief Enumeration of UVC connection states
 */
typedef enum {
    UVC_CONNECTED = 0,     /**< Device is connected and streaming capable */
    UVC_DISCONNECTED,      /**< Device has been disconnected */
} uvc_state_t;

/**
 * @brief Auxiliary information returned by a frame separation callback
 *
 * When a JPEG or H.26x frame spans multiple packets, the separation callback
 * can report how much valid data remains to be processed.
 * - data_len: number of valid bytes observed in the current packet.
 * - data_off: pointer to the first valid byte within the packet buffer.
 */
typedef struct
{
    uint32_t data_len;
    uint8_t *data_off;
} uvc_separate_info_t;

/**
 * @brief UVC camera runtime configuration parameters
 */
typedef struct
{
    bk_image_format_t format;
    uint16_t width;
    uint16_t height;
    uint32_t fps;
    uint8_t port;
    uint8_t drop_num;
} bk_cam_uvc_config_t;

/**
 * @brief Callback hooks provided by the application to the UVC controller
 */
typedef struct
{
    frame_buffer_t *(*frame_malloc)(bk_image_format_t format, uint32_t size);
    void (*frame_complete)(uint8_t port, bk_image_format_t format, frame_buffer_t *buffer, int result);
    void (*state_change_cb)(uvc_state_t state, void *user_data);
    void *user_data;
} bk_uvc_callback_t;

typedef struct bk_uvc_ctlr *bk_uvc_ctlr_handle_t;
typedef struct bk_uvc_ctlr bk_uvc_ctlr_t;

struct bk_uvc_ctlr
{
    avdk_err_t (*init)(bk_uvc_ctlr_t *controller);                      /**< Initialize the controller and allocate resources */
    avdk_err_t (*deinit)(bk_uvc_ctlr_t *controller);                    /**< Deinitialize the controller */
    avdk_err_t (*open)(bk_uvc_ctlr_t *controller, bk_cam_uvc_config_t *config); /**< Open a UVC device with the supplied configuration */
    avdk_err_t (*close)(bk_uvc_ctlr_t *controller);                     /**< Close the active UVC device */
    avdk_err_t (*suspend)(bk_uvc_ctlr_t *controller);                   /**< Suspend the controller for power saving */
    avdk_err_t (*resume)(bk_uvc_ctlr_t *controller);                    /**< Resume controller operation after suspend */
    avdk_err_t (*ioctl)(bk_uvc_ctlr_t *controller, uint32_t event, void *arg); /**< Issue controller specific IOCTL command */
    avdk_err_t (*del)(bk_uvc_ctlr_t *controller);                       /**< Delete the controller instance */
};

/**
 * @brief Per-device callbacks used to separate and process UVC payloads
 */
typedef struct
{
    uint8_t id; // camera_id
    void (*uvc_separate_packet_cb)(uint8_t *data, uint32_t length, uvc_separate_info_t *sepatate_info);
    bk_err_t (*uvc_init_packet_cb)(bk_cam_uvc_config_t *device, uint8_t init, bk_uvc_callback_t *cb);
    void (*uvc_eof_packet_cb)(bk_cam_uvc_config_t *device);
} uvc_separate_config_t;

#define MEDIA_UVC_MJPEG_864X480_30FPS_CONFIG() {     \
        .format = BK_IMAGE_FORMAT_MJPEG,          \
        .width = 864,                \
        .height = 480,               \
        .fps = 30,                    \
        .port = 1,                 \
        .drop_num = 0,                      \
    }

#ifdef __cplusplus
}
#endif
