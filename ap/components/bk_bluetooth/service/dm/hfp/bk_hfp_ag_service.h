/**
 * @file bk_hfp_ag_service.h
 *
 * @brief Hands-Free Profile AG (Audio Gateway) service wrapper.
 *
 * This service owns the HFP AG lifecycle: it registers the AG event/data
 * callbacks, configures the AG feature bitmaps, integrates with bt_manager,
 * tracks the connected peer, and automatically drives the SCO audio engine
 * (hfp_ag_audio) when the audio link comes up / goes down.
 *
 * Product policy (answering AT+CIND?/+COPS/+CNUM/+CLCC, driving the call state
 * machine, choosing an operator name, etc.) stays in the application: register
 * a callback with bk_hfp_ag_service_register_cb(); the service invokes it AFTER
 * applying its own internal handling for each AG event.
 */

#ifndef BK_HFP_AG_SERVICE_H
#define BK_HFP_AG_SERVICE_H

#include <stdint.h>
#include "components/bluetooth/bk_dm_hfp_ag.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Register the application (policy) callback.
 *
 * Call before bk_hfp_ag_service_init(). The callback uses the raw AG event
 * signature; the service has already tracked the peer and driven SCO audio
 * before it is invoked, so the app only implements product behaviour.
 *
 * @param user_cb AG event callback (may be NULL to clear).
 * @return 0 on success.
 */
int bk_hfp_ag_service_register_cb(bk_bt_hf_ag_cb_t user_cb);

/**
 * @brief Initialize the HFP AG service (register callbacks, init AG, configure
 *        features, hook bt_manager).
 * @return 0 on success, negative on failure.
 */
int bk_hfp_ag_service_init(void);

/**
 * @brief Deinitialize the HFP AG service.
 * @return 0 on success.
 */
int bk_hfp_ag_service_deinit(void);

/** @brief Get the tracked peer address (6 bytes), or NULL when none. */
const uint8_t *bk_hfp_ag_service_get_peer(void);

/** @brief Non-zero once the service-level connection (SLC) is up. */
uint8_t bk_hfp_ag_service_is_connected(void);

/** @brief Establish the service-level connection to a remote HF. */
int bk_hfp_ag_service_connect(const uint8_t bda[6]);

/** @brief Disconnect the service-level connection from the tracked peer. */
int bk_hfp_ag_service_disconnect(void);

/** @brief Establish the (e)SCO audio connection to the tracked peer. */
int bk_hfp_ag_service_audio_connect(void);

/** @brief Release the (e)SCO audio connection to the tracked peer. */
int bk_hfp_ag_service_audio_disconnect(void);

/** @brief Choose the SCO codec (1 = mSBC, 0 = CVSD) for the tracked peer. */
int bk_hfp_ag_service_set_codec(uint8_t msbc);

#ifdef __cplusplus
}
#endif

#endif /* BK_HFP_AG_SERVICE_H */
