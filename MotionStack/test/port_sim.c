/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * In-memory step sink for host unit tests (not a platform port).
 */

#include "mplc_motionstack_port.h"
#include <string.h>

typedef struct {
    int32_t axis_steps[2];
    bool    axis_negative[2];
    uint32_t max_steps_per_tick;
} mplc_ms_port_sim_ctx_t;

static void sim_emit_steps(uint8_t axis, int32_t delta, void *ctx)
{
    mplc_ms_port_sim_ctx_t *sim = (mplc_ms_port_sim_ctx_t *)ctx;

    if (!sim || axis >= 2U) {
        return;
    }
    sim->axis_steps[axis] += delta;
}

static void sim_set_direction(uint8_t axis, bool negative, void *ctx)
{
    mplc_ms_port_sim_ctx_t *sim = (mplc_ms_port_sim_ctx_t *)ctx;

    if (!sim || axis >= 2U) {
        return;
    }
    sim->axis_negative[axis] = negative;
}

static uint32_t sim_max_steps_per_tick(uint8_t axis, void *ctx)
{
    mplc_ms_port_sim_ctx_t *sim = (mplc_ms_port_sim_ctx_t *)ctx;

    (void)axis;
    return sim ? sim->max_steps_per_tick : 1U;
}

static int32_t sim_read_encoder_counts(uint8_t axis, void *ctx)
{
    mplc_ms_port_sim_ctx_t *sim = (mplc_ms_port_sim_ctx_t *)ctx;

    if (!sim || axis >= 2U) {
        return 0;
    }
    return sim->axis_steps[axis];
}

static const mplc_motionstack_port_t mplc_ms_port_sim_vtable = {
    sim_emit_steps,
    sim_set_direction,
    sim_max_steps_per_tick,
    sim_read_encoder_counts
};

void mplc_ms_port_sim_init(mplc_ms_port_sim_ctx_t *ctx, uint32_t max_steps_per_tick)
{
    if (!ctx) {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->max_steps_per_tick = max_steps_per_tick ? max_steps_per_tick : 100U;
}

const mplc_motionstack_port_t *mplc_ms_port_sim_get(void)
{
    return &mplc_ms_port_sim_vtable;
}

int32_t mplc_ms_port_sim_axis_steps(const mplc_ms_port_sim_ctx_t *ctx, uint8_t axis)
{
    if (!ctx || axis >= 2U) {
        return 0;
    }
    return ctx->axis_steps[axis];
}

void mplc_ms_port_sim_reset(mplc_ms_port_sim_ctx_t *ctx)
{
    if (!ctx) {
        return;
    }
    ctx->axis_steps[0] = 0;
    ctx->axis_steps[1] = 0;
    ctx->axis_negative[0] = false;
    ctx->axis_negative[1] = false;
}
