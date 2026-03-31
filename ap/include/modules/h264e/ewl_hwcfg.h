/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--      In the event of publication, the following notice is applicable:      --
--                                                                            --
--                   (C) COPYRIGHT 2020 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--         The entire notice above must be reproduced on all copies.          --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Encoder HW Configure features. 
--
------------------------------------------------------------------------------*/

/* Genenrated on 2024-05-31, not modify. */

#ifndef EWL_HWCFG_H
#define EWL_HWCFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "base_type.h"

/** \brief The hardware configuration information. */
typedef struct EWLHwConfig {
    //Sub-System
    /** \brief The ASIC_ID to identify the hardware version. */
    u32 hw_asic_id;
    /** \brief The BUILD_ID of the hardware. */
    u32 hw_build_id;
    //Format
    /** \brief Whether the hardware supports HEVC encoding. \n 0: Does not support. \n 1: Supports. */
    u32 hevcEnabled;
    /** \brief Whether the hardware supports H.264 encoding. \n 0: Does not support. \n 1: Supports. */
    u32 h264Enabled;
    /** \brief Whether the hardware supports JPEG encoding. \n 0: Does not support. \n 1: Supports. */
    u32 jpegEnabled;
    /** \brief Whether the hardware supports VP9 encoding. \n 0: Does not support. \n 1: Supports. */
    u32 vp9Enabled;
    /** \brief Whether the hardware supports AV1 encoding. \n 0: Does not support. \n 1: Supports. */
    u32 av1Enabled;
    //IM
    /** \brief Whether the hardware supports CuTree look-ahead. \n 0: Does not support. \n 1: Supports. */
    u32 cuTreeSupport;
    //Resolution
    /** \brief The maximum video width supported by the hardware for HEVC encoding, in pixels. */
    u32 maxEncodedWidthHEVC;
    /** \brief The maximum video width supported by the hardware for H.264 encoding, in pixels. */
    u32 maxEncodedWidthH264;
    /** \brief The maximum video width supported by the hardware for AV1 encoding, in pixels. */
    u32 maxEncodedWidthAV1;
    /** \brief The maximum video width supported by the hardware for VP9 encoding, in pixels. */
    u32 maxEncodedWidthVP9;
    /** \brief The maximum video height. \n 0: 8192 pixels.  \n 1: 8640 pixels. */
    u32 videoHeightExt;
    /** \brief The maximum video width supported by the hardware for JPEG encoding, in pixels. */
    u32 maxEncodedWidthJPEG;
    //Major 
    /** \brief Whether the hardware supports B-frame. \n 0: Does not support. \n 1: Supports. */
    u32 bFrameEnabled;
    /** \brief Whether the hardware supports main 10 or main 8. \n 0: Supports main 8. \n 1: Supports main 10. */
    u32 main10Enabled;
    /** \brief Whether the hardware supports H.264 rate distorted optimized quantization (RDOQ). \n 0: Does not support. \n 1: Supports. */
    u32 RDOQSupportH264;
    /** \brief Whether the hardware supports HEVC RDOQ. \n 0: Does not support. \n 1: Supports. */
    u32 RDOQSupportHEVC;
    /** \brief Whether the hardware supports AV1 RDOQ. \n 0: Does not support. \n 1: Supports. */
    u32 RDOQSupportAV1;
    /** \brief Whether the hardware supports VP9 RDOQ. \n 0: Does not support. \n 1: Supports. */
    u32 RDOQSupportVP9;
    /** \brief The horizontal search range of motion estimation (ME).
     * \n 0: 128 pixels for P-frame, 64 pixels for B-frame.
     * \n 1: 128 pixels for P-frame, 112 pixels for B-frame.
     * \n 2: 192 pixels for P-frame, 112 pixels for B-frame.
     * \n 3: 240 pixels for P-frame, 240 pixels for B-frame.
     * \n 4: 112 pixels for P-frame, 56 pixels for B-frame. */
    u32 meHorSearchRange;
    /** \brief The ME vertical search range for H.264. */
    u32 meVertSearchRangeH264;
    /** \brief The ME vertical Search range for HEVC, AV1, and VP9. */
    u32 meVertSearchRangeHEVC;
    /** \brief Whether the hardware supports SSIM calculation for HEVC and H.264. \n 0: Does not support. \n 1: Supports. */
    u32 ssimSupport;
    /** \brief Whether the hardware supports PSNR calculation for HEVC and H.264. \n 0: Does not support. \n 1: Supports. */
    u32 psnrSupport;
    /** \brief Whether the hardware supports SSIM calculation for AV1. \n 0: Does not support. \n 1: Supports. */
    u32 ssimSupportAV1;
    /** \brief Whether the hardware supports PSNR calculation for AV1. \n 0: Does not support. \n 1: Supports. */
    u32 psnrSupportAV1;
    /** \brief Whether the hardware supports HEVC tile. \n 0: Does not support. \n 1: Supports. */
    u32 hevcTileSupport;
    /** \brief Whether the hardware supports HEVC SAO. \n 0: Does not support. \n 1: Supports. */
    u32 saoSupport;
    //Minor
    /** \brief Whether the hardware supports reference frame compressor (RFC). \n 0: Does not support. \n 1: Supports. */
    u32 rfcEnable;
    /** \brief the pixel block size controlled by each Qp
0: 8x8   1: 16x16 */
    u32 roiBlkSize;
    /** \brief The hardware version for ROI map. \n For more information, see <i>Hantro VC9000E Series Memory Buffer and Format Organization</i>. */
    u32 roiMapVersion;
    /** \brief The output format of CU statistics information. \n For more information, see <i>Hantro VC9000E Series Memory Buffer and Format Organization</i>. */
    u32 cuInforVersion;
    /** \brief Whether the hardware supports CTB-level or MB-level rate control. \n 0: Does not support. \n 1: Support. */
    u32 ctbRcSupport;
    /** \brief The CTB rate control version supported by hardware.
     * \n 0: The version only supports to adjust QP for subjective quality.
     * \n 1: The version supports to adjust QP for subjective quality and target bits.
     * \n 2: The version supports to adjust QP for subjective quality and target bits with a larger range of adjustment. */
    u32 ctbRcVersion;
    /** \brief Whether the hardware supports frame-level information output. \n 0: Does not support. \n 1: Supports. */
    u32 frameInfoSupport;
    /** \brief Whether the hardware supports programmable RDO level.  \n 0: Does not support. \n 1: Supports. */
    u32 progRdoEnable;
    /** \brief Whether the hardware supports JPEG encoding with lossless process. \n 0: Does not support. \n 1: Supports. */
    u32 ljpegSupport;
    /** \brief Whether the hardware supports ROI map for JPEG encoding. \n 0: Does not support. \n 1: Supports. */
    u32 JpegRoiMapSupport;
    /** \brief Whether the hardware supports monochrome JPEG encoding. \n 0: Does not support. \n 1: Supports. */
    u32 jpeg400Support;
    /** \brief Whether the hardware supports YUV422 JPEG encoding. \n 0: Does not support. \n 1: Supports. */
    u32 jpeg422Support;
    /** \brief Whether the hardware supports Huffman table (DHT) generation. \n 0: Does not support. \n 1: Supports. */
    u32 JpegDHTGenerate;
    /** \brief Whether the hardware supports user-defined Huffman table for JPEG encoding. \n 0: Does not support. \n 1: Supports. */
    u32 JpegExternalDHT;
    /** \brief Whether the hardware supports programmable vertical ME range. \n 0: Does not support. \n 1: Supports */
    u32 meVertRangeProgramable;
    /** \brief Whether the hardware supports monochrome H.264 and HEVC encoding. \n 0: Does not support. \n 1: Supports. */
    u32 MonoChromeSupport;
    /** \brief Whether the hardware supports HEVC YUV422. \n 0: Does not support. \n 1: Supports. */
    u32 HevcYUV422Support;
    /** \brief Whether the hardware supports H.264 YUV422. \n 0: Does not support. \n 1: Supports. */
    u32 H264YUV422Support;
    /** \brief Whether the hardware supports HEVC YUV444. \n 0: Does not support. \n 1: Supports. */
    u32 HevcYUV444Support;
    /** \brief Whether the hardware supports H.264 YUV444. \n 0: Does not support. \n 1: Supports. */
    u32 H264YUV444Support;
    /** \brief Whether the hardware supports 32x32 intra TU. \n 0: Does not support. \n 1: Supports. */
    u32 intraTU32Enable;
    /** \brief Whether the hardware supports look-ahead. \n 0: Does not support. \n 1: Supports. */
    u32 bMultiPassSupport;
    /** \brief Whether the hardware supports 32x32 inter TU. \n 0: Does not support. \n 1: Supports. */
    u32 tu32Enable;
    /** \brief Whether the hardware supports dynamic maximum TU size. \n 0: Does not support \n 1: Supports. */
    u32 dynamicMaxTuSize;
    /** \brief Whether the hardware supports number of bits output for each CTB. \n 0: Does not support. \n 1: Supports. */
    u32 CtbBitsOutSupport;
    /** \brief Whether the hardware supports 32x32 inter TU. \n 0: Does not support. \n 1: Supports. */
    u32 encVisualTuneSupport;
    /** \brief Whether the hardware supports revert quality adjustment when the subjective CTB rate control is enabled. \n 0: Does not support. \n 1: Supports. */
    u32 ctbRcMoreMode;
    /** \brief Whether the hardware supports denoise. \n 0: Does not support. \n 1: Supports. */
    u32 deNoiseEnabled;
    /** \brief Whether use input down sampled pixel as Ref of ME4N. \n 0: Does not support. \n 1: Supports. */
    u32 encRef4NInputSupport;
    /** \brief Whether the hardware supports tuning quality according to psy-rd. \n 0: Does not support. \n 1: Supports. */
    u32 encPsyTuneSupport;
    /** \brief Whether the hardware supports motion score calculation and the storage of the result into a software register. \n 0: Does not support. \n 1: Supports. */
    u32 motionScoreEnable;
    /** \brief Whether the hardware supports SSE calculation for recon pixel. */
    u32 reconSSEsupport;
    /** \brief The maximum active reference frame number for P-frame encoding. \n 0: One reference frame. \n 1: Two reference frames. */
    u32 maxRefNumList0Minus1;
    /** \brief The maximum slice height of CTBs or MBs supported by the hardware. \n If the value is n, then the maximum height is 2 to the Power(2,n)-1. */
    u32 encSliceSizeBits;
    /** \brief Whether the hardware supports asymmetric PU partition for HEVC. \n 0: Does not support. \n 1: Supports. */
    u32 asymPuSupport;
    /** \brief Whether the hardware supports the fourth skip candidate for HEVC and H.264. \n 0: Does not support. \n 1: Supports. */
    u32 maxMergeCand4Support;
    /** \brief Whether the hardware supports outputting adaptive quantization information. \n 0: Does not support. \n 1: Supports. */
    u32 encAQInfoOut;
    //PRP
    /** \brief Whether the hardware supports RGB-to-YUV conversion. \n 0: Does not support. \n 1: Supports. */
    u32 rgbEnabled;
    /** \brief Whether the hardware supports rotation in pre-processing. \n 0: Supports. \n 1: Does not support. */
    u32 NonRotationSupport;
    /** \brief Whether the hardware only supports NV12 as input. \n 0: Supports other format. \n 1: Only supports NV12. */
    u32 NVFormatOnlySupport;
    /** \brief Whether the hardware supports low-latency mechnism on input picture. \n 0: Supports. \n 1: Does not support. */
    u32 prpLowLatencySupport;
    /** \brief Pre-Processing use sync word in DDR to do low latency handshake. <tt>prpLowLatencySupport</tt> should be <tt>1</tt> if this feature is selected. \n 0: Supports. \n 1: Does not support. */
    u32 prpLowlatencySignalbyDDR;
    /** \brief Whether the hardware supports in-loop down-scaler. \n 0: Does not support. \n 1: Supports. */
    u32 inLoopDSRatio;
    /** \brief Whether the hardware supports full or partial range conversion when implementing RGB-to-YUV color conversion. \n 0: Does not support. \n 1: Supports. */
    u32 cscExtendSupport;
    /** \brief Whether the hardware supports down-scaling. \n 0: Does not support. \n 1: Supports. */
    u32 scalingEnabled;
    /** \brief Whether the hardware supports out-loop scaler output in YUV420SP format. \n 0: Does not support. \n 1: Supports. */
    u32 scaled420Support;
    /** \brief Whether the hardware supports video stabilization. \n 0: Does not support. \n 1: Supports. */
    u32 vsSupport;
    /** \brief Whether the hardware supports OSD. \n 0: Does not support. \n 1: Supports. */
    u32 OSDSupport;
    /** \brief The maximum number of OSD rectangles supported by the hardware. */
    u32 maxOsdNum;
    /** \brief Whether the hardware supports mosaic. \n 0: Does not support. \n 1: Supports. */
    u32 MosaicSupport;
    /** \brief The number of mosaic rectangles supported by the hardware. */
    u32 maxMosaicNum;
    /** \brief The version of of mosaic. \n 0: 64x64 for HEVC, 16x16 for H264. \n 1: 8x8/16x16/32x32/64x64/128x128/ for H264 and HEVC. */
    u32 mosaicVersion;
    /** \brief Hardware OSD alpha blending Version in the pre-processing pipeline. \n 0: Does not support. \n 1: Supports. */
    u32 osdBlendVersion;
    /** \brief Whether the hardware supports picture mosaic control by map buffer in DDR or not. The mosaic information of every 8x8 blocks are specified in the map buffer. \n 0: Does not support. \n 1: Supports. */
    u32 OSDMapSupport;
    /** \brief The OSD Map version. \n 0: 4bits for each 8x8. \n 1: 4 bits for each 4x4. */
    u32 OSDMapVersion;
    /** \brief Whether the hardware supports 8x8 tiled input formats.
     * \n For more information about input formats, refer to <i>Hantro VC9000E v1x Hardware Features</i>. */
    u32 tile8x8FormatSupport;
    /** \brief Whether the hardware supports 4x4 tiled input formats.
     * \n For more information about input formats, refer to <i>Hantro VC9000E v1x Hardware Features</i>. */
    u32 tile4x4FormatSupport;
    /** \brief Whether the hardware supports tiled input formats.
     * \n For more information about input formats, refer to <i>Hantro VC9000E v1x Hardware Features</i>. */
    u32 customTileFormatSupport;
    /** \brief Whether the hardware supports RGB888 or BGR888 input formats. \n 0: Does not support. \n 1: Supports. */
    u32 rgb24bitFormatSupport;
    /** \brief Whether the hardware supports FLEXA SBI in the pre-processing pipeline. \n 0: Does not support. \n 1: Supports. */
    u32 prpSbiSupport;
    /** \brief 100: AFBC
200: PVRIC
400: DEC400 */
    u32 ufbcSupport;
    /** \brief Whether the pre-processing module supports the X-major super tile input format that is output by VeriSilicon GPU IPs. \n 0: Does not support. \n 1: Supports */
    u32 superTileXSupport;
    /** \brief Whether the pre-processing module supports input formats YUV420SP8b_Tile64x4_A16N and YVU420SP8b_Tile64x4_A16N. \n 0: Does not support. \n 1: Supports. */
    u32 tile64x4_8bFormatSupport;
    /** \brief Whether the pre-processing module supports input format YUV420SP10bWH_Tile32x4_A16N. */
    u32 tile32x4_10bFormatSupport;
    /** \brief Whether the hardware supports input analyzer in the pre-processing pipeline. \n 0: Does not support. \n 1: Supports. */
    u32 prpAnalyzerSupport;
    /** \brief Whether the hardware supports mirror in pre-processing pipeline. \n 0: Does not support. \n 1: Supports. */
    u32 prpMirrorSupport;
    //HW
    /** \brief Whether the hardware supports external SRAM. \n 0: Does not support. \n 1: Supports. */
    u32 meExternSramSupport;
    /** \brief The bus type of the hardware. \n 1: AHB. \n 2: OCP. \n 3: AXI. \n 4: PCI. \n 5: AXI and AHB. \n 6: AXI and APB. */
    u32 busType;
    /** \brief The bus width. \n The value is fixed to 2, which indicates 128 bits. */
    u32 busWidth;
    /** \brief Reserved. */
    u32 maxAXIAlignment;
    /** \brief AXI alignment setting for writing common data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_wr_common;
    /** \brief AXI alignment setting for writing bit streams. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_wr_stream;
    /** \brief AXI alignment setting for writing chroma reconstructed frame data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_wr_chroma_ref;
    /** \brief AXI alignment setting for writing luma reconstructed frame data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_wr_luma_ref;
    /** \brief AXI alignment setting for reading common data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_rd_common;
    /** \brief AXI alignment setting for reading pre-processing data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_rd_prp;
    /** \brief AXI alignment setting for reading chroma reference frame data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_rd_ch_ref_prefetch;
    /** \brief AXI alignment setting for reading luma reference frame data. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_rd_lu_ref_prefetch;
    /** \brief AXI alignment setting for writing CU information. \n 6: AXI aligned to 64. \n 8: AXI aligned to 256. */
    u32 axi_burst_align_wr_cuinfo;
    /** \brief Whether to have SW registers to control the clock gating behavior when low latency hanshake stall the encoding pipeline */
    u32 lowLatClkGate;
    //supported in most HW (keep them always supported in future HW)
    /** \brief The number of bits used to write the nal_ref_idc syntax element of H.264. \n 0: 1 bit \n 1: 2 bits. */
    u32 h264NalRefIdc2bit;
    /** \brief Whether the hardware supports the av1_extension_flag register which controls the syntax in OBU_Header. \n 0: Not supported. \n 1: Supported. */
    u32 av1ExtensionFlag;
    /** \brief Whether the hardware supports ROIs with absolute QP. \n 0: Does not support.  \n 1: Supports. */
    u32 roiAbsQpSupport;
    /** \brief Whether the hardware supports two or eight ROIs. \n 0: Supports two ROIs.  \n 1: Supports eight ROIs. */
    u32 ROI8Support;
    /** \brief Whether the hardware supports context-adaptive variable-length coding (CAVLC). \n 0: Does not support. \n 1: Supports. */
    u32 h264CavlcEnable;
    /** \brief Whether the hardware supports linear or ring structure for reference and reconstructed frame buffers for P-frame encoding.
     *  \n 0: Only supports linear structure. \n 1: Supports both linear and ring structure. */
    u32 refRingBufEnable;
    /** \brief Whether the hardware supports disabling reconstructed frame writing. \n 0: Does not support. \n 1: Supports. */
    u32 disableRecWtSupport;
    /** \brief Whether the hardware uses CU size map from IntraModeSearch block (not decided by PK inter vs inter rdcost) for the Cus which are forced to Intra by GDR/CIR/ROI. \n 0: Does not support. \n 1: Supports. */
    u32 forceIntraCuSizeOpt;
    //Not supported in most HW
    /** \brief Whether the hardware supports background detection. \n 0: Does not support. \n 1: Supports. (Only for a few customers) */
    u32 backgroundDetSupport;
    /** \brief Whether the hardware supports using P010 as the reference frame format for 10-bit encoding.  \n 0: Does not support. \n 1: Supports. (Only for a few customers)  */
    u32 P010RefSupport;
    /** \brief Whether the hardware supports stream buffer chain. \n 0: Does not support. \n 1: Supports. */
    u32 streamBufferChain;
    /** \brief Whether the hardware supports global motion vector. \n 0: Does not support. \n 1: Supports. (Only for a few customers) */
    u32 gmvSupport;
    /** \brief Whether the hardware supports eight IPCM area setting. \n 0: Does not support. \n 1: Supports. */
    u32 IPCM8Support;
    //Not implemented in HW
    /** \brief Whether the hardware supports AV1 switchable interpolation filter. \n 0: Does not support. \n 1: Supports. (Only for C-model) */
    u32 av1InterpFilterSwitchable;
    /** \brief Whether the hardware supports I-frame encoding only. \n 0: Does not support. \n 1: Supports. */
    u32 IframeOnly;
    /** \brief Whether the hardware supports dynamical choosing of RDO level. \n 0: Does not support. \n 1: Supports. */
    u32 dynamicRdoSupport;
    /** \brief Whether the hardware supports quality improvement defined in set two. \n 0: Does not support. \n 1: Supports. */
    u32 tuneToolsSet2Support;
    /** \brief Whether the hardware supports TMVP for HEVC. \n 0: Does not support. \n 1: Supports. */
    u32 hevcTemporalMvpSupport;
    /** \brief Whether the hardware supports TMVP for AV1. \n 0: Does not support. \n 1: Supports. */
    u32 av1TemporalMvpSupport;
    /** \brief Whether the hardware supports TMVP with multiple cores together. \n 0: Does not support. \n 1: Supports. */
    u32 tmvpMcSupport;
    /** \brief Whether the hardware supports intra mode search with reconstructed frames.  \n 0: Does not support. \n 1: Supports. */
    u32 intraReconSupport;
    /** \brief Whether the hardware supports AV1 intra mode search with reconstructed frames.  \n 0: Does not support. \n 1: Supports. */
    u32 av1IntraReconSupport;
    /** \brief Whether the hardware supports the CuTree RDO architecture for HEVC.  \n 0: Does not support. \n 1: Supports. */
    u32 hevcCuRdoSupport;
    /** \brief Whether the hardware supports the CuTree RDO architecture for AV1.  \n 0: Does not support. \n 1: Supports. */
    u32 av1CuRdoSupport;
    /** \brief Whether the hardware supports simplified cost instead of Meqn binCost.   \n 0: Does not support. \n 1: Supports. */
    u32 meqnSimpleBinCost;
    /** \brief Whether the hardware supports multiple segments for stream buffer. \n 0: Does not support. \n 1: Supports. */
    u32 streamMultiSegment;
    /** \brief Whether the hardware supports the modification of subjective quality improvement for H.264. \n 0: Does not support. \n 1: Supports. */
    u32 modSubjPrefer;
    /** \brief Whether the hardware supports both the 2RefP and TMVP. \n 0: Does not support. \n 1: Supports. */
    u32 twoRefPTMVPSupport;
}EWLHwConfig_t;

#define EWLShowHwConfig(info) \
  do { \
    PTRACE("Get Hantro Encoder Hardware Configure:\n"); \
    PTRACE("                               hw_asic_id = 0x%x\n", info ->hw_asic_id); \
    PTRACE("                              hw_build_id = 0x%x\n", info ->hw_build_id); \
    PTRACE("                              hevcEnabled = %d\n", info ->hevcEnabled); \
    PTRACE("                              h264Enabled = %d\n", info ->h264Enabled); \
    PTRACE("                              jpegEnabled = %d\n", info ->jpegEnabled); \
    PTRACE("                               vp9Enabled = %d\n", info ->vp9Enabled); \
    PTRACE("                               av1Enabled = %d\n", info ->av1Enabled); \
    PTRACE("                            cuTreeSupport = %d\n", info ->cuTreeSupport); \
    PTRACE("                      maxEncodedWidthHEVC = %d\n", info ->maxEncodedWidthHEVC); \
    PTRACE("                      maxEncodedWidthH264 = %d\n", info ->maxEncodedWidthH264); \
    PTRACE("                       maxEncodedWidthAV1 = %d\n", info ->maxEncodedWidthAV1); \
    PTRACE("                       maxEncodedWidthVP9 = %d\n", info ->maxEncodedWidthVP9); \
    PTRACE("                           videoHeightExt = %d\n", info ->videoHeightExt); \
    PTRACE("                      maxEncodedWidthJPEG = %d\n", info ->maxEncodedWidthJPEG); \
    PTRACE("                            bFrameEnabled = %d\n", info ->bFrameEnabled); \
    PTRACE("                            main10Enabled = %d\n", info ->main10Enabled); \
    PTRACE("                          RDOQSupportH264 = %d\n", info ->RDOQSupportH264); \
    PTRACE("                          RDOQSupportHEVC = %d\n", info ->RDOQSupportHEVC); \
    PTRACE("                           RDOQSupportAV1 = %d\n", info ->RDOQSupportAV1); \
    PTRACE("                           RDOQSupportVP9 = %d\n", info ->RDOQSupportVP9); \
    PTRACE("                         meHorSearchRange = %d\n", info ->meHorSearchRange); \
    PTRACE("                    meVertSearchRangeH264 = %d\n", info ->meVertSearchRangeH264); \
    PTRACE("                    meVertSearchRangeHEVC = %d\n", info ->meVertSearchRangeHEVC); \
    PTRACE("                              ssimSupport = %d\n", info ->ssimSupport); \
    PTRACE("                              psnrSupport = %d\n", info ->psnrSupport); \
    PTRACE("                           ssimSupportAV1 = %d\n", info ->ssimSupportAV1); \
    PTRACE("                           psnrSupportAV1 = %d\n", info ->psnrSupportAV1); \
    PTRACE("                          hevcTileSupport = %d\n", info ->hevcTileSupport); \
    PTRACE("                               saoSupport = %d\n", info ->saoSupport); \
    PTRACE("                                rfcEnable = %d\n", info ->rfcEnable); \
    PTRACE("                               roiBlkSize = %d\n", info ->roiBlkSize); \
    PTRACE("                            roiMapVersion = %d\n", info ->roiMapVersion); \
    PTRACE("                           cuInforVersion = %d\n", info ->cuInforVersion); \
    PTRACE("                             ctbRcSupport = %d\n", info ->ctbRcSupport); \
    PTRACE("                             ctbRcVersion = %d\n", info ->ctbRcVersion); \
    PTRACE("                         frameInfoSupport = %d\n", info ->frameInfoSupport); \
    PTRACE("                            progRdoEnable = %d\n", info ->progRdoEnable); \
    PTRACE("                             ljpegSupport = %d\n", info ->ljpegSupport); \
    PTRACE("                        JpegRoiMapSupport = %d\n", info ->JpegRoiMapSupport); \
    PTRACE("                           jpeg400Support = %d\n", info ->jpeg400Support); \
    PTRACE("                           jpeg422Support = %d\n", info ->jpeg422Support); \
    PTRACE("                          JpegDHTGenerate = %d\n", info ->JpegDHTGenerate); \
    PTRACE("                          JpegExternalDHT = %d\n", info ->JpegExternalDHT); \
    PTRACE("                   meVertRangeProgramable = %d\n", info ->meVertRangeProgramable); \
    PTRACE("                        MonoChromeSupport = %d\n", info ->MonoChromeSupport); \
    PTRACE("                        HevcYUV422Support = %d\n", info ->HevcYUV422Support); \
    PTRACE("                        H264YUV422Support = %d\n", info ->H264YUV422Support); \
    PTRACE("                        HevcYUV444Support = %d\n", info ->HevcYUV444Support); \
    PTRACE("                        H264YUV444Support = %d\n", info ->H264YUV444Support); \
    PTRACE("                          intraTU32Enable = %d\n", info ->intraTU32Enable); \
    PTRACE("                        bMultiPassSupport = %d\n", info ->bMultiPassSupport); \
    PTRACE("                               tu32Enable = %d\n", info ->tu32Enable); \
    PTRACE("                         dynamicMaxTuSize = %d\n", info ->dynamicMaxTuSize); \
    PTRACE("                        CtbBitsOutSupport = %d\n", info ->CtbBitsOutSupport); \
    PTRACE("                     encVisualTuneSupport = %d\n", info ->encVisualTuneSupport); \
    PTRACE("                            ctbRcMoreMode = %d\n", info ->ctbRcMoreMode); \
    PTRACE("                           deNoiseEnabled = %d\n", info ->deNoiseEnabled); \
    PTRACE("                     encRef4NInputSupport = %d\n", info ->encRef4NInputSupport); \
    PTRACE("                        encPsyTuneSupport = %d\n", info ->encPsyTuneSupport); \
    PTRACE("                        motionScoreEnable = %d\n", info ->motionScoreEnable); \
    PTRACE("                          reconSSEsupport = %d\n", info ->reconSSEsupport); \
    PTRACE("                     maxRefNumList0Minus1 = %d\n", info ->maxRefNumList0Minus1); \
    PTRACE("                         encSliceSizeBits = %d\n", info ->encSliceSizeBits); \
    PTRACE("                            asymPuSupport = %d\n", info ->asymPuSupport); \
    PTRACE("                     maxMergeCand4Support = %d\n", info ->maxMergeCand4Support); \
    PTRACE("                             encAQInfoOut = %d\n", info ->encAQInfoOut); \
    PTRACE("                               rgbEnabled = %d\n", info ->rgbEnabled); \
    PTRACE("                       NonRotationSupport = %d\n", info ->NonRotationSupport); \
    PTRACE("                      NVFormatOnlySupport = %d\n", info ->NVFormatOnlySupport); \
    PTRACE("                     prpLowLatencySupport = %d\n", info ->prpLowLatencySupport); \
    PTRACE("                 prpLowlatencySignalbyDDR = %d\n", info ->prpLowlatencySignalbyDDR); \
    PTRACE("                            inLoopDSRatio = %d\n", info ->inLoopDSRatio); \
    PTRACE("                         cscExtendSupport = %d\n", info ->cscExtendSupport); \
    PTRACE("                           scalingEnabled = %d\n", info ->scalingEnabled); \
    PTRACE("                         scaled420Support = %d\n", info ->scaled420Support); \
    PTRACE("                                vsSupport = %d\n", info ->vsSupport); \
    PTRACE("                               OSDSupport = %d\n", info ->OSDSupport); \
    PTRACE("                                maxOsdNum = %d\n", info ->maxOsdNum); \
    PTRACE("                            MosaicSupport = %d\n", info ->MosaicSupport); \
    PTRACE("                             maxMosaicNum = %d\n", info ->maxMosaicNum); \
    PTRACE("                            mosaicVersion = %d\n", info ->mosaicVersion); \
    PTRACE("                          osdBlendVersion = %d\n", info ->osdBlendVersion); \
    PTRACE("                            OSDMapSupport = %d\n", info ->OSDMapSupport); \
    PTRACE("                            OSDMapVersion = %d\n", info ->OSDMapVersion); \
    PTRACE("                     tile8x8FormatSupport = %d\n", info ->tile8x8FormatSupport); \
    PTRACE("                     tile4x4FormatSupport = %d\n", info ->tile4x4FormatSupport); \
    PTRACE("                  customTileFormatSupport = %d\n", info ->customTileFormatSupport); \
    PTRACE("                    rgb24bitFormatSupport = %d\n", info ->rgb24bitFormatSupport); \
    PTRACE("                            prpSbiSupport = %d\n", info ->prpSbiSupport); \
    PTRACE("                              ufbcSupport = %d\n", info ->ufbcSupport); \
    PTRACE("                        superTileXSupport = %d\n", info ->superTileXSupport); \
    PTRACE("                 tile64x4_8bFormatSupport = %d\n", info ->tile64x4_8bFormatSupport); \
    PTRACE("                tile32x4_10bFormatSupport = %d\n", info ->tile32x4_10bFormatSupport); \
    PTRACE("                       prpAnalyzerSupport = %d\n", info ->prpAnalyzerSupport); \
    PTRACE("                         prpMirrorSupport = %d\n", info ->prpMirrorSupport); \
    PTRACE("                      meExternSramSupport = %d\n", info ->meExternSramSupport); \
    PTRACE("                                  busType = %d\n", info ->busType); \
    PTRACE("                                 busWidth = %d\n", info ->busWidth); \
    PTRACE("                          maxAXIAlignment = %d\n", info ->maxAXIAlignment); \
    PTRACE("                axi_burst_align_wr_common = %d\n", info ->axi_burst_align_wr_common); \
    PTRACE("                axi_burst_align_wr_stream = %d\n", info ->axi_burst_align_wr_stream); \
    PTRACE("            axi_burst_align_wr_chroma_ref = %d\n", info ->axi_burst_align_wr_chroma_ref); \
    PTRACE("              axi_burst_align_wr_luma_ref = %d\n", info ->axi_burst_align_wr_luma_ref); \
    PTRACE("                axi_burst_align_rd_common = %d\n", info ->axi_burst_align_rd_common); \
    PTRACE("                   axi_burst_align_rd_prp = %d\n", info ->axi_burst_align_rd_prp); \
    PTRACE("       axi_burst_align_rd_ch_ref_prefetch = %d\n", info ->axi_burst_align_rd_ch_ref_prefetch); \
    PTRACE("       axi_burst_align_rd_lu_ref_prefetch = %d\n", info ->axi_burst_align_rd_lu_ref_prefetch); \
    PTRACE("                axi_burst_align_wr_cuinfo = %d\n", info ->axi_burst_align_wr_cuinfo); \
    PTRACE("                            lowLatClkGate = %d\n", info ->lowLatClkGate); \
    PTRACE("                        h264NalRefIdc2bit = %d\n", info ->h264NalRefIdc2bit); \
    PTRACE("                         av1ExtensionFlag = %d\n", info ->av1ExtensionFlag); \
    PTRACE("                          roiAbsQpSupport = %d\n", info ->roiAbsQpSupport); \
    PTRACE("                              ROI8Support = %d\n", info ->ROI8Support); \
    PTRACE("                          h264CavlcEnable = %d\n", info ->h264CavlcEnable); \
    PTRACE("                         refRingBufEnable = %d\n", info ->refRingBufEnable); \
    PTRACE("                      disableRecWtSupport = %d\n", info ->disableRecWtSupport); \
    PTRACE("                      forceIntraCuSizeOpt = %d\n", info ->forceIntraCuSizeOpt); \
    PTRACE("                     backgroundDetSupport = %d\n", info ->backgroundDetSupport); \
    PTRACE("                           P010RefSupport = %d\n", info ->P010RefSupport); \
    PTRACE("                        streamBufferChain = %d\n", info ->streamBufferChain); \
    PTRACE("                               gmvSupport = %d\n", info ->gmvSupport); \
    PTRACE("                             IPCM8Support = %d\n", info ->IPCM8Support); \
    PTRACE("                av1InterpFilterSwitchable = %d\n", info ->av1InterpFilterSwitchable); \
    PTRACE("                               IframeOnly = %d\n", info ->IframeOnly); \
    PTRACE("                        dynamicRdoSupport = %d\n", info ->dynamicRdoSupport); \
    PTRACE("                     tuneToolsSet2Support = %d\n", info ->tuneToolsSet2Support); \
    PTRACE("                   hevcTemporalMvpSupport = %d\n", info ->hevcTemporalMvpSupport); \
    PTRACE("                    av1TemporalMvpSupport = %d\n", info ->av1TemporalMvpSupport); \
    PTRACE("                            tmvpMcSupport = %d\n", info ->tmvpMcSupport); \
    PTRACE("                        intraReconSupport = %d\n", info ->intraReconSupport); \
    PTRACE("                     av1IntraReconSupport = %d\n", info ->av1IntraReconSupport); \
    PTRACE("                         hevcCuRdoSupport = %d\n", info ->hevcCuRdoSupport); \
    PTRACE("                          av1CuRdoSupport = %d\n", info ->av1CuRdoSupport); \
    PTRACE("                        meqnSimpleBinCost = %d\n", info ->meqnSimpleBinCost); \
    PTRACE("                       streamMultiSegment = %d\n", info ->streamMultiSegment); \
    PTRACE("                            modSubjPrefer = %d\n", info ->modSubjPrefer); \
    PTRACE("                       twoRefPTMVPSupport = %d\n", info ->twoRefPTMVPSupport); \
  } while(0)

#ifdef __cplusplus
}
#endif

#endif  /* EWL_HWCFG_H */
