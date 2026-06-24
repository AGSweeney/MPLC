/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTIONSTACK_TYPES_H
#define MPLC_MOTIONSTACK_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MPLC_MOTIONSTACK_AXIS_X       0U
#define MPLC_MOTIONSTACK_AXIS_Y       1U
#define MPLC_MOTIONSTACK_AXIS_COUNT   2U

#define MPLC_MOTIONSTACK_QUEUE_DEPTH  16U
#define MPLC_MOTIONSTACK_DEFAULT_TICK_HZ 5000U

#define MPLC_MOTIONSTACK_WIRE_MAGIC   0x4D535447U /* 'MSTG' */
#define MPLC_MS_SEGMENT_WIRE_MAGIC    MPLC_MOTIONSTACK_WIRE_MAGIC

typedef enum {
    MPLC_MS_UNIT_STEPS = 0,
    MPLC_MS_UNIT_MM    = 1,
    MPLC_MS_UNIT_INCH  = 2
} mplc_ms_unit_t;

typedef enum {
    MPLC_MS_SEGMENT_NONE    = 0,
    MPLC_MS_SEGMENT_LINEAR  = 1,
    MPLC_MS_SEGMENT_ARC     = 2
} mplc_ms_segment_type_t;

typedef struct {
    uint32_t steps_per_revolution;
    double   pitch;
    mplc_ms_unit_t pitch_unit;
    double   gear_ratio;
    bool     inverted;
} mplc_ms_mechanics_t;

typedef struct {
    mplc_ms_segment_type_t type;
    int32_t start_x;
    int32_t start_y;
    int32_t end_x;
    int32_t end_y;
    int32_t center_x;
    int32_t center_y;
    int32_t radius;
    bool    clockwise;
    uint32_t nominal_speed;
    uint32_t entry_speed;
    uint32_t exit_speed;
} mplc_ms_segment_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    int32_t  start_x;
    int32_t  start_y;
    int32_t  end_x;
    int32_t  end_y;
    int32_t  center_x;
    int32_t  center_y;
    int32_t  radius;
    uint32_t nominal_speed;
    uint32_t entry_speed;
    uint32_t exit_speed;
    uint16_t flags;
    uint16_t reserved;
} mplc_ms_segment_wire_t;

#define MPLC_MS_SEGMENT_WIRE_SIZE 56U
#define MPLC_MS_SEGMENT_WIRE_VERSION 1U

#define MPLC_MS_WIRE_FLAG_CLOCKWISE 0x0001U

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTIONSTACK_TYPES_H */
