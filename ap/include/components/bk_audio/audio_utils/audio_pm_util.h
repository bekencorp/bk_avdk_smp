#ifndef __AUDIO_PM_UTIL_H__
#define __AUDIO_PM_UTIL_H__

#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Acquire a refcounted audio (PM_DEV_ID_AUDIO) CPU frequency vote.
 *
 *             bk_pm_module_vote_cpu_freq() keeps a single slot per module (last
 *             write wins). Several audio consumers (audio_play, bk_player_service,
 *             ...) all need PM_DEV_ID_AUDIO at 480M at the same time; a single
 *             consumer releasing with PM_CPU_FRQ_DEFAULT would clobber the others'
 *             vote and throttle the CPU mid-playback. These helpers share one
 *             refcount so the frequency is raised on the first acquire and only
 *             dropped after the last release.
 *
 * @return     The result
 *             - BK_OK: success
 */
bk_err_t bk_audio_pm_vote_cpu_freq_acquire(void);

/**
 * @brief      Release the refcounted audio CPU frequency vote acquired by
 *             bk_audio_pm_vote_cpu_freq_acquire(). The frequency drops back to default
 *             only after the last outstanding acquire is released.
 *
 * @return     The result
 *             - BK_OK: success
 */
bk_err_t bk_audio_pm_vote_cpu_freq_release(void);

#ifdef __cplusplus
}
#endif

#endif