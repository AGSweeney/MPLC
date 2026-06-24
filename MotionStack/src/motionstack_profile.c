/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_profile.h"
#include "mplc_motionstack_q15.h"
#include <stdlib.h>

static int32_t to_q15_per_sec(uint32_t steps_per_sec, uint16_t sample_rate_hz)
{
    if (sample_rate_hz == 0U) {
        return 0;
    }
    return (int32_t)(((int64_t)steps_per_sec << MPLC_MS_FRACT_BITS) / sample_rate_hz);
}

static int32_t abs32(int32_t v)
{
    return v < 0 ? -v : v;
}

void mplc_ms_profile_init(mplc_ms_profile_t *profile)
{
    if (!profile) {
        return;
    }
    profile->state = MPLC_MS_PROFILE_IDLE;
    profile->position_steps = 0;
    profile->target_steps = 0;
    profile->velocity_q15 = 0;
    profile->vel_max_q15 = 0;
    profile->accel_q15 = 0;
    profile->decel_q15 = 0;
    profile->steps_remaining = 0;
    profile->active = false;
}

void mplc_ms_profile_set_limits(mplc_ms_profile_t *profile, uint32_t vel_steps_per_sec,
                                uint32_t accel_steps_per_sec, uint32_t decel_steps_per_sec,
                                uint16_t sample_rate_hz)
{
    if (!profile) {
        return;
    }
    profile->vel_max_q15 = to_q15_per_sec(vel_steps_per_sec, sample_rate_hz);
    profile->accel_q15 = to_q15_per_sec(accel_steps_per_sec, sample_rate_hz);
    profile->decel_q15 = to_q15_per_sec(decel_steps_per_sec, sample_rate_hz);
    if (profile->accel_q15 <= 0) {
        profile->accel_q15 = 1;
    }
    if (profile->decel_q15 <= 0) {
        profile->decel_q15 = 1;
    }
}

int mplc_ms_profile_move_absolute(mplc_ms_profile_t *profile, int32_t target_steps)
{
    if (!profile) {
        return -1;
    }
    profile->target_steps = target_steps;
    profile->steps_remaining = target_steps - profile->position_steps;
    profile->velocity_q15 = 0;
    profile->state = MPLC_MS_PROFILE_ACCEL;
    profile->active = true;
    return 0;
}

int mplc_ms_profile_move_relative(mplc_ms_profile_t *profile, int32_t delta_steps)
{
    if (!profile) {
        return -1;
    }
    return mplc_ms_profile_move_absolute(profile, profile->position_steps + delta_steps);
}

int mplc_ms_profile_move_velocity(mplc_ms_profile_t *profile, int32_t vel_steps_per_sec)
{
    if (!profile) {
        return -1;
    }
    profile->velocity_q15 = vel_steps_per_sec << MPLC_MS_FRACT_BITS;
    profile->state = MPLC_MS_PROFILE_VEL;
    profile->active = true;
    return 0;
}

int mplc_ms_profile_stop(mplc_ms_profile_t *profile)
{
    if (!profile) {
        return -1;
    }
    profile->state = MPLC_MS_PROFILE_STOPPING;
    profile->active = true;
    return 0;
}

int mplc_ms_profile_halt(mplc_ms_profile_t *profile)
{
    return mplc_ms_profile_stop(profile);
}

int mplc_ms_profile_steps_for_tick(mplc_ms_profile_t *profile, int32_t *delta_steps)
{
    int32_t delta = 0;
    int32_t dir = 1;

    if (!profile || !delta_steps) {
        return -1;
    }
    if (!profile->active) {
        *delta_steps = 0;
        return 0;
    }

    switch (profile->state) {
    case MPLC_MS_PROFILE_ACCEL:
        profile->velocity_q15 += profile->accel_q15;
        if (profile->velocity_q15 >= profile->vel_max_q15) {
            profile->velocity_q15 = profile->vel_max_q15;
            profile->state = MPLC_MS_PROFILE_CRUISE;
        }
        break;
    case MPLC_MS_PROFILE_CRUISE:
        if (abs32(profile->steps_remaining) <= abs32(profile->velocity_q15)) {
            profile->state = MPLC_MS_PROFILE_DECEL;
        }
        break;
    case MPLC_MS_PROFILE_DECEL:
        profile->velocity_q15 -= profile->decel_q15;
        if (profile->velocity_q15 <= 0) {
            profile->velocity_q15 = 0;
        }
        break;
    case MPLC_MS_PROFILE_VEL:
        delta = profile->velocity_q15 >> MPLC_MS_FRACT_BITS;
        profile->position_steps += delta;
        *delta_steps = delta;
        return 0;
    case MPLC_MS_PROFILE_STOPPING:
        profile->velocity_q15 -= profile->decel_q15;
        if (profile->velocity_q15 <= 0) {
            profile->velocity_q15 = 0;
            profile->active = false;
            profile->state = MPLC_MS_PROFILE_IDLE;
        }
        break;
    default:
        *delta_steps = 0;
        return 0;
    }

    if (profile->steps_remaining < 0) {
        dir = -1;
    }
    delta = (profile->velocity_q15 >> MPLC_MS_FRACT_BITS) * dir;
    if (delta == 0 && profile->velocity_q15 > 0) {
        delta = dir;
    }
    if (delta == 0 && profile->steps_remaining != 0) {
        delta = (profile->steps_remaining > 0) ? 1 : -1;
    }
    if (dir > 0 && delta > profile->steps_remaining) {
        delta = profile->steps_remaining;
    } else if (dir < 0 && delta < profile->steps_remaining) {
        delta = profile->steps_remaining;
    }
    profile->position_steps += delta;
    profile->steps_remaining -= delta;
    if (profile->steps_remaining == 0 && profile->state != MPLC_MS_PROFILE_VEL) {
        profile->active = false;
        profile->state = MPLC_MS_PROFILE_IDLE;
        profile->velocity_q15 = 0;
    }
    *delta_steps = delta;
    return 0;
}

bool mplc_ms_profile_is_complete(const mplc_ms_profile_t *profile)
{
    return profile ? (!profile->active) : true;
}
