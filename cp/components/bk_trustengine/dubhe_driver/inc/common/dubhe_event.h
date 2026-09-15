/**
 * @if copyright_display
 *      Copyright (C), 2018-2018, Arm Technology (China) Co., Ltd.
 *      All rights reserved
 *
 *      The content of this file or document is CONFIDENTIAL and PROPRIETARY
 *      to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 *      License Agreement between Licensee and Arm Technology (China) Co., Ltd
 *      restricting among other things, the use, reproduction, distribution
 *      and transfer.  Each of the embodiments, including this information and
 *      any derivative work shall retain this copyright notice.
 * @endif
 */

#ifndef __DUBHE_EVENT_H__
#define __DUBHE_EVENT_H__

#include "pal.h"
#include "pal_mutex.h"

/* Self-contained lock context for the adaptive mutex helpers below. It is kept
 * independent from os/os.h on purpose: this header is also compiled in the
 * secure TF-M (TEE_M) build where the armino os abstraction is not on the
 * include path. */
typedef struct {
    uint8_t locked;        /* 1: the underlying mutex was acquired */
    uint8_t irq_disabled;  /* 1: acquired via non-blocking take */
} dubhe_lock_ctx_t;

typedef enum _dubhe_event_type_t {
    DBH_EVENT_SCA_CMD_EXCUTED = 0,
    DBH_EVENT_HASH_CMD_EXCUTED,
    DBH_EVENT_ACA_DONE,
    DBH_EVENT_ACA_FIFO_READY,
} dubhe_event_type_t;

typedef enum _dubhe_mutex_type_t {
    DBH_SCA_MUTEX = 0,
    DBH_HASH_MUTEX,
    DBH_ACA_MUTEX,
    DBH_TRNG_MUTEX,
    DBH_OTP_MUTEX,
} dubhe_mutex_type_t;

int32_t dubhe_event_init( void );
void dubhe_event_cleanup( void );
#if defined(DUBHE_FOR_RUNTIME)
int32_t dubhe_wait_event( dubhe_event_type_t dubhe_event );
int32_t dubhe_release_event( dubhe_event_type_t dubhe_event );
void dubhe_event_set_sca_status(int32_t status);
void dubhe_event_set_hash_status(int32_t status);
#endif
int32_t dubhe_mutex_lock( dubhe_mutex_type_t dubhe_mutex );
int32_t dubhe_mutex_unlock( dubhe_mutex_type_t dubhe_mutex );

/* Adaptive variants for code paths that may run with interrupts disabled.
 * Normal task context blocks on the same mutex as dubhe_mutex_lock(); task
 * context with interrupts disabled tries the same mutex without blocking. ISR
 * context is rejected because mutex ownership cannot be changed safely there.
 * The caller must pass the same @p ctx to the matching unlock. */
int32_t dubhe_mutex_lock_adaptive( dubhe_mutex_type_t dubhe_mutex, dubhe_lock_ctx_t *ctx );
int32_t dubhe_mutex_unlock_adaptive( dubhe_mutex_type_t dubhe_mutex, dubhe_lock_ctx_t *ctx );

#endif
/*************************** The End Of File*****************************/
