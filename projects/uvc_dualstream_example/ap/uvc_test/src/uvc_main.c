#include <components/avdk_utils/avdk_error.h>
#include <components/usbh_hub_multiple_classes_api.h>
#include <components/bk_camera_ctlr.h>
#include "frame_buffer.h"
#include "media_utils.h"
#include "uvc_cli.h"
#include "uvc_common.h"
#include "cli.h"

#define TAG "uvc_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static uvc_test_info_t *g_uvc_main_stream_info = NULL;
static uvc_test_info_t *g_uvc_sub_stream_info  = NULL;
static uvc_test_info_t *g_uvc_test_info[2] = {NULL, NULL};

const int32_t mjpeg_idx = USB_UVC_DEVICE;
const int32_t h26x_idx  = USB_UVC_H26X_DEVICE;

bool cmd_contain(int argc, char **argv, char *string)
{
    int i;
    bool ret = false;

    for (i = 0; i < argc; i++)
    {
        if (os_strcmp(argv[i], string) == 0)
        {
            ret = true;
        }
    }

    return ret;
}

static void uvc_api_cmd_help(void)
{
    LOGD("*************support commands list:********************\n");
    LOGD("uvc_api new port width height img_format\n");
    LOGD("- port: uvc port number, default is 1, range [1- %d]\n", UVC_PORT_MAX);
    LOGD("- width: image width, default is 864\n");
    LOGD("- height: image height, default is 480\n");
    LOGD("- img_format: image format, default is MJPEG, support MJPEG, H264, H265, YUV\n");
    LOGD("uvc_api delete port\n");
    LOGD("uvc_api open port\n");
    LOGD("uvc_api close port\n");
    LOGD("uvc_api suspend port\n");
    LOGD("uvc_api resume port\n");
}

static void uvc_func_cmd_help(void)
{
    LOGD("*************support commands list:********************\n");
    LOGD("uvc open_single port width height img_format\n");
    LOGD("- port: uvc port number, default is 1, range [1- %d]\n", UVC_PORT_MAX);
    LOGD("- width: image width, default is 864\n");
    LOGD("- height: image height, default is 480\n");
    LOGD("- img_format: image format, default is MJPEG, support MJPEG, H264, H265, YUV\n");
    LOGD("uvc open_dual port [options]\n");
    LOGD("- port: uvc port number, default is 1, range [1- %d]\n", UVC_PORT_MAX);
    LOGD("- options: optional parameters for stream configuration\n");
    LOGD("  Examples:\n");
    LOGD("    uvc open_dual 1 H264 1280 720\n");
    LOGD("    uvc open_dual 1 H265 1280 720\n");
    LOGD("    uvc open_dual 1 H264 1280 720 MJPEG 640 480\n");
    LOGD("    uvc open_dual 1 H265 1280 720 MJPEG 640 480\n");
    LOGD("uvc close port [stream_type]\n");
    LOGD("- port: uvc port number\n");
    LOGD("- stream_type: optional, can be 'H26X' or 'MJPEG' to close specific stream\n");
}


static avdk_err_t uvc_test_info_init_single(uvc_test_info_t **info, const char *name)
{
    if (*info == NULL)
    {
        *info = os_malloc(sizeof(uvc_test_info_t));
        if (*info == NULL)
        {
            LOGE("%s: malloc %s failed\n", __func__, name);
            return AVDK_ERR_NOMEM;
        }
        os_memset(*info, 0, sizeof(uvc_test_info_t));

        if (rtos_init_semaphore(&(*info)->uvc_connect_semaphore, 1) != AVDK_ERR_OK)
        {
            LOGE("%s: init semaphore failed\n", __func__);
            os_free(*info);
            *info = NULL;
            return AVDK_ERR_NOMEM;
        }

        if (rtos_init_mutex(&(*info)->uvc_mutex) != AVDK_ERR_OK)
        {
            LOGE("%s: init mutex failed\n", __func__);
            rtos_deinit_semaphore(&(*info)->uvc_connect_semaphore);
            os_free(*info);
            *info = NULL;
            return AVDK_ERR_NOMEM;
        }
    }
    return AVDK_ERR_OK;
}

static avdk_err_t uvc_test_info_init(void)
{
    avdk_err_t ret;

    // Initialize the main stream information
    ret = uvc_test_info_init_single(&g_uvc_main_stream_info, "g_uvc_main_stream_info");
    if (ret != AVDK_ERR_OK) 
    {
        return ret;
    }

    // Initialize the sub stream information
    ret = uvc_test_info_init_single(&g_uvc_sub_stream_info, "g_uvc_sub_stream_info");
    if (ret != AVDK_ERR_OK)
    {
        // Clean up the resources allocated on error
        if (g_uvc_main_stream_info != NULL)
        {
            rtos_deinit_mutex(&g_uvc_main_stream_info->uvc_mutex);
            rtos_deinit_semaphore(&g_uvc_main_stream_info->uvc_connect_semaphore);
            os_free(g_uvc_main_stream_info);
            g_uvc_main_stream_info = NULL;
        }
        return ret;
    }

    // Update the global array references
    g_uvc_test_info[0] = g_uvc_main_stream_info;
    g_uvc_test_info[1] = g_uvc_sub_stream_info;

    return AVDK_ERR_OK;
}

static avdk_err_t uvc_test_info_deinit_single(uvc_test_info_t **info)
{
    if (*info)
    {
        // Check if all ports are closed
        for (int i = 0; i < UVC_PORT_MAX; i++)
        {
            if ((*info)->port_info[i])
            {
                LOGE("%s, %d, port:%d not closed\n", __func__, __LINE__, i + 1);
                return AVDK_ERR_INVAL;
            }
        }

        // Release the resources
        rtos_deinit_mutex(&(*info)->uvc_mutex);
        rtos_deinit_semaphore(&(*info)->uvc_connect_semaphore);
        os_free(*info);
        *info = NULL;
    }
    return AVDK_ERR_OK;
}

static avdk_err_t uvc_test_info_deinit(void)
{
    avdk_err_t ret;

    // Release the main stream information
    ret = uvc_test_info_deinit_single(&g_uvc_main_stream_info);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    // Release the sub stream information
    ret = uvc_test_info_deinit_single(&g_uvc_sub_stream_info);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    // Clear the global array references
    g_uvc_test_info[0] = NULL;
    g_uvc_test_info[1] = NULL;

    return AVDK_ERR_OK;
}


static avdk_err_t uvc_checkout_user_info(uvc_test_info_t *info, bk_cam_uvc_config_t *user_config)
{
    if (info == NULL)
    {
        LOGE("%s: info is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    if (user_config == NULL)
    {
        LOGE("%s: config is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // No need to lock in the main function, as each format processing function handles locking internally
    LOGD("%s, %d, port:%d, format:%d, W*H:%d*%d\r\n", __func__, __LINE__, user_config->port, user_config->img_format,
        user_config->width, user_config->height);

    avdk_err_t ret = AVDK_ERR_OK;
    switch (user_config->img_format)
    {
        case IMAGE_YUV:
            ret = uvc_checkout_port_info_yuv(info, user_config);
            break;

        case IMAGE_MJPEG:
            ret = uvc_checkout_port_info_mjpeg(info, user_config);
            break;

        case IMAGE_H264:
            ret = uvc_checkout_port_info_h264(info, user_config);
            break;

        case IMAGE_H265:
            ret = uvc_checkout_port_info_h265(info, user_config);
            break;

        default:
            LOGE("%s, please check usb output format:%d\r\n", __func__, user_config->img_format);
            ret = AVDK_ERR_UNSUPPORTED;
            break;
    }

    return ret;
}


static frame_buffer_t *uvc_frame_malloc(image_format_t format, uint32_t size)
{
    frame_buffer_t *frame = NULL;
    if (format == IMAGE_YUV)
    {
        frame = frame_buffer_display_malloc(size);
    }
    else
    {
        frame = frame_buffer_encode_malloc(size);
    }

    if (frame)
    {
        frame->sequence = 0;
        frame->length = 0;
        frame->timestamp = 0;
        frame->size = size;
    }

    return frame;
}

static void uvc_frame_complete(uint8_t port, image_format_t format, frame_buffer_t *frame, int result)
{
    if (frame->sequence % 30 == 0)
    {
        LOGD("%s: port:%d, seq:%d, length:%d, format:%s, h264_type:%x, result:%d\n", __func__, port, frame->sequence,
            frame->length, (format == IMAGE_MJPEG) ? "mjpeg" : "h264", frame->h264_type, result);
    }

    if (format == IMAGE_YUV)
    {
        frame_buffer_display_free(frame);
    }
    else
    {
        frame_buffer_encode_free(frame);
    }
}

static void uvc_event_callback(bk_usb_hub_port_info *port_info,void *arg, uvc_error_code_t code)
{
    LOGD("uvc_event_callback: %d", code);
}

static const bk_uvc_callback_t uvc_cbs = {
    .malloc = uvc_frame_malloc,
    .complete = uvc_frame_complete,
    .uvc_event_callback = uvc_event_callback,
};

static avdk_err_t uvc_open_handle(uvc_test_info_t *info, bk_uvc_ctlr_config_t *uvc_ctlr_config)
{
    if (info == NULL)
    {
        LOGE("%s: info is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    uint8_t port = uvc_ctlr_config->config.port;

    // Check if the resolution input by the user is supported, can be selectively enabled
    avdk_err_t ret = uvc_checkout_user_info(info, &uvc_ctlr_config->config);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: uvc_checkout_user_info failed\n", __func__, __LINE__);
        return ret;
    }

    ret = bk_camera_uvc_ctlr_new(&info->handle[port - 1], uvc_ctlr_config);
    if (ret == BK_OK)
    {
        ret = bk_camera_open(info->handle[port - 1]);
        if (ret != BK_OK)
        {
            LOGE("%s, %d: bk_camera_open failed, ret:%d\n", __func__, __LINE__, ret);
            bk_camera_delete(info->handle[port - 1]);
            info->handle[port - 1] = NULL;
        }
        else
        {
            LOGD("%s open successful\n", __func__);
        }
    }
    else
    {
        LOGE("%s, %d: bk_camera_uvc_ctlr_new failed, ret:%d\n", __func__, __LINE__, ret);
    }

    return ret;
}

static avdk_err_t uvc_close_handle(uvc_test_info_t *info, uint8_t port)
{
    avdk_err_t ret = AVDK_ERR_OK;
    if (info == NULL)
    {
        LOGE("%s: info is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    if (info->handle[port - 1] != NULL)
    {
        ret = bk_camera_close(info->handle[port - 1]);
        if (ret == BK_OK)
        {
            bk_camera_delete(info->handle[port - 1]);
            info->handle[port - 1] = NULL;
        }
        else
        {
            LOGE("%s, %d: bk_camera_close failed, ret:%d\n", __func__, __LINE__, ret);
        }
    }

    return ret;
}

avdk_err_t uvc_open_stream_helper(uvc_test_info_t **stream_info, int stream_index, uint8_t port, uint16_t img_format, uint32_t width, uint32_t height)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_uvc_ctlr_config_t uvc_ctlr_config;

    if (port > UVC_PORT_MAX || port == 0) {
        LOGE("Invalid port %d\n", port);
        return AVDK_ERR_INVAL;
    }

    // Check if the stream is already opened
    if (stream_info[stream_index] != NULL && stream_info[stream_index]->handle[port - 1] != NULL) {
        LOGD("The stream already opened on port %d\n", port);
        return AVDK_ERR_BUSY;
    }

    // Initialize the stream_info
    if (img_format == IMAGE_MJPEG) {
        uvc_ctlr_config = (bk_uvc_ctlr_config_t){
            .config = BK_UVC_864X480_30FPS_MJPEG_CONFIG(),
            .cbs = &uvc_cbs,
        };
    } else if (img_format == IMAGE_H264 || img_format == IMAGE_H265) {
        uvc_ctlr_config = (bk_uvc_ctlr_config_t){
            .config = BK_UVC_1920X1080_30FPS_H26X_CONFIG(),
            .cbs = &uvc_cbs,
        };
    } else {
        LOGE("Unsupported image format %d\n", img_format);
        return AVDK_ERR_UNSUPPORTED;
    }

    uvc_ctlr_config.config.img_format = img_format;
    uvc_ctlr_config.config.port       = port;
    uvc_ctlr_config.config.width      = width;
    uvc_ctlr_config.config.height     = height;
    uvc_ctlr_config.config.user_data  = stream_info[stream_index];

    // Open UVC stream
    LOGD("Opening UVC port:%d, img_format:%d, %dx%d\n", 
         port, img_format, width, height);

    ret = uvc_open_handle(stream_info[stream_index], &uvc_ctlr_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("Failed to open UVC handle, ret:%d\n", ret);
    }

    return ret;
}

avdk_err_t uvc_close_stream_helper(uvc_test_info_t **stream_info, uint8_t port)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (port > UVC_PORT_MAX || port == 0) {
        LOGE("Invalid port %d\n", port);
        return AVDK_ERR_INVAL;
    }

    // Close all available stream types
    for (int i = 0; i < 2; i++) {
        if (stream_info[i] && stream_info[i]->handle[port - 1] != NULL) {
            LOGD("Closing UVC port:%d, stream index:%d\n", port, i);
            ret = uvc_close_handle(stream_info[i], port);
            if (ret != AVDK_ERR_OK) {
                LOGE("Failed to close UVC handle, ret:%d\n", ret);
            }
        }
    }

    return ret;
}

void cli_uvc_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_OK;
    char *msg = NULL;
    uint16_t output_format = IMAGE_MJPEG;
    uint8_t port = 1;
    bool power_on_flag = false;

    if (argc < 2) {
        LOGE("Missing command. Please specify a valid command.\n");
        uvc_func_cmd_help();
        ret = AVDK_ERR_INVAL;
        goto exit;
    }

    if (pcWriteBuffer == NULL || xWriteBufferLen <= 0) {
        LOGE("Invalid command buffer\n");
        ret = AVDK_ERR_INVAL;
        goto exit;
    }
    if (strcmp(argv[1], "open_single") == 0) {
        if (strcmp(argv[3], "MJPEG") == 0) {
            output_format = IMAGE_MJPEG;
        } else if (strcmp(argv[3], "H264") == 0) {
            output_format = IMAGE_H264;
        } else if (strcmp(argv[3], "H265") == 0) {
            output_format = IMAGE_H265;
        }

        if (argc < 5) {
            LOGE("Insufficient arguments for 'open_single' command\n");
            uvc_func_cmd_help();
            goto exit;
        }

        port = os_strtoul(argv[2], NULL, 10);
        uint32_t width = os_strtoul(argv[4], NULL, 10);
        uint32_t height = os_strtoul(argv[5], NULL, 10);

        int stream_index = (output_format == IMAGE_MJPEG) ? mjpeg_idx : h26x_idx;

        LOGD("Powering on USB port with delay 4000ms\n");
        ret = uvc_camera_power_on_handle(g_uvc_test_info, 4000);
        if (ret != AVDK_ERR_OK) {
            LOGE("Failed to power on camera\n");
            goto exit;
        }
        power_on_flag = true;  // Mark power on

        ret = uvc_open_stream_helper(g_uvc_test_info, stream_index, 
                                    port, output_format, width, height);
        if (ret != AVDK_ERR_OK) {
            LOGE("Failed to open %s stream\n", (output_format == IMAGE_MJPEG) ? "MJPEG" : "H26X");
            goto exit;
        }
    }
    else if (strcmp(argv[1], "open_dual") == 0) {
        if (argc < 3) {
            LOGE("Insufficient arguments for 'open_dual' command\n");
            uvc_func_cmd_help();
            goto exit;
        }

        port = os_strtoul(argv[2], NULL, 10);

        LOGD("Powering on USB port with delay 4000ms\n");
        ret = uvc_camera_power_on_handle(g_uvc_test_info, 4000);
        if (ret != AVDK_ERR_OK) {
            LOGE("Failed to power on camera\n");
            goto exit;
        }
        power_on_flag = true;  // Mark power on

        // Processing logic supporting multiple parameter combinations
        // 1. Only open MJPEG stream (main stream)
        if (argc >= 6 && strcmp(argv[3], "MJPEG") == 0) {
            uint32_t mjpeg_width = os_strtoul(argv[4], NULL, 10);
            uint32_t mjpeg_height = os_strtoul(argv[5], NULL, 10);

            LOGD("Opening MJPEG stream (main stream)\n");

            ret = uvc_open_stream_helper(g_uvc_test_info, mjpeg_idx, 
                                        port, IMAGE_MJPEG, mjpeg_width, mjpeg_height);
            if (ret != AVDK_ERR_OK) {
                LOGE("Failed to open MJPEG stream\n");
                goto exit;
            }

            // 2. Open both MJPEG and H26X streams simultaneously
            if (argc >= 9 && (strcmp(argv[6], "H264") == 0 || strcmp(argv[6], "H265") == 0)) {
                uint16_t h26x_format = (strcmp(argv[6], "H265") == 0) ? IMAGE_H265 : IMAGE_H264;
                uint32_t h26x_width = os_strtoul(argv[7], NULL, 10);
                uint32_t h26x_height = os_strtoul(argv[8], NULL, 10);

                ret = uvc_open_stream_helper(g_uvc_test_info, h26x_idx, 
                                            port, h26x_format, h26x_width, h26x_height);
                if (ret != AVDK_ERR_OK) {
                    LOGE("Failed to open %s stream\n", (h26x_format == IMAGE_H265) ? "H265" : "H264");
                    // If the H26X stream opening fails, try to close already opened MJPEG stream
                    uvc_close_stream_helper(g_uvc_test_info, port);
                    goto exit;
                }
            }
        }
        // No valid stream type specified
        else {
            LOGE("Invalid stream type. Supported types: MJPEG, H264, H265\n");
            goto exit;
        }
        LOGI("Dual stream operation completed\n");
    }
    else if (strcmp(argv[1], "close") == 0) {
        bool all_closed = true;
        int stream_type = -1;  // -1: all streams, 0: MJPEG, 1: H26X

        if (argc < 4) {
            LOGE("Insufficient arguments for 'close' command\n");
            uvc_func_cmd_help();
            goto exit;
        }
        port = os_strtoul(argv[2], NULL, 10);
        if (port > UVC_PORT_MAX || port == 0) {
            LOGE("[ERROR] Invalid port number %d (must be 1-%d)\n", port, UVC_PORT_MAX);
            goto exit;
        }
        // Parse command arguments
        if (strcmp(argv[3], "all") == 0) {
            // Format: uvc close [port] all
            LOGI("Closing all UVC streams on port %d\n", port);
            stream_type = -1;  // All stream types
        } else {
            // Format: uvc close [port] [MJPEG|H26X]
            // Check if stream type is specified
            if (strcmp(argv[3], "mjpeg") == 0 || strcmp(argv[3], "MJPEG") == 0) {
                stream_type = mjpeg_idx;  // MJPEG stream
                LOGI("Closing MJPEG stream on port %d\n", port);
            } else if (strcmp(argv[3], "h26x") == 0 || strcmp(argv[3], "H26X") == 0) {
                stream_type = h26x_idx;  // H26X stream
                LOGI("Closing H26X stream on port %d\n", port);
            } else {
                LOGE("[ERROR] Invalid stream type: %s. Supported types: MJPEG, H26X\n", argv[3]);
                goto exit;
            }
        }

        // Check if there are any open streams on the specified port
        if (stream_type == -1) {
            // Check all stream types
            for (int i = 0; i < 2; i++) {  // Check both stream types
                if (g_uvc_test_info[i] && g_uvc_test_info[i]->handle[port - 1] != NULL) {
                    all_closed = false;
                    break;
                }
            }
        } else {
            // Check specific stream type
            if (g_uvc_test_info[stream_type] && g_uvc_test_info[stream_type]->handle[port - 1] != NULL) {
                all_closed = false;
            }
        }

        if (all_closed) {
            if (stream_type == -1) {
                LOGD("UVC port %d already closed\n", port);
            } else {
                LOGD("UVC port %d %s stream already closed\n", port, 
                     stream_type == mjpeg_idx ? "MJPEG" : "H26X");
            }
            goto exit;
        }

        // Close the specified streams
        if (stream_type == -1) {
            // Close all streams on the specified port
            LOGI("Closing UVC port:%d (all streams)\n", port);
            ret = uvc_close_stream_helper(g_uvc_test_info, port);
            if (ret != AVDK_ERR_OK) {
                LOGE("[ERROR] Failed to close UVC port %d\n", port);
                goto exit;
            }

            // Check if there are other open streams
            bool any_open = false;
            for (uint8_t i = 1; i <= UVC_PORT_MAX; i++) {
                for (int j = 0; j < 2; j++) {
                    if (g_uvc_test_info[j] && g_uvc_test_info[j]->handle[i - 1] != NULL) {
                    any_open = true;
                    LOGI("UVC port %d %s stream still open\n", i, 
                         j == mjpeg_idx ? "MJPEG" : "H26X");
                    break;
                }
                }
                if (any_open) {
                    break;
                }
            }

            // If no other streams are open, 
            if (!any_open) {
                LOGI("Powering off USB port\n");
                ret = uvc_camera_power_off_handle(g_uvc_test_info);
                power_on_flag = false;  // Mark power off
            }
        } else {
            // Close only the specific stream type
            const char *stream_type_str = (stream_type == mjpeg_idx) ? "MJPEG" : "H26X";
            LOGI("Closing UVC port:%d %s stream\n", port, stream_type_str);

            if (g_uvc_test_info[stream_type] && g_uvc_test_info[stream_type]->handle[port - 1] != NULL) {
                ret = bk_camera_close(g_uvc_test_info[stream_type]->handle[port - 1]);
                if (ret != AVDK_ERR_OK) {
                    LOGE("[ERROR] Failed to close %s stream on port %d\n", stream_type_str, port);
                    goto exit;
                }

                // Release resources
                ret = bk_camera_delete(g_uvc_test_info[stream_type]->handle[port - 1]);
                if (ret != AVDK_ERR_OK) {
                    LOGE("[ERROR] Failed to delete %s stream handle on port %d\n", stream_type_str, port);
                    // Continue anyway to avoid resource leak
                }
                g_uvc_test_info[stream_type]->handle[port - 1] = NULL;
                LOGI("Successfully closed %s stream on port %d\n", stream_type_str, port);
            } else {
                LOGE("[ERROR] No active %s stream found on port %d\n", stream_type_str, port);
                goto exit;
            }

            // After closing a specific stream, check if all streams on this port are closed
            bool port_all_closed = true;
            for (int j = 0; j < 2; j++) {
                if (g_uvc_test_info[j] && g_uvc_test_info[j]->handle[port - 1] != NULL) {
                    port_all_closed = false;
                    break;
                }
            }

            // If all streams on this port are closed, check if there are any other open streams
            if (port_all_closed) {
                LOGI("All streams on port %d are now closed\n", port);
                bool any_open = false;
                for (uint8_t i = 1; i <= UVC_PORT_MAX; i++) {
                    for (int j = 0; j < 2; j++) {
                        if (g_uvc_test_info[j] && g_uvc_test_info[j]->handle[i - 1] != NULL) {
                            any_open = true;
                            break;
                        }
                    }
                    if (any_open) break;
                }

                // If no other streams are open, power off
                if (!any_open) {
                    LOGI("Powering off USB port\n");
                    ret = uvc_camera_power_off_handle(g_uvc_test_info);
                    power_on_flag = false;  // Mark power off
                }
            }

            // Check if there are other streams open, if not, power off
            bool any_stream_open = false;

            // Check all ports and all stream types
            for (uint8_t i = 1; i <= UVC_PORT_MAX; i++) {
                // Check if there are any open streams
                for (int j = 0; j < 2; j++) {  // Check both stream types
                    if (g_uvc_test_info[j] && g_uvc_test_info[j]->handle[i - 1] != NULL) {
                        any_stream_open = true;
                        break;
                    }
                }
                if (any_stream_open) break;
            }

            // If no other streams are open, power off
            if (!any_stream_open) {
                LOGD("No streams open, powering off USB port\n");
                ret = uvc_camera_power_off_handle(g_uvc_test_info);
                power_on_flag = false;  // Mark power off
            } else {
                LOGD("Other streams still open, keeping power on\n");
            }
        }
    }
    else
    {
        LOGE("%s, %d: unknown command %s\n", __func__, __LINE__, argv[1]);
        uvc_func_cmd_help();
    }

exit:
    if (ret != AVDK_ERR_OK) {
        msg = CLI_CMD_RSP_ERROR;
        LOGI("Command failed with error code: %d\n", ret);

        // Ensure power off
        if (power_on_flag) {
            LOGD("Cleaning up resources: powering off USB port\n");
            uvc_camera_power_off_handle(g_uvc_test_info);
            power_on_flag = false;
        }
    } else {
        msg = CLI_CMD_RSP_SUCCEED;
        LOGI("Command executed successfully\n");
    }

    LOGI("%s ---complete\n", __func__);

    if (msg && pcWriteBuffer && xWriteBufferLen > 0) {
        size_t msg_len = os_strlen(msg);
        if (msg_len < (size_t)xWriteBufferLen) {
            os_memcpy(pcWriteBuffer, msg, msg_len);
            pcWriteBuffer[msg_len] = '\0';
        } else {
            os_memcpy(pcWriteBuffer, msg, xWriteBufferLen - 1);
            pcWriteBuffer[xWriteBufferLen - 1] = '\0';
            LOGE("Response message truncated\n");
        }
    }
}

#define CMDS_COUNT  (sizeof(s_uvc_test_commands) / sizeof(struct cli_command))
static const struct cli_command s_uvc_test_commands[] =
{
    {"uvc", " uvc open | close", cli_uvc_func_test_cmd},
};

int cli_uvc_test_init(void)
{
    int ret = uvc_test_info_init();
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: uvc_test_info_init failed\n", __func__, __LINE__);
        return ret;
    }
    return cli_register_commands(s_uvc_test_commands, CMDS_COUNT);
}