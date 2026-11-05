// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

/* encp_nsc: ARMv8-M CMSE gateways that let the Non-secure cpu0_app hand the
 * TrustEngine (TE200) over to the Secure world before a sleep and ask for it
 * back afterwards.
 *
 * Why this must live in the Secure world:
 *   - The TE200 control block (0x0000~0x00ff) and the TRNG CTRL block
 *     (0x0200~0x027f) are Secure-only; a Non-secure write raises an APB error.
 *   - The engine sits in the WRLP_ENCP power domain (SYS reg0x10 bit2), which
 *     the NS PM votes off on sleep entry, so the whole register file including
 *     the TRNG configuration is lost.
 *   - Only the Secure window's TRNG setup values are authoritative, so the
 *     rebuild has to run here as well.
 *
 * The gateways are deliberately thin: all engine work is done by the Dubhe
 * driver already linked into the Secure image (crypto_service_crypto_hw).
 * They must stay self-contained - no PSA/IPC calls, no blocking, no SPM
 * scheduling - because they are reached through plain SG veneers and the NS
 * side calls them from the PM task with interrupts enabled. */

#include <stdint.h>

#include "security_defs.h"
#include "otp_s.h"

/* Secure alias of the TrustEngine register block. */
#define ENCP_TE200_BASE      (0x42110000UL)

/* Dubhe driver entry points (see dubhe_driver.h). */
extern void dubhe_driver_cleanup(void);
extern void dubhe_driver_lv_resume(unsigned long dbh_base_addr);

/* .bss: zero after a cold boot or a Deep-LV reset, so a reset ticket always
 * forces the full rebuild. Only used to make a re-run of the enter callback
 * (aborted suspend) a no-op. */
static volatile uint32_t s_encp_quiesced;

/* Link anchor: referenced from tfm_hal_get_ns_entry_point() under
 * CONFIG_ENCP_NSC so --gc-sections keeps this translation unit (and therefore
 * the gateway veneers) in tfm_s. */
void psa_encp_nsc_stub(void)
{
	return;
}

/* Sleep entry: stop the engine and release the ACA resource table while the
 * domain is still powered. Idempotent - the PM framework may re-run the enter
 * callbacks after an aborted suspend. */
__tz_c_veneer int psa_encp_quiesce(void)
{
	if (s_encp_quiesced != 0U) {
		return 0;
	}

	/* No PSA request can be in flight here: the NS PM task only runs once every
	 * client has stopped issuing crypto, and PSA calls are synchronous. */
	dubhe_driver_cleanup();
	s_encp_quiesced = 1U;
	return 0;
}

/* Wake-up: rebuild the minimum engine state. Power-on reset already cleared
 * every sub-block, so only the sub-block clocks, the engine configuration and
 * the TRNG product settings have to be programmed; the slow OTP manager
 * bring-up is done lazily on the first OTP access instead. Always executed -
 * the NS side cannot know whether the domain really lost power, and the
 * sequence is safe to repeat. */
__tz_c_veneer int psa_encp_resume(void)
{
	dubhe_driver_lv_resume(ENCP_TE200_BASE);

	/* The OTP secure-range protection is a TrustEngine register setting and was
	 * cleared with the domain, so it has to be re-asserted here. Plain register
	 * access needs no OTP power-up handshake. */
	(void)otp_secure_range_enable();

	s_encp_quiesced = 0U;
	return 0;
}

/*************************** The End Of File*****************************/
