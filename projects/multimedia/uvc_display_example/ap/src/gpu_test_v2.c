#include <os/os.h>
#include <os/mem.h>

#include <common/bk_include.h>
#include <avdk_types.h>
#include "components/bk_gpu_ctlr.h"
#include "components/bk_gpu_types.h"
#include "gpu_vn_ctlr_v2.h"
#include "decode_private.h"

//TODO
#include <common/bk_err.h>
#include "modules/vg_lite_gpu/vg_lite.h"
#include <modules/vg_lite_gpu/vg_lite_platform.h>
#include "sys_driver.h"
#include "avdk_monitor.h"
#include <driver/int.h>
#include <soc/bk7259/int_types_impl.h>
#include "avdk_utils.h"
#define TAG "bk_gpu_ctlr"
#include <components/bk_hardware_ram.h>
#include <common/avdk_pixel_types.h>



#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...)

#define GPU_HIGHT_ALIGNMENT (0xF)
#define FLEXA_LINES 16
#define GPU_Y_VADDR_BASE    0x38200000
#define DPU_FRAME_BUFFER_NUM    4

/* Buffer alignment constants */
#define BUFFER_ALIGNMENT_MASK     0x3F        /* 64-byte alignment */
#define BUFFER_ALIGNMENT_SIZE     64


#ifndef MEM_CACHABLE_MASK
#define MEM_CACHABLE_MASK 0x00000000
#endif


static void gpu_test_int_config(void)
{
    // Configure GPU clock (similar to gpu_bsp_init)
    #define SYS_M55_BASE_ADDR    (0x48000000)
    uint32_t reg = REG_READ(SYS_M55_BASE_ADDR + 0x09 * 4);
    reg |= (0x01<<13);  // Set clock selection 0：320M 1：480M
    REG_WRITE(SYS_M55_BASE_ADDR + 0x09 * 4, reg);

    reg = REG_READ(SYS_M55_BASE_ADDR + 0x09 * 4);
    reg |= (0x0<<14);  // clk div
    REG_WRITE(SYS_M55_BASE_ADDR + 0x09 * 4, reg);

    reg = REG_READ(SYS_M55_BASE_ADDR + 0x0A * 4);
    reg |= (0x01<<10);  // Enable GPU clock
    REG_WRITE(SYS_M55_BASE_ADDR + 0x0A * 4, reg);

    bk_int_isr_register(INT_SRC_GPU, vg_lite_IRQHandler, NULL);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPU, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPU, 1);
#endif
}

static void gpu_test_addr_mapping_config(uint16_t width, uint16_t height, uint8_t buf_cnt, uint32_t y_addr)
{
    uint32_t u_addr = y_addr + width * FLEXA_LINES * buf_cnt;
    *(volatile uint32_t *)(0x48000000 + 0x28 * 4) = 1;
    *(volatile uint32_t *)(0x48000000 + 0x29 * 4) = y_addr;
    *(volatile uint32_t *)(0x48000000 + 0x2A * 4) = width * buf_cnt * FLEXA_LINES;
    *(volatile uint32_t *)(0x48000000 + 0x2B * 4) = GPU_Y_VADDR_BASE;
    *(volatile uint32_t *)(0x48000000 + 0x2C * 4) = GPU_Y_VADDR_BASE + width * FLEXA_LINES * buf_cnt / 2;
    *(volatile uint32_t *)(0x48000000 + 0x2D * 4) = GPU_Y_VADDR_BASE + width * height;
    *(volatile uint32_t *)(0x48000000 + 0x30 * 4) = 1;
    *(volatile uint32_t *)(0x48000000 + 0x31 * 4) = u_addr;
    *(volatile uint32_t *)(0x48000000 + 0x32 * 4) = width * buf_cnt * FLEXA_LINES / 2;
    *(volatile uint32_t *)(0x48000000 + 0x33 * 4) = GPU_Y_VADDR_BASE + width * height;
    *(volatile uint32_t *)(0x48000000 + 0x34 * 4) = GPU_Y_VADDR_BASE + width * height + width * FLEXA_LINES * buf_cnt / 4;
    *(volatile uint32_t *)(0x48000000 + 0x35 * 4) = GPU_Y_VADDR_BASE + width * height + width * height / 2;
}

static vg_lite_buffer_format_t gpu_test_vg_lite_format_convert(bk_pixel_format_t bk_format)
{

    switch (bk_format)
    {
        case BK_PIXEL_FORMAT_RGGB8:
            return -1;
        case BK_PIXEL_FORMAT_GRBG8:
            return -1;
        case BK_PIXEL_FORMAT_GBRG8:
            return -1;
        case BK_PIXEL_FORMAT_BGGR8:
            return -1;
        case BK_PIXEL_FORMAT_RAW8:
            return -1;

        case BK_PIXEL_FORMAT_RGGB10:
            return -1;
        case BK_PIXEL_FORMAT_GRBG10:
            return -1;
        case BK_PIXEL_FORMAT_GBRG10:
            return -1;
        case BK_PIXEL_FORMAT_BGGR10:
            return -1;
        case BK_PIXEL_FORMAT_RAW10:
            return -1;

        case BK_PIXEL_FORMAT_RGB565:
            return VG_LITE_BGR565;
        case BK_PIXEL_FORMAT_BGR565:
            return VG_LITE_RGB565;
        case BK_PIXEL_FORMAT_ARGB8565:
            return -1;
        case BK_PIXEL_FORMAT_ABGR8565:
            return -1;
        case BK_PIXEL_FORMAT_RGBA5658:
            return -1;
        case BK_PIXEL_FORMAT_BGRA5658:
            return -1;


        case BK_PIXEL_FORMAT_RGB888:
            return VG_LITE_RGB888;
        case BK_PIXEL_FORMAT_BGR888:
            return VG_LITE_BGR888;

        case BK_PIXEL_FORMAT_ARGB8888:
            return VG_LITE_BGRA8888;
        case BK_PIXEL_FORMAT_ABGR8888:
            return VG_LITE_BGRA8888;
        case BK_PIXEL_FORMAT_RGBA8888:
            return VG_LITE_BGRA8888;
        case BK_PIXEL_FORMAT_BGRA8888:
            return VG_LITE_BGRA8888;


        case BK_PIXEL_FORMAT_NV12:
            return VG_LITE_NV12;
        case BK_PIXEL_FORMAT_NV21:
            return -1;
        case BK_PIXEL_FORMAT_YUYV:
            return VG_LITE_YUYV;
        case BK_PIXEL_FORMAT_VYUY:
            return -1;
        case BK_PIXEL_FORMAT_UYVY:
            return -1;
        case BK_PIXEL_FORMAT_YYUV:
            return -1;

        default:
            return -1;
    }

    return -1;
}

#define CHECK_ERROR
static int gpu_run_once(bk_gpu_ctlr_config_t *config)
{
    vg_lite_matrix_t matrix;

    vg_lite_buffer_t buffer;
    vg_lite_buffer_t image;

    memset(&image, 0, sizeof(image));
    image.width = 16;
    image.height = 16;
    image.format = gpu_test_vg_lite_format_convert(config->dst_format);
    CHECK_ERROR(vg_lite_allocate(&image));

    memset(&buffer, 0, sizeof(vg_lite_buffer_t));
    buffer.width  = 16;
    buffer.height = 16;
    buffer.format = gpu_test_vg_lite_format_convert(config->dst_format);
#if CONFIG_USE_DECNANO_V2P1 // dec v2
    buffer.format = VG_LITE_BGRA8888;
    buffer.tiled = 1;
    buffer.compress_mode = VG_LITE_DEC_HV_SAMPLE;
#endif
    CHECK_ERROR(vg_lite_allocate(&buffer));

    vg_lite_identity(&matrix);

    CHECK_ERROR(vg_lite_blit(&buffer, &image, &matrix, VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT));
    CHECK_ERROR(vg_lite_finish());

    if (buffer.handle != NULL)
    {
        vg_lite_free(&buffer);
    }

    if (image.handle != NULL)
    {
        vg_lite_free(&image);
    }

    return 0;
}

static avdk_err_t gpu_blit_set(bk_gpu_ctlr_handle_t handle, void *src_buffer, bk_gpu_blit_config_t *blit_config)
{
    gpu_vn_ctlr_v2_t *controller =  __containerof(handle, gpu_vn_ctlr_v2_t, ops);

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
    gpu_vn_ctlr_v2_t *controller =  __containerof(handle, gpu_vn_ctlr_v2_t, ops);

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
    gpu_vn_ctlr_v2_t *controller =  __containerof(handle, gpu_vn_ctlr_v2_t, ops);
    gpu_flex_data_v2_t *flex = &controller->flex;


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
    gpu_vn_ctlr_v2_t *controller =  __containerof(handle, gpu_vn_ctlr_v2_t, ops);
    gpu_flex_data_v2_t *flex = &controller->flex;

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

static void gpu_dec_line_done_callback(uint32_t line, void *arg)
{
    gpu_vn_ctlr_v2_t *gpu_vn_ctlr = (gpu_vn_ctlr_v2_t *)arg;

    if (line == 1)
    {
        if (gpu_vn_ctlr->dec_line_err_flag)
        {
            gpu_vn_ctlr->dec_line_err_flag = false;
        }
    }

    gpu_vn_ctlr->dec_line_cnt = line;
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
static int gpu_flex_init_gpu_buffer(gpu_flex_data_v2_t *data)
{
#ifdef CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ
        uint32_t gpu_buffer_len = CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ;
#else
        uint32_t gpu_buffer_len = data->output_width_x_flexa_lines * 2 + BUFFER_ALIGNMENT_SIZE;
#endif
    vg_lite_set_buffer(bk_get_gpu_flexa_buffer(gpu_buffer_len));

    return 0;
}

/**
 * @brief Initialize ping-pong buffer for GPU processing
 * @param data GPU flex data structure
 * @return 0 on success, -1 on failure
 */
static int gpu_flex_init_pingpong_buffer(gpu_flex_data_v2_t *data)
{
    uintptr_t base_addr = (uintptr_t)bk_get_gpu_output_buffer(data->output_width_x_flexa_lines * 2 + BUFFER_ALIGNMENT_SIZE);
    os_memset((void *)base_addr, 0, data->output_width_x_flexa_lines * 2 + BUFFER_ALIGNMENT_SIZE);
    data->buffers[0] = align_buffer_address(base_addr);
    data->buffers[1] = align_buffer_address(base_addr + data->output_width_x_flexa_lines);
    return 0;
}

/**
 * @brief Configure destination buffer based on rotation angle
 * @param data GPU flex data structure
 * @param rotation_degree Rotation angle (0 or 90)
 */
static void gpu_flex_configure_dst_buffer(gpu_flex_data_v2_t *data, bk_gpu_ctlr_config_t *config)
{
    memset(&data->dst_buf, 0, sizeof(vg_lite_buffer_t));

    if (config->rotate_degree == 90)
    {
        data->dst_buf.width  = FLEXA_LINES;
        data->dst_buf.height = data->output_width;
    }
    else /* config->rotate_degree == 0 */
    {
        data->dst_buf.width  = data->output_width;
        data->dst_buf.height = FLEXA_LINES;
    }

    data->dst_buf.compress_mode = config->compress ? VG_LITE_DEC_HV_SAMPLE : VG_LITE_DEC_DISABLE;
    data->dst_buf.format = gpu_test_vg_lite_format_convert(config->dst_format);


    data->dst_buf.tiled = VG_LITE_TILED;
    vg_lite_allocate_with_data(&data->dst_buf,
                                (void *)(uintptr_t)data->buffers[data->dst_buf_idx],
                                NULL, NULL, NULL);
}

/**
 * @brief Initialize DMA for rotation operations
 * @param data GPU flex data structure
 * @param rotation_degree Rotation angle
 */
static void gpu_flex_init_dma_for_rotation(gpu_flex_data_v2_t *data, uint16_t rotation_degree)
{
    LOGI("rotate angle %d \r\n", rotation_degree);

    data->gdma = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (data->gdma >= HPDMA_ID_MAX)
    {
        LOGE("%s, %d bk_hpdma_alloc failed\n", __func__, __LINE__);
        return;
    }

    if (rotation_degree != 0)
    {
        data->link_dma_list_table = bk_hpdma_link_init(1);
        if (data->link_dma_list_table == NULL)
        {
            LOGE("%s, %d bk_hpdma_link_init failed\n", __func__, __LINE__);
            bk_hpdma_free(HPDMA_DEV_DTCM, data->gdma);
            return;
        }

        bk_hpdma_set_dest_burst_len(data->gdma, 0x03);
        bk_hpdma_set_src_burst_len(data->gdma, 0x03);
    }
}

/**
 * @brief Update transformation matrix for current processing line
 * @param data GPU flex data structure
 * @param config GPU controller configuration
 * @param draw_matrix Optional box matrix for face detection (can be NULL)
 */
static void gpu_flex_update_matrix(gpu_flex_data_v2_t *data, 
                                    const bk_gpu_ctlr_config_t *config)
{
    if (config->rotate_degree == 0)
    {
        float offset = -((float)(data->flexa_index - 1) * FLEXA_LINES);
        data->matrix.m[1][2] = offset;

        if (data->draw_enable)
        {
            data->draw_matrix.m[1][2] = offset;
        }
    }
    else if (config->rotate_degree == 90)
    {
        float offset = ((float)(data->flexa_index) * FLEXA_LINES);
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
static inline void gpu_flex_data_init(gpu_flex_data_v2_t *data, gpu_vn_ctlr_v2_t *gpu_vn_ctlr)
{
    bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;

    /* Initialize basic parameters */
    gpu_vn_ctlr->dec_line_cnt = 0;
    data->input_width = config->src_width;
    data->input_height = config->src_height;
    data->output_width = config->dst_width;
    data->output_height = (config->dst_height + GPU_HIGHT_ALIGNMENT) & ~GPU_HIGHT_ALIGNMENT;
    data->flexa_index = 1;
    data->read_lines = 0;
    data->dst_buf_idx = 0;
    LOGI("config->compress %d, input_width %d input_height %d output_width %d output_height %d\r\n", config->compress, data->input_width, data->input_height, data->output_width, data->output_height);
    /* Cache frequently used calculations */
    data->output_width_x_flexa_lines = config->compress ? data->output_width * FLEXA_LINES : bk_pixel_size_get(config->dst_format) * data->output_width * FLEXA_LINES;
    data->flexa_lines_x_4 = bk_pixel_size_get(config->dst_format) * FLEXA_LINES;
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

    /* Get input dimensions from DECODE */
    uint8_t *decode_buf = NULL;
    uint8_t decode_buf_cnt = 0;
    if (decode_test_get_decode_context(&decode_buf, &decode_buf_cnt) != AVDK_ERR_OK) {
        LOGE("%s, %d, get decode flexa buffer failed\n", __func__, __LINE__);
        return;
    }
    gpu_test_addr_mapping_config(data->input_width, data->input_height, decode_buf_cnt, (uint32_t)decode_buf);

    /* Initialize GPU buffer */
    if (gpu_flex_init_gpu_buffer(data) != 0)
    {
        rtos_delete_thread(NULL);
        return;
    }

    /* Initialize VG-Lite */
#if CONFIG_VG_LITE_TESSELLATION
    vg_lite_init(data->input_width / 4, data->input_height / 4);
#else
    vg_lite_init(0, 0);
#endif

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
    gpu_flex_init_dma_for_rotation(data, config->rotate_degree);

    /* Configure source buffer */
    data->src_buf.width = data->input_width;
    data->src_buf.height = data->input_height;
    data->src_buf.format = gpu_test_vg_lite_format_convert(config->src_format);
    data->src_buf.compress_mode = VG_LITE_DEC_DISABLE;
    data->src_buf.tiled = VG_LITE_LINEAR;
    vg_lite_allocate_with_data(&data->src_buf,
                               (void *)(uintptr_t)GPU_Y_VADDR_BASE,
                               (void *)(uintptr_t)(GPU_Y_VADDR_BASE + data->input_width_x_height),
                               NULL, NULL);

    /* Initialize ping-pong buffer */
    if (gpu_flex_init_pingpong_buffer(data) != 0)
    {
        /* Non-fatal error, continue */
    }

    /* Configure destination buffer */
    gpu_flex_configure_dst_buffer(data, config);

    /* Pre-load GPU code into cache */
    //gpu_run_once(config);

    /* Register Decoder callback */
    decode_test_register_isr_callback(gpu_dec_line_done_callback, gpu_vn_ctlr);
}

static inline void gpu_flex_data_deinit(gpu_flex_data_v2_t *data, gpu_vn_ctlr_v2_t *gpu_vn_ctlr)
{
    // TODO: free decode register callback

    rtos_deinit_semaphore(&data->transfer_sem);
    rtos_deinit_mutex(&data->draw_mutex);
}

/**
 * @brief Pull out processed line data from GPU buffer
 * @param data GPU flex data structure
 * @param gpu_vn_ctlr GPU controller handle
 */
static inline void gpu_flex_data_line_pull_out(gpu_flex_data_v2_t *data, gpu_vn_ctlr_v2_t *gpu_vn_ctlr)
{
    const bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;

    /* Calculate read lines */
    data->read_lines = data->need_lines / data->output_height;

    /* Wait for DMA to complete if rotation is enabled */
    while(bk_hpdma_get_next_ll_addr(data->gdma));
    while(bk_hpdma_get_enable_status(data->gdma));

    /* Copy processed data based on rotation angle */
    if (config->rotate_degree == 90)
    {
        uint32_t offset = 0;
        uint32_t stride = 0;
        if (config->compress)
        {
            uint32_t output_height_div_flexa = data->output_height / FLEXA_LINES;
            offset = (output_height_div_flexa - data->flexa_index) * data->flexa_lines_x_4;
            stride = data->output_height * bk_pixel_size_get(config->dst_format) - data->flexa_lines_x_4;
        }
        else
        {
            stride = (data->output_height - FLEXA_LINES) * bk_pixel_size_get(config->dst_format);
            offset = (data->output_height - data->flexa_index * FLEXA_LINES) * bk_pixel_size_get(config->dst_format);
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

        BK_LOG_ON_ERR(bk_hpdma_link_set_descs(data->link_dma_list_table, (hpdma_link_config_t *)dma_config, 1));

        /* Start transfer */
        BK_LOG_ON_ERR(bk_hpdma_link_transfer(data->gdma, data->link_dma_list_table));
        /* Wait for transfer complete */
    }
    else if (config->rotate_degree == 0)
    {
        /* For 0-degree rotation, use simple DMA copy */
        uint32_t offset = (data->flexa_index - 1) * data->output_width_x_flexa_lines;
        uint32_t copy_size = data->output_width_x_flexa_lines;

        bk_hpdma_memcpy(data->dpu_frame_buffers + offset,
                                data->dst_buf.memory,
                                copy_size);
    }

    /* Switch to next ping-pong buffer */
    data->dst_buf_idx = 1 - data->dst_buf_idx;
    data->dst_buf.memory = (vg_lite_pointer)(uintptr_t)data->buffers[data->dst_buf_idx];
    data->dst_buf.address = data->buffers[data->dst_buf_idx];
    data->flexa_index++;
}

static void gpu_flex_data_frame_done_blit(gpu_flex_data_v2_t *flex, const bk_gpu_ctlr_config_t *config, bk_gpu_blit_config_t *blit_config, void *front_frame, void *display_frame)
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
    front_buffer.format = gpu_test_vg_lite_format_convert(blit_config->src_format);
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
static inline void gpu_flex_data_frame_done(gpu_flex_data_v2_t *data, gpu_vn_ctlr_v2_t *gpu_vn_ctlr)
{
    const bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;

    /* Wait for DMA to complete if rotation is enabled */
    while(bk_hpdma_get_next_ll_addr(data->gdma));
    while(bk_hpdma_get_enable_status(data->gdma));

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
    uint32_t frame_size = bk_pixel_size_get(config->dst_format) *
                          (config->compress ? data->output_width / 4 : data->output_width) *
                          data->output_height;
    void *new_buffer = config->malloc(frame_size);
    //void *new_buffer = NULL;
    if (new_buffer != NULL)
    {
        if (config->frame_display == NULL)
        {
            LOGE("%s, %d frame_display is NULL\n", __func__, __LINE__);
            config->free(new_buffer);
            return;
        }

        config->frame_display(data->dpu_frame_buffers, frame_size, config->frame_display_args);
        data->dpu_frame_buffers = new_buffer;
        AVDK_MONITOR_GPU_FRAME_PLUS();
    }
    /* Reset state for next frame */
    //rtos_get_semaphore(&gpu_vn_ctlr->gpu_process_sem, BEKEN_NO_WAIT);
    data->flexa_index = 1;
    data->read_lines = 0;
}

/**
 * @brief Check if enough ISP lines are available for processing
 * @param data GPU flex data structure
 * @param isp_line_count Current ISP line count
 * @return 1 if enough lines available, 0 otherwise
 */
static inline int gpu_flex_has_enough_lines(gpu_flex_data_v2_t *data, uint32_t decode_line_count)
{
    data->need_lines = data->flexa_index * FLEXA_LINES * data->input_height;
    uint32_t decode_lines = decode_line_count * FLEXA_LINES;
    uint32_t required_lines = (data->need_lines + data->output_height - 1) / data->output_height;

    return (decode_lines >= required_lines) ? 1 : 0;
}

/**
 * @brief Process a single line block with GPU
 * @param data GPU flex data structure
 * @param gpu_vn_ctlr GPU controller handle
 */
static void gpu_flex_process_line_block(gpu_flex_data_v2_t *data,
                                        gpu_vn_ctlr_v2_t *gpu_vn_ctlr)
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
    HPDMA_LINE_END();
}

/**
 * @brief Main GPU flex processing entry point
 * @param arg GPU controller handle (gpu_vn_ctlr_v2_t *)
 */
static void gpu_flex_main_entry(void *arg)
{
    gpu_vn_ctlr_v2_t *gpu_vn_ctlr = (gpu_vn_ctlr_v2_t *)arg;
    const bk_gpu_ctlr_config_t *config = &gpu_vn_ctlr->config;
    gpu_flex_data_v2_t *flex = &gpu_vn_ctlr->flex;

    LOGD("%s, input:%dX%d, output:%dX%d, rotate_degree:%d, compress:%d, scale:%d, src_format:%d, dst_format:%d\n",
         __func__, config->src_width, config->src_height, config->dst_width, config->dst_height,
         config->rotate_degree, config->compress, config->scale, config->src_format, config->dst_format);

    /* Initialize GPU hardware */
    gpu_test_int_config();
    gpu_flex_data_init(flex, gpu_vn_ctlr);
    rtos_init_semaphore_ex(&gpu_vn_ctlr->gpu_process_sem, 1, 0);

    /* Initialize face detection matrix */
    vg_lite_identity(&flex->draw_matrix);
    vg_lite_rotate((float)config->rotate_degree, &flex->draw_matrix);

    /* Allocate initial frame buffer */
    uint32_t frame_size = bk_pixel_size_get(config->dst_format) *
                          (config->compress ? flex->output_width / 4 : flex->output_width) *
                          flex->output_height;
    flex->dpu_frame_buffers = config->malloc(frame_size);
    if (flex->dpu_frame_buffers == NULL)
    {
        LOGE("Failed to allocate flex->dpu_frame_buffers, size %d(%d * %d)\r\n",
             frame_size, flex->output_width, flex->output_height);
        return;
    }
    LOGI("flex->dpu_frame_buffers %p, size %d(%d * %d), format %d, compress %d\r\n",
         flex->dpu_frame_buffers, frame_size, flex->output_width, flex->output_height,
         config->dst_format, config->compress);
    gpu_vn_ctlr->dec_line_err_flag = true; // reset error flag
    AVDK_MONITOR_GPU_ENABLE();

    /* Main processing loop */
    while (1)
    {
        /* Wait for semaphore from ISP callback */
        rtos_get_semaphore(&gpu_vn_ctlr->gpu_process_sem, BEKEN_WAIT_FOREVER);

        uint32_t decode_line_count = gpu_vn_ctlr->dec_line_cnt;
        if (decode_line_count == 1)
        {
            flex->read_lines = 0;
            flex->flexa_index = 1;
            GPU_FRAME_START();
        }

        /* Handle line error flag */
        if (gpu_vn_ctlr->dec_line_err_flag)
        {
            // TODO: write decode context to flexa buffer
            // LOGD("%s, %d, write decode flexa index, dec_line_cnt %d\n", __func__, __LINE__, decode_line_count);
            decode_test_set_wr_cnt(decode_line_count);
            continue;
        }

        /* Process available line blocks */
        while (gpu_flex_has_enough_lines(flex, decode_line_count))
        {
            gpu_flex_process_line_block(flex, gpu_vn_ctlr);
            AVDK_MONITOR_GPU_LINE_PLUS();
        }

        /* Check if frame is complete */
        if (flex->read_lines >= flex->input_height)
        {
            //LOGD("%s, %d, frame complete, dec_lines %d\n", __func__, __LINE__, decode_line_count);
            gpu_flex_data_frame_done(flex, gpu_vn_ctlr);
            GPU_FRAME_END();
        }

        //LOGD("%s, %d, read_lines %d, input_height %d, dec_lines %d\n", __func__, __LINE__, flex->read_lines, flex->input_height, decode_line_count);
        uint32_t gpu_rd_cnt = flex->read_lines / FLEXA_LINES;
        decode_test_set_wr_cnt(gpu_rd_cnt);
    }

    /* Cleanup (should never reach here in normal operation) */
    LOGW("%s,%d exit\n", __func__, __LINE__);
    os_free(arg);

    if (config->rotate_degree != 0)
    {
        bk_hpdma_free(HPDMA_DEV_DTCM, flex->gdma);
        bk_hpdma_link_deinit(flex->link_dma_list_table);
    }

    gpu_flex_data_deinit(flex, gpu_vn_ctlr);
    rtos_delete_thread(NULL);
}

static avdk_err_t gpu_ctlr_init(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_v2_t *control =  __containerof(handle, gpu_vn_ctlr_v2_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    //TODO
    control->blit_enable = false;
    control->display_blit_buffer = NULL;
    os_memset(&control->display_blit_config, 0, sizeof(bk_gpu_blit_config_t));
    control->update_blit_buffer = NULL;
    os_memset(&control->update_blit_config, 0, sizeof(bk_gpu_blit_config_t));
    rtos_init_mutex(&control->blit_mutex);

    return AVDK_ERR_OK;
}


static avdk_err_t gpu_ctlr_open(bk_gpu_ctlr_handle_t handle)
{
    gpu_vn_ctlr_v2_t *control =  __containerof(handle, gpu_vn_ctlr_v2_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");



    AVDK_RETURN_ON_ERROR(rtos_create_hsram_thread(&control->flexa_thd,
                       1,
                       "gpu",
                       (beken_thread_function_t)gpu_flex_main_entry,
                       1024 * 10,
                       control), TAG, "create flexa thread failed");
    return AVDK_ERR_OK;
}

avdk_err_t bk_gpu_test_ctlr_new(bk_gpu_ctlr_handle_t *handle, bk_gpu_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    gpu_vn_ctlr_v2_t *controller = os_malloc(sizeof(gpu_vn_ctlr_v2_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(gpu_vn_ctlr_v2_t));

    os_memcpy(&controller->config, config, sizeof(bk_gpu_ctlr_config_t));
    controller->ops.init = gpu_ctlr_init;
    controller->ops.open = gpu_ctlr_open;
    controller->ops.draw_path_clear = gpu_draw_path_clear;
    controller->ops.draw_path_build = gpu_draw_path_build;
    controller->ops.blit_set = gpu_blit_set;
    controller->ops.blit_clear = gpu_blit_clear;
    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}

avdk_err_t bk_gpu_test_init(bk_gpu_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->init, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->init(handle);
}

avdk_err_t bk_gpu_test_open(bk_gpu_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->open, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->open(handle);
}