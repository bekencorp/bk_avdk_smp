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
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

// ISP camera handle structure
typedef struct {
    bk_camera_bus_t *bus;
    bk_camera_sensor_handle_t sensor_handle;
    bk_isp_camera_ctlr_handle_t camera_ctlr_handle;
    uint32_t mp_frame_size;
    uint32_t sp_frame_size;
    /* Per-channel output geometry/format, recorded at channel open time so the
     * DVP capture callback can report the frame attributes to the application. */
    uint16_t mp_width;
    uint16_t mp_height;
    uint16_t sp_width;
    uint16_t sp_height;
    bk_pixel_format_t mp_fmt;
    bk_pixel_format_t sp_fmt;
    uint8_t is_initialized;  // Flag to indicate if camera is initialized
    uint8_t is_mipi;        // Flag to indicate MIPI or DVP
    uint8_t is_sensor_started;  // Flag to indicate if sensor streaming is started
} isp_camera_handle_t;

static isp_camera_handle_t s_isp_camera_handle = {0};

/* DVP/ISP frame capture context: a background task reads frames from the ISP
 * channel and pushes them to the application-registered callback. This is the
 * "callback style" data interface requested for the DVP example. */
typedef struct {
    isp_dvp_frame_cb_t cb;       // Application frame callback
    void *user_arg;              // Argument forwarded to the callback
    beken_thread_t thread;       // Capture task handle
    volatile uint8_t running;    // Capture task run flag
    uint8_t chnl_id;             // ISP_MP_CHN_ID or ISP_SP_CHN_ID
} isp_dvp_capture_ctx_t;

static isp_dvp_capture_ctx_t s_isp_dvp_capture = {0};

/* Default per-frame read timeout (ms) for the capture task; bounded so the
 * loop can observe the stop request in a timely manner. */
#define ISP_DVP_CAPTURE_READ_TIMEOUT_MS   (1000)
#define ISP_DVP_CAPTURE_TASK_PRIORITY     (5)
#define ISP_DVP_CAPTURE_TASK_STACK_SIZE   (4 * 1024)

/* Forward declarations for functions used before their definitions. */
static avdk_err_t isp_cleanup_all_resources(void);
static avdk_err_t isp_close_channel(uint8_t is_mp);
avdk_err_t isp_dvp_capture_stop(void);
avdk_err_t isp_dvp_unregister_frame_cb(void);

/**
 * @brief Detect sensor on a specific camera port and print its information
 * @param port Camera port (DVP_CAMERA_PORT or CSI_CAMERA_PORT)
 * @param bus_config Bus configuration for the port
 * @param sensor_config Sensor configuration
 * @param port_name Port name string for logging (e.g., "DVP" or "CSI")
 * @return 1 if sensor detected, 0 otherwise
 */
static int isp_detect_sensor_on_port(bk_camera_port_t port,
                                      bk_camera_bus_config_t *bus_config,
                                      bk_camera_sensor_config_t *sensor_config,
                                      const char *port_name)
{
    bk_camera_bus_t *bus = NULL;
    bk_camera_sensor_handle_t sensor_handle = NULL;
    bk_camera_sensor_format_array_t format_array = {0};
    int detected = 0;

    LOGI("Checking %s port...\n", port_name);

    bus = bk_camera_bus_new(bus_config);
    if (!bus)
    {
        LOGI("  [%s] Failed to create bus\n", port_name);
        return 0;
    }

    if (bk_camera_bus_enable(bus) != AVDK_ERR_OK)
    {
        LOGI("  [%s] Failed to enable bus\n", port_name);
        bk_camera_bus_delete(bus);
        return 0;
    }

    sensor_config->bus = bus;
    sensor_handle = bk_camera_sensor_auto_detect(sensor_config, port);

    if (sensor_handle)
    {
        detected = 1;
        LOGI("  [%s] Sensor detected!\n", port_name);

        // Query supported formats
        if (bk_camera_sensor_query_support_formats(sensor_handle, &format_array) == AVDK_ERR_OK)
        {
            if (format_array.size > 0)
            {
                LOGI("[%s] Supported formats (%d):\n", port_name, format_array.size);
                for (uint32_t i = 0; i < format_array.size; i++)
                {
                    LOGI("    Format %d: %dx%d @ %dfps\n",
                        i + 1,
                        format_array.format_array[i].width,
                        format_array.format_array[i].height,
                        format_array.format_array[i].fps);
                }
            }
            else
            {
                LOGI("[%s] No supported formats found\n", port_name);
            }
        }
        else
        {
            LOGI("[%s] Failed to query supported formats\n", port_name);
        }
    }
    else
    {
        LOGI("[%s] No sensor detected\n", port_name);
    }

    if (sensor_handle)
    {
        bk_camera_sensor_destroy(sensor_handle);
    }

    bk_camera_bus_disable(bus);
    bk_camera_bus_delete(bus);

    return detected;
}

/**
 * @brief Detect all sensors on DVP and CSI ports
 * @return Number of sensors detected
 */
int isp_detect_all_sensors(void)
{
    int sensor_count = 0;
    bk_camera_bus_config_t bus_config;
    bk_camera_sensor_config_t sensor_config;

    LOGI("Starting sensor detection...\n");

    // Detect DVP sensor
    bus_config = (bk_camera_bus_config_t)DVP_CAM_BUS_I2C1_8BIT_2000TIMEOUT();
    os_memset(&sensor_config, 0, sizeof(sensor_config));
    sensor_config.pin_reset = GPIO_13;
    sensor_config.pin_pwdn = 0xFF;
    sensor_config.pin_xclk = GPIO_27;
    sensor_count += isp_detect_sensor_on_port(DVP_CAMERA_PORT, &bus_config, &sensor_config, "DVP");

    // Detect CSI sensor
    bus_config = (bk_camera_bus_config_t)CSI_CAM_BUS_I2C1_8BIT_2000TIMEOUT();
    os_memset(&sensor_config, 0, sizeof(sensor_config));
    sensor_config.pin_reset = GPIO_71;
    sensor_config.pin_pwdn = 0xFF;
    sensor_config.pin_xclk = GPIO_59;
    sensor_count += isp_detect_sensor_on_port(CSI_CAMERA_PORT, &bus_config, &sensor_config, "CSI");

    LOGI("Sensor detection complete. Found %d sensor(s).\n", sensor_count);

    return sensor_count;
}

/**
 * @brief Check if sensor supports the requested resolution
 * @param sensor_handle Sensor handle
 * @param width Requested width
 * @param height Requested height
 * @param fps Requested fps
 * @return AVDK_ERR_OK if supported, error code otherwise
 */
static avdk_err_t isp_check_sensor_resolution(bk_camera_sensor_handle_t sensor_handle,
                                               uint16_t width,
                                               uint16_t height,
                                               uint16_t fps)
{
    bk_camera_sensor_format_array_t format_array = {0};
    avdk_err_t ret;

    if (!sensor_handle)
    {
        LOGE("Sensor handle is NULL\n");
        return AVDK_ERR_INVAL;
    }

    ret = bk_camera_sensor_query_support_formats(sensor_handle, &format_array);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to query sensor supported formats\n");
        return ret;
    }

    if (format_array.size == 0)
    {
        LOGE("Sensor has no supported formats\n");
        return AVDK_ERR_INVAL;
    }

    // Find matching format
    int found = 0;
    uint16_t max_sensor_width = 0;
    uint16_t max_sensor_height = 0;

    for (uint32_t i = 0; i < format_array.size; i++)
    {
        uint16_t fmt_width = format_array.format_array[i].width;
        uint16_t fmt_height = format_array.format_array[i].height;
        uint16_t fmt_fps = format_array.format_array[i].fps;

        // Track maximum sensor resolution
        if (fmt_width > max_sensor_width)
        {
            max_sensor_width = fmt_width;
        }
        if (fmt_height > max_sensor_height)
        {
            max_sensor_height = fmt_height;
        }

        // Check exact match
        if (fmt_width == width && fmt_height == height && fmt_fps == fps)
        {
            found = 1;
            break;
        }
    }

    // Check if sensor output resolution is less than ISP output resolution
    // Hardware does not support upscaling
    if (max_sensor_width < width || max_sensor_height < height)
    {
        LOGE("Sensor max resolution (%dx%d) is less than ISP output resolution (%dx%d)\n",
            max_sensor_width, max_sensor_height, width, height);
        LOGE("Hardware does not support upscaling\n");
        return AVDK_ERR_INVAL;
    }

    if (!found)
    {
        LOGE("Sensor does not support resolution %dx%d @ %dfps\n", width, height, fps);
        LOGE("Supported formats:\n");
        for (uint32_t i = 0; i < format_array.size; i++)
        {
            LOGE("  %dx%d @ %dfps\n",
                format_array.format_array[i].width,
                format_array.format_array[i].height,
                format_array.format_array[i].fps);
        }
        return AVDK_ERR_INVAL;
    }

    LOGI("Sensor supports resolution %dx%d @ %dfps\n", width, height, fps);
    return AVDK_ERR_OK;
}

/**
 * @brief Initialize MIPI CSI camera (bus, sensor, controller, port)
 * @param width Output width
 * @param height Output height
 * @param fps Frame rate
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_init_mipi_camera(uint16_t width, uint16_t height, uint16_t fps, uint8_t mini_code)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_camera_bus_t *bus = NULL;
    bk_camera_bus_config_t bus_config = (bk_camera_bus_config_t)CSI_CAM_BUS_I2C1_8BIT_2000TIMEOUT();
    bk_camera_sensor_config_t sensor_config = {
        .pin_reset = GPIO_71,
        .pin_pwdn = 0xFF,
        .pin_xclk = GPIO_59,
    };

    LOGI("Initializing MIPI CSI camera: %dx%d @ %dfps\n", width, height, fps);

    // Step 1: Create and enable bus
    bus = bk_camera_bus_new(&bus_config);
    if (!bus)
    {
        LOGE("Failed to create camera bus\n");
        ret = AVDK_ERR_GENERIC;
        goto err;
    }

    ret = bk_camera_bus_enable(bus);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to enable camera bus\n");
        goto err;
    }

    // Step 2: Detect sensor
    sensor_config.bus = bus;
    s_isp_camera_handle.sensor_handle = bk_camera_sensor_auto_detect(&sensor_config, CSI_CAMERA_PORT);
    if (!s_isp_camera_handle.sensor_handle)
    {
        LOGE("No sensor detected on MIPI CSI port\n");
        ret = AVDK_ERR_NODEV;
        goto err;
    }

    // Step 3: Check sensor resolution
    ret = isp_check_sensor_resolution(s_isp_camera_handle.sensor_handle, width, height, fps);
    if (ret != AVDK_ERR_OK)
    {
        goto err;
    }

    if (mini_code == 0) {
        bk_isp_camera_ctlr_config_t isp_ctlr_config = CAM_CSI_DEFAULT_RAW10_CONFIG(width, height, fps);

        /* Pull the raw pixel format from the matched sensor mode (the macro
         * no longer hard-codes it); falls back to the first entry if no exact
         * (w,h,fps) match is found. */
        {
            bk_camera_sensor_format_array_t fmt_arr = {0};
            if (bk_camera_sensor_query_support_formats(s_isp_camera_handle.sensor_handle, &fmt_arr) == AVDK_ERR_OK
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

        // Step 4: Get sensor object
        const void *sensor_object = bk_camera_sensor_get_sensor_object(s_isp_camera_handle.sensor_handle);
        if (!sensor_object)
        {
            LOGE("Failed to get sensor object\n");
            ret = AVDK_ERR_GENERIC;
            goto err;
        }
        isp_ctlr_config.sensor_object = sensor_object;

        // Step 5: Create and init camera controller
        ret = bk_camera_isp_ctlr_new(&s_isp_camera_handle.camera_ctlr_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("Failed to create camera controller\n");
            goto err;
        }

        ret = bk_isp_camera_dev_init(s_isp_camera_handle.camera_ctlr_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("Failed to init camera device\n");
            goto err;
        }

        // Step 6: Init camera port
        ret = bk_isp_camera_port_init(s_isp_camera_handle.camera_ctlr_handle, &isp_ctlr_config);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("Failed to init camera port\n");
            goto err;
        }
    }
    LOGI("%s, %d\n", __func__, __LINE__);

    s_isp_camera_handle.bus = bus;
    s_isp_camera_handle.is_initialized = 1;
    s_isp_camera_handle.is_mipi = 1;
    LOGI("MIPI CSI camera initialized successfully\n");

    return AVDK_ERR_OK;

err:
    if (s_isp_camera_handle.camera_ctlr_handle)
    {
        bk_isp_camera_deinit(s_isp_camera_handle.camera_ctlr_handle);
        bk_isp_camera_delete(s_isp_camera_handle.camera_ctlr_handle);
        s_isp_camera_handle.camera_ctlr_handle = NULL;
    }

    if (s_isp_camera_handle.sensor_handle)
    {
        bk_camera_sensor_destroy(s_isp_camera_handle.sensor_handle);
        s_isp_camera_handle.sensor_handle = NULL;
    }

    if (bus)
    {
        bk_camera_bus_disable(bus);
        bk_camera_bus_delete(bus);
    }

    s_isp_camera_handle.is_sensor_started = 0;
    return ret;
}

/**
 * @brief Create and start a camera instance (MP or SP channel)
 * @param chnl_id Channel ID (0 for MP, 1 for SP)
 * @param width Output width
 * @param height Output height
 * @param flexa_mode Flexa mode (0: frame, 1: hardware flexa, 2: software flexa)
 * @param output_fmt Output pixel format
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_create_instance(uint8_t chnl_id, uint16_t width, uint16_t height,
                                      uint8_t flexa_mode, bk_pixel_format_t output_fmt)
{
    avdk_err_t ret = AVDK_ERR_OK;
    /* MIPI uses ISP port 0, DVP uses ISP port 1. The previous hard-coded
     * port_id = 0 forced DVP onto the MIPI port and broke DVP frame buffer
     * mode; the doorbell reference also opens the DVP channel with port_id = 1
     * (ISP_DVP_PORT_ID). */
    uint8_t port_id = s_isp_camera_handle.is_mipi ? ISP_MIPI_PORT_ID : ISP_DVP_PORT_ID;
    const char *chnl_name = (chnl_id == 0) ? "MP" : "SP";

    if (!s_isp_camera_handle.is_initialized || !s_isp_camera_handle.camera_ctlr_handle)
    {
        LOGE("Camera not initialized, cannot create instance\n");
        return AVDK_ERR_INVAL;
    }

    // Check if instance already exists
    if (chnl_id == 0 && bk_isp_camera_channel_state_get(s_isp_camera_handle.camera_ctlr_handle, ISP_MP_CHN_ID) == ISP_CHANNEL_STATE_TURN_ON)
    {
        LOGW("MP instance already exists\n");
        return AVDK_ERR_OK;
    }
    if (chnl_id == 1 && bk_isp_camera_channel_state_get(s_isp_camera_handle.camera_ctlr_handle, ISP_SP_CHN_ID) == ISP_CHANNEL_STATE_TURN_ON)
    {
        LOGW("SP instance already exists\n");
        return AVDK_ERR_OK;
    }

    if (chnl_id == 0) {
        s_isp_camera_handle.mp_frame_size = bk_image_size_get(width, height, output_fmt);
        s_isp_camera_handle.mp_width = width;
        s_isp_camera_handle.mp_height = height;
        s_isp_camera_handle.mp_fmt = output_fmt;
    }
    else {
        s_isp_camera_handle.sp_frame_size = bk_image_size_get(width, height, output_fmt);
        s_isp_camera_handle.sp_width = width;
        s_isp_camera_handle.sp_height = height;
        s_isp_camera_handle.sp_fmt = output_fmt;
    }

    LOGI("Creating %s instance: %dx%d, flexa=%d, fmt=%d, mp_frame_size=%d, sp_frame_size=%d\n",
        chnl_name, width, height, flexa_mode, output_fmt, s_isp_camera_handle.mp_frame_size, s_isp_camera_handle.sp_frame_size);

    // Create instance
    bk_isp_camera_channel_config_t instance = CAM_MP_NV12_RB_INSTANCE_CONFIG(width, height);
    instance.port_id = port_id;
    instance.enable_flexa = (flexa_mode > 0) ? 1 : 0;
    instance.work_mode = (flexa_mode > 0) ? 1 : 0; // 0: frame mode, 1: flexa mode
    instance.format = output_fmt;

    ret = bk_isp_camera_channel_open(s_isp_camera_handle.camera_ctlr_handle, chnl_id, &instance);

    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to create %s instance\n", chnl_name);
        return ret;
    }

    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to start %s instance\n", chnl_name);
        /* Start failed, destroy instance to avoid leaking pipeline resources. */
        return ret;
    }

    LOGI("%s instance created and started successfully\n", chnl_name);
    return AVDK_ERR_OK;
}

/**
 * @brief Open MIPI CSI camera (legacy function, kept for compatibility)
 * @param width Output width
 * @param height Output height
 * @param fps Frame rate
 * @param chnl_id Channel ID (0 for MP, 1 for SP)
 * @param flexa_mode Flexa mode (0: frame, 1: flexa mode)
 * @param output_fmt Output pixel format
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_open_mipi_camera(uint16_t sensor_width, uint16_t sensor_height, uint16_t fps,
                                       uint8_t chnl_id, uint8_t flexa_mode, bk_pixel_format_t output_fmt,
                                       uint16_t isp_output_width, uint16_t isp_output_height)
{
    avdk_err_t ret;

    if (!s_isp_camera_handle.is_initialized)
    {
        ret = isp_init_mipi_camera(sensor_width, sensor_height, fps, 0);
        if (ret != AVDK_ERR_OK)
        {
            return ret;
        }
    }

    /* Step 6: Create and start instance (MP or SP) */
    ret = isp_create_instance(chnl_id, isp_output_width, isp_output_height, flexa_mode, output_fmt);
    if (ret != AVDK_ERR_OK)
    {
        /* If instance creation fails, rollback pipeline resources to keep system clean. */
        if (bk_isp_camera_channel_state_get(s_isp_camera_handle.camera_ctlr_handle, ISP_MP_CHN_ID) == ISP_CHANNEL_STATE_TURN_OFF
            && bk_isp_camera_channel_state_get(s_isp_camera_handle.camera_ctlr_handle, ISP_SP_CHN_ID) == ISP_CHANNEL_STATE_TURN_OFF)
        {
            isp_cleanup_all_resources();
        }
        return ret;
    }

    /* Step 7: Start sensor streaming at last */
    if (!s_isp_camera_handle.is_sensor_started)
    {
        ret = bk_camera_sensor_init(s_isp_camera_handle.sensor_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("Failed to init sensor\n");
            isp_close_channel(chnl_id == 0);
            return ret;
        }

        bk_camera_sensor_format_t format = {
            .width = sensor_width,
            .height = sensor_height,
            .fps = fps,
        };
        ret = bk_camera_sensor_set_format(s_isp_camera_handle.sensor_handle, &format);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("Failed to set sensor format\n");
            isp_close_channel(chnl_id == 0);
            return ret;
        }

        s_isp_camera_handle.is_sensor_started = 1;
    }

    return AVDK_ERR_OK;
}

/**
 * @brief Initialize DVP camera (bus, sensor, controller, port)
 * @param width Output width
 * @param height Output height
 * @param fps Frame rate
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_init_dvp_camera(uint16_t width, uint16_t height, uint16_t fps)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_camera_bus_t *bus = NULL;
    bk_camera_bus_config_t bus_config = (bk_camera_bus_config_t)DVP_CAM_BUS_I2C1_8BIT_2000TIMEOUT();
    bk_camera_sensor_config_t sensor_config = {
        .pin_reset = GPIO_13,
        .pin_pwdn = 0xFF,
        .pin_xclk = GPIO_27,
    };
    bk_isp_camera_ctlr_config_t isp_ctlr_config = CAM_DVP_DEFAULT_RAW8_CONFIG(width, height, fps);
    /* CAM_DVP_DEFAULT_RAW8_CONFIG hard-codes port_id = ISP_MIPI_PORT_ID (a known
     * SDK bug). The DVP channel is opened on ISP_DVP_PORT_ID, so the port whose
     * ispCoreSize gets configured here must match, otherwise VSI scale attr
     * check fails (outWidth > inWidth==0 => VSI_ERR_ILLEGAL_PARAM). */
    isp_ctlr_config.port_id = ISP_DVP_PORT_ID;
    /* Keep input_type = CSI_SENSOR as the macro sets it. The SDK deliberately
     * routes DVP frame-buffer (online) mode through the CSI_SENSOR ISP pipeline
     * (see bk_camera_configs.h note). Switching to DVP_SENSOR skips the sensor
     * data-path setup and the ISP never receives frames (read times out).
     * The CSI_SENSOR calib path (AE/WB InitAlgo) no longer asserts now that
     * bk_isp_device_config() runs VSI pipeline init for every ISP port, which
     * creates each module's mutex on the DVP port (ISP_DVP_PORT_ID) too. */

    LOGI("Initializing DVP camera: %dx%d @ %dfps\n", width, height, fps);

    // Step 1: Create and enable bus
    bus = bk_camera_bus_new(&bus_config);
    if (!bus)
    {
        LOGE("Failed to create camera bus\n");
        ret = AVDK_ERR_GENERIC;
        goto err;
    }

    ret = bk_camera_bus_enable(bus);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to enable camera bus\n");
        goto err;
    }

    // Step 2: Detect sensor
    sensor_config.bus = bus;
    s_isp_camera_handle.sensor_handle = bk_camera_sensor_auto_detect(&sensor_config, DVP_CAMERA_PORT);
    if (!s_isp_camera_handle.sensor_handle)
    {
        LOGE("No sensor detected on DVP port\n");
        ret = AVDK_ERR_NODEV;
        goto err;
    }

    // Step 3: Check sensor resolution
    ret = isp_check_sensor_resolution(s_isp_camera_handle.sensor_handle, width, height, fps);
    if (ret != AVDK_ERR_OK)
    {
        goto err;
    }

    // Step 4: Get sensor object
    const void *sensor_object = bk_camera_sensor_get_sensor_object(s_isp_camera_handle.sensor_handle);
    if (!sensor_object)
    {
        LOGE("Failed to get sensor object\n");
        ret = AVDK_ERR_GENERIC;
        goto err;
    }
    isp_ctlr_config.sensor_object = sensor_object;

    /* Pull the raw pixel format from the matched sensor mode (the macro no
     * longer hard-codes it); falls back to the first entry if no exact
     * (w,h,fps) match is found. */
    {
        bk_camera_sensor_format_array_t fmt_arr = {0};
        if (bk_camera_sensor_query_support_formats(s_isp_camera_handle.sensor_handle, &fmt_arr) == AVDK_ERR_OK
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

    // Step 5: Create and init camera controller
    ret = bk_camera_isp_ctlr_new(&s_isp_camera_handle.camera_ctlr_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to create camera controller\n");
        goto err;
    }

    ret = bk_isp_camera_dev_init(s_isp_camera_handle.camera_ctlr_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to init camera device\n");
        goto err;
    }

    // Step 6: Init camera port
    ret = bk_isp_camera_port_init(s_isp_camera_handle.camera_ctlr_handle, &isp_ctlr_config);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to init camera port\n");
        goto err;
    }

    // Step 7: Init and set sensor format
    ret = bk_camera_sensor_init(s_isp_camera_handle.sensor_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to init sensor\n");
        goto err;
    }

    bk_camera_sensor_format_t format = {
        .width = width,
        .height = height,
        .fps = fps,
    };
    ret = bk_camera_sensor_set_format(s_isp_camera_handle.sensor_handle, &format);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to set sensor format\n");
        goto err;
    }

    s_isp_camera_handle.bus = bus;
    s_isp_camera_handle.is_initialized = 1;
    s_isp_camera_handle.is_mipi = 0;
    s_isp_camera_handle.is_sensor_started = 1;
    LOGI("DVP camera initialized successfully\n");

    return AVDK_ERR_OK;

err:
    if (s_isp_camera_handle.camera_ctlr_handle)
    {
        bk_isp_camera_deinit(s_isp_camera_handle.camera_ctlr_handle);
        bk_isp_camera_delete(s_isp_camera_handle.camera_ctlr_handle);
        s_isp_camera_handle.camera_ctlr_handle = NULL;
    }

    if (s_isp_camera_handle.sensor_handle)
    {
        bk_camera_sensor_destroy(s_isp_camera_handle.sensor_handle);
        s_isp_camera_handle.sensor_handle = NULL;
    }

    if (bus)
    {
        bk_camera_bus_disable(bus);
        bk_camera_bus_delete(bus);
    }

    s_isp_camera_handle.is_sensor_started = 0;
    return ret;
}

/**
 * @brief Open DVP camera (legacy function, kept for compatibility)
 * @param sensor_width Sensor output width
 * @param sensor_height Sensor output height
 * @param fps Frame rate
 * @param chnl_id Channel ID (0 for MP, 1 for SP)
 * @param flexa_mode Flexa mode (0: frame, 1: flexa mode)
 * @param output_fmt Output pixel format
 * @param isp_output_width ISP output width
 * @param isp_output_height ISP output height
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_open_dvp_camera(uint16_t sensor_width, uint16_t sensor_height, uint16_t fps,
                                      uint8_t chnl_id, uint8_t flexa_mode, bk_pixel_format_t output_fmt,
                                      uint16_t isp_output_width, uint16_t isp_output_height)
{
    avdk_err_t ret;

    // Initialize camera if not already initialized (use sensor resolution)
    if (!s_isp_camera_handle.is_initialized)
    {
        ret = isp_init_dvp_camera(sensor_width, sensor_height, fps);
        if (ret != AVDK_ERR_OK)
        {
            return ret;
        }
    }

    // Create and start instance (use ISP output resolution)
    return isp_create_instance(chnl_id, isp_output_width, isp_output_height, flexa_mode, output_fmt);
}

/**
 * @brief Open ISP camera based on parameters
 * @param is_mipi True for MIPI CSI, false for DVP
 * @param is_mp True for MP channel, false for SP channel
 * @param sensor_width Sensor output width
 * @param sensor_height Sensor output height
 * @param fps Frame rate
 * @param flexa_mode Flexa mode (0: frame, 1: flexa)
 * @param output_fmt Output pixel format
 * @param isp_output_width ISP output width
 * @param isp_output_height ISP output height
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_open_camera(uint8_t is_mipi, uint8_t is_mp,
                                   uint16_t sensor_width, uint16_t sensor_height, uint16_t fps,
                                   uint8_t flexa_mode, bk_pixel_format_t output_fmt,
                                   uint16_t isp_output_width, uint16_t isp_output_height)
{
    uint8_t chnl_id = is_mp ? 0 : 1; // 0: MP, 1: SP

    if (is_mipi)
    {
        return isp_open_mipi_camera(sensor_width, sensor_height, fps, chnl_id, flexa_mode, output_fmt,
                                     isp_output_width, isp_output_height);
    }
    else
    {
        return isp_open_dvp_camera(sensor_width, sensor_height, fps, chnl_id, flexa_mode, output_fmt,
                                   isp_output_width, isp_output_height);
    }
}

/**
 * @brief Cleanup all ISP resources (callbacks, timer, controller, etc.)
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_cleanup_all_resources(void)
{
    avdk_err_t ret = AVDK_ERR_OK;

    /* Stop the callback capture task before tearing down ISP resources. */
    isp_dvp_capture_stop();
    isp_dvp_unregister_frame_cb();

    // Deinit camera device if initialized
    if (s_isp_camera_handle.is_initialized && s_isp_camera_handle.camera_ctlr_handle)
    {
        ret = bk_isp_camera_deinit(s_isp_camera_handle.camera_ctlr_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("Failed to deinit camera device\n");
        }
    }

    // Delete camera controller if exists
    if (s_isp_camera_handle.camera_ctlr_handle)
    {
        ret = bk_isp_camera_delete(s_isp_camera_handle.camera_ctlr_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("Failed to delete camera controller\n");
        }
        s_isp_camera_handle.camera_ctlr_handle = NULL;
    }

    // Disable and delete camera bus if exists
    if (s_isp_camera_handle.bus)
    {
        ret = bk_camera_bus_disable(s_isp_camera_handle.bus);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("Failed to disable camera bus\n");
        }

        ret = bk_camera_bus_delete(s_isp_camera_handle.bus);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("Failed to delete camera bus\n");
        }
        s_isp_camera_handle.bus = NULL;
    }

    if (s_isp_camera_handle.sensor_handle)
    {
        bk_camera_sensor_destroy(s_isp_camera_handle.sensor_handle);
        s_isp_camera_handle.sensor_handle = NULL;
    }

    // Reset handle state
    s_isp_camera_handle.is_initialized = 0;
    s_isp_camera_handle.is_sensor_started = 0;
    s_isp_camera_handle.mp_frame_size = 0;
    s_isp_camera_handle.sp_frame_size = 0;
    s_isp_camera_handle.mp_width = 0;
    s_isp_camera_handle.mp_height = 0;
    s_isp_camera_handle.sp_width = 0;
    s_isp_camera_handle.sp_height = 0;

    LOGI("All ISP resources cleaned up\n");
    return AVDK_ERR_OK;
}

/**
 * @brief Close a specific channel (MP or SP)
 * @param is_mp True for MP channel, false for SP channel
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_close_channel(uint8_t is_mp)
{
    avdk_err_t ret = AVDK_ERR_OK;
    const char *chnl_name = is_mp ? "MP" : "SP";
    uint8_t chnl_id = is_mp ? ISP_MP_CHN_ID : ISP_SP_CHN_ID;

    if (s_isp_dvp_capture.running && s_isp_dvp_capture.chnl_id == chnl_id)
    {
        isp_dvp_capture_stop();
        isp_dvp_unregister_frame_cb();
    }

    // Stop instance using standard API
    ret = bk_isp_camera_channel_close(s_isp_camera_handle.camera_ctlr_handle, chnl_id);

    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to stop %s instance: %d\n", chnl_name, ret);
        return ret;  // Return error, keep instance_handle for retry
    }

    if (is_mp)
    {
        s_isp_camera_handle.mp_frame_size = 0;
        s_isp_camera_handle.mp_width = 0;
        s_isp_camera_handle.mp_height = 0;
    }
    else
    {
        s_isp_camera_handle.sp_frame_size = 0;
        s_isp_camera_handle.sp_width = 0;
        s_isp_camera_handle.sp_height = 0;
    }

    // Check if both channels are closed, if so, cleanup all resources
    if (bk_isp_camera_channel_state_get(s_isp_camera_handle.camera_ctlr_handle, ISP_MP_CHN_ID) == ISP_CHANNEL_STATE_TURN_OFF
        && bk_isp_camera_channel_state_get(s_isp_camera_handle.camera_ctlr_handle, ISP_SP_CHN_ID) == ISP_CHANNEL_STATE_TURN_OFF)
    {
        ret = isp_cleanup_all_resources();
    }

    return ret;
}

static avdk_err_t isp_read_channel(uint8_t is_mp)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    const char *chnl_name = is_mp ? "MP" : "SP";
    uint8_t chnl_id = is_mp ? ISP_MP_CHN_ID : ISP_SP_CHN_ID;

    uint32_t frame_size = is_mp ? s_isp_camera_handle.mp_frame_size : s_isp_camera_handle.sp_frame_size;
    if (frame_size == 0)
    {
        LOGE("Frame size is 0\n");
        goto exit;
    }

    uint8_t *frame_buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
    if (!frame_buffer)
    {
        LOGE("Failed to malloc frame buffer\n");
        ret = AVDK_ERR_NOMEM;
        goto exit;
    }

    ret = bk_isp_camera_read(s_isp_camera_handle.camera_ctlr_handle, chnl_id, frame_buffer, frame_size, -1);

    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to read frame\n");
        goto exit;
    }

    LOGI("read %s frame success, size: %d\n", chnl_name, frame_size);

    for (int i = 0; i < frame_size; i++)
    {
        BK_LOG_RAW("%02x ", frame_buffer[i]);
        if ((i + 1) % 32 == 0)
        {
            BK_LOG_RAW("\n");
        }
    }

    BK_LOG_RAW("\n");

    ret = AVDK_ERR_OK;

exit:
    if (frame_buffer)
    {
        bk_frame_buffer_free(frame_buffer);
    }

    return ret;
}

/**
 * @brief Default demo frame callback used by the "isp dvp_cb on" CLI command.
 *
 * Prints frame attributes plus the first bytes of the payload so the path can
 * be validated without extra tooling. Real applications register their own
 * callback via isp_dvp_register_frame_cb().
 */
static void isp_dvp_demo_frame_cb(uint8_t *frame, uint32_t size,
                                  uint16_t width, uint16_t height,
                                  bk_pixel_format_t fmt, void *user_arg)
{
    static uint32_t s_frame_index = 0;

    LOGI("dvp_cb frame[%u]: %dx%d fmt=%d size=%u head=%02x %02x %02x %02x\n",
        s_frame_index++, width, height, fmt, size,
        (size > 0) ? frame[0] : 0,
        (size > 1) ? frame[1] : 0,
        (size > 2) ? frame[2] : 0,
        (size > 3) ? frame[3] : 0);
}

/**
 * @brief DVP/ISP capture task: read frames in a loop and deliver via callback.
 */
static void isp_dvp_capture_task(beken_thread_arg_t arg)
{
    isp_dvp_capture_ctx_t *ctx = (isp_dvp_capture_ctx_t *)arg;
    uint8_t is_mp = (ctx->chnl_id == ISP_MP_CHN_ID);

    LOGI("dvp capture task start, channel: %s\n", is_mp ? "MP" : "SP");

    while (ctx->running)
    {
        uint32_t frame_size = is_mp ? s_isp_camera_handle.mp_frame_size
                                    : s_isp_camera_handle.sp_frame_size;
        if (frame_size == 0)
        {
            LOGE("dvp capture: frame size is 0, channel not opened?\n");
            break;
        }

        uint8_t *frame_buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
        if (!frame_buffer)
        {
            LOGE("dvp capture: failed to malloc frame buffer\n");
            rtos_delay_milliseconds(10);
            continue;
        }

        avdk_err_t ret = bk_isp_camera_read(s_isp_camera_handle.camera_ctlr_handle,
                                            ctx->chnl_id, frame_buffer, frame_size,
                                            ISP_DVP_CAPTURE_READ_TIMEOUT_MS);
        if (ret == AVDK_ERR_OK)
        {
            isp_dvp_frame_cb_t cb = ctx->cb;
            if (cb)
            {
                uint16_t w = is_mp ? s_isp_camera_handle.mp_width : s_isp_camera_handle.sp_width;
                uint16_t h = is_mp ? s_isp_camera_handle.mp_height : s_isp_camera_handle.sp_height;
                bk_pixel_format_t fmt = is_mp ? s_isp_camera_handle.mp_fmt : s_isp_camera_handle.sp_fmt;
                cb(frame_buffer, frame_size, w, h, fmt, ctx->user_arg);
            }
        }
        else
        {
            /* Timeout/transient error: keep looping so the stop flag is honored. */
            LOGD("dvp capture: read frame ret=%d\n", ret);
        }

        bk_frame_buffer_free(frame_buffer);
    }

    LOGI("dvp capture task exit, channel: %s\n", is_mp ? "MP" : "SP");

    ctx->thread = NULL;
    rtos_delete_thread(NULL);
}

avdk_err_t isp_dvp_register_frame_cb(isp_dvp_frame_cb_t cb, void *user_arg)
{
    s_isp_dvp_capture.cb = cb;
    s_isp_dvp_capture.user_arg = user_arg;
    return AVDK_ERR_OK;
}

avdk_err_t isp_dvp_unregister_frame_cb(void)
{
    s_isp_dvp_capture.cb = NULL;
    s_isp_dvp_capture.user_arg = NULL;
    return AVDK_ERR_OK;
}

avdk_err_t isp_dvp_capture_start(uint8_t is_mp)
{
    if (s_isp_dvp_capture.running || s_isp_dvp_capture.thread)
    {
        LOGW("dvp capture already running\n");
        return AVDK_ERR_OK;
    }

    if (!s_isp_camera_handle.is_initialized || !s_isp_camera_handle.camera_ctlr_handle)
    {
        LOGE("camera not initialized, open the channel before starting capture\n");
        return AVDK_ERR_INVAL;
    }

    uint8_t chnl_id = is_mp ? ISP_MP_CHN_ID : ISP_SP_CHN_ID;
    uint32_t frame_size = is_mp ? s_isp_camera_handle.mp_frame_size
                                : s_isp_camera_handle.sp_frame_size;
    if (frame_size == 0)
    {
        LOGE("channel %s not opened (frame size is 0)\n", is_mp ? "MP" : "SP");
        return AVDK_ERR_INVAL;
    }

    s_isp_dvp_capture.chnl_id = chnl_id;
    s_isp_dvp_capture.running = 1;

    bk_err_t os_ret = rtos_create_thread(&s_isp_dvp_capture.thread,
                                         ISP_DVP_CAPTURE_TASK_PRIORITY,
                                         "isp_dvp_cap",
                                         isp_dvp_capture_task,
                                         ISP_DVP_CAPTURE_TASK_STACK_SIZE,
                                         (beken_thread_arg_t)&s_isp_dvp_capture);
    if (os_ret != BK_OK)
    {
        LOGE("failed to create dvp capture task: %d\n", os_ret);
        s_isp_dvp_capture.running = 0;
        s_isp_dvp_capture.thread = NULL;
        return AVDK_ERR_GENERIC;
    }

    LOGI("dvp capture started on channel %s\n", is_mp ? "MP" : "SP");
    return AVDK_ERR_OK;
}

avdk_err_t isp_dvp_capture_stop(void)
{
    if (!s_isp_dvp_capture.running && !s_isp_dvp_capture.thread)
    {
        LOGW("dvp capture not running\n");
        return AVDK_ERR_OK;
    }

    /* Ask the task to finish; it self-deletes after the current read returns
     * (bounded by ISP_DVP_CAPTURE_READ_TIMEOUT_MS). */
    s_isp_dvp_capture.running = 0;

    uint32_t wait_ms = 0;
    while (s_isp_dvp_capture.thread && wait_ms < (ISP_DVP_CAPTURE_READ_TIMEOUT_MS + 500))
    {
        rtos_delay_milliseconds(10);
        wait_ms += 10;
    }

    if (s_isp_dvp_capture.thread)
    {
        LOGW("dvp capture task did not exit in time\n");
    }

    LOGI("dvp capture stopped\n");
    return AVDK_ERR_OK;
}

void cli_isp_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;
    char *msg = NULL;

    if (argc < 2)
    {
        LOGE("Usage: isp <command>\n");
        goto exit;
    }

    if (os_strcmp(argv[1], "open") == 0)
    {
        // Parse command: isp open mipi mp 1920 1080 20 1920 1080 frame [output_fmt]
        // Parameter order: sensor_width, sensor_height, fps, isp_output_width, isp_output_height, flexa_mode, [output_fmt]
        // output_fmt defaults to 23 (NV12) if not provided
        if (argc < 10)
        {
            LOGE("Usage: isp open <mipi|dvp> <mp|sp> <sensor_width> <sensor_height> <fps> <isp_output_width> <isp_output_height> <frame|software|hardware> [output_fmt]\n");
            LOGE("  sensor_width/height: sensor output resolution (ISP input)\n");
            LOGE("  isp_output_width/height: ISP output resolution\n");
            LOGE("  output_fmt: pixel format number (default: 23 for NV12)\n");
            goto exit;
        }

        // Parse camera type
        uint8_t is_mipi = 0;
        if (os_strcmp(argv[2], "mipi") == 0)
        {
            is_mipi = 1;
        }
        else if (os_strcmp(argv[2], "dvp") == 0)
        {
            is_mipi = 0;
        }
        else
        {
            LOGE("Invalid camera type: %s (expected mipi or dvp)\n", argv[2]);
            goto exit;
        }

        // Parse channel type
        uint8_t is_mp = 0;
        if (os_strcmp(argv[3], "mp") == 0)
        {
            is_mp = 1;
        }
        else if (os_strcmp(argv[3], "sp") == 0)
        {
            is_mp = 0;
        }
        else
        {
            LOGE("Invalid channel type: %s (expected mp or sp)\n", argv[3]);
            goto exit;
        }

        // Parse sensor resolution and fps
        uint16_t sensor_width = (uint16_t)os_strtoul(argv[4], NULL, 10);
        uint16_t sensor_height = (uint16_t)os_strtoul(argv[5], NULL, 10);
        uint16_t fps = (uint16_t)os_strtoul(argv[6], NULL, 10);

        if (sensor_width == 0 || sensor_height == 0 || fps == 0)
        {
            LOGE("Invalid parameters: sensor_width=%d, sensor_height=%d, fps=%d\n", sensor_width, sensor_height, fps);
            goto exit;
        }

        // Parse ISP output resolution (after fps)
        uint16_t isp_output_width = (uint16_t)os_strtoul(argv[7], NULL, 10);
        uint16_t isp_output_height = (uint16_t)os_strtoul(argv[8], NULL, 10);

        if (isp_output_width == 0 || isp_output_height == 0)
        {
            LOGE("Invalid ISP output resolution: width=%d, height=%d\n", isp_output_width, isp_output_height);
            goto exit;
        }

        // Parse flexa mode
        uint8_t flexa_mode = 0;
        if (os_strcmp(argv[9], "frame") == 0)
        {
            flexa_mode = 0; // Frame mode
        }
        else if (os_strcmp(argv[9], "flexa") == 0)
        {
            flexa_mode = 1; // flexa
        }
        else
        {
            LOGE("Invalid flexa mode: %s (expected frame, software, or hardware)\n", argv[9]);
            goto exit;
        }

        // Parse output format (default: 23 for NV12)
        bk_pixel_format_t output_fmt = BK_PIXEL_FORMAT_NV12; // Default to NV12 (21)
        if (argc == 11)
        {
            // output_fmt is provided at argv[10]
            uint32_t fmt_value = os_strtoul(argv[10], NULL, 10);
            output_fmt = (bk_pixel_format_t)fmt_value;
        }

        // Open camera
        ret = isp_open_camera(is_mipi, is_mp, sensor_width, sensor_height, fps, flexa_mode, output_fmt,
                              isp_output_width, isp_output_height);
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        // Parse command: isp close <mp|sp>
        if (argc < 3)
        {
            LOGE("Usage: isp close <mp|sp>\n");
            goto exit;
        }

        // Parse channel type
        uint8_t is_mp = 0;
        if (os_strcmp(argv[2], "mp") == 0)
        {
            is_mp = 1;
        }
        else if (os_strcmp(argv[2], "sp") == 0)
        {
            is_mp = 0;
        }
        else
        {
            LOGE("Invalid channel type: %s (expected mp or sp)\n", argv[2]);
            goto exit;
        }

        ret = isp_close_channel(is_mp);
    }
    else if (os_strcmp(argv[1], "read") == 0)
    {
        if (argc < 3)
        {
            LOGE("Usage: isp read <mp|sp>\n");
            goto exit;
        }

        uint8_t is_mp = 0;
        if (os_strcmp(argv[2], "mp") == 0)
        {
            is_mp = 1;
        }
        else if (os_strcmp(argv[2], "sp") == 0)
        {
            is_mp = 0;
        }
        else
        {
            LOGE("Invalid channel type: %s (expected mp or sp)\n", argv[2]);
            goto exit;
        }

        ret = isp_read_channel(is_mp);
        LOGI("read frame from channel: %d\n", is_mp ? 0 : 1, ret);
    }
    else if (os_strcmp(argv[1], "dvp_cb") == 0)
    {
        // Parse command: isp dvp_cb on [mp|sp] | isp dvp_cb off
        if (argc < 3)
        {
            LOGE("Usage: isp dvp_cb <on|off> [mp|sp]\n");
            LOGE("  on : register demo frame callback and start capture task\n");
            LOGE("  off: stop capture task and clear callback\n");
            LOGE("  default channel is mp\n");
            goto exit;
        }

        if (os_strcmp(argv[2], "on") == 0)
        {
            uint8_t is_mp = 1; // default MP channel
            if (argc >= 4)
            {
                if (os_strcmp(argv[3], "mp") == 0)
                {
                    is_mp = 1;
                }
                else if (os_strcmp(argv[3], "sp") == 0)
                {
                    is_mp = 0;
                }
                else
                {
                    LOGE("Invalid channel type: %s (expected mp or sp)\n", argv[3]);
                    goto exit;
                }
            }

            ret = isp_dvp_register_frame_cb(isp_dvp_demo_frame_cb, NULL);
            if (ret != AVDK_ERR_OK)
            {
                goto exit;
            }

            ret = isp_dvp_capture_start(is_mp);
            if (ret != AVDK_ERR_OK)
            {
                isp_dvp_unregister_frame_cb();
            }
        }
        else if (os_strcmp(argv[2], "off") == 0)
        {
            ret = isp_dvp_capture_stop();
            isp_dvp_unregister_frame_cb();
        }
        else
        {
            LOGE("Invalid dvp_cb option: %s (expected on or off)\n", argv[2]);
        }
    }
    else if (os_strcmp(argv[1], "detect") == 0)
    {
        int sensor_count = isp_detect_all_sensors();
        ret = (sensor_count > 0) ? AVDK_ERR_OK : AVDK_ERR_NODEV;
    }
    else if (os_strcmp(argv[1], "sensor_open") == 0)
    {
        uint16_t width = (uint16_t)os_strtoul(argv[2], NULL, 10);
        uint16_t height = (uint16_t)os_strtoul(argv[3], NULL, 10);
        uint8_t fps = (uint8_t)os_strtoul(argv[4], NULL, 10);
        LOGI("sensor_open width: %d, height: %d, fps: %d\n", width, height, fps);
        ret = isp_init_mipi_camera(width, height, fps, 1);
        //mipi_controller_init(width, height, 0x2B);
    }
    else if (os_strcmp(argv[1], "init") == 0)
    {
        uint8_t param = os_strtoul(argv[2], NULL, 10);
        LOGI("init param: %d\n", param);
        isp_ini(&param);
        ret = AVDK_ERR_OK;
    }
    else if (os_strcmp(argv[1], "soft_reset") == 0)
    {
        ret = bk_isp_camera_ctlr_ioctl(s_isp_camera_handle.camera_ctlr_handle, BK_CAM_IOCTL_SOFTRESET, NULL);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("soft reset failed: %d\n", ret);
        }
    }
    else
    {
        LOGE("Usage: isp <command>\n");
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
