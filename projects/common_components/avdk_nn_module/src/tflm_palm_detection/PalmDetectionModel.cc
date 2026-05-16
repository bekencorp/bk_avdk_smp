#include <math.h>
#include "os/mem.h"
#include "os/str.h"
#include "os/os.h"

#include "PalmDetectionModel.h"
#include "palm_detect_model_data.h"
#include "palm_detection_anchors.h"

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
#include <driver/aon_rtc.h>
#include <driver/aon_rtc_types.h>
#endif

static const char* TAG = "palm-model";
static const int kNumAnchors = 2944;
static const int kBoxValues = 18;
/*
 * We take max score over 2944 anchors; when no palm is in the image, one anchor
 * can still fire on background (e.g. logit 2.56 -> prob 0.93). Use threshold and
 * min box size to reduce false positives.
 */
static const float kPalmScoreProbThreshold = 0.98f;
static const float kPalmMinBoxSize = 25.0f;

#define LOGI(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE((char*)TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD((char*)TAG, ##__VA_ARGS__)
#define LOGV(...)

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
    name = "palmDetection";
    width = 256;
    height = 256;
    format = BK_PIXEL_FORMAT_RGB888;
    model_type = AVDK_NN_MODEL_TYPE_NPU;
    model_ram_type = AVDK_NN_MEM_TYPE_PSRAM_SLAB;
    model_flash_data = (uint8_t*)palm_detection_builtin_256_integer_quant_vela_tflite;
    model_flash_data_size = palm_detection_builtin_256_integer_quant_vela_tflite_size;
    model_data = (uint8_t*)palm_detection_builtin_256_integer_quant_vela_tflite;
    model_data_size = palm_detection_builtin_256_integer_quant_vela_tflite_size;

    fast_ram_type = AVDK_NN_MEM_TYPE_HSRAM;
    fast_ram_data_size = 128 * 1024;
    fast_ram_data = NULL;

    arena_data_size = 2 * 1024 * 1024;
    arena_ram_type = AVDK_NN_MEM_TYPE_PSRAM_SLAB;
    arena_ram_data = NULL;
}

void PalmDetectionModel::resourceUnload(void)
{
    //TODO: Implement resource unload
}


int PalmDetectionModel::run(uint8_t *data, uint32_t size, bk_pixel_format_t format)
{
    uint32_t expected_size = 0;

    if (format == BK_PIXEL_FORMAT_RGB888) {
        expected_size = (uint32_t)width * (uint32_t)height * 3U;
    } else if (format == BK_PIXEL_FORMAT_BGRA8888) {
        expected_size = (uint32_t)width * (uint32_t)height * 4U;
    } else {
        LOGE("Unsupported pixel format: %u\n", (unsigned)format);
        return 0;
    }

    if (size != expected_size) {
        LOGE("Invalid input size: %u, expected: %u\n", (unsigned)size, (unsigned)expected_size);
        return 0;
    }

    TfLiteTensor* input = pinterpreter->input(0);
    int input_is_float = (input->type == kTfLiteFloat32);

    /* Preprocess: align with tflite_micro_example (GetImage / GetImageFloat).
     * Support RGB (size=196608) or BGRA (size=262144); support int8 or float32 input tensor. */
    if (input_is_float) {
        if (format == BK_PIXEL_FORMAT_BGRA8888) {
            for (int i = 0; i < width * height; i++) {
                float b = (float)data[i * 4 + 0] / 255.0f;
                float g = (float)data[i * 4 + 1] / 255.0f;
                float r = (float)data[i * 4 + 2] / 255.0f;
                input->data.f[i * 3 + 0] = 2.0f * (r - 0.5f);
                input->data.f[i * 3 + 1] = 2.0f * (g - 0.5f);
                input->data.f[i * 3 + 2] = 2.0f * (b - 0.5f);
            }
        } else if (format == BK_PIXEL_FORMAT_RGB888) {
            for (int i = 0; i < width * height; i++) {
                float r = (float)data[i * 3 + 0] / 255.0f;
                float g = (float)data[i * 3 + 1] / 255.0f;
                float b = (float)data[i * 3 + 2] / 255.0f;
                input->data.f[i * 3 + 0] = 2.0f * (r - 0.5f);
                input->data.f[i * 3 + 1] = 2.0f * (g - 0.5f);
                input->data.f[i * 3 + 2] = 2.0f * (b - 0.5f);
            }
        } else {
            LOGI("Unsupported format: %d\r\n", format);
            return 0;
        }
    } else {
        if (format == BK_PIXEL_FORMAT_BGRA8888) {
            for (int i = 0; i < width * height; i++) {
                uint8_t b = data[i * 4 + 0];
                uint8_t g = data[i * 4 + 1];
                uint8_t r = data[i * 4 + 2];
                input->data.int8[i * 3 + 0] = (int8_t)(r - 128);
                input->data.int8[i * 3 + 1] = (int8_t)(g - 128);
                input->data.int8[i * 3 + 2] = (int8_t)(b - 128);
            }
        } else if (format == BK_PIXEL_FORMAT_RGB888) {
            for (int i = 0; i < width * height; i++) {
                uint8_t r = data[i * 3 + 0];
                uint8_t g = data[i * 3 + 1];
                uint8_t b = data[i * 3 + 2];
                input->data.int8[i * 3 + 0] = (int8_t)(r - 128);
                input->data.int8[i * 3 + 1] = (int8_t)(g - 128);
                input->data.int8[i * 3 + 2] = (int8_t)(b - 128);
            }
        } else {
            LOGI("Unsupported format: %d\r\n", format);
            return 0;
        }
    }

    if (kTfLiteOk != pinterpreter->Invoke()) {
        MicroPrintf("Invoke failed\r\n");
        return 0;
    }

    /* Output: score 1x2944x1 (logit), box 1x2944x18, first 4 are [dx, dy, w, h] (regression; center = anchor*256 + (dx,dy)) */
    TfLiteTensor* score_out = pinterpreter->output(0);
    TfLiteTensor* box_out = pinterpreter->output(1);

    float score_scale = score_out->params.scale;
    int32_t score_zp = score_out->params.zero_point;
    float box_scale = box_out->params.scale;
    int32_t box_zp = box_out->params.zero_point;

    LOGV("PalmDetectionModel::run score_scale=%f score_zp=%d box_scale=%f box_zp=%d\n",
         score_scale, score_zp, box_scale, box_zp);

    int best_idx = -1;
    float best_score = -1.0f;
    int score_is_float = (score_out->type == kTfLiteFloat32);
    int box_is_float = (box_out->type == kTfLiteFloat32);

    for (int i = 0; i < kNumAnchors; i++) {
        float s;
        if (score_is_float) {
            s = score_out->data.f[i];
        } else {
            s = (score_out->data.int8[i] - score_zp) * score_scale;
        }
        if (s > best_score) {
            best_score = s;
            best_idx = i;
        }
    }

    float palm_cx = 0.f, palm_cy = 0.f, palm_w = 0.f, palm_h = 0.f;
    int has_palm = 0;
    float score_prob = 1.0f / (1.0f + expf(-best_score));

    if (best_idx >= 0 && best_score > 0.f) {
        int off = best_idx * kBoxValues;
        float dx, dy;
        if (box_is_float) {
            dx = box_out->data.f[off + 0];
            dy = box_out->data.f[off + 1];
            palm_w = box_out->data.f[off + 2];
            palm_h = box_out->data.f[off + 3];
        } else {
            dx = (box_out->data.int8[off + 0] - box_zp) * box_scale;
            dy = (box_out->data.int8[off + 1] - box_zp) * box_scale;
            palm_w = (box_out->data.int8[off + 2] - box_zp) * box_scale;
            palm_h = (box_out->data.int8[off + 3] - box_zp) * box_scale;
        }
        /* Same as Python / tflite_micro_example: center = anchor * 256 + (dx, dy) */
        float anchor_cx = kPalmAnchorsCenter[best_idx * 2 + 0];
        float anchor_cy = kPalmAnchorsCenter[best_idx * 2 + 1];
        palm_cx = anchor_cx * (float)width + dx;
        palm_cy = anchor_cy * (float)height + dy;
        /* Only report palm when confident and box is not too small */
        if (score_prob >= kPalmScoreProbThreshold &&
            palm_w >= kPalmMinBoxSize && palm_h >= kPalmMinBoxSize) {
            has_palm = 1;
        }
    }

    LOGI("PalmDetectionModel: score_logit=%.4f score_prob=%.4f\n", best_score, score_prob);
    if (has_palm) {
        LOGI("PalmDetectionModel: cx=%.1f cy=%.1f w=%.1f h=%.1f\n",
             palm_cx, palm_cy, palm_w, palm_h);
        /* Invoke per-instance callback if set. */
        if (result_callback_ != nullptr)
        {
            result_callback_(has_palm, palm_cx, palm_cy, palm_w, palm_h, bk_aon_rtc_get_ms());
        }
    } else if (best_score > 0.f) {
        LOGI("PalmDetectionModel: rejected (prob<%.2f or box small), no palm\n", kPalmScoreProbThreshold);
    } else {
        LOGI("PalmDetectionModel: no palm detected\n");
            result_callback_(0, 0, 0, 0, 0, bk_aon_rtc_get_ms());
    }

    return 1;
}