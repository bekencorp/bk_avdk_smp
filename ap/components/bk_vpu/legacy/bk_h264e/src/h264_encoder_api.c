#include <stdint.h>
#include <stddef.h>
#include <os/os.h>
#include <modules/h264e/enccfg.h>
#include <modules/h264e/jpegencapi.h>
#include <modules/h264e/hevcencapi.h>
#include "h264_encoder_api.h"
#define  LOW_LATENCY_BUILD_SUPPORT
#include <modules/h264e/encinputlinebuffer.h>
#include "driver/int.h"
#include "driver/sys_pm.h"
#include "sys_driver.h"

#include <components/bk_frame_buffer.h>

#ifndef MEM_CACHABLE_MASK
#define MEM_CACHABLE_MASK ((uint32_t)0)
#endif

extern void enable_irq(void);
extern void disable_irq(void);
extern void hantroenc_isr(void);
extern void vcenc_platform_init(void);
extern void vcenc_platform_deinit(void);

static void h264e_clk_enable(uint8_t enable)
{
	uint32_t REG_SYS_BASE_ADDR = 0x48000000;
	if (enable) {
		uint32_t reg_value = 0;
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26E, PM_POWER_MODULE_STATE_ON);
	
		// h264e clock sel
		sys_drv_h265_cksel_clkdiv_set(CKSEL_H265_160M, 1);
	
		// h264e clock enable
		bk_pm_clock_ctrl(PM_CLK_ID_H26E, PM_CLK_CTRL_PWR_UP);
	} else {
		// h264e clock disable
		bk_pm_clock_ctrl(PM_CLK_ID_H26E, PM_CLK_CTRL_PWR_DOWN);

		// h264e power disable
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_H26E, PM_POWER_MODULE_STATE_OFF);
	}
}

typedef struct _VCEncBuf
{
	uint32_t  width;
	uint32_t  height;
	uint32_t  alignment;
	uint32_t  encPicYSize;
	uint32_t  encPicUVSize;
	uint32_t  encOutPPSize;
	uint8_t   encOutPPS[32];
	uint32_t  encOutBuf;
	uint32_t  encOutSize;
	void	(*encOutCallback)(uint8_t* buf, uint32_t size, uint32_t type);
	VCEncGopPicConfig  gopPicCfg[MAX_GOP_PIC_CONFIG_NUM];
	uint32_t  		   lineBufMode;
	inputLineBufferCfg lineBufCfg;
	VCEncFlexaDoneCallback lineBufDoneCallback;
	VCEncStartCallback encStartCallback;
}VCEncBuf;

typedef struct _H264EncoderContext
{
	VCEncInst encHandle;
	VCEncBuf  encBuf;
	VCEncIn   encIn;
	VCEncOut  encOut;
	uint32_t  errCode;
}H264EncoderContext;

typedef struct _JPEGEncoderContext
{
	JpegEncInst encHandle;
	VCEncBuf    encBuf;
	JpegEncIn   encIn;
	JpegEncOut  encOut;
}JPEGEncoderContext;

static void h264e_int_isr()
{
	//printf("%s, %d\n", __func__, __LINE__);
	hantroenc_isr();
}

void h264e_int_register(void)
{
    bk_int_isr_register(INT_SRC_H26E, (int_group_isr_t)&h264e_int_isr, NULL);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_H26E, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_H26E, 1);
#endif
}

static uint8_t vcenc_platform_inited = 0;

#if 1
void* h264_encode_malloc(size_t size) {
	return bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, size);
}

void h264_encode_free(void* pbuf){
	bk_frame_buffer_free(pbuf);
}
#else
void* h264_encode_malloc(size_t size) {
	return os_malloc(size);
}

void h264_encode_free(void* pbuf){
	os_free(pbuf);
}
#endif

static void __vcenc_platform_init(void)
{
	if(vcenc_platform_inited == 0)
	{
		h264e_clk_enable(1);
		h264e_int_register();
		vcenc_platform_init();

		vcenc_memalloc_register(h264_encode_malloc, h264_encode_free);
	}
	vcenc_platform_inited++;
}

static void __vcenc_platform_deinit(void)
{
	if(vcenc_platform_inited)
	{
		vcenc_platform_inited--;
		if(vcenc_platform_inited == 0)
		{
			vcenc_platform_deinit();
			h264e_clk_enable(0);
		}
	}
}

static void vcenc_flexa_input_linebuf_done(void *pAppData)
{
	//bk_printf("%s\n", __func__);

	VCEncBuf* encBuf = (VCEncBuf*)pAppData;

	inputLineBufferCfg* lineBufCfg = &encBuf->lineBufCfg;

	if(encBuf->lineBufDoneCallback)
	{
		if(encBuf->lineBufDoneCallback(lineBufCfg->lumBuf.buf, lineBufCfg->cbBuf.buf, lineBufCfg->crBuf.buf))
		{
			lineBufCfg->wrCnt++;
			VCEncInputLineBufWrCntSet(lineBufCfg);
		}
	}
}

static void vcenc_outbuf_update(void* handle, uint32_t type, uint32_t outBuf, uint32_t outSize)
{
	VCEncBuf* encBuf = &((H264EncoderContext*)handle)->encBuf;

	if(outBuf)
	{
		if(outSize)
		{
			if(encBuf->encOutBuf) os_free((void*)encBuf->encOutBuf);
			encBuf->encOutSize = outSize;
			encBuf->encOutBuf  = 0;
		}
		else
		{
			return;
		}		
	}
	else
	{
		if(outSize)
		{
			if(encBuf->encOutBuf == 0)
			{
				encBuf->encOutSize = outSize;
				encBuf->encOutBuf  = (uint32_t)os_malloc(encBuf->encOutSize);
			}
		else if(outSize != encBuf->encOutSize)
			{
				os_free((void*)encBuf->encOutBuf);
				encBuf->encOutSize = outSize;
				encBuf->encOutBuf  = (uint32_t)os_malloc(encBuf->encOutSize);
			}
		}
		else
		{
			if(encBuf->encOutBuf == 0)
			{
				encBuf->encOutSize = encBuf->width * encBuf->height;
				encBuf->encOutBuf  = (uint32_t)os_malloc(encBuf->encOutSize);
			}
		}
	}

	if(type == 0)
	{
		VCEncIn*  encIn      = &((H264EncoderContext*)handle)->encIn;
		encIn->pOutBuf[0]    = (u32*)(outBuf ? outBuf : encBuf->encOutBuf | MEM_CACHABLE_MASK);
		encIn->busOutBuf[0]  = (ptr_t)encIn->pOutBuf[0];
		encIn->outBufSize[0] = encBuf->encOutSize;
	}
	else
	{
		JpegEncIn*  encIn    = &((JPEGEncoderContext*)handle)->encIn;
		encIn->pOutBuf[0]    = (u8*)(outBuf ? outBuf : encBuf->encOutBuf | MEM_CACHABLE_MASK);
		encIn->busOutBuf[0]  = (ptr_t)encIn->pOutBuf[0];
		encIn->outBufSize[0] = encBuf->encOutSize;
	}
}

void vcenc_read_yuv420_planar_frame(uint32_t dest, uint32_t picture, uint32_t width, uint32_t height, uint32_t alignment)
{
	uint32_t h;
	uint32_t lumaStride   = STRIDE(width, alignment);
	uint32_t chromaStride = STRIDE(width / 2, alignment);

	uint8_t* src = (uint8_t*)picture;
	uint8_t* dst = (uint8_t*)dest;

	for (h = 0; h < height; h++) {
		memcpy(dst, src, width);
		src += width;
		dst += lumaStride;
	}

	width  = (width + 1) >> 1;
	height = (height + 1) >> 1;

	for (h = 0; h < height; h++) {
		os_memcpy(dst, src, width);
		src += width;
		dst += chromaStride;
	}

	for (h = 0; h < height; h++) {
		os_memcpy(dst, src, width);
		src += width;
		dst += chromaStride;
	}
}

void vcenc_read_yuv420_semi_planar_frame(uint32_t dest, uint32_t picture, uint32_t width, uint32_t height, uint32_t alignment)
{
#if 1
	__maybe_unused uint32_t i, w, h;
	uint32_t lumaStride   = STRIDE(width, alignment);
  	uint32_t chromaStride = STRIDE(width / 2, alignment) * 2;

	uint8_t* src = (uint8_t*)picture;
	uint8_t* dst = (uint8_t*)dest;

	for(h = 0; h < height; h++)
	{
		memcpy(dst, src, width);
		src += width;
		dst += lumaStride;
	}

	height = (height + 1) >> 1;

	for(h = 0; h < height; h++)
	{
		memcpy(dst, src, width);
		src += width;
		dst += chromaStride;
	}
#else
	uint32_t w, h;
	uint32_t lumaStride   = STRIDE(width, alignment);
  	uint32_t chromaStride = STRIDE(width / 2, alignment);

	uint8_t* src = (uint8_t*)picture;
	uint8_t* dst = (uint8_t*)dest;
	uint8_t* pu;
	uint8_t* pv;

	for(h = 0; h < height; h++)
	{
		memcpy(dst, src, width);
		src += width;
		dst += lumaStride;
	}

	width  = (width + 1) >> 1;
	height = (height + 1) >> 1;

	pu = dst;
	pv = dst + chromaStride * height;

	for(h = 0; h < height; h++)
	{
		for(w = 0; w < width; w++)
		{
			pu[w] = *src++;
			pv[w] = *src++;
		}

		pu += chromaStride;
		pv += chromaStride;
	}
#endif
}

void vcenc_flexa_input_linebuf_wrcnt_set(void* handle, uint32_t wrcnt)
{
	if(*(volatile uint32_t*)(ENCH2_ASIC_BASE_ADDR + 5 * 4) & 1)
	{
		inputLineBufferCfg* lineBufCfg = &((H264EncoderContext*)handle)->encBuf.lineBufCfg;
		lineBufCfg->wrCnt = wrcnt;
		VCEncInputLineBufWrCntSet(lineBufCfg);
	}
}

void vcenc_asic_encode_start_callback_set(void* handle, VCEncStartCallback scb)
{
	((H264EncoderContext*)handle)->encBuf.encStartCallback = scb;
}

void vcenc_callback_exec(void* pAppData, uint32_t id)
{
	if(pAppData)
	{
		VCEncBuf* encBuf = (VCEncBuf*)pAppData;

		switch(id)
		{
		case 1:
			if(encBuf->encStartCallback) encBuf->encStartCallback();
			break;
		default:
			break;
		}
	}
}

void vcenc_pic_cfg_init(VCEncIn *encIn)
{
	i32 i, j, k, i32Poc;
	i32 i32MaxpicOrderCntLsb = 1 << 16;

	ASSERT(encIn != NULL);

	encIn->gopCurrPicConfig.codingType = 0;//FRAME_TYPE_RESERVED;
	encIn->gopCurrPicConfig.nonReference = FRAME_TYPE_RESERVED;
	encIn->gopCurrPicConfig.numRefPics = NUMREFPICS_RESERVED;
	encIn->gopCurrPicConfig.poc = -1;
	encIn->gopCurrPicConfig.QpFactor = QPFACTOR_RESERVED;
	encIn->gopCurrPicConfig.QpOffset = QPOFFSET_RESERVED;
	encIn->gopCurrPicConfig.temporalId = 0;
	encIn->i8SpecialRpsIdx = -1;

	for (k = 0; k < VCENC_MAX_REF_FRAMES; k++)
	{
		encIn->gopCurrPicConfig.refPics[k].ref_pic = INVALITED_POC;
		encIn->gopCurrPicConfig.refPics[k].used_by_cur = 0;
	}

	for (k = 0; k < VCENC_MAX_LT_REF_FRAMES; k++) encIn->long_term_ref_pic[k] = INVALITED_POC;

	encIn->bIsPeriodUsingLTR = HANTRO_FALSE;
	encIn->bIsPeriodUpdateLTR = HANTRO_FALSE;

	for (i = 0; i < encIn->gopConfig.special_size; i++)
	{
		if (encIn->gopConfig.pGopPicSpecialCfg[i].i32Interval <= 0) continue;

		if (encIn->gopConfig.pGopPicSpecialCfg[i].i32Ltr == 0)
			encIn->bIsPeriodUsingLTR = HANTRO_TRUE;
		else
		{
			encIn->bIsPeriodUpdateLTR = HANTRO_TRUE;

			for (k = 0; k < (i32)encIn->gopConfig.pGopPicSpecialCfg[i].numRefPics; k++)
			{
				i32 i32LTRIdx = encIn->gopConfig.pGopPicSpecialCfg[i].refPics[k].ref_pic;
				if ((IS_LONG_TERM_REF_DELTAPOC(i32LTRIdx)) && ((encIn->gopConfig.pGopPicSpecialCfg[i].i32Ltr - 1) == LONG_TERM_REF_DELTAPOC2ID(i32LTRIdx)))
				{
					encIn->bIsPeriodUsingLTR = HANTRO_TRUE;
				}
			}
		}
	}

	memset(encIn->bLTR_need_update, 0, sizeof(u32) * VCENC_MAX_LT_REF_FRAMES);
	encIn->bIsIDR = HANTRO_TRUE;

	i32Poc = 0;
	/* check current picture encoded as LTR*/
	encIn->u8IdxEncodedAsLTR = 0;
	for (j = 0; j < encIn->gopConfig.special_size; j++)
	{
		if (encIn->bIsPeriodUsingLTR == HANTRO_FALSE) break;

		true_e bLTRUpdatePeriod = encIn->gopConfig.pGopPicSpecialCfg[j].i32Interval > 0;
		true_e bLTRUpdateOneTimes =
			(encIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr > 0) &&
			(encIn->gopConfig.pGopPicSpecialCfg[j].i32Interval == 0) &&
			(encIn->long_term_ref_pic[encIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr - 1] == INVALITED_POC);

		if (!(bLTRUpdatePeriod || bLTRUpdateOneTimes) || (encIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr == 0)) continue;

		i32Poc = i32Poc - encIn->gopConfig.pGopPicSpecialCfg[j].i32Offset;

		if (i32Poc < 0)
		{
			i32Poc += i32MaxpicOrderCntLsb;
			if (i32Poc > (i32MaxpicOrderCntLsb >> 1)) i32Poc = -1;
		}

		i32 interval = encIn->gopConfig.pGopPicSpecialCfg[j].i32Interval ? encIn->gopConfig.pGopPicSpecialCfg[j].i32Interval : i32MaxpicOrderCntLsb;

		if ((i32Poc >= 0) && (i32Poc % interval == 0))
		{
			/* more than one LTR at the same frame position */
			if (0 != encIn->u8IdxEncodedAsLTR)
			{
				// reuse the same POC LTR
				encIn->bLTR_need_update[encIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr - 1] = HANTRO_TRUE;
				continue;
			}

			encIn->gopCurrPicConfig.codingType = ((i32)encIn->gopConfig.pGopPicSpecialCfg[j].codingType == FRAME_TYPE_RESERVED) ? encIn->gopCurrPicConfig.codingType : encIn->gopConfig.pGopPicSpecialCfg[j].codingType;
			encIn->gopCurrPicConfig.nonReference = ((i32)encIn->gopConfig.pGopPicSpecialCfg[j].nonReference == FRAME_TYPE_RESERVED) ? encIn->gopCurrPicConfig.nonReference : encIn->gopConfig.pGopPicSpecialCfg[j].nonReference;
			encIn->gopCurrPicConfig.numRefPics = ((i32)encIn->gopConfig.pGopPicSpecialCfg[j].numRefPics == NUMREFPICS_RESERVED) ? encIn->gopCurrPicConfig.numRefPics : encIn->gopConfig.pGopPicSpecialCfg[j].numRefPics;
			encIn->gopCurrPicConfig.QpFactor = (encIn->gopConfig.pGopPicSpecialCfg[j].QpFactor == QPFACTOR_RESERVED) ? encIn->gopCurrPicConfig.QpFactor : encIn->gopConfig.pGopPicSpecialCfg[j].QpFactor;
			encIn->gopCurrPicConfig.QpOffset = (encIn->gopConfig.pGopPicSpecialCfg[j].QpOffset == QPOFFSET_RESERVED) ? encIn->gopCurrPicConfig.QpOffset : encIn->gopConfig.pGopPicSpecialCfg[j].QpOffset;
			encIn->gopCurrPicConfig.temporalId = (encIn->gopConfig.pGopPicSpecialCfg[j].temporalId == TEMPORALID_RESERVED) ? encIn->gopCurrPicConfig.temporalId : encIn->gopConfig.pGopPicSpecialCfg[j].temporalId;

			if (((i32)encIn->gopConfig.pGopPicSpecialCfg[j].numRefPics != NUMREFPICS_RESERVED))
			{
				for (k = 0; k < (i32)encIn->gopCurrPicConfig.numRefPics; k++)
				{
					encIn->gopCurrPicConfig.refPics[k].ref_pic = encIn->gopConfig.pGopPicSpecialCfg[j].refPics[k].ref_pic;
					encIn->gopCurrPicConfig.refPics[k].used_by_cur = encIn->gopConfig.pGopPicSpecialCfg[j].refPics[k].used_by_cur;
				}
			}

			encIn->bLTR_need_update[encIn->u8IdxEncodedAsLTR - 1] = HANTRO_TRUE;
			encIn->u8IdxEncodedAsLTR = encIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr;
		}
	}

	encIn->poc = 0;
	encIn->timeIncrement = 0;
	encIn->last_idr_picture_cnt = encIn->picture_cnt = encIn->picture_gopIdx = 0;
}

int32_t h264_encoder_init(void** handle, uint32_t width, uint32_t height, uint32_t flexaMode, VCEncFlexaDoneCallback fcb, VCEncOutCallback ocb)
{
	VCEncRet                ret;
	VCEncConfig             cfg;
	VCEncCodingCtrl         cCfg;
	VCEncRateCtrl           rcCfg;
	VCEncInst               encoder = NULL;
	VCEncGopPicConfig*      gpCfg;
	VCEncPreProcessingCfg   preProcCfg;

	i32 tile_width_buffer[1]  = {0};
	i32 tile_height_buffer[1] = {0};

	H264EncoderContext* context = (H264EncoderContext*)os_malloc(sizeof(H264EncoderContext));

	if(!context) return VCENC_MEMORY_ERROR;

	*handle = context;

	VCEncInst* encHandle = &context->encHandle;
	VCEncIn*   encIn     = &context->encIn;
	VCEncOut*  encOut    = &context->encOut;
	VCEncBuf*  encBuf    = &context->encBuf;

	__vcenc_platform_init();

	os_memset(&cfg,   0, sizeof(cfg));
	os_memset(&cCfg,  0, sizeof(cCfg));
	os_memset(&rcCfg, 0, sizeof(rcCfg));
	os_memset(encIn,  0, sizeof(VCEncIn));
	os_memset(encBuf, 0, sizeof(VCEncBuf));

    /* Step 1: Initialize an encoder instance */
    cfg.frameRateDenom   = 1;
    cfg.frameRateNum     = 30;
	cfg.width            = width;
	cfg.height           = height;
	cfg.streamType       = VCENC_BYTE_STREAM;
    cfg.codecFormat      = VCENC_VIDEO_CODEC_H264;
    cfg.level            = VCENC_H264_LEVEL_1_3;//VCENC_H264_LEVEL_5_1;//VCENC_H264_LEVEL_1;
	cfg.parallelCoreNum  = 1;
	cfg.bitDepthLuma     = 8;
	cfg.bitDepthChroma   = 8;
	cfg.profile 		 = VCENC_H264_MAIN_PROFILE;//VCENC_H264_BASE_PROFILE VCENC_H264_MAIN_PROFILE VCENC_H264_HIGH_PROFILE
	cfg.codedChromaIdc   = VCENC_CHROMA_IDC_420;//VCENC_CHROMA_IDC_422
	cfg.maxTLayers       = 1;
	cfg.gopSize          = 1;
	cfg.refFrameAmount   = 1;
	cfg.refRingBufEnable = 1;
	cfg.numRefP          = 1;
	cfg.compressor		 = 3;//3 for both Y&UV
	cfg.writeReconToDDR  = 1;//???
	cfg.dumpCuInfo       = 0;
	cfg.cuInfoVersion    = -1;//??? -1 or 1
	cfg.inLoopDSRatio    = 1;

	//cfg.enableSsim 		 = 1;
	//cfg.enablePsnr 		 = 1;
	//cfg.log2MaxPicOrderCntLsb = 16;
	//cfg.log2MaxFrameNum  = 12;
	//cfg.inLoopDSRatio    = 1;
	//cfg.extSramLumHeightBwd = 16;
	//cfg.extSramChrHeightBwd = 8;
	//cfg.extSramLumHeightFwd = 16;
	//cfg.extSramChrHeightFwd = 8;

	cfg.num_tile_columns    = 1;
	cfg.num_tile_rows       = 1;
	cfg.tile_width          = tile_width_buffer;
	cfg.tile_height         = tile_height_buffer;
	cfg.tiles_enabled_flag  = 0;

	//cfg.av1InterFiltSwitch  = 1;
	//cfg.tune = VCENC_TUNE_PSNR;
	//cfg.loop_filter_across_tiles_enabled_flag = 0;

	if ((ret = VCEncInit(&cfg, &encoder, NULL)) != VCENC_OK) {
		bk_printf("VCEncInit falied with error code %d\r\n", ret);
		goto __END;
	}

	*encHandle = encoder;

	/* Step 2: Optional extra encoder configuration See the next example code for how to change the default encHandle parameters */
	VCEncGetCodingCtrl(encoder, &cCfg);
	cCfg.enableSao = 0;
	ret = VCEncSetCodingCtrl(encoder, &cCfg);
    bk_printf("%s %d %d\n", __func__, __LINE__, ret);

	VCEncGetPreProcessing(encoder, &preProcCfg);
	preProcCfg.inputType = VCENC_YUV420_SEMIPLANAR;//VCENC_YUV420_PLANAR;//VCENC_YUV420_SEMIPLANAR;
	VCEncSetPreProcessing(encoder, &preProcCfg);

	/* Step 3: Allocate linear memory resources. This implementation is OS specific and not part of this example. */
	/* Picture buffer size in YUV420 and Output buffer size */
	encBuf->width          = width;
	encBuf->height         = height;
	encBuf->alignment	   = 64;
	encBuf->encPicYSize    = STRIDE(width, encBuf->alignment) * height;
	encBuf->encPicUVSize   = STRIDE(width / 2, encBuf->alignment) * height / 2 * 2;
	encBuf->encOutSize     = 0;
	encBuf->encOutBuf      = 0;
	encBuf->encOutCallback = ocb;
	encBuf->lineBufMode	   = 0;
	encBuf->lineBufDoneCallback = 0;

	void* ppsBuf = os_malloc(VCENC_STREAM_MIN_BUF0_SIZE);

	encIn->pOutBuf[0] = (u32*)ppsBuf;
	encIn->busOutBuf[0] = (ptr_t)encIn->pOutBuf[0];
	encIn->outBufSize[0] = VCENC_STREAM_MIN_BUF0_SIZE;
	encIn->dec400Enable = 1;//1: bypass 2: enable
	encIn->gopSize = 1;//FIXME@TODO
	encIn->gopConfig.idr_interval = 30;
	encIn->gopConfig.pGopPicCfg = encBuf->gopPicCfg;
	encIn->gopConfig.special_size = 0;
	encIn->gopConfig.ltrcnt = 0;
	/*
	 * Initialize GOP frame rate fields.
	 *
	 * VCEncStrmEncodeExt() calculates the current bitrate using
	 * pEncIn->gopConfig.outputRateNumer/outputRateDenom. If these fields are left
	 * as zero, the division will trigger a UsageFault (DIVBYZERO) at runtime.
	 */
	encIn->gopConfig.outputRateNumer = cfg.frameRateNum;
	encIn->gopConfig.outputRateDenom = cfg.frameRateDenom;
	encIn->gopConfig.inputRateNumer = cfg.frameRateNum;
	encIn->gopConfig.inputRateDenom = cfg.frameRateDenom;
	/* gopCfgOffset: keep record of gopSize's offset in pGopPicCfg */
	encIn->gopConfig.gopCfgOffset[0] = encIn->gopConfig.size;
	/* gop_config : #Frame1: P 1 0 0.578 0 1 -1 1 */
	gpCfg = &(encIn->gopConfig.pGopPicCfg[encIn->gopConfig.size++]);
	gpCfg->poc = 1;
	gpCfg->QpOffset = 0;
	gpCfg->QpFactor = 0.760263;  //sqrt(0.578)
	gpCfg->temporalId = 0;
	gpCfg->codingType = 1;
	gpCfg->nonReference = 0;
	gpCfg->numRefPics = 1;
	gpCfg->refPics->ref_pic = -1;
	gpCfg->refPics->used_by_cur = 1;

	for(int i = 0; i < VCENC_MAX_LT_REF_FRAMES; i++) encIn->long_term_ref_pic[i] = INVALITED_POC;

	/* Step 4: Start the stream */
	if ((ret = VCEncStrmStart(encoder, encIn, encOut)) != VCENC_OK) {
		bk_printf("VCEncStrmStart falied with error code %d\r\n", ret);
		goto __END;
	}

	if(flexaMode)
	{
		inputLineBufferCfg* lineBufCfg = &encBuf->lineBufCfg;

		encBuf->lineBufMode = flexaMode;
		encBuf->lineBufDoneCallback = fcb;

		/* Step 5: low latency setting */
		VCEncGetCodingCtrl(encoder, &cCfg);
		cCfg.inputLineBufEn = 1;
		cCfg.inputLineBufLoopBackEn = 1;
		cCfg.inputLineBufDepth = 1;
		cCfg.amountPerLoopBack = flexaMode == VCENC_FLEXA_MODE_SOFTWARE ? 3 : 3;
		cCfg.inputLineBufHwModeEn = flexaMode == VCENC_FLEXA_MODE_HARDWARE;
		cCfg.inputLineBufCbFunc = vcenc_flexa_input_linebuf_done;
		cCfg.inputLineBufCbData = encBuf;
		cCfg.sbi_id_0 = FLEXA_STREAM_ID_Y;
		cCfg.sbi_id_1 = FLEXA_STREAM_ID_CB;
		cCfg.sbi_id_2 = FLEXA_STREAM_ID_CR;
		cCfg.segmentUnitHeight = 16;
		cCfg.lowlatGatingDisable = 0;
		cCfg.lowlatGatingType = 0;
		cCfg.lowlatGatingCyc  = IS_H264(VCENC_VIDEO_CODEC_H264) ? 7 : 31;
		cCfg.enable_slice_irq = 0;
		#ifdef LOW_LATENCY_SLICEINFO_SUPPORT
		/* poll input sliceinfo for low latency */
		cCfg.inputSliceInfoPollEn = 0;
		#endif
		VCEncSetCodingCtrl(encoder, &cCfg);
        ret = VCEncGetCodingCtrl(encoder, &cCfg);
        bk_printf("%s %d %p %p %d\n", __func__, __LINE__, cCfg.inputLineBufCbFunc, vcenc_flexa_input_linebuf_done, ret);

		lineBufCfg->depth = cCfg.inputLineBufDepth;
		lineBufCfg->hwHandShake = cCfg.inputLineBufHwModeEn;
		lineBufCfg->loopBackEn  = cCfg.inputLineBufLoopBackEn;
		lineBufCfg->amountPerLoopBack = cCfg.amountPerLoopBack;
		lineBufCfg->initSegNum = 0;
		lineBufCfg->inst  = (void*)encoder;
		lineBufCfg->wrCnt = 0;
		lineBufCfg->inputFormat  = preProcCfg.inputType;
		lineBufCfg->lumaStride   = STRIDE(width, encBuf->alignment);
		lineBufCfg->chromaStride = STRIDE(width / 2, encBuf->alignment);
		lineBufCfg->encWidth     = width;
		lineBufCfg->encHeight    = height;
		lineBufCfg->srcHeight    = height;
		lineBufCfg->srcVerOffset = 0;
		lineBufCfg->getMbLines   = &VCEncGetEncodedMbLines;
		lineBufCfg->setMbLines   = &VCEncSetInputMBLines;
		lineBufCfg->ctbSize      = 16;
		lineBufCfg->lumSrc       = NULL;
		lineBufCfg->cbSrc        = NULL;
		lineBufCfg->crSrc        = NULL;
		lineBufCfg->client_type  = VCEncGetClientType(VCENC_VIDEO_CODEC_H264);

		if(VCEncInitInputLineBuffer(lineBufCfg))
		{
			bk_printf("VCEncInitInputLineBuffer failed\n");
			goto __END;
		}

		/* loopback mode */
		if(1 && lineBufCfg->loopBackEn /*&& lineBufCfg->lumBuf.buf*/)
		{
			encIn->busLuma    = lineBufCfg->lumBuf.busAddress;
			encIn->busChromaU = lineBufCfg->cbBuf.busAddress;
			encIn->busChromaV = lineBufCfg->crBuf.busAddress;

			for(int tileId = 1; tileId < cfg.num_tile_columns; tileId++)
			{
				encIn->tileExtra[tileId - 1].busLuma    = lineBufCfg->lumBuf.busAddress;
				encIn->tileExtra[tileId - 1].busChromaU = lineBufCfg->cbBuf.busAddress;
				encIn->tileExtra[tileId - 1].busChromaV = lineBufCfg->crBuf.busAddress;
			}

			/* In loop back mode, data in line buffer start from the line to be encoded*/
			VCEncPreProcessingCfg preProcCfg;
			VCEncGetPreProcessing(encoder, &preProcCfg);
			for(int tileId = 0; tileId < cfg.num_tile_columns; tileId++)
			{
				u32 *yOffset = (tileId == 0) ? (&preProcCfg.yOffset) : (&preProcCfg.tileExtra[tileId - 1].yOffset);
				*yOffset = 0;
			}
			VCEncSetPreProcessing(encoder, &preProcCfg);
		}
	}

	encBuf->encOutPPSize = encOut->streamSize;

	if (encOut->streamSize > sizeof(encBuf->encOutPPS))
	{
		bk_printf("VCEncPPS overflow\n");
	}
	else
	{
		os_memcpy(encBuf->encOutPPS, ppsBuf, encOut->streamSize);
	}

	if(ppsBuf) os_free(ppsBuf);

	if (encBuf->encOutCallback)
	{
		encBuf->encOutCallback((uint8_t*)encIn->pOutBuf[0], encOut->streamSize, VCENC_OUT_HEADER);
	}

__END:
	return ret;
}

int32_t h264_encoder_deinit(void* handle)
{
	VCEncRet ret;

	H264EncoderContext* context = (H264EncoderContext*)handle;

	VCEncInst  encHandle = context->encHandle;
	VCEncIn*   encIn     = &context->encIn;
	VCEncOut*  encOut    = &context->encOut;
	VCEncBuf*  encBuf    = &context->encBuf;

	if (encHandle) {
		ret = VCEncFlush(encHandle, encIn, encOut, NULL, NULL);

		ret = VCEncStrmEnd(encHandle, encIn, encOut);
		if (ret != VCENC_OK) {
			bk_printf("VCEncStrmEnd falied with error code %d\r\n", ret);
		} else {
			if (encBuf->encOutCallback) {
				encBuf->encOutCallback((uint8_t*)encIn->pOutBuf[0], encOut->streamSize, VCENC_OUT_ENDING);
			}
		}

		if(encBuf->encOutBuf) os_free((void*)encBuf->encOutBuf);
	
		/* Last Step: Release the encHandle instance */
		if ((ret = VCEncRelease(encHandle)) != VCENC_OK) {
			bk_printf("VCEncRelease falied with error code %d\r\n", ret);
		}
	}
	else
	{
		ret = VCENC_INSTANCE_ERROR;
	}

	os_free(context);

	__vcenc_platform_deinit();

	return ret;
}

int32_t h264_encoder_pps_data_get(void* handle, uint8_t** data, uint32_t* size)
{
	VCEncBuf* encBuf = &((H264EncoderContext*)handle)->encBuf;

	if(*data)
	{
		if(*size >= encBuf->encOutPPSize)
		{
			*size = encBuf->encOutPPSize;
			os_memcpy(*data, encBuf->encOutPPS, encBuf->encOutPPSize);
		}
		else
		{
			*size = 0;
			return VCENC_OUTPUT_BUFFER_OVERFLOW;
		}
	}
	else
	{
		*data = encBuf->encOutPPS;
		*size = encBuf->encOutPPSize;
	}

	return 0;
}

int32_t h264_encoder_encode(void* handle, uint32_t picBuf, uint32_t picLines, uint32_t codingType, uint32_t outBuf, uint32_t outSize)
{
	VCEncRet ret;

	H264EncoderContext* context = (H264EncoderContext*)handle;

	VCEncInst  encHandle = context->encHandle;
	VCEncIn*   encIn     = &context->encIn;
	VCEncOut*  encOut    = &context->encOut;
	VCEncBuf*  encBuf    = &context->encBuf;

	if(!context || !encHandle) {
		return VCENC_INSTANCE_ERROR;
	}

	vcenc_outbuf_update(handle, 0, outBuf, outSize);

	encIn->bIsIDR = encIn->picture_cnt == 0 || context->errCode != VCENC_FRAME_READY;
	encIn->timeIncrement = encIn->picture_cnt != 0;
	encIn->codingType    = codingType;

	if(encBuf->lineBufMode)
	{
		inputLineBufferCfg* lineBufCfg = &encBuf->lineBufCfg;

		if(lineBufCfg->loopBackEn /*&& lineBufCfg->lumBuf.buf*/)
		{
			#if 0
			for(int tileId = 1; tileId < cfg.num_tile_columns; tileId++)
			{
				encIn->tileExtra[tileId - 1].busLuma    = lineBufCfg->lumBuf.busAddress;
				encIn->tileExtra[tileId - 1].busChromaU = lineBufCfg->cbBuf.busAddress;
				encIn->tileExtra[tileId - 1].busChromaV = lineBufCfg->crBuf.busAddress;
			}

			/* In loop back mode, data in line buffer start from the line to be encoded*/
			VCEncPreProcessingCfg preProcCfg;
			VCEncGetPreProcessing(encoder, &preProcCfg);
			for(int tileId = 0; tileId < cfg.num_tile_columns; tileId++)
			{
				u32 *yOffset = (tileId == 0) ? (&preProcCfg.yOffset) : (&preProcCfg.tileExtra[tileId - 1].yOffset);
				*yOffset = 0;
			}
			VCEncSetPreProcessing(encoder, &preProcCfg);
			#endif
		}

		lineBufCfg->lumSrc  = (u8*)picBuf;
		lineBufCfg->cbSrc   = lineBufCfg->lumSrc + encBuf->encPicYSize;
		lineBufCfg->crSrc   = lineBufCfg->cbSrc  + encBuf->encPicUVSize / 2;
		lineBufCfg->wrCnt   = 0;
		lineBufCfg->sram    = (u8*)picBuf;
		lineBufCfg->sramBusAddr = picBuf;
		lineBufCfg->sramSize= lineBufCfg->depth * lineBufCfg->amountPerLoopBack * 16 * lineBufCfg->encWidth * 3 / 2;
		encIn->lineBufWrCnt = VCEncStartInputLineBufferWithoutCopy(lineBufCfg, picLines);
		encIn->initSegNum   = lineBufCfg->initSegNum;
		encIn->busLuma      = lineBufCfg->lumBuf.busAddress;
		encIn->busChromaU   = lineBufCfg->cbBuf.busAddress;
		encIn->busChromaV   = lineBufCfg->crBuf.busAddress;
	}
	else
	{
		encIn->busLuma    = picBuf;
		encIn->busChromaU = encIn->busLuma + encBuf->encPicYSize;
		encIn->busChromaV = encIn->busChromaU + encBuf->encPicUVSize / 2;
	}

	ret = VCEncStrmEncode(encHandle, encIn, encOut, NULL, encBuf);

	switch (ret) {
		case VCENC_FRAME_ENQUEUE:
			encIn->picture_cnt++;
			break;

		case VCENC_FRAME_READY:
			if(encBuf->lineBufMode) VCEncUpdateInitSegNum(&encBuf->lineBufCfg);
			if(encBuf->encOutCallback) {
				encBuf->encOutCallback((uint8_t*)encIn->pOutBuf[0], encOut->streamSize, encOut->codingType);
			}

			if(encOut->codingType != VCENC_NOTCODED_FRAME) {
				encIn->picture_cnt++;
			}
			break;

		default:
			break;
	}

	context->errCode = ret;

	return ret;
}

int32_t jpeg_encoder_init(void**handle, uint32_t width, uint32_t height, uint32_t flexaMode, VCEncFlexaDoneCallback fcb, VCEncOutCallback ocb)
{
	JpegEncRet  ret;
    JpegEncCfg  cfg;
	JpegEncInst encoder;

	JPEGEncoderContext* context = (JPEGEncoderContext*)os_malloc(sizeof(JPEGEncoderContext));

	if(!context) return VCENC_MEMORY_ERROR;

	*handle = context;

	JpegEncInst* encHandle = &context->encHandle;
	JpegEncIn*   encIn     = &context->encIn;
	JpegEncOut*  encOut    = &context->encOut;
	VCEncBuf*    encBuf    = &context->encBuf;

	__vcenc_platform_init();

	os_memset(&cfg,   0, sizeof(cfg));
	os_memset(encIn,  0, sizeof(JpegEncIn));
	os_memset(encOut, 0, sizeof(JpegEncOut));
	os_memset(encBuf, 0, sizeof(VCEncBuf));

    /* Step 1: Initialize an encoder instance */
	cfg.quality		= 50;
    cfg.qLevel 		= 5;
	cfg.fixedQP		= -1;
	cfg.frameType 	= JPEGENC_YUV420_SEMIPLANAR;
	cfg.markerType 	= JPEGENC_SINGLE_MARKER;
	cfg.unitsType 	= JPEGENC_DOTS_PER_INCH;
	cfg.xDensity 	= 72;
	cfg.yDensity 	= 72;

    if (flexaMode)
    {
        cfg.inputLineBufEn = 1;
        cfg.inputLineBufLoopBackEn = 1;
        cfg.inputLineBufDepth = 1;
		cfg.amountPerLoopBack = flexaMode == VCENC_FLEXA_MODE_SOFTWARE ? 3 : 3;
        cfg.inputLineBufHwModeEn = flexaMode == VCENC_FLEXA_MODE_HARDWARE;
        cfg.inputLineBufCbFunc = vcenc_flexa_input_linebuf_done;
        cfg.inputLineBufCbData = encBuf;
        cfg.sbi_id_0 = FLEXA_STREAM_ID_Y;
        cfg.sbi_id_1 = FLEXA_STREAM_ID_CB;
        cfg.sbi_id_2 = FLEXA_STREAM_ID_CR;
        cfg.segmentUnitHeight = 16;
		cfg.lowlatGatingDisable = 0;
    }

	if((ret = JpegEncInit(&cfg, &encoder, NULL)) != JPEGENC_OK)
    {
        bk_printf("JpegEncInit falied with error code %d\n", ret);
        goto __END;
    }

	*encHandle = encoder;

    /* Step 2: Configuration of picture size */
	/* VGA resolution, no cropping */
	cfg.inputWidth   = width;
	cfg.codingWidth  = width;
	cfg.inputHeight  = height;
	cfg.codingHeight = height;
	cfg.xOffset 	 = 0;
	cfg.yOffset 	 = 0;
	cfg.rotation 	 = JPEGENC_ROTATE_0;
	cfg.codingType 	 = JPEGENC_WHOLE_FRAME;
	/* Restart interval 5 MCU rows (= 80 pixel rows), average quantization, encode whole frame at once */
	cfg.restartInterval = 5;
	if((ret = JpegEncSetPictureSize(encoder, &cfg)) != JPEGENC_OK)
	{
		bk_printf("JpegEncSetPictureSize falied with error code %d\n", ret);
		goto __END;
	}
 
 	/* Step 3: Allocate linear output buffer */
	encBuf->width        = width;
	encBuf->height       = height;
	encBuf->alignment    = 8;
    encBuf->encPicYSize  = STRIDE(width, encBuf->alignment) * height;
	encBuf->encPicUVSize = STRIDE(width / 2, encBuf->alignment) * height / 2 * 2;
	encBuf->encOutSize   = 0;
	encBuf->encOutBuf    = 0;
	encBuf->encOutCallback = ocb;

	encIn->dec400Enable  = 1;//1: bypass 2: enable
	encIn->pOutBuf[0]    = 0;
	encIn->busOutBuf[0]  = (ptr_t)encIn->pOutBuf[0];
    encIn->outBufSize[0] = encBuf->encOutSize;

	if(flexaMode)
	{
		inputLineBufferCfg* lineBufCfg = &encBuf->lineBufCfg;

		encBuf->lineBufMode = flexaMode;
		encBuf->lineBufDoneCallback = fcb;

		lineBufCfg->depth = cfg.inputLineBufDepth;
		lineBufCfg->hwHandShake = cfg.inputLineBufHwModeEn;
		lineBufCfg->loopBackEn  = cfg.inputLineBufLoopBackEn;
		lineBufCfg->amountPerLoopBack = cfg.amountPerLoopBack;
		lineBufCfg->initSegNum = 0;
		lineBufCfg->inst  = (void*)encoder;
		lineBufCfg->wrCnt = 0;
		lineBufCfg->inputFormat  = cfg.frameType;
		lineBufCfg->lumaStride   = STRIDE(width, encBuf->alignment);
		lineBufCfg->chromaStride = STRIDE(width / 2, encBuf->alignment);
		lineBufCfg->encWidth     = width;
		lineBufCfg->encHeight    = height;
		lineBufCfg->srcHeight    = height;
		lineBufCfg->srcVerOffset = 0;
		/* JPEG flexa path must bind JPEG line control APIs. */
		lineBufCfg->getMbLines   = &JpegEncGetEncodedMbLines;
		lineBufCfg->setMbLines   = &JpegEncSetInputMBLines;
		lineBufCfg->ctbSize      = 16;
		lineBufCfg->lumSrc       = NULL;
		lineBufCfg->cbSrc        = NULL;
		lineBufCfg->crSrc        = NULL;
		lineBufCfg->client_type  = EWL_CLIENT_TYPE_JPEG_ENC;

		if(VCEncInitInputLineBuffer(lineBufCfg))
		{
			bk_printf("VCEncInitInputLineBuffer failed\n");
			goto __END;
		}
    }

__END:
    return ret;
}

int32_t jpeg_encoder_deinit(void* handle)
{
	JpegEncRet ret;

	JPEGEncoderContext* context = (JPEGEncoderContext*)handle;

	JpegEncInst  encHandle = context->encHandle;
	VCEncBuf*    encBuf    = &context->encBuf;

	if(encHandle)
	{
		if(encBuf->encOutBuf) os_free((void*)encBuf->encOutBuf);
	
		/* Last Step: Release the encHandle instance */
		if((ret = JpegEncRelease(encHandle)) != JPEGENC_OK)
		{
			bk_printf("JpegEncRelease falied with error code %d\n", ret);
		}
	}
	else
	{
		ret = JPEGENC_INSTANCE_ERROR;
	}

	os_free(context);

	__vcenc_platform_deinit();

	return ret;
}

int32_t jpeg_encoder_encode(void* handle, uint32_t picBuf, uint32_t picLines, uint32_t outBuf, uint32_t outSize)
{
	JpegEncRet ret;

	JPEGEncoderContext* context = (JPEGEncoderContext*)handle;

	JpegEncInst  encHandle = context->encHandle;
	JpegEncIn*   encIn     = &context->encIn;
	JpegEncOut*  encOut    = &context->encOut;
	VCEncBuf*    encBuf    = &context->encBuf;

	if(!context || !encHandle) return JPEGENC_INSTANCE_ERROR;

	vcenc_outbuf_update(handle, 1, outBuf, outSize);

	/* Enable all frame headers */
	encIn->frameHeader = 1;

	if(encBuf->lineBufMode)
	{
		inputLineBufferCfg* lineBufCfg = &encBuf->lineBufCfg;

		//if(cfg.codingType == JPEGENC_WHOLE_FRAME) cfg.yOffset = 0;

		lineBufCfg->lumSrc  = (u8*)picBuf;
		lineBufCfg->cbSrc   = lineBufCfg->lumSrc + encBuf->encPicYSize;
		lineBufCfg->crSrc   = lineBufCfg->cbSrc  + encBuf->encPicUVSize / 2;
		lineBufCfg->wrCnt   = 0;
		lineBufCfg->sram    = (u8*)picBuf;
		lineBufCfg->sramBusAddr = picBuf;
		lineBufCfg->sramSize= lineBufCfg->depth * lineBufCfg->amountPerLoopBack * 16 * lineBufCfg->encWidth * 3 / 2;
		encIn->lineBufWrCnt = VCEncStartInputLineBufferWithoutCopy(lineBufCfg, picLines);
		encIn->initSegNum   = lineBufCfg->initSegNum;
		encIn->busLum       = lineBufCfg->lumBuf.busAddress;
		encIn->busCb        = lineBufCfg->cbBuf.busAddress;
		encIn->busCr        = lineBufCfg->crBuf.busAddress;
	}
	else
	{
        encIn->busLum = picBuf;
        encIn->busCb  = encIn->busLum + encBuf->encPicYSize;
        encIn->busCr  = encIn->busCb + encBuf->encPicUVSize / 2;
	}

	/* Encode the picture, Loop until the frame is ready */
	do
	{
		ret = JpegEncEncode(encHandle, encIn, encOut);
		switch (ret)
		{
		case JPEGENC_RESTART_INTERVAL:
		case JPEGENC_FRAME_READY:
			if(encBuf->lineBufMode) VCEncUpdateInitSegNum(&encBuf->lineBufCfg);
			if(encBuf->encOutCallback) encBuf->encOutCallback((uint8_t*)encIn->pOutBuf[0], encOut->jfifSize, encOut->headerSize);
			break;
		default: /* All the others are error codes */
			break;
		}

	} while (ret == JPEGENC_RESTART_INTERVAL);

	return ret;
}
