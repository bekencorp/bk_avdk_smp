#ifndef __CP_AUDIO_RES_H__
#define __CP_AUDIO_RES_H__

#include <stdbool.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Speaker / mic arbitration for this test project.
 *
 * onboard_speaker_stream and onboard_mic_stream each keep one global instance
 * and their init path reallocates and zeroes it unconditionally. A second owner
 * starting while the first one's DMA is still running therefore clears the
 * semaphore the DMA finish ISR gives, which asserts in xQueueGiveFromISR.
 *
 * This project exposes five speaker owners (voice_service, player_service
 * playback, cp_prompt, cp_audio_play direct-DAC, spk_service) and three mic
 * owners (voice_service, asr, cp_audio_record) as independent CLI commands, so
 * any start order is reachable from the console. Each owner claims its resource
 * by name before init and releases it after deinit.
 *
 * Claims are reference counted per owner name, which is what lets several
 * spk_service sources (A2DP + CALL + HINT) hold the speaker at once while still
 * locking out the owners that would rebuild the speaker underneath them.
 */

#define CP_AUD_OWNER_VOICE_SERVICE   "voice_service"
#define CP_AUD_OWNER_PLAYER_SERVICE  "player_service"
#define CP_AUD_OWNER_PROMPT          "cp_prompt"
#define CP_AUD_OWNER_AUDIO_PLAY      "cp_audio_play"
#define CP_AUD_OWNER_SPK_SERVICE     "spk_service"
#define CP_AUD_OWNER_ASR             "asr"
#define CP_AUD_OWNER_AUDIO_RECORD    "cp_audio_record"

/**
 * @brief  Claim the speaker for @p owner, or add a reference if @p owner
 *         already holds it.
 *
 * Prints the current holder and fails when a different owner holds it.
 */
bk_err_t cp_aud_spk_acquire(const char *owner);

/** @brief  Drop one speaker reference held by @p owner. */
void cp_aud_spk_release(const char *owner);

/** @brief  Current speaker owner name, or NULL when free. */
const char *cp_aud_spk_owner(void);

/**
 * @brief  Whether the speaker is currently held by @p owner.
 *
 * For paths that do not own the speaker but need a specific owner present, such
 * as player_service's prompt_tone riding on voice_service's speaker.
 */
bool cp_aud_spk_owned_by(const char *owner);

/** @brief  Claim the mic for @p owner. See cp_aud_spk_acquire(). */
bk_err_t cp_aud_mic_acquire(const char *owner);

/** @brief  Drop one mic reference held by @p owner. */
void cp_aud_mic_release(const char *owner);

/** @brief  Current mic owner name, or NULL when free. */
const char *cp_aud_mic_owner(void);

/** @brief  Print both resources and their reference counts. */
void cp_aud_res_dump(void);

/**
 * @brief  Bring up spk_service and claim the speaker for it, if not already done.
 *
 * spk_service is a singleton whose lifecycle has exactly one owner; cp_spk is
 * that owner for this project. Anything that needs the service goes through
 * here instead of calling spk_service_init() and taking the token itself, so
 * the token is held for as long as the DAC is alive. Only "cp_spk deinit"
 * tears the service down and returns the speaker.
 *
 * @return BK_OK when the service is running and the speaker is ours.
 */
bk_err_t cp_spk_service_ensure(void);

#ifdef __cplusplus
}
#endif

#endif /* __CP_AUDIO_RES_H__ */
