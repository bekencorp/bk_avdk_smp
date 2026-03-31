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
 * @defgroup mpi_isp Mpp ISP Common Definitions
 * @{
 *
 *
 */

#define ISP_AUTO_STRENGTH_NUN 16     /**< \brief The number of auto strengths. */

typedef vsi_u32_t ISP_DEV;           /**< \brief The ISP device. */

/** \brief   This structure specifies the ISP port. */
typedef struct {
    vsi_u32_t devId;  /**< \brief The device ID. */
    vsi_u32_t portId; /**< \brief The port ID. */
} ISP_PORT;

/** \brief   This structure specifies the ISP channel. */
typedef struct {
    vsi_u32_t devId;  /**< \brief The device ID. */
    vsi_u32_t portId; /**< \brief The port ID. */
    vsi_u32_t chnId;  /**< \brief The channel ID. */
} ISP_CHN;

/** \brief   This enumeration specifies the working mode of ISP. */
typedef enum vsiISP_WORK_MODE_E {
    WORK_MODE_NORMAL = 0,  /**< \brief Normal working mode. */
    WORK_MODE_MCM    = 1,  /**< \brief MCM */
    WORK_MODE_BUTT         /**< \brief (For internal evaluation only) Upper border. */
} ISP_WORK_MODE_E;

/** \brief   This enumeration specifies the device attributes of ISP. */
typedef struct vsiISP_DEV_ATTR_S {
    vsi_u32_t ispWorkMode; /**< \brief ISP working mode. See <tt>ISP_WORK_MODE_E</tt> */
} ISP_DEV_ATTR_S;

/** \brief   This enumeration specifies the data input type of ISP. */
typedef enum vsiISP_INPUT_TYPE_E {
    INPUT_TYPE_SENSOR = 0,  /**< \brief Data from sensor port to ISP. */
    INPUT_TYPE_CSI_SENSOR = 0,
    INPUT_TYPE_TPG    = 1,  /**< \brief Data from ISP TPG to ISP. */
    INPUT_TYPE_DMA    = 2,  /**< \brief Data from DMA to ISP. */
    INPUT_TYPE_DVP_SENSOR = 3,
    INPUT_TYPE_BUTT         /**< \brief (For internal evaluation only) Upper border. */
} ISP_INPUT_TYPE_E;

/** \brief   This enumeration specifies the input data mode of ISP. */
typedef enum vsiISP_MODE_E
{
    ISP_MODE_BT656  = 1,  /**< \brief ITU-R BT.656 (YUV with embedded synchronization). */
    ISP_MODE_BT601  = 2,  /**< \brief ITU-R BT.601 (YUV input with Hsync and Vsync signals). */
    ISP_MODE_RAW    = 3,  /**< \brief RAW picture with BT.601 synchronization (ISP bypass). */
    ISP_MODE_BUTT,        /**< \brief (For internal evaluation only) Upper border. */
} ISP_MODE_E;

/** \brief   This enumeration specifies the HDR mode. */
typedef enum vsiISP_HDR_MODE_E {
    HDR_MODE_LINEAR        = 0,  /**< \brief Linear */
    HDR_MODE_ISP_STICH     = 1,  /**< \brief ISP stitching */
    HDR_MODE_SENSOR_STICH  = 2,  /**< \brief Sensor stitching */
    HDR_MODE_BUTT,               /**< \brief (For internal evaluation only) Upper border. */
} ISP_HDR_MODE_E;

/** \brief   This enumeration specifies the stitching mode. */
typedef enum vsiISP_STICH_MODE_E {
    STICH_MODE_3DOL_DUAL_DCG          = 0, /**< \brief 3DOL dual DCG. */
    STICH_MODE_3DOL                   = 1, /**< \brief 3DOL. */
    STICH_MODE_3DOL_LINEBYLINE        = 2, /**< \brief 3DOL line by line. */
    STICH_MODE_3DOL_16BIT_COMPRESS    = 3, /**< \brief 3DOL 16-bit compression. */
    STICH_MODE_2DOL_DUAL_DCG          = 4, /**< \brief 2DOL dual DCG. */
    STICH_MODE_2DOL                   = 5, /**< \brief 2DOL. */
    STICH_MODE_2DOL_L_AND_S           = 6, /**< \brief 2DOL low and small frequency. */
    STICH_MODE_BUTT,                       /**< \brief (For internal evaluation only) Upper border. */
} ISP_STICH_MODE_E;

/** \brief   This enumeration specifies the Bayer pattern. */
typedef enum vsiISP_BAYER_PAT_E {
    BAYER_PAT_RGGB = 0,      /**< \brief 1st line: RGRG... , 2nd line: GBGB... , etc. */
    BAYER_PAT_GRBG = 1,      /**< \brief 1st line: GRGR... , 2nd line: BGBG... , etc. */
    BAYER_PAT_GBRG = 2,      /**< \brief 1st line: GBGB... , 2nd line: RGRG... , etc. */
    BAYER_PAT_BGGR = 3,      /**< \brief 2st line: BGBG... , 2nd line: GRGR... , etc. */
} ISP_BAYER_PAT_E;

/** \brief   This enumeration specifies the sample edge. */
typedef enum vsiISP_SAMPLE_EDGE_E {
    SAMPLE_EDGE_FALLING = 0,    /**< \brief Sample falling edge. */
    SAMPLE_EDGE_RISING  = 1,    /**< \brief Sample rising edge. */
} ISP_SAMPLE_EDGE_E;

/** \brief   This enumeration specifies the synchronized polarity. */
typedef enum vsiISP_SYNC_POL_E {
    SYNC_POL_HIGH = 0,           /**< \brief The index of rising edge. */
    SYNC_POL_LOW  = 1,           /**< \brief The index of falling edge. */
} ISP_SYNC_POL_E;

/** \brief   This enumeration specifies the subsampling mode. */
typedef enum vsiISP_CONV422_E {
    CONV422_COSITED   = 0,  /**< \brief Co-sited color subsampling Y0Cb0Cr0 - Y1. */
    CONV422_INTER     = 1,  /**< \brief (Not recommended) Interleaved color subsampling Y0Cb0 - Y1Cr1. */
    CONV422_NOCOSITED = 2,  /**< \brief Non-cosited color subsampling Y0Cb(0+1)/2 - Y1Cr(0+1)/2. */
} ISP_CONV422_E;

/** \brief   This enumeration specifies the CCIR sequence. */
typedef enum vsiISP_CCIR_SEQ_E {
    CCIR_YCBYCR = 0,     /**< \brief YCBYCR. */
    CCIR_YCRYCB = 1,     /**< \brief YCRYCB. */
    CCIR_CBYCRY = 2,     /**< \brief CBYCRY. */
    CCIR_CRYCBY = 3,     /**< \brief CRYCBY. */
} ISP_CCIR_SEQ_E;

/** \brief   This enumeration specifies the field of sampling. */
typedef enum vsiISP_FILED_SEL_E {
    FIELDSEL_BOTH = 0,     /**< \brief Sample all fields. */
    FIELDSEL_EVEN = 1,     /**< \brief Sample only even fields. */
    FIELDSEL_ODD  = 2,     /**< \brief Sample only odd fields. */
} ISP_FILED_SEL_E;

/** \brief   This enumeration specifies the input bus width. */
typedef enum vsiISP_INPUT_SEL_E {
    INPUT_SEL_12BIT             = 0,   /**< \brief 12-bit input */
    INPUT_SEL_10BIT_APPEND_ZERO = 1,   /**< \brief 10-bit input with 2 zereos as LSBs */
    INPUT_SEL_10BIT_APPEND_MSB  = 2,   /**< \brief 10-bit input with 2 MSBs as LSBs */
    INPUT_SEL_8BIT_APPEND_ZERO  = 3,   /**< \brief 8-bit input with 4 zeroes as LSBs */
    INPUT_SEL_8BIT_APPEND_MSB   = 4,   /**< \brief 8-bit input with 4 MSBs as LSBs */
} ISP_INPUT_SEL_E;

/** \brief   This enumeration specifies the data input mode of the latency FIFO. */
typedef enum vsiISP_LATENCY_FIFO_E {
    LATENCY_FIFO_INPUT_FMT = 0,     /**< \brief The latency FIFO takes data from the input formatter. */
    LATENCY_FIFO_DMA_READ  = 1,     /**< \brief The latency FIFO takes data from the RGB data from DMA. */
} ISP_LATENCY_FIFO_E;

/** \brief   This structure specifies the auto route. */
typedef struct vsiISP_AUTO_ROUTE_S {
    vsi_u32_t autoRoute[ISP_AUTO_STRENGTH_NUN];  /**< \brief ISP auto route. */
} ISP_AUTO_ROUTE_S;

/** \brief   This structure specifies the synchronization configuration. */
typedef struct vsiISP_SYNC_CONFIG_S {
    vsi_u32_t  sampleEdge;     /**< \brief The sampling edge. For details, see <tt>ISP_SAMPLE_EDGE_E</tt>. */
    vsi_u32_t  hsyncPol;       /**< \brief Horizontal synchronization polarity. For details, see <tt>ISP_SYNC_POL_E</tt>. */
    vsi_u32_t  vsyncPol;       /**< \brief Vertical synchronization polarity. For details, see <tt>ISP_SYNC_POL_E</tt>. */
    vsi_u32_t  con422;         /**< \brief ISP subsampling mode. For details, see <tt>ISP_CONV422_E</tt>. */
    vsi_u32_t  ccirSeq;        /**< \brief ISP CCIR sequence. For details, see <tt>ISP_CCIR_SEQ_E</tt>. */
    vsi_u32_t  filedSelection; /**< \brief ISP field sampling mode. For details, see <tt>ISP_FILED_SEL_E</tt>. */
    vsi_u32_t  lantencyFifo;   /**< \brief ISP latency FIFO mode. For details, see <tt>ISP_LATENCY_FIFO_E</tt>. */
} ISP_SYNC_CONFIG_S;

/** \brief   This structure specifies the port attributes. */
typedef struct vsiISP_PORT_ATTR_S {
    vsi_u32_t         ispInputType;  /**< \brief ISP input type. For details, see <tt>ISP_INPUT_TYPE_E</tt>. */
    vsi_u32_t         ispMode;       /**< \brief ISP mode. For details, see <tt>ISP_MODE_E</tt>. */
    vsi_u32_t         hdrMode;       /**< \brief ISP HDR mode. For details, see <tt>ISP_HDR_MODE_E</tt>. */
    vsi_u32_t         stichMode;     /**< \brief ISP stitching mode. For details, see <tt>ISP_STICH_MODE_E</tt>. */
    vsi_u32_t         pixelFormat;   /**< \brief Pixel format. For details, see <tt>PIXEL_FORMAT_E</tt>. */
    ISP_SYNC_CONFIG_S syncCfg;       /**< \brief ISP synchronization configurations. */
    RECT_S            snsRect;       /**< \brief Sensor rectangle parameters. */
    vsi_u32_t         snsFps;        /**< \brief Sensor FPS. */
    RECT_S            inFormRect;    /**< \brief Inform rectangle parameters. */
    RECT_S            outFormRect;   /**< \brief Outform rectangle parameters. */
    RECT_S            iSRect;        /**< \brief IS-Crop rectangle parameters. */
} ISP_PORT_ATTR_S;

/** \brief   This enumeration specifies the transmission bus mode. */
typedef enum vsiISP_TRANS_BUS_E {
    TRANS_BUS_ONLINE = 0,     /**< \brief Online. */
    TRANS_BUS_FLEXA  = 1,     /**< \brief FLEXA. */
    TRANS_BUS_DMA    = 2,     /**< \brief DMA. */
} ISP_TRANS_BUS_E;

/** \brief   This structure specifies the channel attributes. */
typedef struct vsiISP_CHN_ATTR_S {
    vsi_u32_t transBus;     /**< \brief The transmission bus mode. For details, see <tt>ISP_TRANS_BUS_E</tt>. */
    FORMAT_S chnFormat;     /**< \brief Channel format. */
} ISP_CHN_ATTR_S;

/** \brief   This enumeration specifies the channel ID. */
typedef enum vsiISP_CHN_ID_E {
    CHN_ID_MP   = 0,     /**< \brief Main path. */
    CHN_ID_SP1  = 1,     /**< \brief Self path 1. */
    CHN_ID_SP2  = 2,     /**< \brief Self path 2. */
    CHN_ID_RDMA = 3,     /**< \brief Rdma path. */
} ISP_CHN_ID_E;

/** \brief   This enumeration specifies the streaming status. */
typedef enum vsiISP_STATUS_E {
    ISP_STATE_STREAMOFF = 0,   /**< \brief Streaming off. */
    ISP_STATE_STREAMON  = 1,   /**< \brief Streaming on. */
} ISP_STATUS_E;

/** \brief   This enumeration specifies the operation mode. */
typedef enum vsiISP_OP_TYPE_E {
    OP_TYPE_AUTO = 0,     /**< \brief Auto mode. */
    OP_TYPE_MANUAL = 1,   /**< \brief Manual mode. */
} ISP_OP_TYPE_E;

/** \brief   This enumeration specifies the illuminant type. */
typedef enum vsiISP_ILLUMINANT_TYPE_E {
    ILLUMINANT_A = 0,      /**< \brief A. */
    ILLUMINANT_TL84,       /**< \brief TL84. */
    ILLUMINANT_CWF,        /**< \brief CWF. */
    ILLUMINANT_D50,        /**< \brief D50. */
    ILLUMINANT_D65,        /**< \brief D65. */
    ILLUMINANT_TYPE_CNT,   /**< \brief The number of illuminant types. */
} ISP_ILLUMINANT_TYPE_E;

typedef void (*isp_isr_event_cb)(vsi_u32_t event);

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
