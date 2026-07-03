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

#ifndef __BK_OPENTHREAD_COEX_H__
#define __BK_OPENTHREAD_COEX_H__


#ifdef __cplusplus
extern "C" {
#endif

bool bk_ot_coex_is_attaching_process(void);
void bk_ot_rf_counter_preempted(void);

bool bk_ot_coex_is_preempted(void);
bool bk_ot_update_rf_lvl(void);
bool bk_ot_rf_is_thread(void);
uint8_t bk_ot_get_thread_task_type(void);
uint8_t bk_ot_set_thread_task_type(uint8_t tskType);
uint8_t bk_ot_update_rf_priority(uint8_t uPrio);
uint8_t bk_ot_get_rf_priority(void);
void bk_ieee802154_frame_protect_handle_msg(uint8_t action, uint32_t dur_ms);
void bk_ieee802154_thread_rf_free_impl(uint8_t rf_prio, uint8_t task_type, bool adjust, const char *caller);
#define bk_ieee802154_thread_rf_free(rf_prio, task_type, adjust) \
    bk_ieee802154_thread_rf_free_impl((rf_prio), (task_type), (adjust), __func__)


#ifdef __cplusplus
}
#endif

#endif