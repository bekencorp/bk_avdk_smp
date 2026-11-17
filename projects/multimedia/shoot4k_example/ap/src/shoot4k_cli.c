/**
 * @file shoot4k_cli.c
 * @brief Standalone 4K shoot pipeline for BK7259:
 *        4K RAW sensor capture -> RDMA half-frame replay -> stitched 4K 16bpp
 *        frame (RGB565 or YUYV422, selectable at runtime), with optional JPEG
 *        encode (VCENC) and UART raw dump.
 *
 * Flow (see isp_shoot_4k_capture()):
 *   pass 1: open the probe mode NV12, probe AE (exposure time / analog gain / mean luma)
 *   pass 2: reopen the RAW capture mode, borrow one RAW frame, lock sensor exposure
 *   pass 3: open the half-frame output mode with the borrowed RAW buffer poked
 *           into the MP channel, replay the borrowed RAW half-frame twice
 *           through the RDMA input, then stitch the two halves in place into
 *           one full-resolution 16bpp frame (the RAW buffer is reused as the output).
 * All resolutions and frame rates come from the central configuration block below.
 */

#include "include/shoot4k_cli.h"

#include "cli.h"

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
#include <cache.h>
#include <driver/uart.h>
#include <driver/isp.h>
#include <modules/veri_isp/mpi_isp_wb.h>
#include <driver/sys_pm.h>
#include <common/avdk_pixel_types.h>
#if CONFIG_BK_H264E
#include <modules/pm.h>
#include <modules/h264e/jpegencapi.h>
#include "h264_encoder_api.h"
#endif

/* CISP clock: isp_clock_enable() in isp_core.c programs CKSEL_CISP_240M / ckdiv 0
 * during bk_isp_dev_init(); re-apply the application-selected value afterwards.
 * ckdiv is the 2-bit divider field, same encoding the driver passes. */
#define SHOOT4K_CISP_CKSEL   CKSEL_CISP_240M
#define SHOOT4K_CISP_CKDIV   0

/* ==================== shoot4k 分辨率/帧率集中配置区 ====================
 * 全流水线的分辨率与帧率只在本区修改，其余代码均由这些宏推导。
 * 约束：各 Pass 的组合必须是 sensor 驱动支持的格式，否则首次 open 时
 *       isp_check_sensor_resolution() 会直接失败。
 */
/* Pass 1：AE/AWB 预采样（1080p sensor 模式） */
#define SHOOT4K_PROBE_WIDTH           1920U
#define SHOOT4K_PROBE_HEIGHT          1080U
#define SHOOT4K_PROBE_FPS             15U

/* Pass 2：4K RAW 抓帧（4K sensor 模式） */
#define SHOOT4K_RAW_WIDTH             3840U
#define SHOOT4K_RAW_HEIGHT            2160U
#define SHOOT4K_RAW_FPS               15U

/* 最终输出帧像素尺寸（4K 拼接结果） */
#define SHOOT4K_OUT_WIDTH             3840U
#define SHOOT4K_OUT_HEIGHT            2160U

/* Pass 3：半帧回放通道，宽度为输出宽度的一半、高度相同 */
#define SHOOT4K_HALF_WIDTH            (SHOOT4K_OUT_WIDTH / 2U)
#define SHOOT4K_HALF_HEIGHT           (SHOOT4K_OUT_HEIGHT)
#define SHOOT4K_HALF_FPS              30U

/* 16bpp 单像素 2 字节，半帧行字节数与右半帧偏移相同 */
#define SHOOT4K_POKE_STRIDE           (SHOOT4K_HALF_WIDTH * 2U)

/* 4K pipeline output path: 16bpp in-place output (runtime selectable via
 * `shoot4k fmt`: RGB565 or YUYV422), written back into the borrowed 4K RAW
 * buffer (one 16.6MB buffer). */
#define SHOOT4K_OUT_BUF_SIZE         (SHOOT4K_OUT_WIDTH * SHOOT4K_OUT_HEIGHT * 2U)
#define SHOOT4K_POKE_SECOND_OFFSET   (SHOOT4K_HALF_WIDTH * 2U)
/* MI line length value; if the hardware interprets LLENGTH as bytes this must
 * become SHOOT4K_OUT_WIDTH * 2 (bytes per full 16bpp line, see the bring-up
 * notes). */
#define SHOOT4K_MI_Y_LLENGTH_VALUE   (SHOOT4K_HALF_WIDTH * 2U)

typedef enum {
    SHOOT4K_FMT_RGB565 = 0,
    SHOOT4K_FMT_YUYV422,
} shoot4k_out_fmt_t;

static shoot4k_out_fmt_t s_shoot4k_out_fmt = SHOOT4K_FMT_YUYV422;

static bk_pixel_format_t shoot4k_out_bk_fmt(void)
{
    return (s_shoot4k_out_fmt == SHOOT4K_FMT_YUYV422)
               ? BK_PIXEL_FORMAT_YUYV
               : BK_PIXEL_FORMAT_RGB565;
}

static const char *shoot4k_out_fmt_name(void)
{
    return (s_shoot4k_out_fmt == SHOOT4K_FMT_YUYV422) ? "yuyv422" : "rgb565";
}

/* ISP camera handle */
typedef struct {
    bk_camera_bus_t *bus;
    bk_camera_sensor_handle_t sensor_handle;
    bk_isp_camera_ctlr_handle_t camera_ctlr_handle;
    uint32_t mp_frame_size;
    uint32_t sp_frame_size;
    uint16_t mp_width;
    uint16_t mp_height;
    uint16_t sp_width;
    uint16_t sp_height;
    bk_pixel_format_t mp_fmt;
    bk_pixel_format_t sp_fmt;
    uint8_t is_initialized;
    uint8_t is_sensor_started;
} isp_camera_handle_t;

static isp_camera_handle_t s_isp_camera_handle = {0};

static avdk_err_t isp_cleanup_all_resources(void);
static avdk_err_t isp_close_channel(uint8_t is_mp);
static isp_control_t *isp_shoot4k_isp_control_get(void);

/* Zero-copy: poke a 4K output buffer into the MP channel at next open. */
static uint8_t s_shoot4k_zc_armed = 0;
/* shoot_4K mode gate: 1 during capture; ISP skips prefetch and publishes from ISR. */
static volatile uint8_t s_shoot4k_mode_active = 0;
static uint8_t *s_shoot4k_poke_ybase = NULL;
static uint8_t *s_shoot4k_poke_ybase1 = NULL;
static uint32_t s_shoot4k_poke_ystride = 0;
/* Borrowed buffer kept across channel close; slot detached at deinit. */
static uint8_t s_shoot4k_borrow_owned = 0;
static uint8_t s_shoot4k_borrow_chnl = 0;
static uint8_t s_shoot4k_borrow_slot = 0;

/**
 * @brief Check if sensor supports the requested resolution
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

    int found = 0;
    uint16_t max_sensor_width = 0;
    uint16_t max_sensor_height = 0;

    for (uint32_t i = 0; i < format_array.size; i++)
    {
        uint16_t fmt_width = format_array.format_array[i].width;
        uint16_t fmt_height = format_array.format_array[i].height;
        uint16_t fmt_fps = format_array.format_array[i].fps;

        if (fmt_width > max_sensor_width)
        {
            max_sensor_width = fmt_width;
        }
        if (fmt_height > max_sensor_height)
        {
            max_sensor_height = fmt_height;
        }

        if (fmt_width == width && fmt_height == height && fmt_fps == fps)
        {
            found = 1;
            break;
        }
    }

    /* ISP hardware does not support upscaling. */
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
 */
static avdk_err_t isp_init_mipi_camera(uint16_t width, uint16_t height, uint16_t fps)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_camera_bus_t *bus = NULL;
    bk_camera_bus_config_t bus_config = (bk_camera_bus_config_t)CSI_CAM_BUS_I2C1_8BIT_2000TIMEOUT();
    bk_camera_sensor_config_t sensor_config = {
        .pin_reset = GPIO_71,
        .pin_pwdn = 0xFF,
        .bus = bus,
    };

    bus_config.pin_xclk = GPIO_59;

    LOGI("Initializing MIPI CSI camera: %dx%d @ %dfps\n", width, height, fps);

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

    sensor_config.bus = bus;
    s_isp_camera_handle.sensor_handle = bk_camera_sensor_auto_detect(&sensor_config, CSI_CAMERA_PORT);
    if (!s_isp_camera_handle.sensor_handle)
    {
        LOGE("No sensor detected on MIPI CSI port\n");
        ret = AVDK_ERR_NODEV;
        goto err;
    }

    ret = isp_check_sensor_resolution(s_isp_camera_handle.sensor_handle, width, height, fps);
    if (ret != AVDK_ERR_OK)
    {
        goto err;
    }

    {
        bk_isp_camera_ctlr_config_t isp_ctlr_config = CAM_CSI_DEFAULT_RAW10_CONFIG(width, height, fps);

        /* Raw pixel format from the matched sensor mode (fallback: first entry). */
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

        const void *sensor_object = bk_camera_sensor_get_sensor_object(s_isp_camera_handle.sensor_handle);
        if (!sensor_object)
        {
            LOGE("Failed to get sensor object\n");
            ret = AVDK_ERR_GENERIC;
            goto err;
        }
        isp_ctlr_config.sensor_object = sensor_object;

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
        
        /* Override the CISP clock the driver applied in bk_isp_dev_init(). */
        sys_drv_cisp_cksel_clkdiv_set(SHOOT4K_CISP_CKSEL, SHOOT4K_CISP_CKDIV);

        ret = bk_isp_camera_port_init(s_isp_camera_handle.camera_ctlr_handle, &isp_ctlr_config);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("Failed to init camera port\n");
            goto err;
        }
    }

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

    s_isp_camera_handle.is_sensor_started = 0;
    return ret;
}

/**
 * @brief Create and start a camera instance (MP or SP channel)
 */
static avdk_err_t isp_create_instance(uint8_t chnl_id, uint32_t width, uint32_t height,
                                      bk_pixel_format_t output_fmt)
{
    avdk_err_t ret = AVDK_ERR_OK;
    uint8_t port_id = ISP_MIPI_PORT_ID;
    const char *chnl_name = (chnl_id == 0) ? "MP" : "SP";

    if (!s_isp_camera_handle.is_initialized || !s_isp_camera_handle.camera_ctlr_handle)
    {
        LOGE("Camera not initialized, cannot create instance\n");
        return AVDK_ERR_INVAL;
    }

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

    LOGI("Creating %s instance: %dx%d, fmt=%d, mp_frame_size=%d, sp_frame_size=%d\n",
        chnl_name, width, height, output_fmt, s_isp_camera_handle.mp_frame_size, s_isp_camera_handle.sp_frame_size);

    bk_isp_camera_channel_config_t instance = CAM_MP_NV12_RB_INSTANCE_CONFIG(width, height);
    instance.port_id = port_id;
    instance.enable_flexa = 0;
    instance.work_mode = 0; // 0: frame mode, 1: flexa mode
    instance.format = output_fmt;
    instance.buf_cnt = 1U;

    /* Apply the pending zero-copy poke before channel open consumes it. */
    if (s_shoot4k_zc_armed && s_shoot4k_poke_ybase != NULL)
    {
        isp_control_t *isp_control = isp_shoot4k_isp_control_get();

        if (isp_control != NULL && chnl_id == ISP_MP_CHN_ID)
        {
            isp_control->chn[chnl_id].chn_attr.chnFormat.planeFmt[0].bytesPerLine = s_shoot4k_poke_ystride;
            isp_control->chn[chnl_id].frame_buffer[0] = s_shoot4k_poke_ybase;
            if (s_shoot4k_poke_ybase1 != NULL)
            {
                /* Dual-buffer poke: rotate ping-pongs both buffers. */
                isp_control->chn[chnl_id].frame_buffer[1] = s_shoot4k_poke_ybase1;
                instance.buf_cnt = 2;
            }
        }
    }
    else
    {
        /* Drop a stride left over from the previous poked open so normal opens
         * fall back to the driver default layout. */
        isp_control_t *isp_control = isp_shoot4k_isp_control_get();

        if (isp_control != NULL && chnl_id == ISP_MP_CHN_ID)
        {
            isp_control->chn[chnl_id].chn_attr.chnFormat.planeFmt[0].bytesPerLine = 0;
        }
    }

    ret = bk_isp_camera_channel_open(s_isp_camera_handle.camera_ctlr_handle, chnl_id, &instance);

    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to create %s instance\n", chnl_name);
        return ret;
    }

    LOGI("%s instance created and started successfully\n", chnl_name);
    return AVDK_ERR_OK;
}

/**
 * @brief Open MIPI CSI camera (init + channel open + sensor streaming)
 */
static avdk_err_t isp_open_mipi_camera(uint16_t sensor_width, uint16_t sensor_height, uint16_t fps,
                                       uint8_t chnl_id, bk_pixel_format_t output_fmt,
                                       uint16_t isp_output_width, uint16_t isp_output_height,
                                       bk_isp_camera_exposure_info_t *seed_exposure,
                                       const bk_isp_camera_wb_attr_t *seed_wb)
{
    avdk_err_t ret;

    if (!s_isp_camera_handle.is_initialized)
    {
        ret = isp_init_mipi_camera(sensor_width, sensor_height, fps);
        if (ret != AVDK_ERR_OK)
        {
            return ret;
        }
    }

    if (!s_isp_camera_handle.is_sensor_started)
    {
        bk_camera_sensor_format_t format = {
            .width = sensor_width,
            .height = sensor_height,
            .fps = fps,
        };

        ret = bk_camera_sensor_init(s_isp_camera_handle.sensor_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("Failed to init sensor\n");
            isp_cleanup_all_resources();
            return ret;
        }

        ret = bk_camera_sensor_set_format(s_isp_camera_handle.sensor_handle, &format);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("Failed to set sensor format\n");
            isp_cleanup_all_resources();
            return ret;
        }

        s_isp_camera_handle.is_sensor_started = 1;
    }

    /* Latch the direct-publish policy before channel open consumes it. */
    {
        uint8_t direct_publish = s_shoot4k_mode_active ? 1U : 0U;

        ret = bk_isp_camera_ctlr_ioctl(s_isp_camera_handle.camera_ctlr_handle,
                                       BK_CAM_IOCTL_SET_SHOOT4K_DIRECT_PUBLISH,
                                       &direct_publish);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("Failed to set shoot4k direct publish: %d\n", ret);
        }
    }

    /* Seed the manual exposure while the sensor is still stopped: channel open
     * below turns the sensor stream on, so the very first captured frame already
     * uses this exposure and a later borrow cannot pick a pre-exposure frame. */
    if (seed_exposure != NULL)
    {
        ret = bk_isp_camera_ctlr_ioctl(s_isp_camera_handle.camera_ctlr_handle,
                                       BK_CAM_IOCTL_SET_INITIAL_EXPOSURE, seed_exposure);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("Failed to seed exposure before stream on: %d\n", ret);
        }
    }

    if (seed_wb != NULL)
    {
        ret = bk_isp_camera_ctlr_ioctl(s_isp_camera_handle.camera_ctlr_handle,
                                       BK_CAM_IOCTL_SET_WB, (void *)seed_wb);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("Failed to seed AWB before stream on: %d\n", ret);
        }
    }

    {
        uint8_t acquired_chnl = chnl_id;

        ret = bk_isp_camera_ctlr_ioctl(s_isp_camera_handle.camera_ctlr_handle,
                                       BK_CAM_IOCTL_CHANNEL_ACQUIRE, &acquired_chnl);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("Failed to acquire channel %d for zero-copy: %d\n", chnl_id, ret);
            return ret;
        }
    }

    ret = isp_create_instance(chnl_id, isp_output_width, isp_output_height, output_fmt);
    if (ret != AVDK_ERR_OK)
    {
        uint8_t acquired_chnl = chnl_id;

        (void)bk_isp_camera_ctlr_ioctl(s_isp_camera_handle.camera_ctlr_handle,
                                       BK_CAM_IOCTL_CHANNEL_RELEASE, &acquired_chnl);
        if (bk_isp_camera_channel_state_get(s_isp_camera_handle.camera_ctlr_handle, ISP_MP_CHN_ID) == ISP_CHANNEL_STATE_TURN_OFF
            && bk_isp_camera_channel_state_get(s_isp_camera_handle.camera_ctlr_handle, ISP_SP_CHN_ID) == ISP_CHANNEL_STATE_TURN_OFF)
        {
            isp_cleanup_all_resources();
        }
        return ret;
    }

    return AVDK_ERR_OK;
}

/**
 * @brief Detach app-owned buffers so bk_isp_deinit() does not free them.
 */
static void isp_shoot4k_release_poked_slots(void)
{
    bk_camera_isp_ctlr_t *controller;
    isp_control_t *isp_control;

    if (!s_isp_camera_handle.is_initialized || !s_isp_camera_handle.camera_ctlr_handle)
    {
        return;
    }

    if (!s_shoot4k_zc_armed && !s_shoot4k_borrow_owned)
    {
        return;
    }

    controller = __containerof(s_isp_camera_handle.camera_ctlr_handle, bk_camera_isp_ctlr_t, ops);
    if (controller == NULL)
    {
        return;
    }

    isp_control = (isp_control_t *)controller->isp_handle;
    if (isp_control == NULL)
    {
        return;
    }

    if (s_shoot4k_zc_armed && s_shoot4k_poke_ybase != NULL)
    {
        isp_control->chn[ISP_MP_CHN_ID].frame_buffer[0] = NULL;
        if (s_shoot4k_poke_ybase1 != NULL)
        {
            isp_control->chn[ISP_MP_CHN_ID].frame_buffer[1] = NULL;
        }
    }

    if (s_shoot4k_borrow_owned)
    {
        isp_control->chn[s_shoot4k_borrow_chnl].frame_buffer[s_shoot4k_borrow_slot] = NULL;
        s_shoot4k_borrow_owned = 0;
    }
}

/**
 * @brief Cleanup all ISP resources (callbacks, timer, controller, etc.)
 */
static avdk_err_t isp_cleanup_all_resources(void)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_isp_camera_handle.is_initialized && s_isp_camera_handle.camera_ctlr_handle)
    {
        /* App-owned poked buffers must not be freed by the driver. */
        isp_shoot4k_release_poked_slots();

        ret = bk_isp_camera_deinit(s_isp_camera_handle.camera_ctlr_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("Failed to deinit camera device\n");
        }
    }

    if (s_isp_camera_handle.camera_ctlr_handle)
    {
        ret = bk_isp_camera_delete(s_isp_camera_handle.camera_ctlr_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("Failed to delete camera controller\n");
        }
        s_isp_camera_handle.camera_ctlr_handle = NULL;
    }

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
 */
static avdk_err_t isp_close_channel(uint8_t is_mp)
{
    avdk_err_t ret = AVDK_ERR_OK;
    const char *chnl_name = is_mp ? "MP" : "SP";
    uint8_t chnl_id = is_mp ? ISP_MP_CHN_ID : ISP_SP_CHN_ID;

    isp_shoot4k_release_poked_slots();

    {
        uint8_t acquired_chnl = chnl_id;

        (void)bk_isp_camera_ctlr_ioctl(s_isp_camera_handle.camera_ctlr_handle,
                                       BK_CAM_IOCTL_CHANNEL_RELEASE, &acquired_chnl);
    }

    ret = bk_isp_camera_channel_close(s_isp_camera_handle.camera_ctlr_handle, chnl_id);

    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to stop %s instance: %d\n", chnl_name, ret);
        return ret;
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

    /* Both channels closed: release all resources (incl. PSRAM buffers). */
    if (bk_isp_camera_channel_state_get(s_isp_camera_handle.camera_ctlr_handle, ISP_MP_CHN_ID) == ISP_CHANNEL_STATE_TURN_OFF
        && bk_isp_camera_channel_state_get(s_isp_camera_handle.camera_ctlr_handle, ISP_SP_CHN_ID) == ISP_CHANNEL_STATE_TURN_OFF)
    {
        ret = isp_cleanup_all_resources();
    }

    return ret;
}

#define ISP_DUMP_BORROW_RETRY_MAX      (30U)
#define ISP_DUMP_BORROW_TIMEOUT_MS     (0x1000U)
#define ISP_SHOOT4K_DCACHE_CHUNK_BYTES  (64U * 1024U)

/**
 * @brief Resolve the ISP control behind the camera controller handle.
 *        Zero-copy borrow uses chn pop_buf/free_buf directly here.
 */
static isp_control_t *isp_shoot4k_isp_control_get(void)
{
    bk_camera_isp_ctlr_t *controller;
    isp_control_t *isp_control;

    if (!s_isp_camera_handle.is_initialized || !s_isp_camera_handle.camera_ctlr_handle)
    {
        LOGE("Camera not initialized\n");
        return NULL;
    }

    controller = __containerof(s_isp_camera_handle.camera_ctlr_handle, bk_camera_isp_ctlr_t, ops);
    isp_control = (controller != NULL) ? (isp_control_t *)controller->isp_handle : NULL;
    if (isp_control == NULL || isp_control->pop_buf == NULL || isp_control->free_buf == NULL)
    {
        return NULL;
    }

    return isp_control;
}

/**
 * @brief Pop one frame from a frame-mode ISP channel (zero-copy).
 */
static avdk_err_t isp_shoot4k_pop_buf(uint8_t chnl_id, VIDEO_BUF_S *buf, uint32_t timeout)
{
    isp_control_t *isp_control = isp_shoot4k_isp_control_get();
    isp_channel_config_t *chn;

    if (isp_control == NULL || buf == NULL || chnl_id >= ISP_CHN_CNT)
    {
        return AVDK_ERR_INVAL;
    }

    chn = &isp_control->chn[chnl_id];
    if (chn->enable == false || chn->enable_flexa)
    {
        LOGE("channel %u not in frame mode\n", (unsigned int)chnl_id);
        return AVDK_ERR_GENERIC;
    }

    return (avdk_err_t)isp_control->pop_buf(chn->channel, buf, timeout);
}

/**
 * @brief Return a popped buffer to the ISP channel queue.
 */
static void isp_shoot4k_free_buf(uint8_t chnl_id, VIDEO_BUF_S *buf)
{
    isp_control_t *isp_control = isp_shoot4k_isp_control_get();

    if (isp_control != NULL && buf != NULL)
    {
        isp_control->free_buf(isp_control->chn[chnl_id].channel, buf);
    }
}

static void isp_shoot4k_dcache_invalidate(uint8_t *base, uint32_t len)
{
    uint32_t offset = 0;

    if (base == NULL || len == 0)
    {
        return;
    }

    while (offset < len)
    {
        uint32_t chunk = len - offset;

        if (chunk > ISP_SHOOT4K_DCACHE_CHUNK_BYTES)
        {
            chunk = ISP_SHOOT4K_DCACHE_CHUNK_BYTES;
        }
        arch_dcache_flush_and_invd_range(base + offset, chunk);
        offset += chunk;
    }
}

/**
 * @brief Borrow one frame from MP/SP with retry (zero-copy).
 */
static avdk_err_t isp_borrow_channel_frame(uint8_t is_mp, uint8_t **frame, uint32_t *frame_len)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    uint8_t chnl_id = is_mp ? ISP_MP_CHN_ID : ISP_SP_CHN_ID;
    uint32_t attempt;

    if (frame == NULL || frame_len == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    if (!s_isp_camera_handle.is_initialized || !s_isp_camera_handle.camera_ctlr_handle)
    {
        LOGE("Camera not initialized\n");
        return AVDK_ERR_INVAL;
    }

    for (attempt = 0; attempt < ISP_DUMP_BORROW_RETRY_MAX; attempt++)
    {
        VIDEO_BUF_S buf = {0};
        uint8_t plane_cnt;
        uint8_t *base;
        uint32_t total = 0;
        uint8_t p;

        *frame = NULL;
        *frame_len = 0;
        ret = isp_shoot4k_pop_buf(chnl_id, &buf, ISP_DUMP_BORROW_TIMEOUT_MS);
        if (ret == AVDK_ERR_OK)
        {
            plane_cnt = buf.numPlanes ? buf.numPlanes : 1;
            if (plane_cnt > VIDEO_MAX_PLANES)
            {
                plane_cnt = VIDEO_MAX_PLANES;
            }

            base = (buf.planes[0].pUserAddr != NULL)
                       ? (uint8_t *)buf.planes[0].pUserAddr
                       : (uint8_t *)(uintptr_t)buf.planes[0].dmaPhyAddr;
            for (p = 0; p < plane_cnt; p++)
            {
                total += buf.planes[p].size;
            }

            if (base != NULL && total != 0)
            {
                isp_shoot4k_dcache_invalidate(base, total);
                s_shoot4k_borrow_chnl = chnl_id;
                s_shoot4k_borrow_slot = buf.index;
                s_shoot4k_borrow_owned = 1;
                *frame = base;
                *frame_len = total;
                return AVDK_ERR_OK;
            }

            LOGE("channel %u invalid buffer base=%p len=%u\n",
                 (unsigned int)chnl_id, base, (unsigned int)total);
            isp_shoot4k_free_buf(chnl_id, &buf);
            ret = AVDK_ERR_GENERIC;
        }
        rtos_delay_milliseconds(100);
    }
    return ret;
}

typedef struct {
    uint8_t *full_frame;
    uint32_t borrow_frame_addr;
    uint32_t borrow_frame_len;
} isp_rdma_stitch_ctx_t;

static isp_rdma_stitch_ctx_t s_isp_rdma_stitch_ctx = {0};

/**
 * @brief Record MP output buffers for the next open (zero-copy shoot_4k only).
 */
static void isp_shoot4k_poke_output(uint8_t *y_base, uint8_t *y_base1, uint32_t y_stride)
{
    s_shoot4k_poke_ybase = y_base;
    s_shoot4k_poke_ybase1 = (y_base != NULL) ? y_base1 : NULL;
    s_shoot4k_poke_ystride = (y_base != NULL) ? y_stride : 0;
    s_shoot4k_zc_armed = (y_base != NULL) ? 1 : 0;
}

static void isp_func_rdma_write_config(uint32_t buffer_addr, uint32_t width,uint32_t height, uint32_t part)
{
    *((volatile uint32_t*)(0x4c040000 + 0x000014c8)) = (uint32_t)((uintptr_t)(buffer_addr) + (!!part)*width*2);
    *((volatile uint32_t*)(0x4c040000 + 0x000014cc)) = width;           //width in pixels
    *((volatile uint32_t*)(0x4c040000 + 0x000014d0)) = width*2*2;       //line stride in bytes
    *((volatile uint32_t*)(0x4c040000 + 0x000014d4)) = width*2*2*height;//total image size in bytes
    *((volatile uint32_t*)(0x4c040000 + 0x000015ec)) = width*2;         //line width in bytes
    *((volatile uint32_t*)(0x4c040000 + 0x000015e8)) = 4;//RAW10
    *((volatile uint32_t*)(0x4c040000 + 0x00000404)) &= ~(3 << 15);
    *((volatile uint32_t*)(0x4c040000 + 0x00000404)) |= (1 << 15);
    *((volatile uint32_t*)(0x4c040000 + 0x00001520)) = 0xA;
    *((volatile uint32_t*)(0x4c040000 + 0x00001524)) = 1;

    LOGI("RDMA part%u: start=0x%08x w=%u stride=%u lval=%u h=%u\n",
        (unsigned int)part, (unsigned int)buffer_addr + (!!part)*width*2, (unsigned int)width,
        (unsigned int)width*2*2, (unsigned int)width*2, (unsigned int)height);
}

/**
 * @brief Replay the borrowed 4K RAW through the RDMA input in two half frames,
 *        waiting for each half individually.
 *
 * part0 and part1 share the same RDMA descriptor registers, so part1 can only
 * be programmed after the left half frame has been fully written and published.
 * Popping that frame is the completion barrier; it replaces polling the MI Y
 * base register with an arbitrary timeout.
 */
static avdk_err_t isp_shoot4k_replay_halves(uint32_t raw_addr)
{
    VIDEO_BUF_S left_buf = {0};
    VIDEO_BUF_S right_buf = {0};
    uint32_t t_start = rtos_get_time();
    avdk_err_t ret;

    isp_func_rdma_write_config(raw_addr, SHOOT4K_HALF_WIDTH, SHOOT4K_HALF_HEIGHT, 0);

    ret = isp_shoot4k_pop_buf(ISP_MP_CHN_ID, &left_buf, ISP_DUMP_BORROW_TIMEOUT_MS);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("shoot_4k: left half pop failed: %d\n", ret);
        return ret;
    }

    /* The left half is complete: safe to rewrite the shared DMA descriptors. */
    *((volatile uint32_t *)(0x4c040000U + 0x1404U)) |= 0x10U; /* MI_INIT: MRV_MI_CFG_UPD */
    isp_func_rdma_write_config(raw_addr, SHOOT4K_HALF_WIDTH, SHOOT4K_HALF_HEIGHT, 1);

    ret = isp_shoot4k_pop_buf(ISP_MP_CHN_ID, &right_buf, ISP_DUMP_BORROW_TIMEOUT_MS);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("shoot_4k: right half pop failed: %d\n", ret);
        isp_shoot4k_free_buf(ISP_MP_CHN_ID, &left_buf);
        return ret;
    }

    isp_shoot4k_free_buf(ISP_MP_CHN_ID, &left_buf);
    isp_shoot4k_free_buf(ISP_MP_CHN_ID, &right_buf);

    LOGI("shoot_4k: half-frame replay done in %ums\n",
         (unsigned int)(rtos_get_time() - t_start));

    return AVDK_ERR_OK;
}

#define UART_BASE_ADDR 0x458e0000   //0x48110000
typedef struct _uart_regs
{
    volatile uint32_t did;
    volatile uint32_t vid;
    volatile uint32_t ctrl;
    volatile uint32_t status;
    volatile uint32_t config;
    volatile uint32_t fifo_cfg;
    volatile uint32_t fifo_status;
    volatile uint32_t fifo_port;
    volatile uint32_t int_enable;
    volatile uint32_t int_status;
    volatile uint32_t flow_cfg;
    volatile uint32_t wake_cfg;
    volatile uint32_t glitch_cfg;
}uart_regs_t;

static void shoot4k_uart_init(uint32_t baud_rate, uint32_t data_bits, uint32_t stop_bits, uint32_t parity_mode)
{
    volatile uart_regs_t* uart = (volatile uart_regs_t*)UART_BASE_ADDR;

    uint32_t baud_divisor = 120000000 / baud_rate - 1;

    uart->ctrl   = 0;
    uart->ctrl   = 1;
    uart->config = ((uint32_t)stop_bits << 7)
                 | (((uint32_t)parity_mode & 0x1) << 6)
                 | ((((uint32_t)parity_mode >> 1) & 0x1) << 5)
                 | ((uint32_t)data_bits << 3)
                 | (baud_divisor << 8)
                 | (1 << 1)
                 | (1 << 0);

    uart->flow_cfg   = 0;
    uart->wake_cfg   = 0;
    uart->glitch_cfg = 0;
    uart->fifo_cfg   = ((0x80 << 8) | (0x80 << 0) | (0x0 << 16));
    uart->int_enable = 0;
}

static int32_t shoot4k_uart_send(uint8_t* buf, uint32_t len)
{
    uint32_t size = len;

    volatile uart_regs_t* uart = (volatile uart_regs_t*)UART_BASE_ADDR;

    while (size)
    {
        if (uart->fifo_status & (1 << 20))
        {
            uart->fifo_port = *buf++;
            size--;
        }
    }
    return (int32_t)len;
}

/* MI output geometry registers (see isp_fast_init.h / vsi_isp_reg_def.h). */
#define ISP_MI_MP_Y_SIZE_INIT   (0x4c040000U + 0x140CU)
#define ISP_MI_MP_Y_LLENGTH     (0x4c040000U + 0x1550U)
#define ISP_MI_MP_Y_PIC_WIDTH   (0x4c040000U + 0x1564U)
#define ISP_MI_MP_Y_PIC_SIZE    (0x4c040000U + 0x156CU)

/* VSI_ISP_SetMiV10ChnFmt()'s RGB565 branch ignores the poked stride and programs
 * a compact half-width frame (LLENGTH = width, frame size = 8.29MB). The in-place
 * stitch needs the full output line size and frame span, so override the MI geometry
 * right after channel open and before the RDMA replay starts. */
static void isp_shoot4k_mi_geometry_override(void)
{
    *((volatile uint32_t *)ISP_MI_MP_Y_LLENGTH)   = SHOOT4K_MI_Y_LLENGTH_VALUE;
    *((volatile uint32_t *)ISP_MI_MP_Y_PIC_WIDTH) = SHOOT4K_HALF_WIDTH;
    *((volatile uint32_t *)ISP_MI_MP_Y_SIZE_INIT) = SHOOT4K_OUT_BUF_SIZE;
    *((volatile uint32_t *)ISP_MI_MP_Y_PIC_SIZE)  = SHOOT4K_OUT_BUF_SIZE;

    /* Latch the new geometry: the MI config registers are double buffered and
     * only take effect on an MRV_MI_CFG_UPD pulse. */
    *((volatile uint32_t *)(0x4c040000U + 0x1404U)) |= 0x10U;

    LOGI("shoot_4k: MI geometry override: LLENGTH=%u size=%u\n",
         (unsigned int)SHOOT4K_MI_Y_LLENGTH_VALUE, (unsigned int)SHOOT4K_OUT_BUF_SIZE);
}

/* AWB result sampled from the Pass 1 probe frame and re-applied to the final
 * capture (equivalent to one extra AWB iteration). */
typedef struct {
    uint8_t valid;
    uint16_t r_gain;
    uint16_t gr_gain;
    uint16_t gb_gain;
    uint16_t b_gain;
} shoot4k_wb_seed_t;

static shoot4k_wb_seed_t s_shoot4k_wb_seed = {0};

static void isp_shoot4k_query_awb_seed(void)
{
    isp_control_t *control = isp_shoot4k_isp_control_get();
    ISP_AWB_INFO_S awb = {0};

    s_shoot4k_wb_seed.valid = 0;

    if (control == NULL)
    {
        return;
    }

    if (VSI_MPI_ISP_QueryAwbInfo(control->port, &awb) != VSI_SUCCESS)
    {
        LOGW("shoot_4k: query AWB info failed, keep auto WB\n");
        return;
    }

    if (awb.wbGain.rGain == 0 || awb.wbGain.grGain == 0 ||
        awb.wbGain.gbGain == 0 || awb.wbGain.bGain == 0)
    {
        LOGW("shoot_4k: AWB info invalid r=%u gr=%u gb=%u b=%u\n",
             (unsigned int)awb.wbGain.rGain, (unsigned int)awb.wbGain.grGain,
             (unsigned int)awb.wbGain.gbGain, (unsigned int)awb.wbGain.bGain);
        return;
    }

    s_shoot4k_wb_seed.valid = 1;
    s_shoot4k_wb_seed.r_gain = awb.wbGain.rGain;
    s_shoot4k_wb_seed.gr_gain = awb.wbGain.grGain;
    s_shoot4k_wb_seed.gb_gain = awb.wbGain.gbGain;
    s_shoot4k_wb_seed.b_gain = awb.wbGain.bGain;

    LOGI("shoot_4k: AWB seed r=%u gr=%u gb=%u b=%u ct=%u\n",
         (unsigned int)s_shoot4k_wb_seed.r_gain, (unsigned int)s_shoot4k_wb_seed.gr_gain,
         (unsigned int)s_shoot4k_wb_seed.gb_gain, (unsigned int)s_shoot4k_wb_seed.b_gain,
         (unsigned int)awb.colorTemp);
}

/**
 * @brief Capture one stitched 4K frame via the shoot_4k pipeline.
 *        The borrowed 4K RAW buffer is reused as the output; the runtime format
 *        is RGB565 or YUYV422 (`shoot4k fmt`).
 */
avdk_err_t isp_shoot_4k_capture(uint8_t **out_frame, uint32_t *out_len)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_isp_camera_exposure_info_t info = {0};
    uint8_t *borrow_frame = NULL;
    uint32_t src_raw_len = 0;
    uint8_t *out_buf = NULL;
    uint32_t out_size = SHOOT4K_OUT_BUF_SIZE;
    bk_pixel_format_t out_fmt = shoot4k_out_bk_fmt();
    uint32_t t_pass1_start, t_pass2_start, t_pass2_end;

    if (out_frame == NULL || out_len == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    *out_frame = NULL;
    *out_len = 0;
    s_shoot4k_mode_active = 1;

    if (s_isp_rdma_stitch_ctx.full_frame != NULL)
    {
        bk_frame_buffer_free(s_isp_rdma_stitch_ctx.full_frame);
        s_isp_rdma_stitch_ctx.full_frame = NULL;
    }

    /* ---- Pass 1: AE probe ---- */
    t_pass1_start = rtos_get_time();

    ret = isp_open_mipi_camera(SHOOT4K_PROBE_WIDTH, SHOOT4K_PROBE_HEIGHT, SHOOT4K_PROBE_FPS,
                               0, BK_PIXEL_FORMAT_NV12,
                               SHOOT4K_PROBE_WIDTH, SHOOT4K_PROBE_HEIGHT, NULL, NULL);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("shoot_4k: open %ux%u AE probe failed: %d\n",
             SHOOT4K_PROBE_WIDTH, SHOOT4K_PROBE_HEIGHT, ret);
        s_shoot4k_mode_active = 0;
        return ret;
    }

    uint8_t *probe_frame = NULL;
    uint32_t probe_len = 0;

    ret = isp_borrow_channel_frame(1, &probe_frame, &probe_len);
    if (ret != AVDK_ERR_OK)
    {
        LOGW("shoot_4k: probe borrow failed: %d, continue with AE info only\n", ret);
    }

    ret = bk_isp_camera_ctlr_ioctl(s_isp_camera_handle.camera_ctlr_handle,
                                   BK_CAM_IOCTL_QUERY_EXPOSURE_INFO, &info);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("shoot_4k: query exposure info failed: %d\n", ret);
    }


    isp_shoot4k_query_awb_seed();

    isp_close_channel(1);

    if (probe_frame != NULL)
    {
        bk_frame_buffer_free(probe_frame);
        probe_frame = NULL;
    }
    /* ---- Pass 2: capture with locked exposure, stitch full 4K frame ---- */
    t_pass2_start = rtos_get_time();
    LOGI("shoot_4k: pass1 (AE probe) end %u, cost %ums\n",
         (unsigned int)t_pass2_start, (unsigned int)(t_pass2_start - t_pass1_start));
    ret = isp_open_mipi_camera(SHOOT4K_RAW_WIDTH, SHOOT4K_RAW_HEIGHT, SHOOT4K_RAW_FPS,
                               0, BK_PIXEL_FORMAT_RAW10,
                               SHOOT4K_RAW_WIDTH, SHOOT4K_RAW_HEIGHT, &info, NULL);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("shoot_4k: reopen 4K RAW failed: %d\n", ret);
        s_shoot4k_mode_active = 0;
        return ret;
    }

    borrow_frame = NULL;
    src_raw_len = 0;

    ret = isp_borrow_channel_frame(1, &borrow_frame, &src_raw_len);
    if (ret != AVDK_ERR_OK || borrow_frame == NULL || src_raw_len == 0)
    {
        LOGE("shoot_4k: final borrow RAW failed: %d\n", ret);
        isp_close_channel(1);
        s_shoot4k_mode_active = 0;
        return (ret == AVDK_ERR_OK) ? AVDK_ERR_GENERIC : ret;
    }

    s_isp_rdma_stitch_ctx.borrow_frame_addr = (uint32_t)(uintptr_t)borrow_frame;
    s_isp_rdma_stitch_ctx.borrow_frame_len = src_raw_len;
    isp_close_channel(1);

    /* In-place: the borrowed 4K RAW frame doubles as the RGB565/YUYV422 output. */
    if (src_raw_len < out_size)
    {
        LOGE("shoot_4k: RAW buffer too small for in-place output (%u < %u)\n",
             (unsigned int)src_raw_len, (unsigned int)out_size);
        ret = AVDK_ERR_NOMEM;
        goto fail_borrow;
    }
    out_buf = borrow_frame;

    isp_shoot4k_poke_output(out_buf, out_buf + SHOOT4K_POKE_SECOND_OFFSET, SHOOT4K_POKE_STRIDE);

    {
        bk_isp_camera_wb_attr_t wb_seed;
        const bk_isp_camera_wb_attr_t *wb_seed_ptr = NULL;

        /* Lock the WB gains sampled from the Pass 1 probe frame so the final
         * capture starts from the previous AWB iteration instead of defaults. */
        if (s_shoot4k_wb_seed.valid)
        {
            os_memset(&wb_seed, 0, sizeof(wb_seed));
            wb_seed.enable = 1;
            wb_seed.op_type = BK_ISP_OP_TYPE_MANUAL;
            wb_seed.manual_gain.r_gain = s_shoot4k_wb_seed.r_gain;
            wb_seed.manual_gain.gr_gain = s_shoot4k_wb_seed.gr_gain;
            wb_seed.manual_gain.gb_gain = s_shoot4k_wb_seed.gb_gain;
            wb_seed.manual_gain.b_gain = s_shoot4k_wb_seed.b_gain;
            wb_seed_ptr = &wb_seed;
        }

        ret = isp_open_mipi_camera(SHOOT4K_HALF_WIDTH, SHOOT4K_HALF_HEIGHT, SHOOT4K_HALF_FPS,
                                   0, out_fmt,
                                   SHOOT4K_HALF_WIDTH, SHOOT4K_HALF_HEIGHT, NULL, wb_seed_ptr);
    }
    if (ret != AVDK_ERR_OK)
    {
        LOGE("shoot_4k: open final failed: %d\n", ret);
        goto fail_zerocopy;
    }

    /* The RGB565/YUYV422 channel was opened with the driver's compact layout;
     * switch the MI to the 4K stitch geometry before the RDMA replay produces
     * frames. */
    isp_shoot4k_mi_geometry_override();

    ret = isp_shoot4k_replay_halves(s_isp_rdma_stitch_ctx.borrow_frame_addr);
    isp_close_channel(1);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("shoot_4k: half-frame replay failed: %d\n", ret);
        goto fail_zerocopy;
    }

    /* Clear the poke so later normal opens are not affected. */
    isp_shoot4k_poke_output(NULL, NULL, 0);

    /* Ownership transferred to the caller: out_buf is the borrowed RAW frame. */
    s_isp_rdma_stitch_ctx.borrow_frame_addr = 0;
    s_isp_rdma_stitch_ctx.borrow_frame_len = 0;

    *out_frame = out_buf;
    *out_len = out_size;

    t_pass2_end = rtos_get_time();
    LOGI("shoot_4k: pass2 (capture, zero-copy) end %u, cost %ums\n",
            (unsigned int)t_pass2_end, (unsigned int)(t_pass2_end - t_pass2_start));
    LOGI("shoot_4k: total cost %ums\n", (unsigned int)(t_pass2_end - t_pass1_start));
    s_shoot4k_mode_active = 0;
    return AVDK_ERR_OK;

fail_zerocopy:
    isp_shoot4k_release_poked_slots();
    isp_shoot4k_poke_output(NULL, NULL, 0);
    goto fail_borrow;

fail_borrow:
    if (s_isp_rdma_stitch_ctx.borrow_frame_addr != 0)
    {
        bk_frame_buffer_free((void *)(uintptr_t)s_isp_rdma_stitch_ctx.borrow_frame_addr);
        s_isp_rdma_stitch_ctx.borrow_frame_addr = 0;
        s_isp_rdma_stitch_ctx.borrow_frame_len = 0;
    }
    s_shoot4k_mode_active = 0;
    return ret;
}

void isp_shoot_4k_release(uint8_t *frame)
{
    if (frame == NULL)
    {
        return;
    }

    bk_frame_buffer_free(frame);
    if (s_isp_rdma_stitch_ctx.full_frame == frame)
    {
        s_isp_rdma_stitch_ctx.full_frame = NULL;
    }
}

#define ISP_JPEG_ENC_HDR_ALIGN(sz)     ((((sz) + 63U) >> 6) << 6)
#define ISP_JPEG_ENC_4K_WIDTH          (SHOOT4K_OUT_WIDTH)
#define ISP_JPEG_ENC_4K_HEIGHT         (SHOOT4K_OUT_HEIGHT)
#define ISP_JPEG_ENC_OUT_BUF_SIZE      (4U * 1024U * 1024U)

typedef struct {
    volatile uint32_t result;
    volatile uint32_t jpeg_size;
    uint8_t *out_payload;
} isp_jpeg_enc_ctx_t;

static uint8_t *s_isp_last_jpeg = NULL;
static uint32_t s_isp_last_jpeg_len = 0;

#if CONFIG_BK_H264E
static isp_jpeg_enc_ctx_t *s_isp_jpeg_enc_cb_ctx = NULL;
#endif

static void *isp_jpeg_payload_to_frame(uint8_t *payload)
{
    return (void *)(payload - ISP_JPEG_ENC_HDR_ALIGN(sizeof(frame_buffer_t)));
}

void isp_jpeg_encode_release(uint8_t *jpeg)
{
    if (jpeg == NULL)
    {
        return;
    }

    bk_frame_buffer_free(isp_jpeg_payload_to_frame(jpeg));
    if (s_isp_last_jpeg == jpeg)
    {
        s_isp_last_jpeg = NULL;
        s_isp_last_jpeg_len = 0;
    }
}

#if CONFIG_BK_H264E
static void *isp_jpeg_outbuf_malloc(uint32_t size)
{
    uint32_t frame_size;
    frame_buffer_t *frame;

    frame_size = ISP_JPEG_ENC_HDR_ALIGN(sizeof(frame_buffer_t) + size);
    /* Prefer CODED; fall back to UNCODED when CODED is exhausted. */
    frame = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, frame_size);
    if (frame == NULL)
    {
        frame = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
    }
    if (frame == NULL)
    {
        LOGE("jpeg outbuf malloc failed, size=%u\n", (unsigned int)size);
        return NULL;
    }

    frame->frame = (uint8_t *)frame + ISP_JPEG_ENC_HDR_ALIGN(sizeof(frame_buffer_t));
    frame->size = size;
    frame->length = 0;
    frame->width = ISP_JPEG_ENC_4K_WIDTH;
    frame->height = ISP_JPEG_ENC_4K_HEIGHT;
    return frame->frame;
}

static void isp_jpeg_enc_out_callback(uint8_t *buffer, uint32_t size, uint32_t type)
{
    (void)type;

    if (s_isp_jpeg_enc_cb_ctx == NULL)
    {
        return;
    }

    s_isp_jpeg_enc_cb_ctx->result = BK_OK;
    s_isp_jpeg_enc_cb_ctx->jpeg_size = size;
    s_isp_jpeg_enc_cb_ctx->out_payload = buffer;
    LOGI("jpeg encode callback ok, size=%u\n", (unsigned int)size);
}
#endif

#if CONFIG_BK_H264E
/* VCENC platform bring-up used when calling the low level JpegEnc API directly
 * (h264_encoder_api.c keeps its own static copy). Mirrors that sequence:
 * clock/power -> IRQ -> platform init -> allocator hooks, refcounted. */
extern void h264e_int_register(void);
extern void vcenc_platform_init(void);
extern void vcenc_platform_deinit(void);

static uint8_t s_shoot4k_vcenc_refcnt = 0;

static void *shoot4k_vcenc_malloc(size_t size)
{
    return bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, size);
}

static void shoot4k_vcenc_free(void *ptr)
{
    bk_frame_buffer_free(ptr);
}

static void shoot4k_vcenc_acquire(void)
{
    if (s_shoot4k_vcenc_refcnt++ == 0)
    {
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26E, PM_POWER_MODULE_STATE_ON);
        sys_drv_h265_cksel_clkdiv_set(CKSEL_H265_160M, 1);
        bk_pm_clock_ctrl(PM_CLK_ID_H26E, PM_CLK_CTRL_PWR_UP);
        h264e_int_register();
        vcenc_platform_init();
        vcenc_memalloc_register(shoot4k_vcenc_malloc, shoot4k_vcenc_free);
    }
}

static void shoot4k_vcenc_release(void)
{
    if (s_shoot4k_vcenc_refcnt && --s_shoot4k_vcenc_refcnt == 0)
    {
        vcenc_platform_deinit();
        bk_pm_clock_ctrl(PM_CLK_ID_H26E, PM_CLK_CTRL_PWR_DOWN);
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26E, PM_POWER_MODULE_STATE_OFF);
    }
}
#endif

#if CONFIG_BK_H264E
/* Shared encoder for the 16bpp interleaved formats (RGB565 / YUYV422): both are
 * 2 bytes/pixel with a single input plane, so only the JPEG input frame type
 * differs. */
static avdk_err_t isp_jpeg_encode_16bpp(uint8_t *data, uint32_t data_len,
                                        uint32_t width, uint32_t height,
                                        JpegEncFrameType frame_type, const char *fmt_name,
                                        uint8_t **out_jpeg, uint32_t *out_jpeg_len)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    JpegEncInst enc = NULL;
    JpegEncCfg cfg;
    JpegEncIn enc_in;
    JpegEncOut enc_out;
    isp_jpeg_enc_ctx_t ctx;
    uint8_t *out_payload = NULL;
    uint32_t expect_size;
    uint32_t out_cap;
    int32_t jret;

    if (data == NULL || out_jpeg == NULL || out_jpeg_len == NULL || width == 0 || height == 0)
    {
        return AVDK_ERR_INVAL;
    }

    expect_size = width * height * 2U;
    if (data_len < expect_size)
    {
        LOGE("jpeg encode: %s too small, len=%u expect=%u (%ux%u)\n",
             fmt_name, (unsigned int)data_len, (unsigned int)expect_size,
             (unsigned int)width, (unsigned int)height);
        return AVDK_ERR_INVAL;
    }

    *out_jpeg = NULL;
    *out_jpeg_len = 0;
    os_memset(&ctx, 0, sizeof(ctx));
    os_memset(&cfg, 0, sizeof(cfg));
    os_memset(&enc_in, 0, sizeof(enc_in));
    os_memset(&enc_out, 0, sizeof(enc_out));
    ctx.result = (uint32_t)~0U;

    out_cap = ISP_JPEG_ENC_OUT_BUF_SIZE;
    out_payload = (uint8_t *)isp_jpeg_outbuf_malloc(out_cap);
    if (out_payload == NULL)
    {
        return AVDK_ERR_NOMEM;
    }

    shoot4k_vcenc_acquire();

    cfg.quality    = 50;
    cfg.qLevel     = 5;
    cfg.fixedQP    = -1;
    cfg.frameType  = frame_type;
    cfg.markerType = JPEGENC_SINGLE_MARKER;
    cfg.unitsType  = JPEGENC_DOTS_PER_INCH;
    cfg.xDensity   = 72;
    cfg.yDensity   = 72;

    if (JpegEncInit(&cfg, &enc, NULL) != JPEGENC_OK)
    {
        LOGE("jpeg encode: JpegEncInit failed (%s)\n", fmt_name);
        goto exit_release;
    }

    cfg.inputWidth      = width;
    cfg.codingWidth     = width;
    cfg.inputHeight     = height;
    cfg.codingHeight    = height;
    cfg.xOffset         = 0;
    cfg.yOffset         = 0;
    cfg.rotation        = JPEGENC_ROTATE_0;
    cfg.codingType      = JPEGENC_WHOLE_FRAME;
    cfg.restartInterval = 5;

    if (JpegEncSetPictureSize(enc, &cfg) != JPEGENC_OK)
    {
        LOGE("jpeg encode: JpegEncSetPictureSize failed (%s)\n", fmt_name);
        goto exit_release;
    }

    enc_in.frameHeader   = 1;
    enc_in.busLum        = (ptr_t)(uintptr_t)data;
    enc_in.busCb         = (ptr_t)(uintptr_t)data; /* interleaved input: Cb/Cr unused */
    enc_in.busCr         = (ptr_t)(uintptr_t)data;
    enc_in.dec400Enable  = 1;
    enc_in.pOutBuf[0]    = out_payload;
    enc_in.busOutBuf[0]  = (ptr_t)(uintptr_t)out_payload;
    enc_in.outBufSize[0] = out_cap;

    LOGI("jpeg encode start, %ux%u %s in=%p\n",
         (unsigned int)width, (unsigned int)height, fmt_name, data);

    s_isp_jpeg_enc_cb_ctx = &ctx;
    do
    {
        jret = (int32_t)JpegEncEncode(enc, &enc_in, &enc_out);
        if (jret == JPEGENC_RESTART_INTERVAL || jret == JPEGENC_FRAME_READY)
        {
            isp_jpeg_enc_out_callback((uint8_t *)enc_in.pOutBuf[0],
                                      enc_out.jfifSize, enc_out.headerSize);
        }
    } while (jret == JPEGENC_RESTART_INTERVAL);

    if (jret < 0 || ctx.result != BK_OK || ctx.out_payload == NULL || ctx.jpeg_size == 0U)
    {
        LOGE("jpeg encode bad ret=%d result=%u size=%u buf=%p (%s)\n",
             (int)jret, (unsigned int)ctx.result,
             (unsigned int)ctx.jpeg_size, ctx.out_payload, fmt_name);
        goto exit_release;
    }

    *out_jpeg = ctx.out_payload;
    *out_jpeg_len = ctx.jpeg_size;
    ctx.out_payload = NULL;
    out_payload = NULL; /* ownership transferred to caller */
    LOGI("jpeg encode done, size=%u\n", (unsigned int)*out_jpeg_len);
    ret = AVDK_ERR_OK;

exit_release:
    s_isp_jpeg_enc_cb_ctx = NULL;
    if (enc != NULL)
    {
        (void)JpegEncRelease(enc);
        enc = NULL;
    }
    shoot4k_vcenc_release();
    if (out_payload != NULL)
    {
        isp_jpeg_encode_release(out_payload);
    }
    return ret;
}
#endif

avdk_err_t isp_jpeg_encode_rgb565(uint8_t *rgb565, uint32_t rgb565_len,
                                  uint32_t width, uint32_t height,
                                  uint8_t **out_jpeg, uint32_t *out_jpeg_len)
{
#if !CONFIG_BK_H264E
    (void)rgb565;
    (void)rgb565_len;
    (void)width;
    (void)height;
    (void)out_jpeg;
    (void)out_jpeg_len;
    LOGE("isp_jpeg_encode_rgb565: CONFIG_BK_H264E is disabled\n");
    return AVDK_ERR_UNSUPPORTED;
#else
    return isp_jpeg_encode_16bpp(rgb565, rgb565_len, width, height,
                                 JPEGENC_RGB565, "RGB565", out_jpeg, out_jpeg_len);
#endif
}

avdk_err_t isp_jpeg_encode_yuyv422(uint8_t *yuyv, uint32_t yuyv_len,
                                   uint32_t width, uint32_t height,
                                   uint8_t **out_jpeg, uint32_t *out_jpeg_len)
{
#if !CONFIG_BK_H264E
    (void)yuyv;
    (void)yuyv_len;
    (void)width;
    (void)height;
    (void)out_jpeg;
    (void)out_jpeg_len;
    LOGE("isp_jpeg_encode_yuyv422: CONFIG_BK_H264E is disabled\n");
    return AVDK_ERR_UNSUPPORTED;
#else
    return isp_jpeg_encode_16bpp(yuyv, yuyv_len, width, height,
                                 JPEGENC_YUV422_INTERLEAVED_YUYV, "YUYV422",
                                 out_jpeg, out_jpeg_len);
#endif
}

void cli_shoot4k_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;
    char *msg = NULL;

    if (argc < 2)
    {
        LOGE("Usage: shoot4k <command>\n");
        LOGE("  shoot4k capture                 - capture %ux%u 16bpp frame\n",
             SHOOT4K_OUT_WIDTH, SHOOT4K_OUT_HEIGHT);
        LOGE("  shoot4k jpeg                    - capture then JPEG encode\n");
        LOGE("  shoot4k loop <n> [jpeg]         - repeat capture n times, free frame+JPEG per round\n");
        LOGE("  shoot4k fmt [rgb565|yuyv422]    - show/select the 16bpp output path\n");
        LOGE("  shoot4k uart_init [baud]        - init UART4 data port (default 115200)\n");
        LOGE("  shoot4k uart_send jpeg          - UART dump last JPEG\n");
        goto exit;
    }

    if (os_strcmp(argv[1], "fmt") == 0)
    {
        if (argc < 3)
        {
            os_printf("shoot4k fmt: %s\n", shoot4k_out_fmt_name());
            ret = AVDK_ERR_OK;
        }
        else if (os_strcmp(argv[2], "rgb565") == 0)
        {
            s_shoot4k_out_fmt = SHOOT4K_FMT_RGB565;
            os_printf("shoot4k fmt: rgb565\n");
            ret = AVDK_ERR_OK;
        }
        else if (os_strcmp(argv[2], "yuyv422") == 0 || os_strcmp(argv[2], "yuyv") == 0)
        {
            s_shoot4k_out_fmt = SHOOT4K_FMT_YUYV422;
            os_printf("shoot4k fmt: yuyv422\n");
            ret = AVDK_ERR_OK;
        }
        else
        {
            LOGE("Usage: shoot4k fmt [rgb565|yuyv422]\n");
            ret = AVDK_ERR_INVAL;
        }
    }
    else if (os_strcmp(argv[1], "uart_init") == 0)
    {
        uint32_t baud = (argc >= 3) ? (uint32_t)os_strtoul(argv[2], NULL, 10) : 115200;
        *(volatile uint32_t*)(0x44000400 + 4*4) = 0x67000000;
        *(volatile uint32_t*)(0x44000400 + 5*4) = 0x68000000;
        sys_ll_set_cpu_clk_div_mode2_cksel_uart4(CKSEL_SYS_XTAL_120M_120M);
        shoot4k_uart_init(baud, 3, 0, 0);
        ret = AVDK_ERR_OK;
    }
    else if (os_strcmp(argv[1], "uart_send") == 0)
    {
        uint32_t trans_len;

        if (argc >= 3 && os_strcmp(argv[2], "jpeg") == 0)
        {
            if (s_isp_last_jpeg == NULL || s_isp_last_jpeg_len == 0U)
            {
                LOGE("no jpeg yet, run: shoot4k jpeg\n");
                ret = AVDK_ERR_INVAL;
                goto exit;
            }
            trans_len = (uint32_t)shoot4k_uart_send(s_isp_last_jpeg, s_isp_last_jpeg_len);
            os_printf("jpeg trans_len: %u\n", trans_len);
            ret = AVDK_ERR_OK;
        }
        else
        {
            LOGE("Usage: shoot4k uart_send jpeg\n");
            ret = AVDK_ERR_INVAL;
        }
    }
    else if (os_strcmp(argv[1], "capture") == 0
             || os_strcmp(argv[1], "jpeg") == 0
             || os_strcmp(argv[1], "loop") == 0)
    {
        uint8_t do_jpeg = 0;
        uint32_t loop_count = 1;
        uint32_t ok_count = 0;
        uint32_t idx;
        uint8_t loop_mode;
        uint32_t t_cmd = rtos_get_time();

        if (os_strcmp(argv[1], "loop") == 0)
        {
            if (argc < 3)
            {
                LOGE("Usage: shoot4k loop <count> [jpeg]\n");
                ret = AVDK_ERR_INVAL;
                goto exit;
            }
            loop_count = (uint32_t)os_strtoul(argv[2], NULL, 10);
            if (loop_count == 0)
            {
                LOGE("Invalid loop count: %s\n", argv[2]);
                ret = AVDK_ERR_INVAL;
                goto exit;
            }
            if (argc >= 4 && os_strcmp(argv[3], "jpeg") == 0)
            {
                do_jpeg = 1;
            }
        }
        else if (os_strcmp(argv[1], "jpeg") == 0)
        {
            do_jpeg = 1;
        }

        /* Loop mode releases the captured frame + JPEG each round (~16.6MB peak). */
        loop_mode = (loop_count > 1U) ? 1U : 0U;

        LOGI("shoot4k: start, loop=%u jpeg=%u\n",
             (unsigned int)loop_count, (unsigned int)do_jpeg);

        for (idx = 0; idx < loop_count; idx++)
        {
            uint8_t *frame = NULL;
            uint32_t frame_len = 0;
            uint8_t *jpeg = NULL;
            uint32_t jpeg_len = 0;

            if (loop_mode)
            {
                LOGI("shoot4k: iter %u/%u\n",
                     (unsigned int)(idx + 1U), (unsigned int)loop_count);
            }

            ret = isp_shoot_4k_capture(&frame, &frame_len);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("isp_shoot_4k_capture failed: %d\n", ret);
                break;
            }

            s_isp_rdma_stitch_ctx.full_frame = frame;

            if (do_jpeg)
            {
                if (s_isp_last_jpeg != NULL)
                {
                    isp_jpeg_encode_release(s_isp_last_jpeg);
                    s_isp_last_jpeg = NULL;
                    s_isp_last_jpeg_len = 0;
                }

                if (s_shoot4k_out_fmt == SHOOT4K_FMT_YUYV422)
                {
                    ret = isp_jpeg_encode_yuyv422(frame, frame_len,
                                                  ISP_JPEG_ENC_4K_WIDTH, ISP_JPEG_ENC_4K_HEIGHT,
                                                  &jpeg, &jpeg_len);
                }
                else
                {
                    ret = isp_jpeg_encode_rgb565(frame, frame_len,
                                                 ISP_JPEG_ENC_4K_WIDTH, ISP_JPEG_ENC_4K_HEIGHT,
                                                 &jpeg, &jpeg_len);
                }
                if (ret != AVDK_ERR_OK)
                {
                    LOGE("shoot4k jpeg encode failed: %d\n", ret);
                }
                else
                {
                    s_isp_last_jpeg = jpeg;
                    s_isp_last_jpeg_len = jpeg_len;
                    os_printf("jpeg: %p len: %u\n", jpeg, jpeg_len);
                    if (!loop_mode)
                    {
                        /* Drop the captured 4K frame after encode. */
                        isp_shoot_4k_release(frame);
                        frame = NULL;
                    }
                }
            }

            ok_count++;

            if (loop_mode)
            {
                /* Loop mode: release JPEG each round (single shoot keeps it for uart_send). */
                if (jpeg != NULL)
                {
                    isp_jpeg_encode_release(jpeg);
                    jpeg = NULL;
                    jpeg_len = 0;
                }
                if (frame != NULL)
                {
                    isp_shoot_4k_release(frame);
                    frame = NULL;
                }
            }
        }

        if (loop_mode && do_jpeg)
        {
            os_printf("shoot4k loop: jpeg freed after each round, "
                      "'uart_send jpeg' unavailable\n");
        }

        LOGI("shoot4k: done, ok %u/%u, total %ums\n",
             (unsigned int)ok_count, (unsigned int)loop_count,
             (unsigned int)(rtos_get_time() - t_cmd));
    }
    else
    {
        LOGE("Usage: shoot4k <command>\n");
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

static const struct cli_command s_shoot4k_commands[] =
{
    {"shoot4k", " shoot4k capture | jpeg | loop | uart_init | uart_send", cli_shoot4k_cmd},
};

int shoot4k_cli_init(void)
{
    return cli_register_commands(s_shoot4k_commands,
                                 sizeof(s_shoot4k_commands) / sizeof(struct cli_command));
}
