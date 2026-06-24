/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTIONSTACK_ENDIAN_H
#define MPLC_MOTIONSTACK_ENDIAN_H

#include <stddef.h>
#include <stdint.h>
#include "mplc_motionstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t mplc_ms_segment_wire_pack(const mplc_ms_segment_t *seg, uint8_t *out, size_t out_size);
int    mplc_ms_segment_wire_unpack(const uint8_t *in, size_t in_size, mplc_ms_segment_t *seg);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTIONSTACK_ENDIAN_H */
