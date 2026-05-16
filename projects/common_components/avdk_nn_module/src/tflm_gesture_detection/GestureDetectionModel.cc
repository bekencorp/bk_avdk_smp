#include "os/mem.h"
#include "os/str.h"
#include "os/os.h"

#include "GestureDetectionModel.h"
#include "gesture_detection_model_data.h"
#include "box.h"

static const char* TAG = "ges-model";

#define LOGI(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE((char*)TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD((char*)TAG, ##__VA_ARGS__)
#define LOGV(...)

void GestureDetectionModel::resolverLoad(void)
{
    micro_op_resolver.AddEthosU();
    micro_op_resolver.AddPadV2();
    micro_op_resolver.AddTranspose();
    micro_op_resolver.AddQuantize();
    micro_op_resolver.AddDequantize();
}


void GestureDetectionModel::resourceLoad(void)
{
    name = "gestureDetection";
    width = 192;
    height = 192;
    format = BK_PIXEL_FORMAT_RGB888;
    model_type = AVDK_NN_MODEL_TYPE_NPU;
    model_ram_type = AVDK_NN_MEM_TYPE_PSRAM_SLAB;
    model_flash_data = (uint8_t*)gesture_detection_tflite;
    model_flash_data_size = gesture_detection_tflite_size;

    fast_ram_type = AVDK_NN_MEM_TYPE_HSRAM;
    fast_ram_data_size = 128 * 1024;
    fast_ram_data = NULL;

    arena_data_size = 600 * 1024;
    arena_ram_type = AVDK_NN_MEM_TYPE_PSRAM_SLAB;
    arena_ram_data = NULL;
}

void GestureDetectionModel::resourceUnload(void)
{
    //TODO: Implement resource unload
}

uint8_t GestureDetectionModel::post_process(int8_t *out_data, uint8_t *result)
{
    int max_boxes_num = 128;
    int boxes_num = 0;
    Box *boxes = NULL;

    if(!result)
    {
        LOGI("result is NULL !!!\n");
        return -1;
    }

    /* Default to no valid gesture detected. */
    *result = GESTURE_NONE;

    boxes = (Box *)hsram_malloc(max_boxes_num * sizeof(Box));

    if(boxes == NULL)
    {
        LOGI("malloc boxes failed\r\n");
        return 0;
    }

    //int x1, y1, x2, y2;
    for(int i = 0; i < 2268; i++)
    {

        float score = (out_data[i*8 + 4] - g_zero_point)*g_scale;

        if(score > 30)
        {
            if(boxes_num >= max_boxes_num){
                break;
            }

            int x1, y1, x2, y2;

            int x = (out_data[i*8 + 0] - g_zero_point)*g_scale;
            int y = (out_data[i*8 + 1] - g_zero_point)*g_scale;
            int w = (out_data[i*8 + 2] - g_zero_point)*g_scale;
            int h = (out_data[i*8 + 3] - g_zero_point)*g_scale;
            float paper = (out_data[i*8 + 5] - g_zero_point)*g_scale;
            float rock = (out_data[i*8 + 6] - g_zero_point)*g_scale;
            float scissors = (out_data[i*8 + 7] - g_zero_point)*g_scale;

            x1 = (x - w/2);
            x2 = (x + w/2);
            y1 = (y - h/2);
            y2 = (y + h/2);
            if(x1 < 0) x1 = 0;
            if(y1 < 0) y1 = 0;
            if(x2 > 191) x2 = 191;
            if(y2 > 191) y2 = 191;

            /* x1/y1/x2/y2 are post-clamp INT coordinates (snapped into [0, 191]),
             * so the source `w/h` from above is no longer valid here; recompute
             * the clamped width/height from the clamped corners. The implicit
             * int->float conversion is fine because Box.{x,y,w,h} are float and
             * these clamped values fit losslessly in a float mantissa. */
            boxes[boxes_num].x = (float)x1;
            boxes[boxes_num].y = (float)y1;
            boxes[boxes_num].w = (float)(x2 - x1);
            boxes[boxes_num].h = (float)(y2 - y1);
            boxes[boxes_num].score = score;
            boxes_num++;


            LOGI("score: %.2f, paper: %.2f,rock: %.2f,scissors: %.2f, x1: %d, y1: %d, x2: %d, y2: %d\r\n", score, paper,rock,scissors, x1, y1, x2, y2);

            if (paper > 90)
            {
                LOGI("\n\n\n\n\n\n======Paper is detected, x1:%d, y1:%d, x2:%d, y2:%d======\n\n\n\n\n\n", x1, y1, x2, y2);
                *result = GESTURE_PAPER;
            }
            else if (rock > 90)
            {
                LOGI("\n\n\n\n\n\n======Rock is detected, x1:%d, y1:%d, x2:%d, y2:%d======\n\n\n\n\n\n", x1, y1, x2, y2);
                *result = GESTURE_ROCK;
            }
            else if (scissors > 90)
            {
                LOGI("\n\n\n\n\n\n======Scissors is detected, x1:%d, y1:%d, x2:%d, y2:%d======\n\n\n\n\n\n", x1, y1, x2, y2);
                *result = GESTURE_SCISSORS;
            }
            else
            {
                /* Score is high but class scores are not confident enough,
                 * treat as no valid gesture.
                 */
                *result = GESTURE_NONE;
            }

            break;
        }
    }

    if(boxes){
        free(boxes);
    }

    return boxes_num;
}


int GestureDetectionModel::run(uint8_t *data, uint32_t size, bk_pixel_format_t format)
{
    TfLiteTensor* input = pinterpreter->input(0);

    if (format == BK_PIXEL_FORMAT_BGRA8888)
    {
        for (int i = 0; i < width * height; i++)
        {
            input->data.int8[i * 3 + 0] = (int8_t)data[i * 4 + 2] - 128;
            input->data.int8[i * 3 + 1] = (int8_t)data[i * 4 + 1] - 128;
            input->data.int8[i * 3 + 2] = (int8_t)data[i * 4 + 0] - 128;
        }
    }
    else if (format == BK_PIXEL_FORMAT_RGB888)
    {
        for (int i = 0; i < width * height; i++)
        {
            input->data.int8[i * 3 + 0] = (int8_t)data[i * 3 + 0] - 128;
            input->data.int8[i * 3 + 1] = (int8_t)data[i * 3 + 1] - 128;
            input->data.int8[i * 3 + 2] = (int8_t)data[i * 3 + 2] - 128;
        }
    }
    else
    {
        LOGI("Invalid format: %d\r\n", format);
        return 0;
    }

    if(kTfLiteOk != pinterpreter->Invoke())
    {
        LOGI("Invoke failed\r\n");
        return 0;
    }

    TfLiteTensor* output = pinterpreter->output(0);

    output = pinterpreter->output(0);
    g_scale = output->params.scale;

    g_zero_point = output->params.zero_point;

    uint8_t result = 0;

    LOGV("g_scale: %.2f, g_zero_point: %d\n", g_scale, g_zero_point);

    post_process(output->data.int8, &result);

    /* Invoke per-instance callback if set.
     * The result is always a gesture_result_t value, including GESTURE_NONE
     * when no valid gesture is detected.
     */
    if (result_callback_ != nullptr)
    {
        result_callback_((gesture_result_t)result);
    }

    /* Invoke per-instance image callback if set.
     * Pass through the same buffer and bk_pixel_format_t as inference input.
     * Note: The image data pointer is valid only during callback execution.
     */
    if (image_callback_ != nullptr)
    {
        uint32_t image_data_size = 0;

        if (format == BK_PIXEL_FORMAT_BGRA8888)
        {
            image_data_size = width * height * 4;
        }
        else if (format == BK_PIXEL_FORMAT_RGB888)
        {
            image_data_size = width * height * 3;
        }

        if (size != image_data_size)
        {
            LOGI("Invalid input size: %u, expected: %u\r\n", (unsigned)size, (unsigned)image_data_size);
            return 0;
        }

        image_callback_(data, width, height, format, image_data_size);
    }

    return 1;
}