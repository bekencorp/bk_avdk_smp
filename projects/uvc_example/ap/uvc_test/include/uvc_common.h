#ifndef __UVC_TEST_H__
#define __UVC_TEST_H__

#include <components/uvc_camera_types.h>
#include <components/usbh_hub_multiple_classes_api.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_camera_ctlr.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief UVC test information structure
 * 
 * This structure contains all the necessary information for UVC device testing,
 * including synchronization primitives, port information, and device handles.
 */
typedef struct
{
    beken_semaphore_t uvc_connect_semaphore;    ///< Semaphore for UVC connection synchronization
    beken_mutex_t uvc_mutex;                    ///< Mutex for protecting shared resources
    bk_usb_hub_port_info *port_info[UVC_PORT_MAX];  ///< Array of USB hub port information
    bk_camera_ctlr_handle_t handle[UVC_PORT_MAX];   ///< Array of camera controller handles
} uvc_test_info_t;

/**
 * @brief Checkout port information for YUV format
 * 
 * This function checks out the port information for YUV format.
 * 
 * @param info Pointer to the UVC test information structure
 * @param user_config Pointer to the UVC camera configuration structure
 * @return avdk_err_t AVDK error code
 */
avdk_err_t uvc_checkout_port_info_yuv(uvc_test_info_t *info, bk_cam_uvc_config_t *user_config);

/**
 * @brief Checkout port information for MJPEG format
 * 
 * This function checks out the port information for MJPEG format.
 * 
 * @param info Pointer to the UVC test information structure
 * @param user_config Pointer to the UVC camera configuration structure
 * @return avdk_err_t AVDK error code
 */
avdk_err_t uvc_checkout_port_info_mjpeg(uvc_test_info_t *info, bk_cam_uvc_config_t *user_config);

/**
 * @brief Checkout port information for H264 format
 * 
 * This function checks out the port information for H264 format.
 * 
 * @param info Pointer to the UVC test information structure
 * @param user_config Pointer to the UVC camera configuration structure
 * @return avdk_err_t AVDK error code
 */
avdk_err_t uvc_checkout_port_info_h264(uvc_test_info_t *info, bk_cam_uvc_config_t *user_config);

/**
 * @brief Checkout port information for H265 format
 * 
 * This function checks out the port information for H265 format.
 * 
 * @param info Pointer to the UVC test information structure
 * @param user_config Pointer to the UVC camera configuration structure
 * @return avdk_err_t AVDK error code
 */
avdk_err_t uvc_checkout_port_info_h265(uvc_test_info_t *info, bk_cam_uvc_config_t *user_config);

/**
 * @brief Power on the UVC camera device
 * 
 * This function powers on the UVC camera device.
 * 
 * @param info Pointer to the UVC test information structure
 * @param timeout Timeout value in milliseconds
 * @return avdk_err_t AVDK error code
 */
avdk_err_t uvc_camera_power_on_handle(uvc_test_info_t *info, uint32_t timeout);

/**
 * @brief Power off the UVC camera device
 * 
 * This function powers off the UVC camera device.
 * 
 * @param info Pointer to the UVC test information structure
 * @return avdk_err_t AVDK error code
 */
avdk_err_t uvc_camera_power_off_handle(uvc_test_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* __UVC_TEST_H__ */