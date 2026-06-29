#pragma once

/**
 * @file display_spi_vn_ctlr.h
 * @brief HW SPI display controller. Internal to bk_display.
 */

#include <os/os.h>
#include <components/bk_display.h>
#include "bk_display_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SPI_DISP_STATE_DEINITED = 0,    /**< Bus is not initialized. */
    SPI_DISP_STATE_INITING,         /**< Bus initialization is in progress. */
    SPI_DISP_STATE_INITED,          /**< Bus is initialized; display task is not running. */
    SPI_DISP_STATE_OPENING,         /**< Display task startup is in progress. */
    SPI_DISP_STATE_ACTIVE,          /**< Display task is running and accepts frames. */
    SPI_DISP_STATE_CLOSING,         /**< Display task shutdown is in progress. */
    SPI_DISP_STATE_CLOSED,          /**< Display task is stopped; bus remains initialized. */
    SPI_DISP_STATE_DEINITING,       /**< Bus deinitialization is in progress. */
} spi_display_state_t;

typedef struct
{
    spi_display_state_t state;
    beken_mutex_t lock;
    beken_semaphore_t disp_task_sem;
    beken_thread_t disp_task;
    beken_queue_t queue;
    bool disp_task_running;
    bool lcd_display_flag;
    void *display_frame;
    flush_free_cb_t display_frame_cb;
    bk_display_spi_ctlr_config_t config;
    bk_display_ctlr_t ops;
} spi_vn_ctlr_t;

#ifdef __cplusplus
}
#endif
