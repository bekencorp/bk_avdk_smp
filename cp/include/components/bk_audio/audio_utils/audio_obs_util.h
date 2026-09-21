#pragma once

#include <stdbool.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_OBS_TAG_LEN           20
#define AUDIO_OBS_HIST_MAX_MS      100
#define AUDIO_OBS_INTERVAL_MS      (1000 * 5)

#define AUDIO_OBS_COMP_EQ          (1u << 0)
#define AUDIO_OBS_COMP_SPK         (1u << 1)
#define AUDIO_OBS_COMP_MIC         (1u << 2)
#define AUDIO_OBS_COMP_AEC         (1u << 3)
#define AUDIO_OBS_COMP_ENC         (1u << 4)
#define AUDIO_OBS_COMP_DEC         (1u << 5)
#define AUDIO_OBS_COMP_KWS         (1u << 6)
#define AUDIO_OBS_COMP_ALL         (AUDIO_OBS_COMP_EQ | AUDIO_OBS_COMP_SPK | \
                                    AUDIO_OBS_COMP_MIC | AUDIO_OBS_COMP_AEC | \
                                    AUDIO_OBS_COMP_ENC | AUDIO_OBS_COMP_DEC | \
                                    AUDIO_OBS_COMP_KWS)

typedef struct
{
    uint32_t count;                                 /**< sample count in current window */
    uint32_t sum;                                   /**< cost sum in current window */
    uint32_t min;                                   /**< minimum cost in current window */
    uint32_t max;                                   /**< maximum cost in current window */
    uint16_t hist[AUDIO_OBS_HIST_MAX_MS + 1];       /**< cost histogram, last bucket means >= max */
} audio_obs_cost_stat_t;

typedef struct
{
    char tag[AUDIO_OBS_TAG_LEN];                    /**< observe tag */
    uint32_t interval_ms;                           /**< observe window interval(ms) */
    uint32_t window_start_ms;                       /**< observe window start time(ms) */
    audio_obs_cost_stat_t cost_ms;                  /**< cost statistics in ms */
} audio_obs_util_t;

typedef struct
{
    uint32_t process_count;       /**< process count in current component */
    uint32_t short_count;         /**< short input/read count */
    uint32_t fill_count;          /**< fill/recover count */
    uint32_t sem_timeout_count;   /**< semaphore timeout count */
    uint32_t sem_fail_count;      /**< semaphore operation fail count */
    uint32_t start_ms;            /**< process start time(ms) */
    uint32_t component;           /**< observed component bit */
    bool active;                  /**< whether this sample is active */
} audio_obs_event_stat_t;

typedef struct
{
    audio_obs_util_t cost;
    uint32_t short_count;
    uint32_t window_in_max;
    uint32_t window_out_max;
    uint32_t window_zero_to_nonzero;
    uint32_t frame_in_max;
    uint32_t start_ms;
    uint32_t component;
    bool active;
} audio_obs_process_stat_t;

typedef struct
{
    audio_obs_util_t cost;
    uint32_t process_count;
    uint32_t err_count;
    uint32_t timeout_count;
    uint32_t short_count;
    uint32_t zero_count;
    uint32_t done_count;
    uint32_t start_ms;
    uint32_t component;
    bool active;
} audio_obs_element_stat_t;

/**
 * @brief      Initialize an audio observe util.
 *
 * @param[in]  obs          The observe util handle
 * @param[in]  tag          The observe tag
 * @param[in]  interval_ms  The observe interval(ms)
 *
 * @return
 *             - BK_OK: success
 *             - BK_FAIL: failed
 */
bk_err_t audio_obs_util_init(audio_obs_util_t *obs, const char *tag, uint32_t interval_ms);

/**
 * @brief      Reset the current observe window.
 *
 * @param[in]  obs  The observe util handle
 *
 * @return     None
 */
void audio_obs_util_reset(audio_obs_util_t *obs);

/**
 * @brief      Add one cost sample to the current observe window.
 *
 * @param[in]  obs      The observe util handle
 * @param[in]  cost_ms  The cost sample(ms)
 *
 * @return     None
 */
void audio_obs_util_add_cost(audio_obs_util_t *obs, uint32_t cost_ms);

/**
 * @brief      Check whether the observe window should report.
 *
 * @param[in]  obs     The observe util handle
 * @param[in]  now_ms  Current time(ms)
 *
 * @return
 *             - true: should report
 *             - false: should not report
 */
bool audio_obs_util_should_report(audio_obs_util_t *obs, uint32_t now_ms);

/**
 * @brief      Get current cost statistics.
 *
 * @param[in]   obs        The observe util handle
 * @param[out]  min_ms     The minimum cost(ms)
 * @param[out]  max_ms     The maximum cost(ms)
 * @param[out]  avg_ms     The average cost(ms)
 * @param[out]  p95_ms     The p95 cost(ms)
 * @param[out]  p99_ms     The p99 cost(ms)
 *
 * @return     None
 */
void audio_obs_util_get_cost(audio_obs_util_t *obs, uint32_t *min_ms, uint32_t *max_ms,
                             uint32_t *avg_ms, uint32_t *p95_ms,
                             uint32_t *p99_ms);

void audio_obs_set_components(uint32_t components);

uint32_t audio_obs_get_components(void);

void audio_obs_set_interval_ms(uint32_t interval_ms);

uint32_t audio_obs_get_interval_ms(void);

bool audio_obs_component_is_enabled(uint32_t component);

bool audio_obs_parse_components(const char *value, uint32_t *components);

uint32_t audio_obs_component_from_tag(const char *tag);

void audio_obs_process_begin(audio_obs_process_stat_t *stat, uint32_t component,
                             char *tag, const char *data, int len,
                             int expected_len);

void audio_obs_process_end(audio_obs_process_stat_t *stat, char *log_tag,
                           const char *data, int len, int mode, uint32_t filters,
                           int gain);

void audio_obs_event_begin(audio_obs_event_stat_t *stat, uint32_t component);

void audio_obs_event_report(audio_obs_util_t *obs, audio_obs_event_stat_t *stat,
                            uint32_t component, char *tag,
                            char *log_tag, uint32_t src, int r_size,
                            uint32_t frame_size);

void audio_obs_element_begin(audio_obs_element_stat_t *stat, const char *tag);

void audio_obs_element_report(audio_obs_element_stat_t *stat, const char *tag,
                              char *log_tag, int ret, uint32_t frame_size);

#if CONFIG_ADK_OBS_UTIL
#define AUDIO_OBS_PROCESS_STAT(name) \
    static audio_obs_process_stat_t name = {0}
#define AUDIO_OBS_PROCESS_BEGIN(stat, component, tag, data, len, expected_len) \
    audio_obs_process_begin((stat), (component), (tag), (data), (len), (expected_len))
#define AUDIO_OBS_PROCESS_END(stat, log_tag, data, len, mode, filters, gain) \
    audio_obs_process_end((stat), (log_tag), (data), (len), (mode), (filters), (gain))
#else
#define AUDIO_OBS_PROCESS_STAT(name)
#define AUDIO_OBS_PROCESS_BEGIN(stat, component, tag, data, len, expected_len) do { } while (0)
#define AUDIO_OBS_PROCESS_END(stat, log_tag, data, len, mode, filters, gain) do { } while (0)
#endif

#ifdef __cplusplus
}
#endif
