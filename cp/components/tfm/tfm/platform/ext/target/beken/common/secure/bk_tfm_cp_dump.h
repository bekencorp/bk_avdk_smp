/*
 * Copyright 2026 Beken
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/*
 * Attempt a one-way handoff of the captured CP SecureFault context to the
 * Non-Secure coredump callback. The function returns only when the NS image is
 * not ready, validation fails, or the callback unexpectedly returns; the
 * caller must then preserve TF-M's normal panic policy.
 */
void tfm_hal_secure_fault_handoff(void);
