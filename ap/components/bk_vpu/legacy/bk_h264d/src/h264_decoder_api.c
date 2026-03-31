#include <stdint.h>
#include <stddef.h>
#include <os/os.h>
#include "os/mem.h"
#include <modules/h264d/dwl.h>
#include <modules/h264d/ppapi.h>
#include <modules/h264d/h264decapi.h>
#include <modules/h264d/jpegdecapi.h>
#include "driver/int.h"
#include "sys_driver.h"
#include "h264_decoder_api.h"
#include <components/bk_frame_buffer.h>
#include <components/log.h>

#define TAG "h264_decoder_api"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct _PPOsd
{
    uint32_t enable;
    uint32_t originX;
    uint32_t originY;
    uint32_t width;
    uint32_t height;
    uint8_t* buffer;
}PPOsd;

typedef struct _PPCfg
{
    uint32_t outWidth;
    uint32_t outHeight;
    uint32_t rotation;
    uint32_t rbEnable;
    PPOsd    osd[2];
}PPCfg;

typedef struct _H264DecoderContext
{
	H264DecInst    decHandle;
    H264DecInfo    decInfo;
    H264DecInput   decIn;
    H264DecOutput  decOut;
    H264DecPicture decPic;

    PPCfg    ppCfg;
    PPInst   ppHandle;
    uint32_t ppCfged;
    uint8_t* ppOutbuf;

    VCDecOutCallback decOutCallback;

}H264DecoderContext;

typedef struct _JPEGDecoderContext
{
	JpegDecInst   decHandle;
	JpegDecInput  decIn;
	JpegDecOutput decOut;

    JpegDecImageInfo imgInfo;

    PPCfg    ppCfg;
    PPInst   ppHandle;
    uint32_t ppCfged;

    VCDecOutCallback decOutCallback;

}JPEGDecoderContext;

extern void  enable_irq(void);
extern void  disable_irq(void);
extern int   Platform_init(void);
extern int   Platform_deinit(void);
extern void  hx170dec_isr(int irq, void *dev_id);


static uint8_t pp_rb_rp_clr_request = 0;
static uint8_t vcdec_platform_inited = 0;

static VCDecFlexaDoneCallback pp_rb_flexa_done_callback = 0;

VCDecHWReadyCallback hw_wait_ready_done_callback = 0;

uint32_t ppRbWritePointerGet(void)
{
    return *(volatile uint32_t*)(H264D_ASIC_BASE_ADDR + 0x12C) >> 16;
}

uint32_t ppRbReadPointerGet(void)
{
    return *(volatile uint32_t*)(H264D_ASIC_BASE_ADDR + 0x138) >> 16;
}

void ppRbReadPointerSet(uint32_t value)
{
    volatile uint32_t* preg = (volatile uint32_t*)(H264D_ASIC_BASE_ADDR + 0x138);
    *preg = ((*preg) & 0xFFFF) | (value << 16);
}

void ppRbReadyClear(void)
{
    *(volatile uint32_t*)(H264D_ASIC_BASE_ADDR + 0x0F0) &= ~(1 << 9);
    *(volatile uint32_t*)(H264D_ASIC_BASE_ADDR + 200 * 4) = 0x1;
}

static void int_handler_h26d(void)
{
    hx170dec_isr(0, 0);
}

static void int_handler_h26d_pp(void)
{
    ppRbReadyClear();

    if(pp_rb_flexa_done_callback) pp_rb_flexa_done_callback(ppRbWritePointerGet());
}

static void h264d_int_register(void)
{
    //disable_irq();
    bk_int_isr_register(INT_SRC_H264D, (int_group_isr_t)&int_handler_h26d, NULL);
    bk_int_isr_register(INT_SRC_H264D_PP, (int_group_isr_t)&int_handler_h26d_pp, NULL);
    //enable_irq();

#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H264D, 1);
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H264D_PP, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H264D, 1);
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H264D_PP, 1);
#endif

    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26D, PM_POWER_MODULE_STATE_ON);
}

static void* frame_buffer_uncoded_data_malloc(size_t size)
{
	return bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
}

static void frame_buffer_uncoded_data_free(void* pbuf)
{
	bk_frame_buffer_free(pbuf);
}

static void __vcdec_platform_init(void)
{
	if(vcdec_platform_inited == 0)
	{
		Platform_init();
        h264d_int_register();
		//vcenc_memalloc_register(os_malloc, os_free);
		vcdec_memalloc_register(frame_buffer_uncoded_data_malloc, frame_buffer_uncoded_data_free);
	}
	vcdec_platform_inited++;
}

static void __vcdec_platform_deinit(void)
{
	if(vcdec_platform_inited)
	{
		vcdec_platform_inited--;
		if(vcdec_platform_inited == 0) Platform_deinit();
	}
}

static void vcdec_pp_config(PPCfg* config, uint32_t outWidth, uint32_t outHeight, uint32_t rotation)
{
    config->outWidth  = outWidth;
    config->outHeight = outHeight;

    switch(rotation)
    {
    case PP_ROTATION_NONE:
    case PP_ROTATION_RIGHT_90:
    case PP_ROTATION_LEFT_90:
    case PP_ROTATION_HOR_FLIP:
    case PP_ROTATION_VER_FLIP:
    case PP_ROTATION_180:
        config->rotation = rotation;
        break;
    case 90:
        config->rotation = PP_ROTATION_RIGHT_90;
        break;
    case 180:
        config->rotation = PP_ROTATION_180;
        break;
    case 270:
        config->rotation = PP_ROTATION_LEFT_90;
        break;
    default:
        config->rotation = PP_ROTATION_NONE;
        break;
    }
}

static void vcdec_pp_osd_config(PPCfg* config, uint32_t index, void* buffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if(index < 2)
    {
        PPOsd* osd = &config->osd[index];
        osd->enable  = buffer != 0;
        osd->originX = x;
        osd->originY = y;
        osd->width   = width;
        osd->height  = height;
        osd->buffer  = buffer;
    }
}

static int32_t vcdec_pp_init(PPInst* ppHandle, const void *decoder, uint32_t type)
{
    int32_t ret;

    if((ret = (int32_t)PPInit(ppHandle)) != PP_OK)
    {
        os_printf("PPInit failed with error code %d\n", ret);
        return ret;
    }

    if((ret = (int32_t)PPDecCombinedModeEnable(*ppHandle, decoder, type)) != PP_OK)
    {
        os_printf("PPDecCombinedModeEnable failed with error code %d\n", ret);
        return ret;
    }

    return ret;
}

static int32_t vcdec_pp_set_config(PPInst handle, PPCfg* config, uint32_t inWidth, uint32_t inHeight, uint8_t* outBuffer)
{
    int32_t  ret;
    PPConfig ppCfg;

    uint32_t outWidth;
    uint32_t outHeight;

    if(config->outWidth == 0) config->outWidth = inWidth;
    if(config->outHeight == 0) config->outHeight = inHeight;

    outWidth  = config->outWidth;
    outHeight = config->outHeight;

    ret = PPGetConfig(handle, &ppCfg);
    if(ret) os_printf("PPGetConfig with error code %d\n", ret);

    ppCfg.ppInImg.width  = inWidth;
    ppCfg.ppInImg.height = inHeight;

    ppCfg.ppInRotation.rotation = config->rotation;

    ppCfg.ppOutRb.rb_en                 = config->rbEnable;
    ppCfg.ppOutRb.irq_mask              = 0;
    ppCfg.ppOutRb.segment_height        = 1;
    ppCfg.ppOutRb.segment_num           = 2;

    ppCfg.ppOutMask1.enable             = config->osd[0].enable;
    ppCfg.ppOutMask1.originX            = config->osd[0].originX;
    ppCfg.ppOutMask1.originY            = config->osd[0].originY;
    ppCfg.ppOutMask1.height             = config->osd[0].height;
    ppCfg.ppOutMask1.width              = config->osd[0].width;    
    ppCfg.ppOutMask1.alphaBlendEna      = config->osd[0].buffer != 0;
    ppCfg.ppOutMask1.blendComponentBase = (g1_addr_t)config->osd[0].buffer;
    ppCfg.ppOutMask1.blendOriginX       = 0;
    ppCfg.ppOutMask1.blendOriginY       = 0;
    ppCfg.ppOutMask1.blendWidth         = config->osd[0].width;
    ppCfg.ppOutMask1.blendHeight        = config->osd[0].height;

    ppCfg.ppOutMask2.enable             = config->osd[1].enable;
    ppCfg.ppOutMask2.originX            = config->osd[1].originX;
    ppCfg.ppOutMask2.originY            = config->osd[1].originY;
    ppCfg.ppOutMask2.height             = config->osd[1].height;
    ppCfg.ppOutMask2.width              = config->osd[1].width;    
    ppCfg.ppOutMask2.alphaBlendEna      = config->osd[1].buffer != 0;
    ppCfg.ppOutMask2.blendComponentBase = (g1_addr_t)config->osd[1].buffer;
    ppCfg.ppOutMask2.blendOriginX       = 0;
    ppCfg.ppOutMask2.blendOriginY       = 0;
    ppCfg.ppOutMask2.blendWidth         = config->osd[1].width;
    ppCfg.ppOutMask2.blendHeight        = config->osd[1].height;

    ppCfg.ppOutImg.bufferBusAddr        = (g1_addr_t)outBuffer;
    outBuffer += outWidth * (config->rbEnable ? 16 * ppCfg.ppOutRb.segment_height * ppCfg.ppOutRb.segment_num : outHeight);
    ppCfg.ppOutImg.bufferChromaBusAddr  = (g1_addr_t)outBuffer;
    ppCfg.ppOutImg.width                = outWidth;
    ppCfg.ppOutImg.height               = outHeight;
    ppCfg.ppOutImg.pixFormat            = (ppCfg.ppOutMask1.alphaBlendEna || ppCfg.ppOutMask2.alphaBlendEna) ? PP_PIX_FMT_RGB16_5_6_5 : PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR;

    ret = PPSetConfig(handle, &ppCfg);
    if(ret) os_printf("PPSetConfig  with error code %d\n", ret);

    return ret;
}

void vcdec_flexa_input_linebuf_rdcnt_set(void* handle, uint32_t rdcnt)
{
    //ppRbReadPointerSet(rdcnt);
    // do { ppRbReadPointerSet(rdcnt); } while(ppRbReadPointerGet() != rdcnt);
    //os_printf("ppRbReadPointerSet(%d)\n", rdcnt);
}

int32_t h264_decoder_init(void** handle, uint32_t flexaMode, VCDecFlexaDoneCallback fcb, VCDecHWReadyCallback rcb, VCDecOutCallback ocb)
{
	int32_t ret;

	H264DecInst decoder;

	H264DecoderContext* context = (H264DecoderContext*)os_malloc(sizeof(H264DecoderContext));

	if(!context) return JPEGDEC_MEMFAIL;

    memset(context, 0, sizeof(H264DecoderContext));

	*handle = context;

    context->decOutCallback = ocb;
    pp_rb_flexa_done_callback = fcb;
    hw_wait_ready_done_callback = rcb;

    __vcdec_platform_init();

    #if 0
    H264DecApiVersion decApi;
    H264DecBuild decBuild;
    decApi   = H264DecGetAPIVersion();
    decBuild = H264DecGetBuild();
    os_printf("H264 Decoder API v%d.%d, SW build: %d.%d, HW build: %x\n", decApi.major, decApi.minor, decBuild.swBuild >> 16, decBuild.swBuild & 0xFFFF, decBuild.hwBuild);
    #endif

    if(H264DEC_OK != (ret = H264DecInit(&decoder, 0, 0, 0, DEC_REF_FRM_RASTER_SCAN, 0, 0, 0)))
    {
        os_printf("H264DecInit failed with error code %d\n", ret);
        goto __END;
    }

    if(flexaMode == VCDEC_FLEXA_MODE_FLEXA)
    {
        if(PP_OK != (ret = vcdec_pp_init(&context->ppHandle, decoder, PP_PIPELINED_DEC_TYPE_H264)))
        {
            os_printf("PPInit failed with error code %d\n", ret);
            goto __END;
        }
    }

    H264DecSliceIntEnable(decoder, flexaMode == VCDEC_FLEXA_MODE_FLEXA || flexaMode == VCDEC_FLEXA_MODE_SLICE);

    context->ppCfged   = 0;
    context->decHandle = decoder;
    /*
     * Flexa mode relies on PP output ring-buffer (rb) to produce slice/segment outputs.
     * If rb is not enabled here, the PP write pointer won't advance, which triggers
     * TIMEOUT<wr,last_wr> in ISR and makes flexa done callback always see wrCnt=0.
     */
    context->ppCfg.rbEnable = (flexaMode == VCDEC_FLEXA_MODE_FLEXA);

    return H264DEC_OK;

__END:
    H264DecRelease(decoder);

    return ret;
}

int32_t h264_decoder_deinit(void* handle)
{
    H264DecRet ret = H264DEC_OK;

	H264DecoderContext* context = (H264DecoderContext*)handle;

	H264DecInst     decInst = context->decHandle;
    H264DecPicture* decPic  = &context->decPic;  

    while(H264DecNextPicture(decInst, decPic, 1) == H264DEC_PIC_RDY)
    {
        if(context->decOutCallback) context->decOutCallback((uint8_t*)decPic->pOutputPicture, (uint8_t*)decPic->pOutputPicture + decPic->picWidth * decPic->picHeight, 0, decPic->picWidth, decPic->picHeight, decPic->picCodingType[0]);
    }

    if(context->ppHandle)
    {
        PPRelease(context->ppHandle);
    }

	if(context->decHandle)
	{
		H264DecRelease(context->decHandle);
	}
	else
	{
		ret = H264DEC_NOT_INITIALIZED;
	}

	os_free(context);

	__vcdec_platform_deinit();

    return ret;
}

int32_t h264_decoder_pp_config(void* handle, uint32_t outWidth, uint32_t outHeight, uint32_t rotation)
{
    H264DecoderContext* context = (H264DecoderContext*)handle;

    if(!context->ppHandle) vcdec_pp_init(&context->ppHandle, context->decHandle, PP_PIPELINED_DEC_TYPE_H264);

    vcdec_pp_config(&context->ppCfg, outWidth, outHeight, rotation);

    context->ppCfged = 0;

    return H264DEC_OK;
}

int32_t h264_decoder_osd_config(void* handle, uint32_t index, void* buffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    H264DecoderContext* context = (H264DecoderContext*)handle;

    if(!context->ppHandle) vcdec_pp_init(&context->ppHandle, context->decHandle, PP_PIPELINED_DEC_TYPE_H264);

    vcdec_pp_osd_config(&context->ppCfg, index, buffer, x, y, width, height);

    context->ppCfged = 0;

    return H264DEC_OK;
}

int32_t h264_decoder_decode(void* handle, uint8_t* inBuf, uint32_t inSize, uint8_t* outBuf, uint32_t* usedSize)
{
    int32_t ret;

    H264DecoderContext* context = (H264DecoderContext*)handle;

	H264DecInst     decInst = context->decHandle;
	H264DecInfo*    decInfo = &context->decInfo;
    H264DecInput*   decIn   = &context->decIn;
	H264DecOutput*  decOut  = &context->decOut;
    H264DecPicture* decPic  = &context->decPic;

	if(!context || !decInst) return H264DEC_NOT_INITIALIZED;

    decIn->dataLen = inSize;
    decIn->pStream = (const u8*)inBuf;
    decIn->streamBusAddress = (uintptr_t)decIn->pStream;

    pp_rb_rp_clr_request = 1;

    if(context->ppHandle && decInfo->picWidth != 0)
    {
        if(context->ppCfged == 0 || context->ppOutbuf != outBuf)
        {
            vcdec_pp_set_config(context->ppHandle, &context->ppCfg, decInfo->picWidth, decInfo->picHeight, outBuf);
        }
    }

    context->ppOutbuf = outBuf;

	do
    {
        ret = H264DecDecode(decInst, decIn, decOut);

        if(ret == H264DEC_HDRS_RDY)
        {
            H264DecGetInfo(decInst, decInfo);
            if(context->ppCfged == 0 && context->ppHandle)
            {
                context->ppCfged = 1;
                vcdec_pp_set_config(context->ppHandle, &context->ppCfg, decInfo->picWidth, decInfo->picHeight, outBuf);
            }
            if(context->decOutCallback) context->decOutCallback(NULL, NULL, NULL, decInfo->picWidth, decInfo->picHeight, VCDEC_OUT_INFO);

            break;
        }
        else if(ret == H264DEC_PIC_DECODED)
        {
            decIn->picId++;
            while(H264DecNextPicture(decInst, decPic, 0) == H264DEC_PIC_RDY)
            {
                if(context->decOutCallback)
                {
                    uint32_t width;
                    uint32_t height;
                    uint8_t* buffer;

                    if(context->ppHandle)
                    {
                        width  = context->ppCfg.outWidth;
                        height = context->ppCfg.outHeight;
                        buffer = (uint8_t*)outBuf;
                    }
                    else
                    {
                        width  = decPic->picWidth;
                        height = decPic->picHeight;
                        buffer = (uint8_t*)decPic->outputPictureBusAddress;
                    }

                    context->decOutCallback((uint8_t*)buffer, (uint8_t*)buffer + width * height, 0, width, height, decPic->picCodingType[0]);    
                }
            }

            break;
        }
        else if(ret == H264DEC_STRM_PROCESSED)
        {
            /* Stream processed but no picture ready - need more data or this is expected */
            /* In multi-call scenario, this is normal - just exit and let caller provide next data */
            os_printf("H264DEC_STRM_PROCESSED: stream consumed, no picture ready yet\n");
            break;
        }
        else if(ret < H264DEC_OK)
        {
            os_printf("H264DecDecode error <%d>\n", ret);
            break;
        }

    }while(1);

    if(usedSize) *usedSize = decIn->dataLen - decOut->dataLeft;

    return ret;
}

int32_t jpeg_decoder_init(void** handle, uint32_t flexaMode, VCDecFlexaDoneCallback fcb, VCDecHWReadyCallback rcb, VCDecOutCallback ocb)
{
	int32_t ret;

	JpegDecInst decoder;

	JPEGDecoderContext* context = (JPEGDecoderContext*)os_malloc(sizeof(JPEGDecoderContext));

	if(!context) return JPEGDEC_MEMFAIL;

    memset(context, 0, sizeof(JPEGDecoderContext));

	*handle = context;

    context->decOutCallback = ocb;

    pp_rb_flexa_done_callback = fcb;
    hw_wait_ready_done_callback = rcb;

	JpegDecInst*   decHandle = &context->decHandle;
	JpegDecInput*  decIn     = &context->decIn;

	__vcdec_platform_init();

	if((ret = (int32_t)JpegDecInit(&decoder)) != JPEGDEC_OK)
    {
        os_printf("JpegDecInit failed with error code %d\n", ret);
        goto __END;
    }

    if(flexaMode == VCDEC_FLEXA_MODE_FLEXA)
    {
        if(PP_OK != (ret = vcdec_pp_init(&context->ppHandle, decoder, PP_PIPELINED_DEC_TYPE_JPEG)))
        {
            os_printf("PPInit failed with error code %d\n", ret);
            goto __END;
        }
    }

    JpegDecSliceIntEnable(decoder, flexaMode == VCDEC_FLEXA_MODE_FLEXA || flexaMode == VCDEC_FLEXA_MODE_SLICE);

    context->ppCfged = 0;
    context->ppCfg.rbEnable = flexaMode == VCDEC_FLEXA_MODE_FLEXA;

	*decHandle = decoder;

	decIn->decImageType = JPEGDEC_IMAGE;
    decIn->sliceMbSet   = flexaMode >= VCDEC_FLEXA_MODE_SLICE ? flexaMode - 1 : 0;

__END:
    return ret;
}

int32_t jpeg_decoder_deinit(void* handle)
{
	JpegDecRet ret = JPEGDEC_OK;

	JPEGDecoderContext* context = (JPEGDecoderContext*)handle;

    if(context->ppHandle)
    {
        PPRelease(context->ppHandle);
    }

	if(context->decHandle)
	{
		JpegDecRelease(context->decHandle);
	}
	else
	{
		ret = JPEGDEC_ERROR;
	}

	os_free(context);

	__vcdec_platform_deinit();

	return ret;
}

int32_t jpeg_decoder_pp_config(void* handle, uint32_t outWidth, uint32_t outHeight, uint32_t rotation)
{
    JPEGDecoderContext* context = (JPEGDecoderContext*)handle;

    if(!context->ppHandle) vcdec_pp_init(&context->ppHandle, context->decHandle, PP_PIPELINED_DEC_TYPE_JPEG);

    vcdec_pp_config(&context->ppCfg, outWidth, outHeight, rotation);

    context->ppCfged = 0;

    return JPEGDEC_OK;
}

int32_t jpeg_decoder_osd_config(void* handle, uint32_t index, void* buffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    JPEGDecoderContext* context = (JPEGDecoderContext*)handle;

    if(!context->ppHandle) vcdec_pp_init(&context->ppHandle, context->decHandle, PP_PIPELINED_DEC_TYPE_JPEG);

    vcdec_pp_osd_config(&context->ppCfg, index, buffer, x, y, width, height);

    context->ppCfged = 0;

    return JPEGDEC_OK;
}

int32_t jpeg_decoder_decode(void* handle, uint8_t* inBuf, uint32_t inSize, uint8_t* outBuf, uint32_t outSize)
{
	int32_t  ret, sliceCnt = 0;
    uint8_t* flexa_buffer  = outBuf;

    JPEGDecoderContext* context = (JPEGDecoderContext*)handle;

	JpegDecInst    decHandle = context->decHandle;
	JpegDecInput*  decIn     = &context->decIn;
	JpegDecOutput* decOut    = &context->decOut;

    JpegDecImageInfo* imgInfo = &context->imgInfo;

	if(!context || !decHandle) return JPEGDEC_ERROR;

    decIn->streamBuffer.pVirtualAddress = (u32*)inBuf;
    decIn->streamBuffer.busAddress = (g1_addr_t)inBuf;
	decIn->streamLength = inSize;
    decIn->bufferSize   = 0;

    pp_rb_rp_clr_request = 1;

    if((ret = JpegDecGetImageInfo(decHandle, decIn, imgInfo)) != JPEGDEC_OK)
    {
        os_printf("JpegDecGetImageInfo failed with error code %d\n", ret);
    }

    //CHECK OUTSIZE HERE
    //os_printf("JpegDecGetImageInfo: %d x %d\n", imgInfo->outputWidth, imgInfo->outputHeight);

    decIn->pictureBufferY.pVirtualAddress    = (u32*)outBuf; outBuf += imgInfo->outputWidth * imgInfo->outputHeight;
    decIn->pictureBufferCbCr.pVirtualAddress = (u32*)outBuf; outBuf += imgInfo->outputWidth * imgInfo->outputHeight / 4;
    decIn->pictureBufferCr.pVirtualAddress   = (u32*)outBuf;
    decIn->pictureBufferY.busAddress         = (g1_addr_t)decIn->pictureBufferY.pVirtualAddress;
    decIn->pictureBufferCbCr.busAddress      = (g1_addr_t)decIn->pictureBufferCbCr.pVirtualAddress;
    decIn->pictureBufferCr.busAddress        = (g1_addr_t)decIn->pictureBufferCr.pVirtualAddress;

    if(context->ppCfged == 0 && context->ppHandle)
    {
        context->ppCfged = 1;
        vcdec_pp_set_config(context->ppHandle, &context->ppCfg, imgInfo->outputWidth, imgInfo->outputHeight, flexa_buffer);
    }

	do
    {
        ret = JpegDecDecode(decHandle, decIn, decOut);

        if(ret == JPEGDEC_FRAME_READY)
        {
            //os_printf("JPEGDEC_FRAME_READY\n");
            if(context->decOutCallback)
            {
                if(context->ppHandle)
                {
                    PPResult res;
                    PPOutput ppout;

                    if((res = PPGetNextOutput(context->ppHandle, &ppout)) == PP_OK)
                    {
                        context->decOutCallback((uint8_t*)ppout.bufferBusAddr,
                                                (uint8_t*)ppout.bufferChromaBusAddr,
                                                (uint8_t*)0,
                                                context->ppCfg.outWidth,
                                                context->ppCfg.outHeight,
                                                VCDEC_OUT_JFRAME);
                    }
                    else
                    {
                        os_printf("PPGetNextOutput with error code %d\n", res);
                    }
                }
                else
                {
                    context->decOutCallback((uint8_t*)decOut->outputPictureY.busAddress,
                                            (uint8_t*)decOut->outputPictureCbCr.busAddress,
                                            (uint8_t*)decOut->outputPictureCr.busAddress,
                                            imgInfo->outputWidth,
                                            imgInfo->outputHeight - sliceCnt * decIn->sliceMbSet * 16,
                                            VCDEC_OUT_JFRAME);
                }
            }
        }
        else if(ret == JPEGDEC_SCAN_PROCESSED)
        {
            os_printf("JPEGDEC_SCAN_PROCESSED\n");
        }
        else if(ret == JPEGDEC_SLICE_READY)
        {
            sliceCnt++;
            //os_printf("JPEGDEC_SLICE_READY, slice_count = %d\n", sliceCnt);
            if(context->decOutCallback)
            {
                context->decOutCallback((uint8_t*)decOut->outputPictureY.busAddress,
                                        (uint8_t*)decOut->outputPictureCbCr.busAddress,
                                        (uint8_t*)decOut->outputPictureCr.busAddress,
                                        imgInfo->outputWidth,
                                        decIn->sliceMbSet * 16,
                                        VCDEC_OUT_JFRAME);
            }
        }
        else if(ret == JPEGDEC_STRM_PROCESSED)
        {
            os_printf("JPEGDEC_STRM_PROCESSED\n");
        }
        else if(ret == JPEGDEC_STRM_ERROR)
        {
            os_printf("JPEGDEC_STRM_ERROR\n");
            JpegDecAbortAfter(decHandle);
            break;
        }
        else
        {
            os_printf("JPEGDEC_RET = %d\n", ret);
            JpegDecAbortAfter(decHandle);
            break;
        }

    }while(ret != JPEGDEC_FRAME_READY);

	return ret;
}

int32_t jpeg_decoder_reset(void* handle)
{
    return JpegDecAbortAfter(((JPEGDecoderContext*)handle)->decHandle);
}
