/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_endian.h"
#include "mplc_endian.h"
#include <string.h>

size_t mplc_ms_segment_wire_pack(const mplc_ms_segment_t *seg, uint8_t *out, size_t out_size)
{
    uint16_t flags = 0U;

    if (!seg || !out || out_size < MPLC_MS_SEGMENT_WIRE_SIZE) {
        return 0U;
    }

    if (seg->clockwise) {
        flags |= MPLC_MS_WIRE_FLAG_CLOCKWISE;
    }

    mplc_write_le32(out + 0, MPLC_MS_SEGMENT_WIRE_MAGIC);
    mplc_write_le16(out + 4, MPLC_MS_SEGMENT_WIRE_VERSION);
    mplc_write_le16(out + 6, (uint16_t)seg->type);
    mplc_write_le32(out + 8, (uint32_t)seg->start_x);
    mplc_write_le32(out + 12, (uint32_t)seg->start_y);
    mplc_write_le32(out + 16, (uint32_t)seg->end_x);
    mplc_write_le32(out + 20, (uint32_t)seg->end_y);
    mplc_write_le32(out + 24, (uint32_t)seg->center_x);
    mplc_write_le32(out + 28, (uint32_t)seg->center_y);
    mplc_write_le32(out + 32, (uint32_t)seg->radius);
    mplc_write_le32(out + 36, seg->nominal_speed);
    mplc_write_le32(out + 40, seg->entry_speed);
    mplc_write_le32(out + 44, seg->exit_speed);
    mplc_write_le16(out + 48, flags);
    mplc_write_le16(out + 50, 0U);
    return MPLC_MS_SEGMENT_WIRE_SIZE;
}

int mplc_ms_segment_wire_unpack(const uint8_t *in, size_t in_size, mplc_ms_segment_t *seg)
{
    uint32_t magic;
    uint16_t version;
    uint16_t flags;

    if (!in || !seg || in_size < MPLC_MS_SEGMENT_WIRE_SIZE) {
        return -1;
    }

    magic = mplc_read_le32(in + 0);
    version = mplc_read_le16(in + 4);
    if (magic != MPLC_MS_SEGMENT_WIRE_MAGIC || version != MPLC_MS_SEGMENT_WIRE_VERSION) {
        return -2;
    }

    memset(seg, 0, sizeof(*seg));
    seg->type = (mplc_ms_segment_type_t)mplc_read_le16(in + 6);
    seg->start_x = (int32_t)mplc_read_le32(in + 8);
    seg->start_y = (int32_t)mplc_read_le32(in + 12);
    seg->end_x = (int32_t)mplc_read_le32(in + 16);
    seg->end_y = (int32_t)mplc_read_le32(in + 20);
    seg->center_x = (int32_t)mplc_read_le32(in + 24);
    seg->center_y = (int32_t)mplc_read_le32(in + 28);
    seg->radius = (int32_t)mplc_read_le32(in + 32);
    seg->nominal_speed = mplc_read_le32(in + 36);
    seg->entry_speed = mplc_read_le32(in + 40);
    seg->exit_speed = mplc_read_le32(in + 44);
    flags = mplc_read_le16(in + 48);
    seg->clockwise = (flags & MPLC_MS_WIRE_FLAG_CLOCKWISE) != 0U;
    return 0;
}
