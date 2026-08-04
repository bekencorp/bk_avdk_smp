/****************************************************************************
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2024 Vivante Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

#ifndef __VSI_COMM_ISP_H__
#define __VSI_COMM_ISP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @defgroup mpi_isp ISP Definitions
 * @{
 *
 *
 */

#define ISP_AUTO_STRENGTH_NUM 16     /**< \brief The number of ISO levels. */

typedef vsi_u32_t ISP_DEV;           /**< \brief ISP device ID. */

/** \brief Specifies an ISP device port. */
typedef struct {
    vsi_u32_t devId;  /**< \brief The ID of the ISP device to which the port belongs. */
    vsi_u32_t portId; /**< \brief The ID of the port. */
} ISP_PORT;

/** \brief Specifies an ISP channel. */
typedef struct {
    vsi_u32_t devId;  /**< \brief The ID of the ISP device to which the channel belongs. */
    vsi_u32_t portId; /**< \brief The ID of the port to which the channel belongs. */
    vsi_u32_t chnId;  /**< \brief The ID of the channel. */
} ISP_CHN;

/** \brief Defines the ISP working modes. */
typedef enum vsiISP_WORK_MODE_E {
    WORK_MODE_NORMAL = 0,  /**< \brief Normal mode. */
    WORK_MODE_MCM    = 1,  /**< \brief MCM mode. */
    WORK_MODE_BUTT         /**< \brief (Reserved) The number of ISP working modes. */
} ISP_WORK_MODE_E;

/** \brief Contains the attribute of an ISP device. */
typedef struct vsiISP_DEV_ATTR_S {
    vsi_u32_t ispWorkMode; /**< \brief The working mode of the ISP device.
                                \n Valid values: See <tt> \ref ISP_WORK_MODE_E</tt>. */
} ISP_DEV_ATTR_S;

/** \brief Defines the input data source types of ISP. */
typedef enum vsiISP_INPUT_TYPE_E {
    INPUT_TYPE_SENSOR = 0,  /**< \brief Data input from a sensor port. */
    INPUT_TYPE_ISP_TPG= 1,  /**< \brief Data input from ISP TPG. */
    INPUT_TYPE_DMA    = 2,  /**< \brief Data input from DMA. */
    INPUT_TYPE_VI_TPG = 3,  /**< \brief Data input from VI200 TPG. */
    INPUT_TYPE_BUTT         /**< \brief (Reserved) The number of ISP input data source types. */
} ISP_INPUT_TYPE_E;

/** \brief Defines the input data types of ISP. */
typedef enum vsiISP_MODE_E {
    ISP_MODE_BT656  = 1,  /**< \brief ITU-R BT.656 (YUV with embedded synchronization). */
    ISP_MODE_BT601  = 2,  /**< \brief ITU-R BT.601 (YUV input with HSYNC and VSYNC signals). */
    ISP_MODE_RAW    = 3,  /**< \brief RAW image with BT.601 synchronization (ISP bypass). */
    ISP_MODE_BUTT,        /**< \brief (Reserved) The number of ISP input data types. */
} ISP_MODE_E;

/** \brief Defines the HDR modes. */
typedef enum vsiISP_HDR_MODE_E {
    HDR_MODE_LINEAR        = 0,  /**< \brief Linear mode */
    HDR_MODE_ISP_STICH     = 1,  /**< \brief ISP stitching mode. */
    HDR_MODE_SENSOR_STICH  = 2,  /**< \brief Sensor stitching mode. */
    HDR_MODE_BUTT,               /**< \brief (Reserved) The number of HDR modes. */
} ISP_HDR_MODE_E;

/** \brief Defines the stitching modes. */
typedef enum vsiISP_STICH_MODE_E {
    STICH_MODE_3DOL_DUAL_DCG          = 0, /**< \brief 3-DOL dual conversion gain (DCG) mode. */
    STICH_MODE_3DOL                   = 1, /**< \brief 3-DOL mode. */
    STICH_MODE_3DOL_LINEBYLINE        = 2, /**< \brief 3-DOL line-by-line mode. */
    STICH_MODE_3DOL_16BIT_COMPRESS    = 3, /**< \brief 3-DOL 16-bit compression mode. */
    STICH_MODE_2DOL_DUAL_DCG          = 4, /**< \brief 2-DOL DCG mode. */
    STICH_MODE_2DOL                   = 5, /**< \brief 2-DOL mode. */
    STICH_MODE_2DOL_L_AND_S           = 6, /**< \brief 2-DOL low- and small-frequency mode. */
    STICH_MODE_BUTT,                       /**< \brief (Reserved) The number of stitching modes. */
} ISP_STICH_MODE_E;

/** \brief Defines the Bayer patterns. */
typedef enum vsiISP_BAYER_PAT_E {
    BAYER_PAT_RGGB = 0,      /**< \brief 1st line: RGRG... , 2nd line: GBGB... , etc. */
    BAYER_PAT_GRBG = 1,      /**< \brief 1st line: GRGR... , 2nd line: BGBG... , etc. */
    BAYER_PAT_GBRG = 2,      /**< \brief 1st line: GBGB... , 2nd line: RGRG... , etc. */
    BAYER_PAT_BGGR = 3,      /**< \brief 1st line: BGBG... , 2nd line: GRGR... , etc. */
} ISP_BAYER_PAT_E;

/** \brief Defines the sampling edge types. */
typedef enum vsiISP_SAMPLE_EDGE_E {
    SAMPLE_EDGE_FALLING = 0,    /**< \brief Falling edge. */
    SAMPLE_EDGE_RISING  = 1,    /**< \brief Rising edge. */
} ISP_SAMPLE_EDGE_E;

/** \brief Defines the synchronized polarities. */
typedef enum vsiISP_SYNC_POL_E {
    SYNC_POL_HIGH = 0,           /**< \brief The index of rising edge. */
    SYNC_POL_LOW  = 1,           /**< \brief The index of falling edge. */
} ISP_SYNC_POL_E;

/** \brief Defines the subsampling mode. */
typedef enum vsiISP_CONV422_E {
    CONV422_COSITED   = 0,  /**< \brief Co-sited color subsampling Y0Cb0Cr0 - Y1. */
    CONV422_INTER     = 1,  /**< \brief (Not recommended) Interleaved color subsampling Y0Cb0 - Y1Cr1. */
    CONV422_NOCOSITED = 2,  /**< \brief Non-cosited color subsampling Y0Cb(0+1)/2 - Y1Cr(0+1)/2. */
} ISP_CONV422_E;

/** \brief Defines the CCIR sequences. */
typedef enum vsiISP_CCIR_SEQ_E {
    CCIR_YCBYCR = 0,     /**< \brief YCbYCr. */
    CCIR_YCRYCB = 1,     /**< \brief YCrYCb. */
    CCIR_CBYCRY = 2,     /**< \brief CbYCrY. */
    CCIR_CRYCBY = 3,     /**< \brief CrYCbY. */
} ISP_CCIR_SEQ_E;

/** \brief Defines the field sampling modes. */
typedef enum vsiISP_FILED_SEL_E {
    FIELDSEL_BOTH = 0,     /**< \brief Samples all fields. */
    FIELDSEL_EVEN = 1,     /**< \brief Samples only even fields. */
    FIELDSEL_ODD  = 2,     /**< \brief Samples only odd fields. */
} ISP_FILED_SEL_E;

/** \brief Defines the input bus widths. */
typedef enum vsiISP_INPUT_SEL_E {
    INPUT_SEL_12BIT             = 0,   /**< \brief 12-bit input. */
    INPUT_SEL_10BIT_APPEND_ZERO = 1,   /**< \brief 10-bit input with two zeros at LSBs. */
    INPUT_SEL_10BIT_APPEND_MSB  = 2,   /**< \brief 10-bit input with two MSBs at LSBs. */
    INPUT_SEL_8BIT_APPEND_ZERO  = 3,   /**< \brief 8-bit input with four zeros at LSBs. */
    INPUT_SEL_8BIT_APPEND_MSB   = 4,   /**< \brief 8-bit input with four MSBs at LSBs. */
} ISP_INPUT_SEL_E;

/** \brief Defines the input data source types of the latency FIFO. */
typedef enum vsiISP_LATENCY_FIFO_E {
    LATENCY_FIFO_INPUT_FMT = 0,     /**< \brief Data input from the input formatter. */
    LATENCY_FIFO_DMA_READ  = 1,     /**< \brief RGB data input from DMA. */
} ISP_LATENCY_FIFO_E;

/** \brief Specifies ISO for multiple frame, for RDMA use only*/
typedef struct vsiISP_RDMA_ISO_S {
    vsi_u32_t isoNum;                       /**< \brief The number of ISO. */
    vsi_u32_t isoData[0];                   /**< \brief The multi-frame ISO data. */
} ISP_RDMA_ISO_S;

/** \brief Specifies the auto routes. */
typedef struct vsiISP_AUTO_ROUTE_S {
    vsi_u32_t autoRoute[ISP_AUTO_STRENGTH_NUM];  /**< \brief The auto routes. */
    ISP_RDMA_ISO_S rdmaIso;                      /**< \brief The ISO for multiple frame. For RDMA use only. */
} ISP_AUTO_ROUTE_S;

/** \brief Contains the synchronization configurations. */
typedef struct vsiISP_SYNC_CONFIG_S {
    vsi_u32_t  sampleEdge;     /**< \brief The sampling edge type.
                                    \n Valid values: See <tt> \ref ISP_SAMPLE_EDGE_E</tt>. */
    vsi_u32_t  hsyncPol;       /**< \brief The horizontal synchronization polarity.
                                    \n Valid values: See <tt> \ref ISP_SYNC_POL_E</tt>. */
    vsi_u32_t  vsyncPol;       /**< \brief The vertical synchronization polarity.
                                    \n Valid values: See <tt> \ref ISP_SYNC_POL_E</tt>. */
    vsi_u32_t  con422;         /**< \brief The subsampling mode.
                                    \n Valid values: See <tt> \ref ISP_CONV422_E</tt>. */
    vsi_u32_t  ccirSeq;        /**< \brief The CCIR sequence.
                                    \n Valid values: See <tt> \ref ISP_CCIR_SEQ_E</tt>. */
    vsi_u32_t  filedSelection; /**< \brief The field sampling mode.
                                    \n Valid values: See <tt> \ref ISP_FILED_SEL_E</tt>. */
    vsi_u32_t  lantencyFifo;   /**< \brief The input data source type of the latency FIFO.
                                    \n Valid values: See <tt> \ref ISP_LATENCY_FIFO_E</tt>. */
} ISP_SYNC_CONFIG_S;

/** \brief Contains the port attributes of an ISP device port. */
typedef struct vsiISP_PORT_ATTR_S {
    vsi_u32_t         ispInputType;  /**< \brief The input data source type.
                                          \n Valid values: See <tt> \ref ISP_INPUT_TYPE_E</tt>. */
    vsi_u32_t         ispMode;       /**< \brief The input data type.
                                          \n Valid values: See <tt> \ref ISP_MODE_E</tt>. */
    vsi_u32_t         hdrMode;       /**< \brief The HDR mode.
                                          \n Valid values: See <tt> \ref ISP_HDR_MODE_E</tt>. */
    vsi_u32_t         stichMode;     /**< \brief The stitching mode.
                                          \n Valid values: See <tt> \ref ISP_STICH_MODE_E</tt>. */
    vsi_u32_t         pdafMode;     /**< \brief The PDAF mode.
                                          \n Valid values: See <tt> \ref ISP_PDAF_MODE_E</tt>. */
    vsi_u32_t         pixelFormat;   /**< \brief The pixel data format.
                                          \n Valid values: See <tt> \ref PIXEL_FORMAT_E</tt>. */
    ISP_SYNC_CONFIG_S syncCfg;       /**< \brief The synchronization configurations. */
    RECT_S            snsRect;       /**< \brief The sensor rectangle. */
    vsi_u32_t         snsFps;        /**< \brief The sensor FPS. */
    RECT_S            inFormRect;    /**< \brief The inform rectangle. */
    RECT_S            outFormRect;   /**< \brief The outform rectangle. */
    RECT_S            iSRect;        /**< \brief The IS-Crop rectangle. */
    vsi_u32_t         hdrInterval;   /**< \brief The interval line count from L to VS. '0'-not use hdrrt sram(the total L/S/VS
                                      frame will write into DDR); greater than '0'-use hdrrt sram(only stores the
                                      configured line count into DDR); */
} ISP_PORT_ATTR_S;

/** \brief Defines the transmission bus types. */
typedef enum vsiISP_TRANS_BUS_E {
    TRANS_BUS_ONLINE = 0,     /**< \brief Online. */
    TRANS_BUS_FLEXA  = 1,     /**< \brief FLEXA. */
    TRANS_BUS_DMA    = 2,     /**< \brief DMA. */
} ISP_TRANS_BUS_E;

/** \brief Contains the attributes of a channel. */
typedef struct vsiISP_CHN_ATTR_S {
    vsi_u32_t transBus;     /**< \brief The transmission bus type.
                                 \n Valid values: See <tt> \ref ISP_TRANS_BUS_E</tt>. */
    FORMAT_S chnFormat;     /**< \brief The image format. */
} ISP_CHN_ATTR_S;

/** \brief Defines the channel types. */
typedef enum vsiISP_CHN_ID_E {
    CHN_ID_MP   = 0,     /**< \brief Main path. */
    CHN_ID_SP1  = 1,     /**< \brief Self path 1. */
    CHN_ID_SP2  = 2,     /**< \brief Self path 2. */
    CHN_ID_RDMA = 3,     /**< \brief Rdma path. */
    CHN_ID_MP_RAW   = 4,     /**< \brief Main raw path. */
} ISP_CHN_ID_E;

/** \brief Defines the streaming statuses. */
typedef enum vsiISP_STATUS_E {
    ISP_STATE_STREAMOFF = 0,   /**< \brief Streaming off. */
    ISP_STATE_STREAMON  = 1,   /**< \brief Streaming on. */
} ISP_STATUS_E;

/** \brief Defines the operation modes. */
typedef enum vsiISP_OP_TYPE_E {
    OP_TYPE_AUTO = 0,     /**< \brief Auto mode. */
    OP_TYPE_MANUAL = 1,   /**< \brief Manual mode. */
} ISP_OP_TYPE_E;

/** \brief Defines the illuminant types. */
typedef enum vsiISP_ILLUMINANT_TYPE_E {
    ILLUMINANT_A = 0,      /**< \brief A. */
    ILLUMINANT_TL84,       /**< \brief TL84. */
    ILLUMINANT_CWF,        /**< \brief CWF. */
    ILLUMINANT_D50,        /**< \brief D50. */
    ILLUMINANT_D65,        /**< \brief D65. */
    ILLUMINANT_TYPE_CNT,   /**< \brief (Reserved) The number of illuminant types. */
} ISP_ILLUMINANT_TYPE_E;

/** \brief Defines the PDAF modes. */
typedef enum vsiISP_PDAF_MODE_E {
    PDAF_MODE_NOTSUPPORT = 0,    /**< \brief PDAF not supported. */
    PDAF_MODE_TYPE1      = 1,            /**< \brief Type1 mode. */
    PDAF_MODE_TYPE2      = 2,            /**< \brief Type2 mode. */
    PDAF_MODE_TYPE3      = 3,            /**< \brief Type3 mode. */
} ISP_PDAF_MODE_E;

/** \brief   ISP isr notify. */
typedef struct vsiISP_ISR_CALLBACK_S {
    void (*isp_mis)(vsi_u32_t state, void *arg);
    void (*mi_mis)(vsi_u32_t state, void *arg);
    void *args;
} ISP_ISR_CBS_S;
/* @} vsi_comm_isp */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
