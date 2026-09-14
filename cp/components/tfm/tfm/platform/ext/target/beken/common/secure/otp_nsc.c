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

/* otp_nsc: ARMv8-M CMSE Non-Secure-Callable OTP access for the CP core.
 *
 * TF-M runs as the Secure world (SPE); the cpu0_app runs Non-Secure (NSPE).
 * OTP banks the Secure world owns (the OTP1 key/efuse bank, or any OTP2 region
 * marked secure by the MPC) read back as zero from the NS alias. These gateways
 * let the NS side reach them through the secure world.
 *
 * The gateway is item-based: the NS side passes the map id (1 = OTP1, 2 = OTP2)
 * and the item index (otp1_id_t / otp2_id_t); the secure driver (otp_s.c)
 * resolves the offset/size/permission from the generated OTP map. __tz_c_veneer
 * emits an SG stub into .gnu.sgstubs exported through libtfm_s_veneers.a. */

#include <common/bk_include.h>
#include "security_defs.h"
#include "otp_s.h"

/* Link anchor: referenced from tfm_hal_get_ns_entry_point() under CONFIG_OTP_NSC
 * so --gc-sections keeps this TU (and its gateway veneers) in tfm_s. */
void psa_otp_nsc_stub(void)
{
	return;
}

__tz_c_veneer bk_err_t bk_otp_read_nsc(uint8_t map_id, uint32_t item, uint8_t *buf, uint32_t size)
{
	return otp_read(map_id, item, buf, size);
}

__tz_c_veneer bk_err_t bk_otp_update_nsc(uint8_t map_id, uint32_t item, const uint8_t *buf, uint32_t size)
{
	return otp_update(map_id, item, buf, size);
}

__tz_c_veneer bk_err_t bk_otp_read_permission_nsc(uint8_t map_id, uint32_t item, uint32_t *permission)
{
	if (permission == NULL) {
		return BK_ERR_OTP_READ_BUFFER_NULL;
	}
	*permission = (uint32_t)otp_read_permission(map_id, item);
	return BK_OK;
}

__tz_c_veneer bk_err_t bk_otp_write_permission_nsc(uint8_t map_id, uint32_t item, uint32_t permission)
{
	return otp_write_permission(map_id, item, (otp_privilege_t)permission);
}

__tz_c_veneer bk_err_t bk_otp_write_mask_nsc(uint8_t map_id, uint32_t item, uint32_t permission)
{
	return otp_write_mask(map_id, item, (otp_privilege_t)permission);
}
