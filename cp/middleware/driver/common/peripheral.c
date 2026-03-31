// Copyright 2020-2025 Beken
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

#include "soc/soc.h"
#include "init_export.h"
#include "periph_init_cfg.h"
#include <stdlib.h>
#include "os/os.h"

#ifndef CORE_CNT_IN_USE
uint32_t devs_affinity_cfg[] = {
	AFFINITY_INVULNERABILITY
};
#else
peri_init_section(".dev_affinity.") uint32_t devs_affinity_cfg[] = DEV_AFFINITY_MAP;
#endif

uint32_t periph_init_lock(void)
{
	return 0;
}

void periph_lock(void)
{
}

void periph_unlock(void)
{
}

static int bcompare(const void *a, const void *b)
{
	uint32_t a_id = ((*(uint32_t *)a) >> DEV_ID_POSI) & DEV_ID_MASK;
	uint32_t b_id = ((*(uint32_t *)b) >> DEV_ID_POSI) & DEV_ID_MASK;

	return (a_id - b_id);
}

uint32_t *periph_get_cfg_info(uint32_t dev_id)
{
	#if (1 == CORE_CNT_IN_USE)
	return NULL;
	#else
	uint32_t id = dev_id;

	return bsearch((void *)&id, (void *)devs_affinity_cfg, sizeof(devs_affinity_cfg) / sizeof(devs_affinity_cfg[0]), sizeof(devs_affinity_cfg[0]), bcompare);
	#endif
}

uint32_t periph_has_init_permission(uint32_t *cfg)
{
	uint32_t ret = 1;
	uint32_t core_id, cfg_val;

	#if (1 == CORE_CNT_IN_USE)
	goto perm_exit;
	#endif

	if(NULL == cfg){
		goto perm_exit;
	}

	cfg_val = *cfg;
	if(cfg_val & IS_INITED_FLAG_MASK){
		/* the device is initialized*/
		ret = 0;
		goto perm_exit;
	}

	core_id = rtos_get_core_id();
	if((cfg_val >> IS_INITED_FLAG_BIT_CNT) & (BIT(core_id))){
		ret = 1;
	}else{
		ret = 0;
	}

perm_exit:
	return ret;
}

uint32_t periph_set_inited_flag(uint32_t *cfg)
{
	uint32_t cfg_val;

	#if (1 == CORE_CNT_IN_USE)
	return 0;
	#endif

	if(NULL != cfg){
		cfg_val = *cfg;
		cfg_val &= ~(IS_INITED_FLAG_MASK);
		cfg_val += 1;

		*cfg = cfg_val;
	}

	return 0;
}

void periph_init(void)
{
	int ret;
    init_fn_t fn_ptr;
	struct _init_desc *desc_item;
	extern unsigned char __periph_init_start;
	extern unsigned char __periph_init_end;

    for (desc_item = (struct _init_desc *)&__periph_init_start; desc_item < (struct _init_desc *)&__periph_init_end; desc_item ++)
    {
		#if CONFIG_MULTICORE_AMP
		uint32_t *cfg_ptr;
		int peri_id;

		fn_ptr = (desc_item->fn);
		peri_id = desc_item->peri_id;

		periph_lock();
		cfg_ptr = periph_get_cfg_info(peri_id);
		if(periph_has_init_permission(cfg_ptr) && fn_ptr){
			ret = (*fn_ptr)();
			if(0 == ret){
				periph_set_inited_flag(cfg_ptr);
			}
		}
		periph_unlock();
		#else
			fn_ptr = (desc_item->fn);
			ret = (*fn_ptr)();

			(void)ret;
		#endif

		BK_ASSERT(fn_ptr);
    }
}

void periph_dump(void)
{
	int peri_id;
	struct _init_desc *desc_item;
    init_fn_t fn_ptr;
	uint32_t *cfg_ptr;
	extern unsigned char __periph_init_start;
	extern unsigned char __periph_init_end;

    for (desc_item = (struct _init_desc *)&__periph_init_start; desc_item < (struct _init_desc *)&__periph_init_end; desc_item ++)
    {
		fn_ptr = desc_item->fn;
		peri_id = desc_item->peri_id;

		#if CONFIG_INIT_EXPORT_WITH_NAME
		bk_printf("[PI]:%s\r\n", desc_item->peri_str);
		#endif
		cfg_ptr = periph_get_cfg_info(peri_id);
		bk_printf("    permission:0x%x, fn_ptr:0x%x\r\n", periph_has_init_permission(cfg_ptr), fn_ptr);
    }
}

//eof
