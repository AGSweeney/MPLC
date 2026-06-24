/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTIONSTACK_PORT_SIM_H
#define MPLC_MOTIONSTACK_PORT_SIM_H

#include "mplc_motionstack_port.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mplc_ms_port_sim_ctx mplc_ms_port_sim_ctx_t;

struct mplc_ms_port_sim_ctx {
    int32_t axis_steps[2];
    bool    axis_negative[2];
    uint32_t max_steps_per_tick;
};

void mplc_ms_port_sim_init(mplc_ms_port_sim_ctx_t *ctx, uint32_t max_steps_per_tick);
const mplc_motionstack_port_t *mplc_ms_port_sim_get(void);
int32_t mplc_ms_port_sim_axis_steps(const mplc_ms_port_sim_ctx_t *ctx, uint8_t axis);
void mplc_ms_port_sim_reset(mplc_ms_port_sim_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTIONSTACK_PORT_SIM_H */
