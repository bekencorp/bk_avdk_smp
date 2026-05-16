#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>
#include "box.h"

#include <avdk_check.h>

#include <components/bk_gpu.h>


#define TAG "box"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern bk_gpu_ctlr_handle_t app_gpu_handle_get(void);


/**
 * Map a single point from the source canvas (src_w x src_h) into the rotated
 * canvas. Rotation is interpreted as the clockwise rotation applied to the
 * source image before it is shown on the destination canvas.
 *
 *   rotate = 0   : (x, y)                 canvas size = (src_w,  src_h)
 *   rotate = 90  : (src_h - y, x)         canvas size = (src_h,  src_w)
 *   rotate = 180 : (src_w - x, src_h - y) canvas size = (src_w,  src_h)
 *   rotate = 270 : (y, src_w - x)         canvas size = (src_h,  src_w)
 *
 * Coordinates are float to preserve sub-pixel precision through the
 * rotate -> scale -> clamp pipeline; the final cast to int only happens
 * when emitting the GPU draw command.
 */
static inline void rotate_point(int rot, float src_w, float src_h,
                                float x, float y, float *rx, float *ry)
{
    switch (rot) {
        case 90:  *rx = src_h - y; *ry = x;            break;
        case 180: *rx = src_w - x; *ry = src_h - y;    break;
        case 270: *rx = y;         *ry = src_w - x;    break;
        case 0:
        default:  *rx = x;         *ry = y;            break;
    }
}

/**
 * Build draw-path commands for detection boxes.
 *
 * boxes[].{x,y,w,h} are in the source canvas (src_width x src_height, e.g.
 * 256x256 model coordinates) with (x,y) being the top-left corner and (w,h)
 * the box size. The boxes are first rotated by `rotate` degrees clockwise,
 * then anisotropically scaled onto the destination canvas (dst_width x
 * dst_height).
 *
 * @param boxes        Detection boxes in source coordinates.
 * @param count        Number of valid boxes in `boxes`.
 * @param buffer_count Capacity used to size the on-stack cmd/data buffers.
 *                     Must be >= count, otherwise count will be clamped.
 * @param rotate       Clockwise rotation in degrees, only 0/90/180/270 are
 *                     supported; any other value is treated as 0.
 * @param src_width    Width  of the canvas the box coordinates are in.
 * @param src_height   Height of the canvas the box coordinates are in.
 * @param dst_width    Width  of the canvas the boxes are drawn on.
 * @param dst_height   Height of the canvas the boxes are drawn on.
 */
int box_detection_path_build(Box *boxes, int count, int buffer_count, int rotate,
                             int src_width, int src_height,
                             int dst_width, int dst_height)
{
    if (boxes == NULL || buffer_count <= 0 ||
        src_width <= 0 || src_height <= 0 ||
        dst_width <= 0 || dst_height <= 0) {
        LOGE("box_detection_path_build: bad args\n");
        return -1;
    }

    if (boxes[0].score <= 0.0f) {
        box_detection_path_clear();
        return 0;
    }

    if (count < 0)              count = 0;
    if (count > buffer_count)   count = buffer_count;

    int rot = ((rotate % 360) + 360) % 360;
    if (rot != 0 && rot != 90 && rot != 180 && rot != 270) {
        LOGW("box_detection_path_build: unsupported rotate=%d, fallback to 0\n", rotate);
        rot = 0;
    }

    int rot_w = (rot == 90 || rot == 270) ? src_height : src_width;
    int rot_h = (rot == 90 || rot == 270) ? src_width  : src_height;

    float xscale = (float)dst_width  / (float)rot_w;
    float yscale = (float)dst_height / (float)rot_h;

    uint8_t  cmd_buff[buffer_count * 6 + 1];
    int16_t  dat_buff[buffer_count * 10];
    uint8_t* pcmd = cmd_buff;
    int16_t* pdat = dat_buff;

    const float src_w_f = (float)src_width;
    const float src_h_f = (float)src_height;

    for (int i = 0; i < count; i++) {
        float rx0, ry0, rx1, ry1;
        /* Map the (x,y,w,h) source box to its two opposite corners
         * (top-left and bottom-right) before applying the rotation+scale,
         * because rotation can swap which corner ends up min vs max. */
        rotate_point(rot, src_w_f, src_h_f,
                     boxes[i].x,              boxes[i].y,              &rx0, &ry0);
        rotate_point(rot, src_w_f, src_h_f,
                     boxes[i].x + boxes[i].w, boxes[i].y + boxes[i].h, &rx1, &ry1);

        /* Stay in float through scale + min/max normalization + clamping;
         * only snap to integer pixels at the very last step for the GPU
         * draw command (which takes int16). */
        float fxmin = rx0 * xscale;
        float fymin = ry0 * yscale;
        float fxmax = rx1 * xscale;
        float fymax = ry1 * yscale;

        if (fxmin > fxmax) { float t = fxmin; fxmin = fxmax; fxmax = t; }
        if (fymin > fymax) { float t = fymin; fymin = fymax; fymax = t; }
        if (fxmin < 0.0f)               fxmin = 0.0f;
        if (fymin < 0.0f)               fymin = 0.0f;
        if (fxmax > (float)dst_width)   fxmax = (float)dst_width;
        if (fymax > (float)dst_height)  fymax = (float)dst_height;

        int xmin = (int)(fxmin + 0.5f);
        int ymin = (int)(fymin + 0.5f);
        int xmax = (int)(fxmax + 0.5f);
        int ymax = (int)(fymax + 0.5f);

        LOGI("box_detection_path_build[%d]: rot=%d src=%dx%d dst=%dx%d -> (%d,%d)-(%d,%d)\n",
             i, rot, src_width, src_height, dst_width, dst_height,
             xmin, ymin, xmax, ymax);

        DRAW_RECTANGLE_PATH_BUILD(pcmd, pdat, xmin, ymin, xmax, ymax);
    }

    *pcmd = VLC_OP_END;

    uint32_t cmd_size = (uint32_t)count * 6 + 1;

    bk_gpu_draw_path_set_t path_set = {
        .cmd = cmd_buff,
        .data = dat_buff,
        .size = cmd_size,
        .color = 0x00FF00
    };

    bk_gpu_draw_path_build(app_gpu_handle_get(), &path_set);

    return 0;
}

void box_detection_path_clear(void)
{
    bk_gpu_draw_path_clear(app_gpu_handle_get());
}