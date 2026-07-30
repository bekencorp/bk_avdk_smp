// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: minimal OTP random-number read for SCA.
//
// bk_sca_defense.c uses bk_otp_read_random_number() to seed side-channel random
// delays. The full SDK otp_driver_v1_1.c pulls in PM voting, the otp map tables
// and the trustengine path. This minimal version activates the OTP block via
// the pure inline otp_ll.h (pdstb + clkosc, no PM module dependency), reads the
// hardware random_value register, then powers the block back down.

#include <stdint.h>
#include <common/bk_include.h>
#include "otp_ll.h"

static otp_hw_t *const s_otp_hw = (otp_hw_t *)OTP_LL_REG_BASE(0);

bk_err_t bk_otp_read_random_number(uint32_t *value, uint32_t size)
{
	if (!value) {
		return BK_ERR_NULL_PARAM;
	}

	if (otp_ll_active(s_otp_hw) != 0) {
		return BK_FAIL;
	}

	for (uint32_t i = 0; i < size; i++) {
		value[i] = otp_ll_read_random_number(s_otp_hw);
	}

	otp_ll_sleep(s_otp_hw);
	return BK_OK;
}
