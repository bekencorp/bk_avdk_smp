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
#include <common/bk_include.h>
#include <driver/trng.h>

/*
 * BK7259 has no standalone hardware TRNG peripheral. Random numbers are
 * provided by the TE200 (TrustEngine) via bk_rng_get() implemented in
 * psa_mbedtls (platform.c). The legacy peripheral register/LL/HAL code has
 * been removed; the driver API below is kept as thin stubs for backward
 * compatibility (CONFIG_TRNG_SUPPORT).
 */

static bool s_trng_driver_is_init = false;

#define TRNG_RETURN_ON_NOT_INIT() do {\
		if (!s_trng_driver_is_init) {\
			return BK_ERR_TRNG_DRIVER_NOT_INIT;\
		}\
	} while(0)

bk_err_t bk_trng_driver_init(void)
{
	s_trng_driver_is_init = true;
	return BK_OK;
}

bk_err_t bk_trng_driver_deinit(void)
{
	s_trng_driver_is_init = false;
	return BK_OK;
}

bk_err_t bk_trng_start(void)
{
	TRNG_RETURN_ON_NOT_INIT();
	return BK_OK;
}

bk_err_t bk_trng_stop(void)
{
	TRNG_RETURN_ON_NOT_INIT();
	return BK_OK;
}

#if CONFIG_PSA_MBEDTLS
/* Real entropy source: TE200, implemented in psa_mbedtls platform.c. */
extern int bk_rng_get(unsigned char *output, size_t len);

int bk_rand(void)
{
	int number = 0;

	bk_rng_get((unsigned char *)&number, sizeof(number));

	return (number & RAND_MAX);
}

int bk_fill_rand(void *buff, size_t len)
{
	if (buff == NULL || len == 0) {
		return -1;
	}

	return bk_rng_get((unsigned char *)buff, len);
}
#else
int bk_rand(void)
{
	return (rand() & RAND_MAX);
}

int bk_fill_rand(void *buff, size_t len)
{
	uint8_t *p = (uint8_t *)buff;

	if (buff == NULL || len == 0) {
		return -1;
	}

	for (size_t i = 0; i < len; i++) {
		p[i] = (rand() & 0xff);
	}

	return 0;
}
#endif
