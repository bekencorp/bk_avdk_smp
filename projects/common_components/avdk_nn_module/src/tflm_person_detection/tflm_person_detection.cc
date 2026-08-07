#include "tflm_person_detect.h"

#include <algorithm>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <components/bk_frame_buffer.h>
#include <driver/aon_rtc.h>

#include "box.h"
#include "os/mem.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_log.h"

#if defined(__ARM_FEATURE_MVE)
#include <arm_mve.h>
#endif

static constexpr int kPersonInputH = 180;
static constexpr int kPersonInputW = 320;
#if CONFIG_TFLM_PERSON_DETECTION_RGB
static constexpr int kPersonInputC = 3;
static constexpr bk_pixel_format_t kPersonModelFormat = BK_PIXEL_FORMAT_RGB888;
#elif CONFIG_TFLM_PERSON_DETECTION_GRAY
static constexpr int kPersonInputC = 1;
static constexpr bk_pixel_format_t kPersonModelFormat = BK_PIXEL_FORMAT_NV12;
#else
#error "CONFIG_TFLM_PERSON_DETECTION_RGB or CONFIG_TFLM_PERSON_DETECTION_GRAY must be enabled"
#endif
static constexpr int kPersonInputBytes = kPersonInputH * kPersonInputW * kPersonInputC;
static constexpr int kPersonFeatureLevelCount = 4;
static constexpr int kPersonAnchorsPerCell = 4;
static constexpr int kPersonAnchorCount = 4660;
static constexpr int kPersonMaxCandidates = 200;
static constexpr int kPersonMaxDetections = 32;
static constexpr int kPersonArenaSize = 700 * 1024;
static constexpr int kPersonScratchSize = 128 * 1024;
static constexpr float kPersonScoreThreshold = 0.55f;
static constexpr float kPersonNmsThreshold = 0.5f;
static constexpr float kPersonRegressionStd = 0.2f;

static const int kPersonFeatureShapes[kPersonFeatureLevelCount][2] = {
    {22, 40},
    {11, 20},
    {5, 10},
    {3, 5},
};
static const int kPersonAnchorSizes[kPersonFeatureLevelCount] = {32, 64, 128, 256};
static const int kPersonAnchorStrides[kPersonFeatureLevelCount] = {8, 16, 32, 64};
static const float kPersonAnchorRatios[2] = {1.0f, 2.0f};
static const float kPersonAnchorScales[2] = {1.0f, 1.41421356f};

typedef struct {
    float x1;
    float y1;
    float x2;
    float y2;
} PersonAnchor;

typedef struct {
    Box box;
    int label;
    int order;
} PersonCandidate;

static PersonAnchor s_person_anchors[kPersonAnchorCount];
static bool s_person_anchors_ready = false;
static TfLiteTensor *s_person_regression = nullptr;
static TfLiteTensor *s_person_classification = nullptr;
static PersonCandidate s_person_candidates[kPersonMaxCandidates];
static Box s_person_detections[kPersonMaxDetections];
static int s_person_detection_labels[kPersonMaxDetections];

static void person_detection_log_interval(const char *prefix, unsigned long long start_us)
{
    const unsigned long long elapsed_us = bk_aon_rtc_get_us() - start_us;
    MicroPrintf("%s: %llu us\r\n", prefix, elapsed_us);
}

static float person_detection_tensor_value(const TfLiteTensor *tensor, int flat_index)
{
    if (tensor == nullptr) {
        return 0.0f;
    }

    if (tensor->type == kTfLiteInt8) {
        return ((float)tensor->data.int8[flat_index] - (float)tensor->params.zero_point) * tensor->params.scale;
    }
    if (tensor->type == kTfLiteUInt8) {
        return ((float)tensor->data.uint8[flat_index] - (float)tensor->params.zero_point) * tensor->params.scale;
    }
    if (tensor->type == kTfLiteFloat32) {
        return tensor->data.f[flat_index];
    }

    return 0.0f;
}

static int person_detection_tensor_element_count(const TfLiteTensor *tensor)
{
    if (tensor == nullptr || tensor->dims == nullptr || tensor->dims->size <= 0) {
        return 0;
    }

    int count = 1;
    for (int i = 0; i < tensor->dims->size; i++) {
        count *= tensor->dims->data[i];
    }
    return count;
}

static int person_detection_tensor_last_dim(const TfLiteTensor *tensor)
{
    if (tensor == nullptr || tensor->dims == nullptr || tensor->dims->size <= 0) {
        return 0;
    }
    return tensor->dims->data[tensor->dims->size - 1];
}

static int person_detection_tensor_anchor_dim(const TfLiteTensor *tensor)
{
    if (tensor == nullptr || tensor->dims == nullptr || tensor->dims->size < 2) {
        return 0;
    }
    return tensor->dims->data[tensor->dims->size - 2];
}

static int8_t person_detection_quantize_int8(float value, const TfLiteTensor *tensor)
{
    int quantized = (int)lrintf(value / tensor->params.scale + (float)tensor->params.zero_point);
    if (quantized < -128) {
        quantized = -128;
    } else if (quantized > 127) {
        quantized = 127;
    }
    return (int8_t)quantized;
}

static uint8_t person_detection_quantize_uint8(float value, const TfLiteTensor *tensor)
{
    int quantized = (int)lrintf(value / tensor->params.scale + (float)tensor->params.zero_point);
    if (quantized < 0) {
        quantized = 0;
    } else if (quantized > 255) {
        quantized = 255;
    }
    return (uint8_t)quantized;
}

static bool person_detection_y_to_int8_mve(const uint8_t *src, int8_t *dst, uint32_t pixels)
{
    uint32_t i = 0;

#if defined(__ARM_FEATURE_MVE)
    for (; i + 16U <= pixels; i += 16U) {
        uint8x16_t y = vld1q_u8(src + i);
        y = vsubq_n_u8(y, 128);
        vst1q_u8((uint8_t *)dst + i, y);
    }
#endif

    for (; i < pixels; i++) {
        dst[i] = (int8_t)((int)src[i] - 128);
    }

    return true;
}

static bool person_detection_bgrx_to_rgb_int8_mve(const uint8_t *src, int8_t *dst,
                                                  uint32_t width, uint32_t height)
{
    const uint32_t pixels = width * height;
    uint32_t i = 0;

#if defined(__ARM_FEATURE_MVE)
    for (; i + 16U <= pixels; i += 16U) {
        static const uint8_t k_mve_rgb_offsets[16] = {
            0, 3, 6, 9, 12, 15, 18, 21,
            24, 27, 30, 33, 36, 39, 42, 45,
        };
        const uint8x16x4_t bgrx = vld4q_u8(src + i * 4U);
        const uint8x16_t offset = vld1q_u8(k_mve_rgb_offsets);
        uint8_t *d = (uint8_t *)dst + i * 3U;

        vstrbq_scatter_offset_u8(d, offset, vsubq_n_u8(bgrx.val[2], 128));
        vstrbq_scatter_offset_u8(d + 1, offset, vsubq_n_u8(bgrx.val[1], 128));
        vstrbq_scatter_offset_u8(d + 2, offset, vsubq_n_u8(bgrx.val[0], 128));
    }
#endif

    for (; i < pixels; i++) {
        const uint8_t *s = src + i * 4U;
        int8_t *d = dst + i * 3U;

        d[0] = (int8_t)((int)s[2] - 128);
        d[1] = (int8_t)((int)s[1] - 128);
        d[2] = (int8_t)((int)s[0] - 128);
    }

    return true;
}

static bool person_detection_rgb_to_rgb_int8_mve(const uint8_t *src, int8_t *dst,
                                                 uint32_t width, uint32_t height)
{
    const uint32_t bytes = width * height * 3U;
    uint32_t i = 0;

#if defined(__ARM_FEATURE_MVE)
    for (; i + 16U <= bytes; i += 16U) {
        uint8x16_t rgb = vld1q_u8(src + i);
        rgb = vsubq_n_u8(rgb, 128);
        vst1q_u8((uint8_t *)dst + i, rgb);
    }
#endif

    for (; i < bytes; i++) {
        dst[i] = (int8_t)((int)src[i] - 128);
    }

    return true;
}

static void person_detection_build_anchors(void)
{
    if (s_person_anchors_ready) {
        return;
    }

    int out = 0;
    for (int level = 0; level < kPersonFeatureLevelCount; level++) {
        PersonAnchor base[kPersonAnchorsPerCell];
        int base_count = 0;
        for (int ratio_i = 0; ratio_i < 2; ratio_i++) {
            for (int scale_i = 0; scale_i < 2; scale_i++) {
                const float area_side = (float)kPersonAnchorSizes[level] * kPersonAnchorScales[scale_i];
                const float area = area_side * area_side;
                const float w = sqrtf(area / kPersonAnchorRatios[ratio_i]);
                const float h = w * kPersonAnchorRatios[ratio_i];
                base[base_count].x1 = -0.5f * w;
                base[base_count].y1 = -0.5f * h;
                base[base_count].x2 = 0.5f * w;
                base[base_count].y2 = 0.5f * h;
                base_count++;
            }
        }

        const int rows = kPersonFeatureShapes[level][0];
        const int cols = kPersonFeatureShapes[level][1];
        const int stride = kPersonAnchorStrides[level];
        for (int y = 0; y < rows; y++) {
            const float shift_y = ((float)y + 0.5f) * (float)stride;
            for (int x = 0; x < cols; x++) {
                const float shift_x = ((float)x + 0.5f) * (float)stride;
                for (int anchor_i = 0; anchor_i < base_count; anchor_i++) {
                    if (out >= kPersonAnchorCount) {
                        s_person_anchors_ready = true;
                        return;
                    }
                    s_person_anchors[out].x1 = base[anchor_i].x1 + shift_x;
                    s_person_anchors[out].y1 = base[anchor_i].y1 + shift_y;
                    s_person_anchors[out].x2 = base[anchor_i].x2 + shift_x;
                    s_person_anchors[out].y2 = base[anchor_i].y2 + shift_y;
                    out++;
                }
            }
        }
    }

    s_person_anchors_ready = (out == kPersonAnchorCount);
    if (!s_person_anchors_ready) {
        MicroPrintf("PersonDetection anchor count mismatch: got %d expect %d\r\n", out, kPersonAnchorCount);
    }
}

static void person_detection_decode_box(int anchor_index, Box *box)
{
    const PersonAnchor *anchor = &s_person_anchors[anchor_index];
    const float width = anchor->x2 - anchor->x1;
    const float height = anchor->y2 - anchor->y1;

    float x1 = anchor->x1 + person_detection_tensor_value(s_person_regression, anchor_index * 4 + 0) *
                              kPersonRegressionStd * width;
    float y1 = anchor->y1 + person_detection_tensor_value(s_person_regression, anchor_index * 4 + 1) *
                              kPersonRegressionStd * height;
    float x2 = anchor->x2 + person_detection_tensor_value(s_person_regression, anchor_index * 4 + 2) *
                              kPersonRegressionStd * width;
    float y2 = anchor->y2 + person_detection_tensor_value(s_person_regression, anchor_index * 4 + 3) *
                              kPersonRegressionStd * height;

    if (x1 < 0.0f) {
        x1 = 0.0f;
    }
    if (y1 < 0.0f) {
        y1 = 0.0f;
    }
    if (x2 > (float)(kPersonInputW - 1)) {
        x2 = (float)(kPersonInputW - 1);
    }
    if (y2 > (float)(kPersonInputH - 1)) {
        y2 = (float)(kPersonInputH - 1);
    }

    box->x = x1;
    box->y = y1;
    box->w = x2 - x1;
    box->h = y2 - y1;
}

static float person_detection_iou(const Box *a, const Box *b)
{
    const float ax2 = a->x + a->w;
    const float ay2 = a->y + a->h;
    const float bx2 = b->x + b->w;
    const float by2 = b->y + b->h;
    const float ix1 = fmaxf(a->x, b->x);
    const float iy1 = fmaxf(a->y, b->y);
    const float ix2 = fminf(ax2, bx2);
    const float iy2 = fminf(ay2, by2);
    const float iw = fmaxf(0.0f, ix2 - ix1);
    const float ih = fmaxf(0.0f, iy2 - iy1);
    const float inter = iw * ih;
    const float area_a = fmaxf(0.0f, a->w) * fmaxf(0.0f, a->h);
    const float area_b = fmaxf(0.0f, b->w) * fmaxf(0.0f, b->h);
    const float denom = area_a + area_b - inter;
    return denom > 1e-6f ? inter / denom : 0.0f;
}

static void person_detection_add_candidate(Box *box, int label, int order, int *candidate_count)
{
    if (box->w <= 0.0f || box->h <= 0.0f) {
        return;
    }

    if (*candidate_count < kPersonMaxCandidates) {
        s_person_candidates[*candidate_count].box = *box;
        s_person_candidates[*candidate_count].label = label;
        s_person_candidates[*candidate_count].order = order;
        (*candidate_count)++;
        return;
    }

    int min_index = 0;
    for (int i = 1; i < kPersonMaxCandidates; i++) {
        if (s_person_candidates[i].box.score < s_person_candidates[min_index].box.score) {
            min_index = i;
        }
    }
    if (box->score > s_person_candidates[min_index].box.score) {
        s_person_candidates[min_index].box = *box;
        s_person_candidates[min_index].label = label;
        s_person_candidates[min_index].order = order;
    }
}

static int person_detection_decode_outputs(void)
{
    int candidate_count = 0;
    int order = 0;
    const int class_count = person_detection_tensor_last_dim(s_person_classification);

    for (int anchor_index = 0; anchor_index < kPersonAnchorCount; anchor_index++) {
        for (int label = 0; label < class_count; label++) {
            const int score_index = anchor_index * class_count + label;
            const float score = person_detection_tensor_value(s_person_classification, score_index);
            if (score <= kPersonScoreThreshold) {
                continue;
            }

            Box box = {};
            person_detection_decode_box(anchor_index, &box);
            box.score = score;
            person_detection_add_candidate(&box, label, order, &candidate_count);
        }
        order++;
    }

    std::sort(s_person_candidates, s_person_candidates + candidate_count,
              [](const PersonCandidate &a, const PersonCandidate &b) {
                  if (a.box.score > b.box.score) {
                      return true;
                  }
                  if (a.box.score < b.box.score) {
                      return false;
                  }
                  return a.order < b.order;
              });

    int kept = 0;
    for (int i = 0; i < candidate_count && kept < kPersonMaxDetections; i++) {
        bool keep = true;
        for (int j = 0; j < kept; j++) {
            if (s_person_candidates[i].label == s_person_detection_labels[j] &&
                person_detection_iou(&s_person_candidates[i].box, &s_person_detections[j]) > kPersonNmsThreshold) {
                keep = false;
                break;
            }
        }
        if (keep) {
            s_person_detections[kept++] = s_person_candidates[i].box;
            s_person_detection_labels[kept - 1] = s_person_candidates[i].label;
        }
    }

    MicroPrintf("PersonDetection candidates=%d kept=%d\r\n", candidate_count, kept);
    return kept;
}

static bool person_detection_prepare_input(const uint8_t *data, uint32_t size, bk_pixel_format_t format,
                                           TfLiteTensor *input)
{
    if (data == nullptr || input == nullptr) {
        return false;
    }

#if CONFIG_TFLM_PERSON_DETECTION_RGB
    const uint32_t rgb888_size = (uint32_t)kPersonInputBytes;
    const uint32_t bgra8888_size = (uint32_t)kPersonInputW * (uint32_t)kPersonInputH * 4U;
    if (format != BK_PIXEL_FORMAT_RGB888 && format != BK_PIXEL_FORMAT_BGRA8888) {
        MicroPrintf("PersonDetection unsupported RGB source format=%u\r\n", (unsigned)format);
        return false;
    }
    if ((format == BK_PIXEL_FORMAT_RGB888 && size != rgb888_size) ||
        (format == BK_PIXEL_FORMAT_BGRA8888 && size != bgra8888_size)) {
        MicroPrintf("PersonDetection frame size=%u mismatch for format=%u\r\n",
                    (unsigned)size, (unsigned)format);
        return false;
    }

    if (input->type == kTfLiteInt8) {
        if (format == BK_PIXEL_FORMAT_BGRA8888) {
            return person_detection_bgrx_to_rgb_int8_mve(data, input->data.int8,
                                                         kPersonInputW, kPersonInputH);
        }
        return person_detection_rgb_to_rgb_int8_mve(data, input->data.int8,
                                                    kPersonInputW, kPersonInputH);
    }

    for (int i = 0; i < kPersonInputW * kPersonInputH; i++) {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        if (format == BK_PIXEL_FORMAT_BGRA8888) {
            b = data[i * 4 + 0];
            g = data[i * 4 + 1];
            r = data[i * 4 + 2];
        } else {
            r = data[i * 3 + 0];
            g = data[i * 3 + 1];
            b = data[i * 3 + 2];
        }

        const float rf = (float)r / 127.5f - 1.0f;
        const float gf = (float)g / 127.5f - 1.0f;
        const float bf = (float)b / 127.5f - 1.0f;

        if (input->type == kTfLiteUInt8) {
            input->data.uint8[i * 3 + 0] = person_detection_quantize_uint8(rf, input);
            input->data.uint8[i * 3 + 1] = person_detection_quantize_uint8(gf, input);
            input->data.uint8[i * 3 + 2] = person_detection_quantize_uint8(bf, input);
        } else if (input->type == kTfLiteFloat32) {
            input->data.f[i * 3 + 0] = rf;
            input->data.f[i * 3 + 1] = gf;
            input->data.f[i * 3 + 2] = bf;
        } else {
            MicroPrintf("PersonDetection unsupported input type=%d\r\n", input->type);
            return false;
        }
    }
#elif CONFIG_TFLM_PERSON_DETECTION_GRAY
    const uint32_t y_size = (uint32_t)kPersonInputW * (uint32_t)kPersonInputH;
    if (format != BK_PIXEL_FORMAT_NV12) {
        MicroPrintf("PersonDetection unsupported gray source format=%u\r\n", (unsigned)format);
        return false;
    }
    if (size < y_size) {
        MicroPrintf("PersonDetection NV12 frame size=%u smaller than Y plane=%u\r\n",
                    (unsigned)size, (unsigned)y_size);
        return false;
    }

    if (input->type == kTfLiteInt8) {
        return person_detection_y_to_int8_mve(data, input->data.int8, y_size);
    }

    for (uint32_t i = 0; i < y_size; i++) {
        const float yf = (float)data[i] / 127.5f - 1.0f;

        if (input->type == kTfLiteUInt8) {
            input->data.uint8[i] = person_detection_quantize_uint8(yf, input);
        } else if (input->type == kTfLiteFloat32) {
            input->data.f[i] = yf;
        } else {
            MicroPrintf("PersonDetection unsupported input type=%d\r\n", input->type);
            return false;
        }
    }
#else
#error "CONFIG_TFLM_PERSON_DETECTION_RGB or CONFIG_TFLM_PERSON_DETECTION_GRAY must be enabled"
#endif

    return true;
}

PersonDetectModel::PersonDetectModel()
    : output_tensors_ready(false)
{
}

void PersonDetectModel::resolverLoad(void)
{
    if (micro_op_resolver.AddEthosU() != kTfLiteOk) {
        MicroPrintf("PersonDetection AddEthosU failed\r\n");
    }
    if (micro_op_resolver.AddResizeNearestNeighbor() != kTfLiteOk) {
        MicroPrintf("PersonDetection AddResizeNearestNeighbor failed\r\n");
    }
    MicroPrintf("PersonDetection resolverLoad\r\n");
}

void PersonDetectModel::resourceLoad(void)
{
    name = "PersonDetectionModel";
    width = kPersonInputW;
    height = kPersonInputH;
    format = kPersonModelFormat;

    model_type = AVDK_NN_MODEL_TYPE_NPU;
    model_ram_type = AVDK_NN_MEM_TYPE_PSRAM_SLAB;
    model_flash_data = (uint8_t *)person_detection_vela_tflite;
    model_flash_data_size = person_detection_vela_tflite_len;
    model_data = (uint8_t *)person_detection_vela_tflite;
    model_data_size = person_detection_vela_tflite_len;

    fast_ram_type = AVDK_NN_MEM_TYPE_HSRAM;
    fast_ram_data_size = kPersonScratchSize;
    fast_ram_data = NULL;

    arena_data_size = kPersonArenaSize;
    arena_ram_type = AVDK_NN_MEM_TYPE_PSRAM_SLAB;
    arena_ram_data = NULL;

    MicroPrintf("PersonDetection resourceLoad model=%u arena=%u scratch=%u format=%u\r\n",
                (unsigned)model_flash_data_size,
                (unsigned)arena_data_size,
                (unsigned)fast_ram_data_size,
                (unsigned)format);
}

void PersonDetectModel::resourceUnload(void)
{
    output_tensors_ready = false;
    s_person_regression = nullptr;
    s_person_classification = nullptr;

    if (pinterpreter != nullptr) {
        delete pinterpreter;
        pinterpreter = nullptr;
    }
    if (arena_ram_data != nullptr) {
        freeMemory(arena_ram_type, arena_ram_data);
        arena_ram_data = nullptr;
    }
}

bool PersonDetectModel::buildOutputTensors(void)
{
    if (pinterpreter == nullptr || pinterpreter->outputs_size() != 2) {
        MicroPrintf("PersonDetection output count mismatch\r\n");
        return false;
    }

    TfLiteTensor *out0 = pinterpreter->output(0);
    TfLiteTensor *out1 = pinterpreter->output(1);
    TfLiteTensor *regression = nullptr;
    TfLiteTensor *classification = nullptr;

    if (person_detection_tensor_last_dim(out0) == 4) {
        regression = out0;
        classification = out1;
    } else if (person_detection_tensor_last_dim(out1) == 4) {
        regression = out1;
        classification = out0;
    } else {
        MicroPrintf("PersonDetection cannot identify regression output\r\n");
        return false;
    }

    if (person_detection_tensor_anchor_dim(regression) != kPersonAnchorCount ||
        person_detection_tensor_anchor_dim(classification) != kPersonAnchorCount ||
        person_detection_tensor_last_dim(classification) <= 0) {
        MicroPrintf("PersonDetection output shape mismatch reg=[%d,%d] cls=[%d,%d]\r\n",
                    person_detection_tensor_anchor_dim(regression),
                    person_detection_tensor_last_dim(regression),
                    person_detection_tensor_anchor_dim(classification),
                    person_detection_tensor_last_dim(classification));
        return false;
    }

    s_person_regression = regression;
    s_person_classification = classification;
    output_tensors_ready = true;

    MicroPrintf("PersonDetection outputs anchors=%d classes=%d reg_type=%d cls_type=%d\r\n",
                person_detection_tensor_anchor_dim(regression),
                person_detection_tensor_last_dim(classification),
                regression->type,
                classification->type);
    return true;
}

int PersonDetectModel::run(uint8_t *data, uint32_t size, bk_pixel_format_t format)
{
    if (pinterpreter == nullptr) {
        return 0;
    }

    TfLiteTensor *input = pinterpreter->input(0);
    if (input == nullptr ||
        input->dims == nullptr ||
        input->dims->size != 4 ||
        input->dims->data[1] != kPersonInputH ||
        input->dims->data[2] != kPersonInputW ||
        input->dims->data[3] != kPersonInputC ||
        person_detection_tensor_element_count(input) != kPersonInputBytes) {
        MicroPrintf("PersonDetection input shape mismatch\r\n");
        return 0;
    }

    unsigned long long start_us = bk_aon_rtc_get_us();
    if (!person_detection_prepare_input(data, size, format, input)) {
        onBoxDetectionCallback(NULL, 0);
        return 0;
    }
    person_detection_log_interval("PersonDetection input prepare", start_us);

    start_us = bk_aon_rtc_get_us();
    if (pinterpreter->Invoke() != kTfLiteOk) {
        MicroPrintf("PersonDetection Invoke failed\r\n");
        onBoxDetectionCallback(NULL, 0);
        return 0;
    }
    person_detection_log_interval("PersonDetection Invoke", start_us);

    if (!output_tensors_ready && !buildOutputTensors()) {
        onBoxDetectionCallback(NULL, 0);
        return 0;
    }

    person_detection_build_anchors();
    if (!s_person_anchors_ready) {
        onBoxDetectionCallback(NULL, 0);
        return 0;
    }

    start_us = bk_aon_rtc_get_us();
    const int detection_count = person_detection_decode_outputs();
    person_detection_log_interval("PersonDetection decode + NMS", start_us);

    if (detection_count <= 0) {
        onBoxDetectionCallback(NULL, 0);
        return 0;
    }

    onBoxDetectionCallback(s_person_detections, detection_count);
    return detection_count;
}
