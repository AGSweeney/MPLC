/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_q15.h"
#include "mplc_motionstack_profile.h"
#include "mplc_motionstack_units.h"
#include <stdio.h>

static int test_q15_math(void)
{
    int32_t p;

    if (mplc_ms_q15_mul(16384, 16384) != 8192) {
        return 1;
    }
    if (mplc_ms_q15_div(8192, 16384) != 16384) {
        return 2;
    }
    p = mplc_ms_isqrt32(100U);
    if (p != 10) {
        return 3;
    }
    return 0;
}

static int test_profile_move(void)
{
    mplc_ms_profile_t profile;
    int32_t total = 0;
    int32_t delta;
    uint32_t i;

    mplc_ms_profile_init(&profile);
    mplc_ms_profile_set_limits(&profile, 5000U, 5000U, 5000U, 5000U);
    if (mplc_ms_profile_move_relative(&profile, 100) != 0) {
        return 10;
    }

    for (i = 0U; i < 50000U; i++) {
        if (mplc_ms_profile_steps_for_tick(&profile, &delta) != 0) {
            return 11;
        }
        total += delta;
        if (mplc_ms_profile_is_complete(&profile)) {
            break;
        }
    }

    if (profile.position_steps != 100 || total != 100) {
        return 12;
    }
    return 0;
}

static int test_units(void)
{
    mplc_ms_mechanics_t mech;
    int32_t steps;
    double distance;

    mech.steps_per_revolution = 200U;
    mech.pitch = 5.0;
    mech.pitch_unit = MPLC_MS_UNIT_MM;
    mech.gear_ratio = 1.0;
    mech.inverted = false;

    if (mplc_ms_mechanics_configure(&mech) != 0) {
        return 20;
    }
    if (mplc_ms_distance_to_steps(&mech, 5.0, MPLC_MS_UNIT_MM, &steps) != 0 || steps != 200) {
        return 21;
    }
    if (mplc_ms_steps_to_distance(&mech, 200, MPLC_MS_UNIT_MM, &distance) != 0 ||
        distance < 4.99 || distance > 5.01) {
        return 22;
    }
    return 0;
}

int main(void)
{
    int rc;

    rc = test_q15_math();
    if (rc != 0) {
        printf("test_q15 math failed: %d\n", rc);
        return rc;
    }
    rc = test_profile_move();
    if (rc != 0) {
        printf("test_q15 profile failed: %d\n", rc);
        return rc;
    }
    rc = test_units();
    if (rc != 0) {
        printf("test_q15 units failed: %d\n", rc);
        return rc;
    }
    printf("test_q15 ok\n");
    return 0;
}
