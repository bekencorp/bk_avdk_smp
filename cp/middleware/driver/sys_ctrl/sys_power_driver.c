// Copyright 2021-2025 Beken
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

#include "sys_driver.h"

bk_err_t sys_drv_power_xtal_rx_tx_anabuf_ctrl(pm_xtal_rx_tx_anabuf_state_e sleep_state)
{
	bk_err_t ret = sys_hal_xtal_rx_tx_anabuf_ctrl(sleep_state);
	return ret;
}
