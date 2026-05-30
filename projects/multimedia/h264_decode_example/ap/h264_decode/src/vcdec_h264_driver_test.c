#include <os/os.h>
#include "os/mem.h"
#include "os/str.h"
#include <components/bk_frame_buffer.h>
#include <components/log.h>
#include <driver/int.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "sys_driver.h"
#include <common/avdk_pixel_types.h>
#include "modules/vcdec/vcdec_h264_api.h"
#include "vcdec_h264_driver_test.h"
#include "h264_decode_stream_1280x720.h"
#include "h264_decode_stream_256x128.h"

#define TAG "vcdec_h264_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define CLI_CMD_RSP_SUCCEED "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR   "CMDRSP:ERROR\r\n"

#define VCDEC_H264_TEST_STREAM_1280X720      0
#define VCDEC_H264_TEST_STREAM_256X128       1

#ifndef VCDEC_H264_TEST_STREAM_SELECT
#define VCDEC_H264_TEST_STREAM_SELECT        VCDEC_H264_TEST_STREAM_1280X720
#endif

#if (VCDEC_H264_TEST_STREAM_SELECT != VCDEC_H264_TEST_STREAM_1280X720) && \
    (VCDEC_H264_TEST_STREAM_SELECT != VCDEC_H264_TEST_STREAM_256X128)
#error "Unsupported VCDEC_H264_TEST_STREAM_SELECT"
#endif

#define VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB  1U
#define VCDEC_H264_TEST_FLEXA_SEG_NUM        2U
#define VCDEC_H264_TEST_STACK_SIZE           (16 * 1024)
#ifndef VCDEC_H264_TEST_DUMP_ENABLE
#define VCDEC_H264_TEST_DUMP_ENABLE          0
#endif
#ifndef VCDEC_H264_TEST_DUMP_FRAME_FIRST_I
#define VCDEC_H264_TEST_DUMP_FRAME_FIRST_I   1
#endif
#ifndef VCDEC_H264_TEST_DUMP_FRAME_FIRST_P
#define VCDEC_H264_TEST_DUMP_FRAME_FIRST_P   1
#endif
#ifndef VCDEC_H264_TEST_DUMP_FLEXA_SEGMENT
#define VCDEC_H264_TEST_DUMP_FLEXA_SEGMENT   1
#endif

extern void vcdec_h264_isr(void);
extern void vcdec_h264_pp_isr(void);

typedef struct {
    uint32_t id;
    const char *name;
    const uint8_t *stream;
    const uint32_t *bytes;
    uint32_t width;
    uint32_t height;
} vcdec_h264_test_stream_t;

static const vcdec_h264_test_stream_t s_vcdec_h264_test_stream_1280x720 = {
    .id = VCDEC_H264_TEST_STREAM_1280X720,
    .name = "1280x720",
    .stream = h264_decode_stream_1280x720,
    .bytes = &h264_decode_stream_1280x720_bytes,
    .width = 1280U,
    .height = 720U,
};

static const vcdec_h264_test_stream_t s_vcdec_h264_test_stream_256x128 = {
    .id = VCDEC_H264_TEST_STREAM_256X128,
    .name = "256x128",
    .stream = h264_decode_stream_256x128,
    .bytes = &h264_decode_stream_256x128_bytes,
    .width = 256U,
    .height = 128U,
};

typedef enum {
    VCDEC_H264_TEST_MODE_FRAME = 0,
    VCDEC_H264_TEST_MODE_FLEXA = 1,
} vcdec_h264_test_mode_t;

typedef struct {
    vcdec_handle handle;
    vcdec_h264_test_mode_t mode;
    const vcdec_h264_test_stream_t *stream_cfg;
    uint32_t width;
    uint32_t height;
    volatile uint32_t frame_done_count;
    volatile int last_frame_status;
    volatile uint32_t flexa_done_count;
    volatile uint32_t last_wr_ptr;
    uint8_t *flexa_out_buf;
    uint32_t flexa_seg_height;
    uint32_t flexa_seg_size;
    uint32_t flexa_seg_num;
    uint8_t flexa_dump_active;
    uint8_t flexa_dump_kind;
    uint8_t *flexa_frame_buf;
    uint32_t flexa_frame_size;
    uint32_t flexa_copy_wr_ptr;
} vcdec_h264_test_ctx_t;

typedef struct {
    const uint8_t *data;
    uint32_t size;
    uint32_t byte_pos;
    uint8_t curr_byte;
    uint8_t bit_pos;
    uint8_t zero_count;
} h264_test_bs_t;

static beken_thread_t s_vcdec_h264_test_thread = NULL;
static volatile uint8_t s_vcdec_h264_test_running = 0;

#define VCDEC_H264_TEST_THREAD_ARG(mode, stream_id) \
    ((uintptr_t)(((uint32_t)(mode) & 0xFFU) | (((uint32_t)(stream_id) & 0xFFU) << 8U)))
#define VCDEC_H264_TEST_THREAD_MODE(arg) \
    ((vcdec_h264_test_mode_t)((uint32_t)(uintptr_t)(arg) & 0xFFU))
#define VCDEC_H264_TEST_THREAD_STREAM(arg) \
    (((uint32_t)(uintptr_t)(arg) >> 8U) & 0xFFU)

extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);

static void *vcdec_h264_test_uncoded_malloc(uint32_t size)
{
    return bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
}

static void vcdec_h264_test_uncoded_free(void *ptr)
{
    bk_frame_buffer_free(ptr);
}

static void vcdec_h264_test_dump_buffer(const char *tag, uint8_t *buf, uint32_t size)
{
#if VCDEC_H264_TEST_DUMP_ENABLE
    uint32_t dump_size;
    uint32_t dump_aligned;
    uint32_t irq_flags;

    if (tag == NULL || buf == NULL || size == 0U) {
        return;
    }

    dump_size = size;
    dump_aligned = (dump_size + 3U) & ~3U;

    irq_flags = rtos_enter_critical();
    LOGI("nv12_dump tag=%s size=%u buf=%p aligned=%u\r\n",
         tag,
         (unsigned)dump_size,
         buf,
         (unsigned)dump_aligned);
    stack_mem_dump((uint32_t)(uintptr_t)buf,
                   (uint32_t)(uintptr_t)(buf + dump_aligned));
    rtos_exit_critical(irq_flags);
#else
    (void)tag;
    (void)buf;
    (void)size;
#endif
}

static void vcdec_h264_test_dump_nv12_frame(const char *tag, uint8_t *y, uint32_t width, uint32_t height)
{
    if (tag == NULL || y == NULL || width == 0U || height == 0U) {
        return;
    }

    vcdec_h264_test_dump_buffer(tag, y, bk_image_size_get((uint16_t)width, (uint16_t)height, BK_PIXEL_FORMAT_NV12));
}

static void vcdec_h264_test_copy_flexa_segment(vcdec_h264_test_ctx_t *ctx, uint32_t wr_ptr)
{
#if VCDEC_H264_TEST_DUMP_ENABLE && VCDEC_H264_TEST_DUMP_FLEXA_SEGMENT
    uint32_t curr_wr_ptr;
    uint32_t y_ring_size;
    uint32_t frame_size;

    if (ctx == NULL || ctx->flexa_out_buf == NULL || ctx->flexa_frame_buf == NULL ||
        ctx->width == 0U || ctx->height == 0U || ctx->flexa_seg_size == 0U ||
        ctx->flexa_seg_height == 0U || ctx->flexa_seg_num == 0U || wr_ptr < ctx->flexa_seg_height ||
        ctx->flexa_dump_active == 0U) {
        return;
    }

    frame_size = bk_image_size_get((uint16_t)ctx->width, (uint16_t)ctx->height, BK_PIXEL_FORMAT_NV12);
    if (ctx->flexa_frame_size < frame_size) {
        return;
    }

    y_ring_size = ctx->width * 16U * ctx->flexa_seg_height * ctx->flexa_seg_num;
    for (curr_wr_ptr = ctx->flexa_copy_wr_ptr + ctx->flexa_seg_height;
         curr_wr_ptr <= wr_ptr;
         curr_wr_ptr += ctx->flexa_seg_height) {
        uint32_t seg_idx;
        uint32_t line_start;
        uint32_t line_count;
        uint8_t *seg_y_buf;
        uint8_t *seg_uv_buf;
        uint32_t y_size;
        uint32_t uv_size;
        uint8_t *dst_y;
        uint8_t *dst_uv;

        seg_idx = ((curr_wr_ptr - 1U) / ctx->flexa_seg_height) % ctx->flexa_seg_num;
        line_start = (curr_wr_ptr - ctx->flexa_seg_height) * 16U;
        if (line_start >= ctx->height) {
            continue;
        }
        line_count = 16U * ctx->flexa_seg_height;
        if (line_start + line_count > ctx->height) {
            line_count = ctx->height - line_start;
        }
        y_size = ctx->width * line_count;
        uv_size = y_size / 2U;
        seg_y_buf = ctx->flexa_out_buf + (seg_idx * (ctx->width * 16U * ctx->flexa_seg_height));
        seg_uv_buf = ctx->flexa_out_buf + y_ring_size + (seg_idx * (ctx->width * 8U * ctx->flexa_seg_height));
        dst_y = ctx->flexa_frame_buf + (line_start * ctx->width);
        dst_uv = ctx->flexa_frame_buf + (ctx->width * ctx->height) +
                 ((line_start / 2U) * ctx->width);

        os_memcpy(dst_y, seg_y_buf, y_size);
        os_memcpy(dst_uv, seg_uv_buf, uv_size);
    }
    ctx->flexa_copy_wr_ptr = wr_ptr;
#else
    (void)ctx;
    (void)wr_ptr;
#endif
}

static bk_err_t vcdec_h264_test_power_on(void)
{
    bk_err_t ret = bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26D, PM_POWER_MODULE_STATE_ON);

    if (ret != BK_OK) {
        LOGE("power on h26d failed, ret=%d\r\n", (int)ret);
    }

    return ret;
}

static void vcdec_h264_test_power_off(void)
{
    bk_err_t ret = bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26D, PM_POWER_MODULE_STATE_OFF);

    if (ret != BK_OK) {
        LOGE("power off h26d failed, ret=%d\r\n", (int)ret);
    }
}

static bk_err_t vcdec_h264_test_int_register(void)
{
    bk_int_isr_register(INT_SRC_H264D, (int_group_isr_t)&vcdec_h264_isr, NULL);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H264D, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H264D, 1);
#endif

    bk_int_isr_register(INT_SRC_H264D_PP, (int_group_isr_t)&vcdec_h264_pp_isr, NULL);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H264D_PP, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H264D_PP, 1);
#endif

    return BK_OK;
}

static void vcdec_h264_test_int_unregister(void)
{
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H264D, 0);
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H264D_PP, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H264D, 0);
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H264D_PP, 0);
#endif
    bk_int_isr_unregister(INT_SRC_H264D);
    bk_int_isr_unregister(INT_SRC_H264D_PP);
}

static void vcdec_h264_test_cli_write_rsp(char *pcWriteBuffer, int xWriteBufferLen, const char *msg)
{
    int len;

    if (pcWriteBuffer == NULL || xWriteBufferLen <= 0 || msg == NULL) {
        return;
    }

    len = (int)os_strlen(msg);
    if (len >= xWriteBufferLen) {
        len = xWriteBufferLen - 1;
    }

    os_memcpy(pcWriteBuffer, msg, (size_t)len);
    pcWriteBuffer[len] = '\0';
}

static uint32_t vcdec_h264_test_frame_size(uint32_t width, uint32_t height)
{
    return bk_image_size_get((uint16_t)width, (uint16_t)height, BK_PIXEL_FORMAT_NV12);
}

static uint32_t vcdec_h264_test_flexa_out_size(uint32_t width)
{
    return bk_image_size_get((uint16_t)width,
                             16U * VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB * VCDEC_H264_TEST_FLEXA_SEG_NUM,
                             BK_PIXEL_FORMAT_NV12);
}

static const vcdec_h264_test_stream_t *vcdec_h264_test_get_stream_cfg(uint32_t stream_id)
{
    switch (stream_id) {
    case VCDEC_H264_TEST_STREAM_1280X720:
        return &s_vcdec_h264_test_stream_1280x720;
    case VCDEC_H264_TEST_STREAM_256X128:
        return &s_vcdec_h264_test_stream_256x128;
    default:
        return NULL;
    }
}

static const vcdec_h264_test_stream_t *vcdec_h264_test_get_default_stream_cfg(void)
{
    return vcdec_h264_test_get_stream_cfg(VCDEC_H264_TEST_STREAM_SELECT);
}

static const vcdec_h264_test_stream_t *vcdec_h264_test_parse_stream_arg(const char *arg)
{
    if (arg == NULL) {
        return vcdec_h264_test_get_default_stream_cfg();
    }

    if (os_strcmp(arg, "1280x720") == 0) {
        return &s_vcdec_h264_test_stream_1280x720;
    }

    if (os_strcmp(arg, "256x128") == 0) {
        return &s_vcdec_h264_test_stream_256x128;
    }

    return NULL;
}

static void vcdec_h264_test_log_usage(void)
{
    const vcdec_h264_test_stream_t *default_stream = vcdec_h264_test_get_default_stream_cfg();

    LOGE("usage: vcdec_h264_driver <frame|flexa> [1280x720|256x128]\r\n");
    LOGE("default stream: %s\r\n", (default_stream != NULL) ? default_stream->name : "unknown");
}

static void vcdec_h264_test_frame_done_cb(int status, void *args)
{
    vcdec_h264_test_ctx_t *ctx = (vcdec_h264_test_ctx_t *)args;

    if (ctx == NULL) {
        return;
    }

    ctx->frame_done_count++;
    ctx->last_frame_status = status;
}

static void vcdec_h264_test_flexa_done_cb(uint32_t wr_ptr, void *args)
{
    vcdec_h264_test_ctx_t *ctx = (vcdec_h264_test_ctx_t *)args;

    if (ctx == NULL) {
        return;
    }

    ctx->flexa_done_count++;
    ctx->last_wr_ptr = wr_ptr;
//    LOGI("flexa_done_cb count=%u wr_ptr=%u\r\n",
//         (unsigned)ctx->flexa_done_count,
//         (unsigned)wr_ptr);
    vcdec_h264_test_copy_flexa_segment(ctx, wr_ptr);
    vcdec_h264_set_rd_ptr(ctx->handle, wr_ptr);
}

static bk_err_t vcdec_h264_test_alloc_fb(uint32_t payload_size, frame_buffer_t **fb, uint8_t **buf)
{
    uint32_t alloc_size;
    uintptr_t frame_addr;

    if (fb == NULL || buf == NULL || payload_size == 0U) {
        return BK_FAIL;
    }

    alloc_size = payload_size + (uint32_t)sizeof(frame_buffer_t) + 63U;
    *fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, alloc_size);
    if (*fb == NULL) {
        LOGE("alloc frame buffer failed, size=%u\r\n", (unsigned)alloc_size);
        return BK_FAIL;
    }

    os_memset(*fb, 0, sizeof(frame_buffer_t));
    frame_addr = ((uintptr_t)(*fb + 1U) + 31U) & ~(uintptr_t)31U;
    (*fb)->frame = (uint8_t *)frame_addr;
    (*fb)->size = payload_size;
    *buf = (*fb)->frame;
    return BK_OK;
}

static void vcdec_h264_test_free_fb(frame_buffer_t **fb, uint8_t **buf)
{
    if (fb != NULL && *fb != NULL) {
        bk_frame_buffer_free(*fb);
        *fb = NULL;
    }
    if (buf != NULL) {
        *buf = NULL;
    }
}

static int vcdec_h264_test_find_start_code(const uint8_t *buf, uint32_t len, uint32_t offset, uint32_t *sc_off, uint32_t *sc_len)
{
    uint32_t i;

    for (i = offset; i + 3U < len; i++) {
        if (buf[i] == 0U && buf[i + 1U] == 0U) {
            if (buf[i + 2U] == 0x01U) {
                *sc_off = i;
                *sc_len = 3U;
                return 0;
            }
            if (i + 4U < len && buf[i + 2U] == 0U && buf[i + 3U] == 0x01U) {
                *sc_off = i;
                *sc_len = 4U;
                return 0;
            }
        }
    }

    return -1;
}

static int vcdec_h264_test_bs_next_byte(h264_test_bs_t *bs, uint8_t *value)
{
    while (bs->byte_pos < bs->size) {
        uint8_t byte = bs->data[bs->byte_pos++];

        if (bs->zero_count == 2U && byte == 0x03U) {
            bs->zero_count = 0;
            continue;
        }

        if (byte == 0U) {
            bs->zero_count++;
        } else {
            bs->zero_count = 0;
        }

        *value = byte;
        return 0;
    }

    return -1;
}

static int vcdec_h264_test_bs_read_bit(h264_test_bs_t *bs, uint32_t *value)
{
    if (bs->bit_pos == 0U) {
        if (vcdec_h264_test_bs_next_byte(bs, &bs->curr_byte) != 0) {
            return -1;
        }
        bs->bit_pos = 8U;
    }

    *value = (bs->curr_byte >> (bs->bit_pos - 1U)) & 0x1U;
    bs->bit_pos--;
    return 0;
}

static int vcdec_h264_test_bs_read_ue(h264_test_bs_t *bs, uint32_t *value)
{
    uint32_t zeros = 0;
    uint32_t bit = 0;
    uint32_t suffix = 0;
    uint32_t i;

    while (1) {
        if (vcdec_h264_test_bs_read_bit(bs, &bit) != 0) {
            return -1;
        }
        if (bit != 0U) {
            break;
        }
        zeros++;
        if (zeros > 31U) {
            return -1;
        }
    }

    for (i = 0; i < zeros; i++) {
        if (vcdec_h264_test_bs_read_bit(bs, &bit) != 0) {
            return -1;
        }
        suffix = (suffix << 1) | bit;
    }

    *value = ((1U << zeros) - 1U) + suffix;
    return 0;
}

static int vcdec_h264_test_parse_first_mb(const uint8_t *nal_payload, uint32_t nal_payload_size, uint32_t *first_mb_in_slice)
{
    h264_test_bs_t bs;

    if (nal_payload == NULL || nal_payload_size <= 1U || first_mb_in_slice == NULL) {
        return -1;
    }

    bs.data = nal_payload + 1U;
    bs.size = nal_payload_size - 1U;
    bs.byte_pos = 0U;
    bs.curr_byte = 0U;
    bs.bit_pos = 0U;
    bs.zero_count = 0U;

    return vcdec_h264_test_bs_read_ue(&bs, first_mb_in_slice);
}

static int vcdec_h264_test_parse_slice_type(const uint8_t *nal_payload, uint32_t nal_payload_size, uint32_t *slice_type)
{
    h264_test_bs_t bs;
    uint32_t first_mb;

    if (nal_payload == NULL || nal_payload_size <= 1U || slice_type == NULL) {
        return -1;
    }

    bs.data = nal_payload + 1U;
    bs.size = nal_payload_size - 1U;
    bs.byte_pos = 0U;
    bs.curr_byte = 0U;
    bs.bit_pos = 0U;
    bs.zero_count = 0U;

    if (vcdec_h264_test_bs_read_ue(&bs, &first_mb) != 0) {
        return -1;
    }

    return vcdec_h264_test_bs_read_ue(&bs, slice_type);
}

static int vcdec_h264_test_is_vcl(uint8_t nal_type)
{
    return (nal_type == 1U || nal_type == 2U || nal_type == 5U);
}

static vcdec_h264_frame_type_t vcdec_h264_test_peek_au_frame_type(const uint8_t *au_ptr, uint32_t au_size)
{
    uint32_t pos = 0U;
    uint32_t sc_off;
    uint32_t sc_len;

    if (au_ptr == NULL || au_size == 0U) {
        return VCDEC_H264_FRAME_P;
    }

    while (vcdec_h264_test_find_start_code(au_ptr, au_size, pos, &sc_off, &sc_len) == 0) {
        uint32_t payload_off = sc_off + sc_len;
        uint32_t next_sc_off;
        uint32_t next_sc_len;
        uint8_t nal_type;

        if (payload_off >= au_size) {
            break;
        }

        nal_type = au_ptr[payload_off] & 0x1FU;
        if (vcdec_h264_test_is_vcl(nal_type)) {
            uint32_t slice_type;

            if (nal_type == 5U) {
                return VCDEC_H264_FRAME_IDR;
            }

            if (vcdec_h264_test_find_start_code(au_ptr, au_size, payload_off, &next_sc_off, &next_sc_len) != 0) {
                next_sc_off = au_size;
            }
            (void)next_sc_len;

            if (vcdec_h264_test_parse_slice_type(&au_ptr[payload_off], next_sc_off - payload_off, &slice_type) == 0) {
                return ((slice_type % 5U) == 2U) ? VCDEC_H264_FRAME_I : VCDEC_H264_FRAME_P;
            }
            break;
        }

        pos = payload_off;
    }

    return VCDEC_H264_FRAME_P;
}

static int vcdec_h264_test_next_au(const uint8_t *stream,
                                   uint32_t stream_size,
                                   uint32_t *offset,
                                   const uint8_t **au_ptr,
                                   uint32_t *au_size)
{
    uint32_t au_start;
    uint32_t tmp_sc_len;
    uint32_t pos;
    uint8_t seen_vcl = 0U;

    if (stream == NULL || offset == NULL || au_ptr == NULL || au_size == NULL || *offset >= stream_size) {
        return -1;
    }

    if (vcdec_h264_test_find_start_code(stream, stream_size, *offset, &au_start, &tmp_sc_len) != 0) {
        return -1;
    }
    (void)tmp_sc_len;

    pos = au_start;
    while (pos < stream_size) {
        uint32_t sc_off;
        uint32_t sc_len;
        uint32_t next_sc_off;
        uint32_t next_sc_len;
        uint32_t payload_off;
        uint8_t nal_type;
        uint8_t boundary = 0U;

        if (vcdec_h264_test_find_start_code(stream, stream_size, pos, &sc_off, &sc_len) != 0) {
            break;
        }

        payload_off = sc_off + sc_len;
        if (payload_off >= stream_size) {
            break;
        }

        nal_type = stream[payload_off] & 0x1FU;
        if (vcdec_h264_test_find_start_code(stream, stream_size, payload_off, &next_sc_off, &next_sc_len) != 0) {
            next_sc_off = stream_size;
        }
        (void)next_sc_len;

        if (sc_off != au_start && seen_vcl) {
            if (vcdec_h264_test_is_vcl(nal_type)) {
                uint32_t first_mb_in_slice = 0U;

                if (vcdec_h264_test_parse_first_mb(&stream[payload_off], next_sc_off - payload_off, &first_mb_in_slice) != 0) {
                    boundary = 1U;
                } else if (first_mb_in_slice == 0U) {
                    boundary = 1U;
                }
            } else if (nal_type == 6U || nal_type == 7U || nal_type == 8U || nal_type == 9U ||
                       nal_type == 10U || nal_type == 11U || nal_type == 12U) {
                boundary = 1U;
            }
        }

        if (boundary) {
            *au_ptr = &stream[au_start];
            *au_size = sc_off - au_start;
            *offset = sc_off;
            return 0;
        }

        if (vcdec_h264_test_is_vcl(nal_type)) {
            seen_vcl = 1U;
        }

        if (next_sc_off >= stream_size) {
            break;
        }
        pos = next_sc_off;
    }

    *au_ptr = &stream[au_start];
    *au_size = stream_size - au_start;
    *offset = stream_size;
    return 0;
}

static uint32_t vcdec_h264_test_sample_signature(const uint8_t *buf, uint32_t size)
{
    uint32_t sig = 0U;
    uint32_t i;
    uint32_t step;

    if (buf == NULL || size == 0U) {
        return 0U;
    }

    step = (size > 256U) ? (size / 256U) : 1U;
    for (i = 0; i < size; i += step) {
        sig = (sig * 131U) + buf[i];
    }

    return sig;
}

static bk_err_t vcdec_h264_test_run(vcdec_h264_test_mode_t mode, const vcdec_h264_test_stream_t *stream_cfg)
{
    vcdec_handle handle = NULL;
    vcdec_h264_test_ctx_t ctx;
    vcdec_config_t cfg;
    vcdec_h264_decode_config_t dec_cfg;
    frame_buffer_t *stream_fb = NULL;
    uint8_t *stream_buf = NULL;
    frame_buffer_t *out_fb = NULL;
    uint8_t *out_buf = NULL;
    frame_buffer_t *flexa_frame_fb = NULL;
    uint8_t *flexa_frame_buf = NULL;
    uint32_t out_size;
    uint32_t frame_out_size;
    uint32_t offset = 0U;
    uint32_t au_count = 0U;
    uint32_t non_zero_signature_count = 0U;
    uint32_t idr_count = 0U;
    uint32_t p_count = 0U;
    uint8_t first_i_dumped = 0U;
    uint8_t first_p_dumped = 0U;
    const uint8_t *au_ptr = NULL;
    bk_err_t final_ret = BK_FAIL;
    uint8_t power_on = 0U;
    uint8_t int_registered = 0U;

    if (stream_cfg == NULL || stream_cfg->stream == NULL || stream_cfg->bytes == NULL || *(stream_cfg->bytes) == 0U ||
        stream_cfg->width == 0U || stream_cfg->height == 0U) {
        LOGE("invalid stream cfg\r\n");
        return BK_FAIL;
    }

    os_memset(&ctx, 0, sizeof(ctx));
    os_memset(&cfg, 0, sizeof(cfg));
    os_memset(&dec_cfg, 0, sizeof(dec_cfg));

    frame_out_size = vcdec_h264_test_frame_size(stream_cfg->width, stream_cfg->height);
    out_size = (mode == VCDEC_H264_TEST_MODE_FLEXA) ? vcdec_h264_test_flexa_out_size(stream_cfg->width) : frame_out_size;
    if (vcdec_h264_test_alloc_fb(out_size, &out_fb, &out_buf) != BK_OK) {
        return BK_FAIL;
    }
    if (mode == VCDEC_H264_TEST_MODE_FLEXA) {
        if (vcdec_h264_test_alloc_fb(frame_out_size, &flexa_frame_fb, &flexa_frame_buf) != BK_OK) {
            goto exit;
        }
    }
    if (vcdec_h264_test_alloc_fb(*(stream_cfg->bytes), &stream_fb, &stream_buf) != BK_OK) {
        goto exit;
    }
    os_memcpy(stream_buf, stream_cfg->stream, *(stream_cfg->bytes));

    if (vcdec_h264_test_power_on() != BK_OK) {
        goto exit;
    }
    power_on = 1U;
    if (vcdec_h264_test_int_register() != BK_OK) {
        goto exit;
    }
    int_registered = 1U;

    ctx.mode = mode;
    ctx.stream_cfg = stream_cfg;
    ctx.width = stream_cfg->width;
    ctx.height = stream_cfg->height;
    ctx.flexa_out_buf = out_buf;
    ctx.flexa_seg_height = VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB;
    ctx.flexa_seg_size = bk_image_size_get((uint16_t)stream_cfg->width,
                                           16U * VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB,
                                           BK_PIXEL_FORMAT_NV12);
    ctx.flexa_seg_num = VCDEC_H264_TEST_FLEXA_SEG_NUM;
    ctx.flexa_frame_buf = flexa_frame_buf;
    ctx.flexa_frame_size = frame_out_size;
    cfg.mode = (mode == VCDEC_H264_TEST_MODE_FLEXA) ? VCDEC_FLEXA_MODE_FLEXA : VCDEC_FLEXA_MODE_NONE;
    cfg.timeout_ms = 1000U;
    cfg.frame_done_cb = vcdec_h264_test_frame_done_cb;
    cfg.flexa_done_cb = (mode == VCDEC_H264_TEST_MODE_FLEXA) ? vcdec_h264_test_flexa_done_cb : NULL;
    cfg.args = &ctx;

    if (vcdec_h264_init(&handle, &cfg) != VCDEC_OK || handle == NULL) {
        LOGE("vcdec_h264_init failed\r\n");
        goto exit;
    }
    ctx.handle = handle;

    if (vcdec_h264_memalloc_register(handle, vcdec_h264_test_uncoded_malloc, vcdec_h264_test_uncoded_free) != VCDEC_OK) {
        LOGE("vcdec_h264_memalloc_register failed\r\n");
        goto exit;
    }

    if (vcdec_h264_open(handle) != VCDEC_OK) {
        LOGE("vcdec_h264_open failed\r\n");
        goto exit;
    }

    LOGI("start vcdec h264 %s test, stream=%s %ux%u bytes=%u\r\n",
         (mode == VCDEC_H264_TEST_MODE_FLEXA) ? "flexa" : "frame",
         stream_cfg->name,
         (unsigned)stream_cfg->width,
         (unsigned)stream_cfg->height,
         (unsigned)(*(stream_cfg->bytes)));

    while (vcdec_h264_test_next_au(stream_buf,
                                   *(stream_cfg->bytes),
                                   &offset,
                                   &au_ptr,
                                   &dec_cfg.input_stream_len) == 0) {
        vcdec_h264_info_t info;
#if VCDEC_H264_TEST_DUMP_ENABLE && (VCDEC_H264_TEST_DUMP_FRAME_FIRST_I || VCDEC_H264_TEST_DUMP_FRAME_FIRST_P)
        vcdec_h264_frame_type_t expected_frame_type;
#endif
        vcdec_ret_e ret;
        uint32_t sig;

        os_memset(out_buf, 0, out_size);
        dec_cfg.input_stream = (uint8_t *)au_ptr;
        dec_cfg.output_buffer = out_buf;
        dec_cfg.output_size = out_size;
        dec_cfg.segment_height = VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB;
        dec_cfg.segment_number = VCDEC_H264_TEST_FLEXA_SEG_NUM;
#if VCDEC_H264_TEST_DUMP_ENABLE && (VCDEC_H264_TEST_DUMP_FRAME_FIRST_I || VCDEC_H264_TEST_DUMP_FRAME_FIRST_P)
        expected_frame_type = vcdec_h264_test_peek_au_frame_type(au_ptr, dec_cfg.input_stream_len);
#endif
        ctx.flexa_dump_active = 0U;
        ctx.flexa_dump_kind = 0U;
        if (mode == VCDEC_H264_TEST_MODE_FLEXA) {
#if VCDEC_H264_TEST_DUMP_ENABLE && VCDEC_H264_TEST_DUMP_FRAME_FIRST_I
            if ((expected_frame_type == VCDEC_H264_FRAME_IDR || expected_frame_type == VCDEC_H264_FRAME_I) &&
                first_i_dumped == 0U) {
                ctx.flexa_dump_active = 1U;
                ctx.flexa_dump_kind = 1U;
            }
#endif
#if VCDEC_H264_TEST_DUMP_ENABLE && VCDEC_H264_TEST_DUMP_FRAME_FIRST_P
            if (expected_frame_type == VCDEC_H264_FRAME_P && first_p_dumped == 0U) {
                ctx.flexa_dump_active = 1U;
                ctx.flexa_dump_kind = 2U;
            }
#endif
            if (ctx.flexa_dump_active != 0U && flexa_frame_buf != NULL) {
                os_memset(flexa_frame_buf, 0, frame_out_size);
            }
            ctx.flexa_copy_wr_ptr = 0U;
        }

        ret = vcdec_h264_decode_frame(handle, &dec_cfg);
        if (ret != VCDEC_FRAME_READY && ret != VCDEC_OK) {
            LOGE("vcdec_h264_decode_frame failed, ret=%d au=%u size=%u\r\n",
                 (int)ret,
                 (unsigned)au_count,
                 (unsigned)dec_cfg.input_stream_len);
            goto exit;
        }

        if (vcdec_h264_get_info(handle, &info) != VCDEC_OK) {
            LOGE("vcdec_h264_get_info failed, au=%u\r\n", (unsigned)au_count);
            goto exit;
        }

        if (info.width != stream_cfg->width || info.height != stream_cfg->height) {
            LOGE("unexpected output size %ux%u, expect %ux%u\r\n",
                 (unsigned)info.width,
                 (unsigned)info.height,
                 (unsigned)stream_cfg->width,
                 (unsigned)stream_cfg->height);
            goto exit;
        }

        if (info.frame_type == VCDEC_H264_FRAME_IDR || info.frame_type == VCDEC_H264_FRAME_I) {
            idr_count++;
        } else if (info.frame_type == VCDEC_H264_FRAME_P) {
            p_count++;
        }

        if (mode == VCDEC_H264_TEST_MODE_FRAME) {
#if VCDEC_H264_TEST_DUMP_ENABLE && VCDEC_H264_TEST_DUMP_FRAME_FIRST_I
            if ((info.frame_type == VCDEC_H264_FRAME_IDR || info.frame_type == VCDEC_H264_FRAME_I) &&
                first_i_dumped == 0U) {
                first_i_dumped = 1U;
                vcdec_h264_test_dump_nv12_frame("first_i", out_buf, info.width, info.height);
            }
#endif
#if VCDEC_H264_TEST_DUMP_ENABLE && VCDEC_H264_TEST_DUMP_FRAME_FIRST_P
            if (info.frame_type == VCDEC_H264_FRAME_P && first_p_dumped == 0U) {
                first_p_dumped = 1U;
                vcdec_h264_test_dump_nv12_frame("first_p", out_buf, info.width, info.height);
            }
#endif
        }
        if (mode == VCDEC_H264_TEST_MODE_FLEXA) {
            if ((info.frame_type == VCDEC_H264_FRAME_IDR || info.frame_type == VCDEC_H264_FRAME_I) &&
                first_i_dumped == 0U) {
#if VCDEC_H264_TEST_DUMP_ENABLE && VCDEC_H264_TEST_DUMP_FRAME_FIRST_I
                if (ctx.flexa_dump_kind == 1U && flexa_frame_buf != NULL) {
                    vcdec_h264_test_dump_nv12_frame("first_i", flexa_frame_buf, info.width, info.height);
                }
#endif
                first_i_dumped = 1U;
            }
            if (info.frame_type == VCDEC_H264_FRAME_P && first_p_dumped == 0U) {
#if VCDEC_H264_TEST_DUMP_ENABLE && VCDEC_H264_TEST_DUMP_FRAME_FIRST_P
                if (ctx.flexa_dump_kind == 2U && flexa_frame_buf != NULL) {
                    vcdec_h264_test_dump_nv12_frame("first_p", flexa_frame_buf, info.width, info.height);
                }
#endif
                first_p_dumped = 1U;
            }
            ctx.flexa_dump_active = 0U;
            ctx.flexa_dump_kind = 0U;
        }

        sig = vcdec_h264_test_sample_signature(out_buf, out_size);
        if (sig != 0U) {
            non_zero_signature_count++;
        }

        LOGI("mode=%s au=%u bytes=%u type=%u ref=%u sig=0x%08x frame_cb=%u flexa_cb=%u\r\n",
             (mode == VCDEC_H264_TEST_MODE_FLEXA) ? "flexa" : "frame",
             (unsigned)(au_count + 1U),
             (unsigned)dec_cfg.input_stream_len,
             (unsigned)info.frame_type,
             (unsigned)info.is_reference,
             (unsigned)sig,
             (unsigned)ctx.frame_done_count,
             (unsigned)ctx.flexa_done_count);

        au_count++;
        if (offset >= *(stream_cfg->bytes)) {
            break;
        }
    }

    if (au_count == 0U) {
        LOGE("no access unit decoded\r\n");
        goto exit;
    }

    if (ctx.frame_done_count != au_count || ctx.last_frame_status != VCDEC_OK) {
        LOGE("frame callback mismatch count=%u au=%u last=%d\r\n",
             (unsigned)ctx.frame_done_count,
             (unsigned)au_count,
             ctx.last_frame_status);
        goto exit;
    }

    if (mode == VCDEC_H264_TEST_MODE_FLEXA && ctx.flexa_done_count == 0U) {
        LOGE("flexa callback never fired\r\n");
        goto exit;
    }

    if (non_zero_signature_count == 0U) {
        LOGE("all decoded outputs sampled as zero\r\n");
        goto exit;
    }

    LOGI("vcdec h264 %s test PASS: stream=%s aus=%u idr_or_i=%u p=%u frame_cb=%u flexa_cb=%u last_wr=%u\r\n",
         (mode == VCDEC_H264_TEST_MODE_FLEXA) ? "flexa" : "frame",
         stream_cfg->name,
         (unsigned)au_count,
         (unsigned)idr_count,
         (unsigned)p_count,
         (unsigned)ctx.frame_done_count,
         (unsigned)ctx.flexa_done_count,
         (unsigned)ctx.last_wr_ptr);
    final_ret = BK_OK;

exit:
    if (handle != NULL) {
        (void)vcdec_h264_close(handle);
        vcdec_h264_deinit(handle);
    }
    if (int_registered != 0U) {
        vcdec_h264_test_int_unregister();
    }
    if (power_on != 0U) {
        vcdec_h264_test_power_off();
    }
    vcdec_h264_test_free_fb(&stream_fb, &stream_buf);
    vcdec_h264_test_free_fb(&out_fb, &out_buf);
    vcdec_h264_test_free_fb(&flexa_frame_fb, &flexa_frame_buf);
    return final_ret;
}

static void vcdec_h264_test_thread_entry(void *arg)
{
    vcdec_h264_test_mode_t mode = VCDEC_H264_TEST_THREAD_MODE(arg);
    const vcdec_h264_test_stream_t *stream_cfg = vcdec_h264_test_get_stream_cfg(VCDEC_H264_TEST_THREAD_STREAM(arg));
    bk_err_t ret;

    ret = vcdec_h264_test_run(mode, stream_cfg);
    LOGI("vcdec_h264_test_thread exit, mode=%s stream=%s ret=%d\r\n",
         (mode == VCDEC_H264_TEST_MODE_FLEXA) ? "flexa" : "frame",
         (stream_cfg != NULL) ? stream_cfg->name : "unknown",
         (int)ret);

    s_vcdec_h264_test_running = 0U;
    s_vcdec_h264_test_thread = NULL;
    rtos_delete_thread(NULL);
}

void cli_vcdec_h264_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret;
    vcdec_h264_test_mode_t mode;
    const vcdec_h264_test_stream_t *stream_cfg;
    const char *task_name;

    if (argc != 2 && argc != 3) {
        vcdec_h264_test_log_usage();
        vcdec_h264_test_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    if (os_strcmp(argv[1], "frame") == 0) {
        mode = VCDEC_H264_TEST_MODE_FRAME;
        task_name = "vcdec_h264_frm";
    } else if (os_strcmp(argv[1], "flexa") == 0) {
        mode = VCDEC_H264_TEST_MODE_FLEXA;
        task_name = "vcdec_h264_flx";
    } else {
        vcdec_h264_test_log_usage();
        vcdec_h264_test_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    stream_cfg = vcdec_h264_test_parse_stream_arg((argc == 3) ? argv[2] : NULL);
    if (stream_cfg == NULL) {
        vcdec_h264_test_log_usage();
        vcdec_h264_test_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    if (s_vcdec_h264_test_running != 0U || s_vcdec_h264_test_thread != NULL) {
        LOGE("vcdec h264 test is already running\r\n");
        vcdec_h264_test_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    s_vcdec_h264_test_running = 1U;
    LOGI("create vcdec h264 test thread, mode=%s stream=%s %ux%u\r\n",
         (mode == VCDEC_H264_TEST_MODE_FLEXA) ? "flexa" : "frame",
         stream_cfg->name,
         (unsigned)stream_cfg->width,
         (unsigned)stream_cfg->height);
    ret = rtos_create_thread(&s_vcdec_h264_test_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             task_name,
                             (beken_thread_function_t)vcdec_h264_test_thread_entry,
                             VCDEC_H264_TEST_STACK_SIZE,
                             (beken_thread_arg_t)VCDEC_H264_TEST_THREAD_ARG(mode, stream_cfg->id));
    if (ret != BK_OK) {
        LOGE("create vcdec h264 test thread failed, ret=%d\r\n", (int)ret);
        s_vcdec_h264_test_running = 0U;
        s_vcdec_h264_test_thread = NULL;
        vcdec_h264_test_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    vcdec_h264_test_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
}
