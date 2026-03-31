#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>

#include <common/bk_include.h>
#include <components/log.h>
#include <avdk_error.h>

#include <components/bk_frame_buffer.h>

#include "modules/h264e/jpegencapi.h"
#include "jpege_driver.h"

#define TAG "jpege_drv"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define CHECK_ENC_HANDLE(handle) \
    do { \
        if((handle) == NULL) {\
            LOGE("%s %d handle is NULL\r\n", __func__, __LINE__);\
            return BK_FAIL; \
        } \
    } while(0); \

static void jpege_flexa_input_linebuf_done(void *pAppData)
{
    jpeg_encoder_context *context = (jpeg_encoder_context*)pAppData;
    jpeg_enc_buf_t* enc_buf = &context->enc_buf;

    inputLineBufferCfg* line_buf_cfg = &enc_buf->lineBufCfg;

    if(context->callback.fcb)
    {
        uint8_t ret = 0;
        ret = context->callback.fcb(line_buf_cfg->lumBuf.buf, line_buf_cfg->cbBuf.buf, line_buf_cfg->crBuf.buf);
        if (ret)
        {
            line_buf_cfg->wrCnt++;
            VCEncInputLineBufWrCntSet(line_buf_cfg);
        }
    }
}

static void jpege_out_callback(uint8_t *buffer, uint32_t size, uint32_t type, uint32_t param)
{
    jpeg_encoder_context *context = (jpeg_encoder_context*)param;
    if (context->callback.ocb != NULL)
    {
        context->callback.ocb(buffer, size, type, context->callback.param);
    }
}

static void jpege_set_default_enc_config(jpeg_encoder_context *context)
{
    jpeg_enc_config_t *cfg = &context->enc_config;
    jpeg_enc_buf_t* enc_buf = &context->enc_buf;
    jpeg_enc_in_t *enc_in = &context->enc_in;

    cfg->quality     = 50;
    cfg->qLevel      = 5;
    cfg->fixedQP     = -1;
    cfg->frameType   = context->config.input_type;
    cfg->markerType  = JPEGENC_SINGLE_MARKER;
    cfg->unitsType   = JPEGENC_DOTS_PER_INCH;
    cfg->xDensity    = 72;
    cfg->yDensity    = 72;

    if (context->config.flexa_mode)
    {
        cfg->inputLineBufEn = 1;
        cfg->inputLineBufLoopBackEn = 1;
        cfg->inputLineBufDepth = 1;
        cfg->amountPerLoopBack = 3;
        cfg->inputLineBufHwModeEn = context->config.flexa_mode == JPEG_ENC_FLEXA_MODE_HARDWARE;
        cfg->inputLineBufCbFunc = jpege_flexa_input_linebuf_done;
        cfg->inputLineBufCbData = context;
        cfg->sbi_id_0 = FLEXA_STREAM_ID_Y;
        cfg->sbi_id_1 = FLEXA_STREAM_ID_CB;
        cfg->sbi_id_2 = FLEXA_STREAM_ID_CR;
        cfg->segmentUnitHeight = 16;
    }

    cfg->inputWidth   = context->config.width;
    cfg->codingWidth  = context->config.width;
    cfg->inputHeight  = context->config.height;
    cfg->codingHeight = context->config.height;
    cfg->xOffset      = 0;
    cfg->yOffset      = 0;
    cfg->rotation      = JPEGENC_ROTATE_0;
    cfg->codingType      = JPEGENC_WHOLE_FRAME;
    /* Restart interval 5 MCU rows (= 80 pixel rows), average quantization, encode whole frame at once */
    cfg->restartInterval = 5;
    
    enc_buf->width        = context->config.width;
    enc_buf->height       = context->config.height;
    enc_buf->alignment    = 8;
    enc_buf->encPicYSize  = STRIDE(context->config.width, enc_buf->alignment) * context->config.height;
    enc_buf->encPicUVSize = STRIDE(context->config.width / 2, enc_buf->alignment) * context->config.height / 2 * 2;
    enc_buf->encOutSize   = 0;
    enc_buf->encOutBuf    = 0;
    enc_buf->encOutCallback = jpege_out_callback;

    enc_in->dec400Enable  = 1;//1: bypass 2: enable
    enc_in->pOutBuf[0]    = 0;
    enc_in->busOutBuf[0]  = (ptr_t)enc_in->pOutBuf[0];
    enc_in->outBufSize[0] = enc_buf->encOutSize;
}

static uint32_t init_input_line_buffer(inputLineBufferCfg *line_buf_cfg, JpegEncCfg *enc_cfg,
                        JpegEncIn *enc_in, JpegEncInst inst) {
    u32 luma_stride, chroma_stride;
    JpegEncGetAlignedStride(enc_cfg->inputWidth, enc_cfg->frameType, &luma_stride,
                          &chroma_stride, 1 << enc_cfg->exp_of_input_alignment, enc_cfg->scanType);
    memset(line_buf_cfg, 0, sizeof(inputLineBufferCfg));
    line_buf_cfg->inst = (void *)inst;
    //line_buf_cfg->asic   = &(((jpegInstance_s *)inst)->asic);
    line_buf_cfg->wrCnt = 0;

    line_buf_cfg->depth = JpegGetSbiSupport(inst) ? 1 : enc_cfg->inputLineBufDepth;
    line_buf_cfg->inputFormat = enc_cfg->frameType;
    line_buf_cfg->lumaStride = luma_stride;
    line_buf_cfg->chromaStride = chroma_stride / 2;
    line_buf_cfg->encWidth = enc_cfg->codingWidth;
    line_buf_cfg->encHeight = enc_cfg->codingHeight;
    line_buf_cfg->hwHandShake = enc_cfg->inputLineBufHwModeEn;
    line_buf_cfg->loopBackEn = enc_cfg->inputLineBufLoopBackEn;
    line_buf_cfg->amountPerLoopBack = enc_cfg->amountPerLoopBack;
    line_buf_cfg->srcHeight =
        enc_cfg->codingType ? enc_cfg->restartInterval * 16 : enc_cfg->inputHeight;
    line_buf_cfg->srcVerOffset = enc_cfg->codingType ? 0 : enc_cfg->yOffset;
    line_buf_cfg->getMbLines = &JpegEncGetEncodedMbLines;
    line_buf_cfg->setMbLines = &JpegEncSetInputMBLines;
    line_buf_cfg->ctbSize = 16;
    line_buf_cfg->lumSrc = (u8 *)enc_in->pLum;
    line_buf_cfg->cbSrc = (u8 *)enc_in->pCb;
    line_buf_cfg->crSrc = (u8 *)enc_in->pCr;
    line_buf_cfg->initSegNum = 0;
    line_buf_cfg->client_type = EWL_CLIENT_TYPE_JPEG_ENC;

    if (VCEncInitInputLineBuffer(line_buf_cfg))
    {
        LOGE("%s %d VCEncInitInputLineBuffer failed\n", __func__, __LINE__);
        return -1;
    }

  /* loopback mode */
    if (line_buf_cfg->loopBackEn && line_buf_cfg->lumBuf.buf)
    {
        enc_in->busLum = line_buf_cfg->lumBuf.busAddress;
        enc_in->busCb = line_buf_cfg->cbBuf.busAddress;
        enc_in->busCr = line_buf_cfg->crBuf.busAddress;

        /* data in SRAM start from the line to be encoded*/
        if (enc_cfg->codingType == JPEGENC_WHOLE_FRAME)
        {
            enc_cfg->yOffset = 0;
        }
    }

    return 0;
}

int32_t jpege_init(jpeg_encoder_handle_t* handle, jpeg_encoder_config_t* in_config)
{
    bk_err_t result = BK_OK;
    JpegEncRet  ret;
    jpeg_enc_config_t *cfg = NULL;
    jpeg_enc_inst_t encoder = NULL;

    jpeg_encoder_context* context = (jpeg_encoder_context*)os_malloc(sizeof(jpeg_encoder_context));

    if(!context)
    {
        return VCENC_MEMORY_ERROR;
    }

    jpeg_enc_buf_t *enc_buf = &context->enc_buf;
    jpeg_enc_in_t *enc_in = &context->enc_in;

    os_memset(context, 0, sizeof(jpeg_encoder_context));
    os_memcpy(&context->config, in_config, sizeof(jpeg_encoder_config_t));

    cfg = &context->enc_config;
    jpege_set_default_enc_config(context);

    result = encoder_core_init(&context->module);
    if (result != BK_OK)
    {
        goto error;
    }

    if((ret = JpegEncInit(cfg, &encoder, NULL)) != JPEGENC_OK)
    {
        LOGE("JpegEncInit falied with error code %d\n", ret);
        goto error;
    }

    context->enc_handle = encoder;

    if((ret = JpegEncSetPictureSize(encoder, cfg)) != JPEGENC_OK)
    {
        LOGE("JpegEncSetPictureSize falied with error code %d\n", ret);
        goto error;
    }
 
     /* Step 3: Allocate linear output buffer */

    if (context->config.flexa_mode)
    {
        enc_buf->lineBufMode = context->config.flexa_mode;
        init_input_line_buffer(&enc_buf->lineBufCfg, cfg, enc_in, encoder);
    }

    *handle = context;

    return ret;
error:
    return ret;
}

int32_t jpege_deinit(jpeg_encoder_handle_t* handle)
{
    JpegEncRet ret;

    jpeg_encoder_context* context = (jpeg_encoder_context*)*handle;

    jpeg_enc_inst_t  enc_handle = context->enc_handle;
    jpeg_enc_buf_t*  enc_buf    = &context->enc_buf;

    if(enc_handle)
    {
        if(enc_buf->encOutBuf)
        {
            os_free((void*)enc_buf->encOutBuf);
        }
    
        /* Last Step: Release the encHandle instance */
        if((ret = JpegEncRelease(enc_handle)) != JPEGENC_OK)
        {
            LOGE("JpegEncRelease failed with error code %d\n", ret);
        }
    }
    else
    {
        ret = JPEGENC_INSTANCE_ERROR;
    }

    encoder_core_deinit(&context->module);

    os_free(context);
    *handle = NULL;

    return ret;
}

bk_err_t jpege_register_callback(jpeg_encoder_handle_t* handle, jpeg_encoder_callback_t *callback)
{
    CHECK_ENC_HANDLE(*handle);
    jpeg_encoder_context *context = (jpeg_encoder_context*)*handle;
    os_memcpy(&context->callback, callback, sizeof(jpeg_encoder_callback_t));
    return BK_OK;
}

bk_err_t jpege_deregister_callback(jpeg_encoder_handle_t* handle)
{
    CHECK_ENC_HANDLE(*handle);
    jpeg_encoder_context *context = (jpeg_encoder_context*)*handle;
    bk_err_t result = BK_OK;
    result = encoder_get_module_sem(context->module, 100);
    if (result != BK_OK)
    {
        LOGE("%s %d get_module_sem fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    os_memset(&context->callback, 0, sizeof(jpeg_encoder_callback_t));
    result = encoder_set_module_sem(context->module);
    if (result != BK_OK)
    {
        LOGE("%s %d set_module_sem fail\n", __func__, __LINE__);
    }
    return BK_OK;
}

static bk_err_t jpege_outbuf_update(jpeg_encoder_context* context, uint32_t out_buf, uint32_t out_size)
{
    jpeg_enc_buf_t* enc_buf = &context->enc_buf;
    bk_err_t ret = BK_OK;
    if(out_buf)
    {
        if(out_size)
        {
            if (out_size < VCENC_STREAM_MIN_BUF0_SIZE)
            {
                ret = BK_ERR_PARAM;
                goto error;
            }
            if(enc_buf->encOutBuf) os_free((void *)enc_buf->encOutBuf);
            enc_buf->encOutSize = out_size;
            enc_buf->encOutBuf  = 0;
        }
        else
        {
            return ret;
        }
    }
    else
    {
        if(out_size)
        {
            if (out_size < VCENC_STREAM_MIN_BUF0_SIZE)
            {
                ret = BK_ERR_PARAM;
                goto error;
            }
            if(enc_buf->encOutBuf == 0)
            {
                enc_buf->encOutSize = out_size;
                enc_buf->encOutBuf  = (uint32_t)os_malloc(enc_buf->encOutSize);
                if (enc_buf->encOutBuf == 0)
                {
                    ret = BK_ERR_NO_MEM;
                    goto error;
                }
            }
            else if(out_size != enc_buf->encOutSize)
            {
                os_free((void*)enc_buf->encOutBuf);
                enc_buf->encOutSize = out_size;
                enc_buf->encOutBuf  = (uint32_t)os_malloc(enc_buf->encOutSize);
                if (enc_buf->encOutBuf == 0)
                {
                    ret = BK_ERR_NO_MEM;
                    goto error;
                }
            }
        }
        else
        {
            if(enc_buf->encOutBuf == 0)
            {
                enc_buf->encOutSize = enc_buf->width * enc_buf->height;
                enc_buf->encOutBuf  = (uint32_t)os_malloc(enc_buf->encOutSize);
                if (enc_buf->encOutBuf == 0)
                {
                    ret = BK_ERR_NO_MEM;
                    goto error;
                }
            }
        }
    }
    jpeg_enc_in_t*  enc_in      = &context->enc_in;
    enc_in->pOutBuf[0]    = (uint8_t *)(out_buf ? out_buf : (uint32_t)enc_buf->encOutBuf | MEM_CACHABLE_MASK);
    enc_in->busOutBuf[0]  = (ptr_t)enc_in->pOutBuf[0];
    enc_in->outBufSize[0] = enc_buf->encOutSize;
error:
    return ret;
}

bk_err_t jpege_start_encode(jpeg_encoder_handle_t* handle, jpeg_encoder_parameters_t *para)
{
    bk_err_t result = BK_OK;
    JpegEncRet ret;

    jpeg_encoder_context* context = (jpeg_encoder_context*)*handle;

    jpeg_enc_inst_t  enc_handle = context->enc_handle;
    jpeg_enc_in_t*   enc_in     = &context->enc_in;
    jpeg_enc_out_t*  enc_out    = &context->enc_out;
    jpeg_enc_buf_t*  enc_buf    = &context->enc_buf;

    if(!context || !enc_handle)
    {
        return JPEGENC_INSTANCE_ERROR;
    }

    result = encoder_get_core_lock();
    if (result != BK_OK)
    {
        LOGE("%s %d get_core_lock fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
    result = encoder_get_module_sem(context->module, 500);
    if (result != BK_OK)
    {
        LOGE("%s %d get_module_sem fail\n", __func__, __LINE__);
        encoder_set_core_lock();
        return BK_FAIL;
    }

    ret = jpege_outbuf_update(context, para->out_buf, para->out_size);
    if (ret != BK_OK)
    {
        LOGE("%s %d outbuf_update failed %d\n", __func__, __LINE__, ret);
        goto error;
    }

    /* Enable all frame headers */
    enc_in->frameHeader = 1;

    if(enc_buf->lineBufMode)
    {
        inputLineBufferCfg* lineBufCfg = &enc_buf->lineBufCfg;

        lineBufCfg->lumSrc  = (u8*)para->pic_buf;
        lineBufCfg->cbSrc   = lineBufCfg->lumSrc + enc_buf->encPicYSize;
        lineBufCfg->crSrc   = lineBufCfg->cbSrc  + enc_buf->encPicUVSize / 2;
        lineBufCfg->wrCnt   = 0;
        lineBufCfg->sram    = (u8*)para->pic_buf;
        lineBufCfg->sramBusAddr = para->pic_buf;
        lineBufCfg->sramSize= lineBufCfg->depth * lineBufCfg->amountPerLoopBack * 16 * lineBufCfg->encWidth * 3 / 2;
        enc_in->lineBufWrCnt = VCEncStartInputLineBufferWithoutCopy(lineBufCfg, para->pic_lines);
        enc_in->initSegNum   = lineBufCfg->initSegNum;
        enc_in->busLum      = lineBufCfg->lumBuf.busAddress;
        enc_in->busCb   = lineBufCfg->cbBuf.busAddress;
        enc_in->busCr   = lineBufCfg->crBuf.busAddress;
    }
    else
    {
        enc_in->busLum = para->pic_buf;
        enc_in->busCb  = enc_in->busLum + enc_buf->encPicYSize;
        enc_in->busCr  = enc_in->busCb + enc_buf->encPicUVSize / 2;
    }

    /* Encode the picture, Loop until the frame is ready */
    do
    {
        ret = JpegEncEncode(enc_handle, enc_in, enc_out);
        switch (ret)
        {
        case JPEGENC_RESTART_INTERVAL:
        case JPEGENC_FRAME_READY:
            if(enc_buf->encOutCallback)
            {
                enc_buf->encOutCallback((uint8_t*)enc_in->pOutBuf[0], enc_out->jfifSize, enc_out->headerSize, (uint32_t)context);
            }
            break;
        default: /* All the others are error codes */
            LOGI("%s, %d error ret:%d\n", __func__, __LINE__, ret);
            break;
        }

    } while (ret == JPEGENC_RESTART_INTERVAL);

error:
    result = encoder_set_module_sem(context->module);
    if (result != BK_OK)
    {
        LOGE("%s %d set_module_sem fail\n", __func__, __LINE__);
    }
    result = encoder_set_core_lock();
    if (result != BK_OK)
    {
        LOGE("%s %d set_core_lock fail\n", __func__, __LINE__);
    }

    return ret;
}

