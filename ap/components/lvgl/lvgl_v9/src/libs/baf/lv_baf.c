/**
 * @file lv_baf.c
 */

#include "lv_baf_private.h"

#if LV_USE_BAF

#include "../../core/lv_obj_class_private.h"
#include "../../misc/cache/lv_cache.h"
#include "../../misc/lv_timer_private.h"
#include "../../misc/lv_event.h"
#include <bk_baf.h>                             /* bk_baf decode + compose API */
#include <components/bk_frame_buffer.h>         /* bk_frame_buffer_malloc / _free */
#include "lv_vendor.h"                          /* lv_vnd_data_t (GPU-init backend select) */

#define MY_CLASS (&lv_baf_class)
#define BAF_TIMER_PERIOD_MS 2U

static void lv_baf_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_baf_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void next_frame_task_cb(lv_timer_t * timer);

const lv_obj_class_t lv_baf_class = {
    .constructor_cb = lv_baf_constructor,
    .destructor_cb = lv_baf_destructor,
    .instance_size = sizeof(lv_baf_t),
    .base_class = &lv_image_class,
    .name = "lv_baf",
};

static void close_decoder(lv_obj_t * obj)
{
    lv_baf_t * baf = (lv_baf_t *)obj;

    lv_timer_pause(baf->timer);
    if(baf->decoder == NULL) return;

    const void * src = lv_image_get_src(obj);
    if(src != NULL) lv_image_cache_drop(src);
    bk_baf_close(baf->decoder);
    baf->decoder = NULL;
    baf->imgdsc.data = NULL;

    if(baf->frame_buf != NULL) {
        bk_frame_buffer_free(baf->frame_buf);
        baf->frame_buf = NULL;
    }
}

static bool prepare_image(lv_obj_t * obj)
{
    lv_baf_t * baf = (lv_baf_t *)obj;
    uint16_t width = bk_baf_get_width(baf->decoder);
    uint16_t height = bk_baf_get_height(baf->decoder);
    bk_baf_frame_desc_t canvas_desc;
    bk_baf_get_frame_desc(baf->decoder, &canvas_desc, NULL);

    if(width == 0U || height == 0U || canvas_desc.data == NULL) {
        return false;
    }

    /* Own ARGB8888 frame that LVGL draws (fully managed via lv_image_set_src).
     * Non-cacheable GPU heap so the GPU compose is coherent for the CPU blend. */
    size_t frame_size = (size_t)width * height * 4U;
    baf->frame_buf = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, (uint32_t)frame_size);
    if(baf->frame_buf == NULL) {
        LV_LOG_WARN("BAF: cannot allocate ARGB frame buffer");
        return false;
    }

    baf->imgdsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    baf->imgdsc.header.flags = LV_IMAGE_FLAGS_MODIFIABLE;
    baf->imgdsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
    baf->imgdsc.header.w = width;
    baf->imgdsc.header.h = height;
    baf->imgdsc.header.stride = (uint32_t)width * 4U;
    baf->imgdsc.data_size = frame_size;
    baf->imgdsc.data = baf->frame_buf;

    /* Strip the themed background/border/radius/padding the base image class
     * would otherwise draw around the frame. */
    lv_obj_set_size(obj, width, height);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);

    lv_image_set_src(obj, &baf->imgdsc);
    return true;
}

lv_obj_t * lv_baf_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_baf_set_src(lv_obj_t * obj, const bk_baf_source_t * src)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_baf_t * baf = (lv_baf_t *)obj;

    close_decoder(obj);
    if(src == NULL) return;

    /* Hardware setup: use the GPU compositor only if LVGL actually brought up the
     * GPU (read the vendor data off the default display); otherwise CPU (Helium).
     * LVGL owns the GPU, so init_gpu stays false -- bk_baf must not create/destroy it.
     * bk_baf_init() is idempotent (no GPU work here), so calling it per set_src is fine. */
    lv_display_t * disp = lv_display_get_default();
    lv_vnd_data_t * vnd = (disp != NULL) ? (lv_vnd_data_t *)lv_display_get_user_data(disp) : NULL;
    bk_baf_hw_config_t hw = {
        .backend = (vnd != NULL && vnd->gpu_inited) ? BK_BAF_RENDER_GPU
                                                    : BK_BAF_RENDER_CPU,
    };
    (void)bk_baf_init(&hw);

    bk_baf_config_t cfg = { .source = src };
    baf->decoder = bk_baf_open(&cfg);
    if(baf->decoder == NULL) {
        LV_LOG_WARN("Couldn't load the BAF source");
        return;
    }
    if(!prepare_image(obj)) {
        LV_LOG_WARN("Invalid BAF output canvas");
        close_decoder(obj);
        return;
    }

    lv_timer_resume(baf->timer);
    lv_timer_reset(baf->timer);
    next_frame_task_cb(baf->timer);
}

void lv_baf_restart(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_baf_t * baf = (lv_baf_t *)obj;
    if(baf->decoder == NULL) return;

    bk_baf_ioctl(baf->decoder, BK_BAF_IOCTL_REWIND, NULL);
    bk_baf_ioctl(baf->decoder, BK_BAF_IOCTL_RESUME, NULL);
    lv_timer_resume(baf->timer);
    lv_timer_reset(baf->timer);
}

void lv_baf_pause(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_baf_t * baf = (lv_baf_t *)obj;
    bk_baf_ioctl(baf->decoder, BK_BAF_IOCTL_PAUSE, NULL);
    lv_timer_pause(baf->timer);
}

void lv_baf_resume(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_baf_t * baf = (lv_baf_t *)obj;
    if(baf->decoder != NULL) {
        bk_baf_ioctl(baf->decoder, BK_BAF_IOCTL_RESUME, NULL);
        lv_timer_resume(baf->timer);
    }
}

bool lv_baf_is_loaded(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_baf_t * baf = (lv_baf_t *)obj;
    return baf->decoder != NULL;
}

int32_t lv_baf_get_loop_count(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_baf_t * baf = (lv_baf_t *)obj;
    return bk_baf_get_loop_count(baf->decoder);
}

void lv_baf_set_loop_count(lv_obj_t * obj, int32_t count)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_baf_t * baf = (lv_baf_t *)obj;
    bk_baf_set_loop_count(baf->decoder, count);
}

static void lv_baf_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_baf_t * baf = (lv_baf_t *)obj;

    baf->decoder = NULL;
    baf->frame_buf = NULL;
    baf->timer = lv_timer_create(next_frame_task_cb, BAF_TIMER_PERIOD_MS, obj);
    lv_timer_pause(baf->timer);
}

static void lv_baf_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_baf_t * baf = (lv_baf_t *)obj;

    close_decoder(obj);
    lv_timer_delete(baf->timer);
}

static void next_frame_task_cb(lv_timer_t * timer)
{
    lv_obj_t * obj = timer->user_data;
    lv_baf_t * baf = (lv_baf_t *)obj;
    if(baf->decoder == NULL) return;

    /* poll() paces playback internally: it returns WAIT until the next frame is
     * due, so this 2ms timer just polls and shows whatever FRAME comes back. */
    bk_baf_decoder_result_t result = bk_baf_poll(baf->decoder);
    if(result == BK_BAF_DECODER_RESULT_WAIT) {
        return;
    }
    else if(result == BK_BAF_DECODER_RESULT_END) {
        lv_result_t event_result = lv_obj_send_event(obj, LV_EVENT_READY, NULL);
        lv_timer_pause(timer);
        if(event_result != LV_RESULT_OK) return;
    }
    else if(bk_baf_result_is_error(result)) {
        LV_LOG_WARN("BAF frame decode failed");
        lv_obj_send_event(obj, LV_EVENT_CANCEL, (void *)(lv_intptr_t)result);
        lv_timer_pause(timer);
        return;
    }
    else {
        /* Composite the decoded XRGB canvas + A8 alpha into our own ARGB8888
         * frame over a transparent background (straight alpha), then let LVGL
         * draw it. The heavy RGB+alpha merge runs on the selected backend
         * (GPU/CPU); the GPU path serialises internally, so no explicit lock is
         * needed here. */
        bk_baf_frame_desc_t canvas_desc, alpha_desc;
        bk_baf_get_frame_desc(baf->decoder, &canvas_desc, &alpha_desc);
        if(canvas_desc.data != NULL && baf->frame_buf != NULL) {
            uint16_t cw = baf->imgdsc.header.w;
            uint16_t ch = baf->imgdsc.header.h;
            bk_baf_frame_desc_t dst_desc = {
                .data = baf->frame_buf, .format = BK_BAF_PIXEL_ARGB8888,
                .width = cw, .height = ch, .stride = (uint32_t)cw * 4U,
            };
            bk_baf_compose(&dst_desc, &canvas_desc,
                           alpha_desc.data ? &alpha_desc : NULL, 0x00000000U, false);

            lv_image_cache_drop(lv_image_get_src(obj));
            lv_obj_invalidate(obj);
        }
        lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
    }
}

#endif /* LV_USE_BAF */
