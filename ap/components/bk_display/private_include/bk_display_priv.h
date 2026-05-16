/*
 * SPDX-FileCopyrightText: 2024 Beken Corp.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Internal display controller layout. External callers go through
 * the opaque ::bk_display_ctlr_handle_t and the public bk_display_*
 * API; only sources inside ap/components/bk_display include this.
 */
#pragma once

#include <components/bk_display.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display controller ops table. Backends embed this struct as
 *        the @c ops member and cast the public handle pointer to the
 *        embedded base.
 */
typedef struct bk_display_ctlr_t bk_display_ctlr_t;
struct bk_display_ctlr_t
{
    avdk_err_t (*init)(bk_display_ctlr_t *controller);
    avdk_err_t (*open)(bk_display_ctlr_t *controller);
    avdk_err_t (*close)(bk_display_ctlr_t *controller);
    avdk_err_t (*deinit)(bk_display_ctlr_t *controller);
    avdk_err_t (*suspend)(bk_display_ctlr_t *controller);
    avdk_err_t (*resume)(bk_display_ctlr_t *controller);
    avdk_err_t (*flush)(bk_display_ctlr_t *controller, uint8_t *frame, flush_free_cb_t cb);
    avdk_err_t (*ioctl)(bk_display_ctlr_t *controller, bk_display_ioctl_cmd_t cmd, void *arg);
    avdk_err_t (*del)(bk_display_ctlr_t *controller);
};

#ifdef __cplusplus
}
#endif
