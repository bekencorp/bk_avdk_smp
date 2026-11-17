#pragma once

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <common/avdk_pixel_types.h>
#include <components/log.h>
#include <avdk_error.h>

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define TAG "shoot4k"

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

int shoot4k_cli_init(void);
void cli_shoot4k_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

/**
 * @brief Run the 4K shot pipeline (4K RAW -> RDMA half replay -> stitched 4K frame).
 *        Output is a 3840x2160 16bpp frame (RGB565 or YUYV422, selected at
 *        runtime with `shoot4k fmt`) written in place into the borrowed RAW
 *        buffer (single 16.6MB buffer).
 *        On success, *out_frame holds a frame buffer of size *out_len.
 *        Caller must release with isp_shoot_4k_release().
 */
avdk_err_t isp_shoot_4k_capture(uint8_t **out_frame, uint32_t *out_len);
void isp_shoot_4k_release(uint8_t *frame);

/**
 * @brief Encode one RGB565 frame to JPEG (direct VCENC JpegEnc API).
 *        On success, *out_jpeg is a MEM_SLAB_HEAP_CODED payload; release with
 *        isp_jpeg_encode_release().
 */
avdk_err_t isp_jpeg_encode_rgb565(uint8_t *rgb565, uint32_t rgb565_len,
                                  uint32_t width, uint32_t height,
                                  uint8_t **out_jpeg, uint32_t *out_jpeg_len);

/**
 * @brief Encode one YUYV422 frame to JPEG (direct VCENC JpegEnc API).
 *        On success, *out_jpeg is a MEM_SLAB_HEAP_CODED payload; release with
 *        isp_jpeg_encode_release().
 */
avdk_err_t isp_jpeg_encode_yuyv422(uint8_t *yuyv, uint32_t yuyv_len,
                                   uint32_t width, uint32_t height,
                                   uint8_t **out_jpeg, uint32_t *out_jpeg_len);
void isp_jpeg_encode_release(uint8_t *jpeg);
