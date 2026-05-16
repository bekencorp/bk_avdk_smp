/**
 * @file PalmDetectionModel.cc
 * @brief Mediapipe palm-detection (256x256, 2944 SSD anchors).
 *
 * Post-processing is a direct port of the NXP eIQ reference
 * (`etc/eiq-example-lf-6.12.49_2.2.0/gesture_detection/hand_tracker.py`)
 * cross-checked against the official mediapipe SSD anchor decoding rule
 * documented in `tflite_tensors_to_detections_calculator.cc`.
 *
 * Model contract (fixed by `palm_detection_builtin_256_integer_quant.tflite`):
 *   * input  : 256x256x3 RGB, int8 zero-centered as `(rgb - 128)`
 *              (equivalent to the Python `2 * (rgb/255 - 0.5)` after the
 *              tflite quantizer's scale=1/128, zp=0).
 *   * outputs:
 *       output(0)  "clf"  shape [1, 2944, 1]  -- raw logit per anchor
 *       output(1)  "reg"  shape [1, 2944, 18] -- per-anchor regression:
 *                            [0..3]  dx, dy, w, h        (pixels in 256-px space)
 *                            [4..17] 7 keypoints * (x,y) (pixels in 256-px space,
 *                                                         as offset from anchor center)
 *   * box decode (mediapipe SSD standard):
 *                  cx_px = anchor_cx_norm * 256 + dx
 *                  cy_px = anchor_cy_norm * 256 + dy
 *                  w_px  = w
 *                  h_px  = h
 *   * filter   : sigmoid(logit) > 0.95   (NXP Python default)
 *                + box w,h >= 25 px      (mediapipe min-box-size convention)
 *   * NMS      : added here (Python comment says "NMS not implemented" and
 *                falls back to argmax(h); we run a real IoU-NMS so multiple
 *                anchors firing on the same palm get merged, and so we can
 *                later report multi-palm scenarios cleanly).
 *
 * NOTE on Python vs SDK discrepancy:
 *   The Python computes `center = anchor * 256` (no dx/dy) because it never
 *   uses box center -- it derives orientation from the 7 keypoints and feeds
 *   those into an affine transform for the landmark model. The SDK *does*
 *   want a real box center (for OSD drawing, servo tracking, etc.), so we
 *   apply the full mediapipe formula `center = anchor*256 + (dx, dy)`.
 */

#include "PalmDetectionModel.h"
#include "palm_detect_model_data.h"
#include "palm_detection_anchors.h"
#include <math.h>
#include <components/log.h>   /* BK_LOGW / BK_LOGE / BK_LOGI / BK_LOGD */

static const char* TAG = "palm-model";
#define LOGI(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE((char*)TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD((char*)TAG, ##__VA_ARGS__)

/* ===================== Model architecture (fixed) ===================== */
constexpr int kInputSize    = 256;     /* model expects 256x256 */
constexpr int kNumAnchors   = 2944;    /* SSD anchor count, must match palm_detection_anchors.h */
constexpr int kBoxValues    = 18;      /* per-anchor regression: 4 box + 14 keypoint values */

/* ===================== Tunables ===================== */
/* Mediapipe / NXP Python default. Lower than the previous 0.98 in this file
 * (which was suppressing too many real palms in tilted / partial views). */
constexpr float kProbThreshold     = 0.95f;

/* Standard mediapipe NMS IoU. */
constexpr float kNmsIoUThreshold   = 0.50f;

/* Reject sub-25-px boxes: at this score threshold spurious boxes are usually
 * tiny artifacts on background texture. Matches the previous SDK setting. */
constexpr float kMinBoxSizePixels  = 25.0f;

/* Upper bound on candidates we store before NMS. Most frames have <5 anchors
 * passing prob>=0.95 even with multiple palms; 16 is plenty. */
constexpr int   kMaxCandidates     = 16;

/* ===================== Helpers ===================== */

/* Numerically stable sigmoid (clamps the exponent to avoid overflow on M55 FPU). */
static inline float sigmoid(float x) {
    if (x >  10.0f) return 1.0f;
    if (x < -10.0f) return 0.0f;
    return 1.0f / (1.0f + expf(-x));
}

/* Axis-aligned IoU on Box(x, y, w, h) where (x, y) is top-left. */
static float box_iou(const Box& a, const Box& b) {
    const float ax2 = a.x + a.w;
    const float ay2 = a.y + a.h;
    const float bx2 = b.x + b.w;
    const float by2 = b.y + b.h;

    const float ix1 = (a.x > b.x) ? a.x : b.x;
    const float iy1 = (a.y > b.y) ? a.y : b.y;
    const float ix2 = (ax2 < bx2) ? ax2 : bx2;
    const float iy2 = (ay2 < by2) ? ay2 : by2;

    const float iw = ix2 - ix1; if (iw <= 0.0f) return 0.0f;
    const float ih = iy2 - iy1; if (ih <= 0.0f) return 0.0f;

    const float inter = iw * ih;
    const float uni   = a.w * a.h + b.w * b.h - inter;
    return (uni > 0.0f) ? (inter / uni) : 0.0f;
}

/* In-place: sort by score desc (insertion sort), then drop overlapping boxes. */
static int nms_in_place(Box* boxes, int count, float iou_thresh) {
    for (int i = 1; i < count; i++) {
        Box key = boxes[i];
        int j = i - 1;
        while (j >= 0 && boxes[j].score < key.score) {
            boxes[j + 1] = boxes[j];
            j--;
        }
        boxes[j + 1] = key;
    }

    bool keep[kMaxCandidates];
    for (int i = 0; i < count; i++) keep[i] = true;
    for (int i = 0; i < count; i++) {
        if (!keep[i]) continue;
        for (int j = i + 1; j < count; j++) {
            if (!keep[j]) continue;
            if (box_iou(boxes[i], boxes[j]) > iou_thresh) keep[j] = false;
        }
    }

    int n = 0;
    for (int i = 0; i < count; i++) {
        if (keep[i]) {
            if (n != i) boxes[n] = boxes[i];
            n++;
        }
    }
    return n;
}

/* ===================== AvdkDetectionModel overrides ===================== */

void PalmDetectionModel::resolverLoad(void)
{
    micro_op_resolver.AddEthosU();
    micro_op_resolver.AddPadV2();
    micro_op_resolver.AddTranspose();
    micro_op_resolver.AddQuantize();
    micro_op_resolver.AddDequantize();
}

void PalmDetectionModel::resourceLoad(void)
{
    name   = "palmDetection";
    width  = kInputSize;
    height = kInputSize;
    format = BK_PIXEL_FORMAT_RGB888;

    model_type             = AVDK_NN_MODEL_TYPE_NPU;
    model_ram_type         = AVDK_NN_MEM_TYPE_PSRAM_SLAB;
    model_flash_data       = (uint8_t*)palm_detection_builtin_256_integer_quant_vela_tflite;
    model_flash_data_size  = palm_detection_builtin_256_integer_quant_vela_tflite_size;
    model_data             = (uint8_t*)palm_detection_builtin_256_integer_quant_vela_tflite;
    model_data_size        = palm_detection_builtin_256_integer_quant_vela_tflite_size;

    fast_ram_type          = AVDK_NN_MEM_TYPE_HSRAM;
    fast_ram_data_size     = 128 * 1024;
    fast_ram_data          = NULL;

    arena_data_size        = 2 * 1024 * 1024;
    arena_ram_type         = AVDK_NN_MEM_TYPE_PSRAM_SLAB;
    arena_ram_data         = NULL;
}

void PalmDetectionModel::resourceUnload(void)
{
    /* No dynamic resources owned beyond what AvdkDetectionModel manages. */
}

/* ===================== run() ===================== */

int PalmDetectionModel::run(uint8_t *data, uint32_t size, bk_pixel_format_t format)
{
    /* ---- 1) input sanity ---- */
    if (width != kInputSize || height != kInputSize) {
        LOGE("PalmDetection: model expects %dx%d, but width=%u height=%u\n",
             kInputSize, kInputSize, (unsigned)width, (unsigned)height);
        return 0;
    }

    uint32_t expected_size = 0;
    if (format == BK_PIXEL_FORMAT_RGB888) {
        expected_size = (uint32_t)width * (uint32_t)height * 3U;
    } else if (format == BK_PIXEL_FORMAT_BGRA8888) {
        expected_size = (uint32_t)width * (uint32_t)height * 4U;
    } else {
        LOGE("PalmDetection: unsupported pixel format %u\n", (unsigned)format);
        return 0;
    }
    if (size != expected_size) {
        LOGE("PalmDetection: invalid size %u, expected %u\n",
             (unsigned)size, (unsigned)expected_size);
        return 0;
    }

    /* ---- 2) preprocess: zero-centered RGB into input tensor ----
     * Equivalent to Python `2 * (rgb/255 - 0.5)` after tflite's quantizer
     * (scale=1/128, zp=0). The vela-compiled model has int8 input. We keep
     * the float32 path as a fallback in case the model is later re-exported
     * without quantization. */
    TfLiteTensor* input = pinterpreter->input(0);
    const bool input_is_float = (input->type == kTfLiteFloat32);

    if (input_is_float) {
        if (format == BK_PIXEL_FORMAT_BGRA8888) {
            for (int i = 0; i < width * height; i++) {
                /* BGRA byte order: +0=B, +1=G, +2=R, +3=A. */
                const float b = (float)data[i * 4 + 0] / 255.0f;
                const float g = (float)data[i * 4 + 1] / 255.0f;
                const float r = (float)data[i * 4 + 2] / 255.0f;
                input->data.f[i * 3 + 0] = 2.0f * (r - 0.5f);
                input->data.f[i * 3 + 1] = 2.0f * (g - 0.5f);
                input->data.f[i * 3 + 2] = 2.0f * (b - 0.5f);
            }
        } else { /* BK_PIXEL_FORMAT_RGB888 */
            for (int i = 0; i < width * height; i++) {
                const float r = (float)data[i * 3 + 0] / 255.0f;
                const float g = (float)data[i * 3 + 1] / 255.0f;
                const float b = (float)data[i * 3 + 2] / 255.0f;
                input->data.f[i * 3 + 0] = 2.0f * (r - 0.5f);
                input->data.f[i * 3 + 1] = 2.0f * (g - 0.5f);
                input->data.f[i * 3 + 2] = 2.0f * (b - 0.5f);
            }
        }
    } else {
        if (format == BK_PIXEL_FORMAT_BGRA8888) {
            for (int i = 0; i < width * height; i++) {
                input->data.int8[i * 3 + 0] = (int8_t)((int)data[i * 4 + 2] - 128); /* R */
                input->data.int8[i * 3 + 1] = (int8_t)((int)data[i * 4 + 1] - 128); /* G */
                input->data.int8[i * 3 + 2] = (int8_t)((int)data[i * 4 + 0] - 128); /* B */
            }
        } else { /* BK_PIXEL_FORMAT_RGB888 */
            for (int i = 0; i < width * height; i++) {
                input->data.int8[i * 3 + 0] = (int8_t)((int)data[i * 3 + 0] - 128);
                input->data.int8[i * 3 + 1] = (int8_t)((int)data[i * 3 + 1] - 128);
                input->data.int8[i * 3 + 2] = (int8_t)((int)data[i * 3 + 2] - 128);
            }
        }
    }

    /* ---- 3) invoke ---- */
    if (kTfLiteOk != pinterpreter->Invoke()) {
        LOGE("PalmDetection: invoke failed\n");
        return 0;
    }

    /* ---- 4) grab output tensors ----
     * Per NXP Python:
     *   self.out_clf_idx = output_details[0]['index']   # score / classification
     *   self.out_reg_idx = output_details[1]['index']   # box  / regression
     */
    TfLiteTensor* clf = pinterpreter->output(0);
    TfLiteTensor* reg = pinterpreter->output(1);

    const float   clf_scale = clf->params.scale;
    const int32_t clf_zp    = clf->params.zero_point;
    const float   reg_scale = reg->params.scale;
    const int32_t reg_zp    = reg->params.zero_point;

    const bool clf_is_float = (clf->type == kTfLiteFloat32);
    const bool reg_is_float = (reg->type == kTfLiteFloat32);

    /* First-call sanity log: confirm tensor shapes and dequant params match
     * the architecture this code is hard-coded against. */
    static bool s_shape_logged = false;
    if (!s_shape_logged) {
        LOGI("PalmDetection: clf dims=%d shape=[", clf->dims->size);
        for (int i = 0; i < clf->dims->size; i++) {
            BK_LOGW((char*)TAG, "%d%s", clf->dims->data[i],
                    (i == clf->dims->size - 1) ? "" : ", ");
        }
        LOGI("] scale=%.6f zp=%ld%s\n", clf_scale, (long)clf_zp,
             clf_is_float ? " (float32)" : " (int8)");
        LOGI("PalmDetection: reg dims=%d shape=[", reg->dims->size);
        for (int i = 0; i < reg->dims->size; i++) {
            BK_LOGW((char*)TAG, "%d%s", reg->dims->data[i],
                    (i == reg->dims->size - 1) ? "" : ", ");
        }
        LOGI("] scale=%.6f zp=%ld%s\n", reg_scale, (long)reg_zp,
             reg_is_float ? " (float32)" : " (int8)");
        LOGI("PalmDetection: expecting clf=[1,%d,1] reg=[1,%d,%d]\n",
             kNumAnchors, kNumAnchors, kBoxValues);
        s_shape_logged = true;
    }

    /* ---- 5) decode all anchors with early threshold gate ---- */
    Box candidates[kMaxCandidates];
    int n_cand = 0;
    int n_pass = 0;          /* anchors that beat the prob threshold (incl. cap-dropped) */
    float best_prob_below_thresh = 0.0f;   /* for diagnostics when nothing passes */

    for (int i = 0; i < kNumAnchors; i++) {
        /* Dequant logit + sigmoid, gated early so we don't pay for the box
         * dequant on the >99% of anchors that fail the threshold. */
        float logit;
        if (clf_is_float) {
            logit = clf->data.f[i];
        } else {
            logit = ((float)clf->data.int8[i] - (float)clf_zp) * clf_scale;
        }
        const float prob = sigmoid(logit);
        if (prob < kProbThreshold) {
            if (prob > best_prob_below_thresh) best_prob_below_thresh = prob;
            continue;
        }
        n_pass++;
        if (n_cand >= kMaxCandidates) continue;   /* keep counting for log */

        /* Dequant the 4 box values for this anchor. */
        const int off = i * kBoxValues;
        float dx, dy, bw, bh;
        if (reg_is_float) {
            dx = reg->data.f[off + 0];
            dy = reg->data.f[off + 1];
            bw = reg->data.f[off + 2];
            bh = reg->data.f[off + 3];
        } else {
            dx = ((float)reg->data.int8[off + 0] - (float)reg_zp) * reg_scale;
            dy = ((float)reg->data.int8[off + 1] - (float)reg_zp) * reg_scale;
            bw = ((float)reg->data.int8[off + 2] - (float)reg_zp) * reg_scale;
            bh = ((float)reg->data.int8[off + 3] - (float)reg_zp) * reg_scale;
        }

        /* Mediapipe SSD decode: dx/dy already in 256-px units, anchor in [0,1]. */
        const float anchor_cx = kPalmAnchorsCenter[i * 2 + 0];
        const float anchor_cy = kPalmAnchorsCenter[i * 2 + 1];
        const float cx = anchor_cx * (float)kInputSize + dx;
        const float cy = anchor_cy * (float)kInputSize + dy;

        /* Reject too-small boxes (mediapipe convention; spurious anchors at
         * low confidence often regress to a few-px box on textured backgrounds). */
        if (bw < kMinBoxSizePixels || bh < kMinBoxSizePixels) continue;

        /* Center+size -> top-left+size, then clamp to input frame. */
        float x1 = cx - bw * 0.5f;
        float y1 = cy - bh * 0.5f;
        float x2 = cx + bw * 0.5f;
        float y2 = cy + bh * 0.5f;
        if (x1 < 0.0f) x1 = 0.0f;
        if (y1 < 0.0f) y1 = 0.0f;
        if (x2 > (float)kInputSize) x2 = (float)kInputSize;
        if (y2 > (float)kInputSize) y2 = (float)kInputSize;

        const float cw = x2 - x1;
        const float ch = y2 - y1;
        if (cw <= 0.0f || ch <= 0.0f) continue;

        candidates[n_cand].x     = x1;
        candidates[n_cand].y     = y1;
        candidates[n_cand].w     = cw;
        candidates[n_cand].h     = ch;
        candidates[n_cand].score = prob;
        n_cand++;
    }

    if (n_cand == 0) {
        LOGI("PalmDetection: no palm (best_prob=%.3f, threshold=%.2f)\n",
             best_prob_below_thresh, kProbThreshold);
        /* Skip the callback (do NOT pass NULL/0). Existing consumers in
         * palm_recognition/ap/ap_main.cc dereference boxes[0] without
         * checking count, so an empty event would crash them. The OSD layer
         * holds the previous box until the next successful detection
         * overwrites it via box_detection_path_build(). */
        onBoxDetectionCallback(NULL, 0);
        return 0;
    }

    /* ---- 6) NMS ---- */
    const int n_kept = nms_in_place(candidates, n_cand, kNmsIoUThreshold);

    /* ---- 7) report ---- */
    LOGI("PalmDetection: cand=%d (pass=%d, cap=%d) -> nms_kept=%d, top: xywh=(%.1f,%.1f,%.1fx%.1f) prob=%.3f\n",
         n_cand, n_pass, kMaxCandidates, n_kept,
         candidates[0].x, candidates[0].y, candidates[0].w, candidates[0].h, candidates[0].score);

    onBoxDetectionCallback(candidates, n_kept);
    return n_kept;
}
