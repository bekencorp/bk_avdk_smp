#ifndef SCTP_LWIP_THREAD_SAFE_H
#define SCTP_LWIP_THREAD_SAFE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Thread-safe wrapper for sctp_init_ifns_for_vrf
 *
 * This function ensures that the network interface operations happen in the TCPIP thread
 * to avoid thread safety issues with LWIP when CONFIG_LWIP_CHECK_THREAD_SAFETY is enabled.
 *
 * @param vrfid The VRF ID to initialize interfaces for
 */
void sctp_lwip_init_ifns_for_vrf_safe(uint32_t vrfid);

#ifdef __cplusplus
}
#endif

#endif /* SCTP_LWIP_THREAD_SAFE_H */
