/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_units.h"

static double unit_to_mm_factor(mplc_ms_unit_t unit)
{
    switch (unit) {
    case MPLC_MS_UNIT_MM:   return 1.0;
    case MPLC_MS_UNIT_INCH: return 25.4;
    default:                return 0.0;
    }
}

static double steps_per_mm(const mplc_ms_mechanics_t *mech)
{
    double pitch_mm;

    if (!mech || mech->steps_per_revolution == 0U || mech->pitch <= 0.0 || mech->gear_ratio <= 0.0) {
        return 0.0;
    }
    pitch_mm = mech->pitch * unit_to_mm_factor(mech->pitch_unit);
    if (pitch_mm <= 0.0) {
        return 0.0;
    }
    return ((double)mech->steps_per_revolution * mech->gear_ratio) / pitch_mm;
}

int mplc_ms_mechanics_configure(mplc_ms_mechanics_t *mech)
{
    if (!mech || mech->steps_per_revolution == 0U || mech->pitch <= 0.0 || mech->gear_ratio <= 0.0) {
        return -1;
    }
    return 0;
}

int mplc_ms_distance_to_steps(const mplc_ms_mechanics_t *mech, double distance,
                              mplc_ms_unit_t unit, int32_t *steps_out)
{
    double spm;
    double mm;

    if (!mech || !steps_out) {
        return -1;
    }
    if (unit == MPLC_MS_UNIT_STEPS) {
        *steps_out = (int32_t)distance;
        return 0;
    }
    spm = steps_per_mm(mech);
    if (spm <= 0.0) {
        return -2;
    }
    mm = distance * unit_to_mm_factor(unit);
    *steps_out = (int32_t)(mm * spm);
    if (mech->inverted) {
        *steps_out = -*steps_out;
    }
    return 0;
}

int mplc_ms_steps_to_distance(const mplc_ms_mechanics_t *mech, int32_t steps,
                              mplc_ms_unit_t unit, double *distance_out)
{
    double spm;
    int32_t s = steps;

    if (!mech || !distance_out) {
        return -1;
    }
    if (unit == MPLC_MS_UNIT_STEPS) {
        *distance_out = (double)steps;
        return 0;
    }
    spm = steps_per_mm(mech);
    if (spm <= 0.0) {
        return -2;
    }
    if (mech->inverted) {
        s = -s;
    }
    *distance_out = ((double)s / spm) / unit_to_mm_factor(unit);
    return 0;
}

int mplc_ms_velocity_to_steps_per_sec(const mplc_ms_mechanics_t *mech,
                                      double velocity, mplc_ms_unit_t unit,
                                      uint32_t *steps_per_sec_out)
{
    double spm;
    double mm_per_sec;

    if (!mech || !steps_per_sec_out) {
        return -1;
    }
    if (unit == MPLC_MS_UNIT_STEPS) {
        *steps_per_sec_out = (uint32_t)(velocity > 0.0 ? velocity : 0.0);
        return 0;
    }
    spm = steps_per_mm(mech);
    if (spm <= 0.0) {
        return -2;
    }
    mm_per_sec = velocity * unit_to_mm_factor(unit);
    *steps_per_sec_out = (uint32_t)(mm_per_sec * spm);
    return 0;
}
