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

#include <stdint.h>
#include <avdk_error.h>
#include <driver/hal/hal_yuv_buf_types.h>
#include <driver/isp_types.h>

#define BK_ISP_CAMERA_INVALID_PORT_ID 0xFFU


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
    ID_CV2002,       /**< CV2002 sensor */
    ID_CV2005,       /**< CV2005 sensor */
    ID_CV2008,       /**< CV2008 sensor */
    ID_GC20C3,       /**< GC20C3 sensor */
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
    BK_CAM_IOCTL_GET_EXPOSURE_LUMINANCE,  /**< Get weighted mean exposure luminance (0-255000) into a uint32_t */
    BK_CAM_IOCTL_SET_SKIP_FRAMES, /**< Set skip_frames for a channel before channel_open; arg = bk_isp_camera_skip_frames_config_t * */
    BK_CAM_IOCTL_GET_CPROC, /**< Get CPROC attributes; arg = bk_isp_cproc_attr_t * */
    BK_CAM_IOCTL_SET_CPROC, /**< Set CPROC attributes; arg = bk_isp_cproc_attr_t * */
    BK_CAM_IOCTL_QUERY_EXPOSURE_INFO, /**< Query current AE result; arg = bk_isp_camera_exposure_info_t * */
    BK_CAM_IOCTL_SET_INITIAL_EXPOSURE, /**< Seed manual exposure before channel open; arg = bk_isp_camera_exposure_info_t * */
    BK_CAM_IOCTL_RESUME_AUTO_EXPOSURE, /**< Resume auto AE after the initial frame; arg = NULL */
    BK_CAM_IOCTL_CHANNEL_ACQUIRE, /**< Hand a channel to an external consumer; arg = uint8_t * */
    BK_CAM_IOCTL_CHANNEL_RELEASE, /**< Return a channel to the camera thread; arg = uint8_t * */
    BK_CAM_IOCTL_FRAME_POP, /**< Dequeue a zero-copy frame; arg = bk_isp_camera_frame_info_t * */
    BK_CAM_IOCTL_FRAME_QBUF, /**< Re-queue a zero-copy frame; arg = bk_isp_camera_frame_info_t * */
    BK_CAM_IOCTL_SELECT_ISP_PORT, /**< Select logical ISP port; arg = uint8_t * */
    BK_CAM_IOCTL_GET_WB, /**< Get white balance attributes; arg = bk_isp_camera_wb_attr_t * */
    BK_CAM_IOCTL_SET_WB, /**< Set white balance attributes; arg = bk_isp_camera_wb_attr_t * */
    BK_CAM_IOCTL_GET_EXPOSURE, /**< Get exposure mode and manual values; arg = bk_isp_camera_exposure_attr_t * */
    BK_CAM_IOCTL_SET_EXPOSURE, /**< Set exposure mode and manual values; arg = bk_isp_camera_exposure_attr_t * */
    BK_CAM_IOCTL_FRAME_PORT_GET, /**< Query logical port for a frame; arg = bk_isp_camera_frame_port_query_t * */
    BK_CAM_IOCTL_REGISTER_ISR_CALLBACK, /**< Register ISP callback; arg = bk_isp_camera_isr_callback_config_t * */
    BK_CAM_IOCTL_DEREGISTER_ISR_CALLBACK, /**< Deregister ISP callback; arg = bk_isp_camera_isr_callback_config_t * */
    BK_CAM_IOCTL_REGISTER_PIPELINE_START_CALLBACK, /**< Register pipeline callback; arg = bk_isp_camera_pipeline_callback_config_t * */
    BK_CAM_IOCTL_DEREGISTER_PIPELINE_START_CALLBACK, /**< Deregister pipeline callback; arg = bk_isp_camera_pipeline_callback_config_t * */
    BK_CAM_IOCTL_REGISTER_PIPELINE_DONE_CALLBACK, /**< Register pipeline callback; arg = bk_isp_camera_pipeline_callback_config_t * */
    BK_CAM_IOCTL_DEREGISTER_PIPELINE_DONE_CALLBACK, /**< Deregister pipeline callback; arg = bk_isp_camera_pipeline_callback_config_t * */
    BK_CAM_IOCTL_IS_IN_VBLANK, /**< Query ISP blanking state; arg = bk_isp_camera_vblank_state_t * */
    BK_CAM_IOCTL_SET_SENSOR_UPDATE_MODE, /**< Set Sensor register update timing; arg = bk_isp_camera_sensor_update_mode_t * */
    BK_CAM_IOCTL_GET_GAMMA_OUT, /**< Get gamma-out attributes; arg = bk_isp_camera_gamma_out_attr_t * */
    BK_CAM_IOCTL_SET_GAMMA_OUT, /**< Set gamma-out attributes; arg = bk_isp_camera_gamma_out_attr_t * */
    BK_CAM_IOCTL_GET_FLEXA_BUF_SIZE, /**< Query flexa ring buffer size for a channel geometry; arg = bk_isp_camera_flexa_buf_size_t * */
    BK_CAM_IOCTL_SET_FLEXA_BUF, /**< Set an externally allocated flexa ring buffer before channel open; arg = bk_isp_camera_flexa_buf_t * */
    BK_CAM_IOCTL_PIPELINE_SWITCH_ENABLE, /**< Enable pipeline configuration update; arg = uint8_t * */
} bk_cam_interface_ioctl_t;

/**
 * @brief Skip-frames config for BK_CAM_IOCTL_SET_SKIP_FRAMES.
 *
 * Allowed only when the target channel is not open yet.
 */
typedef struct
{
    uint8_t channel;   /**< ISP channel id (ISP_MP_CHN_ID / ISP_SP_CHN_ID) */
    uint8_t count;     /**< Drop first N frames after channel open (AE warmup), 0 = disabled */
} bk_isp_camera_skip_frames_config_t;

/**
 * @brief Flexa ring buffer size query for BK_CAM_IOCTL_GET_FLEXA_BUF_SIZE.
 *
 * Caller fills buf_cnt/width with the same values that will be passed to
 * channel_open; the controller returns the required buffer size in bytes.
 */
typedef struct
{
    uint8_t buf_cnt;   /**< Flexa ring slice count planned for channel open */
    uint16_t width;    /**< Channel width in pixels planned for channel open */
    uint32_t size;     /**< Output: required flexa ring buffer size in bytes */
} bk_isp_camera_flexa_buf_size_t;

/**
 * @brief Flexa ring buffer release callback.
 *
 * Invoked when the controller drops an externally provided flexa buffer
 * (camera ctlr deinit, or the buffer is replaced by a new SET_FLEXA_BUF).
 */
typedef void (*bk_isp_camera_flexa_buf_release_cb_t)(void *buffer, void *user_data);

/**
 * @brief External flexa ring buffer for BK_CAM_IOCTL_SET_FLEXA_BUF.
 *
 * Hands an externally allocated buffer to the controller before channel open.
 * The controller then uses it instead of allocating internally, and never
 * frees it (the caller keeps ownership).
 */
typedef struct
{
    uint8_t channel;   /**< ISP channel id (ISP_MP_CHN_ID / ISP_SP_CHN_ID) */
    void *buffer;      /**< Externally allocated flexa ring buffer */
    uint32_t size;     /**< Buffer size in bytes (from BK_CAM_IOCTL_GET_FLEXA_BUF_SIZE) */
    /** Optional release callback: fired when the controller drops the buffer
     *  (ctlr deinit / replaced by a new SET). NULL = caller keeps ownership. */
    bk_isp_camera_flexa_buf_release_cb_t release_cb;
    void *user_data;   /**< Context passed back to release_cb */
} bk_isp_camera_flexa_buf_t;

typedef struct
{
    uint8_t channel;
    uint32_t sequence;
    uint8_t port_id;
} bk_isp_camera_frame_port_query_t;

typedef struct
{
    bk_camera_isr_type_t type;
    bk_camera_isr_t callback;
    void *arg;
} bk_isp_camera_isr_callback_config_t;

typedef void (*bk_isp_camera_pipeline_done_cb_t)(
    uint8_t port_id, uint32_t sequence, void *arg);

typedef struct
{
    bk_isp_camera_pipeline_done_cb_t callback;
    void *arg;
} bk_isp_camera_pipeline_callback_config_t;

typedef struct
{
    uint8_t in_vblank;
} bk_isp_camera_vblank_state_t;

typedef enum
{
    BK_ISP_CAMERA_SENSOR_UPDATE_IN_PIPELINE = 0U,
    BK_ISP_CAMERA_SENSOR_UPDATE_AT_FRAME_IN = 1U,
} bk_isp_camera_sensor_update_mode_t;

typedef struct
{
    uint8_t channel;
    uint8_t discard_frames;
    uint32_t frame_size;
    uint16_t width;
    uint16_t height;
    uint8_t runtime_vc_switch; /**< Use lightweight MIPI VC switch (TP2863 dual-VC) */
} bk_isp_camera_vc_mux_config_t;

typedef struct
{
    uint8_t vc;
    uint8_t *frame;
    uint32_t frame_size;
    uint32_t sequence;
} bk_isp_camera_vc_mux_frame_ref_t;

/** Number of strength nodes for CPROC auto attributes (matches ISP_AUTO_STRENGTH_NUN). */
#define BK_ISP_CPROC_AUTO_STRENGTH_NUM 16

/**
 * @brief CPROC manual attributes (public mirror of ISP_CPROC_MANUAL_ATTR_S).
 */
typedef struct
{
    int8_t brightness;   /**< Range: [-127, 127] */
    uint8_t contrast;    /**< Range: [0, 255] */
    uint8_t saturation;  /**< Range: [0, 255] */
    int8_t hue;          /**< Range: [-90, 90] */
} bk_isp_cproc_manual_attr_t;

/**
 * @brief CPROC auto attributes (public mirror of ISP_CPROC_AUTO_ATTR_S).
 */
typedef struct
{
    int8_t brightness[BK_ISP_CPROC_AUTO_STRENGTH_NUM];
    uint8_t contrast[BK_ISP_CPROC_AUTO_STRENGTH_NUM];
    uint8_t saturation[BK_ISP_CPROC_AUTO_STRENGTH_NUM];
    int8_t hue[BK_ISP_CPROC_AUTO_STRENGTH_NUM];
} bk_isp_cproc_auto_attr_t;

/**
 * @brief CPROC attributes for BK_CAM_IOCTL_GET/SET_CPROC.
 */
typedef struct
{
    uint8_t enable;                      /**< 0: disable, 1: enable */
    uint32_t op_type;                    /**< 0: auto, 1: manual */
    bk_isp_cproc_manual_attr_t manual;   /**< Manual attributes */
    bk_isp_cproc_auto_attr_t auto_attr;  /**< Auto attributes */
} bk_isp_cproc_attr_t;

/**
 * @brief Current or initial exposure information for camera AE IOCTLs.
 */
typedef bk_isp_exposure_info_t bk_isp_camera_exposure_info_t;

/**
 * @brief White balance attributes for BK_CAM_IOCTL_GET/SET_WB.
 */
typedef bk_isp_wb_attr_t bk_isp_camera_wb_attr_t;

/**
 * @brief Exposure attributes for BK_CAM_IOCTL_GET/SET_EXPOSURE.
 */
typedef bk_isp_exposure_attr_t bk_isp_camera_exposure_attr_t;

/**
 * @brief Gamma-out attributes for BK_CAM_IOCTL_GET/SET_GAMMA_OUT.
 *
 * 962 bytes, do not place one on a task stack. BK_CAM_IOCTL_SET_GAMMA_OUT
 * clamps out-of-range nodes in place, so the argument must be writable.
 */
typedef bk_isp_gamma_out_attr_t bk_isp_camera_gamma_out_attr_t;

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
 * @brief Zero-copy frame descriptor handed out by the ISP camera controller.
 *
 * Describes one ISP output frame still living in the ISP frame pool (no copy).
 * Pass it to BK_CAM_IOCTL_FRAME_POP with @ref channel and @ref timeout set.
 * On success the caller owns the buffer until it passes the same @ref channel
 * and @ref index to BK_CAM_IOCTL_FRAME_QBUF. Y plane starts at @ref frame_addr;
 * for semi-planar NV12 the chroma plane is contiguous at frame_addr +
 * width*height.
 */
typedef struct
{
    uint32_t frame_addr;   /**< Physical/user address of the frame (Y plane base), zero-copy. */
    uint32_t frame_size;   /**< Total frame size in bytes (all planes). */
    uint32_t timeout;      /**< FRAME_POP timeout in milliseconds; ignored by FRAME_QBUF. */
    uint8_t  channel;      /**< ISP channel id (ISP_MP_CHN_ID / ISP_SP_CHN_ID). */
    uint8_t  index;        /**< Frame-pool buffer index; pass back to BK_CAM_IOCTL_FRAME_QBUF. */
    uint8_t  port_id;      /**< Logical ISP port that produced this frame. */
} bk_isp_camera_frame_info_t;

typedef struct
{
    uint16_t id;
    uint8_t *frame;
    uint32_t size;
    uint32_t timeout;
} multi_port_read_param_t;

typedef struct
{
    uint32_t frame_size;
    uint8_t port_id;
} multi_port_read_result_t;

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
    avdk_err_t (*port_select)(bk_camera_ctlr_t *controller, uint8_t port_id); /**< Select an initialized logical ISP port */
    avdk_err_t (*port_change)(bk_camera_ctlr_t *controller); /**< Switch the active port or update link configuration dynamically */
    avdk_err_t (*open)(bk_camera_ctlr_t *controller, void *parameter); /**< Open the controller for streaming with the specified parameters */
    avdk_err_t (*read)(bk_camera_ctlr_t *controller, uint16_t id, uint8_t *frame, uint32_t size, uint32_t timeout); /**< Read a frame */
    avdk_err_t (*multi_port_read)(bk_camera_ctlr_t *controller,
                                  const multi_port_read_param_t *param,
                                  multi_port_read_result_t *result); /**< Read a frame and return its logical ISP port */
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

/**
 * @brief ISP camera VC mux controller operation table
 */
typedef struct bk_camera_vc_mux_ctlr_t bk_camera_vc_mux_ctlr_t;

struct bk_camera_vc_mux_ctlr_t
{
    avdk_err_t (*start)(bk_camera_vc_mux_ctlr_t *controller, bk_isp_camera_vc_mux_config_t *config); /**< Start VC mux service */
    avdk_err_t (*stop)(bk_camera_vc_mux_ctlr_t *controller); /**< Stop VC mux service */
    avdk_err_t (*vc_enable)(bk_camera_vc_mux_ctlr_t *controller, uint8_t vc, uint8_t discard_frames); /**< Enable one VC route */
    avdk_err_t (*vc_disable)(bk_camera_vc_mux_ctlr_t *controller, uint8_t vc); /**< Disable one VC route */
    avdk_err_t (*peek)(bk_camera_vc_mux_ctlr_t *controller, bk_isp_camera_vc_mux_frame_ref_t *frame); /**< Get VC mux latest frame pointer */
    avdk_err_t (*release)(bk_camera_vc_mux_ctlr_t *controller, bk_isp_camera_vc_mux_frame_ref_t *frame); /**< Release VC mux frame pointer */
    avdk_err_t (*del)(bk_camera_vc_mux_ctlr_t *controller); /**< Destroy the VC mux controller object and free memory */
};

typedef struct bk_camera_vc_mux_ctlr_t *bk_isp_camera_vc_mux_handle_t;


#ifdef __cplusplus
}
#endif

