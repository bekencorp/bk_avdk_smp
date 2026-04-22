#include "YolofaceDetectionModel.h"
#include "yoloface_detect_model_data.h"
#include "box.h"


constexpr int kNumCols = 96;
constexpr int kNumRows = 96;
constexpr int kNumChannels      = 1;
constexpr int kMaxImageSize     = kNumCols * kNumRows * kNumChannels;
constexpr int kCategoryCount    = 2;
constexpr int kPersonIndex      = 1;
constexpr int kNotAPersonIndex  = 0;

// Face detection box structure

// Maximum number of faces to detect
constexpr int kMaxFaces = 100;

/**
 * @brief Face Detection Configuration Structure
 *
 * This structure contains all tunable parameters that affect face detection box size, position, and filtering.
 *
 * HOW TO TUNE THESE PARAMETERS:
 *
 * 1. Box Size Scaling (for close-up faces):
 *    - box_scale_normalized: Increase (1.2->1.5) to make boxes larger for normalized coordinates
 *    - box_scale_logspace_grid: Decrease (1.0->0.8) to make boxes larger for log-space coordinates
 *    - max_box_size_ratio: Increase (0.7->0.8) to allow larger maximum box size
 *
 * 2. Minimum Box Size (for far-away faces):
 *    - min_face_height_ratio: Increase (0.4->0.5) to force minimum height, decrease to allow smaller boxes when far away
 *    - min_box_size_ratio: Increase (0.08->0.12) to enforce larger minimum box size
 *
 * 3. Box Width Control:
 *    - target_width_ratio: Decrease (0.65->0.6) to make boxes narrower, increase to make wider
 *    - min_aspect_ratio: Decrease (0.5->0.4) to allow narrower boxes
 *    - max_aspect_ratio: Decrease (1.0->0.9) to prevent wide boxes
 *
 * 4. Position Adjustment:
 *    - center_y_shift_down: Increase (0.025->0.04) to shift box down more (include chin), decrease to shift up
 *
 * 5. Filtering Thresholds:
 *    - score_threshold: Increase (0.6->0.7) to filter more low-confidence detections
 *    - min_box_size_pixels: Increase (4->6) to filter smaller boxes
 *    - min_center_x/y, max_center_x/y: Adjust to filter boxes at image edges
 *
 * DEBUGGING TIPS:
 * - If boxes are too small: Increase box_scale_normalized, decrease box_scale_logspace_grid
 * - If boxes are too large when far away: Decrease min_face_height_ratio, adjust expansion logic
 * - If boxes don't include chin: Increase center_y_shift_down
 * - If boxes are too wide: Decrease target_width_ratio, decrease max_aspect_ratio
 * - If detecting non-face regions: Tighten position constraints (min/max_center_x/y)
 */
struct FaceDetectionConfig {
    // Score threshold
    float score_threshold;              // Minimum confidence score (0.0-1.0), default: 0.6

    // Box size scaling factors
    float box_scale_normalized;         // Scale factor for normalized coordinates (0.0-1.0), default: 1.35
    float box_scale_logspace_grid;      // Grid multiplier for log-space boxes (smaller=larger), default: 0.95
    float max_box_size_ratio;           // Maximum box size as fraction of image (0.0-1.0), default: 0.72

    // Minimum box size constraints
    float min_face_height_ratio;        // Minimum face height ratio (0.0-1.0), default: 0.45
    float min_box_size_ratio;           // Minimum box size ratio (0.0-1.0), default: 0.08

    // Box width control
    float target_width_ratio;           // Target width/height ratio (0.0-1.0), default: 0.65
    float min_aspect_ratio;             // Minimum width/height ratio, default: 0.5
    float max_aspect_ratio;             // Maximum width/height ratio, default: 1.0

    // Position adjustment
    float center_y_shift_down;          // Downward shift for center Y (0.0-0.1), default: 0.025

    // Filtering thresholds
    int min_box_size_pixels;            // Minimum box size in pixels, default: 4
    float filter_min_aspect_ratio;      // Filter: minimum aspect ratio, default: 0.4
    float filter_max_aspect_ratio;      // Filter: maximum aspect ratio, default: 2.5
    float min_center_x;                 // Filter: minimum center X position (0.0-1.0), default: 0.15
    float max_center_x;                 // Filter: maximum center X position (0.0-1.0), default: 0.85
    float min_center_y;                 // Filter: minimum center Y position (0.0-1.0), default: 0.20
    float max_center_y;                 // Filter: maximum center Y position (0.0-1.0), default: 0.80
};

// Global configuration constant - modify these values to tune face detection
// NOTE: This configuration assumes 56x56 detection input mapped proportionally to 1088x1088 display.
//       The post-processing is kept simple so that the box更贴近模型原始预测，只做必要的几何约束。
static const FaceDetectionConfig g_face_detection_config = {
    // Score threshold
    .score_threshold = 0.60f,

    // Box size scaling factors
    .box_scale_normalized    = 1.20f,  // 轻微放大归一化预测框
    .box_scale_logspace_grid = 1.00f,  // 不额外放大/缩小 log-space 框
    .max_box_size_ratio      = 0.80f,  // 最大不超过整幅图的 80%

    // Minimum box size constraints
    .min_face_height_ratio = 0.30f,    // 最小人脸高度约占 30%
    .min_box_size_ratio    = 0.05f,    // 最小宽/高约 5%

    // Box width control（轻微约束）
    .target_width_ratio = 0.65f,
    .min_aspect_ratio   = 0.50f,
    .max_aspect_ratio   = 1.10f,

    // Position adjustment（不再大幅平移，仅轻微下移）
    .center_y_shift_down = 0.02f,

    // Filtering thresholds
    .min_box_size_pixels     = 6,
    .filter_min_aspect_ratio = 0.40f,
    .filter_max_aspect_ratio = 1.80f,
    .min_center_x            = 0.10f,
    .max_center_x            = 0.90f,
    .min_center_y            = 0.10f,
    .max_center_y            = 0.90f
};

// Forward declarations (using pointers for C compatibility)
static int ProcessYoloGrid4D(const TfLiteTensor* output, float output_scale, int32_t output_zero_point,
                              int batch_size, int grid_h, int grid_w, int output_channels,
                              int width, int height, FaceBox* all_faces, int* all_face_count);
static int ProcessYolo2D(const TfLiteTensor* output, float output_scale, int32_t output_zero_point,
                         int width, int height, FaceBox* all_faces, int* all_face_count);
static int ProcessYolo3D(const TfLiteTensor* output, float output_scale, int32_t output_zero_point,
                         int width, int height, FaceBox* all_faces, int* all_face_count);
static int ProcessYoloOther(const TfLiteTensor* output, float output_scale, int32_t output_zero_point,
                             int output_dims, int width, int height, FaceBox* all_faces, int* all_face_count);
static void SortFacesByScore(FaceBox* faces, int count);
static int FilterAndSelectBestFace(FaceBox* all_faces, int all_face_count, int width, int height,
                                   FaceBox* best_face, int* filtered_count);
static void AdjustBoxCoordinates(float* box_w, float* box_h, float* normalized_x, float* normalized_y,
                                 int width, int height);

// Sigmoid function for YOLO output processing
// Optimized for ARM M55: avoid numerical instability with large values
static inline float sigmoid(float x) {
    // Clamp input to avoid overflow/underflow
    if (x > 10.0f) return 1.0f;
    if (x < -10.0f) return 0.0f;
    // Use more stable computation: 1/(1+exp(-x)) = exp(x)/(1+exp(x))
    float exp_x = expf(x);
    return exp_x / (1.0f + exp_x);
}

// ============================================================================
// Helper Functions for Face Detection Processing
// ============================================================================

/**
 * @brief Adjust box coordinates and size based on configuration
 * This function applies all size constraints, aspect ratio adjustments, and position shifts
 */
static void AdjustBoxCoordinates(float* box_w, float* box_h, float* normalized_x, float* normalized_y,
    int width, int height) {
    const FaceDetectionConfig& cfg = g_face_detection_config;

    (void)width;
    (void)height;

    if (*box_w < 0.0f) *box_w = 0.0f;
    if (*box_h < 0.0f) *box_h = 0.0f;
    if (*box_w > cfg.max_box_size_ratio) *box_w = cfg.max_box_size_ratio;
    if (*box_h > cfg.max_box_size_ratio) *box_h = cfg.max_box_size_ratio;

    if (*box_w < cfg.min_box_size_ratio) *box_w = cfg.min_box_size_ratio;
    if (*box_h < cfg.min_box_size_ratio) *box_h = cfg.min_box_size_ratio;

// 3) 轻微的人脸高度下限（避免“只框眼睛/鼻子”），但不强行拉满整张脸
    if (*box_h < cfg.min_face_height_ratio) {
        *box_h = cfg.min_face_height_ratio;
    }

    float aspect_ratio = (*box_h > 0.0f) ? (*box_w / *box_h) : 1.0f;
    if (aspect_ratio < cfg.min_aspect_ratio) {
        *box_w = *box_h * cfg.min_aspect_ratio;
    } else if (aspect_ratio > cfg.max_aspect_ratio) {
        *box_w = *box_h * cfg.max_aspect_ratio;
    }

    *normalized_y = *normalized_y + cfg.center_y_shift_down;
    if (*normalized_y > 1.0f) *normalized_y = 1.0f;
}

/**
 * @brief Convert normalized box coordinates to pixel coordinates
 */
static void ConvertToPixelCoordinates(float normalized_x, float normalized_y, float box_w, float box_h,
                                      int width, int height, int& xmin, int& ymin, int& xmax, int& ymax) {
    float width_float = (float)width;
    float height_float = (float)height;

    float half_w = box_w * 0.5f;
    float half_h = box_h * 0.5f;

    float x1 = (normalized_x - half_w) * width_float;
    float y1 = (normalized_y - half_h) * height_float;
    float x2 = (normalized_x + half_w) * width_float;
    float y2 = (normalized_y + half_h) * height_float;

    if (x1 < 0.0f) x1 = 0.0f;
    if (y1 < 0.0f) y1 = 0.0f;
    if (x2 > width_float) x2 = width_float;
    if (y2 > height_float) y2 = height_float;

    if (x1 >= x2) {
        if (x2 < width_float - 1.0f) x2 = x1 + 1.0f;
        else x1 = x2 - 1.0f;
    }
    if (y1 >= y2) {
        if (y2 < height_float - 1.0f) y2 = y1 + 1.0f;
        else y1 = y2 - 1.0f;
    }

    xmin = (int)(x1 + 0.5f);
    ymin = (int)(y1 + 0.5f);
    xmax = (int)(x2 + 0.5f);
    ymax = (int)(y2 + 0.5f);

    if (xmin < 0) xmin = 0;
    if (ymin < 0) ymin = 0;
    if (xmax > width) xmax = width;
    if (ymax > height) ymax = height;
    if (xmin >= xmax) {
        if (xmax < width) xmax = xmin + 1;
        else xmin = xmax - 1;
    }
    if (ymin >= ymax) {
        if (ymax < height) ymax = ymin + 1;
        else ymin = ymax - 1;
    }
}

/**
 * @brief Process YOLO 4D grid format output (main path: [batch, grid_h, grid_w, channels])
 * This is the most common format for YOLO face detection models
 */
static int ProcessYoloGrid4D(const TfLiteTensor* output, float output_scale, int32_t output_zero_point,
                             int batch_size, int grid_h, int grid_w, int output_channels,
                             int width, int height, FaceBox* all_faces, int* all_face_count) {
    const FaceDetectionConfig& cfg = g_face_detection_config;

    int num_anchors = 0;
    int values_per_anchor = 0;

    if (output_channels % 6 == 0) {
        num_anchors = output_channels / 6;
        values_per_anchor = 6;
    } else if (output_channels % 9 == 0) {
        num_anchors = output_channels / 9;
        values_per_anchor = 9;
    } else {
        num_anchors = 2;
        values_per_anchor = output_channels / num_anchors;
    }

    MicroPrintf("YOLO Grid Format: [%d, %d, %d, %d], %d anchors, %d values per anchor\r\n",
                batch_size, grid_h, grid_w, output_channels, num_anchors, values_per_anchor);

    for (int b = 0; b < batch_size; b++) {
        for (int y = 0; y < grid_h; y++) {
            for (int x = 0; x < grid_w; x++) {
                for (int a = 0; a < num_anchors && *all_face_count < kMaxFaces; a++) {
                    int base_idx = b * grid_h * grid_w * output_channels +
                                  y * grid_w * output_channels +
                                  x * output_channels +
                                  a * values_per_anchor;

                    int total_elements = batch_size * grid_h * grid_w * output_channels;
                    if (base_idx + values_per_anchor - 1 >= total_elements) {
                        MicroPrintf("Warning: index out of bounds at [b=%d, y=%d, x=%d, a=%d]\r\n", b, y, x, a);
                        break;
                    }

                    float raw_box_x = ((float)output->data.int8[base_idx + 0] - output_zero_point) * output_scale;
float raw_box_y = ((float)output->data.int8[base_idx + 1] - output_zero_point) * output_scale;
float raw_box_w = ((float)output->data.int8[base_idx + 2] - output_zero_point) * output_scale;
float raw_box_h = ((float)output->data.int8[base_idx + 3] - output_zero_point) * output_scale;
float raw_objectness = ((float)output->data.int8[base_idx + 4] - output_zero_point) * output_scale;

// Apply sigmoid if needed
float box_x = (raw_box_x >= 0.0f && raw_box_x <= 1.0f) ? raw_box_x : sigmoid(raw_box_x);
float box_y = (raw_box_y >= 0.0f && raw_box_y <= 1.0f) ? raw_box_y : sigmoid(raw_box_y);
float objectness = (raw_objectness >= 0.0f && raw_objectness <= 1.0f) ? raw_objectness : sigmoid(raw_objectness);

// Process box_w and box_h
float box_w, box_h;
if (raw_box_w >= 0.0f && raw_box_w <= 1.0f &&
raw_box_h >= 0.0f && raw_box_h <= 1.0f) {
box_w = raw_box_w * cfg.box_scale_normalized;
box_h = raw_box_h * cfg.box_scale_normalized;
} else {
float exp_w_input = raw_box_w;
float exp_h_input = raw_box_h;
if (exp_w_input > 5.0f) exp_w_input = 5.0f;
if (exp_w_input < -5.0f) exp_w_input = -5.0f;
if (exp_h_input > 5.0f) exp_h_input = 5.0f;
if (exp_h_input < -5.0f) exp_h_input = -5.0f;

float exp_w = expf(exp_w_input);
float exp_h = expf(exp_h_input);
box_w = exp_w / (float)(grid_w * cfg.box_scale_logspace_grid);
box_h = exp_h / (float)(grid_h * cfg.box_scale_logspace_grid);
}

// Get class score
float class_score = 0.0f;
if (values_per_anchor >= 6) {
float raw_class = ((float)output->data.int8[base_idx + 5] - output_zero_point) * output_scale;
class_score = (raw_class >= 0.0f && raw_class <= 1.0f) ? raw_class : sigmoid(raw_class);
for (int c = 6; c < values_per_anchor; c++) {
float raw_cls = ((float)output->data.int8[base_idx + c] - output_zero_point) * output_scale;
float cls = (raw_cls >= 0.0f && raw_cls <= 1.0f) ? raw_cls : sigmoid(raw_cls);
if (cls > class_score) class_score = cls;
}
} else {
class_score = objectness;
}

float score = objectness * class_score;
if (objectness < 0.1f) continue;

// Convert grid coordinates to normalized image coordinates
float grid_x_float = (float)x;
float grid_y_float = (float)y;
float grid_w_float = (float)grid_w;
float grid_h_float = (float)grid_h;

float normalized_x = (grid_x_float + box_x) / grid_w_float;
float normalized_y = (grid_y_float + box_y) / grid_h_float;

// Clamp coordinates
if (normalized_x < 0.0f) normalized_x = 0.0f;
if (normalized_x > 1.0f) normalized_x = 1.0f;
if (normalized_y < 0.0f) normalized_y = 0.0f;
if (normalized_y > 1.0f) normalized_y = 1.0f;

// Adjust box coordinates
AdjustBoxCoordinates(&box_w, &box_h, &normalized_x, &normalized_y, width, height);

// Convert to pixel coordinates
int xmin, ymin, xmax, ymax;
ConvertToPixelCoordinates(normalized_x, normalized_y, box_w, box_h, width, height,
                 xmin, ymin, xmax, ymax);

// Debug output
if (score > cfg.score_threshold) {
int box_w_pixels = xmax - xmin;
int box_h_pixels = ymax - ymin;
MicroPrintf("Detection %d: grid[%d,%d] anchor[%d], score=%.3f\r\n",
          *all_face_count, x, y, a, score);
MicroPrintf("  raw dequantized: box_x=%.3f, box_y=%.3f, box_w=%.3f, box_h=%.3f\r\n",
      raw_box_x, raw_box_y, raw_box_w, raw_box_h);
MicroPrintf("  after sigmoid/exp: box_x=%.3f, box_y=%.3f, box_w=%.3f, box_h=%.3f\r\n",
      box_x, box_y, box_w, box_h);
MicroPrintf("  normalized: center(%.3f, %.3f), size(%.3f, %.3f)\r\n",
      normalized_x, normalized_y, box_w, box_h);
MicroPrintf("  final pixel: [%d, %d, %d, %d] size=%dx%d (ratio=%.1f%%x%.1f%%) in %dx%d image\r\n",
      xmin, ymin, xmax, ymax, box_w_pixels, box_h_pixels,
      (box_w_pixels * 100.0f / width), (box_h_pixels * 100.0f / height),
      width, height);
}

all_faces[*all_face_count].score = score;
all_faces[*all_face_count].xmin = (short)xmin;
all_faces[*all_face_count].ymin = (short)ymin;
all_faces[*all_face_count].xmax = (short)xmax;
all_faces[*all_face_count].ymax = (short)ymax;
(*all_face_count)++;
}
}
}
}

return *all_face_count;
}

/**
 * @brief Process YOLO 2D format output ([num_boxes, 5])
 * Rarely used, kept for compatibility
 */
static int ProcessYolo2D(const TfLiteTensor* output, float output_scale, int32_t output_zero_point,
int width, int height, FaceBox* all_faces, int* all_face_count) {
int num_boxes = output->dims->data[0];
float width_float = (float)width;
float height_float = (float)height;

for (int i = 0; i < num_boxes && *all_face_count < kMaxFaces; i++) {
int base_idx = i * 5;
float x = ((float)output->data.int8[base_idx + 0] - output_zero_point) * output_scale;
float y = ((float)output->data.int8[base_idx + 1] - output_zero_point) * output_scale;
float w = ((float)output->data.int8[base_idx + 2] - output_zero_point) * output_scale;
float h = ((float)output->data.int8[base_idx + 3] - output_zero_point) * output_scale;
float score = ((float)output->data.int8[base_idx + 4] - output_zero_point) * output_scale;

float x1, y1, x2, y2;
if (x >= 0.0f && x <= 1.0f && y >= 0.0f && y <= 1.0f &&
w >= 0.0f && w <= 1.0f && h >= 0.0f && h <= 1.0f) {
float half_w = w * 0.5f;
float half_h = h * 0.5f;
x1 = (x - half_w) * width_float;
y1 = (y - half_h) * height_float;
x2 = (x + half_w) * width_float;
y2 = (y + half_h) * height_float;
} else {
float half_w = w * 0.5f;
float half_h = h * 0.5f;
x1 = x - half_w;
y1 = y - half_h;
x2 = x + half_w;
y2 = y + half_h;
}

if (x1 < 0.0f) x1 = 0.0f;
if (y1 < 0.0f) y1 = 0.0f;
if (x2 > width_float) x2 = width_float;
if (y2 > height_float) y2 = height_float;

if (x1 >= x2) {
if (x2 < width_float - 1.0f) x2 = x1 + 1.0f;
else x1 = x2 - 1.0f;
}
if (y1 >= y2) {
if (y2 < height_float - 1.0f) y2 = y1 + 1.0f;
else y1 = y2 - 1.0f;
}

all_faces[*all_face_count].score = score;
all_faces[*all_face_count].xmin = (short)x1;
all_faces[*all_face_count].ymin = (short)y1;
all_faces[*all_face_count].xmax = (short)x2;
all_faces[*all_face_count].ymax = (short)y2;
(*all_face_count)++;
}

return *all_face_count;
}

/**
 * @brief Process YOLO 3D format output ([batch, num_boxes, 5])
 * Rarely used, kept for compatibility
 */
static int ProcessYolo3D(const TfLiteTensor* output, float output_scale, int32_t output_zero_point,
int width, int height, FaceBox* all_faces, int* all_face_count) {
int batch_size = output->dims->data[0];
int num_boxes = output->dims->data[1];
float width_float = (float)width;
float height_float = (float)height;

for (int b = 0; b < batch_size; b++) {
for (int i = 0; i < num_boxes && *all_face_count < kMaxFaces; i++) {
int base_idx = (b * num_boxes + i) * 5;
float x = ((float)output->data.int8[base_idx + 0] - output_zero_point) * output_scale;
float y = ((float)output->data.int8[base_idx + 1] - output_zero_point) * output_scale;
float w = ((float)output->data.int8[base_idx + 2] - output_zero_point) * output_scale;
float h = ((float)output->data.int8[base_idx + 3] - output_zero_point) * output_scale;
float score = ((float)output->data.int8[base_idx + 4] - output_zero_point) * output_scale;

float x1, y1, x2, y2;
if (x >= 0.0f && x <= 1.0f && y >= 0.0f && y <= 1.0f &&
w >= 0.0f && w <= 1.0f && h >= 0.0f && h <= 1.0f) {
float half_w = w * 0.5f;
float half_h = h * 0.5f;
x1 = (x - half_w) * width_float;
y1 = (y - half_h) * height_float;
x2 = (x + half_w) * width_float;
y2 = (y + half_h) * height_float;
} else {
float half_w = w * 0.5f;
float half_h = h * 0.5f;
x1 = x - half_w;
y1 = y - half_h;
x2 = x + half_w;
y2 = y + half_h;
}

if (x1 < 0.0f) x1 = 0.0f;
if (y1 < 0.0f) y1 = 0.0f;
if (x2 > width_float) x2 = width_float;
if (y2 > height_float) y2 = height_float;

if (x1 >= x2) {
if (x2 < width_float - 1.0f) x2 = x1 + 1.0f;
else x1 = x2 - 1.0f;
}
if (y1 >= y2) {
if (y2 < height_float - 1.0f) y2 = y1 + 1.0f;
else y1 = y2 - 1.0f;
}

all_faces[*all_face_count].score = score;
all_faces[*all_face_count].xmin = (short)x1;
all_faces[*all_face_count].ymin = (short)y1;
all_faces[*all_face_count].xmax = (short)x2;
all_faces[*all_face_count].ymax = (short)y2;
(*all_face_count)++;
}
}

return *all_face_count;
}

/**
 * @brief Process other YOLO output formats (fallback)
 * Very rarely used, kept for compatibility
 */
static int ProcessYoloOther(const TfLiteTensor* output, float output_scale, int32_t output_zero_point,
                            int output_dims, int width, int height, FaceBox* all_faces, int* all_face_count) {
    int total_elements = 1;
    for (int i = 0; i < output_dims; i++) {
        total_elements *= output->dims->data[i];
    }

    int num_boxes = total_elements / 5;
    for (int i = 0; i < num_boxes && *all_face_count < kMaxFaces; i++) {
        int base_idx = i * 5;
        if (base_idx + 4 >= total_elements) break;

        float x = ((float)output->data.int8[base_idx + 0] - output_zero_point) * output_scale;
        float y = ((float)output->data.int8[base_idx + 1] - output_zero_point) * output_scale;
        float w = ((float)output->data.int8[base_idx + 2] - output_zero_point) * output_scale;
        float h = ((float)output->data.int8[base_idx + 3] - output_zero_point) * output_scale;
        float score = ((float)output->data.int8[base_idx + 4] - output_zero_point) * output_scale;

        float x1, y1, x2, y2;
        if (x >= 0.0f && x <= 1.0f && y >= 0.0f && y <= 1.0f &&
            w >= 0.0f && w <= 1.0f && h >= 0.0f && h <= 1.0f) {
            x1 = (x - w / 2.0f) * width;
            y1 = (y - h / 2.0f) * height;
            x2 = (x + w / 2.0f) * width;
            y2 = (y + h / 2.0f) * height;
        } else {
            x1 = x - w / 2.0f;
            y1 = y - h / 2.0f;
            x2 = x + w / 2.0f;
            y2 = y + h / 2.0f;
        }

        if (x1 < 0.0f) x1 = 0.0f;
        if (y1 < 0.0f) y1 = 0.0f;
        if (x2 > width) x2 = width;
        if (y2 > height) y2 = height;

        all_faces[*all_face_count].score = score;
        all_faces[*all_face_count].xmin = (short)x1;
        all_faces[*all_face_count].ymin = (short)y1;
        all_faces[*all_face_count].xmax = (short)x2;
        all_faces[*all_face_count].ymax = (short)y2;
        (*all_face_count)++;
    }

    return *all_face_count;
}

/**
 * @brief Sort faces by score in descending order
 */
static void SortFacesByScore(FaceBox* faces, int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (faces[j].score < faces[j + 1].score) {
                FaceBox temp = faces[j];
                faces[j] = faces[j + 1];
                faces[j + 1] = temp;
            }
        }
    }
}

/**
 * @brief Filter faces and select the best one based on score, size, aspect ratio, and position
 */
static int FilterAndSelectBestFace(FaceBox* all_faces, int all_face_count, int width, int height,
                                   FaceBox* best_face, int* filtered_count) {
    const FaceDetectionConfig& cfg = g_face_detection_config;

    int best_face_idx = -1;
    float best_score = cfg.score_threshold;
    int face_count = 0;

    for (int i = 0; i < all_face_count; i++) {
        if (all_faces[i].score > cfg.score_threshold) {
            int box_w = all_faces[i].xmax - all_faces[i].xmin;
            int box_h = all_faces[i].ymax - all_faces[i].ymin;
            float aspect_ratio = (box_h > 0) ? ((float)box_w / (float)box_h) : 0.0f;

            float center_x = ((float)(all_faces[i].xmin + all_faces[i].xmax) / 2.0f) / (float)width;
            float center_y = ((float)(all_faces[i].ymin + all_faces[i].ymax) / 2.0f) / (float)height;

            bool is_valid = (box_w >= cfg.min_box_size_pixels && box_h >= cfg.min_box_size_pixels);
            bool has_good_aspect = (aspect_ratio >= cfg.filter_min_aspect_ratio && aspect_ratio <= cfg.filter_max_aspect_ratio);
            bool has_good_position = (center_x >= cfg.min_center_x && center_x <= cfg.max_center_x &&
                                     center_y >= cfg.min_center_y && center_y <= cfg.max_center_y);

            if (is_valid && has_good_aspect && has_good_position) {
                face_count++;
                if (all_faces[i].score > best_score) {
                    best_score = all_faces[i].score;
                    best_face_idx = i;
                    *best_face = all_faces[i];
                }
            } else {
                (*filtered_count)++;
                if (!is_valid) {
                    MicroPrintf("Filtered small box: [%d,%d,%d,%d] size=%dx%d, score=%.3f\r\n",
                               all_faces[i].xmin, all_faces[i].ymin, all_faces[i].xmax, all_faces[i].ymax,
                               box_w, box_h, all_faces[i].score);
                } else if (!has_good_aspect) {
                    MicroPrintf("Filtered bad aspect box: [%d,%d,%d,%d] size=%dx%d (ratio=%.2f), score=%.3f\r\n",
                               all_faces[i].xmin, all_faces[i].ymin, all_faces[i].xmax, all_faces[i].ymax,
                               box_w, box_h, aspect_ratio, all_faces[i].score);
                } else {
                    MicroPrintf("Filtered bad position box: [%d,%d,%d,%d] center=(%.2f,%.2f), score=%.3f\r\n",
                               all_faces[i].xmin, all_faces[i].ymin, all_faces[i].xmax, all_faces[i].ymax,
                               center_x, center_y, all_faces[i].score);
                }
            }
        } else {
            (*filtered_count)++;
        }
    }

    return best_face_idx;
}

void YolofaceDetectionModel::resolverLoad(void)
{
    micro_op_resolver.AddEthosU();
    micro_op_resolver.AddPadV2();
    micro_op_resolver.AddTranspose();
}


void YolofaceDetectionModel::resourceLoad(void)
{
    name = "yolofaceDetection";
    width = 56;
    height = 56;
    format = BK_PIXEL_FORMAT_RGB888;
    model_type = AVDK_NN_MODEL_TYPE_NPU;
    model_ram_type = AVDK_NN_MEM_TYPE_FALSH;
    model_flash_data = (uint8_t*)g_yoloface_int8_vela_tflite;
    model_flash_data_size = YOLOFACE_MODEL_DATA_SIZE;
    model_data = (uint8_t*)g_yoloface_int8_vela_tflite;
    model_data_size = YOLOFACE_MODEL_DATA_SIZE;

    fast_ram_type = AVDK_NN_MEM_TYPE_HSRAM;
    fast_ram_data_size = 40 * 1024;
    fast_ram_data = NULL;

    arena_data_size = 80 * 1024;
    arena_ram_type = AVDK_NN_MEM_TYPE_HSRAM;
    arena_ram_data = NULL;
}

void YolofaceDetectionModel::resourceUnload(void)
{
    //TODO: Implement resource unload
}


int YolofaceDetectionModel::run(uint8_t *data, uint32_t size, bk_pixel_format_t format)
{
    uint32_t expected_size = 0;

    if (format == BK_PIXEL_FORMAT_RGB888) {
        expected_size = (uint32_t)width * (uint32_t)height * 3U;
    } else if (format == BK_PIXEL_FORMAT_BGRA8888) {
        expected_size = (uint32_t)width * (uint32_t)height * 4U;
    } else {
        MicroPrintf("YolofaceDetectionModel: unsupported pixel format %u\r\n", (unsigned)format);
        return 0;
    }

    if (size != expected_size) {
        MicroPrintf("YolofaceDetectionModel: invalid size %u, expected %u\r\n",
                    (unsigned)size, (unsigned)expected_size);
        return 0;
    }

    TfLiteTensor* input = pinterpreter->input(0);

    /* Convert input to model RGB (int8, zero-centered) per bk_pixel_format_t. */
    if (format == BK_PIXEL_FORMAT_BGRA8888) {
        for (int i = 0; i < width * height; i++) {
            input->data.int8[i * 3 + 0] = (int8_t)data[i * 4 + 2] - 128;
            input->data.int8[i * 3 + 1] = (int8_t)data[i * 4 + 1] - 128;
            input->data.int8[i * 3 + 2] = (int8_t)data[i * 4 + 0] - 128;
        }
    } else if (format == BK_PIXEL_FORMAT_RGB888) {
        for (int i = 0; i < width * height; i++) {
            input->data.int8[i * 3 + 0] = (int8_t)data[i * 3 + 0] - 128;
            input->data.int8[i * 3 + 1] = (int8_t)data[i * 3 + 1] - 128;
            input->data.int8[i * 3 + 2] = (int8_t)data[i * 3 + 2] - 128;
        }
    } else {
        MicroPrintf("Unsupported format: %d\r\n", format);
        return 0;
    }

    if(kTfLiteOk != pinterpreter->Invoke())
    {
        MicroPrintf("Invoke failed\r\n");
        return 0;
    }

    TfLiteTensor* output = pinterpreter->output(0);

    // Get output parameters for dequantization
    float output_scale = output->params.scale;
    int32_t output_zero_point = output->params.zero_point;

    // Get output dimensions
    int output_dims = output->dims->size;
    int batch_size = 1;
    int grid_h = 1;
    int grid_w = 1;
    int output_channels = 1;

    if (output_dims >= 1) batch_size = output->dims->data[0];
    if (output_dims >= 2) grid_h = output->dims->data[1];
    if (output_dims >= 3) grid_w = output->dims->data[2];
    if (output_dims >= 4) output_channels = output->dims->data[3];

    // Print output dimensions for debugging
    MicroPrintf("Output dims: %d, shape: [%d", output_dims, batch_size);
    for (int i = 1; i < output_dims; i++) {
        MicroPrintf(", %d", output->dims->data[i]);
    }
    MicroPrintf("], scale: %f, zero_point: %d\r\n", output_scale, output_zero_point);

    // Verify dimension assignment for debugging
    if (output_dims >= 4) {
        MicroPrintf("Dimension mapping: batch=%d, grid_h=%d, grid_w=%d, channels=%d\r\n",
                    batch_size, grid_h, grid_w, output_channels);
    }

    // Parse detection results - collect ALL detections first
    FaceBox all_faces[kMaxFaces];
    int all_face_count = 0;
    int filtered_count = 0;

    // Process output based on dimensions (4D is the most common format)
    if (output_dims == 4) {
        // Main path: YOLO grid format [batch, grid_h, grid_w, channels]
        ProcessYoloGrid4D(output, output_scale, output_zero_point,
                          batch_size, grid_h, grid_w, output_channels,
                          width, height, all_faces, &all_face_count);
    } else if (output_dims == 2 && output->dims->data[1] == 5) {
        // Secondary path: 2D format [num_boxes, 5] - rarely used
        ProcessYolo2D(output, output_scale, output_zero_point,
                      width, height, all_faces, &all_face_count);
    } else if (output_dims == 3 && output->dims->data[2] == 5) {
        // Secondary path: 3D format [batch, num_boxes, 5] - rarely used
        ProcessYolo3D(output, output_scale, output_zero_point,
                      width, height, all_faces, &all_face_count);
    } else {
        // Fallback path: other formats - very rarely used
        ProcessYoloOther(output, output_scale, output_zero_point,
                         output_dims, width, height, all_faces, &all_face_count);
    }

    // Sort faces by score
    SortFacesByScore(all_faces, all_face_count);

    // Filter and select best face
    FaceBox best_face;
    int best_face_idx = FilterAndSelectBestFace(all_faces, all_face_count, width, height,
                                                 &best_face, &filtered_count);

    // Print detection results
    const FaceDetectionConfig& cfg = g_face_detection_config;
    MicroPrintf("========== Face Detection Results ==========\r\n");
    MicroPrintf("Total detections: %d, Filtered (low score, too small, bad aspect, or bad position): %d\r\n",
                all_face_count, filtered_count);
    MicroPrintf("High-score valid faces (threshold > %.2f, size >= %dx%d, aspect %.2f-%.2f, position valid): %d\r\n",
                cfg.score_threshold, cfg.min_box_size_pixels, cfg.min_box_size_pixels,
                cfg.filter_min_aspect_ratio, cfg.filter_max_aspect_ratio,
                (best_face_idx >= 0 ? 1 : 0));

    // Print all high-score faces for debugging
    if (best_face_idx >= 0) {
        MicroPrintf("All high-score faces:\r\n");
        for (int i = 0; i < all_face_count; i++) {
            if (all_faces[i].score > cfg.score_threshold) {
                int box_w = all_faces[i].xmax - all_faces[i].xmin;
                int box_h = all_faces[i].ymax - all_faces[i].ymin;
                float aspect_ratio = (box_h > 0) ? ((float)box_w / (float)box_h) : 0.0f;
                float center_x = ((float)(all_faces[i].xmin + all_faces[i].xmax) / 2.0f) / (float)width;
                float center_y = ((float)(all_faces[i].ymin + all_faces[i].ymax) / 2.0f) / (float)height;
                bool is_valid = (box_w >= cfg.min_box_size_pixels && box_h >= cfg.min_box_size_pixels);
                bool has_good_aspect = (aspect_ratio >= cfg.filter_min_aspect_ratio && aspect_ratio <= cfg.filter_max_aspect_ratio);
                bool has_good_position = (center_x >= cfg.min_center_x && center_x <= cfg.max_center_x &&
                                         center_y >= cfg.min_center_y && center_y <= cfg.max_center_y);
                if (is_valid && has_good_aspect && has_good_position) {
                    MicroPrintf("  Face %d: score=%.3f, box=[%d,%d,%d,%d] size=%dx%d (ratio=%.2f) center=(%.2f,%.2f)\r\n",
                               i, all_faces[i].score,
                               all_faces[i].xmin, all_faces[i].ymin, all_faces[i].xmax, all_faces[i].ymax,
                               box_w, box_h, aspect_ratio, center_x, center_y);
                }
            }
        }
    }

    if (best_face_idx < 0) {
        MicroPrintf("No faces detected above confidence threshold (%.2f).\r\n", cfg.score_threshold);
        if (all_face_count > 0) {
            MicroPrintf("Highest score found: %.3f (below threshold %.2f)\r\n",
                        all_faces[0].score, cfg.score_threshold);
        }
        MicroPrintf("==========================================\r\n");
        box_detection_path_clear();
        return 0;
    }

    // Print only the best (highest score) face
    int img_x1 = best_face.xmin;
    int img_y1 = best_face.ymin;
    int img_x2 = best_face.xmax;
    int img_y2 = best_face.ymax;
    int img_w = img_x2 - img_x1;
    int img_h = img_y2 - img_y1;

    float x_center = (img_x1 + img_x2) / 2.0f;
    float y_center = (img_y1 + img_y2) / 2.0f;

    MicroPrintf("Best Face (Score: %.3f = %.1f%%):\r\n",
                best_face.score, best_face.score * 100.0f);
    MicroPrintf("  Center: (%.1f, %.1f), Size: %d x %d\r\n",
                x_center, y_center, img_w, img_h);
    MicroPrintf("  BBox [xmin, ymin, xmax, ymax]: [%d, %d, %d, %d]\r\n",
                img_x1, img_y1, img_x2, img_y2);
    MicroPrintf("  Corners: (%d, %d) -> (%d, %d)\r\n",
                img_x1, img_y1, img_x2, img_y2);
    MicroPrintf("==========================================\r\n");


    // Prepare face box for display - coordinates are in input image size (56x56)
    // box_detection_path_build will scale them to display size (1920x1088) and apply rotation
    FaceBox faces[1];
    faces[0].xmin = best_face.xmin;
    faces[0].ymin = best_face.ymin;
    faces[0].xmax = best_face.xmax;
    faces[0].ymax = best_face.ymax;
    faces[0].score = best_face.score;

    // Debug: print coordinates before scaling
    MicroPrintf("Before scaling: face box [%d, %d, %d, %d] in %dx%d input image\r\n",
                faces[0].xmin, faces[0].ymin, faces[0].xmax, faces[0].ymax, width, height);
    MicroPrintf("Will scale to display: %dx%d\r\n", 1088, 1088);

    box_detection_path_build(faces, 1, 1, 0, width, height, 1088, 1088);

    return 1;
}