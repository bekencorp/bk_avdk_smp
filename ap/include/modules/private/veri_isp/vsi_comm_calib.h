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

#ifndef __ISP_COMM_CALIB_H__
#define __ISP_COMM_CALIB_H__

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

#include "mpi_isp.h"
#include "mpi_isp_bls.h"
#include "mpi_isp_gamma_in.h"
#include "mpi_isp_lsc.h"
#include "mpi_isp_dg.h"
#include "mpi_isp_expm.h"
#include "mpi_isp_hist256.h"
#include "mpi_isp_ae.h"
#include "mpi_isp_wbm.h"
#include "mpi_isp_wb.h"
#include "mpi_isp_wdr.h"
#include "mpi_isp_ge.h"
#include "mpi_isp_dpcc.h"
#include "mpi_isp_dpf.h"
#include "mpi_isp_2dnr.h"
#include "mpi_isp_dmsc.h"
#include "mpi_isp_flt.h"
#include "mpi_isp_ccm.h"
#include "mpi_isp_gamma_out.h"
#include "mpi_isp_csm.h"
#include "mpi_isp_cproc.h"


/** \brief   Calibration data head. */
typedef struct vsiISP_CALIB_HEAD_S {
    char name[64];    /**< \brief The file name of calibration data */
    char date[64];    /**< \brief The generating date of calibration data */
    char version[64]; /**< \brief The project version of calibration data */
} ISP_CALIB_HEAD_S;

/** \brief   Calibration module. */
typedef struct vsiISP_CALIB_MODULE_S {
    ISP_AUTO_ROUTE_S autoRoute;   /**< \brief Auto route. */
    ISP_BLS_ATTR_S  bls;          /**< \brief BLS. */
    ISP_GAMMA_IN_ATTR_S gammaIn;  /**< \brief Gamma in. */
    ISP_LSC_ATTR_S lsc;           /**< \brief LSC. */
    ISP_DG_ATTR_S dg;             /**< \brief DG. */
    ISP_EXPM_ATTR_S aem;          /**< \brief AE measurement. */
    ISP_HIST256_ATTR_S hist256;   /**< \brief HIST256. */
    ISP_EXPOSURE_ATTR_S ae;       /**< \brief AE. */
    ISP_WBM_ATTR_S wbm;           /**< \brief WB measurement. */
    ISP_WB_ATTR_S wb;             /**< \brief WB. */
    ISP_WDR_ATTR_S wdr;           /**< \brief WDR. */
    ISP_GE_ATTR_S ge;             /**< \brief GE. */
    ISP_DPCC_ATTR_S dpcc;         /**< \brief DPCC. */
    ISP_DPF_ATTR_S dpf;           /**< \brief DPF. */
    ISP_2DNR_ATTR_S nr2d;         /**< \brief 2DNR. */
    ISP_DMSC_ATTR_S dmsc;         /**< \brief Demosaic. */
    ISP_FLT_ATTR_S flt;           /**< \brief Filter. */
    ISP_CCM_ATTR_S ccm;           /**< \brief CCM. */
    ISP_GAMMA_OUT_ATTR_S gammaOut;/**< \brief Gamma out. */
    ISP_CSM_ATTR_S csm;           /**< \brief CSM. */
    ISP_CPROC_ATTR_S cproc;       /**< \brief CPROC. */
} ISP_CALIB_MODULE_S;

/** \brief   Calibration data. */
typedef struct vsiISP_CALIB_DATA_S {
    ISP_CALIB_HEAD_S head;       /**< \brief Calibration data head. */
    ISP_CALIB_MODULE_S modules;  /**< \brief Calibration data module. */
} ISP_CALIB_DATA_S;

/* @} mpi_isp_calib */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif