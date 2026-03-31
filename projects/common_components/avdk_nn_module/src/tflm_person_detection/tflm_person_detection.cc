#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/testing/micro_test.h"

constexpr int kNumCols = 96;
constexpr int kNumRows = 96;
constexpr int kNumChannels      = 1;
constexpr int kMaxImageSize     = kNumCols * kNumRows * kNumChannels;
constexpr int kCategoryCount    = 2;
constexpr int kPersonIndex      = 1;
constexpr int kNotAPersonIndex  = 0;

extern "C" int person_detection_init(void** handle, uint8_t* model, uint32_t tensor_arena_size);
extern "C" int person_detection_deinit(void* handle);
extern "C" int person_detection_run(void* handle, uint8_t* data);

class tflm_context
{
public:
    uint8_t* tensor_arena;
    tflite::MicroInterpreter* pinterpreter;
    tflite::MicroMutableOpResolver<5> micro_op_resolver;
};

int person_detection_init(void** handle, uint8_t* pmodel, uint32_t tensor_arena_size)
{
    int error = 0;

    const tflite::Model* model = ::tflite::GetModel(pmodel);

    if(TFLITE_SCHEMA_VERSION != model->version()) return -1;

    tflm_context* tflm = new tflm_context;

    if(!tflm)
    {
        error = -1;
        goto __error;
    }

    tflm->micro_op_resolver.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());
    tflm->micro_op_resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
    tflm->micro_op_resolver.AddDepthwiseConv2D(tflite::Register_DEPTHWISE_CONV_2D_INT8());
    tflm->micro_op_resolver.AddReshape();
    tflm->micro_op_resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());

    if(!(tflm->tensor_arena = (uint8_t*)new char[tensor_arena_size]))
    {
        error = -2;
        goto __error;
    }

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
    return 0;

__error:
    if(tflm)
    {
        if(tflm->tensor_arena) delete tflm->tensor_arena;
        if(tflm->pinterpreter) delete tflm->pinterpreter;
        delete tflm;
    }

    *handle = 0;
    return error;
}

int person_detection_deinit(void* handle)
{
    if(handle)
    {
        delete ((tflm_context*)handle)->pinterpreter;
        delete ((tflm_context*)handle)->tensor_arena;        
        delete (tflm_context*)handle;
    }

    return 0;
}

int person_detection_run(void* handle, uint8_t* data)
{
    tflite::MicroInterpreter* pinterpreter = ((tflm_context*)handle)->pinterpreter;

    TfLiteTensor* input = pinterpreter->input(0);

    memcpy(input->data.int8, data, input->bytes);

    if(kTfLiteOk != pinterpreter->Invoke()) return 0;

    TfLiteTensor* output = pinterpreter->output(0);
    //TF_LITE_MICRO_EXPECT_EQ(2, output->dims->size);
    //TF_LITE_MICRO_EXPECT_EQ(1, output->dims->data[0]);
    //TF_LITE_MICRO_EXPECT_EQ(kCategoryCount, output->dims->data[1]);
    //TF_LITE_MICRO_EXPECT_EQ(kTfLiteInt8, output->type);

    return (int)output->data.int8[kPersonIndex];//(*(int16_t*)output->data.int8);
}
