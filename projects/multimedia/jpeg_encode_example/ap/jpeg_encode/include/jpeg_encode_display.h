#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	JPEG_ENCODE_OSD_DISPLAY_FRAME = 0,    /* hardware frame mode */
	JPEG_ENCODE_OSD_DISPLAY_FLEXA,        /* software flexa mode */
	JPEG_ENCODE_OSD_DISPLAY_SW_FRAME,     /* software whole-frame mode */
	JPEG_ENCODE_OSD_DISPLAY_ALL,          /* run the three modes one by one */
} jpeg_encode_osd_display_mode_t;

/**
 * OSD encode -> decode -> LCD display test.
 *
 * Each round encodes one 1920x1080 frame with the 8-slot OSD overlay (slot 0
 * shows the active mode, e.g. "HW FRAME" / "SW FLEXA" / "SW FRAME"), feeds the
 * JPEG bitstream into the JPEG flexa decoder, and pushes the decoded NV12 frame
 * through the GPU (NV12 -> rotate 90 -> ARGB8888 compressed) to the MIPI LCD.
 * JPEG_ENCODE_OSD_DISPLAY_ALL runs frame, flexa and sw_frame one by one.
 * Logs [RESULT][PASS]/[FAIL].
 */
int bk_jpeg_encode_osd_display_test(jpeg_encode_osd_display_mode_t mode);

#ifdef __cplusplus
}
#endif
