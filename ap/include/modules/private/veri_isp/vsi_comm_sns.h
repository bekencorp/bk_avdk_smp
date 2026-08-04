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

#ifndef __VSI_COMM_SNS_H__
#define __VSI_COMM_SNS_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @defgroup vsi_comm_sns Mpp Sensor Common Definitions
 * @{
 *
 *
 */

#define HDR_FRAME_MAX 4            /**< \brief The maximum number of frames for HDR. */
#define ISP_MAX_SNS_REG 64         /**< \brief The maximum number of sensor registers. */
#define ISP_SNS_FPS_ACCU 100       /**< \brief The accuracy of sensor FPS. */
#define ISP_SNS_GAIN_ACCU 1024     /**< \brief The accuracy of sensor gain. */
#define ISP_AE_ROUTE_MAX_NODES 16  /**< \brief The maximum number of AE route nodes. */
#define ISP_ANTIFLCKET_ACCU 0x100  /**< \brief (Reserved) The accuracy of anti-flicker. */

/** \brief   This enumeration specifies the type of the ISP sensor. */
typedef enum vsiISP_SNS_TYPE_E {
    ISP_SNS_I2C_TYPE = 0,    /**< \brief I2C. */
    ISP_SNS_SSP_TYPE,        /**< \brief SSP. */
} ISP_SNS_TYPE_E;

/** \brief   This structure contains the data of the ISP sensor. */
typedef struct vsiISP_I2C_DATA_S {
    vsi_u8_t   delayFrameNum;   /**< \brief The number of delayed frames. */
    vsi_u32_t  regAddr;         /**< \brief The register address. */
    vsi_u32_t  data;            /**< \brief The register data. */
} ISP_SNS_DATA_S;

/** \brief   This structure contains the register information of the ISP sensor. */
typedef struct vsiISP_SNS_REGS_INFO_S {
    vsi_u8_t  snsDev;       /**< \brief The sensor device number. */
    vsi_u8_t  slaveAddr;    /**< \brief The sensor slave address. */
    vsi_u8_t  addrByteNum;  /**< \brief The byte number of address. */
    vsi_u8_t  dataByteNum;  /**< \brief The byte number of data. */
    vsi_u32_t regCnt;       /**< \brief The number of registers. */
    vsi_u8_t  delayMax;     /**< \brief The maximum number of delayed frames. */
    ISP_SNS_DATA_S snsData[ISP_MAX_SNS_REG];   /**< \brief The data of sensor. */
} ISP_SNS_REGS_INFO_S;

/** \brief   This structure defines the ISP sensor mode. */
typedef struct vsiISP_SNS_MODE_S {
    vsi_u32_t width;            /**< \brief The width of image. */
    vsi_u32_t height;           /**< \brief The height of image. */
    vsi_u32_t fps;              /**< \brief The FPS of sensor. */
    vsi_u32_t pixelFormat;      /**< \brief The pixel format, reference PIXEL_FORMAT_E. */
    vsi_u32_t hdrMode;          /**< \brief The HDR mode. For details, see <tt>ISP_HDR_MODE_E</tt>. */
    vsi_u32_t stichMode;        /**< \brief The stitching mode. For details, see <tt>ISP_STICH_MODE_E</tt>. */
} ISP_SNS_MODE_S;

/** \brief   (Reserved) This structure contains the ISP exposure ratio. */
typedef struct vsiISP_EXP_RATIO_S {
    vsi_u32_t opType;                   /**< \brief The mode of operation. For details, see <tt>ISP_OP_TYPE_E</tt>. */
    vsi_u32_t ratio[HDR_FRAME_MAX - 1]; /**< \brief The exposure ratio of HDR. */
    vsi_u32_t minRatio;                 /**< \brief The minimum exposure ratio. */
    vsi_u32_t maxRatio;                 /**< \brief The maximum exposure ratio. */
} ISP_EXP_RATIO_S;

/** \brief   This structure defines the ISP AE range. */
typedef struct vsiISP_AE_RANGE_S {
    vsi_u32_t min;     /**< \brief The minimum value. */
    vsi_u32_t max;     /**< \brief The maximum value. */
} ISP_AE_RANGE_S;

/** \brief   This structure contains the ISP AE route node. */
typedef struct vsiISP_AE_ROTUE_NODE_S {
    vsi_u32_t intTime;       /**< \brief The exposure time, whose range depends on the sensor. */
    vsi_u32_t again;         /**< \brief The analog gain, whose range depends on the sensor. */
    vsi_u32_t dgain;         /**< \brief The digital gain, whose range depends on the sensor. */
} ISP_AE_ROTUE_NODE_S;

/** \brief   This structure defines the ISP AE route. */
typedef struct vsiISP_AE_ROUTE_S {
    vsi_u32_t totalNum;       /**< \brief The number of AE route nodes. Range: [0, 16]. */
    ISP_AE_ROTUE_NODE_S routeNode[ISP_AE_ROUTE_MAX_NODES];  /**< \brief The AE route. */
} ISP_AE_ROUTE_S;

/** \brief   This enumeration specifies the ISP AE mode. */
typedef enum vsiISP_AE_MODE_E {
    AE_MODE_FIX_FRAME_RATE = 0,  /**< \brief Fixed frame rate. */
    AE_MODE_SLOW_SHUTTER = 1,    /**< \brief Slow shutter. */
} ISP_AE_MODE_E;

/** \brief   This structure contains the configurations of the ISP AE anti-flicker. */
typedef struct vsiISP_ANTIFLICKER_S {
    vsi_bool_t enable;      /**< \brief Whether to enable anti-flicker.
                                  \n <tt>0</tt>: Disable anti-flicker. \n <tt>1</tt>: Enable anti-flicker. */
    vsi_u32_t flickerFreq;  /**< \brief The frequency of the anti-flicker. */
} ISP_ANTIFLICKER_S;

/** \brief This structure defines the ISP AE delay. */
typedef struct vsiISP_AE_DELAY_S {
    vsi_u16_t blackDelayFrame;   /**< \brief The delay frames when the scene darkens. */
    vsi_u16_t whiteDelayFrame;   /**< \brief The delay frames when the scene gets brighter. */
} ISP_AE_DELAY_S;

/** \brief   This structure contains the default configurations of AE and the sensor. */
typedef struct vsiAE_SNS_DEFAULT_S {
    vsi_u32_t fullLinesMax;     /**< \brief The maximum value of full lines. */
    vsi_u32_t fullLinesStd;     /**< \brief The standard value of full lines. */
    vsi_u32_t fullLines;        /**< \brief The full lines. */

    vsi_u32_t fps;              /**< \brief The frame rate. */
    vsi_u32_t linesPer500ms;    /**< \brief The number of exposure lines per 500 ms. */

    vsi_u32_t maxIntLine;       /**< \brief The maximum number of exposure lines. */
    vsi_u32_t minIntLine;       /**< \brief The minimum number of exposure lines. */
    vsi_u32_t intLineStep;      /**< \brief The step of exposure lines. */
    vsi_u32_t maxAgain;         /**< \brief The maximum value of analog gain. */
    vsi_u32_t minAgain;         /**< \brief The minimum value of analog gain. */
    vsi_u32_t againStep;        /**< \brief The step of analog gain. */
    vsi_u32_t maxDgain;         /**< \brief The maximum value of digital gain. */
    vsi_u32_t minDgain;         /**< \brief The minimum value of digital gain. */
    vsi_u32_t dgainStep;        /**< \brief The step of digital gain. */

    vsi_u8_t  aeRunInterval;    /**< \brief The number of interval frames to run AE. */
    vsi_u8_t  aeTarget;         /**< \brief The target luminance of AE. */
    vsi_u8_t  dampOver;         /**< \brief The overdamping strength.
                                     \n When the current luminance exceeds the target, the larger this member value, the slower the luminance approaches the target. */
    vsi_u8_t  dampUnder;        /**< \brief The underdamping strength.
                                     \n When the current luminance is below the target, the larger this member value, the slower the luminance approaches the target. */
    vsi_u8_t  tolerance;        /**< \brief The tolerance range.
                                     \n If the actual luminance falls in this range, it is considered that the target luminance is reached. */
    vsi_u32_t initExposure;     /**< \brief The initial exposure. */
    ISP_ANTIFLICKER_S antiflicker;   /**< \brief The ISP anti-flicker configuration. */
    ISP_EXP_RATIO_S   expRatio;      /**< \brief The ISP exposure ratio. */
    ISP_AE_ROUTE_S    aeRoute;       /**< \brief The ISP AE route. */
    vsi_u32_t         aeMode;        /**< \brief The ISP AE mode, reference ISP_AE_MODE_E. */
    vsi_u32_t         gainThreshold; /**< \brief Reserved. */
    ISP_AE_DELAY_S    aeDelayAttr;   /**< \brief The delay frames when the scene luminance changes. */
} AE_SNS_DEFAULT_S;

/** \brief   This structure contains the ISP sensor functions. */
typedef struct vsiISP_SNS_FUNC_S {
    int (*pfnSensorInit)(ISP_PORT IspPort, vsi_u8_t snsDev);  /**< \brief Pointer to the function of initializing sensor. */
    int (*pfnSensorExit)(ISP_PORT IspPort);                   /**< \brief Pointer to the function of exiting sensor. */
    int (*pfnWriteReg)(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data);  /**< \brief Pointer to the function of writing sensor register. */
    int (*pfnReadReg)(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData); /**< \brief Pointer to the function of reading sensor register. */
    int (*pfnSetMode)(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode);  /**< \brief Pointer to the function of setting sensor mode. */
    int (*pfnSetStream)(ISP_PORT IspPort, vsi_bool_t stream);       /**< \brief Pointer to the function of setting stream. */
    int (*pfnSetIspDefault)(ISP_PORT IspPort);                      /**< \brief Pointer to the function of setting the default ISP configurations. */
} ISP_SNS_FUNC_S;

/** \brief   This structure contains the functions used by the AE to control the sensor. */
typedef struct vsiAE_SNS_FUNC_S {
    int (*pfnGetAeDefault)(ISP_PORT IspPort, AE_SNS_DEFAULT_S *pAeSnsDft); /**< \brief Pointer to the function of getting default sensor configurations. */
    int (*pfnSetFps)(ISP_PORT IspPort, vsi_u32_t fps);                     /**< \brief Pointer to the function of setting sensor FPS. */
    int (*pfnSlowFrameRate)(ISP_PORT IspPort, vsi_u32_t fullLines);        /**< \brief Reserved */
    int (*pfnIntTimeUpdate)(ISP_PORT IspPort, vsi_u32_t *pIntLine);        /**< \brief Pointer to the function of updating exposure time. */
    int (*pfnGainUpdate)(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain);  /**< \brief Pointer to the function of updating gain. */
    int (*pfnSetExpRatio) (ISP_PORT IspPort, ISP_EXP_RATIO_S *pExpRatio);          /**< \brief Reserved */
    int (*pfnGetSnsRegInfo)(ISP_PORT IspPort, ISP_SNS_REGS_INFO_S *pSnsRegsInfo);  /**< \brief Pointer to the function of getting sensor register information. */
    int (*pfnQueryExpInfo)(ISP_PORT IspPort, vsi_u32_t *pIntLine, vsi_u32_t *pAgain, vsi_u32_t *pDgain);  /**< \brief Pointer to the function of querying exposure information. */
} AE_SNS_FUNC_S;

/** \brief    This structure contains the ISP sensor function object. */
typedef struct vsiISP_SNS_OBJ_S {
    int (*pfnInitIspSnsFunc) (ISP_SNS_FUNC_S *pIspSnsFunc);  /**< \brief Pointer to the function of initializing the sensor functions. */
    int (*pfnInitAeSnsFunc)  (AE_SNS_FUNC_S *pAeSnsFunc);    /**< \brief Pointer to the function of initializing functions used by the AE to control the sensor. */
} ISP_SNS_OBJ_S;

/* @} vsi_comm_sns */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
