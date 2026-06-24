/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTIONSTACK_PROFILE_H
#define MPLC_MOTIONSTACK_PROFILE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MPLC_MS_PROFILE_IDLE = 0,
    MPLC_MS_PROFILE_ACCEL,
    MPLC_MS_PROFILE_CRUISE,
    MPLC_MS_PROFILE_DECEL,
    MPLC_MS_PROFILE_VEL,
    MPLC_MS_PROFILE_STOPPING
} mplc_ms_profile_state_t;

typedef struct {
    mplc_ms_profile_state_t state;
    int32_t                 position_steps;
    int32_t                 target_steps;
    int32_t                 velocity_q15;
    int32_t                 vel_max_q15;
    int32_t                 accel_q15;
    int32_t                 decel_q15;
    int32_t                 steps_remaining;
    bool                    active;
} mplc_ms_profile_t;

void mplc_ms_profile_init(mplc_ms_profile_t *profile);
void mplc_ms_profile_set_limits(mplc_ms_profile_t *profile, uint32_t vel_steps_per_sec,
                                 uint32_t accel_steps_per_sec, uint32_t decel_steps_per_sec,
                                 uint16_t sample_rate_hz);
int  mplc_ms_profile_move_absolute(mplc_ms_profile_t *profile, int32_t target_steps);
int  mplc_ms_profile_move_relative(mplc_ms_profile_t *profile, int32_t delta_steps);
int  mplc_ms_profile_move_velocity(mplc_ms_profile_t *profile, int32_t vel_steps_per_sec);
int  mplc_ms_profile_stop(mplc_ms_profile_t *profile);
int  mplc_ms_profile_halt(mplc_ms_profile_t *profile);
int  mplc_ms_profile_steps_for_tick(mplc_ms_profile_t *profile, int32_t *delta_steps);
bool mplc_ms_profile_is_complete(const mplc_ms_profile_t *profile);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTIONSTACK_PROFILE_H */
