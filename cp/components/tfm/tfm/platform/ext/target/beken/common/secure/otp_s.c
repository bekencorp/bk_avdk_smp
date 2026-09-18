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

/* Secure-world OTP driver for TF-M.
 *
 * Item-based OTP1/OTP2 read/update/permission using the generated OTP map
 * (otp_map_1/otp_map_2 from otp1.csv/otp2.csv). OTP access uses the inline
 * otp_ll (pdstb + clkosc, no PM voting), matching otp_min.c; each call
 * activates the block, performs the access, then powers it down. Runs in the
 * SPE so it can reach OTP regions the Non-Secure world cannot. The OTP NSC
 * gateways (otp_nsc.c) forward to these functions. */

#include <common/bk_include.h>
#include "otp_s.h"
#include "otp_ll.h"
/* Embed the project-generated OTP map (otp_map_1/otp_map_2 with offset/size/
 * security from otp1.csv/otp2.csv). Resolved at compile time from the partitions
 * _build dir (added to the include path in common/secure/CMakeLists.txt),
 * matching the SDK otp_driver pattern. Defines the map symbols used here and,
 * via extern, by bk_tfm_mpc.c. */
#include "_otp.c"

static otp_hw_t  *const s_otp_hw  = (otp_hw_t  *)OTP_LL_REG_BASE(0);
static otp2_hw_t *const s_otp2_hw = (otp2_hw_t *)OTP2_LL_REG_BASE(0);

int otp_active(void)
{
	return otp_ll_active(s_otp_hw);
}

void otp_sleep(void)
{
	otp_ll_sleep(s_otp_hw);
}

static const otp_item_t *otp_item(uint8_t map_id, uint32_t item)
{
	return (map_id == OTP_MAP_AHB) ? &otp_map_2[item] : &otp_map_1[item];
}

static uint32_t otp_max_id(uint8_t map_id)
{
	return (map_id == OTP_MAP_AHB) ? OTP2_MAX_ID : OTP1_MAX_ID;
}

static uint32_t otp_read_word(uint8_t map_id, uint32_t location)
{
	return (map_id == OTP_MAP_AHB) ? otp2_ll_read_otp(s_otp2_hw, location)
	                               : otp_ll_read_otp(s_otp_hw, location);
}

static void otp_write_word(uint8_t map_id, uint32_t location, uint32_t value)
{
	if (map_id == OTP_MAP_AHB) {
		otp2_ll_write_otp(s_otp2_hw, location, value);
	} else {
		otp_ll_write_otp(s_otp_hw, location, value);
	}
}

/* Effective privilege = max(permission, mask): the stricter of the two wins. */
static uint32_t otp_perm_word(uint8_t map_id, uint32_t location)
{
	uint32_t perm, mask;

	if (map_id == OTP_MAP_AHB) {
		perm = otp_ll_read_otp2_permission(s_otp_hw, location);
		mask = otp_ll_read_otp2_mask(s_otp_hw, location);
	} else {
		perm = otp_ll_read_otp_permission(s_otp_hw, location);
		mask = otp_ll_read_otp_mask(s_otp_hw, location);
	}
	return (perm > mask) ? perm : mask;
}

static bool otp_item_valid(uint8_t map_id, uint32_t item)
{
	if (map_id != OTP_MAP_APB && map_id != OTP_MAP_AHB) {
		return false;
	}
	return (item < otp_max_id(map_id));
}

otp_privilege_t otp_read_permission(uint8_t map_id, uint32_t item)
{
	otp_privilege_t priv;

	if (!otp_item_valid(map_id, item)) {
		return OTP_NO_ACCESS;
	}
	if (otp_active() != 0) {
		otp_sleep();
		return OTP_NO_ACCESS;
	}
	priv = (otp_privilege_t)otp_perm_word(map_id, otp_item(map_id, item)->offset / 4);
	otp_sleep();
	return priv;
}

bk_err_t otp_read(uint8_t map_id, uint32_t item, uint8_t *buf, uint32_t size)
{
	const otp_item_t *it;
	uint32_t location, start;

	if (!otp_item_valid(map_id, item)) {
		return BK_ERR_OTP_INDEX_WRONG;
	}
	if (buf == NULL) {
		return BK_ERR_OTP_READ_BUFFER_NULL;
	}
	if (otp_read_permission(map_id, item) > OTP_READ_ONLY) {
		return BK_ERR_NO_READ_PERMISSION;
	}

	it = otp_item(map_id, item);
	OTP_ACTIVE();
	if (size > it->allocated_size) {
		otp_sleep();
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}

	location = it->offset / 4;
	start = it->offset % 4;
	while (size > 0) {
		uint32_t value = otp_read_word(map_id, location);
		uint8_t *src = (uint8_t *)&value + start;
		uint32_t cpy = (size >= (4 - start)) ? (4 - start) : size;

		for (uint32_t i = 0; i < cpy; i++) {
			*buf++ = *src++;
		}
		size -= cpy;
		location++;
		start = 0;
	}
	otp_sleep();
	return BK_OK;
}

bk_err_t otp_update(uint8_t map_id, uint32_t item, const uint8_t *buf, uint32_t size)
{
	const otp_item_t *it;
	uint32_t location, start, remain;
	const uint8_t *p;

	if (!otp_item_valid(map_id, item)) {
		return BK_ERR_OTP_INDEX_WRONG;
	}
	if (buf == NULL) {
		return BK_ERR_OTP_READ_BUFFER_NULL;
	}
	if (otp_read_permission(map_id, item) != OTP_READ_WRITE) {
		return BK_ERR_NO_WRITE_PERMISSION;
	}

	it = otp_item(map_id, item);
	OTP_ACTIVE();
	if (size > it->allocated_size) {
		otp_sleep();
		return BK_ERR_OTP_ADDR_OUT_OF_RANGE;
	}

	/* OTP bits can only flip 0->1; reject any 1->0 request before writing. */
	location = it->offset / 4;
	start = it->offset % 4;
	remain = size;
	p = buf;
	while (remain > 0) {
		uint32_t value = otp_read_word(map_id, location);
		uint8_t *cur = (uint8_t *)&value + start;
		uint32_t cnt = (remain >= (4 - start)) ? (4 - start) : remain;

		for (uint32_t i = 0; i < cnt; i++) {
			if ((uint8_t)(~(*p) & (*cur)) != 0) {
				otp_sleep();
				return BK_ERR_OTP_UPDATE_NOT_EQUAL;
			}
			p++;
			cur++;
		}
		remain -= cnt;
		location++;
		start = 0;
	}

	location = it->offset / 4;
	start = it->offset % 4;
	remain = size;
	p = buf;
	while (remain > 0) {
		uint32_t value = 0;
		uint8_t *dst = (uint8_t *)&value + start;
		uint32_t cnt = (remain >= (4 - start)) ? (4 - start) : remain;

		for (uint32_t i = 0; i < cnt; i++) {
			*dst++ = *p++;
		}
		otp_write_word(map_id, location, value);
		if (otp_read_word(map_id, location) != value) {
			otp_sleep();
			return BK_ERR_OTP_UPDATE_NOT_EQUAL;
		}
		remain -= cnt;
		location++;
		start = 0;
	}
	otp_sleep();
	return BK_OK;
}

bk_err_t otp_write_permission(uint8_t map_id, uint32_t item, otp_privilege_t permission)
{
	const otp_item_t *it;
	uint32_t location;
	int32_t size;

	if (!otp_item_valid(map_id, item)) {
		return BK_ERR_OTP_INDEX_WRONG;
	}
	/* OTP2 permission supports read-only; OTP1 supports read-only / no-access. */
	if (map_id == OTP_MAP_AHB) {
		if (permission != OTP_READ_ONLY) {
			return BK_ERR_OTP_PERMISSION_WRONG;
		}
	} else if (permission != OTP_READ_ONLY && permission != OTP_NO_ACCESS) {
		return BK_ERR_OTP_PERMISSION_WRONG;
	}

	it = otp_item(map_id, item);
	OTP_ACTIVE();
	location = it->offset / 4;
	size = it->allocated_size;
	while (size > 0) {
		if (map_id == OTP_MAP_AHB) {
			otp_ll_write_otp2_permission(s_otp_hw, location, permission);
		} else {
			otp_ll_write_otp_permission(s_otp_hw, location, permission);
		}
		location++;
		size -= 4;
	}
	otp_sleep();
	return BK_OK;
}

bk_err_t otp_write_mask(uint8_t map_id, uint32_t item, otp_privilege_t permission)
{
	const otp_item_t *it;
	uint32_t location;
	int32_t size;

	if (!otp_item_valid(map_id, item)) {
		return BK_ERR_OTP_INDEX_WRONG;
	}

	it = otp_item(map_id, item);
	OTP_ACTIVE();
	location = it->offset / 4;
	size = it->allocated_size;
	while (size > 0) {
		if (map_id == OTP_MAP_AHB) {
			otp_ll_write_otp2_mask(s_otp_hw, location, permission);
		} else {
			otp_ll_write_otp_mask(s_otp_hw, location, permission);
		}
		location++;
		size -= 4;
	}
	otp_sleep();
	return BK_OK;
}

/* Mark the whole OTP1 range (0x0-0x400) as a secure range so only the secure
 * world may read it, then enable the secure protection. Secure-range
 * granularity is 64 words (256 bytes); groups 0..3 cover the entire 0x400 bank.
 * WARNING: enabling the protection is a permanent, irreversible OTP setting. */
bk_err_t otp_secure_range_enable(void)
{
	/* If the secure protection is already enabled, the range was configured on a
	 * previous boot; skip to avoid re-asserting it (a repeated setup can hang). */
	if (otp_ll_read_security_protection(s_otp_hw) != 0) {
		return BK_OK;
	}

	for (uint32_t group = 0; group < 4; group++) {
		otp_ll_write_otp_security(s_otp_hw, group * 64);
	}
	otp_ll_enable_security_protection(s_otp_hw);

	return BK_OK;
}
