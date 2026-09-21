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

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <components/bk_audio/audio_utils/audio_obs_util.h>

#define AUDIO_OBS_UTIL_U32_MAX 0xffffffffu
#define AUDIO_OBS_UTIL_U16_MAX 0xffffu

static uint32_t s_audio_obs_components = 0;
static uint32_t s_audio_obs_interval_ms = AUDIO_OBS_INTERVAL_MS;

static void audio_obs_util_reset_cost(audio_obs_cost_stat_t *cost)
{
    if (!cost)
    {
        return;
    }

    cost->count = 0;
    cost->sum = 0;
    cost->min = AUDIO_OBS_UTIL_U32_MAX;
    cost->max = 0;
    os_memset(cost->hist, 0, sizeof(cost->hist));
}

static uint32_t audio_obs_util_cost_percentile(audio_obs_cost_stat_t *cost, uint32_t percentile)
{
    uint32_t acc = 0;
    uint32_t target;

    if (!cost || cost->count == 0)
    {
        return 0;
    }

    if (percentile > 100)
    {
        percentile = 100;
    }

    target = (cost->count * percentile + 99) / 100;
    if (target == 0)
    {
        target = 1;
    }

    for (uint32_t i = 0; i <= AUDIO_OBS_HIST_MAX_MS; i++)
    {
        acc += cost->hist[i];
        if (acc >= target)
        {
            return i;
        }
    }

    return AUDIO_OBS_HIST_MAX_MS;
}

bk_err_t audio_obs_util_init(audio_obs_util_t *obs, const char *tag, uint32_t interval_ms)
{
    if (!obs || !tag || interval_ms == 0)
    {
        return BK_FAIL;
    }

    os_memset(obs, 0, sizeof(audio_obs_util_t));

    if (os_strlen(tag) >= AUDIO_OBS_TAG_LEN)
    {
        os_memcpy(obs->tag, tag, AUDIO_OBS_TAG_LEN - 1);
        obs->tag[AUDIO_OBS_TAG_LEN - 1] = '\0';
    }
    else
    {
        os_memcpy(obs->tag, tag, os_strlen(tag));
        obs->tag[os_strlen(tag)] = '\0';
    }

    obs->interval_ms = interval_ms;
    obs->window_start_ms = rtos_get_time();
    audio_obs_util_reset_cost(&obs->cost_ms);

    return BK_OK;
}

void audio_obs_util_reset(audio_obs_util_t *obs)
{
    if (!obs)
    {
        return;
    }

    obs->window_start_ms = rtos_get_time();
    audio_obs_util_reset_cost(&obs->cost_ms);
}

void audio_obs_util_add_cost(audio_obs_util_t *obs, uint32_t cost_ms)
{
    uint32_t hist_idx;

    if (!obs)
    {
        return;
    }

    obs->cost_ms.count++;
    obs->cost_ms.sum += cost_ms;

    if (cost_ms < obs->cost_ms.min)
    {
        obs->cost_ms.min = cost_ms;
    }
    if (cost_ms > obs->cost_ms.max)
    {
        obs->cost_ms.max = cost_ms;
    }

    hist_idx = (cost_ms > AUDIO_OBS_HIST_MAX_MS) ? AUDIO_OBS_HIST_MAX_MS : cost_ms;
    if (obs->cost_ms.hist[hist_idx] < AUDIO_OBS_UTIL_U16_MAX)
    {
        obs->cost_ms.hist[hist_idx]++;
    }
}

bool audio_obs_util_should_report(audio_obs_util_t *obs, uint32_t now_ms)
{
    if (!obs || obs->interval_ms == 0)
    {
        return false;
    }

    if (obs->window_start_ms == 0)
    {
        obs->window_start_ms = now_ms;
        return false;
    }

    return (now_ms - obs->window_start_ms >= obs->interval_ms);
}

void audio_obs_util_get_cost(audio_obs_util_t *obs, uint32_t *min_ms, uint32_t *max_ms,
                             uint32_t *avg_ms, uint32_t *p95_ms,
                             uint32_t *p99_ms)
{
    if (!obs)
    {
        return;
    }

    if (min_ms)
    {
        *min_ms = obs->cost_ms.count ? obs->cost_ms.min : 0;
    }
    if (max_ms)
    {
        *max_ms = obs->cost_ms.count ? obs->cost_ms.max : 0;
    }
    if (avg_ms)
    {
        *avg_ms = obs->cost_ms.count ? (obs->cost_ms.sum / obs->cost_ms.count) : 0;
    }
    if (p95_ms)
    {
        *p95_ms = audio_obs_util_cost_percentile(&obs->cost_ms, 95);
    }
    if (p99_ms)
    {
        *p99_ms = audio_obs_util_cost_percentile(&obs->cost_ms, 99);
    }
}

void audio_obs_set_components(uint32_t components)
{
    s_audio_obs_components = components;
}

uint32_t audio_obs_get_components(void)
{
    return s_audio_obs_components;
}

void audio_obs_set_interval_ms(uint32_t interval_ms)
{
    s_audio_obs_interval_ms = interval_ms ? interval_ms : AUDIO_OBS_INTERVAL_MS;
}

uint32_t audio_obs_get_interval_ms(void)
{
    return s_audio_obs_interval_ms;
}

bool audio_obs_component_is_enabled(uint32_t component)
{
    return (s_audio_obs_components & component) != 0;
}

static char audio_obs_tolower(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return (char)(c - 'A' + 'a');
    }

    return c;
}

static bool audio_obs_str_contains(const char *str, const char *needle)
{
    uint32_t str_len;
    uint32_t needle_len;

    if (!str || !needle)
    {
        return false;
    }

    str_len = os_strlen(str);
    needle_len = os_strlen(needle);
    if (needle_len == 0 || str_len < needle_len)
    {
        return false;
    }

    for (uint32_t i = 0; i <= str_len - needle_len; i++)
    {
        uint32_t j;

        for (j = 0; j < needle_len; j++)
        {
            if (audio_obs_tolower(str[i + j]) != audio_obs_tolower(needle[j]))
            {
                break;
            }
        }

        if (j == needle_len)
        {
            return true;
        }
    }

    return false;
}

uint32_t audio_obs_component_from_tag(const char *tag)
{
    if (!tag)
    {
        return 0;
    }

    if (audio_obs_str_contains(tag, "mic"))
    {
        return AUDIO_OBS_COMP_MIC;
    }
    if (audio_obs_str_contains(tag, "aec"))
    {
        return AUDIO_OBS_COMP_AEC;
    }
    if (audio_obs_str_contains(tag, "encoder") || audio_obs_str_contains(tag, "_enc") ||
        audio_obs_str_contains(tag, "encode"))
    {
        return AUDIO_OBS_COMP_ENC;
    }
    if (audio_obs_str_contains(tag, "decoder") || audio_obs_str_contains(tag, "_dec") ||
        audio_obs_str_contains(tag, "decode"))
    {
        return AUDIO_OBS_COMP_DEC;
    }
    if (audio_obs_str_contains(tag, "eq"))
    {
        return AUDIO_OBS_COMP_EQ;
    }
    if (audio_obs_str_contains(tag, "speaker") || audio_obs_str_contains(tag, "spk"))
    {
        return AUDIO_OBS_COMP_SPK;
    }
    if (audio_obs_str_contains(tag, "kws") || audio_obs_str_contains(tag, "asr"))
    {
        return AUDIO_OBS_COMP_KWS;
    }

    return 0;
}

static bool audio_obs_parse_component_name(const char *name, uint32_t len, uint32_t *component)
{
    if (!name || !component)
    {
        return false;
    }

    if (len == 2 && audio_obs_tolower(name[0]) == 'e' && audio_obs_tolower(name[1]) == 'q')
    {
        *component = AUDIO_OBS_COMP_EQ;
        return true;
    }
    if (len == 3 && audio_obs_tolower(name[0]) == 's' && audio_obs_tolower(name[1]) == 'p' && audio_obs_tolower(name[2]) == 'k')
    {
        *component = AUDIO_OBS_COMP_SPK;
        return true;
    }
    if (len == 3 && audio_obs_tolower(name[0]) == 'm' && audio_obs_tolower(name[1]) == 'i' && audio_obs_tolower(name[2]) == 'c')
    {
        *component = AUDIO_OBS_COMP_MIC;
        return true;
    }
    if (len == 3 && audio_obs_tolower(name[0]) == 'a' && audio_obs_tolower(name[1]) == 'e' && audio_obs_tolower(name[2]) == 'c')
    {
        *component = AUDIO_OBS_COMP_AEC;
        return true;
    }
    if (len == 3 && audio_obs_tolower(name[0]) == 'e' && audio_obs_tolower(name[1]) == 'n' && audio_obs_tolower(name[2]) == 'c')
    {
        *component = AUDIO_OBS_COMP_ENC;
        return true;
    }
    if (len == 3 && audio_obs_tolower(name[0]) == 'd' && audio_obs_tolower(name[1]) == 'e' && audio_obs_tolower(name[2]) == 'c')
    {
        *component = AUDIO_OBS_COMP_DEC;
        return true;
    }
    if (len == 3 && audio_obs_tolower(name[0]) == 'k' && audio_obs_tolower(name[1]) == 'w' && audio_obs_tolower(name[2]) == 's')
    {
        *component = AUDIO_OBS_COMP_KWS;
        return true;
    }
    if (len == 3 && audio_obs_tolower(name[0]) == 'a' && audio_obs_tolower(name[1]) == 'l' && audio_obs_tolower(name[2]) == 'l')
    {
        *component = AUDIO_OBS_COMP_ALL;
        return true;
    }
    if (len == 3 && audio_obs_tolower(name[0]) == 'o' && audio_obs_tolower(name[1]) == 'f' && audio_obs_tolower(name[2]) == 'f')
    {
        *component = 0;
        return true;
    }

    return false;
}

bool audio_obs_parse_components(const char *value, uint32_t *components)
{
    const char *start;
    uint32_t result = 0;

    if (!value || !components)
    {
        return false;
    }

    if ((value[0] >= '0' && value[0] <= '9'))
    {
        *components = os_strtoul(value, NULL, 0);
        return true;
    }

    start = value;
    while (*start != '\0')
    {
        const char *end = start;
        uint32_t component = 0;

        while (*end != '\0' && *end != ',')
        {
            end++;
        }

        if (end == start || !audio_obs_parse_component_name(start, (uint32_t)(end - start), &component))
        {
            return false;
        }

        result |= component;
        start = (*end == ',') ? (end + 1) : end;
    }

    *components = result;
    return true;
}

static uint32_t audio_obs_process_pcm_max_abs(const char *data, int len, uint32_t *nonzero_count)
{
    const int16_t *pcm = (const int16_t *)data;
    uint32_t samples = (uint32_t)len / sizeof(int16_t);
    uint32_t max_abs = 0;
    uint32_t nonzero = 0;

    if (!data || len <= 0)
    {
        if (nonzero_count)
        {
            *nonzero_count = 0;
        }
        return 0;
    }

    for (uint32_t i = 0; i < samples; i++)
    {
        int32_t v = pcm[i];
        uint32_t abs_v = (v < 0) ? (uint32_t)(-v) : (uint32_t)v;

        if (abs_v > max_abs)
        {
            max_abs = abs_v;
        }
        if (v != 0)
        {
            nonzero++;
        }
    }

    if (nonzero_count)
    {
        *nonzero_count = nonzero;
    }

    return max_abs;
}

void audio_obs_process_begin(audio_obs_process_stat_t *stat, uint32_t component,
                             char *tag, const char *data, int len,
                             int expected_len)
{
    uint32_t nonzero_count = 0;

    if (!stat || !tag)
    {
        return;
    }

    stat->component = component;
    stat->active = audio_obs_component_is_enabled(component);
    if (!stat->active)
    {
        return;
    }

    if (stat->cost.interval_ms != s_audio_obs_interval_ms)
    {
        audio_obs_util_init(&stat->cost, tag, s_audio_obs_interval_ms);
    }

    if (len != expected_len)
    {
        stat->short_count++;
    }

    stat->frame_in_max = audio_obs_process_pcm_max_abs(data, len, &nonzero_count);
    stat->start_ms = rtos_get_time();
}

void audio_obs_process_end(audio_obs_process_stat_t *stat, char *log_tag,
                           const char *data, int len, int mode, uint32_t filters,
                           int gain)
{
    uint32_t nonzero_count = 0;
    uint32_t out_max_abs;
    uint32_t now_ms;

    if (!stat || !log_tag)
    {
        return;
    }
    if (!stat->active || !audio_obs_component_is_enabled(stat->component))
    {
        return;
    }

    out_max_abs = audio_obs_process_pcm_max_abs(data, len, &nonzero_count);
    audio_obs_util_add_cost(&stat->cost, rtos_get_time() - stat->start_ms);

    if (stat->frame_in_max > stat->window_in_max)
    {
        stat->window_in_max = stat->frame_in_max;
    }
    if (out_max_abs > stat->window_out_max)
    {
        stat->window_out_max = out_max_abs;
    }
    if (stat->frame_in_max == 0 && out_max_abs != 0)
    {
        stat->window_zero_to_nonzero++;
    }

    now_ms = rtos_get_time();
    if (audio_obs_util_should_report(&stat->cost, now_ms))
    {
        uint32_t min_ms = 0;
        uint32_t max_ms = 0;
        uint32_t avg_ms = 0;
        uint32_t p95_ms = 0;
        uint32_t p99_ms = 0;
        uint32_t win_ms = now_ms - stat->cost.window_start_ms;

        audio_obs_util_get_cost(&stat->cost, &min_ms, &max_ms, &avg_ms,
                                &p95_ms, &p99_ms);

        BK_LOGI(log_tag, "obs w:%u n:%u m:%d f:%u g:%d r:%d cost_min/max/avg/p95/p99:%u/%u/%u/%u/%u im:%u om:%u z2n:%u sh:%u\n",
                win_ms, stat->cost.cost_ms.count, mode, filters, gain, len,
                min_ms, max_ms, avg_ms, p95_ms, p99_ms,
                stat->window_in_max, stat->window_out_max,
                stat->window_zero_to_nonzero, stat->short_count);

        audio_obs_util_reset(&stat->cost);
        stat->short_count = 0;
        stat->window_in_max = 0;
        stat->window_out_max = 0;
        stat->window_zero_to_nonzero = 0;
    }
}

void audio_obs_event_begin(audio_obs_event_stat_t *stat, uint32_t component)
{
    if (!stat)
    {
        return;
    }

    stat->component = component;
    stat->active = audio_obs_component_is_enabled(component);
    if (stat->active)
    {
        stat->start_ms = rtos_get_time();
    }
}

void audio_obs_event_report(audio_obs_util_t *obs, audio_obs_event_stat_t *stat,
                            uint32_t component, char *tag,
                            char *log_tag, uint32_t src, int r_size,
                            uint32_t frame_size)
{
    uint32_t now_ms;

    if (!obs || !stat || !tag || !log_tag)
    {
        return;
    }

    if (!audio_obs_component_is_enabled(component))
    {
        os_memset(stat, 0, sizeof(audio_obs_event_stat_t));
        return;
    }

    if (obs->interval_ms != s_audio_obs_interval_ms)
    {
        audio_obs_util_init(obs, tag, s_audio_obs_interval_ms);
    }

    now_ms = rtos_get_time();
    if (stat->active && stat->component == component)
    {
        audio_obs_util_add_cost(obs, now_ms - stat->start_ms);
    }

    if (audio_obs_util_should_report(obs, now_ms))
    {
        uint32_t min_ms = 0;
        uint32_t max_ms = 0;
        uint32_t avg_ms = 0;
        uint32_t p95_ms = 0;
        uint32_t p99_ms = 0;

        audio_obs_util_get_cost(obs, &min_ms, &max_ms, &avg_ms,
                                &p95_ms, &p99_ms);

        BK_LOGI(log_tag, "obs w:%u n:%u src:%u r:%d fs:%u cost_min/max/avg/p95/p99:%u/%u/%u/%u/%u sh:%u fl:%u to:%u sf:%u\n",
                now_ms - obs->window_start_ms,
                stat->process_count, src, r_size, frame_size,
                min_ms, max_ms, avg_ms, p95_ms, p99_ms,
                stat->short_count, stat->fill_count,
                stat->sem_timeout_count, stat->sem_fail_count);

        audio_obs_util_reset(obs);
        os_memset(stat, 0, sizeof(audio_obs_event_stat_t));
    }
}

void audio_obs_element_begin(audio_obs_element_stat_t *stat, const char *tag)
{
    uint32_t component;

    if (!stat || !tag)
    {
        return;
    }

    component = audio_obs_component_from_tag(tag);
    stat->component = component;

    stat->active = (component != 0) && audio_obs_component_is_enabled(component);
    if (!stat->active)
    {
        return;
    }

    if (stat->cost.interval_ms != s_audio_obs_interval_ms)
    {
        audio_obs_util_init(&stat->cost, tag, s_audio_obs_interval_ms);
    }

    stat->start_ms = rtos_get_time();
}

void audio_obs_element_report(audio_obs_element_stat_t *stat, const char *tag,
                              char *log_tag, int ret, uint32_t frame_size)
{
    uint32_t now_ms;

    if (!stat || !tag || !log_tag)
    {
        return;
    }
    if (!stat->active || !audio_obs_component_is_enabled(stat->component))
    {
        return;
    }

    now_ms = rtos_get_time();
    audio_obs_util_add_cost(&stat->cost, now_ms - stat->start_ms);
    stat->process_count++;

    if (ret < 0)
    {
        stat->err_count++;
    }

    if (ret == -4)
    {
        stat->timeout_count++;
    }
    else if (ret == -2)
    {
        stat->done_count++;
    }
    else if (ret == 0)
    {
        stat->zero_count++;
    }
    else if ((frame_size > 0) && ((uint32_t)ret < frame_size))
    {
        stat->short_count++;
    }

    if (audio_obs_util_should_report(&stat->cost, now_ms))
    {
        uint32_t min_ms = 0;
        uint32_t max_ms = 0;
        uint32_t avg_ms = 0;
        uint32_t p95_ms = 0;
        uint32_t p99_ms = 0;

        audio_obs_util_get_cost(&stat->cost, &min_ms, &max_ms, &avg_ms,
                                &p95_ms, &p99_ms);

        BK_LOGI(log_tag, "obs tag:%s comp:0x%x w:%u n:%u r:%d fs:%u cost_min/max/avg/p95/p99:%u/%u/%u/%u/%u err:%u to:%u short:%u zero:%u done:%u\n",
                tag, stat->component, now_ms - stat->cost.window_start_ms,
                stat->process_count, ret, frame_size,
                min_ms, max_ms, avg_ms, p95_ms, p99_ms,
                stat->err_count, stat->timeout_count, stat->short_count,
                stat->zero_count, stat->done_count);

        audio_obs_util_reset(&stat->cost);
        os_memset(stat, 0, sizeof(audio_obs_element_stat_t));
    }
}
