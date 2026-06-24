/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack.h"
#include <string.h>

void mplc_ms_encoder_init(mplc_ms_encoder_state_t *enc)
{
    if (!enc) {
        return;
    }
    enc->position = 0;
    enc->velocity = 0;
    enc->index_latched = false;
    enc->quadrature_error = false;
}

void mplc_ms_encoder_update(mplc_ms_encoder_state_t *enc, int32_t delta, uint16_t sample_rate_hz)
{
    if (!enc || sample_rate_hz == 0U) {
        return;
    }
    enc->position += delta;
    enc->velocity = (delta * (int32_t)sample_rate_hz);
}

static void bind_drive(mplc_motionstack_t *stack, uint8_t axis)
{
    const mplc_ms_drive_ops_t *ops;
    void *ctx;

    if (!stack || axis >= MPLC_MOTIONSTACK_AXIS_COUNT) {
        return;
    }

    ctx = stack->drive_ctx[axis];
    if (stack->cfg.drive_cfg[axis].kind == MPLC_MS_DRIVE_CLEARPATH) {
        ops = &mplc_ms_drive_clearpath_ops;
    } else {
        ops = &mplc_ms_drive_generic_stepper_ops;
    }
    stack->drive_ops[axis] = *ops;
    if (stack->drive_ops[axis].init) {
        stack->drive_ops[axis].init(ctx, &stack->cfg.drive_cfg[axis]);
    }
}

int mplc_motionstack_init(mplc_motionstack_t *stack, const mplc_motionstack_config_t *cfg)
{
    uint8_t axis;

    if (!stack || !cfg) {
        return -1;
    }

    memset(stack, 0, sizeof(*stack));
    stack->cfg = *cfg;
    if (stack->cfg.sample_rate_hz == 0U) {
        stack->cfg.sample_rate_hz = MPLC_MOTIONSTACK_DEFAULT_TICK_HZ;
    }

    mplc_ms_planner_init(&stack->planner, stack->cfg.sample_rate_hz);
    mplc_ms_planner_set_limits(&stack->planner, stack->cfg.vel_max, stack->cfg.accel_max);

    for (axis = 0U; axis < MPLC_MOTIONSTACK_AXIS_COUNT; axis++) {
        mplc_ms_profile_init(&stack->axis_profile[axis]);
        mplc_ms_profile_set_limits(&stack->axis_profile[axis],
                                   stack->cfg.vel_max,
                                   stack->cfg.accel_max,
                                   stack->cfg.accel_max,
                                   stack->cfg.sample_rate_hz);
        mplc_ms_encoder_init(&stack->encoder[axis]);
        bind_drive(stack, axis);
    }

    stack->initialized = true;
    return 0;
}

void mplc_motionstack_shutdown(mplc_motionstack_t *stack)
{
    if (!stack) {
        return;
    }
    memset(stack, 0, sizeof(*stack));
}

void mplc_motionstack_set_port(mplc_motionstack_t *stack,
                               const mplc_motionstack_port_t *port, void *ctx)
{
    if (!stack) {
        return;
    }
    mplc_ms_planner_set_port(&stack->planner, port, ctx);
}

int mplc_motionstack_tick(mplc_motionstack_t *stack)
{
    uint8_t axis;

    if (!stack || !stack->initialized) {
        return -1;
    }

    for (axis = 0U; axis < MPLC_MOTIONSTACK_AXIS_COUNT; axis++) {
        mplc_ms_drive_status_t status;
        if (stack->drive_ops[axis].before_motion &&
            stack->drive_ops[axis].before_motion(stack->drive_ctx[axis], &status) != 0) {
            continue;
        }
    }

    return mplc_ms_planner_tick(&stack->planner);
}

int mplc_motionstack_queue_linear(mplc_motionstack_t *stack, const mplc_ms_segment_t *seg)
{
    if (!stack || !stack->initialized || !seg) {
        return -1;
    }
    return mplc_ms_planner_queue_linear(&stack->planner, seg);
}

int mplc_motionstack_queue_arc(mplc_motionstack_t *stack, const mplc_ms_segment_t *seg)
{
    if (!stack || !stack->initialized || !seg) {
        return -1;
    }
    return mplc_ms_planner_queue_arc(&stack->planner, seg);
}

int mplc_motionstack_start_deferred(mplc_motionstack_t *stack)
{
    if (!stack || !stack->initialized) {
        return -1;
    }
    return mplc_ms_planner_start_deferred(&stack->planner);
}

bool mplc_motionstack_is_busy(const mplc_motionstack_t *stack)
{
    return stack ? mplc_ms_planner_is_busy(&stack->planner) : false;
}
