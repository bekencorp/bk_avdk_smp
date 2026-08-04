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
 * @defgroup mpi_isp_metadata Metadata Definitions
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
#include "mpi_isp_gamma_out_v20.h"
#include "mpi_isp_csm.h"
#include "mpi_isp_cproc.h"
#include "mpi_isp_afm.h"
#include "mpi_isp_af.h"


/** \brief Contains the metadata of an ISP device port. */
typedef struct vsiISP_METADATA_S {
    char ispVersion[32];            /**< \brief The ISP version. */
    ISP_EXPOSURE_META_S  ae;        /**< \brief The metadata of exposure. */
    ISP_BLS_META_S       bls;       /**< \brief The metadata of black level subtraction (BLS). */
    ISP_GAMMAIN_META_S   gammaIn;   /**< \brief The metadata of gamma in. */
    ISP_LSC_META_S       lsc;       /**< \brief The metadata of lens shade correction (LSC). */
    ISP_DG_META_S        dg;        /**< \brief The metadata of digital gain (DG). */
    ISP_EXPM_META_S      aem;       /**< \brief The metadata of exposure measurement (EXPM). */
    ISP_HIST256_META_S   hist256;   /**< \brief The metadata of 256-bin histogram (HIST256). */
    ISP_WBM_META_S       wbm;       /**< \brief The metadata of white balance measurement (WBM). */
    ISP_AWB_META_S       wb;        /**< \brief The metadata of white balance (WB). */
    ISP_WDR_META_S       wdr;       /**< \brief The metadata of wide dynamic range (WDR). */
    ISP_GE_META_S        ge;        /**< \brief The metadata of green equilibrium (GE). */
    ISP_DPCC_META_S      dpcc;      /**< \brief The metadata of defect pixel cluster correction (DPCC). */
    ISP_DPF_META_S       dpf;       /**< \brief The metadata of denoise pre-filter (DPF). */
    ISP_2DNR_META_S      nr2d;      /**< \brief The metadata of 2D noise reduction (2DNR). */
    ISP_DMSC_META_S      dmsc;      /**< \brief The metadata of demosaic (DMSC). */
    ISP_FLT_META_S       flt;       /**< \brief The metadata of filter (FLT). */
    ISP_CCM_META_S       ccm;       /**< \brief The metadata of color correction matrix (CCM). */
    ISP_GAMMA_OUT_V20_META_S gammaOut;  /**< \brief The metadata of gamma out. */
    ISP_CSM_META_S       csm;       /**< \brief The metadata of color space matrix (CSM). */
    ISP_CPROC_META_S     cproc;     /**< \brief The metadata of color processing (CPROC). */
    ISP_AFM_META_S       afm;       /**< \brief The metadata of auto focus measurement (AFM). */
    ISP_FOCUS_META_S     af;        /**< \brief The metadata of auto focus (AF). */
} ISP_METADATA_S;

/* @} mpi_isp_metadata */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
