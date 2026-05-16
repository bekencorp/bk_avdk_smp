#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/testing/micro_test.h"

#include "face_detection.h"

#include "components/bk_frame_buffer.h"
#include "box.h"
#include "bk_ethosu.h"
#include "components/log.h"
#include "os/mem.h"

class tflm_context
{
public:
    uint8_t* tensor_arena;          // Aligned pointer for TFLM
    void* tensor_arena_raw;          // Original pointer for freeing
    tflite::MicroInterpreter* pinterpreter;
    tflite::MicroMutableOpResolver<5> micro_op_resolver;
};

static void *g_ethosu0_scratch = nullptr;

#define ETHOSU0_SCRATCH_SIZE (138 * 1024)

int face_detection_init(void** handle, uint8_t* pmodel, uint32_t tensor_arena_size)
{
    int error = 0;
    tflm_context* tflm = nullptr;
    void* tensor_arena_raw = nullptr;
    void* tensor_arena_aligned = nullptr;

    if (g_ethosu0_scratch == nullptr) {
        g_ethosu0_scratch = hsram_malloc(ETHOSU0_SCRATCH_SIZE + 16);
        if (g_ethosu0_scratch == nullptr) {
            MicroPrintf("os_malloc ethosu0_scratch failed, size=%d\r\n", ETHOSU0_SCRATCH_SIZE);
            return -1;
        }
    }
    void* ethosu0_scratch_aligned = g_ethosu0_scratch ? (void*)(((uint32_t)g_ethosu0_scratch + 15) & ~15) : nullptr;

    MicroPrintf("g_ethosu0_scratch=%p, ethosu0_scratch_aligned=%p\r\n", g_ethosu0_scratch, ethosu0_scratch_aligned);

    int init_result = bk_ethosu_init(ethosu0_scratch_aligned, ETHOSU0_SCRATCH_SIZE);
    if(init_result != 0)
    {
        MicroPrintf("Failed to initialize Ethos-U driver, ret=%d\r\n", init_result);
        if (g_ethosu0_scratch) {
            os_free(g_ethosu0_scratch);
            g_ethosu0_scratch = nullptr;
        }
        return -1;
    }

    const tflite::Model* model = ::tflite::GetModel(pmodel);

    if(TFLITE_SCHEMA_VERSION != model->version()) return -1;

    tflm = new tflm_context;

    if(!tflm)
    {
        error = -1;
        goto __error;
    }

    // Initialize pointers
    tflm->tensor_arena = nullptr;
    tflm->tensor_arena_raw = nullptr;
    tflm->pinterpreter = nullptr;

    tflm->micro_op_resolver.AddEthosU();
    tflm->micro_op_resolver.AddPadV2();
    tflm->micro_op_resolver.AddTranspose();

    // Allocate memory with extra space for alignment, then align the pointer
    tensor_arena_raw = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, tensor_arena_size + 16);
    if(!tensor_arena_raw)
    {
        error = -2;
        goto __error;
    }

    // Align to 16-byte boundary
    tensor_arena_aligned = (void*)(((uint32_t)tensor_arena_raw + 15) & ~15);
    MicroPrintf("tensor_arena_raw=%p, tensor_arena_aligned=%p\r\n", tensor_arena_raw, tensor_arena_aligned);

    tflm->tensor_arena = (uint8_t*)tensor_arena_aligned;
    tflm->tensor_arena_raw = tensor_arena_raw;  // Save original pointer for freeing

    if(!(tflm->pinterpreter = new tflite::MicroInterpreter(model, tflm->micro_op_resolver, tflm->tensor_arena, tensor_arena_size)))
    {
        error = -3;
        goto __error;
    }

    if(kTfLiteOk != tflm->pinterpreter->AllocateTensors())
    {
        error = -4;
        goto __error;
    }
    else
    {
        //TfLiteTensor* input = pinterpreter->input(0);
        //TF_LITE_MICRO_EXPECT(input != nullptr);
        //TF_LITE_MICRO_EXPECT_EQ(4, input->dims->size);
        //TF_LITE_MICRO_EXPECT_EQ(1, input->dims->data[0]);
        //TF_LITE_MICRO_EXPECT_EQ(kNumRows, input->dims->data[1]);
        //TF_LITE_MICRO_EXPECT_EQ(kNumCols, input->dims->data[2]);
        //TF_LITE_MICRO_EXPECT_EQ(kNumChannels, input->dims->data[3]);
        //TF_LITE_MICRO_EXPECT_EQ(kTfLiteInt8, input->type);
    }

    *handle = tflm;
    MicroPrintf("####### Yolo Face Detection Init Success #######\n");
    return 0;

__error:
    if(tflm)
    {
        if(tflm->tensor_arena_raw) bk_frame_buffer_free(tflm->tensor_arena_raw);
        if(tflm->pinterpreter) delete tflm->pinterpreter;
        delete tflm;
    }
    else if(tensor_arena_raw)
    {
        // If tflm allocation failed but tensor_arena was allocated
        os_free(tensor_arena_raw);
    }

    *handle = 0;
    return error;
}

int face_detection_deinit(void* handle)
{
    if(handle)
    {
        bk_frame_buffer_free(((tflm_context*)handle)->pinterpreter);
        bk_frame_buffer_free(((tflm_context*)handle)->tensor_arena);
        delete (tflm_context*)handle;
    }

    return 0;
}

// Maximum number of faces to detect
constexpr int kMaxFaces = 100;
constexpr float kScoreThreshold = 20.0f;  // Minimum confidence score (must be > 0.5)

int face_detection_run(void* handle, int8_t* data, uint16_t width, uint16_t height, uint16_t channels)
{
    MicroPrintf("face_detection_run\r\n");

    tflite::MicroInterpreter* pinterpreter = ((tflm_context*)handle)->pinterpreter;

    TfLiteTensor* input = pinterpreter->input(0);

    // Convert input from BGRA to RGB and normalize
    for (int i = 0; i < width * height; i++)
    {
        input->data.int8[i * 3 + 0] = (int8_t)data[i * 4 + 2] - 128;
        input->data.int8[i * 3 + 1] = (int8_t)data[i * 4 + 1] - 128;
        input->data.int8[i * 3 + 2] = (int8_t)data[i * 4 + 0] - 128;
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
    int num_boxes = 0;
    int values_per_box = 6;  // Default: [x, y, w, h, score, class]

    // Print all dimensions for debugging
    MicroPrintf("Output dims: %d, shape: [", output_dims);
    for (int i = 0; i < output_dims; i++) {
        MicroPrintf("%d", output->dims->data[i]);
        if (i < output_dims - 1) MicroPrintf(", ");
    }
    MicroPrintf("], scale: %f, zero_point: %d\r\n", output_scale, output_zero_point);

    // Handle different output formats
    if (output_dims == 2) {
        // Format: [batch, num_values] or [num_boxes, values_per_box]
        int dim0 = output->dims->data[0];
        int dim1 = output->dims->data[1];
        
        if (dim0 == 1) {
            // Format: [1, num_values] - batch size is 1, flatten the rest
            // Assume 567 values = 567/6 = 94.5 boxes? Or different format?
            // Try: 567 values might be 94 boxes * 6 + 3 extra, or different layout
            if (dim1 % values_per_box == 0) {
                num_boxes = dim1 / values_per_box;
            } else {
                // If not divisible by 6, might be different format
                // Try treating as [1, num_boxes*6] where num_boxes = dim1/6 (rounded)
                num_boxes = dim1 / values_per_box;
                MicroPrintf("Warning: dim1 (%d) not divisible by %d, using %d boxes\r\n", 
                            dim1, values_per_box, num_boxes);
            }
        } else {
            // Format: [num_boxes, values_per_box]
            num_boxes = dim0;
            values_per_box = dim1;
        }
    } else if (output_dims == 3) {
        // Format: [batch, num_boxes, values_per_box]
        int batch_size = output->dims->data[0];
        num_boxes = output->dims->data[1];
        values_per_box = output->dims->data[2];
        MicroPrintf("3D format: batch=%d, boxes=%d, values=%d\r\n", batch_size, num_boxes, values_per_box);
        // For now, process only first batch
    } else {
        MicroPrintf("Unsupported output format with %d dimensions\r\n", output_dims);
        return 0;
    }

    MicroPrintf("Parsing: %d boxes, %d values per box\r\n", num_boxes, values_per_box);

    // Parse detection results - format: [x, y, w, h, score, class]
    //Box all_faces[kMaxFaces];
    int all_face_count = 0;
    int filtered_count = 0;
    float max_score = -1.0f;
    int max_score_idx = -1;
    float max_x = 0.0f, max_y = 0.0f, max_w = 0.0f, max_h = 0.0f;

    for (int i = 0; i < num_boxes && all_face_count < kMaxFaces; i++) {
        int base_idx = i * values_per_box;

        // Dequantize values: [x, y, w, h, score, class]
        float score = ((float)output->data.int8[base_idx + 4] - output_zero_point) * output_scale;
        // class value is at base_idx + 5, but not used currently
        // Skip reading class value to avoid unused variable warning

        // Filter by confidence threshold (must be > 0.5)
        if (score <= kScoreThreshold) {
            filtered_count++;
            continue;
        }

        // Only dequantize position values for detections above threshold
        float x = ((float)output->data.int8[base_idx + 0] - output_zero_point) * output_scale;
        float y = ((float)output->data.int8[base_idx + 1] - output_zero_point) * output_scale;
        float w = ((float)output->data.int8[base_idx + 2] - output_zero_point) * output_scale;
        float h = ((float)output->data.int8[base_idx + 3] - output_zero_point) * output_scale;

        // Track maximum score (only from detections above threshold)
        if (score > max_score) {
            max_score = score;
            max_score_idx = i;
            max_x = x;
            max_y = y;
            max_w = w;
            max_h = h;
        }

        all_face_count++;
    }

    // Print maximum score face (only if score > threshold)
    if (max_score_idx >= 0 && max_score > kScoreThreshold) {
        MicroPrintf("========== Max Score Face ==========\r\n");
        MicroPrintf("Index: %d, Score: %.3f (%.1f%%)\r\n", max_score_idx, max_score, max_score * 100.0f);
        MicroPrintf("Position: x=%.2f, y=%.2f, w=%.2f, h=%.2f\r\n", max_x, max_y, max_w, max_h);
        MicroPrintf("===================================\r\n");
        Box faces[1];
        faces[0].x1    = (int)max_x;
        faces[0].y1    = (int)max_y;
        faces[0].x2    = (int)(max_x + max_w);
        faces[0].y2    = (int)(max_y + max_h);
        faces[0].score = max_score;
        box_detection_path_build(faces, 1, 1, 90, width, height, 1080, 1920);
    } else {
        MicroPrintf("No faces detected above threshold (%.2f)\r\n", kScoreThreshold);
        if (max_score > 0.0f) {
            MicroPrintf("Highest score found: %.3f (below threshold)\r\n", max_score);
        }
    }

    return all_face_count;
}
