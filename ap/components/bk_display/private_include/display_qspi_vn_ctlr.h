#pragma once

/**
 * @file display_qspi_vn_ctlr.h
 * @brief HW QSPI display controller. Internal to bk_display.
 */

#include <os/os.h>
#include <components/bk_display.h>
#include "bk_display_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    QSPI_DISP_STATE_DEINITED = 0,    /**< QSPI driver is not initialized. */
    QSPI_DISP_STATE_INITING,         /**< QSPI initialization is in progress. */
    QSPI_DISP_STATE_INITED,          /**< QSPI is initialized; display task is not running. */
    QSPI_DISP_STATE_OPENING,         /**< Display task startup is in progress. */
    QSPI_DISP_STATE_ACTIVE,          /**< Display task is running and accepts frames. */
    QSPI_DISP_STATE_CLOSING,         /**< Display task shutdown is in progress. */
    QSPI_DISP_STATE_CLOSED,          /**< Display task is stopped; QSPI remains initialized. */
    QSPI_DISP_STATE_DEINITING,       /**< QSPI deinitialization is in progress. */
} qspi_display_state_t;

typedef struct
{
    qspi_display_state_t state;
    beken_mutex_t lock;
    beken_semaphore_t disp_task_sem;
    beken_thread_t disp_task;
    beken_queue_t queue;
    bool disp_task_running;
    bool lcd_display_flag;
    void *display_frame;
    flush_free_cb_t display_frame_cb;
    bk_display_qspi_ctlr_config_t config;
    bk_display_ctlr_t ops;
} qspi_vn_ctlr_t;

#ifdef __cplusplus
}
#endif
