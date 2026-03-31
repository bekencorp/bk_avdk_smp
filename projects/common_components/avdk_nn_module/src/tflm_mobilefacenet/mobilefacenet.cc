#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/testing/micro_test.h"
#include "tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h"

#include <components/bk_frame_buffer.h>

#include <math.h>

#include "mobilefacenet.h"


extern "C" void *frame_buffer_uncoded_data_malloc(size_t size);
extern "C" void frame_buffer_mem_free(void *frame);

extern "C" void bk_printf(const char *fmt, ...);

extern "C" int rtos_delay_milliseconds(uint32_t num_ms);

#if 1
// ----------------------------
// Face feature match utilities
// 余弦相似度打分与判定（不分配内存、逐元素累加）
// ----------------------------

extern "C" int mobilefacenet_match_int8(
    const int8_t* feature_a,
    const int8_t* feature_b,
    uint16_t length,
    float /*scale_unused*/,
    int32_t zero_point,
    float threshold,
    float* out_score)
{
    if (!feature_a || !feature_b || length == 0) {
        if (out_score) *out_score = 0.0f;
        return 0;
    }

    // 说明：在两端量化参数一致时，scale 会在余弦相似度中相互抵消，
    // 仅需对去零点后的整数执行点积与范数累加，最后一次性归一化即可。
    int32_t dot = 0;
    int32_t norm_a = 0;
    int32_t norm_b = 0;
    const int16_t zp = (int16_t)zero_point;

    uint16_t i = 0;
    // 简单的四路展开以减小循环开销
    for (; i + 3 < length; i += 4) {
        int16_t a0 = (int16_t)feature_a[i + 0] - zp;
        int16_t b0 = (int16_t)feature_b[i + 0] - zp;
        int16_t a1 = (int16_t)feature_a[i + 1] - zp;
        int16_t b1 = (int16_t)feature_b[i + 1] - zp;
        int16_t a2 = (int16_t)feature_a[i + 2] - zp;
        int16_t b2 = (int16_t)feature_b[i + 2] - zp;
        int16_t a3 = (int16_t)feature_a[i + 3] - zp;
        int16_t b3 = (int16_t)feature_b[i + 3] - zp;

        dot    += (int32_t)a0 * b0 + (int32_t)a1 * b1 + (int32_t)a2 * b2 + (int32_t)a3 * b3;
        norm_a += (int32_t)a0 * a0 + (int32_t)a1 * a1 + (int32_t)a2 * a2 + (int32_t)a3 * a3;
        norm_b += (int32_t)b0 * b0 + (int32_t)b1 * b1 + (int32_t)b2 * b2 + (int32_t)b3 * b3;
    }
    for (; i < length; ++i) {
        int16_t a = (int16_t)feature_a[i] - zp;
        int16_t b = (int16_t)feature_b[i] - zp;
        dot    += (int32_t)a * b;
        norm_a += (int32_t)a * a;
        norm_b += (int32_t)b * b;
    }

    const float denom = (sqrtf((float)norm_a) * sqrtf((float)norm_b)) + 1e-12f;
    const float score = denom > 0.0f ? ((float)dot / denom) : 0.0f;
    if (out_score) *out_score = score;
    return score >= threshold ? 1 : 0;
}
#endif


#define FEAT_DIM 128

// -------------------------------
// 点积（纯C实现，无DSP）
// -------------------------------
static inline int32_t dot_product_int8(const int8_t *a, const int8_t *b, uint32_t dim)
{
    int32_t sum = 0;
    for (uint32_t i = 0; i < dim; i++) {
        sum += (int32_t)a[i] * (int32_t)b[i];
    }
    return sum;
}

// -------------------------------
// 向量平方范数（|a|^2）
// -------------------------------
static inline int32_t norm_sq_int8(const int8_t *a, uint32_t dim)
{
    int32_t sum = 0;
    for (uint32_t i = 0; i < dim; i++) {
        int32_t v = (int32_t)a[i];
        sum += v * v;
    }
    return sum;
}

// -------------------------------
// Cosine 相似度：返回 [-1.0, 1.0]
// -------------------------------
float cosine_similarity_int8(const int8_t *a, const int8_t *b, uint32_t dim)
{
    int32_t dot = dot_product_int8(a, b, dim);
    int32_t na  = norm_sq_int8(a, dim);
    int32_t nb  = norm_sq_int8(b, dim);

    if (na == 0 || nb == 0)
        return 0.0f;

    float denom = sqrtf((float)na * (float)nb);
    return (float)dot / denom;
}

// -------------------------------
// L2 距离
// -------------------------------
float l2_distance_int8(const int8_t *a, const int8_t *b, uint32_t dim)
{
    int64_t acc = 0;
    for (uint32_t i = 0; i < dim; i++) {
        int32_t d = (int32_t)a[i] - (int32_t)b[i];
        acc += (int64_t)d * d;
    }
    return sqrtf((float)acc);
}



class tflm_context
{
public:
    uint8_t* tensor_arena;
    tflite::MicroInterpreter* pinterpreter;
    tflite::MicroMutableOpResolver<128> micro_op_resolver;
};

static int8_t face_01[128];
static int8_t face_01_enable = 0;

int mobilefacenet_init(void** handle, uint8_t* pmodel, uint32_t tensor_arena_size)
{
    int error = 0;

    tflm_context* tflm = new tflm_context;
    const tflite::Model* model = ::tflite::GetModel(pmodel);

    if(TFLITE_SCHEMA_VERSION != model->version()) 
    {
        error = -1;
        MicroPrintf("model version error: %d != %d\r\n", TFLITE_SCHEMA_VERSION, model->version());
        goto __error;
    }

    MicroPrintf("mobilefacenet_init: version=%d\r\n", model->version());
    rtos_delay_milliseconds(1000);
 
    if(!tflm)
    {
        error = -1;
        goto __error;
    }

    // 基础操作
    tflm->micro_op_resolver.AddAdd();
    tflm->micro_op_resolver.AddMul();
    tflm->micro_op_resolver.AddSub();
    tflm->micro_op_resolver.AddMinimum();
    tflm->micro_op_resolver.AddMaximum();

    // 卷积操作 - 使用INT8版本
    tflm->micro_op_resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
    tflm->micro_op_resolver.AddDepthwiseConv2D(tflite::Register_DEPTHWISE_CONV_2D_INT8());

    // 池化操作
    tflm->micro_op_resolver.AddMaxPool2D();
    tflm->micro_op_resolver.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());

    // 全连接层
    tflm->micro_op_resolver.AddFullyConnected(tflite::Register_FULLY_CONNECTED_INT8());

    // 激活函数
    tflm->micro_op_resolver.AddLogistic();
    tflm->micro_op_resolver.AddRelu();
    tflm->micro_op_resolver.AddRelu6();

    // 形状操作
    tflm->micro_op_resolver.AddReshape();
    tflm->micro_op_resolver.AddTranspose();
    tflm->micro_op_resolver.AddConcatenation();
    tflm->micro_op_resolver.AddSplitV();

    // 填充操作
    tflm->micro_op_resolver.AddPad();
    tflm->micro_op_resolver.AddPadV2();

    // 量化操作
    tflm->micro_op_resolver.AddQuantize();
    tflm->micro_op_resolver.AddDequantize();

    // 其他操作
    tflm->micro_op_resolver.AddResizeNearestNeighbor();
    tflm->micro_op_resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());

    // 更多可能需要的操作
    tflm->micro_op_resolver.AddShape();
    tflm->micro_op_resolver.AddStridedSlice();
    tflm->micro_op_resolver.AddPack();
    tflm->micro_op_resolver.AddPrelu();

    if(!(tflm->tensor_arena = (uint8_t*)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, tensor_arena_size)))
    {
        error = -2;
        MicroPrintf("malloc tensor_arena failed\r\n");
        goto __error;
    }

    if(!(tflm->pinterpreter = new tflite::MicroInterpreter(model, tflm->micro_op_resolver, tflm->tensor_arena, tensor_arena_size)))
    {
        error = -3;
        MicroPrintf("new MicroInterpreter failed\r\n");
        goto __error;
    }

    MicroPrintf("AllocateTensors start\r\n");
    rtos_delay_milliseconds(1000);

    if(kTfLiteOk != tflm->pinterpreter->AllocateTensors())
    {
        error = -4;
        MicroPrintf("AllocateTensors failed\r\n");
        goto __error;
    }
    else
    {
        //TODO
    }

    *handle = tflm;
    return 0;

__error:

    if(tflm)
    {
        if(tflm->tensor_arena) bk_frame_buffer_free(tflm->tensor_arena);
        if(tflm->pinterpreter) delete tflm->pinterpreter;
        delete tflm;
    }

    *handle = 0;
    return error;
}

int mobilefacenet_deinit(void* handle)
{
    if(handle)
    {
        bk_frame_buffer_free(((tflm_context*)handle)->pinterpreter);
        delete ((tflm_context*)handle)->tensor_arena;
        delete (tflm_context*)handle;
    }

    return 0;
}

int mobilefacenet_run(void* handle, int8_t* data, uint16_t width, uint16_t height, uint16_t channels)
{
    MicroPrintf("mobilefacenet_run\r\n");

    if(!handle)
    {
        MicroPrintf("handle is null error return\r\n");
        return -1;
    };


    tflite::MicroInterpreter* pinterpreter = ((tflm_context*)handle)->pinterpreter;

    TfLiteTensor* input = pinterpreter->input(0);

    // 调试：打印输入张量信息
    //MicroPrintf("Input tensor: bytes=%d, type=%d, dims=%d", 
    //            input->bytes, input->type, input->dims->size);
    //for(int i = 0; i < input->dims->size; i++) {
    //    MicroPrintf("  dim[%d]=%d", i, input->dims->data[i]);
    //}

    // 安全拷贝，避免越界
    uint32_t copy_len = width * height * channels;
    if (copy_len > (uint32_t)input->bytes) copy_len = (uint32_t)input->bytes;
    
    // 调试：打印输入数据前几个字节
    //bk_printf("Input data[0-3]: %x %x %x %x\n", 
    //           data[0], data[1], data[2], data[3]);
    //bk_printf("Copy len: %d, expected: %dx%dx%d=%d\n", 
    //            copy_len, width, height, channels, width*height*channels);
    
    memcpy(input->data.int8, data, copy_len);
    
    // 调试：验证拷贝后的数据
    //bk_printf("After copy[0-3]: %x %x %x %x\n", 
    //            input->data.int8[0], input->data.int8[1], 
     //          input->data.int8[2],input->data.int8[3]);
    if(kTfLiteOk != pinterpreter->Invoke()) return 0;

    TfLiteTensor* output = pinterpreter->output(0);
    int8_t *result = &output->data.int8[0];
    // 打印前4字节，便于观测
    bk_printf("mobilefacenet: %d %d %d %d\n", result[0], result[1], result[2], result[3]);

#if 1
    // 使用静态注册模板 face_01 进行比对判定
    // 从输出张量拿量化参数（若为INT8量化）
    float out_scale = 1.0f;
    int32_t out_zp = 0;
    if (output) {
        out_scale = output->params.scale;
        out_zp = output->params.zero_point;
    }
#endif

    // 取比较长度，通常为128；以张量长度与模板长度较小者为准
    uint16_t feat_len = (uint16_t)output->bytes;
    if (feat_len > 128) feat_len = 128;

    if(face_01_enable == 0)
    {
        memcpy(face_01, output->data.int8, feat_len);
        face_01_enable = 1;
        MicroPrintf("face_01_enable = 1\n");
        return 0;
    }

#if 1
    float score = 0.0f;
    const float threshold = 0.60f; // 可按效果微调
    int matched = mobilefacenet_match_int8(
        (const int8_t*)output->data.int8,
        (const int8_t*)face_01,
        feat_len,
        out_scale,
        out_zp,
        threshold,
        &score);

        bk_printf("facenet compare: score=%.4f, threshold=%.2f, %s\n",
              score, threshold, matched ? "MATCH" : "NO_MATCH");

#else
    float cos_sim = cosine_similarity_int8(output->data.int8, face_01, FEAT_DIM);
    float l2 = l2_distance_int8(output->data.int8, face_01, FEAT_DIM);

    printf("cosine=%.6f  l2=%.6f\n", cos_sim, l2);
#endif


    return 0;//(*(int16_t*)output->data.int8);
}


