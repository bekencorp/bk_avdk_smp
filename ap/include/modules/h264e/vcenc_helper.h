/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2023 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Preprocessor setup
--
------------------------------------------------------------------------------*/
#ifndef __VCENC_HELPER_H__
#define __VCENC_HELPER_H__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "base_type.h"
#include "hevcencapi.h"
/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
/**
 * \defgroup api_helper Helper API
 *
 * Helper API will provide some function to help users to implement
 * features in application. currently, the helpers include,
 *  - levelIdx, maxCPBS and maxBR calculation for different codecFormat;
 *
 * @{
 */

/** Get level by levelIdx according to current codec format.
 *
 * \param [in] codecFormat The codec standard defined in API.
 * \param [in] levelIdx level 5.1 =8
 * \return the level of current codec format
 */
i32 EncGetLevelIdx(VCEncVideoCodecFormat codecFormat, VCEncLevel levelIdx);

/** Calculate maxCPBS choosed by current codecFormat and level, and multiply
 * cpbFactor choosed by current codecFormat, profile and nalHrdParaFlag
 *
 * \param [in] codecFormat The codec standard defined in API.
 * \param [in] levelIdx level 5.1 =8
 * \param [in] profile current codec profile
 * \param [in] tier current codec tier
 * \param [in] nalHrdParaFlag NAL HRD parameter
 * \return maxCPBS calculated by cpbFactor * (maxCPBS of level).
 */
u32 EncGetMaxCPBS(VCEncVideoCodecFormat codecFormat, i32 levelIdx, i32 profile, i32 tier, true_e nalHrdParaFlag);

/** Calculate maxBR choosed by current codecFormat and level, and multiply
 * brFactor choosed by current codecFormat, profile and nalHrdParaFlag
 *
 * \param [in] codecFormat The codec standard defined in API.
 * \param [in] levelIdx level 5.1 =8
 * \param [in] profile current codec profile
 * \param [in] tier current codec tier
 * \param [in] nalHrdParaFlag NAL HRD parameter
 * \return maxBR calculated by brFactor * (maxBR of level).
 */
u32 EncGetMaxBR(VCEncVideoCodecFormat codecFormat, i32 levelIdx, i32 profile, i32 tier, true_e nalHrdParaFlag);
/** @} */



/** Copy GOP info
 *
 * \param [in] inst The encoder instance to encode the frame.
 * \param [out] p A pointer to the storage space for receiving GOP Infos.
 * \return None
 */
void CopyAdaptiveGOPInfo2UsrMem(const void *inst, u32 *p);

/** Get pass1 updated GopSize
 *
 * \param [in] inst The encoder instance to encode the frame.
 * \return pass1 updated GopSize
 */
i32 VCEncGetPass1UpdatedGopSize(const void *inst);

/** Get reservedCore number
 *
 * \param [in] inst The encoder instance to encode the frame.
 * \return reservedCore value
 */
u32 GetReservedCore(const void *inst);

#ifdef __cplusplus
}
#endif

#endif  //__BUFFER_INFO_H__
