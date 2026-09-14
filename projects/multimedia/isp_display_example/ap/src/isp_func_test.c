#include "include/isp_cli.h"
#include "include/isp_display_pipeline.h"
#include "include/isp_vc_route.h"

#include <components/bk_camera_sensor.h>
#include <components/bk_camera_bus.h>
#include <components/bk_camera_configs.h>
#include <components/bk_isp_camera.h>
#include <components/bk_camera_isp_ctlr.h>
#include <avdk_check.h>
#include <isp_camera_ctlr.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include <driver/mipi_csi.h>
#include <components/bk_frame_buffer.h>
#include <modules/pm.h>
#include <sys_types.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

/* TP2863 doorbell_lp board: I2C1 @ GPIO69/70, reset GPIO71, no SoC MCLK */
#define ISP_TP2863_PIN_SCL    GPIO_69
#define ISP_TP2863_PIN_SDA    GPIO_70
#define ISP_TP2863_PIN_RESET  GPIO_71
#define ISP_TP2863_I2C_ID     1

static void isp_tp2863_bus_config(bk_camera_bus_config_t *bus_config)
{
    *bus_config = (bk_camera_bus_config_t)CSI_CAM_BUS_I2C1_8BIT_2000TIMEOUT();
    bus_config->pin_scl = ISP_TP2863_PIN_SCL;
    bus_config->pin_sda = ISP_TP2863_PIN_SDA;
    bus_config->i2c_id = ISP_TP2863_I2C_ID;
    bus_config->pin_xclk = BK_CAMERA_PIN_INVALID;
}

static void isp_tp2863_sensor_config(bk_camera_sensor_config_t *sensor_config)
{
    os_memset(sensor_config, 0, sizeof(*sensor_config));
    sensor_config->pin_reset = ISP_TP2863_PIN_RESET;
    sensor_config->pin_pwdn = 0xFF;
}

static avdk_err_t isp_tp2863_camera_power(bool enable)
{
    int ldo_en = enable ? PM_AUXLDO_ENABLE : PM_AUXLDO_DISABLE;
    pm_auxldo_ctrl_cfg_t cfg = {0};

    cfg.ldo = AUXLDOS_SEL_1P8V;
    cfg.out = PM_AUXLDO_1P8V_OUT_1P8V;
    cfg.user = PM_AUXLDO_USER_CAMERA;
    cfg.state = ldo_en;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&cfg), TAG, "camera 1p8v ldo vote failed");

    cfg = (pm_auxldo_ctrl_cfg_t){0};
    cfg.ldo = AUXLDOS_SEL_1P2V;
    cfg.out = PM_AUXLDO_1P2V_OUT_1P2V;
    cfg.user = PM_AUXLDO_USER_CAMERA;
    cfg.state = ldo_en;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&cfg), TAG, "camera 1p2v ldo vote failed");
    rtos_delay_milliseconds(1);
    return AVDK_ERR_OK;
}

// ISP camera handle structure
typedef struct {
    bk_camera_bus_t *bus;
    bk_camera_sensor_handle_t sensor_handle;
    bk_isp_camera_ctlr_handle_t camera_ctlr_handle;
    uint32_t mp_frame_size;
    uint32_t sp_frame_size;
    /* Per-channel output geometry/format, recorded at channel open time. */
    uint16_t mp_width;
    uint16_t mp_height;
    uint16_t sp_width;
    uint16_t sp_height;
    bk_pixel_format_t mp_fmt;
    bk_pixel_format_t sp_fmt;
    uint8_t is_initialized;  // Flag to indicate if camera is initialized
    uint8_t is_sensor_started;  // Flag to indicate if sensor streaming is started
    void *isp_handle;
} isp_camera_handle_t;

static isp_camera_handle_t s_isp_camera_handle = {0};

/* Forward declarations for functions used before their definitions. */
static avdk_err_t isp_cleanup_all_resources(void);
static avdk_err_t isp_close_channel(uint8_t is_mp);

/**
 * @brief Detect sensor on a specific camera port and print its information
 * @param port Camera port (CSI_CAMERA_PORT)
 * @param bus_config Bus configuration for the port
 * @param sensor_config Sensor configuration
 * @param port_name Port name string for logging (e.g., "CSI")
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
 * @brief Detect CSI sensor
 * @return Number of sensors detected
 */
int isp_detect_all_sensors(void)
{
    int sensor_count = 0;
    bk_camera_bus_config_t bus_config;
    bk_camera_sensor_config_t sensor_config;

    LOGI("Starting CSI sensor detection...\n");

    isp_tp2863_bus_config(&bus_config);
    isp_tp2863_sensor_config(&sensor_config);
    sensor_count += isp_detect_sensor_on_port(CSI_CAMERA_PORT, &bus_config, &sensor_config, "CSI");

    LOGI("CSI sensor detection complete. Found %d sensor(s).\n", sensor_count);

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
    bk_camera_bus_config_t bus_config;
    bk_camera_sensor_config_t sensor_config;

    isp_tp2863_bus_config(&bus_config);
    isp_tp2863_sensor_config(&sensor_config);

    LOGI("Initializing MIPI CSI camera (TP2863): %dx%d @ %dfps\n", width, height, fps);

    AVDK_GOTO_ON_ERROR(isp_tp2863_camera_power(true), err, TAG, "camera power on failed");

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

        /* doorbell: start sensor (MIPI/TP2863) before ISP port init */
        bk_mipi_csi_set_default_vc(0);
        ret = bk_camera_sensor_init(s_isp_camera_handle.sensor_handle);
        if (ret != AVDK_ERR_OK) {
            LOGE("Failed to init sensor\n");
            goto err;
        }
        {
            bk_camera_sensor_format_t format = {
                .width = width,
                .height = height,
                .fps = fps,
            };
            ret = bk_camera_sensor_set_format(s_isp_camera_handle.sensor_handle, &format);
            if (ret != AVDK_ERR_OK) {
                LOGE("Failed to set sensor format\n");
                goto err;
            }
        }
        s_isp_camera_handle.is_sensor_started = 1;

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

        {
            bk_camera_isp_ctlr_t *control = __containerof(s_isp_camera_handle.camera_ctlr_handle,
                                                          bk_camera_isp_ctlr_t, ops);
            s_isp_camera_handle.isp_handle = control->isp_handle;
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

    (void)isp_tp2863_camera_power(false);
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
    uint8_t port_id = ISP_MIPI_PORT_ID;
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
    if (chnl_id == 0) {
        /* doorbell_lp: MP frame mode for LCD, larger buffer pool for vc_mux */
        instance.enable_flexa = 0;
        instance.work_mode = 0;
        instance.buf_cnt = 7;
    }

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

    if (isp_vc_route_is_active()) {
        LOGE("vc route is active, close it first: isp close_vc_route\n");
        return AVDK_ERR_BUSY;
    }

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

    if (chnl_id == 0) {
        ret = isp_display_pipeline_start(s_isp_camera_handle.camera_ctlr_handle,
                                         isp_output_width, isp_output_height);
        if (ret != AVDK_ERR_OK) {
            LOGE("Failed to start display pipeline\n");
            isp_close_channel(chnl_id == 0);
            return ret;
        }
    }

    return AVDK_ERR_OK;
}

/**
 * @brief Cleanup all ISP resources (callbacks, timer, controller, etc.)
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_cleanup_all_resources(void)
{
    avdk_err_t ret = AVDK_ERR_OK;

    (void)isp_display_pipeline_stop();

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
    s_isp_camera_handle.isp_handle = NULL;

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

    if (is_mp) {
        (void)isp_display_pipeline_stop();
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

void cli_isp_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;
    char *msg = NULL;

    if (argc < 2)
    {
        LOGE("Usage: isp <command>\n");
        goto exit;
    }

    if (os_strcmp(argv[1], "open_vc_route") == 0)
    {
        uint16_t sensor_w = (argc > 2) ? (uint16_t)os_strtoul(argv[2], NULL, 10) : 1280;
        uint16_t sensor_h = (argc > 3) ? (uint16_t)os_strtoul(argv[3], NULL, 10) : 720;
        uint16_t fps = (argc > 4) ? (uint16_t)os_strtoul(argv[4], NULL, 10) : 25;
        uint16_t isp_w = (argc > 5) ? (uint16_t)os_strtoul(argv[5], NULL, 10) : sensor_w;
        uint16_t isp_h = (argc > 6) ? (uint16_t)os_strtoul(argv[6], NULL, 10) : sensor_h;
        uint8_t default_vc = (argc > 7) ? (uint8_t)os_strtoul(argv[7], NULL, 10) : 0;

        if (default_vc > 1) {
            LOGE("Usage: isp open_vc_route [sensor_w] [sensor_h] [fps] [isp_w] [isp_h] [0|1]\n");
            goto exit;
        }
        if (s_isp_camera_handle.is_initialized) {
            LOGE("single camera already open, close it first\n");
            goto exit;
        }
        ret = isp_vc_route_turn_on(sensor_w, sensor_h, fps, isp_w, isp_h, default_vc);
    }
    else if (os_strcmp(argv[1], "close_vc_route") == 0)
    {
        ret = isp_vc_route_turn_off();
    }
    else if (os_strcmp(argv[1], "vc_route_enable") == 0)
    {
        if (argc < 3) {
            LOGE("Usage: isp vc_route_enable <0|1> [discard_frames]\n");
            goto exit;
        }
        uint8_t vc = (uint8_t)os_strtoul(argv[2], NULL, 10);
        uint8_t discard = (argc > 3) ? (uint8_t)os_strtoul(argv[3], NULL, 10) : 0;
        ret = isp_vc_route_vc_enable(vc, discard);
    }
    else if (os_strcmp(argv[1], "vc_route_disable") == 0)
    {
        if (argc < 3) {
            LOGE("Usage: isp vc_route_disable <0|1>\n");
            goto exit;
        }
        uint8_t vc = (uint8_t)os_strtoul(argv[2], NULL, 10);
        ret = isp_vc_route_vc_disable(vc);
    }
    else if (os_strcmp(argv[1], "vc_route_vc") == 0)
    {
        if (argc < 3) {
            LOGE("Usage: isp vc_route_vc <0|1>\n");
            goto exit;
        }
        uint8_t vc = (uint8_t)os_strtoul(argv[2], NULL, 10);
        ret = isp_vc_route_select(vc);
    }
    else if (os_strcmp(argv[1], "open") == 0)
    {
        if (isp_vc_route_is_active()) {
            LOGE("vc route is active, use isp close_vc_route first\n");
            goto exit;
        }
        // Parse command: isp open mp 1920 1080 20 1920 1080 frame [output_fmt]
        if (argc < 9)
        {
            LOGE("Usage: isp open <mp|sp> <sensor_width> <sensor_height> <fps> <isp_output_width> <isp_output_height> <frame|flexa> [output_fmt]\n");
            LOGE("  sensor_width/height: sensor output resolution (ISP input)\n");
            LOGE("  isp_output_width/height: ISP output resolution\n");
            LOGE("  output_fmt: pixel format number (default: 23 for NV12)\n");
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

        uint16_t sensor_width = (uint16_t)os_strtoul(argv[3], NULL, 10);
        uint16_t sensor_height = (uint16_t)os_strtoul(argv[4], NULL, 10);
        uint16_t fps = (uint16_t)os_strtoul(argv[5], NULL, 10);

        if (sensor_width == 0 || sensor_height == 0 || fps == 0)
        {
            LOGE("Invalid parameters: sensor_width=%d, sensor_height=%d, fps=%d\n", sensor_width, sensor_height, fps);
            goto exit;
        }

        uint16_t isp_output_width = (uint16_t)os_strtoul(argv[6], NULL, 10);
        uint16_t isp_output_height = (uint16_t)os_strtoul(argv[7], NULL, 10);

        if (isp_output_width == 0 || isp_output_height == 0)
        {
            LOGE("Invalid ISP output resolution: width=%d, height=%d\n", isp_output_width, isp_output_height);
            goto exit;
        }

        uint8_t flexa_mode = 0;
        if (os_strcmp(argv[8], "frame") == 0)
        {
            flexa_mode = 0; // Frame mode
        }
        else if (os_strcmp(argv[8], "flexa") == 0)
        {
            flexa_mode = 1; // flexa
        }
        else
        {
            LOGE("Invalid output mode: %s (expected frame or flexa)\n", argv[8]);
            goto exit;
        }

        bk_pixel_format_t output_fmt = BK_PIXEL_FORMAT_NV12;
        if (argc == 10)
        {
            uint32_t fmt_value = os_strtoul(argv[9], NULL, 10);
            output_fmt = (bk_pixel_format_t)fmt_value;
        }

        uint8_t chnl_id = is_mp ? 0 : 1;
        ret = isp_open_mipi_camera(sensor_width, sensor_height, fps, chnl_id, flexa_mode, output_fmt,
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
