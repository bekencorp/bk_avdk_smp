// Copyright 2025-2026 Beken
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

/* Shared refcounted PM_DEV_ID_AUDIO CPU frequency vote. */

#include <os/os.h>
#include <modules/pm.h>
#include <components/log.h>
#include <components/bk_audio/audio_utils/audio_pm_util.h>

#define AUD_PM_UTIL_TAG "aud_pm_util"

static volatile int s_aud_pm_cpu_freq_vote_refs = 0;

bk_err_t bk_audio_pm_vote_cpu_freq_acquire(void)
{
	bk_err_t ret = BK_OK;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	if (s_aud_pm_cpu_freq_vote_refs == 0)
	{
		ret = bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);
		if (ret != BK_OK)
		{
			BK_LOGE(AUD_PM_UTIL_TAG, "vote cpu freq failed, ret=%d\n", (int)ret);
		}
	}
	if (ret == BK_OK)
	{
		s_aud_pm_cpu_freq_vote_refs++;
	}

	GLOBAL_INT_RESTORE();
	return ret;
}

bk_err_t bk_audio_pm_vote_cpu_freq_release(void)
{
	bk_err_t ret = BK_OK;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	if (s_aud_pm_cpu_freq_vote_refs > 0)
	{
		s_aud_pm_cpu_freq_vote_refs--;
		if (s_aud_pm_cpu_freq_vote_refs == 0)
		{
			ret = bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);
			if (ret != BK_OK)
			{
				s_aud_pm_cpu_freq_vote_refs++;
				BK_LOGE(AUD_PM_UTIL_TAG, "vote cpu freq default failed, ret=%d\n", (int)ret);
			}
		}
	}

	GLOBAL_INT_RESTORE();
	return ret;
}