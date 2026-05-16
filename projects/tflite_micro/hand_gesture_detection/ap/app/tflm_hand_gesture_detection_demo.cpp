#include "tflm_hand_gesture_detection_demo.h"

/** 1: Invoke() 前后拉高/拉低 GPIO_55 供逻辑分析仪测时；0: 不编译 GPIO 相关代码（也可传 -DTFLM_INVOKE_TIME_TEST=0）。 */
#ifndef TFLM_INVOKE_TIME_TEST
#define TFLM_INVOKE_TIME_TEST 1
#endif

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "components/log.h"
#include "os/mem.h"

extern "C" {
#include "os/os.h"
#include "os/str.h"
#include <components/system.h>
#include <components/bk_frame_buffer.h>
#if TFLM_INVOKE_TIME_TEST
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#endif
#include "dwt.h"
void *__dso_handle = 0;
}

#include "hand_gesture_image_input.h"
#include "bk_ethosu.h"

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/kernels/conv.h"

static char TAG[] = "hand_gst_tflm";
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

// DWT interval logging (delta / 480 -> us at 480 MHz CPU; >= 1 ms printed as ms).
static void hgd_log_dwt_interval(const char *prefix, uint32_t start_cycles)
{
    uint32_t cycles = dwt_get_cycle_counter_val() - start_cycles;
    uint32_t us = cycles / 480U;
    if (us < 1000U) {
        LOGI("%s: %u us\r\n", prefix, us);
    } else {
        LOGI("%s: %u ms\r\n", prefix, us / 1000U);
    }
}

typedef enum {
    TFLM_MEM_SRC_NONE = 0,
    TFLM_MEM_SRC_HSRAM,
    TFLM_MEM_SRC_PSRAM,
    TFLM_MEM_SRC_MEM_SLAB_UNCODED,
} tflm_mem_src_t;

// Which heap each TFLM buffer uses (set here; tflm_runtime_alloc does not search fallbacks).
static tflm_mem_src_t g_ethosu0_scratch_src = TFLM_MEM_SRC_HSRAM;
static tflm_mem_src_t g_model_data_src = TFLM_MEM_SRC_MEM_SLAB_UNCODED;
static tflm_mem_src_t g_tensor_arena_src = TFLM_MEM_SRC_MEM_SLAB_UNCODED;

static const char *tflm_mem_src_name(tflm_mem_src_t src)
{
    if (src == TFLM_MEM_SRC_HSRAM) {
        return "HSRAM";
    }
    if (src == TFLM_MEM_SRC_PSRAM) {
        return "PSRAM";
    }
    if (src == TFLM_MEM_SRC_MEM_SLAB_UNCODED) {
        return "MEM_SLAB_UNCODED";
    }
    return "NONE";
}

static void *tflm_mem_alloc_one(size_t size, tflm_mem_src_t kind)
{
    switch (kind) {
    case TFLM_MEM_SRC_HSRAM:
        return hsram_malloc(size);
    case TFLM_MEM_SRC_PSRAM:
        return psram_malloc(size);
    case TFLM_MEM_SRC_MEM_SLAB_UNCODED:
        if (size == 0U || size > (size_t)UINT32_MAX) {
            return nullptr;
        }
        return bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, (uint32_t)size);
    case TFLM_MEM_SRC_NONE:
    default:
        return nullptr;
    }
}

// Allocates from *policy only; *policy must be set to a non-NONE type before the call.
static void *tflm_runtime_alloc(size_t size, tflm_mem_src_t *policy)
{
    if (policy == nullptr || *policy == TFLM_MEM_SRC_NONE) {
        return nullptr;
    }
    return tflm_mem_alloc_one(size, *policy);
}

static void tflm_runtime_free(void *ptr, tflm_mem_src_t src)
{
    if (ptr == nullptr) {
        return;
    }
    if (src == TFLM_MEM_SRC_HSRAM) {
        hsram_free(ptr);
    } else if (src == TFLM_MEM_SRC_PSRAM) {
        psram_free(ptr);
    } else if (src == TFLM_MEM_SRC_MEM_SLAB_UNCODED) {
        bk_frame_buffer_free(ptr);
    }
}

static void *tflm_scratch_alloc(size_t size, tflm_mem_src_t *src)
{
    return tflm_runtime_alloc(size, src);
}

static void tflm_scratch_free(void *ptr, tflm_mem_src_t src)
{
    tflm_runtime_free(ptr, src);
}

static uint8_t *g_tensor_arena = nullptr;
static tflite::MicroInterpreter *g_interpreter = nullptr;
static bool g_interpreter_initialized = false;
static void *g_ethosu0_scratch = nullptr;

static uint8_t *g_model_data_raw = nullptr;
static uint8_t *g_model_data = nullptr;
static size_t g_model_data_size = 0;

struct HgdDet {
    float x1, y1, x2, y2;
    float score;
    int class_id;
};

static float g_boxes_xyxy[k_num_candidates][4];
static float g_scores[k_num_candidates][k_num_classes];
static HgdDet g_merge[k_num_candidates];

static float box_iou_xyxy(float x1a, float y1a, float x2a, float y2a, float x1b, float y1b, float x2b, float y2b)
{
    float xx1 = fmaxf(x1a, x1b);
    float yy1 = fmaxf(y1a, y1b);
    float xx2 = fminf(x2a, x2b);
    float yy2 = fminf(y2a, y2b);
    float w = fmaxf(0.f, xx2 - xx1);
    float h = fmaxf(0.f, yy2 - yy1);
    float inter = w * h;
    float aw = fmaxf(0.f, x2a - x1a);
    float ah = fmaxf(0.f, y2a - y1a);
    float bw = fmaxf(0.f, x2b - x1b);
    float bh = fmaxf(0.f, y2b - y1b);
    float u = aw * ah + bw * bh - inter;
    return (u > 1e-6f) ? (inter / u) : 0.f;
}

static void letterbox_params(int orig_w, int orig_h, float *scale, int *pad_top, int *pad_left)
{
    float s = (float)k_input_h / (float)orig_h;
    float s2 = (float)k_input_w / (float)orig_w;
    if (s2 < s) {
        s = s2;
    }
    int nh = (int)(orig_h * s);
    int nw = (int)(orig_w * s);
    *pad_top = (k_input_h - nh) / 2;
    *pad_left = (k_input_w - nw) / 2;
    *scale = s;
}

static float clipf(float v, float lo, float hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static int nms_subset(const int *idxs, int n, int class_id, float iou_thresh, int *out_idx)
{
    static int order[k_num_candidates];
    static float order_score[k_num_candidates];
    for (int i = 0; i < n; i++) {
        order[i] = idxs[i];
        order_score[i] = g_scores[idxs[i]][class_id];
    }
    for (int a = 0; a < n - 1; a++) {
        for (int b = a + 1; b < n; b++) {
            if (order_score[b] > order_score[a]) {
                float ts = order_score[a];
                order_score[a] = order_score[b];
                order_score[b] = ts;
                int ti = order[a];
                order[a] = order[b];
                order[b] = ti;
            }
        }
    }
    int kept = 0;
    for (int i = 0; i < n; i++) {
        int ri = order[i];
        float x1a = g_boxes_xyxy[ri][0], y1a = g_boxes_xyxy[ri][1];
        float x2a = g_boxes_xyxy[ri][2], y2a = g_boxes_xyxy[ri][3];
        bool ok = true;
        for (int t = 0; t < kept; t++) {
            int kj = out_idx[t];
            if (box_iou_xyxy(x1a, y1a, x2a, y2a, g_boxes_xyxy[kj][0], g_boxes_xyxy[kj][1], g_boxes_xyxy[kj][2],
                                              g_boxes_xyxy[kj][3]) > iou_thresh) {
                ok = false;
                break;
            }
        }
        if (ok) {
            out_idx[kept++] = ri;
        }
    }
    return kept;
}

static int postprocess_yolov8_int8(const int8_t *out_buf, float out_scale, int out_zp, float scale, int pad_top,
                                                                    int pad_left, int orig_w, int orig_h, HgdDet *out, int max_out)
{
    for (int i = 0; i < k_num_candidates; i++) {
        float row[k_out_channels];
        for (int k = 0; k < k_out_channels; k++) {
            int8_t q = out_buf[k * k_num_candidates + i];
            row[k] = ((float)q - (float)out_zp) * out_scale;
        }
        float cx = row[0] * (float)k_input_w;
        float cy = row[1] * (float)k_input_h;
        float w = row[2] * (float)k_input_w;
        float h = row[3] * (float)k_input_h;
        g_boxes_xyxy[i][0] = cx - w * 0.5f;
        g_boxes_xyxy[i][1] = cy - h * 0.5f;
        g_boxes_xyxy[i][2] = cx + w * 0.5f;
        g_boxes_xyxy[i][3] = cy + h * 0.5f;
        for (int c = 0; c < k_num_classes; c++) {
            g_scores[i][c] = row[4 + c];
        }
    }

    static int filt[k_num_candidates];
    int nf = 0;
    for (int i = 0; i < k_num_candidates; i++) {
        float m = g_scores[i][0];
        for (int c = 1; c < k_num_classes; c++) {
            if (g_scores[i][c] > m) {
                m = g_scores[i][c];
            }
        }
        if (m > k_conf_threshold) {
            filt[nf++] = i;
        }
    }
    if (nf == 0) {
        return 0;
    }

    static int work[k_num_candidates];
    static int kept_idx[k_num_candidates];
    int nt = 0;

    for (int c = 0; c < k_num_classes; c++) {
        int nw = 0;
        for (int j = 0; j < nf; j++) {
            int i = filt[j];
            if (g_scores[i][c] > k_conf_threshold) {
                work[nw++] = i;
            }
        }
        if (nw == 0) {
            continue;
        }
        int nk = nms_subset(work, nw, c, k_iou_threshold, kept_idx);
        for (int t = 0; t < nk; t++) {
            int ri = kept_idx[t];
            if (nt >= k_num_candidates) {
                break;
            }
            g_merge[nt].x1 = g_boxes_xyxy[ri][0];
            g_merge[nt].y1 = g_boxes_xyxy[ri][1];
            g_merge[nt].x2 = g_boxes_xyxy[ri][2];
            g_merge[nt].y2 = g_boxes_xyxy[ri][3];
            g_merge[nt].score = g_scores[ri][c];
            g_merge[nt].class_id = c;
            nt++;
        }
    }

    for (int a = 0; a < nt - 1; a++) {
        for (int b = a + 1; b < nt; b++) {
            if (g_merge[b].score > g_merge[a].score) {
                HgdDet t = g_merge[a];
                g_merge[a] = g_merge[b];
                g_merge[b] = t;
            }
        }
    }

    int nout = 0;
    for (int i = 0; i < nt && nout < max_out; i++) {
        float x1 = clipf((g_merge[i].x1 - (float)pad_left) / scale, 0.f, (float)orig_w);
        float y1 = clipf((g_merge[i].y1 - (float)pad_top) / scale, 0.f, (float)orig_h);
        float x2 = clipf((g_merge[i].x2 - (float)pad_left) / scale, 0.f, (float)orig_w);
        float y2 = clipf((g_merge[i].y2 - (float)pad_top) / scale, 0.f, (float)orig_h);
        out[nout].x1 = x1;
        out[nout].y1 = y1;
        out[nout].x2 = x2;
        out[nout].y2 = y2;
        out[nout].score = g_merge[i].score;
        out[nout].class_id = g_merge[i].class_id;
        nout++;
    }
    return nout;
}

static bk_err_t tflm_init_interpreter(void)
{
    if (g_interpreter_initialized) {
        return BK_OK;
    }

    dwt_init_cycle_counter();
    uint32_t load_start_cycles = dwt_get_cycle_counter_val();

    if (g_ethosu0_scratch == nullptr) {
        g_ethosu0_scratch = tflm_scratch_alloc((size_t)ETHOSU_SCRATCH_SIZE + 16, &g_ethosu0_scratch_src);
        if (g_ethosu0_scratch == nullptr) {
            LOGE("alloc ethosu scratch failed\r\n");
            return BK_FAIL;
        }
    }
    void *ethosu0_scratch_aligned =
            g_ethosu0_scratch ? (void *)(((uint32_t)g_ethosu0_scratch + 15U) & ~(uint32_t)15U) : nullptr;
    LOGI("ethosu scratch -> %s %p size=%u\r\n", tflm_mem_src_name(g_ethosu0_scratch_src), ethosu0_scratch_aligned,
              (unsigned)ETHOSU_SCRATCH_SIZE);

    int init_result = bk_ethosu_init(ethosu0_scratch_aligned, (uint32_t)ETHOSU_SCRATCH_SIZE);
    if (init_result != BK_OK) {
        LOGE("bk_ethosu_init failed, ret=%d\r\n", init_result);
        if (g_ethosu0_scratch) {
            tflm_scratch_free(g_ethosu0_scratch, g_ethosu0_scratch_src);
            g_ethosu0_scratch = nullptr;
        }
        return BK_FAIL;
    }

    if (g_model_data == nullptr) {
        g_model_data_size = (size_t)hand_gesture_detection_vela_tflite_len;
        g_model_data_raw = (uint8_t *)tflm_runtime_alloc(g_model_data_size + 16, &g_model_data_src);
        if (g_model_data_raw == nullptr) {
            LOGE("alloc model failed, size=%u\r\n", (unsigned)(g_model_data_size + 16));
            return BK_FAIL;
        }
        uintptr_t aligned_addr = ((uintptr_t)g_model_data_raw + 15U) & ~(uintptr_t)15U;
        g_model_data = (uint8_t *)aligned_addr;
        os_memcpy(g_model_data, hand_gesture_detection_vela_tflite, g_model_data_size);
        LOGI("model -> %s %p size=%u\r\n", tflm_mem_src_name(g_model_data_src), g_model_data, (unsigned)g_model_data_size);
    }
    
    if (g_tensor_arena == nullptr) {
        g_tensor_arena = (uint8_t *)tflm_runtime_alloc((size_t)TFLM_ARENA_SIZE, &g_tensor_arena_src);
        if (g_tensor_arena == nullptr) {
            LOGE("alloc arena failed %u\r\n", (unsigned)TFLM_ARENA_SIZE);
            return BK_FAIL;
        }
        LOGI("arena -> %s %p size=%u\r\n", tflm_mem_src_name(g_tensor_arena_src), g_tensor_arena, (unsigned)TFLM_ARENA_SIZE);
    }

    const tflite::Model *model = tflite::GetModel(g_model_data);
    if (!model) {
        LOGE("GetModel failed\r\n");
        return BK_FAIL;
    }
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        LOGE("schema mismatch\r\n");
        return BK_FAIL;
    }

    static tflite::MicroMutableOpResolver<13> micro_op_resolver;
    (void)micro_op_resolver.AddEthosU();

    static tflite::MicroInterpreter static_interpreter(model, micro_op_resolver, g_tensor_arena, TFLM_ARENA_SIZE);
    g_interpreter = &static_interpreter;

    if (g_interpreter->AllocateTensors() != kTfLiteOk) {
        LOGE("AllocateTensors failed (try increasing TFLM_ARENA_SIZE; have %u bytes)\r\n", (unsigned)TFLM_ARENA_SIZE);
        return BK_FAIL;
    }
    LOGI("arena used %u / %u bytes\r\n", (unsigned)g_interpreter->arena_used_bytes(), (unsigned)TFLM_ARENA_SIZE);

    TfLiteTensor *in = g_interpreter->input(0);
    TfLiteTensor *out = g_interpreter->output(0);
    if (in->dims->size != 4 || in->dims->data[1] != k_input_h || in->dims->data[2] != k_input_w ||
            in->dims->data[3] != k_input_c) {
        LOGE("input shape mismatch: expect [1,%d,%d,%d]\r\n", k_input_h, k_input_w, k_input_c);
        return BK_FAIL;
    }
    if (out->dims->size != 3 || out->dims->data[1] != k_out_channels || out->dims->data[2] != k_num_candidates) {
        LOGE("output shape mismatch: expect [1,%d,%d]\r\n", k_out_channels, k_num_candidates);
        return BK_FAIL;
    }

    hgd_log_dwt_interval("model load & init (scratch+ethosu+model+arena+AllocateTensors)", load_start_cycles);
    g_interpreter_initialized = true;
    return BK_OK;
}

static bk_err_t run_one_embedded_image(const char *tag, const uint8_t *img, size_t img_len, int orig_w, int orig_h)
{
    bk_err_t ret = tflm_init_interpreter();
    if (ret != BK_OK) {
        return ret;
    }

    TfLiteTensor *input = g_interpreter->input(0);
    TfLiteTensor *output = g_interpreter->output(0);
    if (!input || !output) {
        LOGE("null tensor\r\n");
        return BK_FAIL;
    }
    if (input->type != kTfLiteInt8 || output->type != kTfLiteInt8) {
        LOGE("expected int8 in/out\r\n");
        return BK_FAIL;
    }
    if ((size_t)input->bytes != img_len) {
        LOGE("%s: input bytes %d != embedded %u\r\n", tag, (int)input->bytes, (unsigned)img_len);
        return BK_FAIL;
    }

    float scale = 0.f;
    int pad_top = 0, pad_left = 0;
    letterbox_params(orig_w, orig_h, &scale, &pad_top, &pad_left);

    memcpy(input->data.int8, img, (size_t)input->bytes);

    char invoke_lbl[48];
#if TFLM_INVOKE_TIME_TEST
    static bool s_tflm_profile_p55_inited;
    if (!s_tflm_profile_p55_inited) {
        (void)bk_gpio_enable_output(GPIO_55);
        (void)bk_gpio_set_output_low(GPIO_55);
        s_tflm_profile_p55_inited = true;
    }
#endif

    dwt_init_cycle_counter();
    uint32_t start_cycles = dwt_get_cycle_counter_val();
#if TFLM_INVOKE_TIME_TEST
    (void)bk_gpio_set_output_high(GPIO_55);
#endif
    TfLiteStatus invoke_ret = g_interpreter->Invoke();
#if TFLM_INVOKE_TIME_TEST
    (void)bk_gpio_set_output_low(GPIO_55);
#endif
    if (invoke_ret != kTfLiteOk) {
        LOGE("%s: Invoke failed\r\n", tag);
        return BK_FAIL;
    }
    os_snprintf(invoke_lbl, sizeof(invoke_lbl), "%s Invoke time", tag);
    hgd_log_dwt_interval(invoke_lbl, start_cycles);

    float out_scale = output->params.scale;
    int out_zp = output->params.zero_point;

    static HgdDet dets[32];
    int nd = postprocess_yolov8_int8(output->data.int8, out_scale, out_zp, scale, pad_top, pad_left, orig_w, orig_h, dets,
                                                                        32);

    LOGI("=== %s (%dx%d) detections=%d ===\r\n", tag, orig_w, orig_h, nd);
    for (int i = 0; i < nd; i++) {
        const char *nm = (dets[i].class_id >= 0 && dets[i].class_id < k_num_classes) ? k_class_names[dets[i].class_id] : "?";
        LOGI("  [%d] class_id=%d (%s) score=%.4f bbox_orig: x1=%.2f y1=%.2f x2=%.2f y2=%.2f\r\n", i, dets[i].class_id, nm,
                  dets[i].score, dets[i].x1, dets[i].y1, dets[i].x2, dets[i].y2);
    }
    if (nd == 0) {
        LOGI("  (none above conf=%.2f)\r\n", k_conf_threshold);
    }
    return BK_OK;
}

bk_err_t tflm_hand_gesture_detection_run_demo(void)
{
    bk_err_t r1 = run_one_embedded_image(
        "test_hands_1", test_hands_1_model_input, test_hands_1_model_input_len, k_test_hands_1_orig_w, k_test_hands_1_orig_h);
    if (r1 != BK_OK) {
        return r1;
    }
    bk_err_t r2 = run_one_embedded_image("test_hands_2", test_hands_2_model_input, test_hands_2_model_input_len,
                                        k_test_hands_2_orig_w, k_test_hands_2_orig_h);
    if (r2 != BK_OK) {
        return r2;
    }
    return BK_OK;
}
