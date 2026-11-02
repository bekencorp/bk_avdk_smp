// Copyright 2020-2021 Beken
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

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <common/bk_include.h>
#include <components/bk_platform.h>
#if CONFIG_OTP_V1
#include <common/bk_err.h>
/* Declared in driver/otp.h, which pulls in the build-generated _otp.h. */
extern bk_err_t bk_otp_read_random_number(uint32_t *value, uint32_t size);
#endif

/*
 * Real libc rand(), exposed by the linker option -Wl,--wrap=rand.
 * Used only by the software fallback paths below so that they never
 * recurse back into __wrap_rand() -> bk_rand().
 */
extern int __real_rand(void);

#if CONFIG_OTP_V1
/*
 * Hardware entropy for the weak default path: the OTP random number register.
 * This keeps bk_rand()/bk_fill_rand() backed by a hardware source even when
 * the SDK PSA stack (and its TE200 strong overrides) is not built, e.g. when
 * a customer brings its own PSA implementation.
 */
static int bk_rand_otp_fill(uint8_t *p, size_t len)
{
	size_t i = 0;

	while (i < len) {
		uint32_t number = 0;
		size_t n = 0;

		if (bk_otp_read_random_number(&number, 1) != BK_OK) {
			break;
		}

		while (n < sizeof(number) && i < len) {
			p[i++] = (number >> (8 * n)) & 0xff;
			n++;
		}
	}

	return (i == len) ? 0 : -1;
}
#endif

/*
 * Default RNG. Overridden by strong symbols in psa_mbedtls platform.c when
 * CONFIG_PSA_MBEDTLS is enabled (TE200 via bk_rng_get).
 * Hardware entropy is used when available (OTP with CONFIG_OTP_V1); libc
 * rand() is only the last-resort fallback.
 */
__attribute__((weak)) int bk_rand(void)
{
#if CONFIG_OTP_V1
	uint32_t number = 0;

	if (bk_otp_read_random_number(&number, 1) == BK_OK) {
		return (number & RAND_MAX);
	}
#endif

	return (__real_rand() & RAND_MAX);
}

__attribute__((weak)) int bk_fill_rand(void *buff, size_t len)
{
	uint8_t *p = (uint8_t *)buff;

	if (buff == NULL || len == 0) {
		return -1;
	}

#if CONFIG_OTP_V1
	if (bk_rand_otp_fill(p, len) == 0) {
		return 0;
	}
#endif

	for (size_t i = 0; i < len; i++) {
		p[i] = (__real_rand() & 0xff);
	}

	return 0;
}

/*
 * Wrapper installed via -Wl,--wrap=rand: every rand() reference in the
 * whole image (Wi-Fi supplicant, mbedtls entropy poll, prebuilt BT host
 * library, ...) is redirected here and served by the secure RNG so that
 * pairing keys / nonces / session keys are no longer predictable.
 */
int __wrap_rand(void)
{
	return bk_rand();
}
