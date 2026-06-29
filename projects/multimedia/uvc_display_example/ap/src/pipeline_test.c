#include <os/str.h>
#include <components/log.h>

#include <components/usb_types.h>
#include <components/bk_uvc_camera.h>
#include <components/bk_flexa_bond.h>

#include "decode_test.h"
#include "test_cli.h"

#define TAG "pipeline"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

extern bk_uvc_ctlr_handle_t uvc_camera_turn_on(bk_cam_uvc_config_t *config);
extern avdk_err_t uvc_camera_turn_off(bk_uvc_ctlr_handle_t handle);

static void *s_mjpegd_gpu_bond = NULL;
static bk_uvc_ctlr_handle_t s_pipeline_uvc_handle = NULL;

static avdk_err_t pipeline_open(uint8_t port, uint16_t width, uint16_t height, uint8_t fps)
{
    avdk_err_t ret;
    bk_jpeg_decode_ctlr_handle_t decode_handle = NULL;
    bk_gpu_ctlr_handle_t gpu_handle = NULL;
    uint8_t *flexa_buf = NULL;
    uint8_t flexa_cnt = 0;

    ret = decode_test_open(width, height, BK_IMAGE_FORMAT_MJPEG, 1);
    if (ret != AVDK_ERR_OK) {
        LOGE("decode_test_open failed, ret=%d\n", ret);
        return ret;
    }

    ret = decode_test_get_flexa_context(&flexa_buf, &flexa_cnt);
    if (ret != AVDK_ERR_OK || flexa_buf == NULL || flexa_cnt == 0) {
        LOGE("decode_test_get_flexa_context failed\n");
        goto err_decode;
    }

    ret = display_test_open_with_gpu_flexa(width, height, flexa_buf, flexa_cnt);
    if (ret != AVDK_ERR_OK) {
        LOGE("display_test_open_with_gpu_flexa failed, ret=%d\n", ret);
        goto err_decode;
    }

    gpu_handle = display_test_get_gpu_handle();
    if (gpu_handle == NULL) {
        LOGE("display_test_get_gpu_handle failed\n");
        goto err_display;
    }

    ret = decode_test_get_handle(&decode_handle);
    if (ret != AVDK_ERR_OK || decode_handle == NULL) {
        LOGE("decode_test_get_handle failed\n");
        goto err_display;
    }

    ret = bk_flexa_mjpegd_gpu_bond_start(&s_mjpegd_gpu_bond, decode_handle, gpu_handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_flexa_mjpegd_gpu_bond_start failed, ret=%d\n", ret);
        goto err_display;
    }

    bk_cam_uvc_config_t uvc_cfg = MEDIA_UVC_MJPEG_864X480_30FPS_CONFIG();
    uvc_cfg.port = port;
    uvc_cfg.width = width;
    uvc_cfg.height = height;
    uvc_cfg.fps = fps;
    uvc_cfg.format = BK_IMAGE_FORMAT_MJPEG;

    s_pipeline_uvc_handle = uvc_camera_turn_on(&uvc_cfg);
    if (s_pipeline_uvc_handle == NULL) {
        LOGE("uvc_camera_turn_on failed\n");
        goto err_bond;
    }

    LOGI("pipeline open ok: port=%u %ux%u@%u\n", port, width, height, fps);
    return AVDK_ERR_OK;

err_bond:
    bk_flexa_mjpegd_gpu_bond_stop(s_mjpegd_gpu_bond);
    s_mjpegd_gpu_bond = NULL;
err_display:
    display_test_close();
err_decode:
    decode_test_close();
    return ret;
}

static avdk_err_t pipeline_close(void)
{
    if (s_pipeline_uvc_handle != NULL) {
        (void)uvc_camera_turn_off(s_pipeline_uvc_handle);
        s_pipeline_uvc_handle = NULL;
    }

    if (s_mjpegd_gpu_bond != NULL) {
        bk_flexa_mjpegd_gpu_bond_stop(s_mjpegd_gpu_bond);
        s_mjpegd_gpu_bond = NULL;
    }

    (void)display_test_close();
    (void)decode_test_close();

    LOGI("pipeline close ok\n");
    return AVDK_ERR_OK;
}

void cli_pipeline_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc < 2) {
        LOGE("Usage: pipeline open [port width height fps] | close\n");
        return;
    }

    if (os_strcmp(argv[1], "open") == 0) {
        uint8_t port = 1;
        uint16_t width = 1920;
        uint16_t height = 1080;
        uint8_t fps = 30;

        if (argc >= 6) {
            port = (uint8_t)os_strtoul(argv[2], NULL, 10);
            width = (uint16_t)os_strtoul(argv[3], NULL, 10);
            height = (uint16_t)os_strtoul(argv[4], NULL, 10);
            fps = (uint8_t)os_strtoul(argv[5], NULL, 10);
        }

        if (s_pipeline_uvc_handle != NULL) {
            LOGE("pipeline already open\n");
            return;
        }

        if (pipeline_open(port, width, height, fps) != AVDK_ERR_OK) {
            LOGE("pipeline open failed\n");
        }
    } else if (os_strcmp(argv[1], "close") == 0) {
        if (pipeline_close() != AVDK_ERR_OK) {
            LOGE("pipeline close failed\n");
        }
    } else {
        LOGE("Usage: pipeline open [port width height fps] | close\n");
    }
}
