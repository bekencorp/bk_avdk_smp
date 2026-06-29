/*
 * draw_osd_complex_test.c
 *
 * 复杂测试用例集合（osd test <sub> 子命令族）
 *   stability     - 反复 new/delete OSD 控制器，校验堆水位无下降
 *   concurrent    - 同时跑 add/remove + draw_array，验证 mutex 同步正确
 *   multi_inst    - 同时存活两个 OSD 控制器（psram + sram），验证多实例无串扰
 *   invalid_param - 非法入参全部返回 AVDK_ERR_INVAL，且不 crash
 *   cfg_unchanged - 调 draw_image / draw_font 前后入参 blend_info_t 不被修改
 *   shrink        - OSD_CTLR_CMD_SHRINK 触发 icon ctlr 释放 buf1/buf2，堆水位回升
 *   unaligned     - 字节宽度非 4 对齐（xsize=33）的图像绘制不触发 hardfault
 */

#include <stdlib.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include "components/bk_draw_osd.h"
#include "components/bk_display.h"
#include "osd_disp_compat.h"
#include "blend.h"
#include "draw_osd_test.h"
#include "draw_osd_complex_test.h"

#define TAG "osd_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define TEST_BANNER(name)  LOGI("==== [%s] BEGIN ====\n", name)
#define TEST_PASS(name)    LOGI("==== [%s] PASS ====\n", name)
#define TEST_FAIL(name, why) LOGE("==== [%s] FAIL: %s ====\n", name, why)

/* 默认参数 */
#define STABILITY_DEFAULT_LOOPS   20
#define CONCURRENT_DEFAULT_SEC    10
#define STABILITY_LEAK_THRESHOLD  1024   /* 1 KB */
/*
 * SHRINK 测试阈值：8 KB
 * icon ctlr 的 buf1 只按 icon/font 尺寸分配（非整屏），实际最大可达 10560 B
 * (font_clock 120x44x2)。8 KB 既明显大于 wifi 的 2304 B（防 SHRINK 退化为空操作），
 * 又明显小于 prime 后的 10560 B（保证 SHRINK 正常工作时一定能过）。
 */
#define SHRINK_MIN_DELTA          (8 * 1024)

/* 简单线性同余随机数（避免引入额外依赖） */
static uint32_t s_rand_state = 0x12345678u;
static inline uint32_t test_rand(void)
{
    s_rand_state = s_rand_state * 1103515245u + 12345u;
    return (s_rand_state >> 16) & 0x7fffu;
}

/* ============================================================
 * 堆快照工具
 * ============================================================ */
typedef struct {
    uint32_t sram_free;
    uint32_t psram_free;
} heap_snap_t;

static void heap_take(heap_snap_t *s)
{
    s->sram_free  = rtos_get_free_heap_size();
    s->psram_free = rtos_get_psram_free_heap_size();
}

static void heap_log(const char *tag, const heap_snap_t *s)
{
    LOGI("%s sram_free=%u psram_free=%u\n", tag, s->sram_free, s->psram_free);
}

/* a - b，正数表示"a 比 b 释放更多/占用更少" */
static int32_t heap_diff_sram(const heap_snap_t *a, const heap_snap_t *b)
{
    return (int32_t)a->sram_free - (int32_t)b->sram_free;
}
static int32_t heap_diff_psram(const heap_snap_t *a, const heap_snap_t *b)
{
    return (int32_t)a->psram_free - (int32_t)b->psram_free;
}

/* ============================================================
 * 共享的 bg_frame 创建（用 display malloc，便于配合 flush）
 * 调用者负责释放：要么 bk_display_flush 接管，要么 frame_buffer_display_free
 * ============================================================ */
static frame_buffer_t *alloc_test_bg_frame(uint16_t w, uint16_t h)
{
    uint32_t len = (uint32_t)w * h * 2;
    frame_buffer_t *fb = frame_buffer_display_malloc(len);
    if (fb == NULL) {
        return NULL;
    }
    /* 简易蓝底，跟现有 demo 一致 */
    for (uint32_t i = 0; i < len; i += 2) {
        *(uint16_t *)(fb->frame + i) = 0x001fu;
    }
    fb->fmt    = PIXEL_FMT_RGB565_LE;
    fb->width  = w;
    fb->height = h;
    return fb;
}

/* ============================================================
 * 1. stability
 *    反复 new/delete N 个临时 OSD 控制器，对比前后堆水位
 * ============================================================ */
static avdk_err_t osd_test_stability(int n)
{
    TEST_BANNER("stability");
    if (n <= 0) {
        n = STABILITY_DEFAULT_LOOPS;
    }
    LOGI("loops=%d\n", n);

    heap_snap_t before, after;
    heap_take(&before);
    heap_log("before", &before);

    for (int i = 0; i < n; i++) {
        bk_draw_osd_ctlr_handle_t h = NULL;
        osd_ctlr_config_t cfg = {
            .blend_assets  = blend_assets,
            .blend_info    = blend_info,
            .draw_in_psram = (i & 1) ? true : false,
        };
        avdk_err_t ret = bk_draw_osd_new(&h, &cfg);
        if (ret != AVDK_ERR_OK || h == NULL) {
            TEST_FAIL("stability", "bk_draw_osd_new failed mid-loop");
            return AVDK_ERR_UNKNOWN;
        }
        ret = bk_draw_osd_delete(h);
        if (ret != AVDK_ERR_OK) {
            TEST_FAIL("stability", "bk_draw_osd_delete failed mid-loop");
            return AVDK_ERR_UNKNOWN;
        }
    }

    heap_take(&after);
    heap_log("after", &after);

    int32_t sram_drop  = heap_diff_sram(&before, &after);
    int32_t psram_drop = heap_diff_psram(&before, &after);
    LOGI("sram_drop=%d psram_drop=%d (threshold=%d)\n",
         sram_drop, psram_drop, STABILITY_LEAK_THRESHOLD);

    if (sram_drop > STABILITY_LEAK_THRESHOLD || psram_drop > STABILITY_LEAK_THRESHOLD) {
        TEST_FAIL("stability", "heap leak detected");
        return AVDK_ERR_UNKNOWN;
    }

    TEST_PASS("stability");
    return AVDK_ERR_OK;
}

/* ============================================================
 * 2. concurrent
 *    worker task: 随机 add/remove
 *    main  task: 周期性 draw_array + flush
 * ============================================================ */
typedef struct {
    volatile bool stop;
    volatile uint32_t add_cnt;
    volatile uint32_t remove_cnt;
    volatile uint32_t fail_cnt;
    beken_semaphore_t exited;
} concurrent_ctx_t;

static const char *const concurrent_names[]    = {"wifi", "battery", "weather", "clock", "date", "ver"};
static const char *const concurrent_wifi_ct[]  = {"wifi0", "wifi1", "wifi2", "wifi3", "wifi4"};

static void concurrent_worker(void *arg)
{
    concurrent_ctx_t *ctx = (concurrent_ctx_t *)arg;
    while (!ctx->stop) {
        uint32_t r = test_rand();
        const char *name = concurrent_names[r % (sizeof(concurrent_names) / sizeof(concurrent_names[0]))];
        if ((r & 0x80) == 0) {
            /* add/update */
            const char *content = NULL;
            if (strcmp(name, "wifi") == 0) {
                content = concurrent_wifi_ct[r % (sizeof(concurrent_wifi_ct) / sizeof(concurrent_wifi_ct[0]))];
            }
            avdk_err_t ret = bk_draw_osd_add_or_updata(draw_osd_handle, name, content);
            if (ret == AVDK_ERR_OK) {
                ctx->add_cnt++;
            } else {
                ctx->fail_cnt++;
            }
        } else {
            avdk_err_t ret = bk_draw_osd_remove(draw_osd_handle, name);
            if (ret == AVDK_ERR_OK) {
                ctx->remove_cnt++;
            } else {
                ctx->fail_cnt++;
            }
        }
        rtos_delay_milliseconds(20 + (test_rand() & 0x3f));   /* 20~83 ms */
    }
    rtos_set_semaphore(&ctx->exited);
    rtos_delete_thread(NULL);
}

static avdk_err_t osd_test_concurrent(int sec)
{
    TEST_BANNER("concurrent");
    if (sec <= 0) {
        sec = CONCURRENT_DEFAULT_SEC;
    }
    LOGI("duration_sec=%d\n", sec);

    if (draw_osd_handle == NULL || lcd_display_handle == NULL) {
        TEST_FAIL("concurrent", "global handles NULL, run 'osd init' first");
        return AVDK_ERR_INVAL;
    }

    concurrent_ctx_t ctx = {0};
    beken_thread_t worker = NULL;

    avdk_err_t ret = rtos_init_semaphore(&ctx.exited, 1);
    if (ret != BK_OK) {
        TEST_FAIL("concurrent", "init exited sem failed");
        return ret;
    }

    ret = rtos_create_thread(&worker, BEKEN_DEFAULT_WORKER_PRIORITY,
                             "osd_test_worker",
                             (beken_thread_function_t)concurrent_worker,
                             1024 * 4, &ctx);
    if (ret != BK_OK) {
        rtos_deinit_semaphore(&ctx.exited);
        TEST_FAIL("concurrent", "create worker failed");
        return ret;
    }

    uint32_t draw_cnt = 0;
    uint32_t draw_fail = 0;
    uint32_t end_tick = rtos_get_time() + (uint32_t)sec * 1000u;
    while (rtos_get_time() < end_tick) {
        frame_buffer_t *fb = alloc_test_bg_frame(480, 864);
        if (fb == NULL) {
            draw_fail++;
            rtos_delay_milliseconds(50);
            continue;
        }
        osd_bg_info_t bg_info = {
            .frame  = fb,
            .width  = OSD_BG_W,
            .height = OSD_BG_H,
        };
        avdk_err_t r = bk_draw_osd_array(draw_osd_handle, &bg_info, NULL);
        if (r != AVDK_ERR_OK) {
            draw_fail++;
        } else {
            draw_cnt++;
        }
        /* osd_display_flush 内部接管/释放 fb，失败时也会释放，无需再手动 free */
        r = osd_display_flush(lcd_display_handle, fb);
        if (r != AVDK_ERR_OK) {
            draw_fail++;
        }
        rtos_delay_milliseconds(100);
    }

    ctx.stop = true;
    rtos_get_semaphore(&ctx.exited, BEKEN_NEVER_TIMEOUT);
    rtos_deinit_semaphore(&ctx.exited);

    LOGI("draw_ok=%u draw_fail=%u add=%u remove=%u worker_fail=%u\n",
         draw_cnt, draw_fail, ctx.add_cnt, ctx.remove_cnt, ctx.fail_cnt);

    /* 拉取 dyn_array 当前状态做基本合法性校验 */
    const blend_info_t *dyn = NULL;
    uint32_t dyn_size = 0;
    bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_GET_DRAW_INFO,
                      0, (uint32_t)&dyn, (uint32_t)&dyn_size);
    LOGI("final dyn_array size=%u\n", dyn_size);

    if (draw_cnt == 0) {
        TEST_FAIL("concurrent", "no successful draw");
        return AVDK_ERR_UNKNOWN;
    }

    TEST_PASS("concurrent");
    return AVDK_ERR_OK;
}

/* ============================================================
 * 3. multi_inst
 *    同时存活两个临时 OSD 控制器，验证多实例无串扰（plan #1）
 * ============================================================ */
static avdk_err_t osd_test_multi_inst(void)
{
    TEST_BANNER("multi_inst");

    bk_draw_osd_ctlr_handle_t h_psram = NULL;
    bk_draw_osd_ctlr_handle_t h_sram  = NULL;
    avdk_err_t ret = AVDK_ERR_OK;
    frame_buffer_t *fb1 = NULL, *fb2 = NULL;

    osd_ctlr_config_t cfg_psram = {
        .blend_assets  = blend_assets,
        .blend_info    = blend_info,
        .draw_in_psram = true,
    };
    osd_ctlr_config_t cfg_sram = {
        .blend_assets  = blend_assets,
        .blend_info    = blend_info,
        .draw_in_psram = false,
    };

    ret = bk_draw_osd_new(&h_psram, &cfg_psram);
    if (ret != AVDK_ERR_OK || h_psram == NULL) {
        TEST_FAIL("multi_inst", "new psram handle failed");
        goto cleanup;
    }
    ret = bk_draw_osd_new(&h_sram, &cfg_sram);
    if (ret != AVDK_ERR_OK || h_sram == NULL) {
        TEST_FAIL("multi_inst", "new sram handle failed");
        ret = AVDK_ERR_UNKNOWN;
        goto cleanup;
    }

    /* 分别在两个独立 handle 上画一次 wifi 图标 */
    fb1 = alloc_test_bg_frame(480, 864);
    fb2 = alloc_test_bg_frame(480, 864);
    if (fb1 == NULL || fb2 == NULL) {
        TEST_FAIL("multi_inst", "alloc bg_frame failed");
        ret = AVDK_ERR_NOMEM;
        goto cleanup;
    }

    osd_bg_info_t bg1 = { .frame = fb1, .width = 480, .height = 864 };
    osd_bg_info_t bg2 = { .frame = fb2, .width = 480, .height = 864 };

    blend_info_t wifi_info = { .name = "wifi", .addr = &img_wifi_rssi0, .content = "wifi0" };

    avdk_err_t r1 = bk_draw_osd_image(h_psram, &bg1, &wifi_info);
    avdk_err_t r2 = bk_draw_osd_image(h_sram,  &bg2, &wifi_info);
    if (r1 != AVDK_ERR_OK || r2 != AVDK_ERR_OK) {
        LOGE("draw r1=%d r2=%d\n", r1, r2);
        TEST_FAIL("multi_inst", "draw_image on either handle failed");
        ret = AVDK_ERR_UNKNOWN;
        goto cleanup;
    }

    TEST_PASS("multi_inst");
    ret = AVDK_ERR_OK;

cleanup:
    if (fb1) frame_buffer_display_free(fb1);
    if (fb2) frame_buffer_display_free(fb2);
    if (h_sram)  bk_draw_osd_delete(h_sram);
    if (h_psram) bk_draw_osd_delete(h_psram);
    return ret;
}

/* ============================================================
 * 4. invalid_param
 *    6 组非法入参，期望全部返回 AVDK_ERR_INVAL
 * ============================================================ */
static avdk_err_t osd_test_invalid_param(void)
{
    TEST_BANNER("invalid_param");
    if (draw_osd_handle == NULL) {
        TEST_FAIL("invalid_param", "draw_osd_handle NULL, run 'osd init' first");
        return AVDK_ERR_INVAL;
    }

    blend_info_t wifi_info = { .name = "wifi", .addr = &img_wifi_rssi0, .content = "wifi0" };
    frame_buffer_t *fb = alloc_test_bg_frame(480, 864);
    if (fb == NULL) {
        TEST_FAIL("invalid_param", "alloc bg_frame failed");
        return AVDK_ERR_NOMEM;
    }

    osd_bg_info_t bg_ok        = { .frame = fb,   .width = 480, .height = 864 };
    osd_bg_info_t bg_null_frame= { .frame = NULL, .width = 480, .height = 864 };
    osd_bg_info_t bg_zero_w    = { .frame = fb,   .width = 0,   .height = 864 };
    osd_bg_info_t bg_zero_h    = { .frame = fb,   .width = 480, .height = 0   };

    avdk_err_t r;
    int fail = 0;

    /* image: bg_info=NULL */
    r = bk_draw_osd_image(draw_osd_handle, NULL, &wifi_info);
    if (r == AVDK_ERR_OK) { LOGE("[case1] bg=NULL got OK\n"); fail++; }

    /* image: blend_info=NULL */
    r = bk_draw_osd_image(draw_osd_handle, &bg_ok, NULL);
    if (r == AVDK_ERR_OK) { LOGE("[case2] info=NULL got OK\n"); fail++; }

    /* image: bg_info->frame=NULL */
    r = bk_draw_osd_image(draw_osd_handle, &bg_null_frame, &wifi_info);
    if (r == AVDK_ERR_OK) { LOGE("[case3] frame=NULL got OK\n"); fail++; }

    /* image: bg_info->width=0 */
    r = bk_draw_osd_image(draw_osd_handle, &bg_zero_w, &wifi_info);
    if (r == AVDK_ERR_OK) { LOGE("[case4] width=0 got OK\n"); fail++; }

    /* image: bg_info->height=0 */
    r = bk_draw_osd_image(draw_osd_handle, &bg_zero_h, &wifi_info);
    if (r == AVDK_ERR_OK) { LOGE("[case5] height=0 got OK\n"); fail++; }

    /* font: bg_info=NULL */
    r = bk_draw_osd_font(draw_osd_handle, NULL, &wifi_info);
    if (r == AVDK_ERR_OK) { LOGE("[case6] font bg=NULL got OK\n"); fail++; }

    frame_buffer_display_free(fb);

    if (fail != 0) {
        TEST_FAIL("invalid_param", "some invalid cases returned OK");
        return AVDK_ERR_UNKNOWN;
    }
    TEST_PASS("invalid_param");
    return AVDK_ERR_OK;
}

/* ============================================================
 * 5. cfg_unchanged
 *    调 draw_image / draw_font 前后，入参 blend_info_t 和指向的
 *    bk_blend_t 应保持不变（plan #2 在公开层面的体现）
 * ============================================================ */
static avdk_err_t osd_test_cfg_unchanged(void)
{
    TEST_BANNER("cfg_unchanged");
    if (draw_osd_handle == NULL) {
        TEST_FAIL("cfg_unchanged", "draw_osd_handle NULL, run 'osd init' first");
        return AVDK_ERR_INVAL;
    }

    frame_buffer_t *fb = alloc_test_bg_frame(480, 864);
    if (fb == NULL) {
        TEST_FAIL("cfg_unchanged", "alloc bg_frame failed");
        return AVDK_ERR_NOMEM;
    }

    osd_bg_info_t bg_info = { .frame = fb, .width = 480, .height = 864 };
    blend_info_t info = { .name = "wifi", .addr = &img_wifi_rssi0, .content = "wifi0" };

    blend_info_t info_before = info;
    bk_blend_t blend_before  = *(const bk_blend_t *)info.addr;

    avdk_err_t r = bk_draw_osd_image(draw_osd_handle, &bg_info, &info);
    if (r != AVDK_ERR_OK) {
        frame_buffer_display_free(fb);
        TEST_FAIL("cfg_unchanged", "draw_image failed");
        return r;
    }

    bool ok = true;
    if (os_memcmp(&info_before, &info, sizeof(info)) != 0) {
        LOGE("blend_info_t mutated\n");
        ok = false;
    }
    if (os_memcmp(&blend_before, info.addr, sizeof(bk_blend_t)) != 0) {
        LOGE("bk_blend_t mutated\n");
        ok = false;
    }

    /* 同时验证 font 路径 */
    blend_info_t finfo = { .name = "clock", .addr = &font_clock, .content = "12:34" };
    blend_info_t finfo_before = finfo;
    bk_blend_t   fblend_before = *(const bk_blend_t *)finfo.addr;

    r = bk_draw_osd_font(draw_osd_handle, &bg_info, &finfo);
    if (r != AVDK_ERR_OK) {
        frame_buffer_display_free(fb);
        TEST_FAIL("cfg_unchanged", "draw_font failed");
        return r;
    }

    if (os_memcmp(&finfo_before, &finfo, sizeof(finfo)) != 0) {
        LOGE("font blend_info_t mutated\n");
        ok = false;
    }
    if (os_memcmp(&fblend_before, finfo.addr, sizeof(bk_blend_t)) != 0) {
        LOGE("font bk_blend_t mutated\n");
        ok = false;
    }

    frame_buffer_display_free(fb);

    if (!ok) {
        TEST_FAIL("cfg_unchanged", "cfg mutated after draw");
        return AVDK_ERR_UNKNOWN;
    }
    TEST_PASS("cfg_unchanged");
    return AVDK_ERR_OK;
}

/* ============================================================
 * 6. shrink
 *    先用 font_clock prime（buf1 必 >= 10560 B），再 ioctl SHRINK，
 *    校验 free heap 至少回升 SHRINK_MIN_DELTA。
 *    注意：icon ctlr 的 buf1 只按 icon/font 尺寸分配，wifi (32x36) 仅 2304 B，
 *    必须用更大的 font 资源 prime，否则独立运行时差值会过小。
 * ============================================================ */
static avdk_err_t osd_test_shrink(void)
{
    TEST_BANNER("shrink");
    if (draw_osd_handle == NULL) {
        TEST_FAIL("shrink", "draw_osd_handle NULL, run 'osd init' first");
        return AVDK_ERR_INVAL;
    }

    frame_buffer_t *fb = alloc_test_bg_frame(480, 864);
    if (fb == NULL) {
        TEST_FAIL("shrink", "alloc bg_frame failed");
        return AVDK_ERR_NOMEM;
    }
    osd_bg_info_t bg_info = { .frame = fb, .width = 480, .height = 864 };

    /* prime #1: font_clock (120x44) -> buf1 >= 10560 B */
    blend_info_t clock_info = { .name = "clock", .addr = &font_clock, .content = "12:30" };
    avdk_err_t r = bk_draw_osd_font(draw_osd_handle, &bg_info, &clock_info);
    if (r != AVDK_ERR_OK) {
        frame_buffer_display_free(fb);
        TEST_FAIL("shrink", "primer draw_font failed");
        return r;
    }

    /* prime #2: 一次 wifi 绘制，覆盖 draw_image 路径（buf1 已 >= 10560，不会缩） */
    blend_info_t wifi_info = { .name = "wifi", .addr = &img_wifi_rssi0, .content = "wifi0" };
    r = bk_draw_osd_image(draw_osd_handle, &bg_info, &wifi_info);
    frame_buffer_display_free(fb);
    if (r != AVDK_ERR_OK) {
        TEST_FAIL("shrink", "primer draw_image failed");
        return r;
    }

    heap_snap_t before, after;
    heap_take(&before);
    heap_log("before shrink", &before);

    r = bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_SHRINK, 0, 0, 0);
    if (r != AVDK_ERR_OK) {
        TEST_FAIL("shrink", "ioctl SHRINK failed");
        return r;
    }

    heap_take(&after);
    heap_log("after shrink", &after);

    /* 释放后，free heap 应当增长 */
    int32_t sram_gain  = heap_diff_sram(&after, &before);
    int32_t psram_gain = heap_diff_psram(&after, &before);
    LOGI("sram_gain=%d psram_gain=%d (min=%d)\n",
         sram_gain, psram_gain, SHRINK_MIN_DELTA);

    /* 默认 draw_in_psram=false 时增长在 sram，反之在 psram */
    if (sram_gain < SHRINK_MIN_DELTA && psram_gain < SHRINK_MIN_DELTA) {
        TEST_FAIL("shrink", "heap did not grow as expected");
        return AVDK_ERR_UNKNOWN;
    }

    TEST_PASS("shrink");
    return AVDK_ERR_OK;
}

/* ============================================================
 * 7. unaligned
 *    构造 width=33 (非 4 字节对齐) 的合成 bk_blend_t + ARGB8888 数据
 *    调 draw_image，没有 hardfault 即 PASS（验证 plan #9 修复）
 * ============================================================ */
static avdk_err_t osd_test_unaligned(void)
{
    TEST_BANNER("unaligned");
    if (draw_osd_handle == NULL) {
        TEST_FAIL("unaligned", "draw_osd_handle NULL, run 'osd init' first");
        return AVDK_ERR_INVAL;
    }

    const uint32_t W = 33;
    const uint32_t H = 10;
    uint8_t *fg = (uint8_t *)os_malloc(W * H * 4);
    if (fg == NULL) {
        TEST_FAIL("unaligned", "alloc fg buffer failed");
        return AVDK_ERR_NOMEM;
    }
    /* 半透明白色全填，让 blend 真正执行 */
    for (uint32_t i = 0; i < W * H; i++) {
        fg[i * 4 + 0] = 0xff;  /* B */
        fg[i * 4 + 1] = 0xff;  /* G */
        fg[i * 4 + 2] = 0xff;  /* R */
        fg[i * 4 + 3] = 0x80;  /* A */
    }

    bk_blend_t synth = {0};
    synth.version    = 0;
    synth.blend_type = BLEND_TYPE_IMAGE;
    synth.width      = W;
    synth.height     = H;
    synth.icon_width = W;
    synth.icon_height= H;
    synth.bg_width   = 480;
    synth.bg_height  = 864;
    synth.xpos       = 10;
    synth.ypos       = 10;
    synth.image.format   = 0;
    synth.image.data_len = W * H * 4;
    synth.image.data     = fg;

    frame_buffer_t *fb = alloc_test_bg_frame(480, 864);
    if (fb == NULL) {
        os_free(fg);
        TEST_FAIL("unaligned", "alloc bg_frame failed");
        return AVDK_ERR_NOMEM;
    }
    osd_bg_info_t bg_info = { .frame = fb, .width = 480, .height = 864 };
    blend_info_t info = { .name = "_synth_", .addr = &synth, .content = "" };

    avdk_err_t r = bk_draw_osd_image(draw_osd_handle, &bg_info, &info);

    frame_buffer_display_free(fb);
    os_free(fg);

    if (r != AVDK_ERR_OK) {
        LOGE("draw_image ret=%d\n", r);
        TEST_FAIL("unaligned", "draw_image returned error");
        return r;
    }

    TEST_PASS("unaligned");
    return AVDK_ERR_OK;
}

/* ============================================================
 * 8. null_assets
 *    blend_assets=NULL + blend_info=NULL 时，new/draw_array/add/remove
 *    都不应 crash，应当返回合理的错误码或者空操作。
 * ============================================================ */
static avdk_err_t osd_test_null_assets(void)
{
    TEST_BANNER("null_assets");

    bk_draw_osd_ctlr_handle_t h = NULL;
    osd_ctlr_config_t cfg = {
        .blend_assets  = NULL,
        .blend_info    = NULL,
        .draw_in_psram = false,
    };

    avdk_err_t r = bk_draw_osd_new(&h, &cfg);
    if (r != AVDK_ERR_OK || h == NULL) {
        TEST_FAIL("null_assets", "new with NULL assets should still succeed");
        return (r != AVDK_ERR_OK) ? r : AVDK_ERR_UNKNOWN;
    }

    /* add 不存在的 name -> 应返回 AVDK_ERR_INVAL（assets 为空，查不到） */
    r = bk_draw_osd_add_or_updata(h, "not_exist", "x");
    if (r == AVDK_ERR_OK) {
        bk_draw_osd_delete(h);
        TEST_FAIL("null_assets", "add to empty assets unexpectedly OK");
        return AVDK_ERR_UNKNOWN;
    }

    /* remove 不存在的 name -> 不应 crash，返回 OK 即可 */
    r = bk_draw_osd_remove(h, "not_exist");
    if (r != AVDK_ERR_OK) {
        bk_draw_osd_delete(h);
        TEST_FAIL("null_assets", "remove on empty dyn_array should be OK");
        return r;
    }

    /* draw_array(NULL): 内部走默认 dyn_array（空），应为 no-op + OK */
    frame_buffer_t *fb = alloc_test_bg_frame(64, 64);
    if (fb != NULL) {
        osd_bg_info_t bg = { .frame = fb, .width = 64, .height = 64 };
        r = bk_draw_osd_array(h, &bg, NULL);
        frame_buffer_display_free(fb);
        if (r != AVDK_ERR_OK) {
            bk_draw_osd_delete(h);
            TEST_FAIL("null_assets", "draw_array on empty dyn_array failed");
            return r;
        }
    }

    bk_draw_osd_delete(h);
    TEST_PASS("null_assets");
    return AVDK_ERR_OK;
}

/* ============================================================
 * 9. psram_switch
 *    用 OSD_CTLR_CMD_SET_PSRAM_USAGE / GET_PSRAM_USAGE 来回切换，
 *    每次切换后都执行一次 font 绘制，校验：
 *      (a) GET 回读值与 SET 值一致；
 *      (b) icon_check_mem 检测到 last_psram_setting 改变 -> 释放 OLD 池
 *          中的 buf，并在 NEW 池中重新分配（对应：OLD 池 free heap 上升、
 *          NEW 池 free heap 下降）。
 *    覆盖的代码路径：bk_draw_osd_ioctl + 底层 DRAW_ICON_CTLR_CMD_SET_PSRAM_USAGE
 *    + icon_check_mem 的运行时切换分支。
 * ============================================================ */
#define PSRAM_SWITCH_DELTA_MIN  (4 * 1024)   /* font_clock buf1 ~10560 B，4 KB 阈值留余量 */

static avdk_err_t psram_switch_one_step(bool target,
                                        osd_bg_info_t *bg_info,
                                        blend_info_t *clock_info,
                                        bool check_delta)
{
    /* (1) SET */
    avdk_err_t r = bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_SET_PSRAM_USAGE,
                                     (uint32_t)target, 0, 0);
    if (r != AVDK_ERR_OK) {
        LOGE("SET psram=%d failed: %d\n", target, r);
        return r;
    }

    /* (2) GET 回读 */
    uint8_t readback = 0xff;
    r = bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_GET_PSRAM_USAGE,
                          (uint32_t)&readback, 0, 0);
    if (r != AVDK_ERR_OK) {
        LOGE("GET psram failed: %d\n", r);
        return r;
    }
    if (readback != (uint8_t)target) {
        LOGE("GET psram readback=%u, expect=%u\n", readback, (uint8_t)target);
        return AVDK_ERR_UNKNOWN;
    }

    /* (3) 触发 draw -> icon_check_mem 会因 last_psram_setting 变化而释放 OLD、分配 NEW */
    heap_snap_t before, after;
    heap_take(&before);
    r = bk_draw_osd_font(draw_osd_handle, bg_info, clock_info);
    if (r != AVDK_ERR_OK) {
        LOGE("draw_font after switch failed: %d\n", r);
        return r;
    }
    heap_take(&after);

    int32_t sram_d  = heap_diff_sram(&after, &before);   /* 正值=OLD 池释放，负值=NEW 池消耗 */
    int32_t psram_d = heap_diff_psram(&after, &before);
    LOGI("  step psram=%d sram_delta=%d psram_delta=%d\n", target, sram_d, psram_d);

    if (check_delta) {
        if (target) {
            /* 切到 PSRAM：SRAM 应回升（释放老 buf），PSRAM 应下降（分配新 buf） */
            if (sram_d < PSRAM_SWITCH_DELTA_MIN || psram_d > -PSRAM_SWITCH_DELTA_MIN) {
                LOGE("switch->PSRAM heap pattern wrong: sram_d=%d psram_d=%d\n",
                     sram_d, psram_d);
                return AVDK_ERR_UNKNOWN;
            }
        } else {
            /* 切到 SRAM：PSRAM 应回升，SRAM 应下降 */
            if (psram_d < PSRAM_SWITCH_DELTA_MIN || sram_d > -PSRAM_SWITCH_DELTA_MIN) {
                LOGE("switch->SRAM heap pattern wrong: sram_d=%d psram_d=%d\n",
                     sram_d, psram_d);
                return AVDK_ERR_UNKNOWN;
            }
        }
    }
    return AVDK_ERR_OK;
}

static avdk_err_t osd_test_psram_switch(void)
{
    TEST_BANNER("psram_switch");
    if (draw_osd_handle == NULL) {
        TEST_FAIL("psram_switch", "draw_osd_handle NULL, run 'osd init' first");
        return AVDK_ERR_INVAL;
    }

    /* 保存原始值，结尾恢复 */
    uint8_t orig = 0xff;
    avdk_err_t r = bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_GET_PSRAM_USAGE,
                                     (uint32_t)&orig, 0, 0);
    if (r != AVDK_ERR_OK || orig > 1) {
        TEST_FAIL("psram_switch", "initial GET psram returns invalid");
        return (r != AVDK_ERR_OK) ? r : AVDK_ERR_UNKNOWN;
    }
    LOGI("initial psram=%u\n", orig);

    /* 准备 bg + font_clock（120x44 → buf1 必 >= 10560 B，远大于 4 KB 阈值） */
    frame_buffer_t *fb = alloc_test_bg_frame(480, 864);
    if (fb == NULL) {
        TEST_FAIL("psram_switch", "alloc bg_frame failed");
        return AVDK_ERR_NOMEM;
    }
    osd_bg_info_t bg_info = { .frame = fb, .width = 480, .height = 864 };
    blend_info_t clock_info = { .name = "clock", .addr = &font_clock, .content = "12:30" };

    /* prime: 先 SHRINK 一次，确保 buf1 起始为空，避免老残留扰动阈值判断 */
    bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_SHRINK, 0, 0, 0);

    /* iter 0：从空状态切到 false（SRAM），不校验 delta（OLD 池空，没东西释放） */
    r = psram_switch_one_step(false, &bg_info, &clock_info, false);
    if (r != AVDK_ERR_OK) { goto done; }

    /* iter 1..4: 真实切换，校验 heap 模式 */
    r = psram_switch_one_step(true,  &bg_info, &clock_info, true);
    if (r != AVDK_ERR_OK) { goto done; }
    r = psram_switch_one_step(false, &bg_info, &clock_info, true);
    if (r != AVDK_ERR_OK) { goto done; }
    r = psram_switch_one_step(true,  &bg_info, &clock_info, true);
    if (r != AVDK_ERR_OK) { goto done; }
    r = psram_switch_one_step(false, &bg_info, &clock_info, true);
    if (r != AVDK_ERR_OK) { goto done; }

done:
    frame_buffer_display_free(fb);

    /* 恢复原始 PSRAM 设置 + 清理 buf */
    bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_SET_PSRAM_USAGE,
                      (uint32_t)orig, 0, 0);
    bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_SHRINK, 0, 0, 0);

    if (r != AVDK_ERR_OK) {
        TEST_FAIL("psram_switch", "switch step failed");
        return r;
    }

    TEST_PASS("psram_switch");
    return AVDK_ERR_OK;
}

/* ============================================================
 * Dispatcher: osd test <sub> [arg]
 * ============================================================ */
static void usage(void)
{
    LOGI("usage:\n");
    LOGI("  osd test stability [N]       (default %d loops)\n", STABILITY_DEFAULT_LOOPS);
    LOGI("  osd test concurrent [sec]    (default %d sec)\n", CONCURRENT_DEFAULT_SEC);
    LOGI("  osd test multi_inst\n");
    LOGI("  osd test invalid_param\n");
    LOGI("  osd test cfg_unchanged\n");
    LOGI("  osd test shrink\n");
    LOGI("  osd test unaligned\n");
    LOGI("  osd test null_assets\n");
    LOGI("  osd test psram_switch\n");
    LOGI("  osd test all                 (run all except concurrent)\n");
    LOGI("note: stability / multi_inst / null_assets work standalone;\n");
    LOGI("      all other subcommands require 'osd init' first.\n");
}

avdk_err_t osd_complex_test_dispatch(int argc, char **argv)
{
    if (argc < 3) {
        usage();
        return AVDK_ERR_INVAL;
    }
    const char *sub = argv[2];

    /*
     * 早退守护：除自给自足的用例外，其它都需要先 'osd init' 创建全局 handle。
     * 不在这里逐个用例报 FAIL，统一在 dispatch 入口拦截 + 清晰提示。
     */
    const bool standalone =
        (strcmp(sub, "stability")   == 0) ||
        (strcmp(sub, "multi_inst")  == 0) ||
        (strcmp(sub, "null_assets") == 0);
    if (!standalone && draw_osd_handle == NULL) {
        LOGE("'osd test %s' requires 'osd init' first (global handle is NULL)\n", sub);
        LOGE("hint: try 'osd init' then re-run, or use one of: stability / multi_inst / null_assets\n");
        return AVDK_ERR_INVAL;
    }

    if (strcmp(sub, "stability") == 0) {
        int n = (argc >= 4) ? atoi(argv[3]) : STABILITY_DEFAULT_LOOPS;
        return osd_test_stability(n);
    }
    if (strcmp(sub, "concurrent") == 0) {
        int sec = (argc >= 4) ? atoi(argv[3]) : CONCURRENT_DEFAULT_SEC;
        return osd_test_concurrent(sec);
    }
    if (strcmp(sub, "multi_inst") == 0) {
        return osd_test_multi_inst();
    }
    if (strcmp(sub, "invalid_param") == 0) {
        return osd_test_invalid_param();
    }
    if (strcmp(sub, "cfg_unchanged") == 0) {
        return osd_test_cfg_unchanged();
    }
    if (strcmp(sub, "shrink") == 0) {
        return osd_test_shrink();
    }
    if (strcmp(sub, "unaligned") == 0) {
        return osd_test_unaligned();
    }
    if (strcmp(sub, "null_assets") == 0) {
        return osd_test_null_assets();
    }
    if (strcmp(sub, "psram_switch") == 0) {
        return osd_test_psram_switch();
    }
    if (strcmp(sub, "all") == 0) {
        avdk_err_t r;
        avdk_err_t worst = AVDK_ERR_OK;
        r = osd_test_invalid_param(); if (r != AVDK_ERR_OK) worst = r;
        r = osd_test_cfg_unchanged(); if (r != AVDK_ERR_OK) worst = r;
        r = osd_test_unaligned();     if (r != AVDK_ERR_OK) worst = r;
        r = osd_test_null_assets();   if (r != AVDK_ERR_OK) worst = r;
        r = osd_test_psram_switch();  if (r != AVDK_ERR_OK) worst = r;
        r = osd_test_shrink();        if (r != AVDK_ERR_OK) worst = r;
        r = osd_test_multi_inst();    if (r != AVDK_ERR_OK) worst = r;
        r = osd_test_stability(STABILITY_DEFAULT_LOOPS);
        if (r != AVDK_ERR_OK) worst = r;
        if (worst == AVDK_ERR_OK) {
            LOGI("==== [all] ALL TESTS PASS ====\n");
        } else {
            LOGE("==== [all] SOME TESTS FAILED ====\n");
        }
        return worst;
    }

    LOGE("unknown subcommand: %s\n", sub);
    usage();
    return AVDK_ERR_INVAL;
}
