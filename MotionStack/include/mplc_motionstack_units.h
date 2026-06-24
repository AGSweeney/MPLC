/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTIONSTACK_UNITS_H
#define MPLC_MOTIONSTACK_UNITS_H

#include "mplc_motionstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int  mplc_ms_mechanics_configure(mplc_ms_mechanics_t *mech);
int  mplc_ms_distance_to_steps(const mplc_ms_mechanics_t *mech, double distance,
                               mplc_ms_unit_t unit, int32_t *steps_out);
int  mplc_ms_steps_to_distance(const mplc_ms_mechanics_t *mech, int32_t steps,
                               mplc_ms_unit_t unit, double *distance_out);
int  mplc_ms_velocity_to_steps_per_sec(const mplc_ms_mechanics_t *mech,
                                       double velocity, mplc_ms_unit_t unit,
                                       uint32_t *steps_per_sec_out);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTIONSTACK_UNITS_H */
