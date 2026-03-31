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

#ifndef __MPI_ISP_CALIB_H__
#define __MPI_ISP_CALIB_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @defgroup mpi_isp_calib Mpp Calibration Definitions
 * @{
 *
 */

#include "vsi_comm_calib.h"

/*****************************************************************************/
/**
 * @brief   Sets calibration data.
 *
 * @param   IspPort             Port ID
 * @param   pCalibData          Pointer to the calibration data

 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
static inline int VSI_MPI_ISP_SetCalib(ISP_PORT IspPort, ISP_CALIB_DATA_S *pCalibData)
{
    VSI_MPI_ISP_SetAutoRoute(IspPort, &pCalibData->modules.autoRoute);
    VSI_MPI_ISP_SetBlsAttr(IspPort, &pCalibData->modules.bls);
    VSI_MPI_ISP_SetGammaInAttr(IspPort, &pCalibData->modules.gammaIn);
    VSI_MPI_ISP_SetDgAttr(IspPort, &pCalibData->modules.dg);
    VSI_MPI_ISP_SetExpmAttr(IspPort, &pCalibData->modules.aem);
    VSI_MPI_ISP_SetHist256Attr(IspPort, &pCalibData->modules.hist256);
    VSI_MPI_ISP_SetExposureAttr(IspPort, &pCalibData->modules.ae);
    VSI_MPI_ISP_SetWbmAttr(IspPort, &pCalibData->modules.wbm);
    VSI_MPI_ISP_SetWbAttr(IspPort, &pCalibData->modules.wb);
    VSI_MPI_ISP_SetLscAttr(IspPort, &pCalibData->modules.lsc);
    VSI_MPI_ISP_SetWdrAttr(IspPort, &pCalibData->modules.wdr);
    VSI_MPI_ISP_SetGeAttr(IspPort, &pCalibData->modules.ge);
    VSI_MPI_ISP_SetDpccAttr(IspPort, &pCalibData->modules.dpcc);
    VSI_MPI_ISP_SetDpfAttr(IspPort, &pCalibData->modules.dpf);
    VSI_MPI_ISP_Set2DnrAttr(IspPort, &pCalibData->modules.nr2d);
    VSI_MPI_ISP_SetDmscAttr(IspPort, &pCalibData->modules.dmsc);
    VSI_MPI_ISP_SetFltAttr(IspPort, &pCalibData->modules.flt);
    VSI_MPI_ISP_SetCcmAttr(IspPort, &pCalibData->modules.ccm);
    VSI_MPI_ISP_SetGammaOutAttr(IspPort, &pCalibData->modules.gammaOut);
    VSI_MPI_ISP_SetCsmAttr(IspPort, &pCalibData->modules.csm);
    VSI_MPI_ISP_SetCprocAttr(IspPort, &pCalibData->modules.cproc);

    return VSI_SUCCESS;
}

/*****************************************************************************/
/**
 * @brief   Gets calibration data.
 *
 * @param   IspPort             Port ID
 * @param   pCalibData          Pointer to the calibration data

 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
static inline int VSI_MPI_ISP_GetCalib(ISP_PORT IspPort, ISP_CALIB_DATA_S *pCalibData)
{
    VSI_MPI_ISP_GetAutoRoute(IspPort, &pCalibData->modules.autoRoute);
    VSI_MPI_ISP_GetBlsAttr(IspPort, &pCalibData->modules.bls);
    VSI_MPI_ISP_GetGammaInAttr(IspPort, &pCalibData->modules.gammaIn);
    VSI_MPI_ISP_GetDgAttr(IspPort, &pCalibData->modules.dg);
    VSI_MPI_ISP_GetExpmAttr(IspPort, &pCalibData->modules.aem);
    VSI_MPI_ISP_GetHist256Attr(IspPort, &pCalibData->modules.hist256);
    VSI_MPI_ISP_GetExposureAttr(IspPort, &pCalibData->modules.ae);
    VSI_MPI_ISP_GetWbmAttr(IspPort, &pCalibData->modules.wbm);
    VSI_MPI_ISP_GetWbAttr(IspPort, &pCalibData->modules.wb);
    VSI_MPI_ISP_GetLscAttr(IspPort, &pCalibData->modules.lsc);
    VSI_MPI_ISP_GetWdrAttr(IspPort, &pCalibData->modules.wdr);
    VSI_MPI_ISP_GetGeAttr(IspPort, &pCalibData->modules.ge);
    VSI_MPI_ISP_GetDpccAttr(IspPort, &pCalibData->modules.dpcc);
    VSI_MPI_ISP_GetDpfAttr(IspPort, &pCalibData->modules.dpf);
    VSI_MPI_ISP_Get2DnrAttr(IspPort, &pCalibData->modules.nr2d);
    VSI_MPI_ISP_GetDmscAttr(IspPort, &pCalibData->modules.dmsc);
    VSI_MPI_ISP_GetFltAttr(IspPort, &pCalibData->modules.flt);
    VSI_MPI_ISP_GetCcmAttr(IspPort, &pCalibData->modules.ccm);
    VSI_MPI_ISP_GetGammaOutAttr(IspPort, &pCalibData->modules.gammaOut);
    VSI_MPI_ISP_GetCsmAttr(IspPort, &pCalibData->modules.csm);
    VSI_MPI_ISP_GetCprocAttr(IspPort, &pCalibData->modules.cproc);

    return VSI_SUCCESS;
}

/* @} mpi_isp_calib */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif