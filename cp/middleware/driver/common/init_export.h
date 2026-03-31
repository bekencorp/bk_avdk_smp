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

#pragma once

#include "periph_id.h"

#define fn_section(x)               __attribute__((section(x)))
#define peri_init_section(x)        __attribute__((section(x)))
#define fn_used                     __attribute__((used))
typedef int (*init_fn_t)(void);

struct _init_desc
{
#if CONFIG_INIT_EXPORT_WITH_NAME
    const char* peri_str;
#endif
    int peri_id;
    const init_fn_t fn;
};

#if CONFIG_INIT_EXPORT_WITH_NAME
    #define DEV_INIT_EXPORT(dev_id, fn, level)\
        const char __exp_##fn##_name[] = #fn;                                            \
        fn_used const struct _init_desc __init_desc_##fn fn_section(".dev_init_fn." level) = \
        { __exp_##fn##_name, dev_id, fn};
    #define COMP_INIT_EXPORT(dev_id, fn, level)\
        const char __exp_##fn##_name[] = #fn;                                            \
        fn_used const struct _init_desc __init_desc_##fn fn_section(".comp_init_fn." level) = \
        { __exp_##fn##_name, dev_id, fn};
    #define APP_INIT_EXPORT(dev_id, fn, level)\
        const char __exp_##fn##_name[] = #fn;                                            \
        fn_used const struct _init_desc __init_desc_##fn fn_section(".app_init_fn." level) = \
        { __exp_##fn##_name, dev_id, fn};
#else
    #define DEV_INIT_EXPORT(dev_id, fn, level)\
        fn_used const struct _init_desc __init_desc_##fn fn_section(".dev_init_fn." level) = \
        { dev_id, fn};
    #define COMP_INIT_EXPORT(dev_id, fn, level)\
        fn_used const struct _init_desc __init_desc_##fn fn_section(".comp_init_fn." level) = \
        { dev_id, fn};
    #define APP_INIT_EXPORT(dev_id, fn, level)\
        fn_used const struct _init_desc __init_desc_##fn fn_section(".app_init_fn." level) = \
        { dev_id, fn};
#endif /* CONFIG_INIT_EXPORT_WITH_NAME */

/* pre/device/component/env/app init routines will be called in init_thread */
/* components pre-initialization (pure software initialization) */
#define INIT_PREV_EXPORT(id, fn)            DEV_INIT_EXPORT(id, fn, "3")

/* device initialization */
#define INIT_DEV_PRIO0_EXPORT(id, fn)          DEV_INIT_EXPORT(id, fn, "3.0")
#define INIT_DEV_PRIO1_EXPORT(id, fn)          DEV_INIT_EXPORT(id, fn, "3.1")
#define INIT_DEV_PRIO2_EXPORT(id, fn)          DEV_INIT_EXPORT(id, fn, "3.2")

#define INIT_COMP_PRIO0_EXPORT(id, fn)          COMP_INIT_EXPORT(id, fn, "4.0")
#define INIT_COMP_PRIO1_EXPORT(id, fn)          COMP_INIT_EXPORT(id, fn, "4.1")
#define INIT_COMP_PRIO2_EXPORT(id, fn)          COMP_INIT_EXPORT(id, fn, "4.2")

#define INIT_APP_PRIO0_EXPORT(id, fn)          APP_INIT_EXPORT(id, fn, "5.0")
#define INIT_APP_PRIO1_EXPORT(id, fn)          APP_INIT_EXPORT(id, fn, "5.1")
#define INIT_APP_PRIO2_EXPORT(id, fn)          APP_INIT_EXPORT(id, fn, "5.2")
//eof
