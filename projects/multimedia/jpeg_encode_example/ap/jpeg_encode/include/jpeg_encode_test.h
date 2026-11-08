#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

/*
 * 软件整帧编码的 OSD 合成方式：
 * 1 - 输入源可以修改：直接在输入 buffer 上叠加 OSD，无整帧拷贝（最快，但 OSD 会烧进源图，
 *     要求该 buffer 可写且编码期间不被其它模块使用）；
 * 0 - 输入源不能修改：先整帧拷贝到内部 staging buffer 再叠加，源图保持原样。
 */
#ifndef JPEG_SW_OSD_IN_PLACE
#define JPEG_SW_OSD_IN_PLACE              (1)
#endif

void cli_jpeg_encode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int cli_jpeg_encode_init(void);

/** VCENC JPEG frame-mode test using bk_jpeg_encode_* (256x128 NV12). Logs [RESULT][PASS]/[FAIL]. */
int bk_jpeg_encode_frame_test(void);

/**
 * OSD overlay test covering frame, software Flexa and software frame paths.
 * Each mode runs with the same 8-slot overlay (ARGB/NV12/Bitmap) and logs its
 * own [RESULT][PASS]/[FAIL] line, followed by a summary result.
 */
int bk_jpeg_encode_osd_test(void);

/**
 * Encode exactly one 1920x1080 frame with the 8-slot OSD overlay into a
 * caller-provided buffer using software whole-frame mode (YUYV input).
 * mode_text is drawn in OSD slot 0 (e.g. "SW FRAME"). Returns the encoded JPEG
 * length through out_len.
 */
int bk_jpeg_encode_osd_sw_frame_once(uint8_t *out_buf, uint32_t out_size, uint32_t *out_len,
				     const char *mode_text);

/** VCENC JPEG software-Flexa test using 256x128 NV12 input. Logs [RESULT][PASS]/[FAIL]. */
int bk_jpeg_encode_sw_flexa_test(void);

/**
 * Encode exactly one 1920x1080 frame with the 8-slot OSD overlay into a
 * caller-provided buffer (frame mode, 512KB recommended). mode_text is drawn in
 * OSD slot 0 (e.g. "HW FRAME"). Returns the encoded JPEG length through out_len.
 * Used by the decode/display pipeline.
 */
int bk_jpeg_encode_osd_frame_once(uint8_t *out_buf, uint32_t out_size, uint32_t *out_len,
				  const char *mode_text);

/**
 * Encode exactly one 1920x1080 frame with the 8-slot OSD overlay into a
 * caller-provided buffer using software Flexa mode. The encoder allocates its own
 * output frame and the JPEG is copied into out_buf on completion. mode_text is
 * drawn in OSD slot 0 (e.g. "SW FLEXA").
 */
int bk_jpeg_encode_osd_flexa_once(uint8_t *out_buf, uint32_t out_size, uint32_t *out_len,
				  const char *mode_text);

/**
 * Release the shared 1920x1080 input frame used by the *_once() encoders. The
 * display test keeps decode/GPU buffers in the UNCODED heap, so the input is
 * allocated once and reused across rounds instead of per round; call this when
 * the display session is over.
 */
void bk_jpeg_encode_osd_input_release(void);

/**
 * Software OSD timing benchmark (software whole-frame mode).
 * Encodes a YUYV frame of width x height, measuring the average per-frame time
 * without OSD and with 1/16, 1/4, 9/16 and full-frame overlays in both
 * ARGB8888 and BITMAP format (sizes rounded down to 8 pixels), then logs the
 * overhead versus the no-OSD baseline followed by a [RESULT][PASS]/[FAIL] line.
 * Both dimensions must be multiples of 8.
 */
int bk_jpeg_encode_osd_bench_test(uint32_t width, uint32_t height);

/** Optional boot demo: delayed run of frame and software-Flexa JPEG tests. */
void vcenc_jpeg_run_boot_demo(void);

#ifdef __cplusplus
}
#endif
