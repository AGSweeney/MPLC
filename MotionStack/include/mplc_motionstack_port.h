/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTIONSTACK_PORT_H
#define MPLC_MOTIONSTACK_PORT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mplc_motionstack_port mplc_motionstack_port_t;

struct mplc_motionstack_port {
    void (*emit_steps)(uint8_t axis, int32_t delta, void *ctx);
    void (*set_direction)(uint8_t axis, bool negative, void *ctx);
    uint32_t (*max_steps_per_tick)(uint8_t axis, void *ctx);
    int32_t (*read_encoder_counts)(uint8_t axis, void *ctx);
};

typedef struct {
    int32_t  position;
    int32_t  velocity;
    bool     index_latched;
    bool     quadrature_error;
} mplc_ms_encoder_state_t;

void mplc_ms_encoder_init(mplc_ms_encoder_state_t *enc);
void mplc_ms_encoder_update(mplc_ms_encoder_state_t *enc, int32_t delta, uint16_t sample_rate_hz);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTIONSTACK_PORT_H */
