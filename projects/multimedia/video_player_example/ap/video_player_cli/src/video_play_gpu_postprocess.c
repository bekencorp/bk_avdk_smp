#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <common/bk_include.h>
#include <os/mem.h>
#include <os/os.h>

#include <components/bk_frame_buffer.h>
#include <components/bk_hardware_ram.h>
#include <components/media_types.h>
#include <driver/hpdma.h>
#include "modules/vg_lite_gpu/vg_lite.h"
#include "soc/reg_base.h"

#include "video_play_gpu_postprocess.h"

#define TAG "video_play_gpu_pp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define VIDEO_PLAY_GPU_POST_ALIGN_BYTES      64U
#define VIDEO_PLAY_GPU_POST_PAD_BYTES        128U
#define VIDEO_PLAY_GPU_POST_FLEXA_LINES      16U
#define VIDEO_PLAY_GPU_POST_HPDMA_TIMEOUT_MS 3000U
#define VIDEO_PLAY_GPU_POST_PANEL_WIDTH      1080U
#define VIDEO_PLAY_GPU_POST_PANEL_HEIGHT     1920U

void bk_gpu_driver_init(void);
void bk_gpu_driver_deinit(void);

typedef struct
{
    uintptr_t pingpong_raw;
    uintptr_t buffers[2];
    uint32_t strip_bytes;
    uint8_t dst_idx;
    hpdma_id_t gdma;
    void *link_table;
    beken_semaphore_t transfer_sem;
} video_play_gpu_post_dma_t;

static bool s_gpu_post_initialized = false;
static void *s_gpu_post_contiguous_buffer = NULL;
static beken_mutex_t s_gpu_post_mutex = NULL;
static video_play_gpu_post_dma_t s_gpu_post_dma = {
    .gdma = HPDMA_ID_MAX,
};

static inline uint32_t video_play_gpu_post_align_up(uint32_t value, uint32_t align)
{
    return (value + align - 1U) & ~(align - 1U);
}

static inline uintptr_t video_play_gpu_post_align_ptr(uintptr_t value, uintptr_t align)
{
    return (value + align - 1U) & ~(align - 1U);
}

static uint32_t video_play_gpu_post_compressed_argb_size(uint32_t width, uint32_t height)
{
    /* DEC400 HV-sampled ARGB8888 stores width / 4 physical pixels per row. */
    return bk_pixel_size_get(BK_PIXEL_FORMAT_ARGB8888) * (width / 4U) * height;
}

static avdk_err_t video_play_gpu_post_lock(void)
{
    if (s_gpu_post_mutex == NULL)
    {
        if (rtos_init_mutex(&s_gpu_post_mutex) != BK_OK)
        {
            LOGE("%s: init mutex failed\n", __func__);
            return AVDK_ERR_GENERIC;
        }
    }

    rtos_lock_mutex(&s_gpu_post_mutex);
    return AVDK_ERR_OK;
}

static void video_play_gpu_post_unlock(void)
{
    if (s_gpu_post_mutex != NULL)
    {
        rtos_unlock_mutex(&s_gpu_post_mutex);
    }
}

static void video_play_gpu_post_dma_finish_cb(hpdma_id_t hpdma_id, void *user_data)
{
    (void)hpdma_id;

    if (user_data != NULL)
    {
        rtos_set_semaphore((beken_semaphore_t *)user_data);
    }
}

static void video_play_gpu_post_dma_wait_idle(void)
{
    if (s_gpu_post_dma.gdma >= HPDMA_ID_MAX)
    {
        return;
    }

    for (uint32_t wait_ms = 0; wait_ms < VIDEO_PLAY_GPU_POST_HPDMA_TIMEOUT_MS; wait_ms++)
    {
        if (bk_hpdma_get_next_ll_addr(s_gpu_post_dma.gdma) == 0U &&
            bk_hpdma_get_enable_status(s_gpu_post_dma.gdma) == 0U)
        {
            return;
        }
        rtos_delay_milliseconds(1);
    }

    LOGE("%s: ch%d still busy before deinit\n", __func__, (int)s_gpu_post_dma.gdma);
}

static void video_play_gpu_post_dma_deinit(void)
{
    if (s_gpu_post_dma.gdma < HPDMA_ID_MAX)
    {
        video_play_gpu_post_dma_wait_idle();
        (void)bk_hpdma_disable_finish_interrupt(s_gpu_post_dma.gdma);
        (void)bk_hpdma_register_isr(s_gpu_post_dma.gdma, NULL, NULL, NULL, NULL);
        bk_err_t free_ret = bk_hpdma_free(HPDMA_DEV_DTCM, s_gpu_post_dma.gdma);
        if (free_ret == BK_ERR_HPDMA_TIMEOUT)
        {
            LOGE("%s: ch%d free timeout, force reclaim on playback stop\n",
                 __func__, (int)s_gpu_post_dma.gdma);
            (void)bk_hpdma_force_reclaim(HPDMA_DEV_DTCM, s_gpu_post_dma.gdma);
        }
        else if (free_ret != BK_OK)
        {
            LOGE("%s: ch%d free failed, ret=%d\n",
                 __func__, (int)s_gpu_post_dma.gdma, (int)free_ret);
        }
        s_gpu_post_dma.gdma = HPDMA_ID_MAX;
    }

    if (s_gpu_post_dma.link_table != NULL)
    {
        bk_hpdma_link_deinit(s_gpu_post_dma.link_table);
        s_gpu_post_dma.link_table = NULL;
    }

    if (s_gpu_post_dma.transfer_sem != NULL)
    {
        (void)rtos_deinit_semaphore(&s_gpu_post_dma.transfer_sem);
        s_gpu_post_dma.transfer_sem = NULL;
    }

    if (s_gpu_post_dma.pingpong_raw != 0U)
    {
        hsram_free((void *)s_gpu_post_dma.pingpong_raw);
        s_gpu_post_dma.pingpong_raw = 0U;
    }

    s_gpu_post_dma.buffers[0] = 0U;
    s_gpu_post_dma.buffers[1] = 0U;
    s_gpu_post_dma.strip_bytes = 0U;
    s_gpu_post_dma.dst_idx = 0U;
}

static avdk_err_t video_play_gpu_post_dma_init(uint32_t strip_bytes)
{
    const uint32_t pingpong_size = (strip_bytes * 2U) + VIDEO_PLAY_GPU_POST_ALIGN_BYTES;
    uintptr_t base = 0U;
    bk_err_t bk_ret;

    s_gpu_post_dma.gdma = HPDMA_ID_MAX;
    s_gpu_post_dma.link_table = NULL;
    s_gpu_post_dma.transfer_sem = NULL;
    s_gpu_post_dma.pingpong_raw = 0U;
    s_gpu_post_dma.strip_bytes = strip_bytes;
    s_gpu_post_dma.dst_idx = 0U;

    base = (uintptr_t)bk_get_gpu_output_buffer(pingpong_size);
    if (base == 0U)
    {
        LOGE("%s: alloc strip ping-pong failed, size=%u\n",
             __func__, (unsigned)pingpong_size);
        return AVDK_ERR_NOMEM;
    }

    s_gpu_post_dma.pingpong_raw = base;
    os_memset((void *)base, 0, pingpong_size);
    s_gpu_post_dma.buffers[0] = video_play_gpu_post_align_ptr(base,
                                                              VIDEO_PLAY_GPU_POST_ALIGN_BYTES);
    s_gpu_post_dma.buffers[1] = video_play_gpu_post_align_ptr(base + strip_bytes,
                                                              VIDEO_PLAY_GPU_POST_ALIGN_BYTES);

    s_gpu_post_dma.link_table = bk_hpdma_link_init(1);
    if (s_gpu_post_dma.link_table == NULL)
    {
        LOGE("%s: bk_hpdma_link_init failed\n", __func__);
        goto fail;
    }

    s_gpu_post_dma.gdma = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (s_gpu_post_dma.gdma >= HPDMA_ID_MAX)
    {
        LOGE("%s: bk_hpdma_alloc failed\n", __func__);
        goto fail;
    }

    (void)bk_hpdma_set_dest_burst_len(s_gpu_post_dma.gdma, HPDMA_BURST_LEN_INC16);
    (void)bk_hpdma_set_src_burst_len(s_gpu_post_dma.gdma, HPDMA_BURST_LEN_INC16);

    bk_ret = rtos_init_semaphore(&s_gpu_post_dma.transfer_sem, 1);
    if (bk_ret != BK_OK)
    {
        LOGE("%s: init transfer_sem failed, ret=%d\n", __func__, (int)bk_ret);
        goto fail;
    }

    (void)bk_hpdma_register_isr(s_gpu_post_dma.gdma,
                                NULL,
                                NULL,
                                video_play_gpu_post_dma_finish_cb,
                                &s_gpu_post_dma.transfer_sem);
    (void)bk_hpdma_enable_finish_interrupt(s_gpu_post_dma.gdma);

    LOGI("%s: strip ping-pong ready, strip=%u buf0=0x%x buf1=0x%x gdma=%d\n",
         __func__, (unsigned)strip_bytes,
         (unsigned)s_gpu_post_dma.buffers[0],
         (unsigned)s_gpu_post_dma.buffers[1],
         (int)s_gpu_post_dma.gdma);
    return AVDK_ERR_OK;

fail:
    video_play_gpu_post_dma_deinit();
    return AVDK_ERR_GENERIC;
}

static avdk_err_t video_play_gpu_post_dma_wait(void)
{
    if (s_gpu_post_dma.transfer_sem == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    if (rtos_get_semaphore(&s_gpu_post_dma.transfer_sem,
                           VIDEO_PLAY_GPU_POST_HPDMA_TIMEOUT_MS) != BK_OK)
    {
        LOGE("%s: strip dma wait timeout\n", __func__);
        return AVDK_ERR_TIMEOUT;
    }
    return AVDK_ERR_OK;
}

static void video_play_gpu_post_dma_normalize_sem(void)
{
    if (s_gpu_post_dma.transfer_sem == NULL)
    {
        return;
    }

    while (rtos_get_semaphore(&s_gpu_post_dma.transfer_sem, BEKEN_NO_WAIT) == BK_OK)
    {
    }
}

static avdk_err_t video_play_gpu_post_dma_transfer(uint32_t src_addr,
                                                   void *frame,
                                                   uint32_t offset,
                                                   uint32_t xsize,
                                                   uint32_t ysize,
                                                   uint32_t dst_step)
{
    hpdma_link_config_t cfg;
    os_memset(&cfg, 0, sizeof(cfg));

    cfg.src_addr = src_addr;
    cfg.dst_addr = (uint32_t)((uintptr_t)frame + offset);
    cfg.src_xsize = (uint16_t)xsize;
    cfg.src_ysize = (uint16_t)ysize;
    cfg.dst_xsize = (uint16_t)xsize;
    cfg.dst_ysize = (uint16_t)ysize;
    cfg.src_step = 0;
    cfg.dst_step = (uint16_t)dst_step;
    cfg.finish_int_en = 1;
    cfg.half_finish_int_en = 0;

    video_play_gpu_post_dma_normalize_sem();
    if (bk_hpdma_link_set_descs(s_gpu_post_dma.link_table, &cfg, 1) != BK_OK ||
        bk_hpdma_link_transfer(s_gpu_post_dma.gdma,
                               (void *)SOC_SRAM_PERI_ADDR((uintptr_t)s_gpu_post_dma.link_table)) != BK_OK)
    {
        LOGE("%s: start strip dma failed\n", __func__);
        return AVDK_ERR_GENERIC;
    }

    return video_play_gpu_post_dma_wait();
}

static void video_play_gpu_postprocess_deinit_locked(void)
{
    if (!s_gpu_post_initialized)
    {
        return;
    }

    video_play_gpu_post_dma_deinit();
    (void)vg_lite_close();
    bk_gpu_driver_deinit();

    if (s_gpu_post_contiguous_buffer != NULL)
    {
        hsram_free(s_gpu_post_contiguous_buffer);
        s_gpu_post_contiguous_buffer = NULL;
    }

    s_gpu_post_initialized = false;
}

static avdk_err_t video_play_gpu_post_ensure_init(uint32_t strip_bytes)
{
    if (s_gpu_post_initialized)
    {
        if (s_gpu_post_dma.strip_bytes >= strip_bytes)
        {
            return AVDK_ERR_OK;
        }
        video_play_gpu_postprocess_deinit_locked();
    }

    if (strip_bytes == 0U)
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

    avdk_err_t ret = video_play_gpu_post_dma_init(strip_bytes);
    if (ret != AVDK_ERR_OK)
    {
        (void)vg_lite_close();
        hsram_free(s_gpu_post_contiguous_buffer);
        s_gpu_post_contiguous_buffer = NULL;
        bk_gpu_driver_deinit();
        return ret;
    }

    s_gpu_post_initialized = true;
    return AVDK_ERR_OK;
}

void video_play_gpu_postprocess_deinit(void)
{
    if (s_gpu_post_mutex == NULL)
    {
        return;
    }

    if (video_play_gpu_post_lock() != AVDK_ERR_OK)
    {
        return;
    }

    video_play_gpu_postprocess_deinit_locked();
    video_play_gpu_post_unlock();
}

static void video_play_gpu_post_set_strip_matrix(vg_lite_matrix_t *matrix,
                                                 video_play_rotate_mode_t rotate,
                                                 uint32_t output_w,
                                                 float scale_x,
                                                 float scale_y,
                                                 uint32_t strip_index)
{
    vg_lite_identity(matrix);

    if (rotate == VIDEO_PLAY_ROTATE_90)
    {
        const float strip_offset = (float)((strip_index - 1U) * VIDEO_PLAY_GPU_POST_FLEXA_LINES);

        vg_lite_rotate(90.0f, matrix);
        vg_lite_scale(scale_x, -scale_y, matrix);
        matrix->m[0][2] = (vg_lite_float_t)(-strip_offset);
        matrix->m[1][2] = 0.0f;
    }
    else if (rotate == VIDEO_PLAY_ROTATE_270)
    {
        vg_lite_rotate(270.0f, matrix);
        vg_lite_scale(scale_x, -scale_y, matrix);
        matrix->m[0][2] = (vg_lite_float_t)(strip_index * VIDEO_PLAY_GPU_POST_FLEXA_LINES);
        matrix->m[1][2] = (vg_lite_float_t)output_w;
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

    uint32_t visible_w = VIDEO_PLAY_GPU_POST_PANEL_WIDTH;
    uint32_t visible_h = VIDEO_PLAY_GPU_POST_PANEL_HEIGHT;
    uint32_t render_w = VIDEO_PLAY_GPU_POST_PANEL_WIDTH;
    uint32_t render_h = VIDEO_PLAY_GPU_POST_PANEL_HEIGHT;
    if (rotate == VIDEO_PLAY_ROTATE_90 || rotate == VIDEO_PLAY_ROTATE_270)
    {
        render_w = VIDEO_PLAY_GPU_POST_PANEL_HEIGHT;
        render_h = VIDEO_PLAY_GPU_POST_PANEL_WIDTH;
    }
    render_w = video_play_gpu_post_align_up(render_w, 16U);
    render_h = video_play_gpu_post_align_up(render_h,
                                                           VIDEO_PLAY_GPU_POST_FLEXA_LINES);
    const uint32_t strip_bytes = render_w * VIDEO_PLAY_GPU_POST_FLEXA_LINES;
    const uint32_t frame_size = video_play_gpu_post_compressed_argb_size(render_w, render_h);
    const uint32_t alloc_size = video_play_gpu_post_align_up(frame_size + VIDEO_PLAY_GPU_POST_PAD_BYTES,
                                                             VIDEO_PLAY_GPU_POST_ALIGN_BYTES);

    avdk_err_t ret = video_play_gpu_post_lock();
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    ret = video_play_gpu_post_ensure_init(strip_bytes);
    if (ret != AVDK_ERR_OK)
    {
        video_play_gpu_post_unlock();
        return ret;
    }

    void *gpu_frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, alloc_size);
    if (gpu_frame == NULL)
    {
        LOGE("%s: alloc GPU output failed, size=%u alloc=%u\n",
             __func__, (unsigned)frame_size, (unsigned)alloc_size);
        video_play_gpu_post_unlock();
        return AVDK_ERR_NOMEM;
    }
    os_memset(gpu_frame, 0, alloc_size);

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
        video_play_gpu_post_unlock();
        return AVDK_ERR_GENERIC;
    }

    dst_buf.width = (vg_lite_uint32_t)VIDEO_PLAY_GPU_POST_FLEXA_LINES;
    dst_buf.height = (vg_lite_uint32_t)render_w;
    dst_buf.format = VG_LITE_BGRA8888;
    dst_buf.compress_mode = VG_LITE_DEC_HV_SAMPLE;
    dst_buf.tiled = VG_LITE_TILED;
    vg_ret = vg_lite_allocate_with_data(&dst_buf,
                                        (void *)s_gpu_post_dma.buffers[s_gpu_post_dma.dst_idx],
                                        NULL,
                                        NULL,
                                        NULL);
    if (vg_ret != VG_LITE_SUCCESS)
    {
        LOGE("%s: wrap GPU dst failed, ret=%d\n", __func__, (int)vg_ret);
        (void)vg_lite_free_without_free_data(&src_buf);
        bk_frame_buffer_free(gpu_frame);
        video_play_gpu_post_unlock();
        return AVDK_ERR_GENERIC;
    }

    const uint32_t start_ms = rtos_get_time();
    const uint32_t strip_count = render_h / VIDEO_PLAY_GPU_POST_FLEXA_LINES;
    const uint32_t dma_xsize = VIDEO_PLAY_GPU_POST_FLEXA_LINES *
                               bk_pixel_size_get(BK_PIXEL_FORMAT_ARGB8888);
    const uint32_t dma_ysize = render_w / 4U;
    const uint32_t dma_dst_step = (render_h - VIDEO_PLAY_GPU_POST_FLEXA_LINES) *
                                  bk_pixel_size_get(BK_PIXEL_FORMAT_ARGB8888);
    const float scale_x = (float)render_w / (float)width;
    const float scale_y = (float)render_h / (float)height;

    for (uint32_t strip_index = 1U; strip_index <= strip_count; strip_index++)
    {
        const uint32_t dma_offset = (rotate == VIDEO_PLAY_ROTATE_90)
                                    ? ((strip_index - 1U) * dma_xsize)
                                    : ((render_h - (strip_index * VIDEO_PLAY_GPU_POST_FLEXA_LINES)) *
                                       bk_pixel_size_get(BK_PIXEL_FORMAT_ARGB8888));

        dst_buf.memory = (vg_lite_pointer)(uintptr_t)s_gpu_post_dma.buffers[s_gpu_post_dma.dst_idx];
        dst_buf.address = SOC_SRAM_PERI_ADDR(s_gpu_post_dma.buffers[s_gpu_post_dma.dst_idx]);
        video_play_gpu_post_set_strip_matrix(&matrix, rotate, render_w, scale_x, scale_y, strip_index);

        vg_ret = vg_lite_blit(&dst_buf, &src_buf, &matrix, VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
        if (vg_ret == VG_LITE_SUCCESS)
        {
            vg_ret = vg_lite_finish();
        }
        if (vg_ret != VG_LITE_SUCCESS)
        {
            break;
        }

        ret = video_play_gpu_post_dma_transfer((uint32_t)s_gpu_post_dma.buffers[s_gpu_post_dma.dst_idx],
                                               gpu_frame,
                                               dma_offset,
                                               dma_xsize,
                                               dma_ysize,
                                               dma_dst_step);
        if (ret != AVDK_ERR_OK)
        {
            break;
        }

        s_gpu_post_dma.dst_idx = 1U - s_gpu_post_dma.dst_idx;
    }
    const uint32_t wall_ms = rtos_get_time() - start_ms;

    (void)vg_lite_free_without_free_data(&dst_buf);
    (void)vg_lite_free_without_free_data(&src_buf);

    if (vg_ret != VG_LITE_SUCCESS || ret != AVDK_ERR_OK)
    {
        LOGE("%s: strip rotate failed, vg_ret=%d, ret=%d, wall_ms=%u\n",
             __func__, (int)vg_ret, ret, (unsigned)wall_ms);
        bk_frame_buffer_free(gpu_frame);
        video_play_gpu_post_unlock();
        return (ret != AVDK_ERR_OK) ? ret : AVDK_ERR_GENERIC;
    }

    out_frame->data = gpu_frame;
    out_frame->size = frame_size;
    out_frame->visible_width = (uint16_t)visible_w;
    out_frame->visible_height = (uint16_t)visible_h;
    out_frame->render_width = (uint16_t)render_w;
    out_frame->render_height = (uint16_t)render_h;
    video_play_gpu_post_unlock();
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
