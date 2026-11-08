/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 * Copyright (c)     2023-2028 Cypress Semiconductor Corporation (an Infineon company)
 * or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* This file provides implementation of TF-M NS os wrapper functions for the
 * RTOS use case. This implementation provides multithread safety, so it
 * can be used in RTOS environment.
 */

#include <stdint.h>
#include <os/os.h>

#include "os_wrapper/mutex.h"

#include "tfm_ns_interface.h"

/**
 * \brief the ns_lock ID
 */
static void *ns_lock_handle = NULL;

/* Saved interrupt level of the current secure call. Single level: the secure
 * call lock does not support nesting. */
static uint32_t ns_secure_call_int_level;

/* Lazily create the NS lock: NSC gateways can be called before
 * tfm_ns_interface_init(), and first callers from different tasks would race on
 * creation, so it is done under a critical section. */
static int32_t ns_lock_init(void)
{
    uint32_t int_status;

    if (ns_lock_handle != NULL) {
        return OS_WRAPPER_SUCCESS;
    }

    int_status = rtos_disable_int();
    if (ns_lock_handle == NULL) {
        ns_lock_handle = os_wrapper_mutex_create();
    }
    rtos_enable_int(int_status);

    return (ns_lock_handle != NULL) ? OS_WRAPPER_SUCCESS : OS_WRAPPER_ERROR;
}

void tfm_ns_secure_call_lock(void)
{
    if (ns_lock_init() != OS_WRAPPER_SUCCESS) {
        return;
    }

    /* Plain mutex: call sites must not nest, re-locking from the same task
     * would deadlock. */
    while (os_wrapper_mutex_acquire(ns_lock_handle, OS_WRAPPER_WAIT_FOREVER)
            != OS_WRAPPER_SUCCESS) {
    }

    ns_secure_call_int_level = rtos_disable_int();
}

void tfm_ns_secure_call_unlock(void)
{
    if (ns_lock_handle == NULL) {
        return;
    }

    rtos_enable_int(ns_secure_call_int_level);

    while (os_wrapper_mutex_release(ns_lock_handle) != OS_WRAPPER_SUCCESS) {
    }
}

int32_t tfm_ns_interface_dispatch(veneer_fn fn,
                                  uint32_t arg0, uint32_t arg1,
                                  uint32_t arg2, uint32_t arg3)
{
    int32_t result;

    if (ns_lock_init() != OS_WRAPPER_SUCCESS) {
        return OS_WRAPPER_ERROR;
    }

    /* TFM request protected by NS lock. Interrupts stay enabled: a PSA call can
     * be long, and the SG handler cannot be preempted by a task switch. */
    while (os_wrapper_mutex_acquire(ns_lock_handle, OS_WRAPPER_WAIT_FOREVER)
            != OS_WRAPPER_SUCCESS) {
    }

    result = fn(arg0, arg1, arg2, arg3);

    while (os_wrapper_mutex_release(ns_lock_handle) != OS_WRAPPER_SUCCESS) {
    }

    return result;
}

uint32_t tfm_ns_interface_init(void)
{
    return (ns_lock_init() != OS_WRAPPER_SUCCESS) ? OS_WRAPPER_ERROR
                                                  : OS_WRAPPER_SUCCESS;
}
