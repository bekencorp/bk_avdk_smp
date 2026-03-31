#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>


#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cfacedetectcnn.h"
#include "cfacedetectcnn.mdat"

#include <components/frame_buffer.h>

#pragma GCC optimize ("O3")

#ifndef MIN
#define MIN(a,b)            ((a) > (b) ? (b) : (a))
#endif
#ifndef MAX
#define MAX(a,b)            ((a) < (b) ? (b) : (a))
#endif

#define ASSERT(cond)        //if(!(cond)) printf("[ASSERT]: failed @ %s:%d @ function %s\n", __FILE__, __LINE__, __func__)
#define FILTERS_COUNT       (sizeof(param_conv_info) / sizeof(ConvInfoStruct))

#if 1
#define port_malloc frame_buffer_coded_data_malloc
#define port_free   frame_buffer_coded_data_free
#else
void* port_malloc(int size);
void  port_free(void* buff);
#endif

typedef struct _ConvInfoStruct
{
    int16_t channels;
    int16_t num_filters;
    int8_t  is_depthwise;
    int8_t  is_pointwise;
    int8_t  with_relu;
    const float* pWeights;
    const float* pBiases;
}ConvInfoStruct;

typedef struct _DataBlobBase
{
    int16_t  rows;
    int16_t  cols;
    int16_t  channels;
    int16_t  stride;
    int16_t  typesize;
    int16_t  maxrows;
    int16_t  maxcols;
    int16_t  maxchannels;
    void*    data;
}DataBlobBase, *pDataBlobBase;

typedef struct _DataBlobUint8
{
    int16_t  rows;
    int16_t  cols;
    int16_t  channels;
    int16_t  stride;
    int16_t  typesize;
    int16_t  maxrows;
    int16_t  maxcols;
    int16_t  maxchannels;
    uint8_t* data;
}DataBlobUint8, *pDataBlobUint8;

typedef struct _DataBlob
{
    int16_t rows;
    int16_t cols;
    int16_t channels;
    int16_t stride;
    int16_t typesize;
    int16_t maxrows;
    int16_t maxcols;
    int16_t maxchannels;
    float*   data;
}DataBlobType, *pDataBlobType;

typedef struct _Filters
{
    int16_t  channels;
    int16_t  num_filters;
    int8_t   is_depthwise;
    int8_t   is_pointwise;
    int8_t   with_relu;
    int8_t   reserved;
    DataBlobType weights;
    DataBlobType biases;
}Filter;

//(in_channels, out_channels, is_depthwise, is_pointwise, with_bn, weight_ptr, bias_ptr)
static const ConvInfoStruct param_conv_info[] =
{
	{ 32, 16, 0, 1, 1, backbone__model0_pw_weight,               backbone__model0_pw_bias               },
	{ 16, 16, 0, 1, 0, backbone__model0_dp_pw_weight,            backbone__model0_dp_pw_bias            },
	{ 16, 16, 1, 0, 1, backbone__model0_dp_dw_weight,            backbone__model0_dp_dw_bias            },
	{ 16, 16, 0, 1, 0, backbone__model1_dp1_pw_weight,           backbone__model1_dp1_pw_bias           },
	{ 16, 16, 1, 0, 1, backbone__model1_dp1_dw_weight,           backbone__model1_dp1_dw_bias           },
	{ 16, 32, 0, 1, 0, backbone__model1_dp2_pw_weight,           backbone__model1_dp2_pw_bias           },
	{ 32, 32, 1, 0, 1, backbone__model1_dp2_dw_weight,           backbone__model1_dp2_dw_bias           },
	{ 32, 32, 0, 1, 0, backbone__model2_dp1_pw_weight,           backbone__model2_dp1_pw_bias           },
	{ 32, 32, 1, 0, 1, backbone__model2_dp1_dw_weight,           backbone__model2_dp1_dw_bias           },
	{ 32, 64, 0, 1, 0, backbone__model2_dp2_pw_weight,           backbone__model2_dp2_pw_bias           },
	{ 64, 64, 1, 0, 1, backbone__model2_dp2_dw_weight,           backbone__model2_dp2_dw_bias           },
	{ 64, 64, 0, 1, 0, backbone__model3_dp1_pw_weight,           backbone__model3_dp1_pw_bias           },
	{ 64, 64, 1, 0, 1, backbone__model3_dp1_dw_weight,           backbone__model3_dp1_dw_bias           },
	{ 64, 64, 0, 1, 0, backbone__model3_dp2_pw_weight,           backbone__model3_dp2_pw_bias           },
	{ 64, 64, 1, 0, 1, backbone__model3_dp2_dw_weight,           backbone__model3_dp2_dw_bias           },
	{ 64, 64, 0, 1, 0, backbone__model4_dp1_pw_weight,           backbone__model4_dp1_pw_bias           },
	{ 64, 64, 1, 0, 1, backbone__model4_dp1_dw_weight,           backbone__model4_dp1_dw_bias           },
	{ 64, 64, 0, 1, 0, backbone__model4_dp2_pw_weight,           backbone__model4_dp2_pw_bias           },
	{ 64, 64, 1, 0, 1, backbone__model4_dp2_dw_weight,           backbone__model4_dp2_dw_bias           },
	{ 64, 64, 0, 1, 0, backbone__model5_dp1_pw_weight,           backbone__model5_dp1_pw_bias           },
	{ 64, 64, 1, 0, 1, backbone__model5_dp1_dw_weight,           backbone__model5_dp1_dw_bias           },
	{ 64, 64, 0, 1, 0, backbone__model5_dp2_pw_weight,           backbone__model5_dp2_pw_bias           },
	{ 64, 64, 1, 0, 1, backbone__model5_dp2_dw_weight,           backbone__model5_dp2_dw_bias           },
	{ 64, 64, 0, 1, 0, neck__lateral_convs__0_pw_weight,         neck__lateral_convs__0_pw_bias         },
	{ 64, 64, 1, 0, 1, neck__lateral_convs__0_dw_weight,         neck__lateral_convs__0_dw_bias         },
	{ 64, 64, 0, 1, 0, neck__lateral_convs__1_pw_weight,         neck__lateral_convs__1_pw_bias         },
	{ 64, 64, 1, 0, 1, neck__lateral_convs__1_dw_weight,         neck__lateral_convs__1_dw_bias         },
	{ 64, 64, 0, 1, 0, neck__lateral_convs__2_pw_weight,         neck__lateral_convs__2_pw_bias         },
	{ 64, 64, 1, 0, 1, neck__lateral_convs__2_dw_weight,         neck__lateral_convs__2_dw_bias         },
	{ 64,  1, 0, 1, 0, bbox_head__multi_level_cls__0_pw_weight,  bbox_head__multi_level_cls__0_pw_bias  },
	{  1,  1, 1, 0, 0, bbox_head__multi_level_cls__0_dw_weight,  bbox_head__multi_level_cls__0_dw_bias  },
	{ 64,  1, 0, 1, 0, bbox_head__multi_level_cls__1_pw_weight,  bbox_head__multi_level_cls__1_pw_bias  },
	{  1,  1, 1, 0, 0, bbox_head__multi_level_cls__1_dw_weight,  bbox_head__multi_level_cls__1_dw_bias  },
	{ 64,  1, 0, 1, 0, bbox_head__multi_level_cls__2_pw_weight,  bbox_head__multi_level_cls__2_pw_bias  },
	{  1,  1, 1, 0, 0, bbox_head__multi_level_cls__2_dw_weight,  bbox_head__multi_level_cls__2_dw_bias  },
	{ 64,  4, 0, 1, 0, bbox_head__multi_level_bbox__0_pw_weight, bbox_head__multi_level_bbox__0_pw_bias },
	{  4,  4, 1, 0, 0, bbox_head__multi_level_bbox__0_dw_weight, bbox_head__multi_level_bbox__0_dw_bias },
	{ 64,  4, 0, 1, 0, bbox_head__multi_level_bbox__1_pw_weight, bbox_head__multi_level_bbox__1_pw_bias },
	{  4,  4, 1, 0, 0, bbox_head__multi_level_bbox__1_dw_weight, bbox_head__multi_level_bbox__1_dw_bias },
	{ 64,  4, 0, 1, 0, bbox_head__multi_level_bbox__2_pw_weight, bbox_head__multi_level_bbox__2_pw_bias },
	{  4,  4, 1, 0, 0, bbox_head__multi_level_bbox__2_dw_weight, bbox_head__multi_level_bbox__2_dw_bias },
	{ 64,  1, 0, 1, 0, bbox_head__multi_level_obj__0_pw_weight,  bbox_head__multi_level_obj__0_pw_bias  },
	{  1,  1, 1, 0, 0, bbox_head__multi_level_obj__0_dw_weight,  bbox_head__multi_level_obj__0_dw_bias  },
	{ 64,  1, 0, 1, 0, bbox_head__multi_level_obj__1_pw_weight,  bbox_head__multi_level_obj__1_pw_bias  },
	{  1,  1, 1, 0, 0, bbox_head__multi_level_obj__1_dw_weight,  bbox_head__multi_level_obj__1_dw_bias  },
	{ 64,  1, 0, 1, 0, bbox_head__multi_level_obj__2_pw_weight,  bbox_head__multi_level_obj__2_pw_bias  },
	{  1,  1, 1, 0, 0, bbox_head__multi_level_obj__2_dw_weight,  bbox_head__multi_level_obj__2_dw_bias  },
	{ 64, 10, 0, 1, 0, bbox_head__multi_level_kps__0_pw_weight,  bbox_head__multi_level_kps__0_pw_bias  },
	{ 10, 10, 1, 0, 0, bbox_head__multi_level_kps__0_dw_weight,  bbox_head__multi_level_kps__0_dw_bias  },
	{ 64, 10, 0, 1, 0, bbox_head__multi_level_kps__1_pw_weight,  bbox_head__multi_level_kps__1_pw_bias  },
	{ 10, 10, 1, 0, 0, bbox_head__multi_level_kps__1_dw_weight,  bbox_head__multi_level_kps__1_dw_bias  },
	{ 64, 10, 0, 1, 0, bbox_head__multi_level_kps__2_pw_weight,  bbox_head__multi_level_kps__2_pw_bias  },
	{ 10, 10, 1, 0, 0, bbox_head__multi_level_kps__2_dw_weight,  bbox_head__multi_level_kps__2_dw_bias  }
};

static float dot_product(float* in1, float* in2, int num)
{
    float sum = 0;

    int i; for(i = 0; i < num; i++) sum += *in1++ * *in2++;

    return sum;
}

static float dot_product_uint8(uint8_t* in1, float* in2, int num)
{
    float sum = 0;

    int i; for(i = 0; i < num; i++) sum += *in1++ * *in2++;

    return sum;
}

static void vector_mula(float* out, float* in1, float* in2, int num)
{
    int i; for(i = 0; i < num; i++) *out++ += *in1++ * *in2++;
}

static void vector_add3(float* out, float* in, int num)
{
    int i; for(i = 0; i < num; i++) *out++ += *in++;
}

static DataBlobBase* data_blob_base_create(int rows, int cols, int channels, int typesize)
{
    DataBlobBase* db;

    //if((db = (DataBlobBase*)port_malloc(sizeof(DataBlobUint8) + rows * cols * channels * typesize)))
    if((db = (DataBlobBase*)malloc(sizeof(DataBlobUint8))))
    {
        db->rows        = rows;
        db->cols        = cols;
        db->channels    = channels;
        db->stride      = channels;
        db->typesize    = typesize;
        db->data        = port_malloc(rows * cols * channels * typesize);//(uint8_t*)&db->data + sizeof(db->data);
        db->maxrows     = rows;
        db->maxcols     = cols;
        db->maxchannels = channels;
        memset(db->data, 0, rows * cols * channels * typesize);
    }

    return db;
}

static DataBlobType* data_blob_create(int rows, int cols, int channels)
{
    return (DataBlobType*)data_blob_base_create(rows, cols, channels, sizeof(float));
}

static void data_blob_destory(DataBlobType* db)
{
    port_free(db->data);
    free(db);
}

static void data_blob_init(DataBlobType* db, int rows, int cols, int channels, float* data)
{
    db->rows        = rows;
    db->cols        = cols;
    db->channels    = channels;
    db->stride      = channels;
    db->data        = data;
    db->typesize    = sizeof(float);
    db->maxrows     = 0;
    db->maxcols     = 0;
    db->maxchannels = 0;
}

static float* data_blob_data(DataBlobBase* db, int row, int col)
{
    return db->data + (row * db->cols + col) * db->stride * db->typesize;
}

static void data_blob_add2(DataBlobType* in, DataBlobType* inout)
{
    int r, c;
    int rows = inout->rows;
    int cols = inout->cols;
    int channels = inout->channels;

    ASSERT(in->rows * 2 == inout->rows && in->cols * 2 == inout->cols && in->channels == inout->channels);

    for(r = 0; r < rows; r++)
    {
        for(c = 0; c < cols; c++)
        {
            vector_add3(data_blob_data((DataBlobBase*)inout, r, c), data_blob_data((DataBlobBase*)in, r/2, c/2), channels);
        }
    }
}

static void filter_init(Filter* filter, ConvInfoStruct* cis)
{
    filter->channels     = cis->channels;
    filter->num_filters  = cis->num_filters;
    filter->is_depthwise = cis->is_depthwise;
    filter->is_pointwise = cis->is_pointwise;
    filter->with_relu    = cis->with_relu;

    if(!filter->is_depthwise && filter->is_pointwise) //1x1 point wise
    {
        data_blob_init(&filter->weights, 1, filter->num_filters, filter->channels, (float*)cis->pWeights);
    }
    else if(filter->is_depthwise && !filter->is_pointwise) //3x3 depth wise
    {
        data_blob_init(&filter->weights, 1, 9, filter->channels, (float*)cis->pWeights);
    }
    else
    {
        //bk_printf("Unsupported filter type. Only 1x1 point-wise and 3x3 depth-wise are supported.\n");
    }

    data_blob_init(&filter->biases, 1, 1, filter->num_filters, (float*)cis->pBiases);
}

static void convolution_1x1pointwise(DataBlobType* in, DataBlobType* out, Filter* filter)
{
    int r, c, ch;
    int rows = out->rows;
    int cols = out->cols;
    int ichannels = in->channels;
    int ochannels = filter->num_filters;

    ASSERT(in != out);

#if 0
    //131ms
    for(r = 0; r < rows; r++)
    {
        for(c = 0; c < cols; c++)
        {
            float* pin   = data_blob_data((DataBlobBase*)in, r, c);
            float* pout  = data_blob_data((DataBlobBase*)out, r, c);
            float* pbias = filter->biases.data;
            float* pweight;

            for(ch = 0; ch < ochannels; ch++)
            {
                pweight = data_blob_data((DataBlobBase*)&filter->weights, 0, ch);
                *pout++ = dot_product(pin, pweight, ichannels) + *pbias++;
            }
        }
    }
#else
    //127ms
    DataBlobBase *out_b = (DataBlobBase*)out;
    DataBlobBase *in_b = (DataBlobBase*)in;

    float* pout = out_b->data;
    float* pin = in_b->data;
    float* pbias = filter->biases.data;

    float *pweight[64] = {0};
    for(ch = 0; ch < ochannels; ch++)
    {
        pweight[ch] = data_blob_data((DataBlobBase*)&filter->weights, 0, ch);
    }

    for(r = 0; r < rows; r++)
    {
        for(c = 0; c < cols; c++)
        {
            for(ch = 0; ch < ochannels; ch++)
            {
                *pout++ = dot_product(pin, pweight[ch], ichannels) + pbias[ch];
            }
            pin += in->stride;
        }
    }
#endif
}

static void convolution_1x1pointwise_uint8(DataBlobUint8* in, DataBlobType* out, Filter* filter)
{
    int r, c, ch;
    int rows = out->rows;
    int cols = out->cols;
    int ichannels = in->channels;
    int ochannels = filter->num_filters;

    ASSERT(in != out);

#if 0
    //110ms
    for(r = 0; r < rows; r++)
    {
        for(c = 0; c < cols; c++)
        {
            float  value;
            float* pweight;
            float* pbias = filter->biases.data;
            float* pout  = data_blob_data((DataBlobBase*)out, r, c);
            uint8_t* pin = (uint8_t*)data_blob_data((DataBlobBase*)in, r, c);

            for(ch = 0; ch < ochannels; ch++)
            {
                pweight = data_blob_data((DataBlobBase*)&filter->weights, 0, ch);
                value   = dot_product_uint8(pin, pweight, ichannels) + *pbias++;
                *pout++ = value > 0 ? value : 0;
            }
        }
    }
#else
    //108ms
    DataBlobBase *out_b = (DataBlobBase*)out;
    DataBlobBase *in_b = (DataBlobBase*)in;

    float* pout = out_b->data;
    uint8_t* pin = (uint8_t *)in_b->data;
    float* pbias = filter->biases.data;

    float *pweight[64] = {0};
    for(ch = 0; ch < ochannels; ch++)
    {
        pweight[ch] = data_blob_data((DataBlobBase*)&filter->weights, 0, ch);
    }

    for(r = 0; r < rows; r++)
    {
        for(c = 0; c < cols; c++)
        {
            float  value;
            for(ch = 0; ch < ochannels; ch++)
            {
                value   = dot_product_uint8(pin, pweight[ch], ichannels) + pbias[ch];
                pout[ch] = value > 0 ? value : 0;
            }
            pout += out->stride;
            pin += in->stride;
        }
    }
#endif
}

static void convolution_3x3depthwise(DataBlobType* in, DataBlobType* out, Filter* filter)
{
    int r, r2, c, c2;
    int rows = out->rows;
    int cols = out->cols;
    int num_filters = filter->num_filters;

    float* pbias = filter->biases.data;

    ASSERT(in != out);
    ASSERT(in->channels == filter->num_filters);

    for(r = 0; r < rows; r++) 
    {
        int srcy_start = MAX(0, r - 1);
        int srcy_end   = MIN(r + 2, rows);

        for(c = 0; c < cols; c++)
        {
            int srcx_start = MAX(0, c - 1);
            int srcx_end   = MIN(c + 2, cols);

            float* pout = data_blob_data((DataBlobBase*)out, r, c);

            for(r2 = srcy_start; r2 < srcy_end; r2++)
            {
                for(c2 = srcx_start; c2 < srcx_end; c2++)
                {
                    vector_mula(pout, data_blob_data((DataBlobBase*)in, r2, c2), data_blob_data((DataBlobBase*)&filter->weights, 0, (r2 - r + 1) * 3 + c2 - c + 1), num_filters);
                }
            }

            vector_add3(pout, pbias, num_filters);
        }
    }
}

static void relu(DataBlobType* db)
{
    int r, c, ch;
    int rows = db->rows;
    int cols = db->cols;
    int channels = db->channels;

    for(r = 0; r < rows; r++)
    {
        for(c = 0; c < cols; c++)
        {
            float* pout = data_blob_data((DataBlobBase*)db, r, c);

            for(ch = 0; ch < channels; ch++)
            {
                if(pout[ch] < 0) pout[ch] = 0;
            }
        }
    }
}

static float jaccard_overlap(FaceInfo* face1, FaceInfo* face2)
{
    FaceInfo intersect_face;

    float intersect_width, intersect_height;

    if(face2->xmin > face1->xmax || face2->xmax < face1->xmin || face2->ymin > face1->ymax || face2->ymax < face1->ymin) 
    {
        intersect_face.xmin = 0;
        intersect_face.ymin = 0;
        intersect_face.xmax = 0;
        intersect_face.ymax = 0;
    }
    else
    {
        intersect_face.xmin = MAX(face1->xmin, face2->xmin);
        intersect_face.ymin = MAX(face1->ymin, face2->ymin);
        intersect_face.xmax = MIN(face1->xmax, face2->xmax);
        intersect_face.ymax = MIN(face1->ymax, face2->ymax);
    }

    intersect_width  = intersect_face.xmax - intersect_face.xmin;
    intersect_height = intersect_face.ymax - intersect_face.ymin;

    if(intersect_width > 0 && intersect_height > 0) 
    {
        float intersect_size = intersect_width * intersect_height;
        float bsize1 = (face1->xmax - face1->xmin) * (face1->ymax - face1->ymin);
        float bsize2 = (face2->xmax - face2->xmin) * (face2->ymax - face2->ymin);
        return intersect_size / ( bsize1 + bsize2 - intersect_size);
    }
    else
    {
        return 0.f;
    }
}

static void convolution(DataBlobType* in, DataBlobType** out, Filter* filter, uint32_t do_relu)
{
    int fflag = in == *out;

    *out = data_blob_create(in->rows, in->cols, filter->num_filters);

    if(filter->is_pointwise && !filter->is_depthwise)
    {
        convolution_1x1pointwise(in, *out, filter);
    }
    else if(!filter->is_pointwise && filter->is_depthwise)
    {
        convolution_3x3depthwise(in, *out, filter);
    }
    else
    {
        ASSERT(0);
        //bk_printf("Unsupported filter type. Only 1x1 point-wise and 3x3 depth-wise are supported.\n");
    }

    if(do_relu) relu(*out);
    if(fflag)   data_blob_destory(in);
}

static void convolution2(DataBlobType* in, DataBlobType** out, Filter* filter1, Filter* filter2, uint32_t do_relu)
{
    convolution(in, out, filter1, 0);
    convolution(*out, out, filter2, do_relu);
}

static void convolution4(DataBlobType* in, DataBlobType** out, Filter* filter1, Filter* filter2, Filter* filter3, Filter* filter4, uint32_t do_relu)
{
    convolution2(in, out, filter1, filter2, 1);
    convolution2(*out, out, filter3, filter4, do_relu);
}

static void convolution8(DataBlobUint8* in, DataBlobType** out, Filter* filter, uint32_t do_relu)
{
    *out = data_blob_create(in->rows, in->cols, filter->num_filters);

    convolution_1x1pointwise_uint8(in, *out, filter);
}

static void maxpooling2x2S2(DataBlobType* in, DataBlobType** out)
{
    int r, c, ch;
    int irows = in->rows;
    int icols = in->cols;
    int rows  = irows / 2;
    int cols  = icols / 2;
    int stride= in->stride;
    int channels = in->channels;

    if(in == *out)
    {
        in->rows = rows;
        in->cols = cols;
    }
    else
    {
        *out = data_blob_create(rows, cols, channels);
    }

    for(r = 0; r < rows; r++)
    {
        for(c = 0; c < cols; c++)
        {
            int fr, fc;
            int elementCount = 0;
            int inputMatOffsetsInElement[4];
            int rstart = r * 2;
            int cstart = c * 2;
            int rend = MIN(rstart + 2, irows);
            int cend = MIN(cstart + 2, icols);

            float* pin  = in->data;
            float* pout = data_blob_data((DataBlobBase*)*out, r, c);

            for(fr = rstart; fr < rend; fr++)
            {
                for(fc = cstart; fc < cend; fc++)
                {
                    inputMatOffsetsInElement[elementCount++] = (fr * icols + fc) * stride;
                }
            }

            for(ch = 0; ch < channels; ch++)
            {
                int  ec;
                float maxVal = pin[ch + inputMatOffsetsInElement[0]];

                for(ec = 1; ec < elementCount; ec++)
                {
                    maxVal = MAX(maxVal, pin[ch + inputMatOffsetsInElement[ec]]);
                }

                pout[ch] = maxVal;
            }
        }
    }
}

static DataBlobType* meshgrid(int feature_width, int feature_height, int stride)
{
    int r, c;

    DataBlobType* out = data_blob_create(feature_height, feature_width, 2);

    float* p = out->data;

    for(r = 0; r < feature_height; ++r)
    {
        float rx = r * stride;

        for(c = 0; c < feature_width; ++c)
        {
            *p++ = c * stride;
            *p++ = rx;
        }
    }

    return out;
}

static void bbox_decode(DataBlobType* bbox_pred, DataBlobType* priors, int stride)
{
    int r, c;
    int rows = bbox_pred->rows;
    int cols = bbox_pred->cols;

    float* pb = bbox_pred->data;
    float* pp = priors->data;

    ASSERT(priors->channels == priors->stride);
    ASSERT(bbox_pred->channels == bbox_pred->stride);
    ASSERT(bbox_pred->rows == priors->rows && bbox_pred->cols == priors->cols);

    for(r = 0; r < rows; ++r)
    {
        for(c = 0; c < cols; ++c)
        {
            float x = pb[0] * stride + *pp++;
            float y = pb[1] * stride + *pp++;
            float w = (float)exp(pb[2]) * stride;
            float h = (float)exp(pb[3]) * stride;

            *pb++ = (float)(x - w / 2.f);
            *pb++ = (float)(y - h / 2.f);
            *pb++ = (float)(x + w / 2.f);
            *pb++ = (float)(y + h / 2.f);
        }
    }
}

static void kps_decode(DataBlobType* kps_pred, DataBlobType* priors, int stride)
{
    int r, c, n;
    int rows = kps_pred->rows;
    int cols = kps_pred->cols;
    int num_points = kps_pred->channels >> 1;

    float* pb = kps_pred->data;
    float* pp = priors->data;

    ASSERT(priors->channels == priors->stride);
    ASSERT(kps_pred->channels == kps_pred->stride);
    ASSERT(kps_pred->rows == priors->rows && kps_pred->cols == priors->cols);

    for(r = 0; r < rows; ++r)
    {
        for(c = 0; c < cols; ++c)
        {
            for(n = 0; n < num_points; ++n)
            {
                pb[0] = pb[0] * stride + pp[0];
                pb[1] = pb[1] * stride + pp[1];
                pb += 2;
            }
            pp += 2;
        }
    }
}

static DataBlobType* concat3(DataBlobType* in1, DataBlobType* in2, DataBlobType* in3)
{
    int r, c;
    int rows = in1->rows;
    int cols = in1->cols;
    int channels1 = in1->channels;
    int channels2 = in2->channels;
    int channels3 = in3->channels;

    DataBlobType* out = data_blob_create(rows, cols, channels1 + channels2 + channels3);

    float* data = out->data;

    for(r = 0; r < rows; r++)
    {
        for(c = 0; c < cols; c++)
        {
            memcpy(data, data_blob_data((DataBlobBase*)in1, r, c), sizeof(float)* channels1); data += channels1;
            memcpy(data, data_blob_data((DataBlobBase*)in2, r, c), sizeof(float)* channels2); data += channels2;
            memcpy(data, data_blob_data((DataBlobBase*)in3, r, c), sizeof(float)* channels3); data += channels3;
        }
    }

    return out;
}

static DataBlobType* blob2vector(DataBlobType* db)
{
    ASSERT(db->channels == db->stride);

    db->channels = db->rows * db->cols * db->channels;
    db->stride   = db->channels;
    db->rows     = 1;
    db->cols     = 1;

    return db;
}

static void sigmoid(DataBlobType* db)
{
    int r, c, ch;
    int rows = db->rows;
    int cols = db->cols;
    int channels = db->channels;

    float* data = db->data;

    ASSERT(db->channels == db->stride);

    for(r = 0; r < rows; ++r)
    {
        for(c = 0; c < cols; ++c)
        {
            for(ch = 0; ch < channels; ch++)
            {
                float v = *data;
                v = MIN(v, 88.3762626647949f);
                v = MAX(v, -88.3762626647949f);
                *data++ = (float)(1.f / (1.f + exp(-v)));
            }
        }
    }
}

static DataBlobUint8* load_image(unsigned char* data, int width, int height, int imgchannels)
{
    int r, c, fx, fy, offset;
    int rows = ((height - 1) / 32 + 1) * 32 / 2;
    int cols = ((width  - 1) / 32 + 1) * 32 / 2;
    int channels = 27;//27;//32;

    DataBlobUint8* out = (DataBlobUint8*)data_blob_base_create(rows, cols, channels, sizeof(uint8_t));

    for(r = 0; r < rows; r++)
    {
        for(c = 0; c < cols; c++)
        {
            uint8_t* pdata;
            uint8_t* pout = (uint8_t*)data_blob_data((DataBlobBase*)out, r, c);

            for(fy = -1; fy <= 1; fy++)
            {
                int srcy = r * 2 + fy;

                if(srcy < 0 || srcy >= height) continue;

                for(fx = -1; fx <= 1; fx++)
                {
                    int srcx = c * 2 + fx;

                    if(srcx < 0 || srcx >= width) continue;

                    offset = ((fy + 1) * 3 + fx + 1) * 3;
                    pdata  = data + width * imgchannels * srcy + imgchannels * srcx;

                    pout[offset + 0] = pdata[0];
                    pout[offset + 1] = pdata[1];
                    pout[offset + 2] = pdata[2];
                }
            }
        }
    }

    return out;
}

int facedetectcnn_size(void)
{
    return sizeof(Filter) * FILTERS_COUNT;
}

int facedetectcnn_init(void* handle)
{
    Filter* filters = (Filter*)handle;

    int i; for(i = 0; i < FILTERS_COUNT; i++) filter_init(&filters[i], (ConvInfoStruct*)&param_conv_info[i]);

    return 0;
}

int facedetectcnn_deinit(void* handle)
{
    return 0;
}

int facedetectcnn_run(void* handle, unsigned char* image, int width, int height, int channels, unsigned char* facebuf, int facelimit, int landmark, float confidence_threshold, float overlap_threshold)
{
    Filter* filters = (Filter*)handle;

    int i, j, keep, faces, faces2, faceinfosize;

    float *pCls, *pReg, *pObj, *pKps = 0;

    //float overlap_threshold = 0.3;
    //float confidence_threshold = 0.5f;

    unsigned char* facebuf2;
    pFaceInfo pface, pface2;

    pDataBlobType fxx = NULL;
    pDataBlobType fb1 = NULL;
    pDataBlobType fb2 = NULL;
    pDataBlobType fb3 = NULL;
    pDataBlobType cls, reg, kps, obj;
    pDataBlobType prior3, prior4, prior5;

    pDataBlobType pred_reg[3] = { NULL, NULL, NULL };
    pDataBlobType pred_cls[3] = { NULL, NULL, NULL };
    pDataBlobType pred_kps[3] = { NULL, NULL, NULL };
    pDataBlobType pred_obj[3] = { NULL, NULL, NULL };

    pDataBlobUint8 img = load_image(image, width, height, channels);
    convolution8(img, &fxx, &filters[0], 1); data_blob_destory((pDataBlobType)img);
    convolution2(fxx, &fxx, &filters[1], &filters[2], 1);
    maxpooling2x2S2(fxx, &fxx);
    convolution4(fxx, &fxx, &filters[3], &filters[4], &filters[5], &filters[6], 1);
    convolution4(fxx, &fxx, &filters[7], &filters[8], &filters[9], &filters[10], 1);
    maxpooling2x2S2(fxx, &fxx);

    convolution4(fxx, &fb1, &filters[11], &filters[12], &filters[13], &filters[14], 1);
    data_blob_destory(fxx); maxpooling2x2S2(fb1, &fxx);
    convolution4(fxx, &fb2, &filters[15], &filters[16], &filters[17], &filters[18], 1);
    data_blob_destory(fxx); maxpooling2x2S2(fb2, &fxx);
    convolution4(fxx, &fb3, &filters[19], &filters[20], &filters[21], &filters[22], 1);
    data_blob_destory(fxx);

    convolution2(fb3, &fb3, &filters[27], &filters[28], 1);
    data_blob_add2(fb3, fb2);
    convolution2(fb2, &fb2, &filters[25], &filters[26], 1);
    data_blob_add2(fb2, fb1);
    convolution2(fb1, &fb1, &filters[23], &filters[24], 1);

    convolution2(fb3, &pred_cls[2], &filters[33], &filters[34], 0);
    convolution2(fb3, &pred_reg[2], &filters[39], &filters[40], 0);
    convolution2(fb3, &pred_obj[2], &filters[45], &filters[46], 0);
    if(landmark) convolution2(fb3, &pred_kps[2], &filters[51], &filters[52], 0);

    width = fb3->cols; height = fb3->rows; data_blob_destory(fb3);
    prior5 = meshgrid(width, height, 32);

    convolution2(fb2, &pred_cls[1], &filters[31], &filters[32], 0);
    convolution2(fb2, &pred_reg[1], &filters[37], &filters[38], 0);
    convolution2(fb2, &pred_obj[1], &filters[43], &filters[44], 0);
    if(landmark) convolution2(fb2, &pred_kps[1], &filters[49], &filters[50], 0);

    width = fb2->cols; height = fb2->rows; data_blob_destory(fb2);
    prior4 = meshgrid(width, height, 16);

    convolution2(fb1, &pred_cls[0], &filters[29], &filters[30], 0);
    convolution2(fb1, &pred_reg[0], &filters[35], &filters[36], 0);
    convolution2(fb1, &pred_obj[0], &filters[41], &filters[42], 0);
    if(landmark) convolution2(fb1, &pred_kps[0], &filters[47], &filters[48], 0);

    width  = fb1->cols; height = fb1->rows; data_blob_destory(fb1);
    prior3 = meshgrid(width, height, 8);

    bbox_decode(pred_reg[0], prior3, 8);
    bbox_decode(pred_reg[1], prior4, 16);
    bbox_decode(pred_reg[2], prior5, 32);

    if(landmark)
    {
        kps_decode(pred_kps[0], prior3, 8);
        kps_decode(pred_kps[1], prior4, 16);
        kps_decode(pred_kps[2], prior5, 32);
    }

    data_blob_destory(prior3);
    data_blob_destory(prior4);
    data_blob_destory(prior5);

    cls = concat3(blob2vector(pred_cls[0]), blob2vector(pred_cls[1]), blob2vector(pred_cls[2]));
    data_blob_destory(pred_cls[0]); data_blob_destory(pred_cls[1]); data_blob_destory(pred_cls[2]);
    reg = concat3(blob2vector(pred_reg[0]), blob2vector(pred_reg[1]), blob2vector(pred_reg[2]));
    data_blob_destory(pred_reg[0]); data_blob_destory(pred_reg[1]); data_blob_destory(pred_reg[2]);
    obj = concat3(blob2vector(pred_obj[0]), blob2vector(pred_obj[1]), blob2vector(pred_obj[2]));
    data_blob_destory(pred_obj[0]); data_blob_destory(pred_obj[1]); data_blob_destory(pred_obj[2]);

    if(landmark)
    {
        kps = concat3(blob2vector(pred_kps[0]), blob2vector(pred_kps[1]), blob2vector(pred_kps[2]));
        data_blob_destory(pred_kps[0]); data_blob_destory(pred_kps[1]); data_blob_destory(pred_kps[2]);
        pKps = kps->data;
    }

    sigmoid(cls);
    sigmoid(obj);

    pCls = cls->data;
    pReg = reg->data;
    pObj = obj->data;

    pface = (FaceInfo*)facebuf;

    faceinfosize = sizeof(FaceInfo) + (!!landmark) * FACE_LANDMARK_BUFSIZE;

    for(i = 0, faces = 0; i < cls->channels; i++)
    {
        float score = (float)sqrt(pCls[i] * pObj[i]);

        if(score >= confidence_threshold)
        {
            pface->score = score;
            pface->xmin  = (short)pReg[4 * i + 0];
            pface->ymin  = (short)pReg[4 * i + 1];
            pface->xmax  = (short)pReg[4 * i + 2];
            pface->ymax  = (short)pReg[4 * i + 3];

            if(landmark) for(j = 0; j < 10; j++) pface->lm[j] = (short)pKps[10 * i + j];

            if(++faces >= facelimit) break;

            pface = (FaceInfo*)(facebuf + faceinfosize * faces);
        }
    }

    data_blob_destory(cls);
    data_blob_destory(reg);
    data_blob_destory(obj);
    if(landmark) data_blob_destory(kps);

    //Sort
    facebuf2 = port_malloc(faces * faceinfosize);
    pface2   = (FaceInfo*)facebuf2;

    for(i = 0; i < faces; i++)
    {
        int   idx = 0;
        float score, maxscore = 0;

        for(j = 0; j < faces; j++)
        {
            score = ((FaceInfo*)(facebuf + faceinfosize * j))->score;

            if(score > maxscore)
            {
                idx = j;
                maxscore = score;
            }
        }

        pface = (FaceInfo*)(facebuf + faceinfosize * idx);

        memcpy((FaceInfo*)(facebuf2 + faceinfosize * i), pface, faceinfosize);

        pface->score = 0;
    }

    //Do NMS
    pface  = (FaceInfo*)facebuf;
    pface2 = (FaceInfo*)facebuf2;

    for(i = 0, faces2 = 0; i < faces; i++)
    {
        keep = 1;

        for(j = 0; j < faces2; j++)
        {
            if(jaccard_overlap((FaceInfo*)(facebuf + faceinfosize * j), pface2) > overlap_threshold)
            {
                keep = 0;
                break;
            }
        }

        if(keep)
        {
            memcpy(pface, pface2, faceinfosize);
            pface = (FaceInfo*)(facebuf + faceinfosize * (++faces2));
        }

        pface2 = (FaceInfo*)(facebuf2 + faceinfosize * i);
    }

    port_free(facebuf2);

    return faces2;
}
