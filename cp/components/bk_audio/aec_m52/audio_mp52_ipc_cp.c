#include "audio_mp52_ipc_cp.h"

#include <string.h>
#include <os/os.h>
#include <driver/mailbox_channel.h>
#include <components/log.h>
#include <modules/audio_mp52_ipc.h>
#include "audio_mp52_engine.h"

#define TAG "aec_m52_cp"

#ifndef AEC_M52_MB_CHNL
#define AEC_M52_MB_CHNL MB_CHNL_AUD_AEC
#endif

#define AEC_M52_WORKER_PRIO         (2)
#define AEC_M52_WORKER_NAME         "aec_m52"
#define AEC_M52_WORKER_STACK_BYTES  (4 * 1024)

typedef struct {
    volatile uint32_t pending;
    volatile uint32_t pending_seq;
    volatile uint32_t pending_slot;
    volatile aec_m52_slot_desc_t *pending_desc;
    beken_semaphore_t run_sem;
    beken_thread_t worker;
} aec_m52_cp_ctx_t;

typedef struct {
    uint32_t run_cnt;
    uint32_t busy_cnt;
    uint32_t proc_fail_cnt;
} aec_m52_cp_debug_stats_t;

static aec_m52_cp_ctx_t s_cp_ctx = {0};
static aec_m52_cp_debug_stats_t s_cp_stats = {0};

static void aec_m52_send_done(uint32_t seq, uint32_t slot, uint32_t status)
{
    mb_chnl_cmd_t cmd;
    os_memset(&cmd, 0x00, sizeof(cmd));
    cmd.hdr.cmd = AEC_M52_IPC_CMD_DONE;
    cmd.param1 = seq;
    cmd.param2 = slot;
    cmd.param3 = status;
    if (mb_chnl_write(AEC_M52_MB_CHNL, &cmd) != BK_OK) {
        BK_LOGE(TAG, "send done fail seq=%lu slot=%lu status=%lu\n",
                (unsigned long)seq, (unsigned long)slot, (unsigned long)status);
    }
}

static void aec_m52_cp_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
    (void)param;
    (void)ack_buf;
}

static void aec_m52_cp_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
    (void)param;
    BK_LOGV(TAG, "rx isr cmd=%lu param1=%lu param2=%lu param3=%lu\n",
            (unsigned long)cmd_buf->hdr.cmd,
            (unsigned long)(uintptr_t)cmd_buf->param1,
            (unsigned long)(uintptr_t)cmd_buf->param2,
            (unsigned long)(uintptr_t)cmd_buf->param3);

    if (cmd_buf->hdr.cmd == AEC_M52_IPC_CMD_CTRL) {
        const aec_m52_ctrl_cfg_t *ctrl_cfg = (const aec_m52_ctrl_cfg_t *)(uintptr_t)cmd_buf->param1;
        uint32_t cfg_size = cmd_buf->param2;

        if ((ctrl_cfg == NULL) || (cfg_size < sizeof(aec_m52_ctrl_cfg_t))) {
            BK_LOGE(TAG, "recv ctrl invalid ptr = 0x%lx, size = %lu\n",
                    (unsigned long)(uintptr_t)ctrl_cfg, (unsigned long)cfg_size);
            cmd_buf->param3 = AEC_M52_IPC_STATUS_INVALID;
            return;
        }

        if ((aec_m52_engine_apply_ctrl(ctrl_cfg) != BK_OK) ||
            (aec_m52_engine_init(ctrl_cfg->fs) != BK_OK)) {
            BK_LOGE(TAG, "recv ctrl apply fail fs = %lu\n", (unsigned long)ctrl_cfg->fs);
            cmd_buf->param3 = AEC_M52_IPC_STATUS_PROC_FAIL;
            return;
        }

        BK_LOGI(TAG, "recv ctrl fs = %lu frame = %lu size = %lu\n",
                (unsigned long)ctrl_cfg->fs, (unsigned long)ctrl_cfg->frame_bytes, (unsigned long)cfg_size);
        cmd_buf->param1 = ctrl_cfg->fs;
        cmd_buf->param2 = 0;
        cmd_buf->param3 = AEC_M52_IPC_STATUS_OK;
        return;
    }

    if (cmd_buf->hdr.cmd != AEC_M52_IPC_CMD_RUN) {
        cmd_buf->param3 = AEC_M52_IPC_STATUS_INVALID;
        return;
    }

    if ((s_cp_ctx.worker == NULL) || (s_cp_ctx.run_sem == NULL)) {
        cmd_buf->param1 = cmd_buf->param2;
        cmd_buf->param2 = cmd_buf->param3;
        cmd_buf->param3 = AEC_M52_IPC_STATUS_PROC_FAIL;
        BK_LOGE(TAG, "worker not ready seq = %lu slot = %lu\n",
                (unsigned long)cmd_buf->param1, (unsigned long)cmd_buf->param2);
        return;
    }

    if (s_cp_ctx.pending) {
        s_cp_stats.busy_cnt++;
        if ((s_cp_stats.busy_cnt % 50) == 1) {
            BK_LOGW(TAG, "rx busy cnt = %lu seq = %lu slot = %lu\n",
                    (unsigned long)s_cp_stats.busy_cnt, (unsigned long)cmd_buf->param2, (unsigned long)cmd_buf->param3);
        }
        cmd_buf->param1 = cmd_buf->param2;
        cmd_buf->param2 = cmd_buf->param3;
        cmd_buf->param3 = AEC_M52_IPC_STATUS_BUSY;
        return;
    }

    s_cp_ctx.pending_desc = (aec_m52_slot_desc_t *)(uintptr_t)cmd_buf->param1;
    if (s_cp_ctx.pending_desc == NULL) {
        cmd_buf->param1 = cmd_buf->param2;
        cmd_buf->param2 = cmd_buf->param3;
        cmd_buf->param3 = AEC_M52_IPC_STATUS_INVALID;
        return;
    }

    s_cp_ctx.pending_seq  = cmd_buf->param2;
    s_cp_ctx.pending_slot = cmd_buf->param3;
    s_cp_ctx.pending = 1;

    if (rtos_set_semaphore(&s_cp_ctx.run_sem) != BK_OK) {
        s_cp_ctx.pending = 0;
        cmd_buf->param1 = cmd_buf->param2;
        cmd_buf->param2 = cmd_buf->param3;
        cmd_buf->param3 = AEC_M52_IPC_STATUS_PROC_FAIL;
        BK_LOGE(TAG, "wake worker fail seq = %lu slot = %lu\n",
                (unsigned long)cmd_buf->param1, (unsigned long)cmd_buf->param2);
        return;
    }

    cmd_buf->param1 = s_cp_ctx.pending_seq;
    cmd_buf->param2 = s_cp_ctx.pending_slot;
    cmd_buf->param3 = AEC_M52_IPC_STATUS_OK;
}

static void aec_m52_worker(void *param)
{
    (void)param;
    while (1) 
    {
        if (rtos_get_semaphore(&s_cp_ctx.run_sem, BEKEN_WAIT_FOREVER) != BK_OK) {
            continue;
        }
        if (!s_cp_ctx.pending) {
            continue;
        }
        BK_LOGV(TAG, "worker pending desc=0x%lx seq=%lu slot=%lu\n",
                (unsigned long)(uintptr_t)s_cp_ctx.pending_desc,
                (unsigned long)s_cp_ctx.pending_seq,
                (unsigned long)s_cp_ctx.pending_slot);

        aec_m52_slot_desc_t *desc = (aec_m52_slot_desc_t *)s_cp_ctx.pending_desc;
        uint32_t seq  = s_cp_ctx.pending_seq;
        uint32_t slot = s_cp_ctx.pending_slot;
        s_cp_ctx.pending = 0;

        uint32_t status = AEC_M52_IPC_STATUS_OK;
        if (aec_m52_engine_process(desc) != BK_OK) {
            status = AEC_M52_IPC_STATUS_PROC_FAIL;
            s_cp_stats.proc_fail_cnt++;
            BK_LOGE(TAG, "process fail cnt=%lu seq=%lu slot=%lu\n",
                    (unsigned long)s_cp_stats.proc_fail_cnt,
                    (unsigned long)seq,
                    (unsigned long)slot);
        }
        s_cp_stats.run_cnt++;
        if ((s_cp_stats.run_cnt % 500) == 0) {
            BK_LOGD(TAG, "run cnt=%lu seq=%lu slot=%lu status=%lu\n",
                    (unsigned long)s_cp_stats.run_cnt,
                    (unsigned long)seq,
                    (unsigned long)slot,
                    (unsigned long)status);
        }
        aec_m52_send_done(seq, slot, status);
    }
}

bk_err_t aec_m52_ipc_cp_init(void)
{
    bk_err_t ret = BK_OK;

    ret = mb_chnl_open(AEC_M52_MB_CHNL, NULL);
    if (ret == BK_ERR_OPEN) {
        BK_LOGW(TAG, "mb_chnl already open, continue chnl = %d\n", AEC_M52_MB_CHNL);
        ret = BK_OK;
    } else if (ret != BK_OK) {
        BK_LOGE(TAG, "mb_chnl_open fail ret = %d\n", ret);
        return ret;
    }

    ret = mb_chnl_ctrl(AEC_M52_MB_CHNL, MB_CHNL_SET_RX_ISR, aec_m52_cp_rx_isr);
    if (ret != BK_OK) {
        BK_LOGE(TAG, "set rx isr fail ret = %d\n", ret);
        return ret;
    }

    ret = mb_chnl_ctrl(AEC_M52_MB_CHNL, MB_CHNL_SET_TX_CMPL_ISR, aec_m52_cp_tx_cmpl_isr);
    if (ret != BK_OK) {
        BK_LOGE(TAG, "set tx cmpl isr fail ret = %d\n", ret);
        return ret;
    }

    if (s_cp_ctx.run_sem == NULL) {
        ret = rtos_init_semaphore_ex(&s_cp_ctx.run_sem, 1, 0);
        if (ret != BK_OK) {
            BK_LOGE(TAG, "init run sem fail ret = %d\n", ret);
            return ret;
        }
    }

    if (s_cp_ctx.worker == NULL) {
        ret = rtos_create_thread(&s_cp_ctx.worker,
                                 AEC_M52_WORKER_PRIO,
                                 AEC_M52_WORKER_NAME,
                                 aec_m52_worker,
                                 AEC_M52_WORKER_STACK_BYTES,
                                 NULL);
        if (ret != BK_OK) {
            BK_LOGE(TAG, "create worker fail ret = %d\n", ret);
            return ret;
        }
    }

    BK_LOGI(TAG, "audio mp52_cp_init ok, chnl = %d\n", AEC_M52_MB_CHNL);
    return ret;
}
