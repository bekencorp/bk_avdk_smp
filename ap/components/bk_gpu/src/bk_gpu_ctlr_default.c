#include <os/os.h>
#include <os/mem.h>
#include <components/bk_gpu_ctlr.h>
#include <components/bk_hardware_ram.h>
#include "avdk_monitor.h"
#include "gpu_vn_ctlr.h"
#include "gpu_core.h"
#include "sys_driver.h"
#include <bk_flexa_bond_types.h>
#include "soc/reg_base.h"   /* SOC_SRAM_PERI_ADDR: GPU flexa 总线只能访问 0x28 SRAM 别名 */

#define TAG "bk_gpu_ctlr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...)

#define GPU_HIGHT_ALIGNMENT (0xF)
#define GPU_Y_VADDR_BASE    0x38200000

/* Buffer alignment constants */
#define BUFFER_ALIGNMENT_MASK     0x3F        /* 64-byte alignment */
#define BUFFER_ALIGNMENT_SIZE     64

#ifndef MEM_CACHABLE_MASK
#define MEM_CACHABLE_MASK 0x00000000
#endif

#define HDMA_OPEN_ISR_ENABLE 1

#define GPU_HPDMA_TRANSFER_TIMEOUT_MS 3000

/* Worker doorbell wait poll period. Acts as a safety net: even if a wakeup post
 * is ever missed/consumed, the worker re-checks flexa_stop at least this often,
 * so shutdown can never hang. */
#define GPU_WORKER_POLL_MS 100

static void gpu_flexa_addr_mapping(uint16_t width, uint16_t height, uint32_t base_addr, uint16_t flexa_lines, uint8_t buf_cnt)
{
    /*
     * base_addr 来自 ISP MP 通道的 y_addr(0x2Cxxxxxx CPU 直访别名)，
     * GPU flexa 作为总线 master 只能访问 0x28xxxxxx 外设别名，需转换；
     * pica/picb 用的 GPU_Y_VADDR_BASE 不在 SRAM 别名段，宏会原样透传。
     */
    base_addr = SOC_SRAM_PERI_ADDR(base_addr);
    sys_hal_set_gpu_buffa_enable_value(1);
    sys_hal_set_gpu_buffa_begin_value(base_addr);
    sys_hal_set_gpu_buffa_size_value(width * buf_cnt * flexa_lines);
    sys_hal_set_gpu_pica_begin_value(GPU_Y_VADDR_BASE);
    sys_hal_set_gpu_pica_halfbuff_end_value(GPU_Y_VADDR_BASE + width * flexa_lines * buf_cnt / 2);
    sys_hal_set_gpu_pica_end_value(GPU_Y_VADDR_BASE + width * height);

    sys_hal_set_gpu_buffb_enable_value(1);
    sys_hal_set_gpu_buffb_begin_value(base_addr + width * flexa_lines * buf_cnt);
    sys_hal_set_gpu_buffb_size_value(width * buf_cnt * flexa_lines / 2);
    sys_hal_set_gpu_picb_begin_value(GPU_Y_VADDR_BASE + width * height);
    sys_hal_set_gpu_picb_halfbuff_end_value(GPU_Y_VADDR_BASE + width * height + width * flexa_lines * buf_cnt / 4);
    sys_hal_set_gpu_picb_end_value(GPU_Y_VADDR_BASE + width * height + width * height / 2);
}

static void gpu_flexa_addr_unmapping(void)
{
    sys_hal_set_gpu_buffa_enable_value(0);
    sys_hal_set_gpu_buffa_begin_value(0);
    sys_hal_set_gpu_buffa_size_value(0);
    sys_hal_set_gpu_pica_begin_value(0);
    sys_hal_set_gpu_pica_halfbuff_end_value(0);
    sys_hal_set_gpu_pica_end_value(0);

    sys_hal_set_gpu_buffb_enable_value(0);
    sys_hal_set_gpu_buffb_begin_value(0);
    sys_hal_set_gpu_buffb_size_value(0);
    sys_hal_set_gpu_picb_begin_value(0);
    sys_hal_set_gpu_picb_halfbuff_end_value(0);
    sys_hal_set_gpu_picb_end_value(0);
}

vg_lite_buffer_format_t gpu_format_convert(bk_pixel_format_t bk_format)
{
    /* Default to unsupported format, then override supported cases only. */
    vg_lite_buffer_format_t vg_format = (vg_lite_buffer_format_t)-1;

    switch (bk_format)
    {
        case BK_PIXEL_FORMAT_RGB565:
            vg_format = VG_LITE_BGR565;
            break;
        case BK_PIXEL_FORMAT_BGR565:
            vg_format = VG_LITE_RGB565;
            break;
        case BK_PIXEL_FORMAT_RGB888:
            vg_format = VG_LITE_BGR888;
            break;
        case BK_PIXEL_FORMAT_BGR888:
            vg_format = VG_LITE_RGB888;
            break;

        case BK_PIXEL_FORMAT_ARGB8888:
            vg_format = VG_LITE_BGRA8888;
            break;
        case BK_PIXEL_FORMAT_ABGR8888:
            vg_format = VG_LITE_RGBA8888;
            break;
        case BK_PIXEL_FORMAT_RGBA8888:
            vg_format = VG_LITE_ABGR8888;
            break;
        case BK_PIXEL_FORMAT_BGRA8888:
            vg_format = VG_LITE_ARGB8888;
            break;

        case BK_PIXEL_FORMAT_NV12:
            vg_format = VG_LITE_NV12;
            break;
        case BK_PIXEL_FORMAT_YUYV:
            vg_format = VG_LITE_YUYV;
            break;
        default:
            break;
    }

    return vg_format;
}

static vg_lite_buffer_format_t gpu_blit_format_convert(bk_pixel_format_t bk_format)
{
    if (bk_format == BK_PIXEL_FORMAT_BGRA8888) {
        /*
         * The ISP SP path uses this public format as 32bpp BGRX. When a
         * rotated blit forces an alpha-aware path in VG-Lite, treating X as A
         * can make the overlay fully transparent.
         */
        return VG_LITE_BGRX8888;
    }

    return gpu_format_convert(bk_format);
}

static void *gpu_blit_uv_plane_get(const bk_gpu_blit_config_t *blit_config, void *front_frame)
{
    if (blit_config == NULL || front_frame == NULL) {
        return NULL;
    }

    if (blit_config->src_format == BK_PIXEL_FORMAT_NV12) {
        return (uint8_t *)front_frame +
               ((uint32_t)blit_config->src_width * (uint32_t)blit_config->src_height);
    }

    return NULL;
}

#define CHECK_ERROR
static bool gpu_blit_rotate_degree_is_valid(uint16_t rotate_degree);

static avdk_err_t gpu_blit_set(bk_gpu_ctlr_handle_t handle, void *src_buffer, bk_gpu_blit_config_t *blit_config)
{
    gpu_vn_ctlr_t *controller =  __containerof(handle, gpu_vn_ctlr_t, ops);

    if (blit_config == NULL) {
        LOGW("%s %d blit config is NULL\r\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }
    if (!gpu_blit_rotate_degree_is_valid(blit_config->rotate_degree)) {
        LOGW("%s %d unsupported blit rotate %u\r\n",
             __func__, __LINE__, (unsigned)blit_config->rotate_degree);
        return AVDK_ERR_INVAL;
    }

    rtos_lock_mutex(&controller->blit_mutex);
    controller->update_blit_buffer = src_buffer;
    os_memcpy(&controller->update_blit_config, blit_config, sizeof(bk_gpu_blit_config_t));
    controller->blit_enable = true;
    LOGI("%s, %d, blit_enable %d\n", __func__, __LINE__, controller->blit_enable);
    rtos_unlock_mutex(&controller->blit_mutex);

    return 0;
}

static avdk_err_t gpu_blit_clear(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_t *controller =  __containerof(handle, gpu_vn_ctlr_t, ops);

    rtos_lock_mutex(&controller->blit_mutex);

    if (controller->update_blit_buffer)
    {
        controller->update_blit_config.free(controller->update_blit_buffer, controller->update_blit_config.args);
        controller->update_blit_buffer = NULL;
        os_memset(&controller->update_blit_config, 0, sizeof(bk_gpu_blit_config_t));
    }

    if (controller->display_blit_buffer)
    {
        controller->display_blit_config.free(controller->display_blit_buffer, controller->display_blit_config.args);
        controller->display_blit_buffer = NULL;
        os_memset(&controller->display_blit_config, 0, sizeof(bk_gpu_blit_config_t));
    }

    controller->blit_enable = false;

    rtos_unlock_mutex(&controller->blit_mutex);

    return 0;
}

static avdk_err_t gpu_draw_path_build(bk_gpu_ctlr_handle_t handle, bk_gpu_draw_path_set_t *path_set)
{
    gpu_vn_ctlr_t *controller =  __containerof(handle, gpu_vn_ctlr_t, ops);
    gpu_flex_data_t *flex = &controller->flex;

    rtos_lock_mutex(&flex->draw_mutex);

    CHECK_ERROR(vg_lite_clear_path(&flex->draw_path));

    memset(&flex->draw_path, 0, sizeof(vg_lite_path_t));

	vg_lite_init_path(&flex->draw_path, VG_LITE_S16, VG_LITE_HIGH, vg_lite_get_path_length(path_set->cmd, path_set->size, VG_LITE_S16), NULL, 0, 0, 0, 0);

	CHECK_ERROR(vg_lite_append_path(&flex->draw_path, path_set->cmd, path_set->data, path_set->size));
	CHECK_ERROR(vg_lite_set_stroke(&flex->draw_path, VG_LITE_CAP_ROUND, VG_LITE_JOIN_MITER, 4, 8, NULL, 0, 8, path_set->color));
	CHECK_ERROR(vg_lite_update_stroke(&flex->draw_path));
	CHECK_ERROR(vg_lite_set_path_type(&flex->draw_path,VG_LITE_DRAW_STROKE_PATH));

    controller->flex.draw_enable = 1;

    rtos_unlock_mutex(&flex->draw_mutex);

    return 0;
}

static avdk_err_t gpu_draw_path_clear(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_t *controller =  __containerof(handle, gpu_vn_ctlr_t, ops);
    gpu_flex_data_t *flex = &controller->flex;

    rtos_lock_mutex(&flex->draw_mutex);
    controller->flex.draw_enable = 0;
    CHECK_ERROR(vg_lite_clear_path(&flex->draw_path));
    rtos_unlock_mutex(&flex->draw_mutex);
    return 0;
}

static int gpu_draw_path_process(vg_lite_matrix_t *matrix, vg_lite_path_t *path, vg_lite_buffer_t* buffer)
{
	CHECK_ERROR(vg_lite_draw(buffer, path, VG_LITE_FILL_EVEN_ODD, matrix, VG_LITE_BLEND_NONE, 0xFFFFFFFF));
    CHECK_ERROR(vg_lite_finish());
    return 0;
}

static bool gpu_blit_rotate_degree_is_valid(uint16_t rotate_degree)
{
    return rotate_degree == 0 || rotate_degree == 90 ||
           rotate_degree == 180 || rotate_degree == 270;
}

static void gpu_flexa_event_ready_handle(uint32_t frame_seq, uint32_t line, gpu_vn_ctlr_t *gpu_vn_ctlr)
{
    if (gpu_vn_ctlr == NULL) {
        return;
    }

    if (gpu_vn_ctlr->flexa_stop) {
        return;
    }

    if (line == 1)
    {
        if (gpu_vn_ctlr->line_err_flag)
        {
            gpu_vn_ctlr->line_err_flag = 0;
            gpu_vn_ctlr->flexa_abort_notified = false;
        }
    }

    gpu_vn_ctlr->line_cnt = line;
    gpu_vn_ctlr->line_frame_seq = frame_seq;
    if (gpu_vn_ctlr->gpu_process_sem)
    {
        rtos_set_semaphore(&gpu_vn_ctlr->gpu_process_sem);
    }
}

static void gpu_isp_line_done_handle(uint32_t line, void *arg)
{
    gpu_vn_ctlr_t *gpu_vn_ctlr = (gpu_vn_ctlr_t *)arg;
    uint32_t frame_seq;

    if (gpu_vn_ctlr == NULL) {
        return;
    }

    frame_seq = gpu_vn_ctlr->line_frame_seq;
    if ((line == 1) || ((gpu_vn_ctlr->line_cnt != 0) && (line < gpu_vn_ctlr->line_cnt))) {
        frame_seq++;
    }
    if (frame_seq == 0) {
        frame_seq = 1;
    }

    gpu_flexa_event_ready_handle(frame_seq, line, gpu_vn_ctlr);
}

/**
 * @brief Align buffer address to 64-byte boundary and set cacheable mask
 * @param addr Raw buffer address
 * @return Aligned and cacheable address
 */
static inline uintptr_t align_buffer_address(uintptr_t addr)
{
    LOGI("%s, %d, addr %x\n", __func__, __LINE__, addr);
    return (((addr + BUFFER_ALIGNMENT_MASK) | MEM_CACHABLE_MASK) & ~BUFFER_ALIGNMENT_MASK);
}

/**
 * @brief Initialize ping-pong buffer for GPU processing
 * @param data GPU flex data structure
 * @return 0 on success, -1 on failure
 */
static int gpu_flex_init_pingpong_buffer(gpu_flex_data_t *data)
{
    /* Allocate extra bytes so each ping/pong start can be aligned to 64 bytes. */
    uint32_t pingpong_size = data->output_width_x_flexa_lines * 2 + BUFFER_ALIGNMENT_SIZE;
    uintptr_t base_addr = (uintptr_t)bk_get_gpu_output_buffer(pingpong_size);
    if (base_addr == 0)
    {
        LOGE("%s, %d bk_get_gpu_output_buffer failed, size=%u\n", __func__, __LINE__, pingpong_size);
        return -1;
    }

    /* Keep the original allocation pointer for deinit; buffers[] stores aligned addresses only. */
    data->pingpong_raw_addr = base_addr;
    os_memset((void *)base_addr, 0, pingpong_size);
    data->buffers[0] = align_buffer_address(base_addr);
    data->buffers[1] = align_buffer_address(base_addr + data->output_width_x_flexa_lines);

    return 0;
}

static void gpu_flex_deinit_pingpong_buffer(gpu_flex_data_t *data)
{
    if (data == NULL)
    {
        LOGE("%s, %d data is NULL\n", __func__, __LINE__);
        return;
    }

    if (data->pingpong_raw_addr != 0)
    {
        /* Free the original non-aligned address returned by allocator. */
        hsram_free((void *)data->pingpong_raw_addr);
        data->pingpong_raw_addr = 0;
    }

    data->buffers[0] = 0;
    data->buffers[1] = 0;
}
/**
 * @brief Configure destination buffer based on rotation angle
 * @param data GPU flex data structure
 * @param rotation_degree Rotation angle (0, 90 or 270)
 */
static void gpu_flex_configure_dst_buffer(gpu_flex_data_t *data, bk_gpu_ctlr_config_t *config)
{
    memset(&data->dst_buf, 0, sizeof(vg_lite_buffer_t));

    if (config->rotate_degree == 90 || config->rotate_degree == 270)
    {
        data->dst_buf.width  = config->flexa_lines;
        data->dst_buf.height = data->output_width;
    }
    else /* config->rotate_degree == 0 */
    {
        data->dst_buf.width  = data->output_width;
        data->dst_buf.height = config->flexa_lines;
    }

    data->dst_buf.compress_mode = config->compress ? VG_LITE_DEC_HV_SAMPLE : VG_LITE_DEC_DISABLE;
    data->dst_buf.format = gpu_format_convert(config->dst_format);

    data->dst_buf.tiled = config->compress == true ? VG_LITE_TILED : VG_LITE_LINEAR;
    data->dst_buf.screen_copy = config->compress ? 1 : 0;
    vg_lite_allocate_with_data(&data->dst_buf, (void *)(uintptr_t)data->buffers[data->dst_buf_idx], NULL, NULL, NULL);
}

static void gpu_flex_hpdma_link_transfer_complete_callback(hpdma_id_t hpdma_id, void *user_data)
{
    HPDMA_LINE_END();
    if (user_data != NULL) {
        beken_semaphore_t *sem_ptr = (beken_semaphore_t *)user_data;
        rtos_set_semaphore(sem_ptr);
    }
}

/**
 * @brief Initialize DMA for rotation operations
 * @param data GPU flex data structure
 * @param rotation_degree Rotation angle
 */
static void gpu_flex_init_dma(gpu_flex_data_t *data)
{
    uint32_t link_cnt = 1;

    data->link_dma_list_table = bk_hpdma_link_init(link_cnt);
    if (data->link_dma_list_table == NULL)
    {
        LOGE("%s, %d bk_hpdma_link_init failed, link_cnt=%d\n", __func__, __LINE__, link_cnt);
        return;
    }

    data->gdma = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (data->gdma >= HPDMA_ID_MAX)
    {
        LOGE("%s, %d bk_hpdma_alloc failed\n", __func__, __LINE__);
        bk_hpdma_link_deinit(data->link_dma_list_table);
        data->link_dma_list_table = NULL;
        return;
    }
    /*
     * P1 (HPDMA review): use HPDMA_BURST_LEN_INC16 enum; the SMEM-same-
     *   block downgrade to INC8 (if applicable) is now applied inside
     *   the driver at start time, removing the need for callers to
     *   second-guess the burst length.
     */
    bk_hpdma_set_dest_burst_len(data->gdma, HPDMA_BURST_LEN_INC16);
    bk_hpdma_set_src_burst_len(data->gdma, HPDMA_BURST_LEN_INC16);
#if HDMA_OPEN_ISR_ENABLE
    BK_LOG_ON_ERR(bk_hpdma_register_isr(data->gdma, NULL, NULL, gpu_flex_hpdma_link_transfer_complete_callback, &data->transfer_sem));
    BK_LOG_ON_ERR(bk_hpdma_enable_finish_interrupt(data->gdma));
#endif
}

static void gpu_flex_update_horizontal_mirror_matrix(gpu_flex_data_t *data,
                                                     const bk_gpu_ctlr_config_t *config)
{
    if (config->rotate_degree != 0 &&
        config->rotate_degree != 90 &&
        config->rotate_degree != 180 &&
        config->rotate_degree != 270)
    {
        return;
    }

    float mirror_scale_x = config->scale ? data->scale_x : 1.0f;
    float mirror_scale_y = config->scale ? data->scale_y : 1.0f;
    float draw_scale_x = 1.0f;
    float draw_scale_y = 1.0f;
    float offset_x;
    float offset_y;

    if (config->rotate_degree == 0)
    {
        mirror_scale_x = -mirror_scale_x;
        draw_scale_x = -draw_scale_x;
        offset_x = (float)data->output_width;
        offset_y = -((float)(data->flexa_index - 1) * config->flexa_lines);
    }
    else if (config->rotate_degree == 90)
    {
        mirror_scale_y = -mirror_scale_y;
        draw_scale_y = -draw_scale_y;
        offset_x = -((float)(data->flexa_index - 1) * config->flexa_lines);
        offset_y = 0.0f;
    }
    else if (config->rotate_degree == 180)
    {
        mirror_scale_x = -mirror_scale_x;
        draw_scale_x = -draw_scale_x;
        offset_x = 0.0f;
        offset_y = ((float)data->flexa_index * config->flexa_lines);
    }
    else if (config->rotate_degree == 270)
    {
        mirror_scale_y = -mirror_scale_y;
        draw_scale_y = -draw_scale_y;
        offset_x = ((float)data->flexa_index * config->flexa_lines);
        offset_y = (float)data->output_width;
    }
    else
    {
        return;
    }

    vg_lite_identity(&data->matrix);
    if (config->rotate_degree != 0)
    {
        vg_lite_rotate((float)config->rotate_degree, &data->matrix);
    }
    vg_lite_scale(mirror_scale_x, mirror_scale_y, &data->matrix);
    data->matrix.m[0][2] = offset_x;
    data->matrix.m[1][2] = offset_y;

    if (data->draw_enable)
    {
        vg_lite_identity(&data->draw_matrix);
        if (config->rotate_degree != 0)
        {
            vg_lite_rotate((float)config->rotate_degree, &data->draw_matrix);
        }
        vg_lite_scale(draw_scale_x, draw_scale_y, &data->draw_matrix);
        data->draw_matrix.m[0][2] = offset_x;
        data->draw_matrix.m[1][2] = offset_y;
    }

    return;
}

static void gpu_flex_update_strip_translate_matrix(gpu_flex_data_t *data,
                                                   const bk_gpu_ctlr_config_t *config)
{
    if (config->rotate_degree == 0)
    {
        float offset = -((float)(data->flexa_index - 1) * config->flexa_lines);
        data->matrix.m[1][2] = offset;

        if (data->draw_enable)
        {
            data->draw_matrix.m[1][2] = offset;
        }
    }
    else if (config->rotate_degree == 90)
    {
        float offset = ((float)(data->flexa_index) * config->flexa_lines);
        data->matrix.m[0][2] = offset;

        if (data->draw_enable)
        {
            data->draw_matrix.m[0][2] = offset;
        }
    }
    else if (config->rotate_degree == 180)
    {
        float offset_x = (float)data->output_width;
        float offset_y = ((float)data->flexa_index * config->flexa_lines);
        data->matrix.m[0][2] = offset_x;
        data->matrix.m[1][2] = offset_y;

        if (data->draw_enable)
        {
            data->draw_matrix.m[0][2] = offset_x;
            data->draw_matrix.m[1][2] = offset_y;
        }
    }
    else if (config->rotate_degree == 270)
    {
        float offset_x = -((float)(data->flexa_index - 1) * config->flexa_lines);
        float offset_y = (float)data->output_width;
        data->matrix.m[0][2] = offset_x;
        data->matrix.m[1][2] = offset_y;

        if (data->draw_enable)
        {
            data->draw_matrix.m[0][2] = offset_x;
            data->draw_matrix.m[1][2] = offset_y;
        }
    }
}

static void gpu_flex_update_matrix(gpu_flex_data_t *data,
                                    const bk_gpu_ctlr_config_t *config)
{
    if (config->horizontal_mirror)
    {
        gpu_flex_update_horizontal_mirror_matrix(data, config);
        return;
    }

    gpu_flex_update_strip_translate_matrix(data, config);
}

/**
 * @brief Initialize GPU flex data structure
 * @param data GPU flex data structure to initialize
 * @param gpu_vn_ctlr GPU controller handle
 */
static inline void gpu_flex_data_init(gpu_flex_data_t *data, gpu_vn_ctlr_t *gpu_vn_ctlr)
{
    bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;

    /* Initialize basic parameters */
    gpu_vn_ctlr->line_cnt = 0;
    gpu_vn_ctlr->line_frame_seq = 0;
    gpu_vn_ctlr->active_frame_seq = 0;
    gpu_vn_ctlr->flexa_frame_active = false;
    gpu_vn_ctlr->flexa_abort_notified = false;
    data->input_width = config->src_width;
    data->input_height = config->src_height;
    data->output_width = config->compress ? ((config->dst_width + GPU_HIGHT_ALIGNMENT) & ~GPU_HIGHT_ALIGNMENT) : config->dst_width;
    data->output_height = (config->dst_height + GPU_HIGHT_ALIGNMENT) & ~GPU_HIGHT_ALIGNMENT;
    data->flexa_index = 1;
    data->read_lines = 0;
    data->dst_buf_idx = 0;
    LOGI("config->compress %d, input_width %d input_height %d output_width %d output_height %d\r\n", config->compress, data->input_width, data->input_height, data->output_width, data->output_height);
    /* Cache frequently used calculations */
    data->output_width_x_flexa_lines = config->compress ? data->output_width * config->flexa_lines : bk_pixel_size_get(config->dst_format) * data->output_width * config->flexa_lines;
    data->flexa_lines_x_4 = bk_pixel_size_get(config->dst_format) * config->flexa_lines;
    data->input_width_x_height = data->input_width * data->input_height;
    data->scale_x = (float)data->output_width / (float)data->input_width;
    data->scale_y = (float)data->output_height / (float)data->input_height;

    if (rtos_init_mutex(&data->draw_mutex) != BK_OK)
    {
        LOGE("%s, %d init draw_mutex\n", __func__, __LINE__);
        return;
    }

    if (rtos_init_semaphore(&data->transfer_sem, 1) != BK_OK)
    {
        LOGE("%s, %d init transfer_sem\n", __func__, __LINE__);
        return;
    }
    rtos_set_semaphore(&data->transfer_sem);

    if (config->flexa)
    {
        if (config->src_buffer == NULL) {
            LOGE("%s, %d gpu flexa src_buffer is NULL, please set flexa addr mapping first\n", __func__, __LINE__);
        } else {
            gpu_flexa_addr_mapping(data->input_width, data->input_height,
                                            (uint32_t)config->src_buffer,
                                            config->flexa_lines,
                                            config->flexa_buff_cnt);
        }
    }

    /* Setup transformation matrix */
    vg_lite_identity(&data->matrix);
    if (config->rotate_degree != 0)
    {
        vg_lite_rotate((float)config->rotate_degree, &data->matrix);
    }

    if (config->scale)
    {
        vg_lite_scale(data->scale_x, data->scale_y, &data->matrix);
    }

    /* Initialize DMA for rotation if needed */
    gpu_flex_init_dma(data);

    /* Configure source buffer */
    data->src_buf.width = data->input_width;
    data->src_buf.height = data->input_height;
    data->src_buf.format = gpu_format_convert(config->src_format);
    data->src_buf.compress_mode = VG_LITE_DEC_DISABLE;
    data->src_buf.tiled = VG_LITE_LINEAR;
    vg_lite_allocate_with_data(&data->src_buf,
                               (void *)(uintptr_t)GPU_Y_VADDR_BASE,
                               (void *)(uintptr_t)(GPU_Y_VADDR_BASE + data->input_width_x_height),
                               NULL, NULL);

    /* Initialize ping-pong buffer */
    if (gpu_flex_init_pingpong_buffer(data) != 0)
    {
        LOGE("%s, %d gpu_flex_init_pingpong_buffer failed\n", __func__, __LINE__);
        return;
    }

    /* Configure destination buffer */
    gpu_flex_configure_dst_buffer(data, config);
}

static inline void gpu_flex_data_deinit(gpu_flex_data_t *data, gpu_vn_ctlr_t *gpu_vn_ctlr)
{
    /* Deinit only resources that were successfully initialized. */
    if (data->transfer_sem)
    {
        rtos_deinit_semaphore(&data->transfer_sem);
        data->transfer_sem = NULL;
    }

    if (data->draw_mutex)
    {
        rtos_deinit_mutex(&data->draw_mutex);
        data->draw_mutex = NULL;
    }
}

/**
 * @brief Reset GPU flex pipeline state so that next frame can start cleanly.
 *        This is typically used when the upstream JPEG decoder reports a fatal error
 *        and the current frame should be discarded.
 */
static inline void gpu_flex_restart(gpu_vn_ctlr_t *gpu_vn_ctlr)
{
    gpu_flex_data_t *flex = &gpu_vn_ctlr->flex;

    /* Mark error so main loop will skip current frame and wait for a fresh one. */
    gpu_vn_ctlr->line_err_flag = 1;
    gpu_vn_ctlr->flexa_frame_active = false;
    gpu_vn_ctlr->flexa_abort_notified = false;

    /* Reset per-frame counters; next frame will start from index 1. */
    flex->flexa_index = 1;
    flex->read_lines = 0;
}

static inline bool gpu_flex_current_block_has_padding(const gpu_flex_data_t *data, const bk_gpu_ctlr_config_t *config)
{
    if (data->output_height <= config->dst_height || (config->dst_height % config->flexa_lines) == 0)
    {
        return false;
    }

    uint16_t last_block_index = data->output_height / config->flexa_lines;

    /*
    * The aligned tail is contained in one edge block. Clear both edge
    * blocks so reused ping-pong memory cannot leak into the visible edge
    * after rotation/cropping policy changes.
    */
    return data->flexa_index == 1 || data->flexa_index == last_block_index;
}

static inline bool gpu_flex_draw_path_intersects_block(const gpu_flex_data_t *data)
{
    const vg_lite_path_t *path = &data->draw_path;
    const vg_lite_matrix_t *matrix = &data->draw_matrix;
    vg_lite_float_t x[4] = {
        path->bounding_box[0],
        path->bounding_box[2],
        path->bounding_box[2],
        path->bounding_box[0],
    };
    vg_lite_float_t y[4] = {
        path->bounding_box[1],
        path->bounding_box[1],
        path->bounding_box[3],
        path->bounding_box[3],
    };
    vg_lite_float_t min_x;
    vg_lite_float_t min_y;
    vg_lite_float_t max_x;
    vg_lite_float_t max_y;

    min_x = max_x = matrix->m[0][0] * x[0] + matrix->m[0][1] * y[0] + matrix->m[0][2];
    min_y = max_y = matrix->m[1][0] * x[0] + matrix->m[1][1] * y[0] + matrix->m[1][2];

    for (uint32_t i = 1; i < 4; i++)
    {
        vg_lite_float_t tx = matrix->m[0][0] * x[i] + matrix->m[0][1] * y[i] + matrix->m[0][2];
        vg_lite_float_t ty = matrix->m[1][0] * x[i] + matrix->m[1][1] * y[i] + matrix->m[1][2];

        if (tx < min_x) min_x = tx;
        if (tx > max_x) max_x = tx;
        if (ty < min_y) min_y = ty;
        if (ty > max_y) max_y = ty;
    }

    if (path->stroke != NULL)
    {
        vg_lite_float_t margin = path->stroke->line_width + 1.0f;

        min_x -= margin;
        min_y -= margin;
        max_x += margin;
        max_y += margin;
    }

    return (max_x > 0.0f) &&
           (max_y > 0.0f) &&
           (min_x < (vg_lite_float_t)data->dst_buf.width) &&
           (min_y < (vg_lite_float_t)data->dst_buf.height);
}

static inline void gpu_flex_abort_current_frame(gpu_vn_ctlr_t *gpu_vn_ctlr)
{
    bool notify_frame_fail = gpu_vn_ctlr->flexa_frame_active;

    gpu_flex_restart(gpu_vn_ctlr);

    if (notify_frame_fail &&
        gpu_vn_ctlr->bond != NULL &&
        gpu_vn_ctlr->bond->frame_done != NULL) {
        gpu_vn_ctlr->bond->frame_done(BK_FAIL, gpu_vn_ctlr->bond);
        gpu_vn_ctlr->flexa_abort_notified = true;
    }
}

static inline void gpu_flex_data_dma_transfer(gpu_flex_data_t *data, uint32_t offset,
                                              uint32_t xsize, uint32_t ysize, uint32_t dst_step)
{
    hpdma_link_config_t dma_config[1];

    dma_config[0].src_addr = (uint32_t)data->dst_buf.memory;
    dma_config[0].dst_addr = (uint32_t)(data->dpu_frame_buffers + offset);
    dma_config[0].src_xsize = xsize;
    dma_config[0].dst_xsize = xsize;
    dma_config[0].src_ysize = ysize;
    dma_config[0].dst_ysize = ysize;
    dma_config[0].src_step = 0;
    dma_config[0].dst_step = dst_step;
    dma_config[0].finish_int_en = 1;
    dma_config[0].half_finish_int_en = 0;
    BK_LOG_ON_ERR(bk_hpdma_link_set_descs(data->link_dma_list_table, (hpdma_link_config_t *)dma_config, 1));
    BK_LOG_ON_ERR(bk_hpdma_link_transfer(data->gdma, data->link_dma_list_table));
}

/**
 * @brief Pull out processed line data from GPU buffer
 * @param data GPU flex data structure
 * @param gpu_vn_ctlr GPU controller handle
 */
static inline bool gpu_flex_data_line_pull_out(gpu_flex_data_t *data, gpu_vn_ctlr_t *gpu_vn_ctlr)
{
    const bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;

    /* Calculate read lines */
    data->read_lines = data->need_lines / data->output_height;
#if HDMA_OPEN_ISR_ENABLE
    bk_err_t ret = rtos_get_semaphore(&data->transfer_sem, GPU_HPDMA_TRANSFER_TIMEOUT_MS);
    if (ret != BK_OK)
    {
        LOGE("%s,%d wait hpdma semaphore failed, dma_id=%d ret=%d\n", __func__, __LINE__, data->gdma, ret);
        gpu_flex_restart(gpu_vn_ctlr);
        return false;
    }
#else
    while(bk_hpdma_get_next_ll_addr(data->gdma));
    while(bk_hpdma_get_enable_status(data->gdma));
#endif
    if (gpu_vn_ctlr->flexa_stop)
    {
        return false;
    }

    /* Copy processed data based on rotation angle */
    if (config->rotate_degree == 90 || config->rotate_degree == 270)
    {
        uint32_t pixel_size = bk_pixel_size_get(config->dst_format);
        uint32_t stride = (data->output_height - config->flexa_lines) * pixel_size;
        uint32_t ysize = config->compress ? data->output_width / 4 : data->output_width;
        uint32_t offset;

        if ((config->rotate_degree == 90 && !config->horizontal_mirror) ||
            (config->rotate_degree == 270 && config->horizontal_mirror))
        {
            offset = (data->output_height - data->flexa_index * config->flexa_lines) * pixel_size;
        }
        else
        {
            offset = (data->flexa_index - 1) * data->flexa_lines_x_4;
        }

        gpu_flex_data_dma_transfer(data, offset, data->flexa_lines_x_4, ysize, stride);
    }
    else if (config->rotate_degree == 0)
    {
        uint32_t offset = (data->flexa_index - 1) * data->output_width_x_flexa_lines;
        uint32_t xsize = data->output_width * bk_pixel_size_get(config->dst_format);
        uint32_t ysize = config->compress ? config->flexa_lines / 4 : config->flexa_lines;

        gpu_flex_data_dma_transfer(data, offset, xsize, ysize, 0);
    }
    else if (config->rotate_degree == 180)
    {
        uint32_t pixel_size = bk_pixel_size_get(config->dst_format);
        uint32_t offset = (data->output_height - data->flexa_index * config->flexa_lines) *
                          (config->compress ? data->output_width : data->output_width * pixel_size);
        uint32_t xsize = data->output_width * pixel_size;
        uint32_t ysize = config->compress ? config->flexa_lines / 4 : config->flexa_lines;

        gpu_flex_data_dma_transfer(data, offset, xsize, ysize, 0);
    }

    /* Switch to next ping-pong buffer */
    data->dst_buf_idx = 1 - data->dst_buf_idx;
    /* .memory 留 0x2C 给 CPU/HPDMA(hpdma_hal 内部再转)；.address 是 GPU 渲染写入目标，需转 0x28 */
    data->dst_buf.memory = (vg_lite_pointer)(uintptr_t)data->buffers[data->dst_buf_idx];
    data->dst_buf.address = SOC_SRAM_PERI_ADDR(data->buffers[data->dst_buf_idx]);
    data->flexa_index++;

    return true;
}

static void gpu_flex_data_frame_done_blit(gpu_flex_data_t *flex, const bk_gpu_ctlr_config_t *config, bk_gpu_blit_config_t *blit_config, void *front_frame, void *display_frame)
{
    vg_lite_buffer_t display_buffer;
    vg_lite_buffer_t front_buffer;
    vg_lite_matrix_t display_matrix;

    LOGV("%s, frame_done_blit\n", __func__);

    memset(&display_buffer , 0, sizeof(display_buffer));
    memset(&front_buffer , 0, sizeof(front_buffer));

    if (config->rotate_degree == 90 || config->rotate_degree == 270)
    {
        display_buffer.width = flex->output_height;
        display_buffer.height = flex->output_width;
    }
    else
    {
        display_buffer.width = flex->output_width;
        display_buffer.height = flex->output_height;
    }

    display_buffer.format = VG_LITE_BGRA8888;
    display_buffer.tiled = VG_LITE_TILED;
    display_buffer.compress_mode = VG_LITE_DEC_HV_SAMPLE;

    memset(&front_buffer , 0, sizeof(front_buffer));
    front_buffer.width = blit_config->src_width;
    front_buffer.height = blit_config->src_height;
    front_buffer.format = gpu_blit_format_convert(blit_config->src_format);
    vg_lite_allocate_with_data(&front_buffer,
                               front_frame,
                               gpu_blit_uv_plane_get(blit_config, front_frame),
                               NULL,
                               NULL);

    memset(&display_matrix , 0, sizeof(display_matrix));
    vg_lite_identity(&display_matrix);
    vg_lite_allocate_with_data(&display_buffer, display_frame, NULL, NULL, NULL);

    vg_lite_rectangle_t rect = {
        .x = blit_config->src_x,
        .y = blit_config->src_y,
        .width = blit_config->src_width,
        .height = blit_config->src_height,
    };

    switch (blit_config->rotate_degree)
    {
        case 90:
            vg_lite_rotate(90.0f, &display_matrix);
            display_matrix.m[0][2] = (vg_lite_float_t)blit_config->dst_x +
                                     (vg_lite_float_t)blit_config->src_height;
            display_matrix.m[1][2] = (vg_lite_float_t)blit_config->dst_y;
            break;
        case 180:
            vg_lite_rotate(180.0f, &display_matrix);
            display_matrix.m[0][2] = (vg_lite_float_t)blit_config->dst_x +
                                     (vg_lite_float_t)blit_config->src_width;
            display_matrix.m[1][2] = (vg_lite_float_t)blit_config->dst_y +
                                     (vg_lite_float_t)blit_config->src_height;
            break;
        case 270:
            vg_lite_rotate(270.0f, &display_matrix);
            display_matrix.m[0][2] = (vg_lite_float_t)blit_config->dst_x;
            display_matrix.m[1][2] = (vg_lite_float_t)blit_config->dst_y +
                                     (vg_lite_float_t)blit_config->src_width;
            break;
        case 0:
        default:
            vg_lite_translate(blit_config->dst_x, blit_config->dst_y, &display_matrix);
            break;
    }

    int ret = vg_lite_blit_rect(
        &display_buffer,
        &front_buffer,
        &rect,
        &display_matrix,
        VG_LITE_BLEND_NONE,
        0,
        VG_LITE_FILTER_POINT
    );

    if (ret != VG_LITE_SUCCESS)
    {
        LOGE("vg_lite_blit_rect failed, ret %d\r\n", ret);
    }

    vg_lite_finish();

    vg_lite_free_without_free_data(&display_buffer);
    vg_lite_free_without_free_data(&front_buffer);
}

/**
 * @brief Handle frame completion
 * @param data GPU flex data structure
 * @param gpu_vn_ctlr GPU controller handle
 */
static inline bool gpu_flex_data_frame_done(gpu_flex_data_t *data, gpu_vn_ctlr_t *gpu_vn_ctlr)
{
    const bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;

#if HDMA_OPEN_ISR_ENABLE
    bk_err_t ret = rtos_get_semaphore(&data->transfer_sem, GPU_HPDMA_TRANSFER_TIMEOUT_MS);
    if (ret != BK_OK)
    {
        LOGE("%s,%d wait hpdma semaphore failed, dma_id=%d ret=%d\n", __func__, __LINE__, data->gdma, ret);
        gpu_flex_restart(gpu_vn_ctlr);
        return false;
    }
#else
    while(bk_hpdma_get_next_ll_addr(data->gdma));
    while(bk_hpdma_get_enable_status(data->gdma));
#endif
    rtos_lock_mutex(&gpu_vn_ctlr->blit_mutex);

    if (gpu_vn_ctlr->blit_enable)
    {
        LOGV("%s, check blit\n", __func__);

        if (gpu_vn_ctlr->display_blit_buffer && gpu_vn_ctlr->update_blit_buffer)
        {
            gpu_vn_ctlr->display_blit_config.free(gpu_vn_ctlr->display_blit_buffer, gpu_vn_ctlr->display_blit_config.args);
            gpu_vn_ctlr->display_blit_buffer = NULL;
            os_memset(&gpu_vn_ctlr->display_blit_config, 0, sizeof(bk_gpu_blit_config_t));
        }

        if (gpu_vn_ctlr->update_blit_buffer)
        {
            gpu_vn_ctlr->display_blit_buffer = gpu_vn_ctlr->update_blit_buffer;
            os_memcpy(&gpu_vn_ctlr->display_blit_config, &gpu_vn_ctlr->update_blit_config, sizeof(bk_gpu_blit_config_t));
            gpu_vn_ctlr->update_blit_buffer = NULL;
        }

        if (gpu_vn_ctlr->display_blit_buffer)
        {
            rtos_lock_mutex(&gpu_vn_ctlr->gpu_mutex);
            gpu_flex_data_frame_done_blit(data,
                                          config,
                                          &gpu_vn_ctlr->display_blit_config,
                                          gpu_vn_ctlr->display_blit_buffer,
                                          data->dpu_frame_buffers);
            rtos_unlock_mutex(&gpu_vn_ctlr->gpu_mutex);
        }
    }

    rtos_unlock_mutex(&gpu_vn_ctlr->blit_mutex);

    /* Swap frame buffer */
    uint32_t frame_size = bk_pixel_size_get(config->dst_format) * (config->compress ? data->output_width / 4 : data->output_width) * data->output_height;
    void *new_buffer = config->frame_malloc(frame_size);
    //void *new_buffer = NULL;
    if (new_buffer)
    {
        void *done_frame = data->dpu_frame_buffers;
        data->dpu_frame_buffers = new_buffer;
        AVDK_MONITOR_GPU_FRAME_PLUS();

        if (config->frame_done)
        {
            config->frame_done(done_frame, frame_size, config->frame_done_args);
        }
    }

    if (gpu_vn_ctlr->bond != NULL && gpu_vn_ctlr->bond->frame_done != NULL) {
        gpu_vn_ctlr->bond->frame_done(BK_OK, gpu_vn_ctlr->bond);
    }
    /* Reset state for next frame.
     * Drain a possibly-leftover doorbell post that belongs to the just-finished
     * frame's final line, so the worker does not spuriously wake for a frame
     * that already completed (which would emit a stray "waits frame start" and a
     * stale bond->flexa_done()). The condition only fires when the latest seen
     * event is this frame's last line, so a new frame's line==1 post
     * (line_cnt != last) is never consumed. */
    uint32_t frame_last_line_count = (data->input_height + config->flexa_lines - 1) / config->flexa_lines;
    if ((gpu_vn_ctlr->line_frame_seq == gpu_vn_ctlr->active_frame_seq) &&
        (gpu_vn_ctlr->line_cnt == frame_last_line_count))
    {
        rtos_get_semaphore(&gpu_vn_ctlr->gpu_process_sem, BEKEN_NO_WAIT);
    }
    data->flexa_index = 1;
    data->read_lines = 0;
    gpu_vn_ctlr->flexa_frame_active = false;

#if HDMA_OPEN_ISR_ENABLE
    rtos_set_semaphore(&data->transfer_sem);
#endif

    return true;
}

/**
 * @brief Check if enough ISP lines are available for processing
 * @param data GPU flex data structure
 * @param isp_line_count Current ISP line count
 * @return 1 if enough lines available, 0 otherwise
 */
static inline int gpu_flex_has_enough_lines(gpu_flex_data_t *data, uint32_t src_line_count, uint16_t flexa_lines)
{
    data->need_lines = data->flexa_index * flexa_lines * data->input_height;
    uint32_t src_lines = src_line_count * flexa_lines;
    uint32_t required_lines = (data->need_lines + data->output_height - 1) / data->output_height;

    return (src_lines >= required_lines) ? 1 : 0;
}

static inline bool gpu_flex_frame_abort_needed(gpu_flex_data_t *data,
                                               gpu_vn_ctlr_t *gpu_vn_ctlr,
                                               uint32_t frame_seq)
{
    const bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;
    uint32_t current_src_lines;
    uint32_t buffered_lines;

    if (gpu_vn_ctlr->line_frame_seq != frame_seq) {
        LOGW("%s, flexa frame changed, active %u latest %u latest_line %u\n",
             __func__,
             frame_seq,
             gpu_vn_ctlr->line_frame_seq,
             gpu_vn_ctlr->line_cnt);
        return true;
    }

    current_src_lines = gpu_vn_ctlr->line_cnt * config->flexa_lines;
    if (current_src_lines > data->input_height) {
        current_src_lines = data->input_height;
    }
    buffered_lines = data->read_lines + config->flexa_lines * config->flexa_buff_cnt;
    if (current_src_lines > buffered_lines)
    {
        uint32_t overrun_lines = current_src_lines - buffered_lines;

        LOGW("%s, flexa overrun, frame %u, line_cnt %u, buff_cnt %u, overrun %u\n",
             __func__,
             frame_seq,
             gpu_vn_ctlr->line_cnt,
             config->flexa_buff_cnt,
             overrun_lines);
        return true;
    }

    return false;
}

/**
 * @brief Process a single line block with GPU
 * @param data GPU flex data structure
 * @param gpu_vn_ctlr GPU controller handle
 */
static bool gpu_flex_process_line_block(gpu_flex_data_t *data,
                                        gpu_vn_ctlr_t *gpu_vn_ctlr,
                                        uint32_t frame_seq)
{
    const bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;
    GPU_LINE_START();
    /* Update transformation matrix for current line */
    gpu_flex_update_matrix(data, config);

    rtos_lock_mutex(&gpu_vn_ctlr->gpu_mutex);

    if (gpu_flex_frame_abort_needed(data, gpu_vn_ctlr, frame_seq))
    {
        gpu_flex_abort_current_frame(gpu_vn_ctlr);
        GPU_LINE_END();
        rtos_unlock_mutex(&gpu_vn_ctlr->gpu_mutex);
        return false;
    }

    if (gpu_flex_current_block_has_padding(data, config))
    {
        vg_lite_clear(&data->dst_buf, NULL, 0x00000000);
    }

    /* Perform GPU blit operation */
    vg_lite_blit(&data->dst_buf, &data->src_buf, &data->matrix,
                 VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
    vg_lite_finish();

    /* Draw path changes are rare; avoid taking draw_mutex on every block when disabled. */
    if (data->draw_enable)
    {
        rtos_lock_mutex(&data->draw_mutex);
        if (data->draw_enable && gpu_flex_draw_path_intersects_block(data))
        {
            gpu_draw_path_process(&data->draw_matrix, &data->draw_path, &data->dst_buf);
        }
        rtos_unlock_mutex(&data->draw_mutex);
    }

    if (gpu_flex_frame_abort_needed(data, gpu_vn_ctlr, frame_seq))
    {
        gpu_flex_abort_current_frame(gpu_vn_ctlr);
        GPU_LINE_END();
        rtos_unlock_mutex(&gpu_vn_ctlr->gpu_mutex);
        return false;
    }
    GPU_LINE_END();
    rtos_unlock_mutex(&gpu_vn_ctlr->gpu_mutex);
    HPDMA_LINE_START();
    /* Pull out processed line data */
    return gpu_flex_data_line_pull_out(data, gpu_vn_ctlr);
}

/**
 * @brief Main GPU flex processing entry point
 * @param arg GPU controller handle (gpu_vn_ctlr_t *)
 */
static void gpu_flex_main_entry(void *arg)
{
    LOGI("%s %p\n", __func__, arg);

    gpu_vn_ctlr_t *gpu_vn_ctlr = (gpu_vn_ctlr_t *)arg;
    const bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;
    gpu_flex_data_t *flex = &gpu_vn_ctlr->flex;

    bk_err_t ret = rtos_set_semaphore(&gpu_vn_ctlr->gpu_start_sem);
    if (ret != BK_OK) {
        LOGE("%s, %d rtos_set_semaphore gpu_start_sem failed\n", __func__, __LINE__);
        goto thread_exit;
    }

    /* Main processing loop */
    while (1)
    {
        bk_err_t proc_ret = rtos_get_semaphore(&gpu_vn_ctlr->gpu_process_sem, GPU_WORKER_POLL_MS);

        if (gpu_vn_ctlr->flexa_stop)
        {
            break;
        }

        if (proc_ret != BK_OK)
        {
            continue;
        }

        uint32_t src_line_count = gpu_vn_ctlr->line_cnt;
        uint32_t src_frame_seq = gpu_vn_ctlr->line_frame_seq;
        bool can_sync_frame = (src_line_count == 1) ||
                              (!gpu_vn_ctlr->flexa_frame_active &&
                               (src_line_count > 0) &&
                               (src_line_count <= config->flexa_buff_cnt));
        if (can_sync_frame)
        {
            if (src_line_count != 1) {
                LOGW("%s, flexa sync from early block, frame %u line %u\n",
                     __func__, src_frame_seq, src_line_count);
            }
            else if (gpu_vn_ctlr->flexa_frame_active &&
                     (flex->read_lines > 0) &&
                     (flex->read_lines < flex->input_height) &&
                     gpu_vn_ctlr->bond != NULL &&
                     gpu_vn_ctlr->bond->frame_done != NULL) {
                gpu_vn_ctlr->bond->frame_done(BK_FAIL, gpu_vn_ctlr->bond);
            }
            flex->flexa_index = 1;
            flex->read_lines = 0;
            gpu_vn_ctlr->line_err_flag = 0;
            gpu_vn_ctlr->flexa_abort_notified = false;
            gpu_vn_ctlr->active_frame_seq = src_frame_seq;
            gpu_vn_ctlr->flexa_frame_active = true;
            GPU_FRAME_START();
        }

        if (!gpu_vn_ctlr->line_err_flag)
        {
            if (!gpu_vn_ctlr->flexa_frame_active)
            {
                LOGW("%s, flexa waits frame start, frame %u line %u\n",
                     __func__, src_frame_seq, src_line_count);
                gpu_flex_restart(gpu_vn_ctlr);
            }
            else if (src_frame_seq != gpu_vn_ctlr->active_frame_seq)
            {
                LOGW("%s, flexa frame mismatch, active %u event %u line %u\n",
                     __func__,
                     gpu_vn_ctlr->active_frame_seq,
                     src_frame_seq,
                     src_line_count);
                gpu_flex_abort_current_frame(gpu_vn_ctlr);
            }
        }

        /* Handle line error flag */
        if (gpu_vn_ctlr->line_err_flag)
        {
            if (!gpu_vn_ctlr->flexa_abort_notified &&
                !gpu_vn_ctlr->flexa_stop &&
                gpu_vn_ctlr->bond != NULL &&
                gpu_vn_ctlr->bond->flexa_done != NULL) {
                gpu_vn_ctlr->bond->flexa_done(src_line_count, gpu_vn_ctlr->bond);
            }

            continue;
        }

        /* Process available line blocks */
        bool frame_aborted = false;
        while (!gpu_vn_ctlr->flexa_stop && gpu_flex_has_enough_lines(flex, src_line_count, config->flexa_lines))
        {
            if (!gpu_flex_process_line_block(flex, gpu_vn_ctlr, src_frame_seq))
            {
                frame_aborted = true;
                break;
            }
            AVDK_MONITOR_GPU_LINE_PLUS();
        }

        if (gpu_vn_ctlr->flexa_stop)
        {
            break;
        }

        if (frame_aborted)
        {
            continue;
        }

        uint32_t gpu_rd_cnt = flex->read_lines / config->flexa_lines;

        if (gpu_vn_ctlr->bond != NULL && gpu_vn_ctlr->bond->flexa_done != NULL) {
            gpu_vn_ctlr->bond->flexa_done(gpu_rd_cnt, gpu_vn_ctlr->bond);
        }

        if (gpu_vn_ctlr->config.flexa_line_done)
        {
            gpu_vn_ctlr->config.flexa_line_done(gpu_rd_cnt, gpu_vn_ctlr->config.flexa_line_done_args);
        }

        if (gpu_vn_ctlr->flexa_stop)
        {
            break;
        }

        /* Check if frame is complete */
        if (flex->read_lines >= flex->input_height)
        {
            if (gpu_flex_data_frame_done(flex, gpu_vn_ctlr))
            {
                GPU_FRAME_END();
            }
        }
    }

thread_exit:
    LOGW("%s,%d exit\n", __func__, __LINE__);

    /* Self-delete only. gpu_ctlr_close() joins this thread via rtos_thread_join()
     * and is the sole owner of control->flexa_thd, so the worker must NOT clear
     * it here (doing so previously created a re-open race). */
    rtos_delete_thread(NULL);
}

static avdk_err_t gpu_ctlr_init(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_t *control =  __containerof(handle, gpu_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    avdk_err_t ret = AVDK_ERR_OK;
    vg_lite_error_t vg_ret = VG_LITE_SUCCESS;

    if (control->config.flexa) {
        control->blit_enable = false;
        control->display_blit_buffer = NULL;
        os_memset(&control->display_blit_config, 0, sizeof(bk_gpu_blit_config_t));
        control->update_blit_buffer = NULL;
        os_memset(&control->update_blit_config, 0, sizeof(bk_gpu_blit_config_t));
        ret = rtos_init_mutex(&control->blit_mutex);
        if (ret != AVDK_ERR_OK) {
            LOGE("%s, %d rtos_init_mutex failed\n", __func__, __LINE__);
            return ret;
        }
        ret = rtos_init_mutex(&control->gpu_mutex);
        if (ret != AVDK_ERR_OK) {
            LOGE("%s, %d rtos_init_mutex gpu_mutex failed\n", __func__, __LINE__);
            rtos_deinit_mutex(&control->blit_mutex);
            control->blit_mutex = NULL;
            return ret;
        }
    } else {
        LOGE("%s %d flexa is not enabled\r\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }

    bk_gpu_driver_init();

    control->gpu_contiguous_buffer = bk_get_gpu_flexa_buffer(CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ);
    if (control->gpu_contiguous_buffer == NULL)
    {
        LOGE("%s, %d bk_get_gpu_flexa_buffer failed\n", __func__, __LINE__);
        ret = AVDK_ERR_NOMEM;
        goto error;
    }
    vg_lite_set_buffer(control->gpu_contiguous_buffer);
    vg_ret = vg_lite_init(control->config.tess_width, control->config.tess_height);
    if (vg_ret != VG_LITE_SUCCESS) {
        LOGE("%s, %d vg_lite_init failed %d\n", __func__, __LINE__, vg_ret);
        ret = AVDK_ERR_HWERROR;
        goto error;
    }

    return AVDK_ERR_OK;

error:
    vg_lite_set_buffer(NULL);
    if (control->gpu_contiguous_buffer) {
        os_free(control->gpu_contiguous_buffer);
        control->gpu_contiguous_buffer = NULL;
    }
    if (control->gpu_mutex) {
        rtos_deinit_mutex(&control->gpu_mutex);
        control->gpu_mutex = NULL;
    }
    if (control->blit_mutex) {
        rtos_deinit_mutex(&control->blit_mutex);
        control->blit_mutex = NULL;
    }
    return ret;
}

static avdk_err_t gpu_ctlr_deinit(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_t *control =  __containerof(handle, gpu_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    vg_lite_error_t vg_ret = VG_LITE_SUCCESS;

    if (control->config.flexa) {
        if (control->blit_mutex) {
            avdk_err_t ret = rtos_deinit_mutex(&control->blit_mutex);
            if (ret != AVDK_ERR_OK) {
                LOGE("%s, %d rtos_deinit_mutex failed\n", __func__, __LINE__);
                return ret;
            }
            control->blit_mutex = NULL;
        }
        if (control->gpu_mutex) {
            avdk_err_t ret = rtos_deinit_mutex(&control->gpu_mutex);
            if (ret != AVDK_ERR_OK) {
                LOGE("%s, %d rtos_deinit_mutex gpu_mutex failed\n", __func__, __LINE__);
                return ret;
            }
            control->gpu_mutex = NULL;
        }
    } else {
        LOGE("%s %d flexa is not enabled\r\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }

    vg_ret = vg_lite_close();
    if (vg_ret != VG_LITE_SUCCESS) {
        LOGE("%s, %d vg_lite_close failed %d\n", __func__, __LINE__, vg_ret);
        return AVDK_ERR_HWERROR;
    }
    vg_lite_set_buffer(NULL);

    bk_gpu_driver_deinit();

    if (control->gpu_contiguous_buffer) {
        os_free(control->gpu_contiguous_buffer);
        control->gpu_contiguous_buffer = NULL;
    }

    return AVDK_ERR_OK;
}

/**
 * @brief Release all flexa runtime resources allocated by gpu_ctlr_open().
 *
 * Shared by the gpu_ctlr_open() failure path and gpu_ctlr_close() so the
 * teardown sequence cannot drift between the two. Every step is individually
 * guarded so it is safe to call after a partial open. The caller is responsible
 * for stopping/joining the worker thread first (close) or for never having
 * started it (open failure) before invoking this.
 */
static void gpu_flex_resource_teardown(gpu_vn_ctlr_t *control)
{
    const bk_gpu_ctlr_config_t *config = &control->config;
    gpu_flex_data_t *flex = &control->flex;

#if HDMA_OPEN_ISR_ENABLE
    if (flex->gdma < HPDMA_ID_MAX) {
        bk_hpdma_disable_finish_interrupt(flex->gdma);
        bk_hpdma_register_isr(flex->gdma, NULL, NULL, NULL, NULL);
    }
#endif

    if (flex->gdma < HPDMA_ID_MAX) {
        bk_err_t free_ret = bk_hpdma_free(HPDMA_DEV_DTCM, flex->gdma);
        if (free_ret != BK_OK) {
            LOGE("%s,%d bk_hpdma_free(ch=%d) failed ret=%d, DMA may still be active\n",
                 __func__, __LINE__, flex->gdma, free_ret);
        }
        flex->gdma = HPDMA_ID_MAX;
    }

    if (flex->dpu_frame_buffers != NULL && config->frame_free != NULL) {
        config->frame_free(flex->dpu_frame_buffers);
        flex->dpu_frame_buffers = NULL;
    }

    if (control->gpu_process_sem) {
        rtos_deinit_semaphore(&control->gpu_process_sem);
        control->gpu_process_sem = NULL;
    }

    if (flex->link_dma_list_table != NULL) {
        bk_hpdma_link_deinit(flex->link_dma_list_table);
        flex->link_dma_list_table = NULL;
    }

    gpu_flex_deinit_pingpong_buffer(flex);
    gpu_flex_data_deinit(flex, control);
}

static avdk_err_t gpu_ctlr_open(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_t *control =  __containerof(handle, gpu_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (control->config.flexa) {
        const bk_gpu_ctlr_config_t *config = &control->config;
        gpu_flex_data_t *flex = &control->flex;
        avdk_err_t ret;

        if (control->flexa_thd) {
            LOGW("%s, %d gpu flexa thread is already opened\n", __func__, __LINE__);
            return AVDK_ERR_OK;
        }

        flex->gdma = HPDMA_ID_MAX;
        flex->link_dma_list_table = NULL;
        flex->dpu_frame_buffers = NULL;

        ret = rtos_init_semaphore_ex(&control->gpu_process_sem, 1, 0);
        if (ret != AVDK_ERR_OK) {
            LOGE("%s, %d rtos_init_semaphore_ex gpu_process_sem failed\n", __func__, __LINE__);
            return ret;
        }

        /* Initialize GPU hardware */
        gpu_flex_data_init(flex, control);

        /* Initialize face detection matrix */
        vg_lite_identity(&flex->draw_matrix);
        vg_lite_rotate((float)config->rotate_degree, &flex->draw_matrix);

        /* Allocate initial frame buffer */
        flex->dpu_frame_buffers = config->frame_malloc(bk_pixel_size_get(config->dst_format) * (config->compress ? flex->output_width / 4 : flex->output_width) * flex->output_height);
        if (flex->dpu_frame_buffers == NULL)
        {
            LOGE("Failed to allocate flex->dpu_frame_buffers\r\n");
            ret = AVDK_ERR_NOMEM;
            goto open_fail;
        }
        AVDK_MONITOR_GPU_ENABLE();

        control->line_err_flag = 1;
        control->flexa_stop = false;

        ret = rtos_init_semaphore_ex(&control->gpu_start_sem, 1, 0);
        if (ret != AVDK_ERR_OK) {
            LOGE("%s, %d rtos_init_semaphore_ex gpu_start_sem failed\n", __func__, __LINE__);
            goto open_fail;
        }

        ret = rtos_create_hsram_thread(&control->flexa_thd,
                                        1,
                                        "gpu",
                                        (beken_thread_function_t)gpu_flex_main_entry,
                                        1024 * 10,
                                        control);

        if (ret != AVDK_ERR_OK) {
            LOGE("%s, %d rtos_create_hsram_thread failed\n", __func__, __LINE__);
            rtos_deinit_semaphore(&control->gpu_start_sem);
            control->gpu_start_sem = NULL;
            goto open_fail;
        }

        /* Wait until the worker has signalled it is up, then dispose of the
         * one-shot startup semaphore. */
        rtos_get_semaphore(&control->gpu_start_sem, BEKEN_WAIT_FOREVER);
        rtos_deinit_semaphore(&control->gpu_start_sem);
        control->gpu_start_sem = NULL;

        LOGI("%s, %d worker started\n", __func__, __LINE__);
        return AVDK_ERR_OK;

open_fail:
        gpu_flex_resource_teardown(control);
        return ret;
    } else {
        LOGE("%s %d flexa is not enabled\r\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }

    return AVDK_ERR_OK;
}


static avdk_err_t gpu_ctlr_close(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_t *control =  __containerof(handle, gpu_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (control->config.flexa) {
        if (control->flexa_thd == NULL) {
            LOGW("%s, %d gpu flexa thread is already closed\n", __func__, __LINE__);
            return AVDK_ERR_OK;
        }

        control->flexa_stop = true;

#if HDMA_OPEN_ISR_ENABLE
        if (control->flex.transfer_sem) {
            rtos_set_semaphore(&control->flex.transfer_sem);
        }
#endif

        if (control->gpu_process_sem)
        {
            bk_err_t ret = rtos_set_semaphore(&control->gpu_process_sem);
            if (ret != BK_OK) {
                LOGW("%s, %d rtos_set_semaphore failed\n", __func__, __LINE__);
            }
        }

        /* Block until the worker has fully terminated, then take ownership of
         * the handle here. This removes the previous race where the worker
         * cleared flexa_thd after close() had already returned, which could make
         * a fast re-open mis-detect the controller as still open. */
        rtos_thread_join(&control->flexa_thd);
        control->flexa_thd = NULL;

        gpu_flex_resource_teardown(control);

        LOGI("%s, %d flexa thread joined and resources released\n", __func__, __LINE__);
    } else {
        LOGE("%s %d flexa is not enabled\r\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t gpu_ctlr_ioctl(bk_gpu_ctlr_handle_t handle, uint32_t cmd, void *args)
{
    gpu_vn_ctlr_t *control =  __containerof(handle, gpu_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    switch (cmd)
    {
        case BK_GPU_IOCTL_SET_FLEXA_LINES_READY:
            gpu_isp_line_done_handle((uint32_t)args, control);
            break;

        case BK_GPU_IOCTL_SET_FLEXA_EVENT_READY:
        {
            bk_gpu_flexa_event_t *event = (bk_gpu_flexa_event_t *)args;
            if (event == NULL) {
                LOGW("%s %d flexa event is NULL\r\n", __func__, __LINE__);
                return AVDK_ERR_INVAL;
            }
            gpu_flexa_event_ready_handle(event->frame_seq, event->line_cnt, control);
        }
        break;

        case BK_GPU_IOCTL_SET_NOTIFY:
            gpu_flex_restart(control);
            break;

        case BK_GPU_IOCTL_REGISTER_BOND:
        {
            bk_flexa_bond_t *bond = (bk_flexa_bond_t *)args;
            if (bond == NULL) {
                LOGW("%s %d bond is NULL\r\n", __func__, __LINE__);
                return AVDK_ERR_INVAL;
            }
            else if (control->bond != NULL) {
                LOGW("%s %d bond is already registered\r\n", __func__, __LINE__);
                return AVDK_ERR_OK;
            }
            else {
                control->bond = bond;
            }
        }
        break;

        case BK_GPU_IOCTL_UNREGISTER_BOND:
        {
            bk_flexa_bond_t *bond = (bk_flexa_bond_t *)args;
            if (control->bond == bond) {
                control->bond = NULL;
                return AVDK_ERR_OK;
            }
            else {
                LOGW("%s %d bond is not registered\r\n", __func__, __LINE__);
                return AVDK_ERR_INVAL;
            }
        }
        break;

        case BK_GPU_IOCTL_FLEXA_ADDR_MAPPING:
            if (control->config.flexa) {
                uint8_t *src_buffer = (uint8_t *)args;
                gpu_flexa_addr_mapping(control->config.src_width, control->config.src_height,
                                        (uint32_t)src_buffer,
                                        control->config.flexa_lines,
                                        control->config.flexa_buff_cnt);
            } else {
                LOGW("%s %d flexa is not enabled\r\n", __func__, __LINE__);
            }
            break;

        case BK_GPU_IOCTL_FLEXA_ADDR_UNMAPPING:
            gpu_flexa_addr_unmapping();
            break;

        case BK_GPU_IOCTL_LOCK:
            if (control->gpu_mutex == NULL) {
                LOGW("%s %d gpu_mutex is NULL\r\n", __func__, __LINE__);
                return AVDK_ERR_INVAL;
            }
            rtos_lock_mutex(&control->gpu_mutex);
            break;

        case BK_GPU_IOCTL_UNLOCK:
            if (control->gpu_mutex == NULL) {
                LOGW("%s %d gpu_mutex is NULL\r\n", __func__, __LINE__);
                return AVDK_ERR_INVAL;
            }
            rtos_unlock_mutex(&control->gpu_mutex);
            break;

        default:
            return AVDK_ERR_UNSUPPORTED;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t gpu_ctlr_delete(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_t *control =  __containerof(handle, gpu_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    os_free(control);
    control = NULL;

    return AVDK_ERR_OK;
}

avdk_err_t bk_gpu_ctlr_new(bk_gpu_ctlr_handle_t *handle, bk_gpu_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    gpu_vn_ctlr_t *controller = os_malloc(sizeof(gpu_vn_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(gpu_vn_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_gpu_ctlr_config_t));
    controller->ops.init = gpu_ctlr_init;
    controller->ops.open = gpu_ctlr_open;
    controller->ops.close = gpu_ctlr_close;
    controller->ops.deinit = gpu_ctlr_deinit;
    controller->ops.ioctl = gpu_ctlr_ioctl;
    controller->ops.del = gpu_ctlr_delete;
    controller->ops.draw_path_clear = gpu_draw_path_clear;
    controller->ops.draw_path_build = gpu_draw_path_build;
    controller->ops.blit_set = gpu_blit_set;
    controller->ops.blit_clear = gpu_blit_clear;

    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}
