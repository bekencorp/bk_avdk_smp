#include <os/os.h>
#include <os/mem.h>
#include <driver/sys_pm.h>
#include <components/bk_gpu_ctlr.h>
#include <components/bk_hardware_ram.h>
#include <modules/vg_lite_gpu/vg_lite_platform.h>
#include "avdk_monitor.h"
#include "gpu_vn_ctlr.h"
#include "sys_driver.h"
#include <bk_flexa_bond_types.h>

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

static void gpu_driver_init(void)
{
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_GPU, PM_POWER_MODULE_STATE_ON);

    sys_drv_gpu_cksel_clkdiv_set(CKSEL_GPU_480M, 0);

    bk_pm_clock_ctrl(PM_CLK_ID_GPU, PM_CLK_CTRL_PWR_UP);

    bk_int_isr_register(INT_SRC_GPU, vg_lite_IRQHandler, NULL);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPU, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPU, 1);
#endif
}

static void gpu_driver_deinit(void)
{
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPU, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPU, 0);
#endif
    bk_int_isr_unregister(INT_SRC_GPU);

    bk_pm_clock_ctrl(PM_CLK_ID_GPU, PM_CLK_CTRL_PWR_DOWN);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_GPU, PM_POWER_MODULE_STATE_OFF);
}

static void gpu_flexa_addr_mapping(uint16_t width, uint16_t height, uint32_t base_addr, uint16_t flexa_lines, uint8_t buf_cnt)
{
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
            vg_format = VG_LITE_RGB888;
            break;
        case BK_PIXEL_FORMAT_BGR888:
            vg_format = VG_LITE_BGR888;
            break;

        case BK_PIXEL_FORMAT_ARGB8888:
        case BK_PIXEL_FORMAT_ABGR8888:
        case BK_PIXEL_FORMAT_RGBA8888:
        case BK_PIXEL_FORMAT_BGRA8888:
            vg_format = VG_LITE_BGRA8888;
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

#define CHECK_ERROR
static avdk_err_t gpu_blit_set(bk_gpu_ctlr_handle_t handle, void *src_buffer, bk_gpu_blit_config_t *blit_config)
{
    gpu_vn_ctlr_t *controller =  __containerof(handle, gpu_vn_ctlr_t, ops);

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

static void gpu_isp_line_done_handle(uint32_t line, void *arg)
{
    gpu_vn_ctlr_t *gpu_vn_ctlr = (gpu_vn_ctlr_t *)arg;

    if (line == 1)
    {
        if (gpu_vn_ctlr->line_err_flag)
        {
            gpu_vn_ctlr->line_err_flag = 0;
        }
    }

    gpu_vn_ctlr->line_cnt = line;
    if (gpu_vn_ctlr->gpu_process_sem)
    {
        rtos_set_semaphore(&gpu_vn_ctlr->gpu_process_sem);
    }
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
 * @brief Initialize GPU buffer allocation
 * @param data GPU flex data structure
 * @return 0 on success, -1 on failure
 */
static int gpu_flex_init_gpu_buffer(gpu_flex_data_t *data)
{


    return 0;
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
 * @param rotation_degree Rotation angle (0 or 90)
 */
static void gpu_flex_configure_dst_buffer(gpu_flex_data_t *data, bk_gpu_ctlr_config_t *config)
{
    memset(&data->dst_buf, 0, sizeof(vg_lite_buffer_t));

    if (config->rotate_degree == 90)
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

    data->dst_buf.tiled = VG_LITE_TILED;
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
    bk_hpdma_set_dest_burst_len(data->gdma, 0x03);
    bk_hpdma_set_src_burst_len(data->gdma, 0x03);
#if HDMA_OPEN_ISR_ENABLE
    BK_LOG_ON_ERR(bk_hpdma_register_isr(data->gdma, NULL, NULL, gpu_flex_hpdma_link_transfer_complete_callback, &data->transfer_sem));
    BK_LOG_ON_ERR(bk_hpdma_enable_finish_interrupt(data->gdma));
#endif
}

/**
 * @brief Update transformation matrix for current processing line
 * @param data GPU flex data structure
 * @param config GPU controller configuration
 * @param draw_matrix Optional box matrix for face detection (can be NULL)
 */
static void gpu_flex_update_matrix(gpu_flex_data_t *data, 
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
    data->input_width = config->src_width;
    data->input_height = config->src_height;
    data->output_width = config->dst_width;
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
    }

    if (data->draw_mutex)
    {
        rtos_deinit_mutex(&data->draw_mutex);
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

    /* Reset per-frame counters; next frame will start from index 1. */
    flex->flexa_index = 1;
    flex->read_lines = 0;
}

/**
 * @brief Pull out processed line data from GPU buffer
 * @param data GPU flex data structure
 * @param gpu_vn_ctlr GPU controller handle
 */
static inline void gpu_flex_data_line_pull_out(gpu_flex_data_t *data, gpu_vn_ctlr_t *gpu_vn_ctlr)
{
    const bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;

    /* Calculate read lines */
    data->read_lines = data->need_lines / data->output_height;
#if HDMA_OPEN_ISR_ENABLE
    rtos_get_semaphore(&data->transfer_sem, BEKEN_WAIT_FOREVER);
#else
    while(bk_hpdma_get_next_ll_addr(data->gdma));
    while(bk_hpdma_get_enable_status(data->gdma));
#endif
    /* Copy processed data based on rotation angle */
    if (config->rotate_degree == 90)
    {
        uint32_t offset = 0;
        uint32_t stride = 0;
        if (config->compress)
        {
            uint32_t output_height_div_flexa = data->output_height / config->flexa_lines;
            offset = (output_height_div_flexa - data->flexa_index) * data->flexa_lines_x_4;
            stride = data->output_height * bk_pixel_size_get(config->dst_format) - data->flexa_lines_x_4;
        }
        else
        {
            stride = (data->output_height - config->flexa_lines) * bk_pixel_size_get(config->dst_format);
            offset = (data->output_height - data->flexa_index * config->flexa_lines) * bk_pixel_size_get(config->dst_format);
        }

        // Use single descriptor for all rows (2D transfer)
        hpdma_link_config_t dma_config[1];

        // Source: continuous storage starting from dst_buf.memory
        dma_config[0].src_addr = (uint32_t)data->dst_buf.memory;
        // Destination: 2D layout starting from offset
        dma_config[0].dst_addr = (uint32_t)(data->dpu_frame_buffers + offset);
        // X size: 64 bytes per row (16 lines * 4 bytes)
        dma_config[0].src_xsize = data->flexa_lines_x_4;
        dma_config[0].dst_xsize = data->flexa_lines_x_4;
        dma_config[0].src_ysize =  config->compress ? data->output_width / 4 : data->output_width;
        dma_config[0].dst_ysize =  config->compress ? data->output_width / 4 : data->output_width;
        // Source step: 0 (continuous storage)
        dma_config[0].src_step = 0;
        // Destination step: stride bytes per row (for 2D layout)
        dma_config[0].dst_step = stride;
        // Enable interrupt on completion
        dma_config[0].finish_int_en = 1;
        dma_config[0].half_finish_int_en = 0;
        //LOGI("dma_config.src_xsize %d dma_config.dst_xsize %d dma_config.src_ysize %d dma_config.dst_ysize %d\r\n", dma_config[0].src_xsize, dma_config[0].dst_xsize,  dma_config[0].src_ysize, dma_config[0].dst_ysize);
        BK_LOG_ON_ERR(bk_hpdma_link_set_descs(data->link_dma_list_table, (hpdma_link_config_t *)dma_config, 1));
        /* Start transfer */
        BK_LOG_ON_ERR(bk_hpdma_link_transfer(data->gdma, data->link_dma_list_table));
        /* Wait for transfer complete */
    }
    else if (config->rotate_degree == 0)
    {
        uint32_t offset = (data->flexa_index - 1) * data->output_width_x_flexa_lines;
        // uint32_t copy_size = data->output_width_x_flexa_lines;

        // bk_hpdma_memcpy(data->dpu_frame_buffers + offset,
        //                         data->dst_buf.memory,
        //                         copy_size);
        hpdma_link_config_t dma_config[1];

        dma_config[0].src_addr = (uint32_t)data->dst_buf.memory;
        dma_config[0].dst_addr = (uint32_t)(data->dpu_frame_buffers + offset);
        dma_config[0].src_xsize = data->output_width;
        dma_config[0].dst_xsize = data->output_width;
        dma_config[0].src_ysize = 16;
        dma_config[0].dst_ysize = 16;
        dma_config[0].src_step = 0;
        dma_config[0].dst_step = 0;
        dma_config[0].finish_int_en = 1;
        dma_config[0].half_finish_int_en = 0;
        BK_LOG_ON_ERR(bk_hpdma_link_set_descs(data->link_dma_list_table, (hpdma_link_config_t *)dma_config, 1));
        BK_LOG_ON_ERR(bk_hpdma_link_transfer(data->gdma, data->link_dma_list_table));
    }

    /* Switch to next ping-pong buffer */
    data->dst_buf_idx = 1 - data->dst_buf_idx;
    data->dst_buf.memory = (vg_lite_pointer)(uintptr_t)data->buffers[data->dst_buf_idx];
    data->dst_buf.address = data->buffers[data->dst_buf_idx];
    data->flexa_index++;
}

static void gpu_flex_data_frame_done_blit(gpu_flex_data_t *flex, const bk_gpu_ctlr_config_t *config, bk_gpu_blit_config_t *blit_config, void *front_frame, void *display_frame)
{
    vg_lite_buffer_t display_buffer;
    vg_lite_buffer_t front_buffer;
    vg_lite_matrix_t display_matrix;

    LOGV("%s, frame_done_blit\n", __func__);

    memset(&display_buffer , 0, sizeof(display_buffer));
    memset(&front_buffer , 0, sizeof(front_buffer));

    if (config->rotate_degree == 90)
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
    front_buffer.format = gpu_format_convert(blit_config->src_format);
    vg_lite_allocate_with_data(&front_buffer, front_frame, NULL, NULL, NULL);

    memset(&display_matrix , 0, sizeof(display_matrix));
    vg_lite_identity(&display_matrix);
    vg_lite_allocate_with_data(&display_buffer, display_frame, NULL, NULL, NULL);

    vg_lite_rectangle_t rect = {
        .x = blit_config->src_x,
        .y = blit_config->src_y,
        .width = blit_config->src_width,
        .height = blit_config->src_height,
    };

    vg_lite_translate(blit_config->dst_x, blit_config->dst_y, &display_matrix);

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

    vg_lite_free(&display_buffer);
    vg_lite_free(&front_buffer);
}

/**
 * @brief Handle frame completion
 * @param data GPU flex data structure
 * @param gpu_vn_ctlr GPU controller handle
 */
static inline void gpu_flex_data_frame_done(gpu_flex_data_t *data, gpu_vn_ctlr_t *gpu_vn_ctlr)
{
    const bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;

#if HDMA_OPEN_ISR_ENABLE
    rtos_get_semaphore(&data->transfer_sem, BEKEN_WAIT_FOREVER);
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
            gpu_flex_data_frame_done_blit(data, config, &gpu_vn_ctlr->display_blit_config, gpu_vn_ctlr->display_blit_buffer, data->dpu_frame_buffers);
        }
    }

    rtos_unlock_mutex(&gpu_vn_ctlr->blit_mutex);

    /* Swap frame buffer */
    uint32_t frame_size = bk_pixel_size_get(config->dst_format) * (config->compress ? data->output_width / 4 : data->output_width) * data->output_height;
    void *new_buffer = config->malloc(frame_size);
    //void *new_buffer = NULL;
    if (new_buffer)
    {
        if(config->frame_display)
        {
            config->frame_display(data->dpu_frame_buffers, frame_size, config->frame_display_args);
        }
        data->dpu_frame_buffers = new_buffer;
        AVDK_MONITOR_GPU_FRAME_PLUS();
    }

    if (gpu_vn_ctlr->bond != NULL && gpu_vn_ctlr->bond->frame_done != NULL) {
        gpu_vn_ctlr->bond->frame_done(BK_OK, gpu_vn_ctlr->bond);
    }

    if (config->frame_done)
    {
        config->frame_done(data->dpu_frame_buffers, frame_size, config->frame_done_args);
    }
    /* Reset state for next frame */
    //rtos_get_semaphore(&gpu_vn_ctlr->gpu_process_sem, BEKEN_NO_WAIT);
    data->flexa_index = 1;
    data->read_lines = 0;

#if HDMA_OPEN_ISR_ENABLE
    rtos_set_semaphore(&data->transfer_sem);
#endif
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

/**
 * @brief Process a single line block with GPU
 * @param data GPU flex data structure
 * @param gpu_vn_ctlr GPU controller handle
 */
static void gpu_flex_process_line_block(gpu_flex_data_t *data,
                                        gpu_vn_ctlr_t *gpu_vn_ctlr)
{
    const bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;
    GPU_LINE_START();
    /* Update transformation matrix for current line */
    gpu_flex_update_matrix(data, config);

    /* Perform GPU blit operation */
    vg_lite_blit(&data->dst_buf, &data->src_buf, &data->matrix,
                 VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
    vg_lite_finish();

    /* Draw face detection rectangles if enabled */
    rtos_lock_mutex(&data->draw_mutex);

    if (data->draw_enable)
    {
        gpu_draw_path_process(&data->draw_matrix, &data->draw_path, &data->dst_buf);
    }
    GPU_LINE_END();
    rtos_unlock_mutex(&data->draw_mutex);
    HPDMA_LINE_START();
    /* Pull out processed line data */
    gpu_flex_data_line_pull_out(data, gpu_vn_ctlr);
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

    rtos_init_semaphore_ex(&gpu_vn_ctlr->gpu_process_sem, 1, 0);

    /* Initialize GPU hardware */
    gpu_flex_data_init(flex, gpu_vn_ctlr);

    /* Initialize face detection matrix */
    vg_lite_identity(&flex->draw_matrix);
    vg_lite_rotate((float)config->rotate_degree, &flex->draw_matrix);

    /* Allocate initial frame buffer */
    flex->dpu_frame_buffers = config->malloc(bk_pixel_size_get(config->dst_format) * (config->compress ? flex->output_width / 4 : flex->output_width) * flex->output_height);
    if (flex->dpu_frame_buffers == NULL)
    {
        LOGE("Failed to allocate flex->dpu_frame_buffers\r\n");
        goto thread_exit;
    }
    AVDK_MONITOR_GPU_ENABLE();

    gpu_vn_ctlr->line_err_flag = 1;

    gpu_vn_ctlr->flexa_stop = false;

    bk_err_t ret = rtos_set_semaphore(&gpu_vn_ctlr->gpu_flex_task_sem);
    if (ret != BK_OK) {
        LOGE("%s, %d rtos_set_semaphore failed\n", __func__, __LINE__);
        goto thread_exit;
    }

    /* Main processing loop */
    while (1)
    {
        /* Wait for semaphore from ISP callback */
        rtos_get_semaphore(&gpu_vn_ctlr->gpu_process_sem, BEKEN_WAIT_FOREVER);

        if (gpu_vn_ctlr->flexa_stop)
        {
            break;
        }

        uint32_t src_line_count = gpu_vn_ctlr->line_cnt;
        if (src_line_count == 1)
        {
            if ((flex->read_lines > 0) && (flex->read_lines < flex->input_height))
            {
                flex->flexa_index = 1;
                flex->read_lines = 0;
            }
            GPU_FRAME_START();
        }

        /* Handle line error flag */
        if (gpu_vn_ctlr->line_err_flag)
        {
            if (gpu_vn_ctlr->bond != NULL && gpu_vn_ctlr->bond->flexa_done != NULL) {
                gpu_vn_ctlr->bond->flexa_done(src_line_count, gpu_vn_ctlr->bond);
            }

            LOGW("%s, %d line error flag, src_line_count %d\n", __func__, __LINE__, src_line_count);
            continue;
        }

        /* Process available line blocks */
        while (gpu_flex_has_enough_lines(flex, src_line_count, config->flexa_lines))
        {
            gpu_flex_process_line_block(flex, gpu_vn_ctlr);
            AVDK_MONITOR_GPU_LINE_PLUS();
        }

        uint32_t gpu_rd_cnt = flex->read_lines / config->flexa_lines;

        if (gpu_vn_ctlr->bond != NULL && gpu_vn_ctlr->bond->flexa_done != NULL) {
            gpu_vn_ctlr->bond->flexa_done(gpu_rd_cnt, gpu_vn_ctlr->bond);
        }

        if (gpu_vn_ctlr->config.flexa_line_done)
        {
            gpu_vn_ctlr->config.flexa_line_done(gpu_rd_cnt, gpu_vn_ctlr->config.flexa_line_done_args);
        }

        /* Check if frame is complete */
        if (flex->read_lines >= flex->input_height)
        {
            gpu_flex_data_frame_done(flex, gpu_vn_ctlr);
            GPU_FRAME_END();
        }
    }

thread_exit:
    LOGW("%s,%d exit\n", __func__, __LINE__);
    if (flex->dpu_frame_buffers != NULL && config->free != NULL)
    {
        config->free(flex->dpu_frame_buffers);
        flex->dpu_frame_buffers = NULL;
    }

    if (gpu_vn_ctlr->gpu_process_sem) {
        rtos_deinit_semaphore(&gpu_vn_ctlr->gpu_process_sem);
        gpu_vn_ctlr->gpu_process_sem = NULL;
    }

#if HDMA_OPEN_ISR_ENABLE
    if (flex->gdma < HPDMA_ID_MAX) {
        bk_hpdma_disable_finish_interrupt(flex->gdma);
        bk_hpdma_register_isr(flex->gdma, NULL, NULL, NULL, NULL);
    }
#endif
    bk_hpdma_free(HPDMA_DEV_DTCM, flex->gdma);
    bk_hpdma_link_deinit(flex->link_dma_list_table);
    gpu_flex_deinit_pingpong_buffer(flex);
    
    gpu_flex_data_deinit(flex, gpu_vn_ctlr);
    
    rtos_set_semaphore(&gpu_vn_ctlr->gpu_flex_task_sem);

    gpu_vn_ctlr->flexa_thd = NULL;
    rtos_delete_thread(NULL);
}

static avdk_err_t gpu_ctlr_init(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_t *control =  __containerof(handle, gpu_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    //TODO
    if (control->config.flexa) {
        control->blit_enable = false;
        control->display_blit_buffer = NULL;
        os_memset(&control->display_blit_config, 0, sizeof(bk_gpu_blit_config_t));
        control->update_blit_buffer = NULL;
        os_memset(&control->update_blit_config, 0, sizeof(bk_gpu_blit_config_t));
        avdk_err_t ret = rtos_init_mutex(&control->blit_mutex);
        if (ret != AVDK_ERR_OK) {
            LOGE("%s, %d rtos_init_mutex failed\n", __func__, __LINE__);
            return ret;
        }
    }

    gpu_driver_init();

    control->gpu_contiguous_buffer = bk_get_gpu_flexa_buffer(CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ);
    if (control->gpu_contiguous_buffer == NULL)
    {
        LOGE("%s, %d bk_get_gpu_flexa_buffer failed\n", __func__, __LINE__);
        return AVDK_ERR_NOMEM;
    }
    vg_lite_set_buffer(control->gpu_contiguous_buffer);
    vg_lite_init(control->config.tess_width, control->config.tess_height);

    return AVDK_ERR_OK;
}

static avdk_err_t gpu_ctlr_deinit(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_t *control =  __containerof(handle, gpu_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (control->config.flexa) {
        if (control->blit_mutex) {
            avdk_err_t ret = rtos_deinit_mutex(&control->blit_mutex);
            if (ret != AVDK_ERR_OK) {
                LOGE("%s, %d rtos_deinit_mutex failed\n", __func__, __LINE__);
                return ret;
            }
        }
    }

    if (control->gpu_contiguous_buffer) {
        os_free(control->gpu_contiguous_buffer);
        control->gpu_contiguous_buffer = NULL;
    }

    vg_lite_close();

    gpu_driver_deinit();

    return AVDK_ERR_OK;
}

static avdk_err_t gpu_ctlr_open(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_t *control =  __containerof(handle, gpu_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (control->config.flexa) {
        if (control->flexa_thd) {
            LOGW("%s, %d gpu flexa thread is already opened\n", __func__, __LINE__);
            return AVDK_ERR_OK;
        }

        avdk_err_t ret = rtos_init_semaphore_ex(&control->gpu_flex_task_sem, 1, 0);
        if (ret != AVDK_ERR_OK) {
            LOGE("%s, %d rtos_init_semaphore_ex failed\n", __func__, __LINE__);
            return ret;
        }

        ret = rtos_create_hsram_thread(&control->flexa_thd,
                                        1,
                                        "gpu",
                                        (beken_thread_function_t)gpu_flex_main_entry,
                                        1024 * 10,
                                        control);

        if (ret != AVDK_ERR_OK) {
            LOGE("%s, %d rtos_create_hsram_thread failed\n", __func__, __LINE__);
            rtos_deinit_semaphore(&control->gpu_flex_task_sem);
            control->gpu_flex_task_sem = NULL;
            return ret;
        }

        rtos_get_semaphore(&control->gpu_flex_task_sem, BEKEN_WAIT_FOREVER);

        LOGI("%s, %d sem get successful\n", __func__, __LINE__);
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

	    if (control->gpu_process_sem)
	    {
	        bk_err_t ret = rtos_set_semaphore(&control->gpu_process_sem);
            if (ret != BK_OK) {
                LOGE("%s, %d rtos_set_semaphore failed\n", __func__, __LINE__);
                return ret;
            }
	    }
#if HDMA_OPEN_ISR_ENABLE
        if (control->flex.transfer_sem) {
            rtos_set_semaphore(&control->flex.transfer_sem);
        }
#endif
        rtos_get_semaphore(&control->gpu_flex_task_sem, BEKEN_WAIT_FOREVER);

        rtos_deinit_semaphore(&control->gpu_flex_task_sem);
        control->gpu_flex_task_sem = NULL;

        LOGI("%s, %d sem get successful, flexa thread has exited\n", __func__, __LINE__);
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
