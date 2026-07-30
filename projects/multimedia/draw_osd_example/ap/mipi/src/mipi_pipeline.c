/*
 * Local MIPI camera live video pipeline (camera + orchestration, like uvc/src/uvc_pipeline.c):
 *   GC2053 CSI + ISP MP(NV12 flexa) (in-file, merged from mipi_camera.c)
 *   --> bk_flexa_isp_gpu_bond (ISP MP flexa lines feed GPU)
 *   --> module-owned GPU (ISP NV12 -> rotate90 -> ARGB8888 + HV compress)
 *   --> LCD/DPU (shared draw_osd/src/display.c hx8399c 1080x1920 + DPU decompress)
 *
 * frame_done flushes directly to display module DPU. GPU handle is exposed via
 * mipi_pipeline_get_gpu_handle() for SRC_OVER overlay in osd_mipi.c.
 * Public API: mipi_pipeline_open/close/is_open/get_gpu_handle only.
 */
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <avdk_error.h>
#include <avdk_check.h>

#include <components/bk_gpu.h>
#include <components/bk_gpu_ctlr.h>
#include <components/bk_gpu_types.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_display.h>
#include <components/bk_flexa_bond.h>
#include <common/avdk_pixel_types.h>
#include <modules/pm.h>

/* Camera segment (ISP CSI) dependencies */
#include <components/bk_isp_camera.h>
#include <components/bk_camera_isp_ctlr.h>
#include <components/bk_camera_configs.h>
#include <components/bk_camera_sensor.h>
#include <components/bk_camera_bus.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include <driver/isp_base.h>     /* ISP_MP_CHN_ID */
#include <driver/isp_types.h>    /* isp_control_t (chn[].y_addr/buf_cnt) */
#include <sys_types.h>           /* __containerof */

#include "mipi_pipeline.h"
#include "display.h"    /* shared LCD/DPU: display_open / _get_dpu_handle / _close */

#define TAG "mipi_pipe"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define MIPI_GPU_FLEXA_LINES  16

/* ==================== MIPI CSI camera + ISP MP(NV12 flexa) (merged from mipi_camera.c) ====================
 *
 * Self-contained excerpt from multimedia_device_service/app_camera.c ISP CSI section
 * (doorbell board GC2053 pins: XCLK=59, I2C=69/70, reset=71), avoiding symbol clashes with
 * uvc path uvc_camera_turn_on / encode_frame_que when linking multimedia_device_service.
 * All camera APIs below are static.
 */
#define MIPI_CAM_PIN_SCL    GPIO_69
#define MIPI_CAM_PIN_SDA    GPIO_70
#define MIPI_CAM_PIN_RESET  GPIO_71
#define MIPI_CAM_PIN_XCLK   GPIO_59
#define MIPI_CAM_I2C_ID     1

typedef struct {
    bk_camera_sensor_handle_t   sensor_handle;
    bk_isp_camera_ctlr_handle_t camera_ctlr_handle;
    void                       *isp_handle;   /* == isp_control_t* */
} mipi_cam_handle_t;

static mipi_cam_handle_t s_cam = {0};

static avdk_err_t mipi_camera_close(void);   /* forward decl for mipi_camera_open err path */

/* MIPI sensor needs 1.8V iovdd + 1.2V dvdd AuxLDO; this module is sole voter for both */
static avdk_err_t mipi_camera_power(bool enable)
{
    int ldo_en = enable ? PM_AUXLDO_ENABLE : PM_AUXLDO_DISABLE;

    pm_auxldo_ctrl_cfg_t cfg = {0};
    cfg.ldo = AUXLDOS_SEL_1P8V;
    cfg.out = PM_AUXLDO_1P8V_OUT_1P8V;
    cfg.user = PM_AUXLDO_USER_CAMERA;
    cfg.state = ldo_en;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&cfg), TAG, "camera 1p8v ldo vote failed");
    LOGI("%s, camera 1p8v ldo vote %d ok\n", __func__, enable);

    cfg = (pm_auxldo_ctrl_cfg_t){0};
    cfg.ldo = AUXLDOS_SEL_1P2V;
    cfg.out = PM_AUXLDO_1P2V_OUT_1P2V;
    cfg.user = PM_AUXLDO_USER_CAMERA;
    cfg.state = ldo_en;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&cfg), TAG, "camera 1p2v ldo vote failed");
    LOGI("%s, camera 1p2v ldo vote %d ok\n", __func__, enable);
    rtos_delay_milliseconds(1);
    return AVDK_ERR_OK;
}

static void *mipi_camera_isp_handle_get(void)
{
    return s_cam.isp_handle;
}

/* Get ISP MP channel flexa context (GPU flexa src addr + buffer count) */
static avdk_err_t mipi_camera_get_flexa(uint8_t **buf, uint8_t *cnt)
{
    isp_control_t *isp = (isp_control_t *)s_cam.isp_handle;
    if (isp == NULL || buf == NULL || cnt == NULL) {
        return AVDK_ERR_INVAL;
    }
    *buf = (uint8_t *)(uintptr_t)isp->chn[ISP_MP_CHN_ID].y_addr;
    *cnt = isp->chn[ISP_MP_CHN_ID].buf_cnt;
    if (*buf == NULL || *cnt == 0) {
        LOGE("ISP MP flexa not ready (y_addr=%p cnt=%u)\n", *buf, *cnt);
        return AVDK_ERR_GENERIC;
    }
    return AVDK_ERR_OK;
}

/* 1) I2C bus + sensor detect + match (w,h,fps) mode */
static avdk_err_t mipi_sensor_init(uint16_t w, uint16_t h, uint8_t fps,
                                   bk_isp_camera_ctlr_config_t *isp_cfg)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    bk_camera_bus_t *bus = NULL;

    bk_camera_bus_config_t bus_cfg = CSI_CAM_BUS_I2C1_8BIT_2000TIMEOUT();
    bus_cfg.pin_scl = MIPI_CAM_PIN_SCL;
    bus_cfg.pin_sda = MIPI_CAM_PIN_SDA;
    bus_cfg.i2c_id  = MIPI_CAM_I2C_ID;

    bk_camera_sensor_config_t sensor_cfg = {
        .pin_reset = MIPI_CAM_PIN_RESET,
        .pin_pwdn  = -1,
        .pin_xclk  = MIPI_CAM_PIN_XCLK,
    };

    bus = bk_camera_bus_new(&bus_cfg);
    AVDK_GOTO_ON_FALSE(bus, AVDK_ERR_GENERIC, err, TAG, "camera bus new failed");
    AVDK_GOTO_ON_ERROR(bk_camera_bus_enable(bus), err, TAG, "camera bus enable failed");

    /* Same as doorbell app_camera.c: auto_detect right after bus_enable;
     * reset driven high inside gc2053_detect. No extra reset pulse/warm-up read. */
    sensor_cfg.bus = bus;
    s_cam.sensor_handle = bk_camera_sensor_auto_detect(&sensor_cfg, CSI_CAMERA_PORT);
    AVDK_GOTO_ON_FALSE(s_cam.sensor_handle, AVDK_ERR_NODEV, err, TAG, "sensor handle NULL");

    bk_camera_sensor_format_array_t fmts = {0};
    AVDK_GOTO_ON_ERROR(bk_camera_sensor_query_support_formats(s_cam.sensor_handle, &fmts),
                       err, TAG, "query support formats failed");
    AVDK_GOTO_ON_FALSE(fmts.size > 0, AVDK_ERR_INVAL, err, TAG, "format array size 0");

    int idx;
    for (idx = 0; idx < fmts.size; idx++) {
        if (fmts.format_array[idx].width == w &&
            fmts.format_array[idx].height == h &&
            fmts.format_array[idx].fps == fps) {
            break;
        }
    }
    if (idx >= fmts.size) {
        LOGE("no sensor mode %ux%u@%u\n", w, h, fps);
        ret = AVDK_ERR_INVAL;
        goto err;
    }
    LOGI("sensor mode[%d] %ux%u@%u\n", idx, w, h, fps);

    isp_cfg->input_pixel_fmt = fmts.format_array[idx].output_pixel_fmt;

    const void *sensor_object = bk_camera_sensor_get_sensor_object(s_cam.sensor_handle);
    AVDK_GOTO_ON_FALSE(sensor_object, AVDK_ERR_GENERIC, err, TAG, "sensor object NULL");
    isp_cfg->sensor_object = sensor_object;

    return AVDK_ERR_OK;

err:
    if (s_cam.sensor_handle) {
        bk_camera_sensor_destroy(s_cam.sensor_handle);
        s_cam.sensor_handle = NULL;
    }
    if (bus) {
        bk_camera_bus_disable(bus);
        bk_camera_bus_delete(bus);
    }
    return ret;
}

/* 2) ISP controller + MP channel (NV12 flexa) */
static avdk_err_t mipi_mp_channel_on(uint16_t w, uint16_t h, bk_isp_camera_ctlr_config_t *isp_cfg)
{
    AVDK_RETURN_ON_ERROR(bk_camera_isp_ctlr_new(&s_cam.camera_ctlr_handle), TAG, "isp ctlr new failed");
    bk_camera_isp_ctlr_t *control = __containerof(s_cam.camera_ctlr_handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_GENERIC, TAG, "isp control NULL");

    AVDK_RETURN_ON_ERROR(bk_isp_camera_dev_init(s_cam.camera_ctlr_handle), TAG, "isp dev init failed");
    s_cam.isp_handle = control->isp_handle;

    AVDK_RETURN_ON_ERROR(bk_isp_camera_port_init(s_cam.camera_ctlr_handle, isp_cfg), TAG, "isp port init failed");

    bk_isp_camera_channel_config_t inst = CAM_MP_NV12_RB_INSTANCE_CONFIG(w, h);
    inst.port_id      = ISP_MP_CHN_ID;
    inst.enable_flexa = 1;
    inst.work_mode    = 1;
    inst.format       = BK_PIXEL_FORMAT_NV12;

    AVDK_RETURN_ON_ERROR(bk_isp_camera_channel_open(s_cam.camera_ctlr_handle, ISP_MP_CHN_ID, &inst),
                         TAG, "isp channel open failed");
    return AVDK_ERR_OK;
}

static avdk_err_t mipi_camera_open(uint16_t width, uint16_t height, uint8_t fps)
{
    avdk_err_t ret;
    bool powered = false;

    AVDK_RETURN_ON_FALSE((s_cam.camera_ctlr_handle == NULL), AVDK_ERR_BUSY, TAG, "camera already open");

    AVDK_RETURN_ON_ERROR(mipi_camera_power(true), TAG, "camera power on failed");
    powered = true;

    bk_isp_camera_ctlr_config_t isp_cfg = CAM_CSI_DEFAULT_RAW10_CONFIG(width, height, fps);

    ret = mipi_sensor_init(width, height, fps, &isp_cfg);
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, ret, err, TAG, "mipi_sensor_init failed");

    ret = mipi_mp_channel_on(width, height, &isp_cfg);
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, ret, err, TAG, "mipi_mp_channel_on failed");

    /* 3) Start sensor streaming */
    bk_camera_sensor_init(s_cam.sensor_handle);
    bk_camera_sensor_format_t fmt = { .width = width, .height = height, .fps = fps };
    ret = bk_camera_sensor_set_format(s_cam.sensor_handle, &fmt);
    AVDK_GOTO_ON_FALSE(ret == AVDK_ERR_OK, ret, err, TAG, "sensor set format failed");

    LOGI("mipi camera open ok: %ux%u@%u\n", width, height, fps);
    return AVDK_ERR_OK;

err:
    (void)mipi_camera_close();
    (void)powered;
    return (ret != AVDK_ERR_OK) ? ret : AVDK_ERR_GENERIC;
}

static avdk_err_t mipi_camera_close(void)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_cam.camera_ctlr_handle) {
        if (bk_isp_camera_channel_state_get(s_cam.camera_ctlr_handle, ISP_MP_CHN_ID) == ISP_CHANNEL_STATE_TURN_ON) {
            ret = bk_isp_camera_channel_close(s_cam.camera_ctlr_handle, ISP_MP_CHN_ID);
            if (ret != AVDK_ERR_OK) {
                LOGE("isp channel close failed %d\n", ret);
            }
        }
        (void)bk_isp_camera_deinit(s_cam.camera_ctlr_handle);
        (void)bk_isp_camera_delete(s_cam.camera_ctlr_handle);
        s_cam.camera_ctlr_handle = NULL;
    }

    bk_camera_bus_t *bus = bk_camera_bus_get();
    if (bus != NULL) {
        (void)bk_camera_bus_disable(bus);
        (void)bk_camera_bus_delete(bus);
    }

    s_cam.isp_handle = NULL;
    if (s_cam.sensor_handle) {
        bk_camera_sensor_destroy(s_cam.sensor_handle);
        s_cam.sensor_handle = NULL;
    }

    (void)mipi_camera_power(false);
    LOGI("mipi camera closed\n");
    return AVDK_ERR_OK;
}

/* ==================== Full pipeline orchestration (four public entry points) ==================== */

static bk_gpu_ctlr_handle_t s_mipi_gpu  = NULL;
static void               *s_isp_gpu_bond = NULL;

static void *mipi_frame_malloc(uint32_t size)
{
    void *f = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    if (f == NULL) {
        LOGE("gpu frame malloc %u failed\n", size);
    }
    return f;
}

static avdk_err_t mipi_frame_free(void *ptr)
{
    bk_frame_buffer_free(ptr);
    return AVDK_ERR_OK;
}

static void mipi_frame_done(void *frame, uint32_t frame_size, void *args)
{
    (void)frame_size;
    (void)args;
    void *dpu = display_get_dpu_handle();
    if (dpu == NULL) {
        mipi_frame_free(frame);
        return;
    }
    if (bk_display_flush(dpu, frame, mipi_frame_free) != AVDK_ERR_OK) {
        mipi_frame_free(frame);
    }
}

static avdk_err_t mipi_gpu_enable(uint16_t src_w, uint16_t src_h, uint8_t *src_buf, uint8_t buf_cnt)
{
    if (s_mipi_gpu != NULL) {
        return AVDK_ERR_BUSY;
    }

    bk_gpu_ctlr_config_t cfg;
    os_memset(&cfg, 0, sizeof(cfg));
    cfg.rotate_degree = 90;
    cfg.src_width  = src_w;
    cfg.src_height = src_h;
    cfg.dst_width  = 1920;                 /* after rotate90: portrait 1080x1920 */
    cfg.dst_height = 1080;
    cfg.src_format = BK_PIXEL_FORMAT_NV12;
    cfg.dst_format = BK_PIXEL_FORMAT_ARGB8888;
    cfg.compress   = true;                 /* HV compress; DPU decompresses for display */
    cfg.scale      = (src_w != 1920);      /* 1080p passthrough; 720p upscaled to 1080p */
    cfg.flexa          = true;
    cfg.flexa_lines    = MIPI_GPU_FLEXA_LINES;
    cfg.flexa_buff_cnt = buf_cnt;
    cfg.src_buffer     = src_buf;
    cfg.flexa_line_done = NULL;
    cfg.flexa_line_done_args = NULL;
    cfg.frame_malloc = mipi_frame_malloc;
    cfg.frame_free   = mipi_frame_free;
    cfg.frame_done   = mipi_frame_done;
    cfg.frame_done_args = NULL;

    avdk_err_t ret = bk_gpu_ctlr_new(&s_mipi_gpu, &cfg);
    if (ret != AVDK_ERR_OK) { LOGE("gpu ctlr new %d\n", ret); return ret; }
    ret = bk_gpu_init(s_mipi_gpu);
    if (ret != AVDK_ERR_OK) { LOGE("gpu init %d\n", ret); goto del; }
    ret = bk_gpu_open(s_mipi_gpu);
    if (ret != AVDK_ERR_OK) { LOGE("gpu open %d\n", ret); goto deinit; }
    return AVDK_ERR_OK;

deinit:
    (void)bk_gpu_deinit(s_mipi_gpu);
del:
    (void)bk_gpu_delete(s_mipi_gpu);
    s_mipi_gpu = NULL;
    return ret;
}

bool mipi_pipeline_is_open(void)
{
    return s_isp_gpu_bond != NULL;
}

bk_gpu_ctlr_handle_t mipi_pipeline_get_gpu_handle(void)
{
    return s_mipi_gpu;
}

avdk_err_t mipi_pipeline_open(uint16_t width, uint16_t height, uint8_t fps)
{
    avdk_err_t ret;
    uint8_t *flexa_buf = NULL;
    uint8_t  flexa_cnt = 0;

    if (mipi_pipeline_is_open()) {
        LOGW("mipi pipeline already open\n");
        return AVDK_ERR_OK;
    }

    /* 1) LCD + DPU (display_open enables panel VDDIO; GPU owned by this module) */
    ret = display_open();
    if (ret != AVDK_ERR_OK) {
        LOGE("open LCD failed %d\n", ret);
        return ret;
    }

    /* 2) MIPI CSI + ISP MP(NV12 flexa) */
    ret = mipi_camera_open(width, height, fps);
    if (ret != AVDK_ERR_OK) {
        LOGE("mipi_camera_open failed %d\n", ret);
        goto err_disp;
    }

    ret = mipi_camera_get_flexa(&flexa_buf, &flexa_cnt);
    if (ret != AVDK_ERR_OK) {
        goto err_cam;
    }

    /* 3) GPU (ISP NV12 -> ARGB8888 compress, rotate90) */
    ret = mipi_gpu_enable(width, height, flexa_buf, flexa_cnt);
    if (ret != AVDK_ERR_OK) {
        goto err_cam;
    }

    /* 4) ISP -> GPU flexa bond */
    ret = bk_flexa_isp_gpu_bond_start(&s_isp_gpu_bond, mipi_camera_isp_handle_get(), s_mipi_gpu);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_flexa_isp_gpu_bond_start failed %d\n", ret);
        goto err_gpu;
    }

    LOGI("mipi pipeline open ok: %ux%u@%u\n", width, height, fps);
    return AVDK_ERR_OK;

err_gpu:
    (void)bk_gpu_close(s_mipi_gpu);
    (void)bk_gpu_deinit(s_mipi_gpu);
    (void)bk_gpu_delete(s_mipi_gpu);
    s_mipi_gpu = NULL;
err_cam:
    (void)mipi_camera_close();
err_disp:
    (void)display_close();
    return ret;
}

avdk_err_t mipi_pipeline_close(void)
{
    if (s_isp_gpu_bond != NULL) {
        bk_flexa_isp_gpu_bond_stop(s_isp_gpu_bond);
        s_isp_gpu_bond = NULL;
    }
    if (s_mipi_gpu != NULL) {
        (void)bk_gpu_close(s_mipi_gpu);
        (void)bk_gpu_deinit(s_mipi_gpu);
        (void)bk_gpu_delete(s_mipi_gpu);
        s_mipi_gpu = NULL;
    }
    (void)mipi_camera_close();
    (void)display_close();
    LOGI("mipi pipeline closed\n");
    return AVDK_ERR_OK;
}
