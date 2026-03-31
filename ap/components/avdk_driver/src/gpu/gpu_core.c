#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>

#include <common/bk_include.h>
#include <components/log.h>
#include <avdk_error.h>
#include <driver/gpu_types.h>
#include "vg_lite.h"
#include "vg_lite_platform.h"
#include "driver/int.h"
#include "sys_driver.h"


#define TAG "gpu_core"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#define GPU_YUV_VADDR_BASE    0x38200000

static bool gpu_driver_is_init = false;

/* Get the bpp information of a color format. */
static void get_format_bytes(vg_lite_buffer_format_t format,
                             uint32_t *mul,
                             uint32_t *div,
                             uint32_t *bytes_align)
{
    *mul = *div = 1;
    *bytes_align = 4;
    switch (format) {
        case VG_LITE_L8:
        case VG_LITE_A8:
        case VG_LITE_RGBA8888_ETC2_EAC:
            break;

        case VG_LITE_A4:
            *div = 2;
            break;

        case VG_LITE_ABGR1555:
        case VG_LITE_ARGB1555:
        case VG_LITE_BGRA5551:
        case VG_LITE_RGBA5551:
        case VG_LITE_RGBA4444:
        case VG_LITE_BGRA4444:
        case VG_LITE_ABGR4444:
        case VG_LITE_ARGB4444:
        case VG_LITE_RGB565:
        case VG_LITE_BGR565:
        case VG_LITE_YUYV:
        case VG_LITE_YUY2:
        case VG_LITE_YUY2_TILED:
        /* AYUY2 buffer memory = YUY2 + alpha. */
        case VG_LITE_AYUY2:
        case VG_LITE_AYUY2_TILED:
        /* ABGR8565_PLANAR buffer memory = RGB565 + alpha. */
        case VG_LITE_ABGR8565_PLANAR:
        case VG_LITE_ARGB8565_PLANAR:
        case VG_LITE_RGBA5658_PLANAR:
        case VG_LITE_BGRA5658_PLANAR:
            *mul = 2;
            break;

        case VG_LITE_RGBA8888:
        case VG_LITE_BGRA8888:
        case VG_LITE_ABGR8888:
        case VG_LITE_ARGB8888:
        case VG_LITE_RGBX8888:
        case VG_LITE_BGRX8888:
        case VG_LITE_XBGR8888:
        case VG_LITE_XRGB8888:
            *mul = 4;
            break;

        case VG_LITE_NV12:
        case VG_LITE_NV12_TILED:
            *mul = 1;
            break;

        case VG_LITE_ANV12:
        case VG_LITE_ANV12_TILED:
            *mul = 4;
            break;

        case VG_LITE_INDEX_1:
            *div = 8;
            *bytes_align = 8;
            break;

        case VG_LITE_INDEX_2:
            *div = 4;
            *bytes_align = 8;
            break;

        case VG_LITE_INDEX_4:
            *div = 2;
            *bytes_align = 8;
            break;

        case VG_LITE_INDEX_8:
            *bytes_align = 1;
            break;

        case VG_LITE_RGBA2222:
        case VG_LITE_BGRA2222:
        case VG_LITE_ABGR2222:
        case VG_LITE_ARGB2222:
            *mul = 1;
            break;

        case VG_LITE_RGB888:
        case VG_LITE_BGR888:
        case VG_LITE_ABGR8565:
        case VG_LITE_BGRA5658:
        case VG_LITE_ARGB8565:
        case VG_LITE_RGBA5658:
            *mul = 3;
            break;

        /* OpenVG format*/
        case OPENVG_sRGBX_8888:
        case OPENVG_sRGBX_8888_PRE:
        case OPENVG_sRGBA_8888:
        case OPENVG_sRGBA_8888_PRE:
        case OPENVG_lRGBX_8888:
        case OPENVG_lRGBX_8888_PRE:
        case OPENVG_lRGBA_8888:
        case OPENVG_lRGBA_8888_PRE:
        case OPENVG_sXRGB_8888:
        case OPENVG_sARGB_8888:
        case OPENVG_sARGB_8888_PRE:
        case OPENVG_lXRGB_8888:
        case OPENVG_lARGB_8888:
        case OPENVG_lARGB_8888_PRE:
        case OPENVG_sBGRX_8888:
        case OPENVG_sBGRA_8888:
        case OPENVG_sBGRA_8888_PRE:
        case OPENVG_lBGRX_8888:
        case OPENVG_lBGRA_8888:
        case OPENVG_sXBGR_8888:
        case OPENVG_sABGR_8888:
        case OPENVG_lBGRA_8888_PRE:
        case OPENVG_sABGR_8888_PRE:
        case OPENVG_lXBGR_8888:
        case OPENVG_lABGR_8888:
        case OPENVG_lABGR_8888_PRE:
            *mul = 4;
            break;

        case OPENVG_sRGBA_5551:
        case OPENVG_sRGBA_5551_PRE:
        case OPENVG_lRGBA_5551:
        case OPENVG_lRGBA_5551_PRE:
        case OPENVG_sRGBA_4444:
        case OPENVG_sRGBA_4444_PRE:
        case OPENVG_lRGBA_4444:
        case OPENVG_lRGBA_4444_PRE:
        case OPENVG_sARGB_1555:
        case OPENVG_sARGB_4444:
        case OPENVG_sBGRA_5551:
        case OPENVG_sBGRA_4444:
        case OPENVG_sABGR_1555:
        case OPENVG_sABGR_4444:
        case OPENVG_sRGB_565:
        case OPENVG_sRGB_565_PRE:
        case OPENVG_sBGR_565:
        case OPENVG_lRGB_565:
        case OPENVG_lRGB_565_PRE:
            * mul = 2;
            break;

        case OPENVG_sL_8:
        case OPENVG_lL_8:
        case OPENVG_A_8:
            break;

        case OPENVG_BW_1:
        case OPENVG_A_4:
        case OPENVG_A_1:
            * div = 2;
            break;

        default:
            break;
    }
}

/* Handle tiled & yuv allocation. Currently including NV12, ANV12, YV12, YV16, NV16, YV24, NV24. */
static vg_lite_error_t _allocate_tiled_yuv_planar_with_data(vg_lite_buffer_t * buffer, void* y, void *u, void* v, void* alpha)
{
    vg_lite_error_t error = VG_LITE_SUCCESS;
    // uint32_t    yplane_size = 0;
    // vg_lite_kernel_allocate_t allocate, uv_allocate, v_allocate;

    if (((buffer->format < VG_LITE_NV12) || (buffer->format > VG_LITE_ANV12_TILED)
        || (buffer->format == VG_LITE_AYUY2) || (buffer->format == VG_LITE_YUY2_TILED))
        && ((buffer->format != VG_LITE_NV24) && (buffer->format != VG_LITE_NV24_TILED)))
    {
        return error;
    }

    if (y== NULL) {
        BK_LOGE("VGLITE", "invalid input\r\n");
        return VG_LITE_INVALID_ARGUMENT;
    }

    /* For NV12, there are 2 planes (Y, UV);
     For ANV12, there are 3 planes (Y, UV, Alpha).
     Each plane must be aligned by (4, 8).
     Then Y plane must be aligned by (8, 8).
     For YVxx, there are 3 planes (Y, U, V).
     YV12 is similar to NV12, both YUV420 format.
     YV16 and NV16 are YUV422 format.
     YV24 is YUV444 format.
     */
    buffer->width = VG_LITE_ALIGN(buffer->width, 8);
    buffer->height = VG_LITE_ALIGN(buffer->height, 8);
    buffer->stride = VG_LITE_ALIGN(buffer->width, 16); //qy to do why 64 byte

    switch (buffer->format) {
        case VG_LITE_NV12:
        case VG_LITE_ANV12:
        case VG_LITE_NV12_TILED:
        case VG_LITE_ANV12_TILED:
            buffer->yuv.uv_stride = buffer->stride;
            buffer->yuv.alpha_stride = buffer->stride;
            buffer->yuv.uv_height = buffer->height / 2;
            break;

        case VG_LITE_NV16:
            buffer->yuv.uv_stride = buffer->stride;
            buffer->yuv.uv_height = buffer->height;
            break;

        case VG_LITE_NV24:
        case VG_LITE_NV24_TILED:
            buffer->yuv.uv_stride = buffer->stride * 2;
            buffer->yuv.uv_height = buffer->height;
            break;

        case VG_LITE_YV12:
            buffer->yuv.uv_stride =
            buffer->yuv.v_stride = buffer->stride / 2;
            buffer->yuv.uv_height =
            buffer->yuv.v_height = buffer->height / 2;
            break;

        case VG_LITE_YV16:
            buffer->yuv.uv_stride =
            buffer->yuv.v_stride = buffer->stride;
            buffer->yuv.uv_height =
            buffer->yuv.v_height = buffer->height / 2;
            break;

        case VG_LITE_YV24:
            buffer->yuv.uv_stride =
            buffer->yuv.v_stride = buffer->stride;
            buffer->yuv.uv_height =
            buffer->yuv.v_height = buffer->height;
            break;

        default:
            return error;
    }

    // yplane_size = buffer->stride * buffer->height;

    /* Save the allocation. */
    buffer->handle  = y;
    buffer->memory  = y;
    buffer->address = (uintptr_t)y;

    if ((buffer->format == VG_LITE_NV12) || (buffer->format == VG_LITE_ANV12)
        || (buffer->format == VG_LITE_NV16) || (buffer->format == VG_LITE_NV24)
        || (buffer->format == VG_LITE_NV12_TILED) || (buffer->format == VG_LITE_ANV12_TILED)) {
        /* Allocate buffer memory: UV. */

        if (u == NULL) {
            BK_LOGE("VGLITE", "invalid input\r\n");
            return VG_LITE_INVALID_ARGUMENT;
        }

        buffer->yuv.uv_handle = u;
        buffer->yuv.uv_memory = u;
        buffer->yuv.uv_planar = (uintptr_t)u;

        if ((buffer->format == VG_LITE_ANV12) || (buffer->format == VG_LITE_ANV12_TILED)) {
            if (alpha == NULL) {
                BK_LOGE("VGLITE", "invalid input\r\n");
                return VG_LITE_INVALID_ARGUMENT;
            }

            buffer->yuv.alpha_planar = (uintptr_t)alpha;
        }
    } else {

        if (u == NULL || v==NULL) {
            BK_LOGE("VGLITE", "invalid input\r\n");
            return VG_LITE_INVALID_ARGUMENT;
        }

        /* Allocate buffer memory: U, V. */
        buffer->yuv.uv_handle = u;
        buffer->yuv.uv_memory = u;
        buffer->yuv.uv_planar = (uintptr_t)u;

        buffer->yuv.v_handle = v;
        buffer->yuv.v_memory = v;
        buffer->yuv.v_planar = (uintptr_t)v;
    }

    return error;
}

/**
 * @details only support continous storage format
*/
static vg_lite_error_t vg_lite_allocate_with_data(vg_lite_buffer_t * buffer, void* y, void *u, void* v, void* alpha)
{
#if DUMP_API
    FUNC_DUMP(vg_lite_allocate)(buffer);
#endif

    // vg_lite_error_t error = VG_LITE_SUCCESS;
    // vg_lite_kernel_allocate_t allocate;

#if gcFEATURE_VG_TRACE_API
    VGLITE_LOG("vg_lite_allocate %p  (w: %d, h: %d, fmt: %d)\n", buffer, buffer->width, buffer->height, buffer->format);
#endif

    if (y == NULL) {
        BK_LOGE("VGLITE", "invalid input\r\n");
        return VG_LITE_INVALID_ARGUMENT;
    }

    if (buffer->format == VG_LITE_RGBA8888_ETC2_EAC &&
#if (CHIPID == 0x555)
       (buffer->width % 16 || buffer->height % 4)
#else
       (buffer->width % 4 || buffer->height % 4)
#endif
        )
    {
        return VG_LITE_INVALID_ARGUMENT;
    }

    /* Set buffer->premultiplied properly according to buffer->format */
    if (buffer->format < VG_LITE_RGBA8888)
    {   /* For all OpenVG VG_* formats */
#if gcFEATURE_VG_HW_PREMULTIPLY
        switch (buffer->format) {
            case OPENVG_sRGBA_8888_PRE:
            case OPENVG_lRGBA_8888_PRE:
            case OPENVG_sARGB_8888_PRE:
            case OPENVG_lARGB_8888_PRE:
            case OPENVG_sBGRA_8888_PRE:
            case OPENVG_lBGRA_8888_PRE:
            case OPENVG_sABGR_8888_PRE:
            case OPENVG_lABGR_8888_PRE:
            case OPENVG_sRGBX_8888_PRE:
            case OPENVG_lRGBX_8888_PRE:
            case OPENVG_sRGB_565_PRE:
            case OPENVG_lRGB_565_PRE:
            case OPENVG_sRGBA_5551_PRE:
            case OPENVG_lRGBA_5551_PRE:
            case OPENVG_sRGBA_4444_PRE:
            case OPENVG_lRGBA_4444_PRE:
                buffer->premultiplied = 1;
                break;
            default:
                buffer->premultiplied = 0;
                break;
        };
#else
        /* Cannot support OpenVG VG_* format if HW does not support premultiply */
        return VG_LITE_INVALID_ARGUMENT;
#endif
    }
    else {
        /* All VG_LITE_* formats are not premultiplied */
        buffer->premultiplied = 0;
    }

    /* Reset planar. */
    buffer->yuv.uv_planar =
    buffer->yuv.v_planar =
    buffer->yuv.alpha_planar = 0;

    /* Align height in case format is tiled. */
    if ((buffer->format >= VG_LITE_YUY2 && buffer->format <= VG_LITE_NV16) || buffer->format == VG_LITE_NV24) {
        buffer->height = VG_LITE_ALIGN(buffer->height, 4);
        buffer->yuv.swizzle = VG_LITE_SWIZZLE_UV;
    }

    if ((buffer->format >= VG_LITE_YUY2_TILED && buffer->format <= VG_LITE_AYUY2_TILED) || buffer->format == VG_LITE_NV24_TILED) {
        buffer->height = VG_LITE_ALIGN(buffer->height, 4);
        buffer->tiled = VG_LITE_TILED;
        buffer->yuv.swizzle = VG_LITE_SWIZZLE_UV;
    }

    if ((buffer->format >= VG_LITE_NV12 && buffer->format <= VG_LITE_ANV12_TILED
         && buffer->format != VG_LITE_AYUY2 && buffer->format != VG_LITE_YUY2_TILED) 
        || (buffer->format >= VG_LITE_NV24 && buffer->format <= VG_LITE_NV24_TILED)) {
        _allocate_tiled_yuv_planar_with_data(buffer, y, u, v, alpha);
    }
    else {
        /* Driver need compute the stride always with RT500 project. */

        // vg_lite_float_t ratio = 1.0f;
        uint32_t mul, div, align;
        get_format_bytes(buffer->format, &mul, &div, &align);
        buffer->stride = buffer->width * mul / div;

#if 0 && gcFEATURE_VG_16PIXELS_ALIGNED
        int tmp_align = 16 * mul / div;
        if ((mul / div) % 2 != 0) {
            if (buffer->stride % tmp_align != 0) {
                buffer->stride = (buffer->stride + tmp_align) / tmp_align * tmp_align;
            }
        }
        else {
            buffer->stride = VG_LITE_ALIGN(buffer->stride, tmp_align);
        }
#endif

        /* Save the buffer allocation. */
        buffer->handle  = y;
        buffer->memory  = y;
        buffer->address = (uintptr_t)y;
        buffer->pool    = 2; // VG_LITE_MEMORY_POOL_EXT;

        if ((buffer->format == VG_LITE_AYUY2) || (buffer->format == VG_LITE_AYUY2_TILED) || ((buffer->format >= VG_LITE_ABGR8565_PLANAR)
             && (buffer->format <= VG_LITE_RGBA5658_PLANAR))) {

            if (alpha == NULL) {
                BK_LOGE("VGLITE", "invalid input\r\n");
                return VG_LITE_INVALID_ARGUMENT;
            }

            buffer->yuv.alpha_planar = (uintptr_t)alpha;
        }
    }

#if gcFEATURE_VG_TRACE_API
    VGLITE_LOG("=>buffer: width=%d, height=%d, stride=%d, bytes=%d, format=%d\n",
        buffer->width, buffer->height, buffer->stride, allocate.bytes, buffer->format);
#endif

    return VG_LITE_SUCCESS;
}


static vg_lite_error_t vg_lite_free_without_free_data(vg_lite_buffer_t * buffer)
{
#if DUMP_API
    FUNC_DUMP(vg_lite_free)(buffer);
#endif

    // vg_lite_error_t error;
    // vg_lite_kernel_free_t free;
    // vg_lite_kernel_free_t uv_free, v_free;

#if gcFEATURE_VG_TRACE_API
    VGLITE_LOG("vg_lite_free %p\n", buffer);
#endif

    if (buffer == NULL)
        return VG_LITE_INVALID_ARGUMENT;
#if 0
    if (!(memcmp(s_context.rtbuffer,buffer,sizeof(vg_lite_buffer_t))) ) {
        if (VG_LITE_SUCCESS == submit(&s_context)) {
            CHECK_ERROR(stall(&s_context, 0, ~0));
        }

#if !DUMP_COMMAND_CAPTURE
        vglitemDUMP("@[swap 0x%08X %dx%d +%u]",
            s_context.rtbuffer->address,
            s_context.rtbuffer->width, s_context.rtbuffer->height,
            s_context.rtbuffer->stride);
        vglitemDUMP_BUFFER(
            "framebuffer",
            (size_t)s_context.rtbuffer->address,s_context.rtbuffer->memory,
            0,
            s_context.rtbuffer->stride*(s_context.rtbuffer->height));
#endif

        memset(s_context.rtbuffer, 0, sizeof(vg_lite_buffer_t));
    }
#endif
#if !gcFEATURE_VG_LVGL_SUPPORT
    if (buffer->lvgl_buffer != NULL) {
        // free.memory_handle = buffer->lvgl_buffer->handle;
        // CHECK_ERROR(vg_lite_kernel(VG_LITE_FREE, &free));
        // vg_lite_os_free(buffer->lvgl_buffer);
        buffer->lvgl_buffer = NULL;
    }
#endif

    if (buffer->yuv.uv_planar) {

        /* Mark the buffer as freed. */
        buffer->yuv.uv_handle = NULL;
        buffer->yuv.uv_memory = NULL;
    }

    if (buffer->yuv.v_planar) {
        /* Mark the buffer as freed. */
        buffer->yuv.v_handle = NULL;
        buffer->yuv.v_memory = NULL;
    }

#if gcFEATURE_VG_IM_FASTCLEAR
    if (buffer->fc_buffer[0].handle != 0)
    {
#if VG_TARGET_FC_DUMP
        vglitemDUMP_BUFFER(
            "fcbuffer",
            (uint64_t)buffer->fc_buffer[0].address,buffer->fc_buffer[0].memory,
            0,
            buffer->fc_buffer[0].stride*(buffer->fc_buffer[0].height));
#endif
        _free_fc_buffer(&buffer->fc_buffer[0]);
    }
#endif

    /* Make sure we have a valid memory handle. */
    if (buffer->handle == NULL) {
        return VG_LITE_INVALID_ARGUMENT;
    }

    /* Mark the buffer as freed. */
    buffer->handle = NULL;
    buffer->memory = NULL;

    return VG_LITE_SUCCESS;
}

static void bk_gpu_driver_init(uint8_t sel, uint8_t div)
{
    if (gpu_driver_is_init == true)
    {
        LOGW("%s has inited %x\r\n", __func__);
        return;
    }

    // set clock
    sys_hal_set_gpu_clk_sel(sel);  // 0: 320M, 1: 480M
    sys_hal_set_gpu_clk_div(div);  // F / (div + 1)

    // enable clock
    sys_hal_set_gpu_clk_en(1);

    // enable interrupt
    bk_int_isr_register(INT_SRC_VID_DISP0, vg_lite_IRQHandler, NULL);
    sys_drv_a35_int_enable(VID_DISP0_INTERRUPT_CTRL_BIT);

    gpu_driver_is_init = true;
}

static void bk_gpu_driver_deinit(void)
{
    if (gpu_driver_is_init == false)
    {
        LOGW("%s has inited %x\r\n", __func__);
        return;
    }

    sys_drv_a35_int_disable(VID_DISP0_INTERRUPT_CTRL_BIT);
    bk_int_isr_unregister(INT_SRC_VID_DISP0);

    sys_hal_set_gpu_clk_en(0);

    gpu_driver_is_init = false;
}

static void bk_gpu_yuv_buffer_config(bk_gpu_yuv_buff_t *yuv_buffer)
{
    if (yuv_buffer == NULL)
    {
        LOGE("%s %d yuv_buffer is NULL\r\n", __func__, __LINE__);
        return;
    }

    sys_hal_set_buffa_enable_value(yuv_buffer->y_buff.buff_enable);
    sys_hal_set_gpu_buffa_begin_value(yuv_buffer->y_buff.buff_begin);
    sys_hal_set_gpu_buffa_size_value(yuv_buffer->y_buff.buff_size);
    sys_hal_set_gpu_pica_begin_value(yuv_buffer->y_buff.pic_begin);
    sys_hal_set_gpu_pica_halfbuff_end_value(yuv_buffer->y_buff.pic_halfbuf_end);
    sys_hal_set_gpu_pica_end_value(yuv_buffer->y_buff.pic_end);

    sys_hal_set_buffb_enable_value(yuv_buffer->uv_buff.buff_enable);
    sys_hal_set_gpu_buffb_begin_value(yuv_buffer->uv_buff.buff_begin);
    sys_hal_set_gpu_buffb_size_value(yuv_buffer->uv_buff.buff_size);
    sys_hal_set_gpu_picb_begin_value(yuv_buffer->uv_buff.pic_begin);
    sys_hal_set_gpu_picb_halfbuff_end_value(yuv_buffer->uv_buff.pic_halfbuf_end);
    sys_hal_set_gpu_picb_end_value(yuv_buffer->uv_buff.pic_end);

    LOGI("%s is completed\r\n", __func__);
}

bk_err_t bk_gpu_drv_init(bk_gpu_handle_t *gpu_handle, bk_dgpu_config_t *gpu_config)
{
    bk_err_t ret = BK_OK;
    uint32_t dst_buf_idx = 0;

    if (*gpu_handle != NULL)
    {
        LOGW("%s %d gpu_handle is not NULL %x\r\n", __func__, __LINE__, *gpu_handle);
        return BK_FAIL;
    }

    if (gpu_config == NULL)
    {
        LOGW("%s %d gpu_config is NULL\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    bk_gpu_context_t *gpu_context = (bk_gpu_context_t *)os_malloc(sizeof(bk_gpu_context_t));
    if (!gpu_context)
    {
        LOGE("%s %d gpu_context malloc failed %x\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    os_memset(gpu_context, 0, sizeof(bk_gpu_context_t));

    bk_gpu_driver_init(1, 0);

    gpu_context->flexa_mode = gpu_config->flexa_mode;

    if (gpu_config->flexa_mode == 1)
    {
        bk_gpu_yuv_buffer_config(gpu_config->yuv_buff);
    }

    vg_lite_init(gpu_config->tessellation_width, gpu_config->tessellation_height);
    vg_lite_identity(&gpu_context->gpu_matrix);

    if (gpu_config->gpu_rotate)
    {
        if (gpu_config->gpu_rotate->rotate_enable == 1)
        {
            vg_lite_rotate(gpu_config->gpu_rotate->rotate_degree, &gpu_context->gpu_matrix);
        }
    }

    if (gpu_config->gpu_scale)
    {
        if (gpu_config->gpu_scale->scale_enable == 1)
        {
            float scale_x = (float)gpu_config->gpu_scale->scale_width / (float)gpu_config->gpu_scale->src_width;
            float scale_y = (float)gpu_config->gpu_scale->scale_height / (float)gpu_config->gpu_scale->src_height;
            vg_lite_scale(scale_x, scale_y, &gpu_context->gpu_matrix);
        }
    }

    gpu_context->gpu_srcbuf.width = gpu_config->srcbuf->width;
    gpu_context->gpu_srcbuf.height = gpu_config->srcbuf->height;
    gpu_context->gpu_srcbuf.format = gpu_config->srcbuf->format;
    gpu_context->gpu_srcbuf.compress_mode = gpu_config->srcbuf->compress_mode;
    gpu_context->gpu_srcbuf.tiled = gpu_config->srcbuf->tiled;

    gpu_context->gpu_dstbuf.width = gpu_config->dstbuf->width;
    gpu_context->gpu_dstbuf.height = gpu_config->dstbuf->height;
    gpu_context->gpu_dstbuf.format = gpu_config->dstbuf->format;
    gpu_context->gpu_dstbuf.compress_mode = gpu_config->dstbuf->compress_mode;
    gpu_context->gpu_dstbuf.tiled = gpu_config->dstbuf->tiled;

    if (gpu_context->flexa_mode == 1)
    {
        vg_lite_allocate_with_data(&gpu_context->gpu_srcbuf, (void *)gpu_config->yuv_buff->y_buff.pic_begin, (void *)gpu_config->yuv_buff->uv_buff.pic_begin, NULL, NULL);
        vg_lite_allocate_with_data(&gpu_context->gpu_dstbuf, (void *)gpu_config->dstbuf->memory, NULL, NULL, NULL);
    }
    else
    {
        vg_lite_allocate(&gpu_context->gpu_srcbuf);
        vg_lite_allocate(&gpu_context->gpu_dstbuf);
    }

    *gpu_handle = gpu_context;

    return ret;
}

bk_err_t bk_gpu_drv_deinit(bk_gpu_handle_t *gpu_handle)
{
    bk_err_t ret = BK_OK;

    bk_gpu_context_t *context = (bk_gpu_context_t *)*gpu_handle;
    if (context == NULL)
    {
        LOGE("%s %d context is NULL\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (context->flexa_mode == 1)
    {
        vg_lite_free_without_free_data(&context->gpu_srcbuf);
        vg_lite_free_without_free_data(&context->gpu_dstbuf);
    }
    else
    {
        vg_lite_free(&context->gpu_srcbuf);
        vg_lite_free(&context->gpu_dstbuf);
    }

    if (context)
    {
        os_free(context);
        context = NULL;
    }

    vg_lite_close();

    bk_gpu_driver_deinit();

    return ret;
}

void bk_gpu_drv_blit_start(bk_gpu_handle_t *gpu_handle)
{
    bk_gpu_context_t * context = (bk_gpu_context_t *)*gpu_handle;
    if (context == NULL)
    {
        LOGE("%s %d context is NULL\r\n", __func__, __LINE__);
        return;
    }

    vg_lite_blit(&context->gpu_dstbuf, &context->gpu_srcbuf, &context->gpu_matrix, VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);

    vg_lite_finish();
}

