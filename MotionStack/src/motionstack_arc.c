/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_interp.h"
#include "mplc_motionstack_q15.h"
#include <math.h>
#include <string.h>

#define MPLC_MS_TRIG_LUT_SIZE 256

static const int16_t mplc_ms_sin_lut[MPLC_MS_TRIG_LUT_SIZE] = {
    0, 402, 804, 1206, 1608, 2009, 2410, 2811, 3212, 3612, 4011, 4410, 4808, 5205, 5602, 5998,
    6393, 6786, 7179, 7571, 7962, 8351, 8739, 9126, 9512, 9896, 10278, 10659, 11039, 11417, 11793, 12167,
    12539, 12910, 13279, 13645, 14010, 14372, 14732, 15090, 15446, 15800, 16151, 16500, 16846, 17189, 17530, 17869,
    18204, 18537, 18868, 19195, 19519, 19841, 20159, 20475, 20787, 21096, 21403, 21706, 22005, 22301, 22594, 22884,
    23170, 23452, 23731, 24007, 24279, 24547, 24811, 25072, 25329, 25582, 25832, 26077, 26319, 26556, 26790, 27019,
    27245, 27466, 27683, 27896, 28105, 28310, 28510, 28706, 28898, 29085, 29268, 29447, 29621, 29791, 29956, 30117,
    30273, 30424, 30571, 30714, 30852, 30985, 31113, 31237, 31356, 31470, 31580, 31685, 31785, 31880, 31971, 32057,
    32137, 32213, 32285, 32351, 32412, 32469, 32521, 32567, 32609, 32646, 32678, 32705, 32728, 32745, 32757, 32765,
    32767, 32765, 32757, 32745, 32728, 32705, 32678, 32646, 32609, 32567, 32521, 32469, 32412, 32351, 32285, 32213,
    32137, 32057, 31971, 31880, 31785, 31685, 31580, 31470, 31356, 31237, 31113, 30985, 30852, 30714, 30571, 30424,
    30273, 30117, 29956, 29791, 29621, 29447, 29268, 29085, 28898, 28706, 28510, 28310, 28105, 27896, 27683, 27466,
    27245, 27019, 26790, 26556, 26319, 26077, 25832, 25582, 25329, 25072, 24811, 24547, 24279, 24007, 23731, 23452,
    23170, 22884, 22594, 22301, 22005, 21706, 21403, 21096, 20787, 20475, 20159, 19841, 19519, 19195, 18868, 18537,
    18204, 17869, 17530, 17189, 16846, 16500, 16151, 15800, 15446, 15090, 14732, 14372, 14010, 13645, 13279, 12910,
    12539, 12167, 11793, 11417, 11039, 10659, 10278, 9896, 9512, 9126, 8739, 8351, 7962, 7571, 7179, 6786,
    6393, 5998, 5602, 5205, 4808, 4410, 4011, 3612, 3212, 2811, 2410, 2009, 1608, 1206, 804, 402
};

static int32_t abs32(int32_t v)
{
    return v < 0 ? -v : v;
}

static int32_t sin_q15(int32_t angle_qx)
{
    int32_t idx = angle_qx / (MPLC_MS_TWO_PI_QX / MPLC_MS_TRIG_LUT_SIZE);
    while (idx < 0) {
        idx += MPLC_MS_TRIG_LUT_SIZE;
    }
    idx %= MPLC_MS_TRIG_LUT_SIZE;
    return (int32_t)mplc_ms_sin_lut[idx];
}

static int32_t cos_q15(int32_t angle_qx)
{
    return sin_q15(angle_qx + (MPLC_MS_TWO_PI_QX / 4));
}

static int32_t normalize_angle_qx(int32_t angle_qx)
{
    while (angle_qx < 0) {
        angle_qx += MPLC_MS_TWO_PI_QX;
    }
    while (angle_qx >= MPLC_MS_TWO_PI_QX) {
        angle_qx -= MPLC_MS_TWO_PI_QX;
    }
    return angle_qx;
}

static int32_t angle_from_point(int32_t cx, int32_t cy, int32_t x, int32_t y)
{
    double angle_rad;
    int32_t dx = x - cx;
    int32_t dy = y - cy;

    if (dx == 0 && dy == 0) {
        return 0;
    }
    angle_rad = atan2((double)dy, (double)dx);
    if (angle_rad < 0.0) {
        angle_rad += 2.0 * 3.14159265358979323846;
    }
    return (int32_t)((angle_rad / (2.0 * 3.14159265358979323846)) *
                     (double)MPLC_MS_TWO_PI_QX + 0.5);
}

static int32_t arc_span_qx(int32_t start_qx, int32_t end_qx, bool clockwise)
{
    start_qx = normalize_angle_qx(start_qx);
    end_qx = normalize_angle_qx(end_qx);
    if (clockwise) {
        if (end_qx < start_qx) {
            return MPLC_MS_TWO_PI_QX - (start_qx - end_qx);
        }
        return MPLC_MS_TWO_PI_QX - (end_qx - start_qx);
    }
    if (end_qx > start_qx) {
        return end_qx - start_qx;
    }
    return MPLC_MS_TWO_PI_QX - (start_qx - end_qx);
}

static uint32_t speed_from_dist(uint32_t v0, uint32_t accel, uint32_t dist)
{
    uint64_t v2 = (uint64_t)v0 * (uint64_t)v0 + 2ULL * (uint64_t)accel * (uint64_t)dist;
    return mplc_ms_u64_sqrt(v2);
}


void mplc_ms_arc_interp_init(mplc_ms_arc_interp_t *interp)
{
    if (!interp) {
        return;
    }
    memset(interp, 0, sizeof(*interp));
}

int mplc_ms_arc_interp_begin(mplc_ms_arc_interp_t *interp,
                             int32_t center_x, int32_t center_y, int32_t radius,
                             int32_t start_x, int32_t start_y,
                             int32_t end_x, int32_t end_y,
                             bool clockwise, uint32_t velocity_max,
                             uint32_t accel_max, uint16_t sample_rate_hz,
                             uint32_t entry_speed, uint32_t exit_speed)
{
    int32_t start_angle;
    int32_t end_angle;
    int32_t span;
    uint32_t total_steps;

    if (!interp || radius <= 0 || sample_rate_hz == 0U) {
        return -1;
    }

    start_angle = angle_from_point(center_x, center_y, start_x, start_y);
    end_angle = angle_from_point(center_x, center_y, end_x, end_y);
    span = arc_span_qx(start_angle, end_angle, clockwise);
    total_steps = (uint32_t)(((int64_t)radius * (int64_t)span) >> MPLC_MS_FRACT_BITS);
    if (total_steps == 0U) {
        return -2;
    }

    interp->center_x = center_x;
    interp->center_y = center_y;
    interp->radius = radius;
    interp->start_angle_qx = start_angle;
    interp->end_angle_qx = end_angle;
    interp->current_angle_qx = start_angle;
    interp->angle_span_qx = span;
    interp->angle_inc_qx = span / (int32_t)total_steps;
    if (interp->angle_inc_qx == 0) {
        interp->angle_inc_qx = clockwise ? -1 : 1;
    } else if (clockwise) {
        interp->angle_inc_qx = -abs32(interp->angle_inc_qx);
    } else {
        interp->angle_inc_qx = abs32(interp->angle_inc_qx);
    }
    interp->last_x = start_x;
    interp->last_y = start_y;
    interp->end_x = end_x;
    interp->end_y = end_y;
    interp->clockwise = clockwise;
    interp->total_steps = total_steps;
    interp->steps_remaining = total_steps;
    interp->velocity_max = velocity_max;
    interp->accel_max = accel_max;
    interp->entry_speed = entry_speed;
    interp->exit_speed = exit_speed;
    interp->sample_rate_hz = sample_rate_hz;
    interp->active = true;
    return 0;
}

bool mplc_ms_arc_interp_next(mplc_ms_arc_interp_t *interp,
                             int32_t *steps_x, int32_t *steps_y)
{
    uint32_t dist_from_start;
    uint32_t remaining_dist;
    uint32_t target_speed;
    uint32_t min_speed;
    double angle_step;
    int32_t new_x;
    int32_t new_y;

    if (!interp || !steps_x || !steps_y) {
        return false;
    }

    if (!interp->active ||
        (interp->last_x == interp->end_x && interp->last_y == interp->end_y)) {
        interp->active = false;
        *steps_x = 0;
        *steps_y = 0;
        return false;
    }

    remaining_dist = interp->steps_remaining;
    dist_from_start = interp->total_steps - remaining_dist;

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

    angle_step = ((double)target_speed / (double)interp->sample_rate_hz) *
                 ((double)interp->angle_span_qx / (double)interp->total_steps);
    if (interp->clockwise) {
        interp->current_angle_qx -= (int32_t)(angle_step + 0.5);
    } else {
        interp->current_angle_qx += (int32_t)(angle_step + 0.5);
    }

    new_x = interp->center_x + mplc_ms_q15_mul(interp->radius, cos_q15(interp->current_angle_qx));
    new_y = interp->center_y + mplc_ms_q15_mul(interp->radius, sin_q15(interp->current_angle_qx));

    if (remaining_dist <= 1U ||
        (interp->clockwise ?
            (interp->current_angle_qx <= interp->end_angle_qx) :
            (interp->current_angle_qx >= interp->end_angle_qx))) {
        *steps_x = interp->end_x - interp->last_x;
        *steps_y = interp->end_y - interp->last_y;
        interp->last_x = interp->end_x;
        interp->last_y = interp->end_y;
        interp->steps_remaining = 0U;
        interp->active = false;
        return (*steps_x != 0 || *steps_y != 0);
    }

    *steps_x = new_x - interp->last_x;
    *steps_y = new_y - interp->last_y;
    interp->last_x = new_x;
    interp->last_y = new_y;
    interp->steps_remaining--;
    return true;
}

bool mplc_ms_arc_interp_complete(const mplc_ms_arc_interp_t *interp)
{
    return interp ? (!interp->active) : true;
}

void mplc_ms_arc_interp_set_exit_speed(mplc_ms_arc_interp_t *interp, uint32_t exit_speed)
{
    if (interp) {
        interp->exit_speed = exit_speed;
    }
}
