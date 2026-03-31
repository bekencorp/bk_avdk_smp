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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void bk_gpio_log_print(const char *fmt, ...);

#if CONFIG_BK_TRACE_PRINT_ENABLE
#define BK_TRACE_POINT() bk_gpio_log_print("%s:%d\r\n", __func__, __LINE__)
#else
#define BK_TRACE_POINT()
#endif


#ifdef __cplusplus
}
#endif