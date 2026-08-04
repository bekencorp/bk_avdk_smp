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
 * @defgroup vsi_comm_sns Sensor Definitions
 * @{
 *
 *
 */

#define HDR_FRAME_MAX 4            /**< \brief The maximum number of HDR frames. */
#define HDR_EXPOSURE_RATIO_MAX_NUM (HDR_FRAME_MAX - 1)   /**< \brief The maximum number of HDR exposure ratio. */
#define ISP_MAX_SNS_REG 64         /**< \brief The maximum number of sensor registers. */
#define ISP_SNS_FPS_ACCU 100       /**< \brief The accuracy of sensor FPS. */
#define ISP_SNS_GAIN_ACCU 1024     /**< \brief The accuracy of sensor gain. */
#define ISP_AE_ROUTE_MAX_NODES 16  /**< \brief The maximum number of nodes on each auto exposure (AE) route. */
#define ISP_ANTIFLCKET_ACCU 0x100  /**< \brief (Reserved) The accuracy of anti-flicker. */

#define ISP_SNS_LINEAR_INDEX         0  /**< \brief The index of linear mode. */
#define ISP_SNS_DUAL_EXP_L_INDEX     0  /**< \brief The index of of long frame in dual-exposure HDR. */
#define ISP_SNS_DUAL_EXP_S_INDEX     1  /**< \brief The index of of short frame in dual-exposure HDR. */
#define ISP_SNS_TRI_EXP_L_INDEX      0  /**< \brief The index of of long frame in triple-exposure HDR. */
#define ISP_SNS_TRI_EXP_S_INDEX      1  /**< \brief The index of of short frame in triple-exposure HDR. */
#define ISP_SNS_TRI_EXP_VS_INDEX     2  /**< \brief The index of of very short frame triple-exposure HDR. */
#define ISP_SNS_QUAD_EXP_L_INDEX     0  /**< \brief The index of of long frame in quad-exposure HDR. */
#define ISP_SNS_QUAD_EXP_S_INDEX     1  /**< \brief The index of of short frame in quad-exposure HDR. */
#define ISP_SNS_QUAD_EXP_VS_INDEX    2  /**< \brief The index of of very short frame in quad-exposure HDR. */
#define ISP_SNS_QUAD_EXP_VVS_INDEX   3  /**< \brief The index of of very very short frame in quad-exposure HDR. */

/** \brief Defines the sensor types. */
typedef enum vsiISP_SNS_TYPE_E {
    ISP_SNS_I2C_TYPE = 0,    /**< \brief I2C. */
    ISP_SNS_SSP_TYPE,        /**< \brief SSP. */
} ISP_SNS_TYPE_E;

/** \brief Contains the information of a sensor register used in register write.
    \n The value will be written to the register after the specified number of delayed frames. */
typedef struct vsiISP_I2C_DATA_S {
    vsi_u8_t   delayFrameNum;   /**< \brief The number of delayed frames. */
    vsi_u32_t  regAddr;         /**< \brief The address of the register. */
    vsi_u32_t  data;            /**< \brief The value of the register. */
} ISP_SNS_DATA_S;

/** \brief Contains the register information of a sensor. */
typedef struct vsiISP_SNS_REGS_INFO_S {
    vsi_u8_t  snsDev;       /**< \brief The ID of the sensor device. */
    vsi_u8_t  slaveAddr;    /**< \brief The slave address of the sensor. */
    vsi_u8_t  addrByteNum;  /**< \brief The number of address bytes. */
    vsi_u8_t  dataByteNum;  /**< \brief The number of data bytes. */
    vsi_u32_t regCnt;       /**< \brief The number of registers. */
    vsi_u8_t  delayMax;     /**< \brief The maximum number of delayed frames. */
    ISP_SNS_DATA_S snsData[ISP_MAX_SNS_REG];   /**< \brief The information of each register. */
} ISP_SNS_REGS_INFO_S;

/** \brief Specifies an ISP sensor mode. */
typedef struct vsiISP_SNS_MODE_S {
    vsi_u32_t width;            /**< \brief The image width of the sensor. */
    vsi_u32_t height;           /**< \brief The image height of the sensor. */
    vsi_u32_t fps;              /**< \brief The FPS of the sensor. */
    vsi_u32_t pixelFormat;      /**< \brief The pixel data format.
                                     \n Valid values: See <tt> \ref PIXEL_FORMAT_E</tt>. */
    vsi_u32_t hdrMode;          /**< \brief The HDR mode.
                                     \n Valid values: See <tt> \ref ISP_HDR_MODE_E</tt>. */
    vsi_u32_t stichMode;        /**< \brief The stitching mode.
                                     \n Valid values: See <tt> \ref ISP_STICH_MODE_E</tt>. */
    vsi_u32_t pdafMode;         /**< \brief The PDAF mode.
                                     \n Valid values: See <tt> \ref ISP_PDAF_MODE_E</tt>. */

} ISP_SNS_MODE_S;

/** \brief (Reserved) Contains the configurations of the exposure ratio. */
typedef struct vsiISP_EXP_RATIO_S {
    vsi_u32_t opType;                   /**< \brief The operation mode of the exposure module.
                                             \n Valid values: See <tt> \ref ISP_OP_TYPE_E</tt>. */
    vsi_u32_t ratio[HDR_EXPOSURE_RATIO_MAX_NUM]; /**< \brief The exposure ratio of HDR. */
    vsi_u32_t minRatio;                 /**< \brief The minimum exposure ratio. */
    vsi_u32_t maxRatio;                 /**< \brief The maximum exposure ratio. */
} ISP_EXP_RATIO_S;

/** \brief Specifies a range for auto exposure (AE). */
typedef struct vsiISP_AE_RANGE_S {
    vsi_u32_t min;     /**< \brief The minimum value of the range. */
    vsi_u32_t max;     /**< \brief The maximum value of the range. */
} ISP_AE_RANGE_S;

/** \brief Specifies an auto exposure (AE) route node. */
typedef struct vsiISP_AE_ROUTE_NODE_S {
    vsi_u32_t intTime;       /**< \brief The exposure time.
                                  \n Vaid value range: Depends on the sensor. */
    vsi_u32_t aGain;         /**< \brief The analog gain.
                                  \n Vaid value range: Depends on the sensor. */
    vsi_u32_t dGain;         /**< \brief The digital gain.
                                  \n Vaid value range: Depends on the sensor. */
} ISP_AE_ROUTE_NODE_S;

/** \brief Specifies an auto exposure (AE) route. */
typedef struct vsiISP_AE_ROUTE_S {
    vsi_u32_t totalNum;       /**< \brief The number of nodes on the AE route.
                                   \n Valid value range: [0, 16]. */
    ISP_AE_ROUTE_NODE_S routeNode[ISP_AE_ROUTE_MAX_NODES];  /**< \brief The AE route nodes. */
} ISP_AE_ROUTE_S;

/** \brief Defines the auto exposure (AE) modes. */
typedef enum vsiISP_AE_MODE_E {
    AE_MODE_FIX_FRAME_RATE = 0,  /**< \brief Fixed frame rate. */
    AE_MODE_SLOW_SHUTTER = 1,    /**< \brief Slow shutter. */
} ISP_AE_MODE_E;

/** \brief Contains the configurations of the anti-flicker in auto exposure (AE). */
typedef struct vsiISP_ANTIFLICKER_S {
    vsi_bool_t enable;      /**< \brief Whether to enable anti-flicker.
                                 \n Valid values:
                                 \n - <tt>0</tt>: Disable.
                                 \n - <tt>1</tt>: Enable. */
    vsi_u32_t flickerFreq;  /**< \brief The frequency of the anti-flicker. */
} ISP_ANTIFLICKER_S;

/** \brief Contains the delay configurations of auto exposure (AE). */
typedef struct vsiISP_AE_DELAY_S {
    vsi_u16_t blackDelayFrame;   /**< \brief The number of frames to be delayed when the scene darkens. */
    vsi_u16_t whiteDelayFrame;   /**< \brief The number of frames to be delayed when the scene brightens. */
} ISP_AE_DELAY_S;

/** \brief Contains the default configurations of auto exposure (AE) and the sensor of an ISP device port. */
typedef struct vsiAE_SNS_DEFAULT_S {
    vsi_u32_t fullLinesMax;     /**< \brief The maximum number of full exposure lines. */
    vsi_u32_t fullLinesStd;     /**< \brief The standard number of full exposure lines. */
    vsi_u32_t fullLines;        /**< \brief The actual number of full exposure lines. */

    vsi_u32_t fps;              /**< \brief The FPS of the sensor. */
    vsi_u32_t linesPer500ms;    /**< \brief The number of exposure lines per 500 ms. */

    vsi_u32_t maxIntLine;       /**< \brief The maximum number of exposure lines. */
    vsi_u32_t minIntLine;       /**< \brief The minimum number of exposure lines. */
    vsi_u32_t intLineStep;      /**< \brief The step in the number of exposure lines. */
    vsi_u32_t maxAgain;         /**< \brief The maximum analog gain. */
    vsi_u32_t minAgain;         /**< \brief The minimum analog gain. */
    vsi_u32_t againStep;        /**< \brief The step in the analog gain. */
    vsi_u32_t maxDgain;         /**< \brief The maximum digital gain. */
    vsi_u32_t minDgain;         /**< \brief The minimum digital gain. */
    vsi_u32_t dgainStep;        /**< \brief The step in the digital gain. */

    vsi_u8_t  runInterval;    /**< \brief The interval at which to run AE.
                                     \n Unit: Frames. */
    vsi_u8_t  target;         /**< \brief The target luminance of AE. */
    vsi_u8_t  dampOver;         /**< \brief The overdamping strength.
                                     \n When the current luminance exceeds the target, a greater value indicates a slower speed at which the luminance approaches the target. */
    vsi_u8_t  dampUnder;        /**< \brief The underdamping strength.
                                     \n When the current luminance is below the target, a greater value indicates a slower speed at which the luminance approaches the target. */
    vsi_u8_t  tolerance;        /**< \brief The tolerance range of luminance.
                                     \n If the actual luminance falls in the tolerance range, it is considered that the target luminance is reached. */
    vsi_u32_t initExposure;     /**< \brief The initial exposure. */
    ISP_ANTIFLICKER_S antiFlicker;   /**< \brief The anti-flicker configurations. */
    ISP_EXP_RATIO_S   expRatio;      /**< \brief The exposure ratio configurations. */
    ISP_AE_ROUTE_S    aeRoute;       /**< \brief The AE routes. */
    vsi_u32_t         aeMode;        /**< \brief The AE mode.
                                          \n Valid values: See <tt> \ref ISP_AE_MODE_E</tt>. */
    vsi_u32_t         gainThreshold; /**< \brief The gain threshold.
                                          \n This field is valid only if <tt>AE_SNS_DEFAULT_S.aeMode</tt> is set to <tt>AE_MODE_SLOW_SHUTTER</tt>. */
    ISP_AE_DELAY_S    delayAttr;   /**< \brief The delay configurations. */
} AE_SNS_DEFAULT_S;

/** \brief Defines the auto focus (AF) modes. */
typedef enum vsiISP_AF_MODE_E {
    AF_MODE_NOT_SUPP = 0,    /**< \brief AF not supported. */
    AF_MODE_CDAF = 1,        /**< \brief CDAF mode. */
    AF_MODE_PDAF = 2,        /**< \brief PDAF mode. */
} ISP_AF_MODE_E;

/** \brief Defines the POS focusing modes. */
typedef enum vsiISP_FOCUS_POS_MODE_E {
    FOCUS_POS_MODE_ABSOLUTE = 0,   /**< \brief Absolute focusing mode. */
    FOCUS_POS_MODE_RELATIVE = 1,   /**< \brief Relative focusing mode. */
} ISP_FOCUS_POS_MODE_E;

/** \brief Defines the PDAF sensor types. */
typedef enum vsiISP_PDAF_SENSOR_TYPE_E {
    PDAF_SENSOR_DUAL_PIXEL = 0,    /**< \brief Dual-pixel PDAF sensor. */
    PDAF_SENSOR_OCL2X1 = 1,        /**< \brief 2x1 overlapped cell PDAF sensor. */
    PDAF_SENSOR_TYPE_MAX,          /**< \brief The number of PDAF sensor types. */
} ISP_PDAF_SENSOR_TYPE_E;

/** \brief Contains the focus position information. */
typedef struct vsiISP_FOCUS_POS_INFO_S {
    vsi_u32_t minPos;             /**< \brief The minimum focus position. */
    vsi_u32_t maxPos;             /**< \brief The maximum focus position. */
    vsi_u32_t minStep;            /**< \brief The minimum focus position step. */
} ISP_FOCUS_POS_INFO_S;

/** \brief Contains the focus phase detection information. */
typedef struct vsiISP_FOCUS_PDINFO_S {
    vsi_u32_t sensorType;            /**< \brief The ISP PDAF sensor type.
                                          \n Valid values: See <tt> \ref ISP_PDAF_SENSOR_TYPE_E</tt>. */
    vsi_u32_t bayerPattern;          /**< \brief The Bayer patterns.
                                          \n Valid values: See <tt> \ref ISP_BAYER_PAT_E</tt>. */
    vsi_bool_t   ocl2x1Shield;       /**< \brief The OCL2X1 shield. */
    vsi_u8_t  bitWidth;              /**< \brief The bit width. */
    vsi_u32_t imageWidth;            /**< \brief The image width. */
    vsi_u32_t imageHeight;           /**< \brief The image height. */
    vsi_u16_t pdArea[4];             /**< \brief The phase detection area. */
    vsi_u32_t correctRect[4];        /**< \brief The rectangle correction. */
    vsi_u8_t  pdNumPerArea[2];       /**< \brief The number of phase detection points per area. */
    vsi_u8_t  pdShiftL2R[2];         /**< \brief The pixel offsets from the left phase unit (L-PD) to the
                                                 right phase unit (R-PD) in horizontal and vertical dimensions. */
    vsi_u8_t  pdShiftMark[32];       /**< \brief The phase detection shift of the mark points. */
    vsi_u8_t  pdFocalHeight;         /**< \brief The height of the phase detection ROI focal. */
    vsi_u8_t  pdFocalWidth;          /**< \brief The width of the phase detection ROI focal. */
    vsi_u8_t  pdDistance;            /**< \brief The phase detection distance. */
    vsi_u8_t  pdFocal[48];           /**< \brief The phase detection focal spots. */
} ISP_FOCUS_PDINFO_S;

/** \brief Contains the focus attributes of sensor. */
typedef struct vsiISP_FOCUS_CALIB_ATTR_S {
    ISP_FOCUS_POS_INFO_S posInfo;    /**< \brief The focus position information. */
    ISP_FOCUS_PDINFO_S  pdInfo;      /**< \brief The focus phase detection information. */
} ISP_FOCUS_CALIB_ATTR_S;

/** \brief Specifies a focus position. */
typedef struct vsiISP_FOCUS_POS_S {
    vsi_u32_t posType;           /**< \brief The POS focusing mode applied when the scene darkens.
                                      \n Valid values: See <tt> \ref ISP_FOCUS_POS_MODE_E</tt>. */
    vsi_u32_t pos;               /**< \brief The focused posotion. */
} ISP_FOCUS_POS_S;

/** \brief Contains the default configurations of auto focus (AF) and the sensor of an ISP device port. */
typedef struct vsiAF_SNS_DEFAULT_S {
    vsi_u32_t afMode;                       /**< \brief The ISP AE mode.
                                                 \n Valid values: See <tt> \ref ISP_AF_MODE_E</tt>. */
    ISP_FOCUS_CALIB_ATTR_S focusCalibAttr;  /**< \brief The focus attributes of sensor. */
    ISP_FOCUS_POS_S focusPos;               /**< \brief The focused posotion information. */
} AF_SNS_DEFAULT_S;

/** \brief Contains the functions that control the sensor of an ISP device port. */
typedef struct vsiISP_SNS_FUNC_S {
    int (*pfnSensorInit)(ISP_PORT IspPort, vsi_u8_t snsDev);  /**< \brief A pointer to the function that initializes the sensor. */
    int (*pfnSensorExit)(ISP_PORT IspPort);                   /**< \brief A pointer to the function that exits the sensor. */
    int (*pfnWriteReg)(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data);  /**< \brief A pointer to the function that writes a register for the sensor. */
    int (*pfnReadReg)(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData); /**< \brief A pointer to the function that reads a register for the sensor. */
    int (*pfnSetMode)(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode);  /**< \brief A pointer to the function that sets the mode for the sensor. */
    int (*pfnSetStream)(ISP_PORT IspPort, vsi_bool_t stream);       /**< \brief A pointer to the function that sets a stream for the sensor. */
    int (*pfnSetIspDefault)(ISP_PORT IspPort);                      /**< \brief A pointer to the function that sets the default AE and sensor configurations. */
} ISP_SNS_FUNC_S;


/** \brief Contains the functions that auto exposure (AE) uses to control the sensor of an ISP device port. */
typedef struct vsiAE_SNS_FUNC_S {
    int (*pfnGetAeDefault)(ISP_PORT IspPort, AE_SNS_DEFAULT_S *pAeSnsDft); /**< \brief A pointer to the function that gets the default AE and sensor configurations. */
    int (*pfnSetFps)(ISP_PORT IspPort, vsi_u32_t fps);                     /**< \brief A pointer to the function that sets the sensor FPS. */
    int (*pfnSlowFrameRate)(ISP_PORT IspPort, vsi_u32_t fullLines);        /**< \brief A pointer to the function that reduces the sensor FPS. */
    int (*pfnIntTimeUpdate)(ISP_PORT IspPort, vsi_u32_t *pIntLine);        /**< \brief A pointer to the function that updates the exposure time of the sensor. */
    int (*pfnGainUpdate)(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain);  /**< \brief A pointer to the function that updates the gains of the sensor. */
    int (*pfnSetExpRatio) (ISP_PORT IspPort, ISP_EXP_RATIO_S *pExpRatio);          /**< \brief (Reserved) A pointer to the function that sets the exposure ratio of the sensor. */
    int (*pfnGetSnsRegInfo)(ISP_PORT IspPort, ISP_SNS_REGS_INFO_S *pSnsRegsInfo);  /**< \brief A pointer to the function that gets the sensor register information. */
    int (*pfnQueryExpInfo)(ISP_PORT IspPort, vsi_u32_t *pIntLine, vsi_u32_t *pAgain, vsi_u32_t *pDgain);  /**< \brief A pointer to the function that queries the AE route information of the sensor. */
} AE_SNS_FUNC_S;

/** \brief Contains the functions that auto focus (AF) uses to control the sensor of an ISP device port. */
typedef struct vsiAF_SNS_FUNC_S {
    int (*pfnFocusInit)(ISP_PORT IspPort, vsi_u8_t snsDev);                /**< \brief A pointer to the function that initializes focusing for the sensor. */
    int (*pfnFocusExit)(ISP_PORT IspPort);                                 /**< \brief A pointer to the function that releases focusing for the sensor. */
    int (*pfnFocusSetPos)(ISP_PORT IspPort, ISP_FOCUS_POS_S *pPos);        /**< \brief A pointer to the function that sets the focused position of the sensor. */
    int (*pfnFocusGetPos)(ISP_PORT IspPort, ISP_FOCUS_POS_S *pPos);        /**< \brief A pointer to the function that gets the focused position of the sensor. */
    int (*pfnGetAfDefault)(ISP_PORT IspPort, AF_SNS_DEFAULT_S *pAfSnsDft); /**< \brief A pointer to the function that gets the default configurations of the sensor. */
} AF_SNS_FUNC_S;

/** \brief Contains the functions that initialize the sensor-related functions. */
typedef struct vsiISP_SNS_OBJ_S {
    int (*pfnInitIspSnsFunc) (ISP_SNS_FUNC_S *pIspSnsFunc);  /**< \brief A pointer to the function that initializes the sensor functions. */
    int (*pfnInitAeSnsFunc)  (AE_SNS_FUNC_S *pAeSnsFunc);    /**< \brief A pointer to the function that initializes the functions for AE to control a sensor. */
    int (*pfnInitAfSnsFunc)  (AF_SNS_FUNC_S *pAfSnsFunc);    /**< \brief A pointer to the function that initializes the functions for AF to control a sensor. */
} ISP_SNS_OBJ_S;

/* @} vsi_comm_sns */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
