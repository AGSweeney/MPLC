/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * ClearPath golden profile (logic only):
 * - Step/direction mode with 1:1 pulse-to-position resolution
 * - HLFB must be asserted before coordinated motion when configured
 * - Typical step clock: 5000 Hz coordinated with MotionStack tick rate
 */

#include "mplc_motionstack_drive.h"
#include <string.h>

typedef struct {
    mplc_ms_drive_config_t cfg;
    mplc_ms_drive_status_t status;
    mplc_ms_hlfb_state_t hlfb_state;
} mplc_ms_clearpath_drive_ctx_t;

static void clearpath_init(void *ctx, const mplc_ms_drive_config_t *cfg)
{
    mplc_ms_clearpath_drive_ctx_t *d = (mplc_ms_clearpath_drive_ctx_t *)ctx;

    if (!d) {
        return;
    }
    memset(d, 0, sizeof(*d));
    if (cfg) {
        d->cfg = *cfg;
    } else {
        d->cfg.kind = MPLC_MS_DRIVE_CLEARPATH;
        d->cfg.hlfb_mode = MPLC_MS_HLFB_STATIC;
        d->cfg.step_clock_hz = 5000U;
        d->cfg.require_hlfb_for_motion = true;
    }
    d->status.kind = MPLC_MS_DRIVE_CLEARPATH;
    d->hlfb_state = MPLC_MS_HLFB_DEASSERTED;
}

static int clearpath_enable(void *ctx, bool on)
{
    mplc_ms_clearpath_drive_ctx_t *d = (mplc_ms_clearpath_drive_ctx_t *)ctx;

    if (!d) {
        return -1;
    }
    d->status.enabled = on;
    if (on) {
        d->hlfb_state = MPLC_MS_HLFB_ASSERTED;
    } else {
        d->hlfb_state = MPLC_MS_HLFB_DEASSERTED;
    }
    return 0;
}

static int clearpath_clear_faults(void *ctx)
{
    mplc_ms_clearpath_drive_ctx_t *d = (mplc_ms_clearpath_drive_ctx_t *)ctx;

    if (!d) {
        return -1;
    }
    d->status.alert_present = false;
    d->status.alert_code = 0;
    if (d->status.enabled) {
        d->hlfb_state = MPLC_MS_HLFB_ASSERTED;
    }
    return 0;
}

static int clearpath_before_motion(void *ctx, mplc_ms_drive_status_t *status)
{
    mplc_ms_clearpath_drive_ctx_t *d = (mplc_ms_clearpath_drive_ctx_t *)ctx;

    if (!d || !status) {
        return -1;
    }
    if (!d->status.enabled) {
        return -2;
    }
    if (d->status.alert_present) {
        return -3;
    }
    if (d->cfg.require_hlfb_for_motion && d->hlfb_state != MPLC_MS_HLFB_ASSERTED) {
        return -4;
    }
    *status = d->status;
    return 0;
}

static void clearpath_on_steps(void *ctx, int32_t delta)
{
    mplc_ms_clearpath_drive_ctx_t *d = (mplc_ms_clearpath_drive_ctx_t *)ctx;

    if (!d) {
        return;
    }
    d->status.commanded_steps += delta;
    if (delta < 0) {
        d->status.direction_negative = true;
    } else if (delta > 0) {
        d->status.direction_negative = false;
    }
}

static void clearpath_get_status(void *ctx, mplc_ms_drive_status_t *status)
{
    mplc_ms_clearpath_drive_ctx_t *d = (mplc_ms_clearpath_drive_ctx_t *)ctx;

    if (!d || !status) {
        return;
    }
    *status = d->status;
}

const mplc_ms_drive_ops_t mplc_ms_drive_clearpath_ops = {
    clearpath_init,
    clearpath_enable,
    clearpath_clear_faults,
    clearpath_before_motion,
    clearpath_on_steps,
    clearpath_get_status
};
