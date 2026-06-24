/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_interp.h"
#include "mplc_motionstack_q15.h"

static int32_t q15_from_double(double v)
{
    if (v >= 0.0) {
        return (int32_t)(v * (double)(1 << MPLC_MS_FRACT_BITS) + 0.5);
    }
    return (int32_t)(v * (double)(1 << MPLC_MS_FRACT_BITS) - 0.5);
}

static uint32_t speed_from_dist(uint32_t v0, uint32_t accel, uint32_t dist)
{
    uint64_t v2 = (uint64_t)v0 * (uint64_t)v0 + 2ULL * (uint64_t)accel * (uint64_t)dist;
    return mplc_ms_u64_sqrt(v2);
}

void mplc_ms_linear_interp_init(mplc_ms_linear_interp_t *interp)
{
    if (!interp) {
        return;
    }
    interp->start_x = 0;
    interp->start_y = 0;
    interp->end_x = 0;
    interp->end_y = 0;
    interp->current_x = 0;
    interp->current_y = 0;
    interp->last_x = 0;
    interp->last_y = 0;
    interp->current_x_qx = 0;
    interp->current_y_qx = 0;
    interp->step_inc_x_qx = 0;
    interp->step_inc_y_qx = 0;
    interp->dir_x_q15 = 0;
    interp->dir_y_q15 = 0;
    interp->total_steps = 0U;
    interp->steps_remaining = 0U;
    interp->velocity_max = 0U;
    interp->accel_max = 0U;
    interp->entry_speed = 0U;
    interp->exit_speed = 0U;
    interp->sample_rate_hz = 0U;
    interp->active = false;
}

int mplc_ms_linear_interp_begin(mplc_ms_linear_interp_t *interp,
                                int32_t start_x, int32_t start_y,
                                int32_t end_x, int32_t end_y,
                                uint32_t velocity_max, uint32_t accel_max,
                                uint16_t sample_rate_hz,
                                uint32_t entry_speed, uint32_t exit_speed)
{
    int32_t dx;
    int32_t dy;
    uint32_t total;

    if (!interp || sample_rate_hz == 0U) {
        return -1;
    }

    dx = end_x - start_x;
    dy = end_y - start_y;
    total = mplc_ms_interp_distance(start_x, start_y, end_x, end_y);
    if (total == 0U) {
        interp->current_x = end_x;
        interp->current_y = end_y;
        interp->last_x = end_x;
        interp->last_y = end_y;
        interp->active = false;
        return 0;
    }

    interp->start_x = start_x;
    interp->start_y = start_y;
    interp->end_x = end_x;
    interp->end_y = end_y;
    interp->current_x = start_x;
    interp->current_y = start_y;
    interp->last_x = start_x;
    interp->last_y = start_y;
    interp->current_x_qx = (int64_t)start_x << MPLC_MS_FRACT_BITS;
    interp->current_y_qx = (int64_t)start_y << MPLC_MS_FRACT_BITS;
    interp->dir_x_q15 = (int32_t)(((int64_t)dx << MPLC_MS_FRACT_BITS) / (int64_t)total);
    interp->dir_y_q15 = (int32_t)(((int64_t)dy << MPLC_MS_FRACT_BITS) / (int64_t)total);
    interp->total_steps = total;
    interp->steps_remaining = total;
    interp->velocity_max = velocity_max;
    interp->accel_max = accel_max;
    interp->entry_speed = entry_speed;
    interp->exit_speed = exit_speed;
    interp->sample_rate_hz = sample_rate_hz;
    interp->step_inc_x_qx = 0;
    interp->step_inc_y_qx = 0;
    interp->active = true;
    return 0;
}

bool mplc_ms_linear_interp_next(mplc_ms_linear_interp_t *interp,
                                int32_t *steps_x, int32_t *steps_y)
{
    uint32_t remaining_dist;
    uint32_t dist_from_start;
    uint32_t target_speed;
    uint32_t min_speed;
    double steps_per_sample;
    int32_t new_x;
    int32_t new_y;
    bool past_end_x;
    bool past_end_y;

    if (!interp || !steps_x || !steps_y) {
        return false;
    }

    if (!interp->active ||
        (interp->current_x == interp->end_x && interp->current_y == interp->end_y)) {
        interp->active = false;
        *steps_x = 0;
        *steps_y = 0;
        return false;
    }

    if (interp->steps_remaining == 0U) {
        remaining_dist = mplc_ms_interp_distance(interp->current_x, interp->current_y,
                                                 interp->end_x, interp->end_y);
        if (remaining_dist == 0U) {
            interp->current_x = interp->end_x;
            interp->current_y = interp->end_y;
            interp->active = false;
            *steps_x = 0;
            *steps_y = 0;
            return false;
        }
        interp->steps_remaining = remaining_dist;
    }

    remaining_dist = mplc_ms_interp_distance(interp->current_x, interp->current_y,
                                             interp->end_x, interp->end_y);
    dist_from_start = (interp->total_steps > remaining_dist) ?
        (interp->total_steps - remaining_dist) : 0U;

    target_speed = interp->velocity_max;
    if (interp->accel_max > 0U) {
        uint32_t from_start = speed_from_dist(interp->entry_speed, interp->accel_max, dist_from_start);
        uint32_t from_end = speed_from_dist(interp->exit_speed, interp->accel_max, remaining_dist);
        if (from_start < target_speed) {
            target_speed = from_start;
        }
        if (from_end < target_speed) {
            target_speed = from_end;
        }
    }

    min_speed = interp->velocity_max / 20U;
    if (min_speed < 1U) {
        min_speed = 1U;
    }
    if (target_speed < min_speed) {
        target_speed = min_speed;
    }

    steps_per_sample = (double)target_speed / (double)interp->sample_rate_hz;
    interp->step_inc_x_qx = q15_from_double(((double)interp->dir_x_q15 / (double)(1 << MPLC_MS_FRACT_BITS)) *
                                            steps_per_sample);
    interp->step_inc_y_qx = q15_from_double(((double)interp->dir_y_q15 / (double)(1 << MPLC_MS_FRACT_BITS)) *
                                            steps_per_sample);

    interp->current_x_qx += interp->step_inc_x_qx;
    interp->current_y_qx += interp->step_inc_y_qx;

    new_x = (int32_t)(interp->current_x_qx >> MPLC_MS_FRACT_BITS);
    new_y = (int32_t)(interp->current_y_qx >> MPLC_MS_FRACT_BITS);

    past_end_x = (interp->step_inc_x_qx > 0) ? (new_x >= interp->end_x) :
                 (interp->step_inc_x_qx < 0) ? (new_x <= interp->end_x) :
                 (new_x == interp->end_x);
    past_end_y = (interp->step_inc_y_qx > 0) ? (new_y >= interp->end_y) :
                 (interp->step_inc_y_qx < 0) ? (new_y <= interp->end_y) :
                 (new_y == interp->end_y);

    if (past_end_x && past_end_y) {
        *steps_x = interp->end_x - interp->last_x;
        *steps_y = interp->end_y - interp->last_y;
        interp->last_x = interp->end_x;
        interp->last_y = interp->end_y;
        interp->current_x = interp->end_x;
        interp->current_y = interp->end_y;
        interp->current_x_qx = (int64_t)interp->end_x << MPLC_MS_FRACT_BITS;
        interp->current_y_qx = (int64_t)interp->end_y << MPLC_MS_FRACT_BITS;
        interp->steps_remaining = 0U;
        interp->active = false;
        return (*steps_x != 0 || *steps_y != 0);
    }

    *steps_x = new_x - interp->last_x;
    *steps_y = new_y - interp->last_y;
    interp->last_x = new_x;
    interp->last_y = new_y;
    interp->current_x = new_x;
    interp->current_y = new_y;

    {
        int32_t abs_sx = *steps_x < 0 ? -*steps_x : *steps_x;
        int32_t abs_sy = *steps_y < 0 ? -*steps_y : *steps_y;
        uint32_t steps_this = (uint32_t)((abs_sx > abs_sy) ?
            (abs_sx + (abs_sy >> 1)) : (abs_sy + (abs_sx >> 1)));
        if (steps_this >= interp->steps_remaining) {
            interp->steps_remaining = 0U;
        } else {
            interp->steps_remaining -= steps_this;
        }
    }

    return true;
}

bool mplc_ms_linear_interp_complete(const mplc_ms_linear_interp_t *interp)
{
    return interp ? (!interp->active) : true;
}

void mplc_ms_linear_interp_set_exit_speed(mplc_ms_linear_interp_t *interp, uint32_t exit_speed)
{
    if (interp) {
        interp->exit_speed = exit_speed;
    }
}

uint32_t mplc_ms_interp_distance(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int64_t dx = (int64_t)x2 - (int64_t)x1;
    int64_t dy = (int64_t)y2 - (int64_t)y1;
    int64_t abs_dx = dx < 0 ? -dx : dx;
    int64_t abs_dy = dy < 0 ? -dy : dy;

    if (abs_dx > abs_dy) {
        return (uint32_t)(abs_dx + (abs_dy >> 1));
    }
    return (uint32_t)(abs_dy + (abs_dx >> 1));
}

uint32_t mplc_ms_u64_sqrt(uint64_t v)
{
    uint64_t res = 0ULL;
    uint64_t bit = 1ULL << 62;

    if (v == 0ULL) {
        return 0U;
    }
    while (bit > v) {
        bit >>= 2;
    }
    while (bit != 0ULL) {
        if (v >= res + bit) {
            v -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)res;
}
