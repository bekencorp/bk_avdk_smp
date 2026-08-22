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

/*
 * Real libc rand(), exposed by the linker option -Wl,--wrap=rand.
 * Used only by the software fallback paths below so that they never
 * recurse back into __wrap_rand() -> bk_rand().
 */
extern int __real_rand(void);

/*
 * Default software RNG. Overridden by strong symbols in psa_mbedtls
 * platform.c when CONFIG_PSA_MBEDTLS is enabled (TE200 via bk_rng_get).
 */
__attribute__((weak)) int bk_rand(void)
{
	return (__real_rand() & RAND_MAX);
}

__attribute__((weak)) int bk_fill_rand(void *buff, size_t len)
{
	uint8_t *p = (uint8_t *)buff;

	if (buff == NULL || len == 0) {
		return -1;
	}

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
