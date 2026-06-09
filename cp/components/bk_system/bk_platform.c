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
#include <os/os.h>
#include <os/mem.h>

#if CONFIG_TRUSTENGINE && (!CONFIG_TRNG_SUPPORT)
void arm_ce_trng_driver_init( void );
int arm_ce_seed_read( unsigned char *buf, size_t buf_len );
void arm_ce_init(void)
{
	arm_ce_trng_driver_init();
}

bool random_is_initial = false;

int bk_rand(void)
{
	if (!random_is_initial)
	{
		arm_ce_init();
		random_is_initial = true;
	}

	int number, ret;

	ret = arm_ce_seed_read((unsigned char *)&number, sizeof(number));
	if(0 != ret){
		number = 0;
	}

	return (number & RAND_MAX);
}
#elif !CONFIG_TRNG_SUPPORT
int bk_rand(void)
{
	return rand();
}
#endif
