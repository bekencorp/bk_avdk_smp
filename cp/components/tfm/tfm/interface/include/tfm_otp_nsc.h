// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/* Non-secure prototypes for the CMSE OTP gateways implemented in the secure
 * world (otp_nsc.c). The CP core runs TF-M as Secure and the cpu0_app as
 * Non-Secure; OTP banks owned by the Secure world (e.g. the OTP1 key/efuse
 * bank, or any OTP2 region marked secure) are not reachable from the Non-Secure
 * alias, so these gateways let the NS side read/write them through the secure
 * world.
 *
 * map_id : 1 = OTP1 (APB bank), 2 = OTP2 (AHB bank)
 * item   : item index (otp1_id_t / otp2_id_t) in the generated OTP map
 * The secure driver resolves offset/size/permission from the OTP map, so NS and
 * S share one item numbering. The linker resolves these to the SG-stub veneers
 * in libtfm_s_veneers.a. */

#pragma once

#include <stdint.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTP_NSC_MAP_APB   1   /**< OTP1 (APB) bank */
#define OTP_NSC_MAP_AHB   2   /**< OTP2 (AHB) bank */

bk_err_t bk_otp_read_nsc(uint8_t map_id, uint32_t item, uint8_t *buf, uint32_t size);
bk_err_t bk_otp_update_nsc(uint8_t map_id, uint32_t item, const uint8_t *buf, uint32_t size);
bk_err_t bk_otp_read_permission_nsc(uint8_t map_id, uint32_t item, uint32_t *permission);
bk_err_t bk_otp_write_permission_nsc(uint8_t map_id, uint32_t item, uint32_t permission);
bk_err_t bk_otp_write_mask_nsc(uint8_t map_id, uint32_t item, uint32_t permission);

#ifdef __cplusplus
}
#endif
