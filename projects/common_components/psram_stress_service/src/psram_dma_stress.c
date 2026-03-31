#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "driver/hal/hal_hpdma_types.h"
#include <driver/hpdma.h>
#include "psram_dma_stress.h"

#define TAG "psram_dma_stress"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

struct psram_dma_stress {
    hpdma_id_t dma_id;
    void *desc_table;
    uint8_t *src_addr;
    uint8_t *dst_addr;
    uint32_t transfer_size;
    uint8_t running;
    uint8_t deiniting;
};

static void psram_dma_stress_cleanup_locked(psram_dma_stress_handle_t handle)
{
    if (handle->dma_id < HPDMA_ID_MAX) {
        bk_hpdma_register_isr(handle->dma_id, NULL, NULL, NULL, NULL);
        bk_hpdma_free(HPDMA_DEV_DTCM, handle->dma_id);
        handle->dma_id = HPDMA_ID_MAX;
    }

    if (handle->desc_table) {
        bk_hpdma_link_deinit(handle->desc_table);
        handle->desc_table = NULL;
    }

    handle->running = 0;
    handle->src_addr = NULL;
    handle->dst_addr = NULL;
    handle->transfer_size = 0;
}

static void psram_dma_stress_complete_cb(hpdma_id_t hpdma_id, void *user_data)
{
    psram_dma_stress_handle_t handle = (psram_dma_stress_handle_t)user_data;
    bk_err_t ret;

    if (handle == NULL) {
        LOGE("dma callback handle is NULL, id=%d\n", hpdma_id);
        return;
    }

    /*
     * This callback runs in interrupt context.
     * Do not take mutex or perform heavy cleanup here.
     * Only restart transfer when state looks valid; otherwise just return.
     */
    if (handle->deiniting || !handle->running || handle->desc_table == NULL || handle->dma_id >= HPDMA_ID_MAX) {
        return;
    }

    LOGI("dma callback handle is running, id=%d\n", hpdma_id);

    /*
     * Continuous stress mode:
     * Start next transfer in finish callback for sustained pressure.
     */
    ret = bk_hpdma_link_transfer(handle->dma_id, handle->desc_table);
    if (ret != BK_OK) {
        LOGE("restart dma transfer failed, ret=%d id=%d\n", ret, handle->dma_id);
        /* Mark as stopped; cleanup will be handled in stop/deinit from task context. */
        handle->running = 0;
    }
}

bk_err_t psram_dma_stress_create(psram_dma_stress_handle_t *out_handle)
{
    psram_dma_stress_handle_t handle;

    if (out_handle == NULL) {
        return BK_ERR_PARAM;
    }

    handle = (psram_dma_stress_handle_t)os_malloc(sizeof(struct psram_dma_stress));
    if (handle == NULL) {
        return BK_ERR_NO_MEM;
    }

    os_memset(handle, 0, sizeof(*handle));
    handle->dma_id = HPDMA_ID_MAX;

    *out_handle = handle;
    return BK_OK;
}

bk_err_t psram_dma_stress_start(psram_dma_stress_handle_t handle,
                                uint8_t *src_addr, uint8_t *dst_addr, uint32_t size)
{
    hpdma_link_config_t config;
    bk_err_t ret;
    hpdma_id_t dma_id;
    void *desc_table = NULL;

    if (handle == NULL || src_addr == NULL || dst_addr == NULL || size == 0) {
        LOGE("invalid dma start params, handle=%p src=%p dst=%p size=%u\n", handle, src_addr, dst_addr, size);
        return BK_ERR_PARAM;
    }

    if (handle->deiniting || handle->running) {
        return BK_ERR_BUSY;
    }

    desc_table = bk_hpdma_link_init(1);
    if (desc_table == NULL) {
        LOGE("bk_hpdma_link_init failed\n");
        return BK_FAIL;
    }

    os_memset(&config, 0, sizeof(config));
    config.src_addr = (uint32_t)src_addr;
    config.dst_addr = (uint32_t)dst_addr;

    /* 
     * Configure xsize and ysize with following constraints:
     * 1) xsize <= 0xFFFF
     * 2) ysize <= 0xFFFF
     * 3) xsize * ysize == size
     */
    if (size <= 0xFFFFu) {
        /* Small size: single line transfer. */
        config.src_xsize = size;
        config.src_ysize = 1;
        config.dst_xsize = size;
        config.dst_ysize = 1;
    } else {
        uint32_t xsize = 0;
        uint32_t ysize = 0;
        uint32_t y;

        /* Try to find factors: size = x * y, with x <= 0xFFFF, y <= 0xFFFF. */
        for (y = 0xFFFFu; y >= 1u; y--) {
            if ((size % y) == 0u) {
                uint32_t x = size / y;
                if (x <= 0xFFFFu) {
                    xsize = x;
                    ysize = y;
                    break;
                }
            }
            if (y == 1u) {
                break;
            }
        }

        if (xsize == 0u || ysize == 0u) {
            LOGE("no valid xsize/ysize factors for size=%u\n", size);
            bk_hpdma_link_deinit(desc_table);
            return BK_ERR_PARAM;
        }

        config.src_xsize = xsize;
        config.src_ysize = ysize;
        config.dst_xsize = xsize;
        config.dst_ysize = ysize;
    }

    config.src_step = 0;
    config.dst_step = 0;
    config.finish_int_en = 1;
    config.half_finish_int_en = 0;

    ret = bk_hpdma_link_set_descs(desc_table, &config, 1);
    if (ret != BK_OK) {
        LOGE("bk_hpdma_link_set_descs failed, ret=%d\n", ret);
        bk_hpdma_link_deinit(desc_table);
        return ret;
    }

    dma_id = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (dma_id >= HPDMA_ID_MAX) {
        LOGE("bk_hpdma_alloc failed\n");
        bk_hpdma_link_deinit(desc_table);
        return BK_FAIL;
    }

    ret = bk_hpdma_register_isr(dma_id, NULL, NULL, psram_dma_stress_complete_cb, handle);
    if (ret != BK_OK) {
        LOGE("bk_hpdma_register_isr failed, ret=%d id=%d\n", ret, dma_id);
        bk_hpdma_free(HPDMA_DEV_DTCM, dma_id);
        bk_hpdma_link_deinit(desc_table);
        return ret;
    }

    ret = bk_hpdma_enable_finish_interrupt(dma_id);
    if (ret != BK_OK) {
        LOGE("bk_hpdma_enable_finish_interrupt failed, ret=%d id=%d\n", ret, dma_id);
        bk_hpdma_register_isr(dma_id, NULL, NULL, NULL, NULL);
        bk_hpdma_free(HPDMA_DEV_DTCM, dma_id);
        bk_hpdma_link_deinit(desc_table);
        return ret;
    }

    if (handle->deiniting || handle->running) {
        bk_hpdma_register_isr(dma_id, NULL, NULL, NULL, NULL);
        bk_hpdma_free(HPDMA_DEV_DTCM, dma_id);
        bk_hpdma_link_deinit(desc_table);
        return BK_ERR_BUSY;
    }

    handle->dma_id = dma_id;
    handle->desc_table = desc_table;
    handle->src_addr = src_addr;
    handle->dst_addr = dst_addr;
    handle->transfer_size = size;
    handle->running = 1;

    ret = bk_hpdma_link_transfer(dma_id, desc_table);
    if (ret != BK_OK) {
        LOGE("bk_hpdma_link_transfer first start failed, ret=%d id=%d\n", ret, dma_id);
        psram_dma_stress_cleanup_locked(handle);
        return ret;
    }

    LOGI("dma stress started, handle=%p id=%d src=%p dst=%p size=%u\n",
         handle, dma_id, src_addr, dst_addr, size);
    return BK_OK;
}

bk_err_t psram_dma_stress_stop(psram_dma_stress_handle_t handle)
{
    if (handle == NULL) {
        return BK_ERR_PARAM;
    }

    if (!handle->running) {
        return BK_OK;
    }

    psram_dma_stress_cleanup_locked(handle);
    return BK_OK;
}

bk_err_t psram_dma_stress_deinit(psram_dma_stress_handle_t handle)
{
    if (handle == NULL) {
        return BK_ERR_PARAM;
    }

    handle->deiniting = 1;
    psram_dma_stress_cleanup_locked(handle);
    os_free(handle);
    return BK_OK;
}

