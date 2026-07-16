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

#include <stdint.h>

#include <os/os.h>
#include "components/bk_decode/bk_pp_ctlr.h"
#include "modules/vcdec/vcdec_pp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	vcdec_pp_handle vcdec_pp_handle; /* underlying vcdec standalone PP instance */
	uint8_t         hw_registered;   /* decoder/PP power domain + IRQ held */

	vcdec_pp_process_req_t process_req; /* staging for hw_decoder task callback */
	avdk_err_t             process_result;
	beken_semaphore_t      process_done_sem;

	bk_pp_config_t  config;
	bk_pp_ctlr_t    ops;
} private_pp_ctlr_t;

#ifdef __cplusplus
}
#endif
