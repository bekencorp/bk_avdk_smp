/**
 * @file isp_dump_test.c
 * @brief ISP dump TCP server and CLI command for frame dump (start/stop ISP).
 *        CMD_START_ISP: start MIPI camera and ISP. CMD_RESET_ISP: stop MIPI camera and ISP (UI button "停止ISP").
 *        isp_ui_* interfaces are implemented in this file.
 */

#include <common/bk_include.h>
#include <os/os.h>
#include <os/str.h>

#include "lwip/sockets.h"
#include "lwip/inet.h"

#include <common/avdk_pixel_types.h>
#include <avdk_error.h>
#include "include/isp_cli.h"

#include <components/bk_camera_sensor.h>
#include <components/bk_camera_bus.h>
#include <components/bk_camera_configs.h>
#include <components/bk_isp_camera.h>
#include <components/bk_camera_isp_ctlr.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include <driver/isp_base.h>
#include <components/bk_frame_buffer.h>
#include <sys_types.h>
#include <modules/pm.h>

#define ISP_DUMP_TAG "isp-dump"
#define ISP_DUMP_LOGI(...) BK_LOGI(ISP_DUMP_TAG, ##__VA_ARGS__)
#define ISP_DUMP_LOGW(...) BK_LOGW(ISP_DUMP_TAG, ##__VA_ARGS__)
#define ISP_DUMP_LOGE(...) BK_LOGE(ISP_DUMP_TAG, ##__VA_ARGS__)

#define ISP_DUMP_SERVER_PORT          (554)
#define ISP_DUMP_LISTEN_BACKLOG      (1)
#define ISP_DUMP_RECV_BUF_LEN        (256)
#define ISP_DUMP_SERVER_STACK_SIZE   (4096)

#define ISP_DUMP_MAGIC_WORD          (0x778899AAU)
#define ISP_DUMP_CMD_START_ISP       (0U)
#define ISP_DUMP_CMD_SEND_FRAME      (1U)
#define ISP_DUMP_CMD_RESET_ISP       (2U)
#define ISP_DUMP_CMD_WORD_COUNT      (7U)
#define ISP_DUMP_CMD_PACKET_LEN      (ISP_DUMP_CMD_WORD_COUNT * sizeof(uint32_t))
#define ISP_DUMP_FMT_RAW10           (21U)
#define ISP_DUMP_FMT_NV12            (23U)
#define ISP_DUMP_FMT_RGB565          (36U)
#define ISP_DUMP_FMT_RGB888          (38U)
#define ISP_DUMP_FMT_TESTPATTERN     (40U)

#define ISP_DUMP_DEFAULT_FPS         (30)
#define ISP_DUMP_MP_CHNL_ID          (0)
#define ISP_DUMP_PORT_ID             (0)

/* Local camera/ISP handle for isp_ui_* (independent of isp_func_test.c) */
typedef struct {
    bk_camera_bus_t *bus;
    bk_camera_sensor_handle_t sensor_handle;
    bk_isp_camera_ctlr_handle_t camera_ctlr_handle;
    uint32_t mp_frame_size;
    uint8_t is_initialized;
    uint8_t is_sensor_started;
} isp_dump_camera_handle_t;

static isp_dump_camera_handle_t s_isp_dump_cam = {0};

/* ISP UI APIs declared in isp_cli.h; implemented below in this file. */

static beken_thread_t s_isp_dump_thread = NULL;
static int s_listen_fd = -1;
static volatile int s_isp_dump_stop = 0;

/**
 * @brief Send all data to socket, with error handling
 */
static int isp_dump_send_all(int sock, const uint8_t *data, uint32_t len)
{
    uint32_t sent_total = 0;
    int ret;

    if (data == NULL || len == 0)
    {
        return -1;
    }

    while (sent_total < len)
    {
        ret = send(sock, data + sent_total, len - sent_total, 0);
        if (ret <= 0)
        {
            return -1;
        }
        sent_total += (uint32_t)ret;
    }

    return 0;
}

static avdk_err_t camera_power_enable(bool enable)
{
    int ldo_en = enable ? PM_AUXLDO_ENABLE : PM_AUXLDO_DISABLE;
    ISP_DUMP_LOGI("%s, iovdd dvdd enable: %d\n", __func__, ldo_en);

    pm_auxldo_ctrl_cfg_t auxldo_cfg = {0};
    auxldo_cfg.ldo = AUXLDOS_SEL_1P8V;
    auxldo_cfg.out = PM_AUXLDO_1P8V_OUT_1P8V;
    auxldo_cfg.user = PM_AUXLDO_USER_CAMERA;
    auxldo_cfg.state = ldo_en;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&auxldo_cfg), TAG, "camera 1p8v ldo vote failed");

    auxldo_cfg = (pm_auxldo_ctrl_cfg_t){0};
    auxldo_cfg.ldo = AUXLDOS_SEL_1P2V;
    auxldo_cfg.out = PM_AUXLDO_1P2V_OUT_1P2V;
    auxldo_cfg.user = PM_AUXLDO_USER_CAMERA;
    auxldo_cfg.state = ldo_en;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&auxldo_cfg), TAG, "camera 1p2v ldo vote failed");
    rtos_delay_milliseconds(1);
    return AVDK_ERR_OK;
}

/**
 * @brief Read little-endian uint32 from buffer
 */
static uint32_t isp_dump_read_u32_le(const uint8_t *buf)
{
    return ((uint32_t)buf[0]) |
           (((uint32_t)buf[1]) << 8) |
           (((uint32_t)buf[2]) << 16) |
           (((uint32_t)buf[3]) << 24);
}

/**
 * @brief Map protocol pixel format to BK pixel format
 */
static bk_err_t isp_dump_map_fmt_to_bk(uint32_t proto_fmt, bk_pixel_format_t *out_bk_fmt)
{
    if (out_bk_fmt == NULL)
    {
        return BK_ERR_PARAM;
    }

    switch (proto_fmt)
    {
        case ISP_DUMP_FMT_RAW10:
            *out_bk_fmt = BK_PIXEL_FORMAT_RAW10;
            return BK_OK;
        case ISP_DUMP_FMT_NV12:
            *out_bk_fmt = BK_PIXEL_FORMAT_NV12;
            return BK_OK;
        case ISP_DUMP_FMT_RGB565:
            *out_bk_fmt = BK_PIXEL_FORMAT_RGB565;
            return BK_OK;
        case ISP_DUMP_FMT_RGB888:
            *out_bk_fmt = BK_PIXEL_FORMAT_RGB888;
            return BK_OK;
        case ISP_DUMP_FMT_TESTPATTERN:
            *out_bk_fmt = BK_PIXEL_FORMAT_NV12;
            return BK_OK;
        default:
            return BK_ERR_PARAM;
    }
}

/* Forward declarations for MIPI/ISP helpers used by isp_ui_* */
static avdk_err_t isp_dump_cleanup_all_resources(void);
static avdk_err_t isp_dump_close_mp_channel(void);

/**
 * @brief Check if sensor supports the requested resolution (for MIPI start)
 */
static avdk_err_t isp_dump_check_sensor_resolution(bk_camera_sensor_handle_t sensor_handle,
                                                    uint16_t width, uint16_t height, uint16_t fps)
{
    bk_camera_sensor_format_array_t format_array = {0};
    avdk_err_t ret;
    uint32_t i;
    int found = 0;

    if (sensor_handle == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    ret = bk_camera_sensor_query_support_formats(sensor_handle, &format_array);
    if (ret != AVDK_ERR_OK || format_array.size == 0)
    {
        return ret != AVDK_ERR_OK ? ret : AVDK_ERR_INVAL;
    }

    for (i = 0; i < format_array.size; i++)
    {
        if (format_array.format_array[i].width == width &&
            format_array.format_array[i].height == height &&
            format_array.format_array[i].fps == fps)
        {
            found = 1;
            break;
        }
    }

    return found ? AVDK_ERR_OK : AVDK_ERR_INVAL;
}

/**
 * @brief Initialize MIPI CSI camera (bus, sensor, controller, port) for dump pipeline
 */
static avdk_err_t isp_dump_init_mipi_camera(uint16_t width, uint16_t height, uint16_t fps)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_camera_bus_t *bus = NULL;
    bk_camera_bus_config_t bus_config = (bk_camera_bus_config_t)CSI_CAM_BUS_I2C1_8BIT_2000TIMEOUT();
    bk_camera_sensor_config_t sensor_config = {
        .pin_reset = GPIO_71,
        .pin_pwdn = 0xFF,
        .pin_xclk = GPIO_59,
    };

    bus = bk_camera_bus_new(&bus_config);
    if (bus == NULL)
    {
        ISP_DUMP_LOGE("Failed to create camera bus\n");
        return AVDK_ERR_GENERIC;
    }

    ret = bk_camera_bus_enable(bus);
    if (ret != AVDK_ERR_OK)
    {
        ISP_DUMP_LOGE("Failed to enable camera bus\n");
        bk_camera_bus_delete(bus);
        return ret;
    }

    sensor_config.bus = bus;
    s_isp_dump_cam.sensor_handle = bk_camera_sensor_auto_detect(&sensor_config, CSI_CAMERA_PORT);
    if (s_isp_dump_cam.sensor_handle == NULL)
    {
        ISP_DUMP_LOGE("No sensor detected on MIPI CSI port\n");
        bk_camera_bus_disable(bus);
        bk_camera_bus_delete(bus);
        return AVDK_ERR_NODEV;
    }

    ret = isp_dump_check_sensor_resolution(s_isp_dump_cam.sensor_handle, width, height, fps);
    if (ret != AVDK_ERR_OK)
    {
        ISP_DUMP_LOGE("Sensor does not support %dx%d @ %dfps\n", width, height, fps);
        bk_camera_sensor_destroy(s_isp_dump_cam.sensor_handle);
        s_isp_dump_cam.sensor_handle = NULL;
        bk_camera_bus_disable(bus);
        bk_camera_bus_delete(bus);
        return ret;
    }

    {
        bk_isp_camera_ctlr_config_t isp_ctlr_config = CAM_CSI_DEFAULT_RAW10_CONFIG(width, height, fps);
        const void *sensor_object = bk_camera_sensor_get_sensor_object(s_isp_dump_cam.sensor_handle);

        if (sensor_object == NULL)
        {
            ISP_DUMP_LOGE("Failed to get sensor object\n");
            bk_camera_sensor_destroy(s_isp_dump_cam.sensor_handle);
            s_isp_dump_cam.sensor_handle = NULL;
            bk_camera_bus_disable(bus);
            bk_camera_bus_delete(bus);
            return AVDK_ERR_GENERIC;
        }
        isp_ctlr_config.sensor_object = sensor_object;

        /* Pull the raw pixel format from the matched sensor mode (the macro no
         * longer hard-codes it); falls back to the first entry if no exact
         * (w,h,fps) match is found. */
        {
            bk_camera_sensor_format_array_t fmt_arr = {0};
            if (bk_camera_sensor_query_support_formats(s_isp_dump_cam.sensor_handle, &fmt_arr) == AVDK_ERR_OK
                && fmt_arr.size > 0)
            {
                uint32_t i;
                isp_ctlr_config.input_pixel_fmt = fmt_arr.format_array[0].output_pixel_fmt;
                for (i = 0; i < fmt_arr.size; i++)
                {
                    if (fmt_arr.format_array[i].width == width
                        && fmt_arr.format_array[i].height == height
                        && fmt_arr.format_array[i].fps == fps)
                    {
                        isp_ctlr_config.input_pixel_fmt = fmt_arr.format_array[i].output_pixel_fmt;
                        break;
                    }
                }
            }
        }

        ret = bk_camera_isp_ctlr_new(&s_isp_dump_cam.camera_ctlr_handle);
        if (ret != AVDK_ERR_OK)
        {
            ISP_DUMP_LOGE("Failed to create camera controller\n");
            goto err_bus;
        }

        ret = bk_isp_camera_dev_init(s_isp_dump_cam.camera_ctlr_handle);
        if (ret != AVDK_ERR_OK)
        {
            ISP_DUMP_LOGE("Failed to init camera device\n");
            goto err_ctlr;
        }

        ret = bk_isp_camera_port_init(s_isp_dump_cam.camera_ctlr_handle, &isp_ctlr_config);
        if (ret != AVDK_ERR_OK)
        {
            ISP_DUMP_LOGE("Failed to init camera port\n");
            goto err_ctlr;
        }
    }

    s_isp_dump_cam.bus = bus;
    s_isp_dump_cam.is_initialized = 1;
    ISP_DUMP_LOGI("MIPI CSI camera initialized: %dx%d @ %dfps\n", width, height, fps);
    return AVDK_ERR_OK;

err_ctlr:
    bk_isp_camera_deinit(s_isp_dump_cam.camera_ctlr_handle);
    bk_isp_camera_delete(s_isp_dump_cam.camera_ctlr_handle);
    s_isp_dump_cam.camera_ctlr_handle = NULL;
err_bus:
    bk_camera_bus_disable(bus);
    bk_camera_bus_delete(bus);
    if (s_isp_dump_cam.sensor_handle)
    {
        bk_camera_sensor_destroy(s_isp_dump_cam.sensor_handle);
        s_isp_dump_cam.sensor_handle = NULL;
    }
    return ret;
}

/**
 * @brief Create and start MP instance for dump pipeline
 */
static avdk_err_t isp_dump_create_mp_instance(uint16_t width, uint16_t height, bk_pixel_format_t output_fmt)
{
    avdk_err_t ret;
    bk_isp_camera_channel_config_t instance = CAM_MP_NV12_RB_INSTANCE_CONFIG(width, height);

    instance.port_id = ISP_DUMP_PORT_ID;
    instance.enable_flexa = 0;
    instance.work_mode = 0;
    instance.format = output_fmt;

    s_isp_dump_cam.mp_frame_size = bk_image_size_get(width, height, output_fmt);

    ret = bk_isp_camera_channel_open(s_isp_dump_cam.camera_ctlr_handle, ISP_DUMP_MP_CHNL_ID, &instance);

    if (ret != AVDK_ERR_OK)
    {
        ISP_DUMP_LOGE("Failed to start MP instance\n");
        return ret;
    }

    return AVDK_ERR_OK;
}

/**
 * @brief Open MIPI camera and start ISP MP channel (for CMD_START_ISP)
 */
static avdk_err_t isp_dump_open_mipi_camera(uint16_t sensor_w, uint16_t sensor_h, uint16_t fps,
                                            bk_pixel_format_t output_fmt,
                                            uint16_t isp_w, uint16_t isp_h)
{
    avdk_err_t ret;

    if (!s_isp_dump_cam.is_initialized)
    {
        ret = camera_power_enable(true);
        if (ret != AVDK_ERR_OK)
        {
            return ret;
        }

        ret = isp_dump_init_mipi_camera(sensor_w, sensor_h, fps);
        if (ret != AVDK_ERR_OK)
        {
            return ret;
        }
    }

    ret = isp_dump_create_mp_instance(isp_w, isp_h, output_fmt);
    if (ret != AVDK_ERR_OK)
    {
        if (bk_isp_camera_channel_state_get(s_isp_dump_cam.camera_ctlr_handle, ISP_MP_CHN_ID) == ISP_CHANNEL_STATE_TURN_OFF)
        {
            isp_dump_cleanup_all_resources();
        }
        return ret;
    }

    if (!s_isp_dump_cam.is_sensor_started)
    {
        ret = bk_camera_sensor_init(s_isp_dump_cam.sensor_handle);
        if (ret != AVDK_ERR_OK)
        {
            ISP_DUMP_LOGE("Failed to init sensor\n");
            isp_dump_close_mp_channel();
            return ret;
        }

        {
            bk_camera_sensor_format_t format = {
                .width = sensor_w,
                .height = sensor_h,
                .fps = fps,
            };
            ret = bk_camera_sensor_set_format(s_isp_dump_cam.sensor_handle, &format);
            if (ret != AVDK_ERR_OK)
            {
                ISP_DUMP_LOGE("Failed to set sensor format\n");
                isp_dump_close_mp_channel();
                return ret;
            }
        }

        s_isp_dump_cam.is_sensor_started = 1;
    }

    return AVDK_ERR_OK;
}

/**
 * @brief Cleanup all dump pipeline resources (bus, sensor, controller)
 */
static avdk_err_t isp_dump_cleanup_all_resources(void)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_isp_dump_cam.is_initialized && s_isp_dump_cam.camera_ctlr_handle != NULL)
    {
        ret = bk_isp_camera_deinit(s_isp_dump_cam.camera_ctlr_handle);
        if (ret != AVDK_ERR_OK)
        {
            ISP_DUMP_LOGW("Failed to deinit camera device\n");
        }
    }

    if (s_isp_dump_cam.camera_ctlr_handle != NULL)
    {
        ret = bk_isp_camera_delete(s_isp_dump_cam.camera_ctlr_handle);
        if (ret != AVDK_ERR_OK)
        {
            ISP_DUMP_LOGW("Failed to delete camera controller\n");
        }
        s_isp_dump_cam.camera_ctlr_handle = NULL;
    }

    if (s_isp_dump_cam.bus != NULL)
    {
        ret = bk_camera_bus_disable(s_isp_dump_cam.bus);
        if (ret != AVDK_ERR_OK)
        {
            ISP_DUMP_LOGW("Failed to disable camera bus\n");
        }
        ret = bk_camera_bus_delete(s_isp_dump_cam.bus);
        if (ret != AVDK_ERR_OK)
        {
            ISP_DUMP_LOGW("Failed to delete camera bus\n");
        }
        s_isp_dump_cam.bus = NULL;
    }

    if (s_isp_dump_cam.sensor_handle)
    {
        bk_camera_sensor_destroy(s_isp_dump_cam.sensor_handle);
        s_isp_dump_cam.sensor_handle = NULL;
    }
    s_isp_dump_cam.is_initialized = 0;
    s_isp_dump_cam.is_sensor_started = 0;
    s_isp_dump_cam.mp_frame_size = 0;
    ISP_DUMP_LOGI("ISP dump pipeline resources cleaned up\n");
    return AVDK_ERR_OK;
}

/**
 * @brief Close MP channel and fully stop ISP pipeline (for CMD_RESET_ISP / UI button "停止ISP").
 *        Stops sensor first (power_down) so no new frames are fed, then stops ISP
 *        instance and cleans up to ensure ISP is fully closed and no image is generated.
 */
static avdk_err_t isp_dump_close_mp_channel(void)
{
    avdk_err_t ret;

    if (bk_isp_camera_channel_state_get(s_isp_dump_cam.camera_ctlr_handle, ISP_MP_CHN_ID) == ISP_CHANNEL_STATE_TURN_OFF)
    {
        return AVDK_ERR_OK;
    }

    /* Stop sensor streaming first so MIPI/CSI stops receiving data and no new image is generated */
    if (s_isp_dump_cam.sensor_handle != NULL && s_isp_dump_cam.sensor_handle->power_down != NULL)
    {
        s_isp_dump_cam.sensor_handle->power_down(s_isp_dump_cam.sensor_handle);
        s_isp_dump_cam.is_sensor_started = 0;
        rtos_delay_milliseconds(50);
    }

    ret = bk_isp_camera_channel_close(s_isp_dump_cam.camera_ctlr_handle, ISP_MP_CHN_ID);
    if (ret != AVDK_ERR_OK)
    {
        ISP_DUMP_LOGE("Failed to close MP channel: %d\n", ret);
        return ret;
    }

    ret = camera_power_enable(false);
    if (ret != AVDK_ERR_OK)
    {
        ISP_DUMP_LOGE("Failed to power down camera: %d\n", ret);
        return ret;
    }

    return isp_dump_cleanup_all_resources();
}

/* ---------------------------------------------------------------------------/
 * isp_ui_* API implementation (used by TCP CMD_START_ISP / CMD_RESET_ISP / CMD_SEND_FRAME)
 * --------------------------------------------------------------------------- */

bk_err_t isp_ui_start_with_params(uint16_t sensor_w, uint16_t sensor_h,
                                   uint16_t isp_w, uint16_t isp_h, uint32_t bk_fmt)
{
    avdk_err_t ret;

    ret = isp_dump_open_mipi_camera(sensor_w, sensor_h, ISP_DUMP_DEFAULT_FPS,
                                   (bk_pixel_format_t)bk_fmt, isp_w, isp_h);
    if (ret != AVDK_ERR_OK)
    {
        ISP_DUMP_LOGE("isp_ui_start_with_params failed: %d\n", ret);
        return BK_FAIL;
    }
    return BK_OK;
}

bk_err_t isp_ui_stop_isp(void)
{
    avdk_err_t ret = isp_dump_close_mp_channel();

    if (ret != AVDK_ERR_OK)
    {
        ISP_DUMP_LOGE("isp_ui_stop_isp failed: %d\n", ret);
        return BK_FAIL;
    }
    return BK_OK;
}

bk_err_t isp_ui_capture_frame(uint8_t **frame, uint32_t *frame_len)
{
    avdk_err_t ret;
    uint8_t *buf = NULL;
    uint32_t size;

    if (frame == NULL || frame_len == NULL)
    {
        ISP_DUMP_LOGE("isp_ui_capture_frame: NULL output\n");
        return BK_FAIL;
    }

    if (bk_isp_camera_channel_state_get(s_isp_dump_cam.camera_ctlr_handle, ISP_MP_CHN_ID) == ISP_CHANNEL_STATE_TURN_OFF)
    {
        ISP_DUMP_LOGE("isp_ui_capture_frame: MP not opened\n");
        return BK_FAIL;
    }

    size = s_isp_dump_cam.mp_frame_size;
    if (size == 0)
    {
        ISP_DUMP_LOGE("isp_ui_capture_frame: frame size 0\n");
        return BK_FAIL;
    }

    buf = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, size);
    if (buf == NULL)
    {
        ISP_DUMP_LOGE("isp_ui_capture_frame: malloc failed\n");
        return BK_FAIL;
    }

    ret = bk_isp_camera_read(s_isp_dump_cam.camera_ctlr_handle, 0, buf, size, -1);
    if (ret != AVDK_ERR_OK)
    {
        ISP_DUMP_LOGE("isp_ui_capture_frame: read failed: %d\n", ret);
        bk_frame_buffer_free(buf);
        return BK_FAIL;
    }

    *frame = buf;
    *frame_len = size;
    return BK_OK;
}

void isp_ui_release_frame(uint8_t *frame)
{
    if (frame != NULL)
    {
        bk_frame_buffer_free(frame);
    }
}

/**
 * @brief Handle one binary command packet from client
 */
static void isp_dump_handle_binary_cmd(int client_fd, const uint8_t *packet)
{
    uint32_t magic;
    uint32_t cmd;
    uint32_t sensor_w, sensor_h, isp_w, isp_h, fmt;
    bk_pixel_format_t bk_fmt = BK_PIXEL_FORMAT_UNKNOW;
    uint8_t *frame = NULL;
    uint32_t frame_len = 0;
    bk_err_t ret;

    magic = isp_dump_read_u32_le(packet + 0);
    cmd = isp_dump_read_u32_le(packet + 4);
    sensor_w = isp_dump_read_u32_le(packet + 8);
    sensor_h = isp_dump_read_u32_le(packet + 12);
    isp_w = isp_dump_read_u32_le(packet + 16);
    isp_h = isp_dump_read_u32_le(packet + 20);
    fmt = isp_dump_read_u32_le(packet + 24);

    if (magic != ISP_DUMP_MAGIC_WORD)
    {
        ISP_DUMP_LOGW("invalid magic: 0x%08x\n", (unsigned int)magic);
        return;
    }

    if (sensor_w > 0xFFFF || sensor_h > 0xFFFF || isp_w > 0xFFFF || isp_h > 0xFFFF)
    {
        ISP_DUMP_LOGE("invalid resolution: sensor=%ux%u isp=%ux%u\n",
                      (unsigned int)sensor_w, (unsigned int)sensor_h,
                      (unsigned int)isp_w, (unsigned int)isp_h);
        return;
    }

    if (cmd == ISP_DUMP_CMD_START_ISP)
    {
        ret = isp_dump_map_fmt_to_bk(fmt, &bk_fmt);
        if (ret != BK_OK)
        {
            ISP_DUMP_LOGE("CMD_START_ISP unsupported fmt=%u\n", (unsigned int)fmt);
            return;
        }

        ret = isp_ui_start_with_params((uint16_t)sensor_w, (uint16_t)sensor_h,
                                       (uint16_t)isp_w, (uint16_t)isp_h, (uint32_t)bk_fmt);
        if (ret != BK_OK)
        {
            ISP_DUMP_LOGE("CMD_START_ISP failed, fmt=%u\n", (unsigned int)fmt);
        }
        return;
    }

    if (cmd == ISP_DUMP_CMD_SEND_FRAME)
    {
        ret = isp_ui_capture_frame(&frame, &frame_len);
        if (ret != BK_OK || frame == NULL || frame_len == 0)
        {
            ISP_DUMP_LOGE("CMD_SEND_FRAME capture failed\n");
            if (frame != NULL)
            {
                isp_ui_release_frame(frame);
            }
            return;
        }

        if (isp_dump_send_all(client_fd, frame, frame_len) != 0)
        {
            ISP_DUMP_LOGE("CMD_SEND_FRAME send failed, len=%u\n", (unsigned int)frame_len);
            isp_ui_release_frame(frame);
            return;
        }

        isp_ui_release_frame(frame);
        return;
    }

    if (cmd == ISP_DUMP_CMD_RESET_ISP)
    {
        ret = isp_ui_stop_isp();
        if (ret != BK_OK)
        {
            ISP_DUMP_LOGE("CMD_STOP_ISP (stop ISP) failed\n");
        }
        return;
    }

    ISP_DUMP_LOGW("unknown cmd: %u\n", (unsigned int)cmd);
}

/**
 * @brief TCP server task: accept clients and handle binary commands
 */
static void isp_dump_server_task(beken_thread_arg_t arg)
{
    int listen_fd = -1;
    int client_fd = -1;
    int ret;
    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    uint8_t recv_buf[ISP_DUMP_RECV_BUF_LEN];
    uint8_t cmd_buf[ISP_DUMP_CMD_PACKET_LEN];
    uint32_t cmd_buf_len = 0;

    (void)arg;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        ISP_DUMP_LOGE("socket create failed\n");
        goto exit;
    }

    s_listen_fd = listen_fd;

    {
        int opt = 1;
        (void)setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }

    os_memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ISP_DUMP_SERVER_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret != 0)
    {
        ISP_DUMP_LOGE("bind failed, ret=%d\n", ret);
        goto exit;
    }

    ret = listen(listen_fd, ISP_DUMP_LISTEN_BACKLOG);
    if (ret != 0)
    {
        ISP_DUMP_LOGE("listen failed, ret=%d\n", ret);
        goto exit;
    }

    ISP_DUMP_LOGI("ISP dump server started on port %d\n", ISP_DUMP_SERVER_PORT);

    while (!s_isp_dump_stop)
    {
        client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            if (s_isp_dump_stop)
            {
                break;
            }
            ISP_DUMP_LOGW("accept failed\n");
            rtos_delay_milliseconds(100);
            continue;
        }

        ISP_DUMP_LOGI("client connected: %s:%d\n",
                      inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        while (!s_isp_dump_stop)
        {
            ret = recv(client_fd, recv_buf, sizeof(recv_buf), 0);
            if (ret <= 0)
            {
                ISP_DUMP_LOGI("client disconnected\n");
                break;
            }

            for (int i = 0; i < ret; i++)
            {
                if (cmd_buf_len < ISP_DUMP_CMD_PACKET_LEN)
                {
                    cmd_buf[cmd_buf_len++] = recv_buf[i];
                }

                if (cmd_buf_len == ISP_DUMP_CMD_PACKET_LEN)
                {
                    isp_dump_handle_binary_cmd(client_fd, cmd_buf);
                    cmd_buf_len = 0;
                }
            }
        }

        close(client_fd);
        client_fd = -1;
        cmd_buf_len = 0;
    }

exit:
    if (client_fd >= 0)
    {
        close(client_fd);
    }
    if (listen_fd >= 0)
    {
        close(listen_fd);
    }
    s_listen_fd = -1;
    s_isp_dump_thread = NULL;
    rtos_delete_thread(NULL);
}

/**
 * @brief Start ISP dump TCP server (called by CLI "isp_dump start")
 */
void isp_dump_server_init(void)
{
    bk_err_t ret;

    if (s_isp_dump_thread != NULL)
    {
        ISP_DUMP_LOGW("ISP dump server already started\n");
        return;
    }

    s_isp_dump_stop = 0;

    ret = rtos_create_thread(&s_isp_dump_thread,
                              BEKEN_DEFAULT_WORKER_PRIORITY,
                              "isp_dump_srv",
                              (beken_thread_function_t)isp_dump_server_task,
                              ISP_DUMP_SERVER_STACK_SIZE,
                              NULL);
    if (ret != BK_OK)
    {
        ISP_DUMP_LOGE("create isp_dump_server thread failed: %d\n", ret);
        s_isp_dump_thread = NULL;
    }
}

/**
 * @brief Stop ISP dump TCP server (called by CLI "isp_dump stop")
 */
void isp_dump_server_deinit(void)
{
    if (s_isp_dump_thread == NULL)
    {
        ISP_DUMP_LOGW("ISP dump server not started\n");
        return;
    }

    s_isp_dump_stop = 1;
    if (s_listen_fd >= 0)
    {
        (void)shutdown(s_listen_fd, SHUT_RDWR);
        close(s_listen_fd);
        s_listen_fd = -1;
    }

    /* Wait for task to exit (it clears s_isp_dump_thread) */
    for (int i = 0; i < 50 && s_isp_dump_thread != NULL; i++)
    {
        rtos_delay_milliseconds(100);
    }

    if (s_isp_dump_thread != NULL)
    {
        ISP_DUMP_LOGW("isp_dump server thread did not exit in time\n");
        s_isp_dump_thread = NULL;
    }
}

/**
 * @brief CLI command handler for ISP dump server (isp_dump start | stop)
 * @param pcWriteBuffer Buffer to write command response
 * @param xWriteBufferLen Length of the write buffer
 * @param argc Number of command arguments
 * @param argv Command arguments array
 */
void cli_isp_dump_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;
    char *msg = NULL;

    (void)xWriteBufferLen;

    if (argc < 2)
    {
        LOGE("Usage: isp_dump <start|stop|sns_read|sns_write>\n");
        ret = AVDK_ERR_UNSUPPORTED;
        goto exit;
    }

    /* Sensor register read test command: isp_dump sns_read <addr> <width:8|16> */
    if (os_strcmp(argv[1], "sns_read") == 0)
    {
        if (argc < 4 || argv[2] == NULL || argv[3] == NULL)
        {
            LOGE("Usage: isp_dump sns_read <addr> <width:8|16>\n");
            ret = AVDK_ERR_UNSUPPORTED;
            goto exit;
        }

        unsigned long reg_addr = os_strtoul(argv[2], NULL, 0);
        unsigned long width = os_strtoul(argv[3], NULL, 10);
        bk_camera_bus_t *bus = bk_camera_bus_get();
        if (bus == NULL)
        {
            LOGE("Camera bus not initialized\n");
            ret = AVDK_ERR_GENERIC;
            goto exit;
        }

        if ((width != 8) && (width != 16))
        {
            LOGE("Invalid width %lu, only 8 or 16 are supported\n", width);
            ret = AVDK_ERR_UNSUPPORTED;
            goto exit;
        }

        uint32_t value = 0;
        if (width == 8)
        {
            if (bus->read8(bus, (uint32_t)reg_addr, (uint8_t *)&value) != BK_OK)
            {
                LOGE("I2C read8 failed at addr=0x%lx\n", reg_addr);
                ret = AVDK_ERR_GENERIC;
                goto exit;
            }
            LOGI("Sensor reg[0x%lx] (8-bit) = 0x%02x\n", reg_addr, (unsigned int)(value & 0xFF));
        }
        else
        {
            if (bus->read16(bus, (uint32_t)reg_addr, (uint8_t *)&value) != BK_OK)
            {
                LOGE("I2C read16 failed at addr=0x%lx\n", reg_addr);
                ret = AVDK_ERR_GENERIC;
                goto exit;
            }
            LOGI("Sensor reg[0x%lx] (16-bit) = 0x%04x\n", reg_addr, (unsigned int)(value & 0xFFFF));
        }

        ret = AVDK_ERR_OK;
        goto exit;
    }

    /* Sensor register write test command: isp_dump sns_write <addr> <value> <width:8|16> */
    if (os_strcmp(argv[1], "sns_write") == 0)
    {
        if (argc < 5 || argv[2] == NULL || argv[3] == NULL || argv[4] == NULL)
        {
            LOGE("Usage: isp_dump sns_write <addr> <value> <width:8|16>\n");
            ret = AVDK_ERR_UNSUPPORTED;
            goto exit;
        }

        unsigned long reg_addr = os_strtoul(argv[2], NULL, 0);
        unsigned long reg_val  = os_strtoul(argv[3], NULL, 0);
        unsigned long width    = os_strtoul(argv[4], NULL, 10);

        bk_camera_bus_t *bus = bk_camera_bus_get();
        if (bus == NULL)
        {
            LOGE("Camera bus not initialized\n");
            ret = AVDK_ERR_GENERIC;
            goto exit;
        }

        if ((width != 8) && (width != 16))
        {
            LOGE("Invalid width %lu, only 8 or 16 are supported\n", width);
            ret = AVDK_ERR_UNSUPPORTED;
            goto exit;
        }

        if (width == 8)
        {
            uint8_t v8 = (uint8_t)(reg_val & 0xFF);
            if (bus->write8(bus, (uint32_t)reg_addr, (uint32_t)v8) != BK_OK)
            {
                LOGE("I2C write8 failed at addr=0x%lx, val=0x%02x\n", reg_addr, (unsigned int)v8);
                ret = AVDK_ERR_GENERIC;
                goto exit;
            }
            LOGI("Sensor reg[0x%lx] (8-bit) <= 0x%02x\n", reg_addr, (unsigned int)v8);
        }
        else
        {
            uint16_t v16 = (uint16_t)(reg_val & 0xFFFF);
            if (bus->write16(bus, (uint32_t)reg_addr, (uint32_t)v16) != BK_OK)
            {
                LOGE("I2C write16 failed at addr=0x%lx, val=0x%04x\n", reg_addr, (unsigned int)v16);
                ret = AVDK_ERR_GENERIC;
                goto exit;
            }
            LOGI("Sensor reg[0x%lx] (16-bit) <= 0x%04x\n", reg_addr, (unsigned int)v16);
        }

        ret = AVDK_ERR_OK;
        goto exit;
    }

    if (os_strcmp(argv[1], "start") == 0)
    {
        LOGI("Starting ISP dump server...\n");
        isp_dump_server_init();
        LOGI("ISP dump server started successfully\n");
        ret = AVDK_ERR_OK;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        LOGI("Stopping ISP dump server...\n");
        isp_dump_server_deinit();
        LOGI("ISP dump server stopped successfully\n");
        ret = AVDK_ERR_OK;
    }
    else
    {
        LOGE("Invalid command: %s (expected start or stop)\n", argv[1]);
        LOGE("Usage: isp_dump <start|stop>\n");
        ret = AVDK_ERR_UNSUPPORTED;
        goto exit;
    }

exit:
    if (ret != AVDK_ERR_OK)
    {
        msg = CLI_CMD_RSP_ERROR;
    }
    else
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
