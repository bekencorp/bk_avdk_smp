#pragma once

#include <components/bk_encode/bk_jpeg_encode_ctlr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure all JPEG OSD slots with the built-in 8-channel test pattern
 *
 * Clears every slot first, then installs 8 overlays on an 8-slot layout:
 *   row0: ARGB8888 time / NV12 text / Bitmap text / ARGB8888 text
 *   row1: ARGB8888 text x4
 * Every slot is a 240x120 solid color block centered on the 1920x1080 frame,
 * so the overlay stays obvious after JPEG decode + rotate + LCD display.
 * Buffers are allocated by this module and released by the encoder.
 *
 * @param handle    Opened JPEG encoder handle
 * @param mode_text Text drawn in slot 0, e.g. "HW FRAME" / "SW FLEXA" / "SW FRAME";
 *                  NULL keeps the default "00:00:00" label
 * @return AVDK error code
 */
avdk_err_t jpeg_encode_osd_submit_all_channels(bk_jpeg_encode_ctlr_handle_t handle,
                                               const char *mode_text);

/**
 * Install the benchmark overlay used by the software OSD timing test.
 *
 * Clears every slot and then installs a single block at (0,0) with the requested
 * size, so the OSD compositing cost depends only on the covered area.
 * width/height == 0 removes the overlay (all slots cleared).
 *
 * @param handle    Opened JPEG encoder handle
 * @param format    BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888 or
 *                  BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP
 * @param width     Overlay width in pixels (multiple of 8), 0 = no OSD
 * @param height    Overlay height in pixels (BITMAP: multiple of 8), 0 = no OSD
 * @return AVDK error code
 */
avdk_err_t jpeg_encode_osd_submit_bench_osd(bk_jpeg_encode_ctlr_handle_t handle,
                                            uint32_t format,
                                            uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif
