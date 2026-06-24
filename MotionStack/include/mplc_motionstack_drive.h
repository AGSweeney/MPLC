/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTIONSTACK_DRIVE_H
#define MPLC_MOTIONSTACK_DRIVE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MPLC_MS_DRIVE_GENERIC_STEPPER = 0,
    MPLC_MS_DRIVE_CLEARPATH         = 1
} mplc_ms_drive_kind_t;

typedef enum {
    MPLC_MS_HLFB_STATIC = 0,
    MPLC_MS_HLFB_PWM,
    MPLC_MS_HLFB_BIPOLAR_PWM
} mplc_ms_hlfb_mode_t;

typedef enum {
    MPLC_MS_HLFB_DEASSERTED = 0,
    MPLC_MS_HLFB_ASSERTED,
    MPLC_MS_HLFB_MEASUREMENT
} mplc_ms_hlfb_state_t;

typedef struct {
    mplc_ms_drive_kind_t kind;
    bool                 enabled;
    bool                 direction_negative;
    int32_t              commanded_steps;
    bool                 alert_present;
    int32_t              alert_code;
} mplc_ms_drive_status_t;

typedef struct {
    mplc_ms_drive_kind_t kind;
    mplc_ms_hlfb_mode_t  hlfb_mode;
    uint32_t             hlfb_carrier_hz;
    uint32_t             step_clock_hz;
    bool                 require_hlfb_for_motion;
} mplc_ms_drive_config_t;

typedef struct mplc_ms_drive_ops mplc_ms_drive_ops_t;

struct mplc_ms_drive_ops {
    void (*init)(void *ctx, const mplc_ms_drive_config_t *cfg);
    int  (*enable)(void *ctx, bool on);
    int  (*clear_faults)(void *ctx);
    int  (*before_motion)(void *ctx, mplc_ms_drive_status_t *status);
    void (*on_steps)(void *ctx, int32_t delta);
    void (*get_status)(void *ctx, mplc_ms_drive_status_t *status);
};

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTIONSTACK_DRIVE_H */
