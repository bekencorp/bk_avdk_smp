/**
 * @file lv_hpdma.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "hpdma/lv_hpdma.h"
#include <driver/hpdma.h>
#include <driver/hal/hal_hpdma_types.h>

#define TAG "LVGL_HPDMA"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static lv_vnd_data_t *s_hpdma_vnd_data = NULL;

static lv_vnd_data_t *lv_hpdma_get_vnd_data(void)
{
#if CONFIG_LVGL_V8
    lv_disp_t *disp = lv_disp_get_default();
    if ((disp == NULL) || (disp->driver == NULL)) {
        return s_hpdma_vnd_data;
    }

#if LV_USE_USER_DATA
    if (disp->driver->user_data != NULL) {
        return (lv_vnd_data_t *)disp->driver->user_data;
    }
#else
    return s_hpdma_vnd_data;
#endif
    return s_hpdma_vnd_data;
#else
    return (lv_vnd_data_t *)lv_display_get_user_data(lv_disp_get_default());
#endif
}

static void lv_hpdma_transfer_complete_callback(hpdma_id_t hpdma_id, void *user_data)
{
    lv_vnd_data_t *vnd_data = lv_hpdma_get_vnd_data();
    if (vnd_data == NULL) {
        LOGE("%s %d vnd_data is NULL\n", __func__, __LINE__);
        return;
    }

    if (vnd_data->lv_hpdma_sem != NULL) {
        rtos_set_semaphore(&vnd_data->lv_hpdma_sem);
    }
}

void lv_hpdma_memcpy_init(lv_vnd_data_t *vnd_data)
{
    if (vnd_data == NULL) {
        LOGE("%s %d vnd_data is NULL\n", __func__, __LINE__);
        return;
    }

    s_hpdma_vnd_data = vnd_data;

    vnd_data->link_dma_list_table = bk_hpdma_link_init(1);
    if (vnd_data->link_dma_list_table == NULL)
    {
        LOGE("%s, %d bk_hpdma_link_init failed\n", __func__, __LINE__);
        return;
    }

    vnd_data->lv_hpdma_id = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (vnd_data->lv_hpdma_id >= HPDMA_ID_MAX)
    {
        LOGE("%s, %d bk_hpdma_alloc failed\n", __func__, __LINE__);
        bk_hpdma_link_deinit(vnd_data->link_dma_list_table);
        vnd_data->link_dma_list_table = NULL;
        return;
    }

    bk_err_t ret = rtos_init_semaphore_ex(&vnd_data->lv_hpdma_sem, 1, 0);
    if (BK_OK != ret) {
        LOGE("%s vnd_data->lv_hpdma_sem init failed\n", __func__);
        bk_hpdma_free(HPDMA_DEV_DTCM, vnd_data->lv_hpdma_id);
        vnd_data->lv_hpdma_id = HPDMA_ID_MAX;
        bk_hpdma_link_deinit(vnd_data->link_dma_list_table);
        vnd_data->link_dma_list_table = NULL;
        return;
    }
}

void lv_hpdma_memcpy_deinit(lv_vnd_data_t *vnd_data)
{
    if (vnd_data == NULL) {
        LOGE("%s %d vnd_data is NULL\n", __func__, __LINE__);
        return;
    }

    bk_hpdma_stop(vnd_data->lv_hpdma_id);
    bk_hpdma_disable_finish_interrupt(vnd_data->lv_hpdma_id);

    bk_hpdma_link_deinit(vnd_data->link_dma_list_table);
    vnd_data->link_dma_list_table = NULL;

    bk_hpdma_free(HPDMA_DEV_DTCM, vnd_data->lv_hpdma_id);
    vnd_data->lv_hpdma_id = HPDMA_ID_MAX;

    if (vnd_data->lv_hpdma_sem != NULL) {
        rtos_deinit_semaphore(&vnd_data->lv_hpdma_sem);
        vnd_data->lv_hpdma_sem = NULL;
    }

    vnd_data->lv_hpdma_in_use = false;
    if (s_hpdma_vnd_data == vnd_data) {
        s_hpdma_vnd_data = NULL;
    }
}

bk_err_t lv_hpdma_memcpy_start(void *src_buf, void *dst_buf, uint16_t src_xsize, uint16_t src_ysize,
                               uint16_t dst_xsize, uint16_t dst_ysize, uint16_t src_step, uint16_t dst_step)
{
    hpdma_link_config_t config = {0};

    lv_vnd_data_t *vnd_data = lv_hpdma_get_vnd_data();
    if (vnd_data == NULL) {
        LOGE("%s %d vnd_data is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if ((vnd_data->link_dma_list_table == NULL) || (vnd_data->lv_hpdma_id >= HPDMA_ID_MAX) || (vnd_data->lv_hpdma_sem == NULL)) {
        LOGE("%s hpdma not ready: table=%p id=%d sem=%p\n", __func__, vnd_data->link_dma_list_table, vnd_data->lv_hpdma_id, vnd_data->lv_hpdma_sem);
        return BK_ERR_STATE;
    }

    if ((src_buf == NULL) || (dst_buf == NULL) || (src_xsize == 0) || (src_ysize == 0) || (dst_xsize == 0) || (dst_ysize == 0)) {
        LOGE("%s invalid param: src=%p dst=%p sx=%u sy=%u dx=%u dy=%u\n", __func__, src_buf, dst_buf, src_xsize, src_ysize, dst_xsize, dst_ysize);
        return BK_ERR_PARAM;
    }

    config.src_addr = (uint32_t)src_buf;
    config.dst_addr = (uint32_t)dst_buf;
    config.src_xsize = src_xsize;
    config.src_ysize = src_ysize;
    config.dst_xsize = dst_xsize;
    config.dst_ysize = dst_ysize;
    config.src_step = src_step;
    config.dst_step = dst_step;
    config.finish_int_en = 1;
    config.half_finish_int_en = 0;

    bk_err_t ret = bk_hpdma_link_set_desc(vnd_data->link_dma_list_table, 0, &config);
    if (ret != BK_OK) {
        LOGE("%s bk_hpdma_link_set_desc failed, ret=%d\n", __func__, ret);
        return ret;
    }

    /*
     * P1 (HPDMA review): use the new HPDMA_BURST_LEN_INC16 enum instead of
     *   the magic literal 0x03. The actual SMEM-same-block downgrade now
     *   happens inside the driver at start time (see
     *   hpdma_apply_smem_burst_policy_at_start), so a request for INC16
     *   here may be silently programmed as INC8 when src and dst share a
     *   physical SMEM block - this is the intended safe behaviour.
     */
    bk_hpdma_set_dest_burst_len(vnd_data->lv_hpdma_id, HPDMA_BURST_LEN_INC16);
    bk_hpdma_set_src_burst_len(vnd_data->lv_hpdma_id, HPDMA_BURST_LEN_INC16);

    bk_hpdma_register_isr(vnd_data->lv_hpdma_id, NULL, NULL, lv_hpdma_transfer_complete_callback, NULL);
    bk_hpdma_enable_finish_interrupt(vnd_data->lv_hpdma_id);

    ret = bk_hpdma_link_transfer(vnd_data->lv_hpdma_id, vnd_data->link_dma_list_table);
    if (ret != BK_OK) {
        LOGE("%s bk_hpdma_link_transfer failed, ret=%d\n", __func__, ret);
        return ret;
    }

    vnd_data->lv_hpdma_in_use = true;

    return BK_OK;
}

bk_err_t lv_hpdma_memcpy_wait_finish(uint32_t timeout_ms)
{
    bk_err_t ret = BK_OK;

    lv_vnd_data_t *vnd_data = lv_hpdma_get_vnd_data();
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return BK_FAIL;
    }

    if (vnd_data->lv_hpdma_in_use && (vnd_data->lv_hpdma_sem != NULL)) {
        ret = rtos_get_semaphore(&vnd_data->lv_hpdma_sem, timeout_ms);
        if (ret != BK_OK) {
            LOGE("%s rtos_get_semaphore failed\n", __func__);
            return ret;
        }
        vnd_data->lv_hpdma_in_use = false;
    }

    return ret;
}

bk_err_t lv_hpdma_memcpy_stop(void)
{
    lv_vnd_data_t *vnd_data = lv_hpdma_get_vnd_data();
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return BK_FAIL;
    }

    if (vnd_data->lv_hpdma_in_use) {
        bk_err_t ret = bk_hpdma_stop(vnd_data->lv_hpdma_id);
        if (ret != BK_OK) {
            LOGE("%s bk_hpdma_stop failed, ret=%d\n", __func__, ret);
            return ret;
        }
        vnd_data->lv_hpdma_in_use = false;
    }

    return BK_OK;
}

