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

#ifndef __ISP_COMM_METADATA_H__
#define __ISP_COMM_METADATA_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @defgroup mpi_isp_metadata Mpp Metadata Definitions
 * @{
 *
 */

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


/** \brief   The ISP version of MetaData. */
#define ISP_VERSION_META "ISPNANO_V2401"

/** \brief   MetaData. */
typedef struct vsiISP_METADATA_S {
    char ispVersion[32];            /**< \brief The ISP version. */
    ISP_EXPOSURE_META_S  ae;        /**< \brief AE. */
    ISP_BLS_META_S       bls;       /**< \brief BLS. */
    ISP_GAMMAIN_META_S   gammaIn;   /**< \brief Gamma in. */
    ISP_LSC_META_S       lsc;       /**< \brief LSC. */
    ISP_DG_META_S        dg;        /**< \brief DG. */
    ISP_EXPM_META_S      aem;       /**< \brief AE measurement. */
    ISP_HIST256_META_S   hist256;   /**< \brief HIST256. */
    ISP_WBM_META_S       wbm;       /**< \brief WB measurement. */
    ISP_AWB_META_S       wb;        /**< \brief AWB. */
    ISP_WDR_META_S       wdr;       /**< \brief WDR. */
    ISP_GE_META_S        ge;        /**< \brief Ge. */
    ISP_DPCC_META_S      dpcc;      /**< \brief Dpcc. */
    ISP_DPF_META_S       dpf;       /**< \brief Dpf. */
    ISP_2DNR_META_S      nr2d;      /**< \brief 2DNR. */
    ISP_DMSC_META_S      dmsc;      /**< \brief Demosaic. */
    ISP_FLT_META_S       flt;       /**< \brief Filter. */
    ISP_CCM_META_S       ccm;       /**< \brief CCM. */
    ISP_GAMMA_OUT_META_S gammaOut;  /**< \brief Gamma out. */
    ISP_CSM_META_S       csm;       /**< \brief CSM. */
    ISP_CPROC_META_S     cproc;     /**< \brief CPROC. */
} ISP_METADATA_S;

/* @} mpi_isp_metadata */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
