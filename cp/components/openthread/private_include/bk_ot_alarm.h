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

#ifndef __BK_OT_ALARM_H__
#define __BK_oT_ALARM_H__

#include "openthread/error.h"
#include "sdkconfig.h"
#include "components/system.h"
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#ifdef __cplusplus
extern "C" {
#endif
void bk_ot_alarm_init(void);
void bk_ot_alarm_deinit(void);
bk_err_t bk_ot_alarm_process(otInstance *aInstance);



#ifdef __cplusplus
}
#endif
#endif