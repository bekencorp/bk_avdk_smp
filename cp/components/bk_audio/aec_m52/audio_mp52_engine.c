#include "audio_mp52_engine.h"

#include <string.h>
#include <components/log.h>
#include <driver/aon_rtc.h>
#include <modules/aec_v3_1.h>
#include "FreeRTOS.h"
#include "task.h"

#define TAG "audio_mp52_eng"

#define AEC_M52_STATIC_MEM_BYTES   (36u * 1024u)
/* Temporary replacement size until CP-side real aec_size is available. */
#define AEC_M52_TEMP_AEC_MEM_BYTES (32u * 1024u)

typedef struct {
    uint32_t fs;
    uint8_t inited;
    uint32_t aec_mem_bytes;
    AECContext *aec_ctx;
    int16_t *ref_stage;
    int16_t *mic_stage;
    int16_t *out_stage;
    aec_m52_ctrl_cfg_t ctrl_cfg;
} aec_m52_engine_ctx_t;

static aec_m52_engine_ctx_t s_engine = {0};

static uint8_t s_aec_static_mem[AEC_M52_STATIC_MEM_BYTES] __attribute__((aligned(32))) = {0};

typedef struct {
    TaskHandle_t task_handle;
    uint64_t wall_us_sum;
    uint32_t wall_us_max;
    uint64_t cpu_tick_sum;
    uint32_t cpu_tick_max;
    uint32_t sample_cnt;
} aec_proc_prof_t;

static aec_proc_prof_t s_aec_prof = {0};

static uint32_t aec_get_task_runtime_counter(TaskHandle_t task)
{
    TaskStatus_t status;

    if (task == NULL) {
        return 0;
    }

    vTaskGetInfo(task, &status, pdFALSE, eInvalid);
    return (uint32_t)status.ulRunTimeCounter;
}

static void aec_profile_accumulate_and_log(uint32_t wall_us, uint32_t cpu_ticks)
{
    s_aec_prof.sample_cnt++;
    s_aec_prof.wall_us_sum  += wall_us;
    s_aec_prof.cpu_tick_sum += cpu_ticks;

    if (wall_us > s_aec_prof.wall_us_max) {
        s_aec_prof.wall_us_max = wall_us;
    }
    if (cpu_ticks > s_aec_prof.cpu_tick_max) {
        s_aec_prof.cpu_tick_max = cpu_ticks;
    }

    os_printf("%lu %lu %lu %lu %lu %lu %lu\n",
            (unsigned long)s_aec_prof.sample_cnt,
            (unsigned long)wall_us,
            (unsigned long)(s_aec_prof.wall_us_sum / s_aec_prof.sample_cnt),
            (unsigned long)s_aec_prof.wall_us_max,
            (unsigned long)cpu_ticks,
            (unsigned long)(s_aec_prof.cpu_tick_sum / s_aec_prof.sample_cnt),
            (unsigned long)s_aec_prof.cpu_tick_max);

}

static uint32_t aec_size0(uint32_t delay_points)
{
    (void)delay_points;
    return AEC_M52_TEMP_AEC_MEM_BYTES;
}

static void aec_dump_ctx(const AECContext *aec_ctx)
{
    if (aec_ctx == NULL) {
        BK_LOGE(TAG, "aec_dump_ctx null\n");
        return;
    }

    BK_LOGI(TAG, "aec_ctx: base=0x%lx size=%lu\n",
            (unsigned long)(uintptr_t)aec_ctx,
            (unsigned long)sizeof(AECContext));

    BK_LOGI(TAG,
            "aec_ctx[01] flags=0x%x mic_swap=%u interweave=%u relay=%u test=%u vol=%u vol_mem=%u ref_up=%u ec_filter=0x%x ns_filter=0x%x\n",
            (unsigned int)aec_ctx->flags,
            (unsigned int)aec_ctx->mic_swap,
            (unsigned int)aec_ctx->interweave,
            (unsigned int)aec_ctx->relay,
            (unsigned int)aec_ctx->test,
            (unsigned int)aec_ctx->vol,
            (unsigned int)aec_ctx->vol_mem,
            (unsigned int)aec_ctx->ref_up,
            (unsigned int)aec_ctx->ec_filter,
            (unsigned int)aec_ctx->ns_filter);

    BK_LOGI(TAG,
            "aec_ctx[02] mu_dm=%d mu_ec=%d ec_depth=%d ref_scale=%d mic_scale=%d dc_scale=%d drc_mode=%d vad=%d fs=%d is_ec=%d\n",
            (int)aec_ctx->mu_dm,
            (int)aec_ctx->mu_ec,
            (int)aec_ctx->ec_depth,
            (int)aec_ctx->ref_scale,
            (int)aec_ctx->mic_scale,
            (int)aec_ctx->dc_scale,
            (int)aec_ctx->drc_mode,
            (int)aec_ctx->vad,
            (int)aec_ctx->fs,
            (int)aec_ctx->is_ec);

    BK_LOGI(TAG,
            "aec_ctx[03] cutbin1=%d cutbin2=%d hr_bin=%d sbnum=%d fftlen=%d Flen=%d ovlp=%d frame_samples=%d freq_scale=%d\n",
            (int)aec_ctx->cutbin1,
            (int)aec_ctx->cutbin2,
            (int)aec_ctx->hr_bin,
            (int)aec_ctx->sbnum,
            (int)aec_ctx->fftlen,
            (int)aec_ctx->Flen,
            (int)aec_ctx->ovlp,
            (int)aec_ctx->frame_samples,
            (int)aec_ctx->freq_scale);

    BK_LOGI(TAG,
            "aec_ctx[04] rin_delay_rp=%d rin_delay_wp=%d max_mic_delay=%d mic_delay=%d delay_offset=%d ec_rsd_thr1=%d ec_rsd_thr2=%d ec_rsd_thr3=%d ec_rsd_thr4=%d\n",
            (int)aec_ctx->rin_delay_rp,
            (int)aec_ctx->rin_delay_wp,
            (int)aec_ctx->max_mic_delay,
            (int)aec_ctx->mic_delay,
            (int)aec_ctx->delay_offset,
            (int)aec_ctx->ec_rsd_thr1,
            (int)aec_ctx->ec_rsd_thr2,
            (int)aec_ctx->ec_rsd_thr3,
            (int)aec_ctx->ec_rsd_thr4);

    BK_LOGI(TAG,
            "aec_ctx[05] ec_guard=%d spcnt=%d dcnt=%d spcnt2=%d dcnt2=%d Astep=%d dist=%d phs_s0=%d phs_s1=%d minG=%u\n",
            (int)aec_ctx->ec_guard,
            (int)aec_ctx->spcnt,
            (int)aec_ctx->dcnt,
            (int)aec_ctx->spcnt2,
            (int)aec_ctx->dcnt2,
            (int)aec_ctx->Astep,
            (int)aec_ctx->dist,
            (int)aec_ctx->phs_s0,
            (int)aec_ctx->phs_s1,
            (unsigned int)aec_ctx->minG);

    BK_LOGI(TAG,
            "aec_ctx[06] NspCoe=%ld gainp=%ld ec_thr=%ld ec_thr2=%ld mic_bg=%ld mic_max=%ld mic_min=%ld ref_max=%ld\n",
            (long)aec_ctx->NspCoe,
            (long)aec_ctx->gainp,
            (long)aec_ctx->ec_thr,
            (long)aec_ctx->ec_thr2,
            (long)aec_ctx->mic_bg,
            (long)aec_ctx->mic_max,
            (long)aec_ctx->mic_min,
            (long)aec_ctx->ref_max);

    BK_LOGI(TAG,
            "aec_ctx[07] mic_eng=%ld ref_eng=%ld dc=%ld dc2=%ld dcr=%ld cni_floor=%ld cni_fade=%ld s_max=%ld\n",
            (long)aec_ctx->mic_eng,
            (long)aec_ctx->ref_eng,
            (long)aec_ctx->dc,
            (long)aec_ctx->dc2,
            (long)aec_ctx->dcr,
            (long)aec_ctx->cni_floor,
            (long)aec_ctx->cni_fade,
            (long)aec_ctx->s_max);

    BK_LOGI(TAG,
            "aec_ctx[08] ns_mean=%ld drc_gain=%ld phs_min=%ld phs_old=%ld phs_thr=%ld phs_cur=%ld vad_hr=%ld frame_cnt=%lu\n",
            (long)aec_ctx->ns_mean,
            (long)aec_ctx->drc_gain,
            (long)aec_ctx->phs_min,
            (long)aec_ctx->phs_old,
            (long)aec_ctx->phs_thr,
            (long)aec_ctx->phs_cur,
            (long)aec_ctx->vad_hr,
            (unsigned long)aec_ctx->frame_cnt);

    BK_LOGI(TAG,
            "aec_ctx ptr: rin=0x%lx sin=0x%lx out=0x%lx rin_delay=0x%lx ana_win=0x%lx syn_win=0x%lx\n",
            (unsigned long)(uintptr_t)aec_ctx->rin,
            (unsigned long)(uintptr_t)aec_ctx->sin,
            (unsigned long)(uintptr_t)aec_ctx->out,
            (unsigned long)(uintptr_t)aec_ctx->rin_delay,
            (unsigned long)(uintptr_t)aec_ctx->ana_win,
            (unsigned long)(uintptr_t)aec_ctx->syn_win);

    BK_LOGI(TAG,
            "aec_ctx ptr2: TP=0x%lx ST=0x%lx SF=0x%lx RF=0x%lx AF=0x%lx ang=0x%lx EP=0x%lx EO=0x%lx\n",
            (unsigned long)(uintptr_t)aec_ctx->TP,
            (unsigned long)(uintptr_t)aec_ctx->ST,
            (unsigned long)(uintptr_t)aec_ctx->SF,
            (unsigned long)(uintptr_t)aec_ctx->RF,
            (unsigned long)(uintptr_t)aec_ctx->AF,
            (unsigned long)(uintptr_t)aec_ctx->ang,
            (unsigned long)(uintptr_t)aec_ctx->EP,
            (unsigned long)(uintptr_t)aec_ctx->EO);

}
#define AEC_EC_OUT_BUF_LEN (770 * sizeof(int32_t))

uint8_t ecout_buf[AEC_EC_OUT_BUF_LEN]  __attribute__((aligned(64))) = {0};

static void aec_init0(AECContext *aec_ctx,
                      int16_t fs,
                      uint32_t aec_mem_bytes,
                      const aec_m52_ctrl_cfg_t *ctrl_cfg)
{
    uint32_t val = 0;
    uint16_t init_flags = 0;

    if (ctrl_cfg == NULL) {
        BK_LOGE(TAG, "aec_init0 ctrl_cfg is null\n");
        return;
    }
    bk_printf("aec_version: %d\n",aec_ver());
    /* Keep same init entry behavior as AP-side AEC flow. */
    aec_ctx->fs = 0;
    aec_init(aec_ctx, fs);

    aec_ctrl(aec_ctx, AEC_CTRL_CMD_GET_TX_BUF, (uint32_t)(uintptr_t)(&val));
    if (val != 0) {
        s_engine.mic_stage = (int16_t *)(uintptr_t)val;
    }
    aec_ctrl(aec_ctx, AEC_CTRL_CMD_GET_RX_BUF, (uint32_t)(uintptr_t)(&val));
    if (val != 0) {
        s_engine.ref_stage = (int16_t *)(uintptr_t)val;
    }
    aec_ctrl(aec_ctx, AEC_CTRL_CMD_GET_OUT_BUF, (uint32_t)(uintptr_t)(&val));
    if (val != 0) {
        s_engine.out_stage = (int16_t *)(uintptr_t)val;
    }

    init_flags = ctrl_cfg->init_flags;
    if (ctrl_cfg->dual_ch) {
        init_flags |= AEC_DM_FLAG_MSK;
    } else {
        init_flags &= (uint16_t)(~AEC_DM_FLAG_MSK);
    }
    aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_FLAGS, init_flags);

    aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_MIC_DELAY, ctrl_cfg->delay_points);
    aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_EC_DEPTH, ctrl_cfg->ec_depth);
    aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_REF_SCALE, ctrl_cfg->ref_scale);
    aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_VOL, ctrl_cfg->voice_vol);
    if (ctrl_cfg->max_delay_points > 0) {
        aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_MAX_DELAY, ctrl_cfg->max_delay_points);
    }
    aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_NS_LEVEL, ctrl_cfg->ns_level);
    aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_NS_PARA, ctrl_cfg->ns_para);
    aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_DRC, ctrl_cfg->drc);
    aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_EC_FILTER, ctrl_cfg->ec_filter);
    aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_NS_FILTER, ctrl_cfg->ns_filter);

    if (ctrl_cfg->dual_ch) {
        aec_ctx->interweave = ctrl_cfg->interweave;
        aec_ctx->dist = ctrl_cfg->dist;
        aec_ctx->mic_swap = ctrl_cfg->mic_swap;
        if (ctrl_cfg->vad_enable) {
            aec_ctx->vad = 1;
        }
        aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_DUAL_PERP, ctrl_cfg->dual_perp);
    }

    if (ctrl_cfg->spthr_valid) {
        os_memcpy(aec_ctx->SPthr, ctrl_cfg->spthr, sizeof(ctrl_cfg->spthr));
    }
    aec_ctx->phs_s1 = ctrl_cfg->phs_s1;

    aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_DELAY_BUFF, (uint32_t)(uintptr_t)aec_ctx->refbuff);

    if (ctrl_cfg->ec_only_output) {
        aec_ctx->ec_filter |= (1 << 5);
        aec_ctrl(aec_ctx, AEC_CTRL_CMD_SET_EOBUFF, (uint32_t)ecout_buf);
    }

    BK_LOGI(TAG,
            "aec_init0: aec=0x%lx fs=%d mem=%lu frame=%lu init_flags=0x%x delay=%lu depth=%lu\n",
            (unsigned long)(uintptr_t)aec_ctx,
            (int)fs,
            (unsigned long)aec_mem_bytes,
            (unsigned long)ctrl_cfg->frame_bytes,
            (unsigned int)init_flags,
            (unsigned long)ctrl_cfg->delay_points,
            (unsigned long)ctrl_cfg->ec_depth);
    BK_LOGI(TAG,
            "aec_init0: mic/ref/out=%lu/%lu/%lu dual=%u ns(type=%u filter=0x%x level=%u para=%u) vad=%u\n",
            (unsigned long)ctrl_cfg->mic_bytes,
            (unsigned long)ctrl_cfg->ref_bytes,
            (unsigned long)ctrl_cfg->out_bytes,
            (unsigned int)ctrl_cfg->dual_ch,
            (unsigned int)ctrl_cfg->ns_type,
            (unsigned int)ctrl_cfg->ns_filter,
            (unsigned int)ctrl_cfg->ns_level,
            (unsigned int)ctrl_cfg->ns_para,
            (unsigned int)ctrl_cfg->vad_enable);
    BK_LOGI(TAG,
            "aec_init0: ptr rin=0x%lx sin=0x%lx out=0x%lx stage_ref=0x%lx stage_mic=0x%lx stage_out=0x%lx\n",
            (unsigned long)(uintptr_t)aec_ctx->rin,
            (unsigned long)(uintptr_t)aec_ctx->sin,
            (unsigned long)(uintptr_t)aec_ctx->out,
            (unsigned long)(uintptr_t)s_engine.ref_stage,
            (unsigned long)(uintptr_t)s_engine.mic_stage,
            (unsigned long)(uintptr_t)s_engine.out_stage);
    aec_dump_ctx(aec_ctx);
}

static void aec_run(AECContext *aec_ctx,
                    int16_t *rin,
                    int16_t *sin,
                    int16_t *out,
                    uint32_t rin_bytes,
                    uint32_t mic_bytes,
                    uint32_t out_bytes)
{

    if ((aec_ctx == NULL) || (aec_ctx->out == NULL) || (aec_ctx->rin == NULL) || (aec_ctx->sin == NULL)) {
        BK_LOGE(TAG, "aec_run invalid ctx ptr aec=0x%lx out=0x%lx rin=0x%lx sin=0x%lx\n",
                (unsigned long)(uintptr_t)aec_ctx,
                (unsigned long)(uintptr_t)(aec_ctx ? aec_ctx->out : NULL),
                (unsigned long)(uintptr_t)(aec_ctx ? aec_ctx->rin : NULL),
                (unsigned long)(uintptr_t)(aec_ctx ? aec_ctx->sin : NULL));
        return;
    }

    /* Step2: update AEC rin/sin buffers with latest frame input. */
    if ((rin_bytes > 0) && (rin != NULL)) {
        os_memcpy(aec_ctx->rin, rin, rin_bytes);
    }
    if ((mic_bytes > 0) && (sin != NULL)) {
        os_memcpy(aec_ctx->sin, sin, mic_bytes);
    }

    TaskHandle_t cur_task = xTaskGetCurrentTaskHandle();
    uint32_t cpu_tick_before = aec_get_task_runtime_counter(cur_task);
    uint64_t wall_us_before = bk_aon_rtc_get_us();

    aec_proc(aec_ctx, aec_ctx->rin, aec_ctx->sin, aec_ctx->out); ////

    uint64_t __maybe_unused wall_us_after = bk_aon_rtc_get_us();
    uint32_t __maybe_unused cpu_tick_after = aec_get_task_runtime_counter(cur_task);
    uint32_t __maybe_unused wall_delta_us = (uint32_t)(wall_us_after - wall_us_before);
    uint32_t __maybe_unused cpu_delta_ticks = cpu_tick_after - cpu_tick_before;

    if (s_aec_prof.task_handle == NULL) {
        s_aec_prof.task_handle = cur_task;
    }
/*    aec_profile_accumulate_and_log(wall_delta_us, cpu_delta_ticks);*/
    // os_printf("%lu %lu %lu\n", (unsigned long)(wall_delta_us-cpu_delta_ticks),(unsigned long)wall_delta_us, (unsigned long)cpu_delta_ticks);
    /* Step1: output previous AEC out buffer. */
    if ((out_bytes > 0) && (out != NULL)) {
        os_memcpy(out, aec_ctx->out, out_bytes);
    }
}

bk_err_t aec_m52_engine_init(uint32_t fs)
{
    if ((fs != 8000) && (fs != 16000)) {
        fs = 16000;
    }
    s_engine.fs = fs;

    return BK_OK;
}

bk_err_t aec_m52_engine_apply_ctrl(const aec_m52_ctrl_cfg_t *ctrl_cfg)
{
    if ((ctrl_cfg == NULL) || (ctrl_cfg->magic != AEC_M52_CTRL_MAGIC)) {
        BK_LOGE(TAG, "invalid ctrl cfg\n");
        return BK_FAIL;
    }

    uint32_t aec_mem_bytes = 0;
    uintptr_t stage_base = 0;
    uintptr_t stage_end = 0;

    s_engine.ctrl_cfg = *ctrl_cfg;
    s_engine.fs = ctrl_cfg->fs;

    aec_mem_bytes = aec_size0(ctrl_cfg->delay_points);
    if ((aec_mem_bytes == 0) || (aec_mem_bytes > AEC_M52_STATIC_MEM_BYTES)) {
        BK_LOGE(TAG, "aec mem size invalid bytes=%lu delay=%lu\n",
                (unsigned long)aec_mem_bytes,
                (unsigned long)ctrl_cfg->delay_points);
        s_engine.inited = 0;
        return BK_FAIL;
    }

    stage_base = ((uintptr_t)&s_aec_static_mem[aec_mem_bytes] + 63U) & ~(uintptr_t)63U;
    stage_end  = (uintptr_t)&s_aec_static_mem[AEC_M52_STATIC_MEM_BYTES];
    if (stage_base > stage_end) {
        BK_LOGE(TAG, "aec stage base overflow\n");
        s_engine.inited = 0;
        return BK_FAIL;
    }

    if ((uint32_t)(stage_end - stage_base) < (ctrl_cfg->ref_bytes + ctrl_cfg->mic_bytes)) {
        BK_LOGE(TAG, "aec stage bytes insufficient stage=%lu need=%lu\n",
                (unsigned long)(stage_end - stage_base),
                (unsigned long)(ctrl_cfg->ref_bytes + ctrl_cfg->mic_bytes));
        s_engine.inited = 0;
        return BK_FAIL;
    }

    s_engine.aec_ctx = (AECContext *)s_aec_static_mem;
    s_engine.aec_mem_bytes = aec_mem_bytes;
    s_engine.ref_stage = (int16_t *)stage_base;
    s_engine.mic_stage = (int16_t *)(stage_base + ctrl_cfg->ref_bytes);

    aec_init0(s_engine.aec_ctx, (int16_t)s_engine.fs, s_engine.aec_mem_bytes, ctrl_cfg);
    s_engine.inited = 1;

    BK_LOGI(TAG, "ctrl sync aec=0x%lx fs=%lu frame=%lu flags=0x%x ns_type=%u dual=%u vad=%u\n",
            (unsigned long)(uintptr_t)s_engine.aec_ctx,
            (unsigned long)ctrl_cfg->fs,
            (unsigned long)ctrl_cfg->frame_bytes,
            ctrl_cfg->init_flags,
            (unsigned int)ctrl_cfg->ns_type,
            (unsigned int)ctrl_cfg->dual_ch,
            (unsigned int)ctrl_cfg->vad_enable);
    BK_LOGI(TAG, "aec init done mem=%lu ref=%lu mic=%lu\n",
            (unsigned long)s_engine.aec_mem_bytes,
            (unsigned long)ctrl_cfg->ref_bytes,
            (unsigned long)ctrl_cfg->mic_bytes);
    return BK_OK;
}


bk_err_t aec_m52_engine_process(aec_m52_slot_desc_t *slot_desc)
{
    if ((slot_desc == NULL) || (slot_desc->magic != AEC_M52_SLOT_MAGIC)) {
        BK_LOGW(TAG, "invalid slot desc magic=0x%lx\n",
                (unsigned long)(slot_desc ? slot_desc->magic : 0));
        return BK_FAIL;
    }

    if (!s_engine.inited) {
        BK_LOGE(TAG, "engine not initialized\n");
        return BK_FAIL;
    }

    if ((slot_desc->mic_addr == NULL) || (slot_desc->ref_addr == NULL) || (slot_desc->out_addr == NULL)) {
        BK_LOGE(TAG, "invalid mic/ref/out buffer mic=0x%lx ref=0x%lx out=0x%lx\n",
                (unsigned long)(uintptr_t)slot_desc->mic_addr,
                (unsigned long)(uintptr_t)slot_desc->ref_addr,
                (unsigned long)(uintptr_t)slot_desc->out_addr);
        return BK_FAIL;
    }

    if ((slot_desc->ref_bytes > s_engine.ctrl_cfg.ref_bytes) ||
        (slot_desc->mic_bytes > s_engine.ctrl_cfg.mic_bytes)) {
        BK_LOGE(TAG, "slot bytes overflow ref=%lu/%lu mic=%lu/%lu\n",
                (unsigned long)slot_desc->ref_bytes,
                (unsigned long)s_engine.ctrl_cfg.ref_bytes,
                (unsigned long)slot_desc->mic_bytes,
                (unsigned long)s_engine.ctrl_cfg.mic_bytes);
        return BK_FAIL;
    }

    if (slot_desc->ref_bytes > 0) {
        os_memcpy(s_engine.ref_stage, slot_desc->ref_addr, slot_desc->ref_bytes);
    }
    if (slot_desc->mic_bytes > 0) {
        os_memcpy(s_engine.mic_stage, slot_desc->mic_addr, slot_desc->mic_bytes);
    }

    aec_run(s_engine.aec_ctx, s_engine.ref_stage, s_engine.mic_stage,
            slot_desc->out_addr, slot_desc->ref_bytes, slot_desc->mic_bytes, slot_desc->out_bytes);

    if ((slot_desc->ecout_addr != NULL) && (slot_desc->ecout_bytes > 0)) {
        uint32_t copy_bytes = slot_desc->ecout_bytes;
        if (copy_bytes > AEC_EC_OUT_BUF_LEN) {
            copy_bytes = AEC_EC_OUT_BUF_LEN;
        }
        if (s_engine.ctrl_cfg.ec_only_output) {
            os_memcpy(slot_desc->ecout_addr, ecout_buf, copy_bytes);
        } else {
            os_memset(slot_desc->ecout_addr, 0x00, copy_bytes);
        }
    }

    slot_desc->aec_test  = s_engine.aec_ctx->test;
    slot_desc->aec_spcnt = s_engine.aec_ctx->spcnt;
    slot_desc->aec_dcnt  = s_engine.aec_ctx->dcnt;
    slot_desc->aec_dc    = s_engine.aec_ctx->dc;
    slot_desc->aec_mic_max = s_engine.aec_ctx->mic_max;
    slot_desc->aec_vad_hr  = s_engine.aec_ctx->vad_hr;
    slot_desc->aec_phs_cur = s_engine.aec_ctx->phs_cur;

    return BK_OK;
}
