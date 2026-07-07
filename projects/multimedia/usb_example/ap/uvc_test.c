/* =======================================================================
 *  USB HOST mode: UVC camera MJPEG receive self-test.
 *
 *  Adds a `uvc` CLI to the usb_example project that:
 *    1. Frees the default MSC device gadget and brings the shared MUSB
 *       controller up in HOST mode (bk_usb_driver_init + bk_usb_open(HOST)
 *       via the hub-multiple-classes power_on helper).
 *    2. Powers / enumerates the UVC camera on the requested hub port.
 *    3. Opens an MJPEG stream (default 1920x1080@30, overridable on the CLI).
 *    4. Counts *complete* MJPEG frames in the frame_complete callback,
 *       validating each one carries a JPEG SOI (0xFFD8) header and EOI
 *       (0xFFD9) trailer, and declares PASS once >= UVC_TEST_TARGET_FRAMES
 *       valid frames have been received.
 *
 *  Usage:
 *    uvc test [port] [width] [height] [fps]   - run the headline self-test
 *    uvc open [port] [width] [height] [fps]   - just open + keep streaming
 *    uvc close [port]                          - stop + close a stream
 *
 *  All numeric args are optional; defaults are port=1, 1920x1080, 30fps.
 * ===================================================================== */
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>

#if CONFIG_USB_CAMERA

#include <components/bk_frame_buffer.h>
#include <components/usb_types.h>
#include <components/bk_uvc_camera.h>
#include <components/usbh_hub_multiple_classes_api.h>
#include <avdk_error.h>

#if CONFIG_CLI
#include "cli.h"
#endif

#define TAG "uvc_test"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

/* The default MSC gadget owns the controller at boot; tear it down before
 * we flip into host mode. bk_usb_driver_init() registers the host class
 * drivers (UVC/hub) and is required before bk_usb_open(HOST). */
extern bk_err_t bk_usb_driver_init(void);
extern int msc_storage_deinit(void);

#define UVC_TEST_TARGET_FRAMES   20u      /* prove at least this many complete frames */
#define UVC_TEST_TIMEOUT_MS      20000u   /* give 1080p enumeration + streaming time */
#define UVC_TEST_CONNECT_MS      10000u   /* 1080p enum + UVC class id can take several seconds */
#define UVC_TEST_CONNECT_POLL_MS 100u

#define UVC_DEF_WIDTH            1920
#define UVC_DEF_HEIGHT          1080
#define UVC_DEF_FPS             30

static bk_uvc_ctlr_handle_t s_uvc_handle[UVC_PORT_MAX] = {NULL};
static beken_semaphore_t    s_uvc_connect_sem = NULL;
static beken_semaphore_t    s_uvc_done_sem    = NULL;

static volatile uint32_t s_uvc_valid_frames;   /* SOI+EOI verified complete frames */
static volatile uint32_t s_uvc_bad_frames;     /* delivered but malformed / errored */
static volatile uint32_t s_uvc_total_frames;   /* every frame_complete invocation */
static volatile uint8_t  s_uvc_test_running;
static volatile uint8_t  s_uvc_host_ready;
static volatile uint8_t  s_fb_inited;

/* ----------------------------------------------------------------- frame I/O */

static frame_buffer_t *uvc_camera_frame_malloc(bk_image_format_t format, uint32_t size)
{
    (void)format;
    frame_buffer_t *frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, size + sizeof(frame_buffer_t));
    if (frame == NULL) {
        return NULL;
    }
    os_memset(frame, 0, sizeof(frame_buffer_t));
    frame->frame = (uint8_t *)(frame + 1);
    frame->size = size;
    return frame;
}

static void uvc_camera_frame_complete(uint8_t port, bk_image_format_t format,
                                      frame_buffer_t *frame, int result)
{
    (void)port;
    (void)format;

    if (s_uvc_test_running) {
        s_uvc_total_frames++;

        bool ok = (result == 0) && frame && (frame->frame != NULL) && (frame->length >= 4);
        if (ok) {
            const uint8_t *d = frame->frame;
            uint32_t n = frame->length;
            /* JPEG: SOI 0xFFD8 at the start, EOI 0xFFD9 at the end. */
            bool soi = (d[0] == 0xFF) && (d[1] == 0xD8);
            bool eoi = (d[n - 2] == 0xFF) && (d[n - 1] == 0xD9);
            ok = soi && eoi;
        }

        if (ok) {
            uint32_t idx = ++s_uvc_valid_frames;
            if (idx <= 3 || idx == UVC_TEST_TARGET_FRAMES || (idx % 5 == 0)) {
                LOGI("MJPEG frame #%u OK seq=%u len=%u (%ux%u)\n",
                     idx, frame->sequence, frame->length, frame->width, frame->height);
            }
            if (idx == UVC_TEST_TARGET_FRAMES && s_uvc_done_sem) {
                rtos_set_semaphore(&s_uvc_done_sem);
            }
        } else {
            s_uvc_bad_frames++;
            LOGW("frame dropped: result=%d len=%u (not a complete JPEG)\n",
                 result, frame ? frame->length : 0);
        }
    }

    if (frame) {
        bk_frame_buffer_free(frame);
    }
}

static void uvc_event_callback(uvc_state_t state, void *user_data)
{
    (void)user_data;
    LOGI("uvc state change: %s\n", state == UVC_CONNECTED ? "CONNECTED" : "DISCONNECTED");
}

static const bk_uvc_callback_t s_uvc_cbs = {
    .frame_malloc   = uvc_camera_frame_malloc,
    .frame_complete = uvc_camera_frame_complete,
    .state_change_cb = uvc_event_callback,
    .user_data      = NULL,
};

/* ------------------------------------------------------------- host bring-up */

/* Boot default on usb_example is the MSC device gadget. Free it and make sure
 * the host class drivers are registered so the UVC power_on helper can
 * bk_usb_open(HOST) the shared controller cleanly. */
static void uvc_prepare_host(void)
{
    if (s_uvc_host_ready) {
        return;
    }
    if (!s_fb_inited) {
        bk_frame_buffer_init();
        s_fb_inited = 1;
    }
    /* Returns BK_FAIL if already initialised elsewhere (e.g. udisk); harmless. */
    (void)bk_usb_driver_init();
    /* Idempotent: only deinits if the MSC gadget is currently up. */
    (void)msc_storage_deinit();
    s_uvc_host_ready = 1;
    LOGI("host prepared (MSC gadget released, host class drivers registered)\n");
}

static void uvc_device_connect_callback(bk_usb_hub_port_info *port_info, void *arg)
{
    (void)port_info;
    beken_semaphore_t sem = (beken_semaphore_t)arg;
    LOGI("UVC device connected\n");
    if (sem) {
        rtos_set_semaphore(&sem);
    }
}

static avdk_err_t uvc_power_on_wait(uint8_t port, uint32_t timeout_ms)
{
    if (s_uvc_connect_sem == NULL) {
        if (rtos_init_semaphore(&s_uvc_connect_sem, 1) != BK_OK) {
            LOGE("connect sem init failed\n");
            return AVDK_ERR_GENERIC;
        }
    }
    /* drain any stale token */
    rtos_get_semaphore(&s_uvc_connect_sem, BEKEN_NO_WAIT);

    bk_usbh_hub_port_register_connect_callback(port, USB_UVC_DEVICE,
                                               uvc_device_connect_callback, s_uvc_connect_sem);
    bk_usbh_hub_multiple_devices_power_on(USB_HOST_MODE, port, USB_UVC_DEVICE);

    /* The connect callback / device-param readiness for a 1080p UVC camera can
     * trail the raw USB connect by a few seconds (full descriptor parse + UVC
     * class identification). Poll check_device rather than trust a single
     * callback edge: it returns OK only once usb_device_param is populated,
     * which is exactly the state uvc_check_mjpeg() needs. */
    bk_usb_hub_port_info *port_info = NULL;
    uint32_t waited = 0;
    while (waited <= timeout_ms) {
        if (bk_usbh_hub_port_check_device(port, USB_UVC_DEVICE, &port_info) == AVDK_ERR_OK
            && port_info != NULL && port_info->usb_device_param != NULL) {
            LOGI("camera ready on port %u after %u ms\n", port, waited);
            return AVDK_ERR_OK;
        }
        rtos_delay_milliseconds(UVC_TEST_CONNECT_POLL_MS);
        waited += UVC_TEST_CONNECT_POLL_MS;
    }

    return AVDK_ERR_TIMEOUT;
}

static void uvc_power_off(uint8_t port)
{
    bk_usbh_hub_port_register_connect_callback(port, USB_UVC_DEVICE, NULL, NULL);
    bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, port, USB_UVC_DEVICE);
}

/* --------------------------------------------------------- MJPEG capability */

/* Confirm the camera advertises the requested MJPEG resolution; if the exact
 * fps is not listed, fall back to the first fps the camera reports for it. */
static avdk_err_t uvc_check_mjpeg(uint8_t port, bk_cam_uvc_config_t *cfg)
{
    bk_usb_hub_port_info *port_info = NULL;
    avdk_err_t ret = bk_usbh_hub_port_check_device(port, USB_UVC_DEVICE, &port_info);
    if (ret != AVDK_ERR_OK || port_info == NULL) {
        LOGE("check_device failed (camera not present on port %u)\n", port);
        return AVDK_ERR_NODEV;
    }

    bk_uvc_device_brief_info_t *info = (bk_uvc_device_brief_info_t *)port_info->usb_device_param;
    if (info == NULL) {
        LOGE("device brief info NULL\n");
        return AVDK_ERR_INVAL;
    }

    LOGI("camera VID:PID=%04x:%04x, mjpeg_frame_num=%u\n",
         info->vendor_id, info->product_id, info->all_frame.mjpeg_frame_num);

    for (uint8_t i = 0; i < info->all_frame.mjpeg_frame_num; i++) {
        uint16_t w = info->all_frame.mjpeg_frame[i].width;
        uint16_t h = info->all_frame.mjpeg_frame[i].height;
        LOGI("  MJPEG[%u] %ux%u\n", i, w, h);
        if (w == cfg->width && h == cfg->height) {
            uint8_t fps_ok = 0;
            for (int j = 0; j < info->all_frame.mjpeg_frame[i].fps_num; j++) {
                if (info->all_frame.mjpeg_frame[i].fps[j] == cfg->fps) {
                    fps_ok = 1;
                    break;
                }
            }
            if (!fps_ok && info->all_frame.mjpeg_frame[i].fps_num > 0) {
                LOGW("fps %u not listed for %ux%u, falling back to %u\n",
                     cfg->fps, cfg->width, cfg->height, info->all_frame.mjpeg_frame[i].fps[0]);
                cfg->fps = info->all_frame.mjpeg_frame[i].fps[0];
            }
            return AVDK_ERR_OK;
        }
    }

    LOGE("camera does not advertise MJPEG %ux%u\n", cfg->width, cfg->height);
    return AVDK_ERR_UNSUPPORTED;
}

/* ------------------------------------------------------------- open / close */

static avdk_err_t uvc_stream_open(bk_cam_uvc_config_t *cfg)
{
    avdk_err_t ret;
    bk_uvc_ctlr_handle_t handle = NULL;

    uvc_prepare_host();

    ret = uvc_power_on_wait(cfg->port, UVC_TEST_CONNECT_MS);
    if (ret != AVDK_ERR_OK) {
        LOGE("camera not connected on port %u (ret=%d)\n", cfg->port, ret);
        goto fail;
    }

    ret = uvc_check_mjpeg(cfg->port, cfg);
    if (ret != AVDK_ERR_OK) {
        goto fail;
    }

    ret = bk_uvc_ctrl_new(&handle, &s_uvc_cbs);
    if (ret != AVDK_ERR_OK) { LOGE("ctrl_new failed %d\n", ret); goto fail; }

    ret = bk_uvc_init(handle);
    if (ret != AVDK_ERR_OK) { LOGE("uvc_init failed %d\n", ret); goto fail; }

    ret = bk_uvc_open(handle, cfg);
    if (ret != AVDK_ERR_OK) { LOGE("uvc_open failed %d\n", ret); goto fail; }

    s_uvc_handle[cfg->port - 1] = handle;
    LOGI("UVC opened: port=%u MJPEG %ux%u@%u\n", cfg->port, cfg->width, cfg->height, cfg->fps);
    return AVDK_ERR_OK;

fail:
    if (handle) {
        bk_uvc_deinit(handle);
        bk_uvc_delete(handle);
    }
    uvc_power_off(cfg->port);
    return ret;
}

static avdk_err_t uvc_stream_close(uint8_t port)
{
    bk_uvc_ctlr_handle_t handle = s_uvc_handle[port - 1];
    if (handle == NULL) {
        return AVDK_ERR_OK;
    }
    bk_uvc_close(handle);
    bk_uvc_deinit(handle);
    bk_uvc_delete(handle);
    s_uvc_handle[port - 1] = NULL;
    uvc_power_off(port);
    LOGI("UVC closed: port=%u\n", port);
    return AVDK_ERR_OK;
}

/* --------------------------------------------------------------- self test */

static int uvc_run_test(bk_cam_uvc_config_t *cfg)
{
    int rc = -1;

    LOGI("==== UVC MJPEG receive test BEGIN (port=%u %ux%u@%u, target=%u frames) ====\n",
         cfg->port, cfg->width, cfg->height, cfg->fps, UVC_TEST_TARGET_FRAMES);

    if (s_uvc_handle[cfg->port - 1] != NULL) {
        LOGE("port %u already open; run `uvc close %u` first\n", cfg->port, cfg->port);
        return -1;
    }

    if (s_uvc_done_sem == NULL) {
        if (rtos_init_semaphore(&s_uvc_done_sem, 1) != BK_OK) {
            LOGE("done sem init failed\n");
            return -1;
        }
    }
    rtos_get_semaphore(&s_uvc_done_sem, BEKEN_NO_WAIT);

    s_uvc_valid_frames = 0;
    s_uvc_bad_frames   = 0;
    s_uvc_total_frames = 0;
    s_uvc_test_running = 1;

    if (uvc_stream_open(cfg) != AVDK_ERR_OK) {
        LOGE("stream open failed\n");
        goto out;
    }

    /* Block until we have collected the target number of complete frames. */
    if (rtos_get_semaphore(&s_uvc_done_sem, UVC_TEST_TIMEOUT_MS) == BK_OK) {
        rc = 0;
    } else {
        LOGE("timeout: only %u/%u valid frames in %u ms\n",
             s_uvc_valid_frames, UVC_TEST_TARGET_FRAMES, UVC_TEST_TIMEOUT_MS);
    }

out:
    s_uvc_test_running = 0;
    uvc_stream_close(cfg->port);

    LOGI("---- stats: valid=%u bad=%u total=%u ----\n",
         s_uvc_valid_frames, s_uvc_bad_frames, s_uvc_total_frames);
    LOGI("==== UVC MJPEG receive test %s ====\n",
         (rc == 0) ? "PASS" : "FAIL");
    return rc;
}

/* --------------------------------------------------------------------- CLI */

#if CONFIG_CLI
static void cli_uvc_help(void)
{
    LOGI("usage:\n");
    LOGI("  uvc test  [port] [w] [h] [fps] - open MJPEG + receive >=%u complete frames\n",
         UVC_TEST_TARGET_FRAMES);
    LOGI("  uvc open  [port] [w] [h] [fps] - open MJPEG stream and keep running\n");
    LOGI("  uvc close [port]               - stop + close a stream\n");
    LOGI("  defaults: port=1 %ux%u@%u\n", UVC_DEF_WIDTH, UVC_DEF_HEIGHT, UVC_DEF_FPS);
}

static void uvc_fill_config(bk_cam_uvc_config_t *cfg, int argc, char **argv)
{
    cfg->format   = BK_IMAGE_FORMAT_MJPEG;
    cfg->port     = 1;
    cfg->width    = UVC_DEF_WIDTH;
    cfg->height   = UVC_DEF_HEIGHT;
    cfg->fps      = UVC_DEF_FPS;
    cfg->drop_num = 0;

    if (argc >= 3) cfg->port   = (uint8_t)os_strtoul(argv[2], NULL, 10);
    if (argc >= 4) cfg->width  = (uint16_t)os_strtoul(argv[3], NULL, 10);
    if (argc >= 5) cfg->height = (uint16_t)os_strtoul(argv[4], NULL, 10);
    if (argc >= 6) cfg->fps    = os_strtoul(argv[5], NULL, 10);
}

static void cli_uvc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc < 2) {
        cli_uvc_help();
        return;
    }

    bk_cam_uvc_config_t cfg;
    uvc_fill_config(&cfg, argc, argv);

    if (cfg.port == 0 || cfg.port > UVC_PORT_MAX) {
        LOGE("port %u out of range (1..%u)\n", cfg.port, UVC_PORT_MAX);
        return;
    }

    if (os_strcmp(argv[1], "test") == 0) {
        (void)uvc_run_test(&cfg);
    } else if (os_strcmp(argv[1], "open") == 0) {
        if (s_uvc_handle[cfg.port - 1] != NULL) {
            LOGE("port %u already open\n", cfg.port);
            return;
        }
        s_uvc_test_running = 1;          /* keep counting/validating frames */
        s_uvc_valid_frames = 0;
        s_uvc_bad_frames   = 0;
        s_uvc_total_frames = 0;
        if (uvc_stream_open(&cfg) != AVDK_ERR_OK) {
            s_uvc_test_running = 0;
            LOGE("uvc open failed\n");
        }
    } else if (os_strcmp(argv[1], "close") == 0) {
        s_uvc_test_running = 0;
        (void)uvc_stream_close(cfg.port);
    } else {
        cli_uvc_help();
    }
}

COMPONENTS_CLI_CMD_EXPORT
static const struct cli_command s_uvc_cmds[] = {
    {"uvc", "uvc test|open|close [port] [w] [h] [fps]", cli_uvc_cmd},
};
#endif /* CONFIG_CLI */

#endif /* CONFIG_USB_CAMERA */
