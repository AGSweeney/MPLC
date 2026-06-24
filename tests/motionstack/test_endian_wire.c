/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_endian.h"
#include "mplc_endian.h"
#include <stdio.h>
#include <string.h>

static int test_round_trip(void)
{
    mplc_ms_segment_t in;
    mplc_ms_segment_t out;
    uint8_t wire[MPLC_MS_SEGMENT_WIRE_SIZE];
    size_t n;

    memset(&in, 0, sizeof(in));
    in.type = MPLC_MS_SEGMENT_LINEAR;
    in.start_x = 10;
    in.start_y = -20;
    in.end_x = 1000;
    in.end_y = 2000;
    in.nominal_speed = 5000U;
    in.entry_speed = 100U;
    in.exit_speed = 200U;
    in.clockwise = true;

    n = mplc_ms_segment_wire_pack(&in, wire, sizeof(wire));
    if (n != MPLC_MS_SEGMENT_WIRE_SIZE) {
        return 1;
    }

    if (mplc_ms_segment_wire_unpack(wire, sizeof(wire), &out) != 0) {
        return 2;
    }

    if (out.type != in.type || out.start_x != in.start_x || out.start_y != in.start_y ||
        out.end_x != in.end_x || out.end_y != in.end_y ||
        out.nominal_speed != in.nominal_speed || out.entry_speed != in.entry_speed ||
        out.exit_speed != in.exit_speed || out.clockwise != in.clockwise) {
        return 3;
    }

    /* Wire bytes must be little-endian regardless of host. */
    if (wire[0] != (uint8_t)(MPLC_MS_SEGMENT_WIRE_MAGIC & 0xFFU)) {
        return 4;
    }
    return 0;
}

int main(void)
{
    int rc = test_round_trip();
    if (rc != 0) {
        printf("test_endian_wire failed: %d\n", rc);
        return rc;
    }
    printf("test_endian_wire ok\n");
    return 0;
}
