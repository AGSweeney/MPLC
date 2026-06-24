/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_drive.h"
#include <string.h>

typedef struct {
    mplc_ms_drive_config_t cfg;
    mplc_ms_drive_status_t status;
} mplc_ms_generic_drive_ctx_t;

static void generic_init(void *ctx, const mplc_ms_drive_config_t *cfg)
{
    mplc_ms_generic_drive_ctx_t *d = (mplc_ms_generic_drive_ctx_t *)ctx;

    if (!d) {
        return;
    }
    memset(d, 0, sizeof(*d));
    if (cfg) {
        d->cfg = *cfg;
    }
    d->status.kind = MPLC_MS_DRIVE_GENERIC_STEPPER;
}

static int generic_enable(void *ctx, bool on)
{
    mplc_ms_generic_drive_ctx_t *d = (mplc_ms_generic_drive_ctx_t *)ctx;

    if (!d) {
        return -1;
    }
    d->status.enabled = on;
    return 0;
}

static int generic_clear_faults(void *ctx)
{
    mplc_ms_generic_drive_ctx_t *d = (mplc_ms_generic_drive_ctx_t *)ctx;

    if (!d) {
        return -1;
    }
    d->status.alert_present = false;
    d->status.alert_code = 0;
    return 0;
}

static int generic_before_motion(void *ctx, mplc_ms_drive_status_t *status)
{
    mplc_ms_generic_drive_ctx_t *d = (mplc_ms_generic_drive_ctx_t *)ctx;

    if (!d || !status) {
        return -1;
    }
    if (!d->status.enabled) {
        return -2;
    }
    if (d->status.alert_present) {
        return -3;
    }
    *status = d->status;
    return 0;
}

static void generic_on_steps(void *ctx, int32_t delta)
{
    mplc_ms_generic_drive_ctx_t *d = (mplc_ms_generic_drive_ctx_t *)ctx;

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

static void generic_get_status(void *ctx, mplc_ms_drive_status_t *status)
{
    mplc_ms_generic_drive_ctx_t *d = (mplc_ms_generic_drive_ctx_t *)ctx;

    if (!d || !status) {
        return;
    }
    *status = d->status;
}

const mplc_ms_drive_ops_t mplc_ms_drive_generic_stepper_ops = {
    generic_init,
    generic_enable,
    generic_clear_faults,
    generic_before_motion,
    generic_on_steps,
    generic_get_status
};
