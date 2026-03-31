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

/* interrupt controller enumeration definition */
enum {
        NVIC_0 = 0,
        NVIC_1 = 1,
};

/* get corresponding section name based on interrupt controller */
#define INT_SECTION(nvic) \
    __attribute__((section(".int_list" #nvic)))

/* weak definition macro for generating default weak objects */
#define WEAK_OBJECT(nvic, n) \
    __attribute__((weak, section(".int_list" #nvic))) \
    isr_item_t object##nvic##_##n = {NULL};

/* used when users register interrupt service routines */
#define IRQ_CONNECT(nvic, n, func_val) \
    __attribute__((section(".int_list" #nvic))) \
    isr_item_t object##nvic##_##n = {func_val};

/* helper function for recursive macros to concatenate two identifiers */
#define CAT(a, b) CAT_HELPER(a, b)
#define CAT_HELPER(a, b) a##b

/* termination condition for recursive macro */
#define _REGISTRY_0(nvic, n, max) _STOP_REGISTRY_

/* continuation condition for recursive macro */
#define _REGISTRY_1(nvic, n, max) \
    WEAK_OBJECT(nvic, n); \
    CAT(_REGISTRY_, (n + 1 < max))(nvic, n + 1, max)

/* macro to check if registration should continue */
#define _CHECK_REGISTRY(nvic, n, max) \
    CAT(_REGISTRY_, (n < max))(nvic, n, max)

#define _STOP_REGISTRY_
#define _CONTINUE_REGISTRY(nvic, n, max) REGISTRY_LOOP(nvic, n, max)

/* loop macro for generating object registry */
#define REGISTRY_LOOP(nvic, start, end) _CHECK_REGISTRY(nvic, start, end)