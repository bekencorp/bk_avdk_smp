/**
 * @file bk_baf_adapter.c
 *
 * Thin, LVGL-independent adapter over the closed BAF core. Owns the public
 * bk_baf_* API: assembles the bk_baf_decoder_ops vtable from the closed
 * baf_decoder_* functions, drives a decoder through it, and re-exports the
 * GPU/CPU compositor (the GPU path taking the shared GPU lock). Reaches the
 * closed core only through the codec module's decoder interface
 * (modules/baf_decoder.h).
 */

#include "bk_baf.h"
#include "bk_baf_internal.h"            /* struct bk_baf_decoder_ops / _bk_baf_decoder_t */
#include "bk_baf_container.h"           /* baf_view_t + baf_parse_inplace (cfg.data path) */
#include <modules/baf_decoder.h>

#include <os/mem.h>
#include <os/os.h>                         /* rtos_get_time (pacing clock) */
#include <components/bk_gpu.h>              /* bk_gpu_ioctl */
#include <components/bk_gpu_types.h>        /* BK_GPU_IOCTL_LOCK / _UNLOCK, handle */
#include <components/avdk_utils/avdk_error.h>
#include "gpu_core.h"                       /* bk_gpu_driver_init / _deinit */
#include <modules/vg_lite_gpu/vg_lite.h>    /* vg_lite_init / vg_lite_close */

static bool ops_are_valid(const bk_baf_decoder_ops_t * ops)
{
    return ops != NULL && ops->open != NULL && ops->close != NULL &&
           ops->poll_frame != NULL && ops->get_width != NULL &&
           ops->get_height != NULL && ops->get_frame_duration != NULL &&
           ops->get_canvas != NULL && ops->get_canvas_size != NULL;
}

/* Public backend vtable, assembled from the closed core's baf_decoder_*
 * functions (see modules/baf_decoder.h). Generated assets reference it via
 * &bk_baf_decoder_ops; the type is opaque in the public headers. */
const bk_baf_decoder_ops_t bk_baf_decoder_ops = {
    .open = baf_decoder_open,
    .close = baf_decoder_close,
    .poll_frame = baf_decoder_poll_frame,
    .rewind = baf_decoder_rewind,
    .pause = baf_decoder_pause,
    .resume = baf_decoder_resume,
    .get_width = baf_decoder_get_width,
    .get_height = baf_decoder_get_height,
    .get_frame_duration = baf_decoder_get_duration,
    .get_canvas = baf_decoder_get_canvas,
    .get_canvas_size = baf_decoder_get_canvas_size,
    .get_alpha_mask = baf_decoder_get_alpha_mask,
    .get_loop_count = baf_decoder_get_loop_count,
    .set_loop_count = baf_decoder_set_loop_count,
};

/* --- Render backend + shared GPU serialization (internal; set once by bk_baf_init) ---
 * s_backend picks the compositor (GPU=VG-Lite / CPU=Helium). For the GPU path the
 * product's bk_gpu_ctlr handle (bk_baf_hw_config_t.gpu_handle) is registered here, and
 * bk_baf_compose() takes that controller's gpu_mutex (the one flexa/LVGL draws also
 * use) so BAF's GPU work serialises with every other VG-Lite user; no handle (single
 * GPU user) means it runs unlocked. s_gpu_owned records whether bk_baf itself brought
 * the GPU up (hw->init_gpu) so bk_baf_deinit() only tears down what it owns.
 * Hardware setup is ONE-SHOT (bk_baf_init / bk_baf_deinit); per-source open/close
 * never touch it, so switching sources does not power-cycle the GPU. */
static void * s_baf_gpu_handle = NULL;
static bk_baf_render_backend_t s_backend = BK_BAF_RENDER_GPU;
static bool s_gpu_owned = false;

avdk_err_t bk_baf_init(const bk_baf_hw_config_t * hw)
{
    if(hw == NULL) return AVDK_ERR_INVAL;

    s_backend = hw->backend;
    if(hw->gpu_handle != NULL) s_baf_gpu_handle = hw->gpu_handle;

    /* Bring up + own the GPU only when asked (RAW/standalone). When it is owned
     * elsewhere (LVGL/flexa) init_gpu stays false and we just record backend/handle.
     * Guarded so repeat calls are a no-op. */
    if(hw->init_gpu && !s_gpu_owned) {
        bk_gpu_driver_init();
        if(vg_lite_init(0, 0) != VG_LITE_SUCCESS) return AVDK_ERR_GENERIC;
        s_gpu_owned = true;
    }
    return AVDK_ERR_OK;
}

void bk_baf_deinit(void)
{
    /* Creator destroys: only tear down a GPU that bk_baf_init() itself brought up. */
    if(s_gpu_owned) {
        vg_lite_close();
        bk_gpu_driver_deinit();
        s_gpu_owned = false;
    }
    s_baf_gpu_handle = NULL;
}

bk_baf_decoder_t * bk_baf_open(const bk_baf_config_t * cfg)
{
    if(cfg == NULL) return NULL;

    /* Two ways to supply the asset: a pre-parsed .source, or a raw BAF v1 container
     * in .data (parsed here; the decoder then owns the parsed view). */
    const bk_baf_source_t * source = cfg->source;
    baf_view_t * owned = NULL;
    if(source == NULL) {
        if(cfg->data == NULL) return NULL;
        owned = os_malloc(sizeof(*owned));
        if(owned == NULL) return NULL;
        if(baf_parse_inplace(cfg->data, cfg->data_len, owned) != AVDK_ERR_OK) {
            os_free(owned);
            return NULL;
        }
        source = &owned->source;
    }
    if(source->magic != BK_BAF_SOURCE_MAGIC || !ops_are_valid(source->ops)) {
        os_free(owned);   /* os_free(NULL) is a no-op */
        return NULL;
    }

    /* Per-source only: allocate the decoder and open the backend context. The render
     * backend + GPU must already be up via bk_baf_init(); switching sources is just a
     * close()/open() and never re-inits the GPU. */
    bk_baf_decoder_t * decoder = os_zalloc(sizeof(*decoder));
    if(decoder == NULL) {
        os_free(owned);
        return NULL;
    }
    decoder->context = source->ops->open(source->data);
    if(decoder->context == NULL) {
        os_free(decoder);
        os_free(owned);
        return NULL;
    }
    decoder->ops = source->ops;
    decoder->owned_view = owned;   /* freed in bk_baf_close(); NULL for .source path */
    bk_baf_pacer_reset(&decoder->pacer);

    /* Playback options. */
    bk_baf_set_loop_count(decoder, cfg->loop_count);
    if(cfg->free_run) {
        bool on = true;
        bk_baf_ioctl(decoder, BK_BAF_IOCTL_SET_FREERUN, &on);
    }
    return decoder;
}

void bk_baf_close(bk_baf_decoder_t * decoder)
{
    /* Per-source only: the GPU stays up (torn down by bk_baf_deinit()). */
    if(decoder == NULL) return;
    decoder->ops->close(decoder->context);
    os_free(decoder->owned_view);   /* parsed from cfg.data; os_free(NULL) is a no-op */
    os_free(decoder);
}

bk_baf_decoder_result_t bk_baf_poll(bk_baf_decoder_t * decoder)
{
    if(decoder == NULL) return BK_BAF_DECODER_RESULT_ERROR;

    /* Pace playback to the source's per-frame durations: hold off (report WAIT)
     * until the current frame is due, then advance the schedule when a new frame
     * is produced. The caller just polls and shows whatever FRAME comes back. */
    uint32_t now = rtos_get_time();
    if(bk_baf_pacer_time_until_due(&decoder->pacer, now) > 0) {
        return BK_BAF_DECODER_RESULT_WAIT;
    }

    bk_baf_decoder_result_t result = decoder->ops->poll_frame(decoder->context);
    /* Only advance the schedule while pacing. In free-run the pacer is bypassed,
     * so leaving it frozen means that when free-run is switched back off the stale
     * schedule is > 1 frame behind and frame_shown() re-baselines on the next
     * frame -- instead of next_due racing ahead and stalling playback for seconds. */
    if(result == BK_BAF_DECODER_RESULT_FRAME && !decoder->pacer.free_run) {
        bk_baf_pacer_frame_shown(&decoder->pacer, now,
                                 decoder->ops->get_frame_duration(decoder->context));
    }
    return result;
}

avdk_err_t bk_baf_ioctl(bk_baf_decoder_t * decoder, bk_baf_ioctl_cmd_t cmd, void * arg)
{
    if(decoder == NULL) return AVDK_ERR_INVAL;

    switch(cmd) {
    case BK_BAF_IOCTL_REWIND:
        if(decoder->ops->rewind != NULL) {
            decoder->ops->rewind(decoder->context);
            bk_baf_pacer_reset(&decoder->pacer);   /* restart cadence from the top */
        }
        return AVDK_ERR_OK;

    case BK_BAF_IOCTL_PAUSE:
        if(decoder->ops->pause != NULL) decoder->ops->pause(decoder->context);
        return AVDK_ERR_OK;

    case BK_BAF_IOCTL_RESUME:
        if(decoder->ops->resume != NULL) {
            decoder->ops->resume(decoder->context);
            bk_baf_pacer_reset(&decoder->pacer);   /* re-baseline cadence after pause */
        }
        return AVDK_ERR_OK;

    case BK_BAF_IOCTL_SET_FREERUN:
        if(arg != NULL) decoder->pacer.free_run = *(const bool *)arg;
        return AVDK_ERR_OK;

    default:
        return AVDK_ERR_UNSUPPORTED;
    }
}

uint16_t bk_baf_get_width(const bk_baf_decoder_t * decoder)
{
    return decoder != NULL ? decoder->ops->get_width(decoder->context) : 0U;
}

uint16_t bk_baf_get_height(const bk_baf_decoder_t * decoder)
{
    return decoder != NULL ? decoder->ops->get_height(decoder->context) : 0U;
}

void bk_baf_get_frame_desc(bk_baf_decoder_t * decoder,
                           bk_baf_frame_desc_t * rgb,
                           bk_baf_frame_desc_t * alpha)
{
    uint16_t w = bk_baf_get_width(decoder);
    uint16_t h = bk_baf_get_height(decoder);
    if(rgb != NULL) {
        rgb->data = (decoder != NULL) ? decoder->ops->get_canvas(decoder->context) : NULL;
        rgb->format = BK_BAF_PIXEL_XRGB8888;
        rgb->width = w;
        rgb->height = h;
        rgb->stride = (uint32_t)w * 4U;
    }
    if(alpha != NULL) {
        uint8_t * a = (decoder != NULL && decoder->ops->get_alpha_mask != NULL)
                      ? decoder->ops->get_alpha_mask(decoder->context) : NULL;
        /* Alpha mask is a tight A8 plane at canvas resolution. */
        alpha->data = a;
        alpha->format = BK_BAF_PIXEL_A8;
        alpha->width = a ? w : 0U;
        alpha->height = a ? h : 0U;
        alpha->stride = a ? (uint32_t)w : 0U;
    }
}

int32_t bk_baf_get_loop_count(const bk_baf_decoder_t * decoder)
{
    if(decoder == NULL || decoder->ops->get_loop_count == NULL) return -1;
    return decoder->ops->get_loop_count(decoder->context);
}

void bk_baf_set_loop_count(bk_baf_decoder_t * decoder, int32_t count)
{
    if(decoder != NULL && decoder->ops->set_loop_count != NULL) {
        decoder->ops->set_loop_count(decoder->context, count);
    }
}

/* Public compositor: composites via the selected backend. CPU (Helium) is
 * lock-free; GPU (VG-Lite) serialises against the registered handle (if any). The
 * GPU must already be up -- brought up by bk_baf_init() when init_gpu was set, or
 * owned externally (LVGL/flexa). */
avdk_err_t bk_baf_compose(const bk_baf_frame_desc_t * dst,
                          const bk_baf_frame_desc_t * canvas,
                          const bk_baf_frame_desc_t * alpha,
                          uint32_t clear_argb, bool is_new_layer)
{
    if(s_backend == BK_BAF_RENDER_CPU) {
        return baf_cpu_compose_frame(dst, canvas, alpha, clear_argb, is_new_layer);
    }

    bk_gpu_ctlr_handle_t handle = (bk_gpu_ctlr_handle_t)s_baf_gpu_handle;
    bool locked = (handle != NULL) &&
                  (bk_gpu_ioctl(handle, BK_GPU_IOCTL_LOCK, NULL) == AVDK_ERR_OK);
    avdk_err_t err = baf_gpu_compose_frame(dst, canvas, alpha, clear_argb, is_new_layer);
    if(locked) {
        (void)bk_gpu_ioctl(handle, BK_GPU_IOCTL_UNLOCK, NULL);
    }
    return err;
}
