#include <common/bk_include.h>
#include <os/mem.h>
#include <os/os.h>

#include <components/bk_frame_buffer.h>
#include <components/bk_hardware_ram.h>
#include <components/media_types.h>
#include "modules/vg_lite_gpu/vg_lite.h"

#include "video_play_gpu_postprocess.h"

#define TAG "video_play_gpu_pp"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define VIDEO_PLAY_GPU_POST_ALIGN_BYTES      64U
#define VIDEO_PLAY_GPU_POST_PAD_BYTES        128U

void bk_gpu_driver_init(void);
void bk_gpu_driver_deinit(void);

static bool s_gpu_post_initialized = false;
static void *s_gpu_post_contiguous_buffer = NULL;

static inline uint32_t video_play_gpu_post_align_up(uint32_t value, uint32_t align)
{
    return (value + align - 1U) & ~(align - 1U);
}

static uint32_t video_play_gpu_post_compressed_argb_size(uint32_t width, uint32_t height)
{
    /* DEC400 HV-sampled ARGB8888 stores width / 4 physical pixels per row. */
    return bk_pixel_size_get(BK_PIXEL_FORMAT_ARGB8888) * (width / 4U) * height;
}

static avdk_err_t video_play_gpu_post_ensure_init(void)
{
    if (s_gpu_post_initialized)
    {
        return AVDK_ERR_OK;
    }

    bk_gpu_driver_init();

    s_gpu_post_contiguous_buffer = bk_get_gpu_flexa_buffer(CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ);
    if (s_gpu_post_contiguous_buffer == NULL)
    {
        LOGE("%s: alloc VG-Lite contiguous buffer failed, size=%u\n",
             __func__, (unsigned)CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ);
        bk_gpu_driver_deinit();
        return AVDK_ERR_NOMEM;
    }

    vg_lite_error_t vg_ret = vg_lite_set_buffer((uint8_t *)s_gpu_post_contiguous_buffer);
    if (vg_ret == VG_LITE_SUCCESS)
    {
        vg_ret = vg_lite_init(0, 0);
    }
    if (vg_ret != VG_LITE_SUCCESS)
    {
        LOGE("%s: VG-Lite init failed, ret=%d\n", __func__, (int)vg_ret);
        hsram_free(s_gpu_post_contiguous_buffer);
        s_gpu_post_contiguous_buffer = NULL;
        bk_gpu_driver_deinit();
        return AVDK_ERR_GENERIC;
    }

    s_gpu_post_initialized = true;
    return AVDK_ERR_OK;
}

void video_play_gpu_postprocess_deinit(void)
{
    if (!s_gpu_post_initialized)
    {
        return;
    }

    (void)vg_lite_close();
    bk_gpu_driver_deinit();

    if (s_gpu_post_contiguous_buffer != NULL)
    {
        hsram_free(s_gpu_post_contiguous_buffer);
        s_gpu_post_contiguous_buffer = NULL;
    }

    s_gpu_post_initialized = false;
}

static void video_play_gpu_post_set_rotate_matrix(vg_lite_matrix_t *matrix,
                                                  video_play_rotate_mode_t rotate,
                                                  uint32_t src_w,
                                                  uint32_t src_h)
{
    vg_lite_identity(matrix);

    if (rotate == VIDEO_PLAY_ROTATE_90)
    {
        vg_lite_rotate(90.0f, matrix);
        matrix->m[0][2] = (vg_lite_float_t)src_h;
        matrix->m[1][2] = 0.0f;
    }
    else if (rotate == VIDEO_PLAY_ROTATE_270)
    {
        vg_lite_rotate(270.0f, matrix);
        matrix->m[0][2] = 0.0f;
        matrix->m[1][2] = (vg_lite_float_t)src_w;
    }
}

avdk_err_t video_play_gpu_postprocess_nv12_rotate(const uint8_t *nv12,
                                                  uint32_t width,
                                                  uint32_t height,
                                                  uint32_t stride,
                                                  uint32_t y_plane_height,
                                                  video_play_rotate_mode_t rotate,
                                                  video_play_gpu_postprocess_frame_t *out_frame)
{
    if (nv12 == NULL || out_frame == NULL || width == 0U || height == 0U ||
        stride < width || y_plane_height < height || ((width | height | stride | y_plane_height) & 1U) != 0U)
    {
        return AVDK_ERR_INVAL;
    }
    if (rotate != VIDEO_PLAY_ROTATE_90 && rotate != VIDEO_PLAY_ROTATE_270)
    {
        return AVDK_ERR_UNSUPPORTED;
    }

    avdk_err_t ret = video_play_gpu_post_ensure_init();
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    const uint32_t visible_w = height;
    const uint32_t visible_h = width;
    const uint32_t render_w = video_play_gpu_post_align_up(visible_w, 16U);
    const uint32_t render_h = video_play_gpu_post_align_up(visible_h, 4U);
    const uint32_t frame_size = video_play_gpu_post_compressed_argb_size(render_w, render_h);
    const uint32_t alloc_size = video_play_gpu_post_align_up(frame_size + VIDEO_PLAY_GPU_POST_PAD_BYTES,
                                                             VIDEO_PLAY_GPU_POST_ALIGN_BYTES);

    void *gpu_frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, alloc_size);
    if (gpu_frame == NULL)
    {
        LOGE("%s: alloc GPU output failed, size=%u alloc=%u\n",
             __func__, (unsigned)frame_size, (unsigned)alloc_size);
        return AVDK_ERR_NOMEM;
    }

    vg_lite_buffer_t src_buf;
    vg_lite_buffer_t dst_buf;
    vg_lite_matrix_t matrix;
    os_memset(&src_buf, 0, sizeof(src_buf));
    os_memset(&dst_buf, 0, sizeof(dst_buf));
    os_memset(&matrix, 0, sizeof(matrix));

    src_buf.width = (vg_lite_uint32_t)width;
    src_buf.height = (vg_lite_uint32_t)height;
    src_buf.stride = (vg_lite_int32_t)stride;
    src_buf.format = VG_LITE_NV12;
    src_buf.compress_mode = VG_LITE_DEC_DISABLE;
    src_buf.tiled = VG_LITE_LINEAR;
    src_buf.yuv.uv_stride = (vg_lite_uint32_t)stride;
    src_buf.yuv.uv_height = (vg_lite_uint32_t)(y_plane_height / 2U);

    vg_lite_error_t vg_ret = vg_lite_allocate_with_data(&src_buf,
                                                        (void *)nv12,
                                                        (void *)(nv12 + (stride * y_plane_height)),
                                                        NULL,
                                                        NULL);
    if (vg_ret != VG_LITE_SUCCESS)
    {
        LOGE("%s: wrap NV12 source failed, ret=%d\n", __func__, (int)vg_ret);
        bk_frame_buffer_free(gpu_frame);
        return AVDK_ERR_GENERIC;
    }

    dst_buf.width = (vg_lite_uint32_t)render_w;
    dst_buf.height = (vg_lite_uint32_t)render_h;
    dst_buf.format = VG_LITE_BGRA8888;
    dst_buf.compress_mode = VG_LITE_DEC_HV_SAMPLE;
    dst_buf.tiled = VG_LITE_TILED;
    vg_ret = vg_lite_allocate_with_data(&dst_buf, gpu_frame, NULL, NULL, NULL);
    if (vg_ret != VG_LITE_SUCCESS)
    {
        LOGE("%s: wrap GPU dst failed, ret=%d\n", __func__, (int)vg_ret);
        (void)vg_lite_free_without_free_data(&src_buf);
        bk_frame_buffer_free(gpu_frame);
        return AVDK_ERR_GENERIC;
    }

    video_play_gpu_post_set_rotate_matrix(&matrix, rotate, width, height);
    const uint32_t start_ms = rtos_get_time();
    vg_ret = vg_lite_blit(&dst_buf, &src_buf, &matrix, VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
    if (vg_ret == VG_LITE_SUCCESS)
    {
        vg_ret = vg_lite_finish();
    }
    const uint32_t wall_ms = rtos_get_time() - start_ms;

    (void)vg_lite_free_without_free_data(&dst_buf);
    (void)vg_lite_free_without_free_data(&src_buf);

    if (vg_ret != VG_LITE_SUCCESS)
    {
        LOGE("%s: VG-Lite blit failed, ret=%d, wall_ms=%u\n",
             __func__, (int)vg_ret, (unsigned)wall_ms);
        bk_frame_buffer_free(gpu_frame);
        return AVDK_ERR_GENERIC;
    }

    out_frame->data = gpu_frame;
    out_frame->size = frame_size;
    out_frame->visible_width = (uint16_t)visible_w;
    out_frame->visible_height = (uint16_t)visible_h;
    out_frame->render_width = (uint16_t)render_w;
    out_frame->render_height = (uint16_t)render_h;
    return AVDK_ERR_OK;
}

avdk_err_t video_play_gpu_postprocess_free_frame(void *frame)
{
    if (frame != NULL)
    {
        bk_frame_buffer_free(frame);
    }
    return AVDK_ERR_OK;
}
