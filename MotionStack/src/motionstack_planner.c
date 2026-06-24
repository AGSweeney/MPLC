/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_planner.h"
#include <math.h>
#include <string.h>

typedef struct {
    int32_t entry_dir_x;
    int32_t entry_dir_y;
    int32_t exit_dir_x;
    int32_t exit_dir_y;
    uint32_t length_steps;
    uint32_t junction_entry_cap;
} mplc_ms_planner_meta_t;

static void emit_axis_steps(mplc_ms_planner_t *planner, uint8_t axis, int32_t delta)
{
    if (!planner || delta == 0) {
        return;
    }
    if (planner->port && planner->port->set_direction) {
        planner->port->set_direction(axis, delta < 0, planner->port_ctx);
    }
    if (planner->port && planner->port->emit_steps) {
        planner->port->emit_steps(axis, delta, planner->port_ctx);
    }
}

static int32_t arc_span_qx_helper(double start_a, double end_a, bool clockwise)
{
    double span;
    if (start_a < 0.0) {
        start_a += 2.0 * 3.14159265358979323846;
    }
    if (end_a < 0.0) {
        end_a += 2.0 * 3.14159265358979323846;
    }
    if (clockwise) {
        span = start_a - end_a;
        if (span <= 0.0) {
            span += 2.0 * 3.14159265358979323846;
        }
    } else {
        span = end_a - start_a;
        if (span <= 0.0) {
            span += 2.0 * 3.14159265358979323846;
        }
    }
    return (int32_t)((span / (2.0 * 3.14159265358979323846)) * (double)32768.0 + 0.5);
}

static void segment_dirs(const mplc_ms_segment_t *seg,
                         int32_t *entry_x, int32_t *entry_y,
                         int32_t *exit_x, int32_t *exit_y,
                         uint32_t *length_out)
{
    int32_t dx;
    int32_t dy;
    uint32_t len;

    if (!seg || !entry_x || !entry_y || !exit_x || !exit_y || !length_out) {
        return;
    }

    if (seg->type == MPLC_MS_SEGMENT_LINEAR) {
        dx = seg->end_x - seg->start_x;
        dy = seg->end_y - seg->start_y;
        len = mplc_ms_interp_distance(seg->start_x, seg->start_y, seg->end_x, seg->end_y);
        if (len == 0U) {
            *entry_x = 0;
            *entry_y = 0;
            *exit_x = 0;
            *exit_y = 0;
            *length_out = 0U;
            return;
        }
        *entry_x = (int32_t)(((int64_t)dx << 15) / (int64_t)len);
        *entry_y = (int32_t)(((int64_t)dy << 15) / (int64_t)len);
        *exit_x = *entry_x;
        *exit_y = *entry_y;
        *length_out = len;
        return;
    }

    if (seg->type == MPLC_MS_SEGMENT_ARC && seg->radius > 0) {
        double start_a;
        double end_a;
        double tx;
        double ty;

        start_a = atan2((double)(seg->start_y - seg->center_y),
                        (double)(seg->start_x - seg->center_x));
        end_a = atan2((double)(seg->end_y - seg->center_y),
                      (double)(seg->end_x - seg->center_x));
        len = (uint32_t)seg->radius;
        if (seg->clockwise) {
            tx = sin(start_a);
            ty = -cos(start_a);
        } else {
            tx = -sin(start_a);
            ty = cos(start_a);
        }
        *entry_x = (int32_t)(tx * 32768.0);
        *entry_y = (int32_t)(ty * 32768.0);
        if (seg->clockwise) {
            tx = sin(end_a);
            ty = -cos(end_a);
        } else {
            tx = -sin(end_a);
            ty = cos(end_a);
        }
        *exit_x = (int32_t)(tx * 32768.0);
        *exit_y = (int32_t)(ty * 32768.0);
        {
            int32_t span = arc_span_qx_helper(start_a, end_a, seg->clockwise);
            *length_out = (uint32_t)(((uint64_t)(uint32_t)seg->radius * (uint64_t)(uint32_t)span) >> 15U);
        }
        return;
    }

    *entry_x = 0;
    *entry_y = 0;
    *exit_x = 0;
    *exit_y = 0;
    *length_out = 0U;
}

static int start_segment(mplc_ms_planner_t *planner, const mplc_ms_segment_t *seg)
{
    int rc = 0;

    if (!planner || !seg) {
        return -1;
    }

    planner->active_type = seg->type;
    if (seg->type == MPLC_MS_SEGMENT_LINEAR) {
        rc = mplc_ms_linear_interp_begin(&planner->linear,
                                         seg->start_x, seg->start_y,
                                         seg->end_x, seg->end_y,
                                         seg->nominal_speed ? seg->nominal_speed : planner->vel_max,
                                         planner->accel_max,
                                         planner->sample_rate_hz,
                                         seg->entry_speed,
                                         seg->exit_speed);
        planner->pos_x = seg->start_x;
        planner->pos_y = seg->start_y;
    } else if (seg->type == MPLC_MS_SEGMENT_ARC) {
        rc = mplc_ms_arc_interp_begin(&planner->arc,
                                      seg->center_x, seg->center_y, seg->radius,
                                      seg->start_x, seg->start_y,
                                      seg->end_x, seg->end_y,
                                      seg->clockwise,
                                      seg->nominal_speed ? seg->nominal_speed : planner->vel_max,
                                      planner->accel_max,
                                      planner->sample_rate_hz,
                                      seg->entry_speed,
                                      seg->exit_speed);
        planner->pos_x = seg->start_x;
        planner->pos_y = seg->start_y;
    } else {
        return -2;
    }

    planner->driving = (rc == 0);
    return rc;
}

void mplc_ms_planner_init(mplc_ms_planner_t *planner, uint16_t sample_rate_hz)
{
    if (!planner) {
        return;
    }
    memset(planner, 0, sizeof(*planner));
    planner->sample_rate_hz = sample_rate_hz ? sample_rate_hz : MPLC_MOTIONSTACK_DEFAULT_TICK_HZ;
    planner->junction_deviation_steps = MPLC_MS_DEFAULT_JUNCTION_DEVIATION_STEPS;
    planner->stop_at_queue_end = true;
    planner->use_grbl_planner = true;
    mplc_ms_linear_interp_init(&planner->linear);
    mplc_ms_arc_interp_init(&planner->arc);
}

void mplc_ms_planner_set_port(mplc_ms_planner_t *planner,
                              const mplc_motionstack_port_t *port, void *ctx)
{
    if (!planner) {
        return;
    }
    planner->port = port;
    planner->port_ctx = ctx;
}

void mplc_ms_planner_set_limits(mplc_ms_planner_t *planner,
                                uint32_t vel_max, uint32_t accel_max)
{
    if (!planner) {
        return;
    }
    planner->vel_max = vel_max;
    planner->accel_max = accel_max;
}

void mplc_ms_planner_set_grbl_mode(mplc_ms_planner_t *planner, bool enabled)
{
    if (planner) {
        planner->use_grbl_planner = enabled;
    }
}

void mplc_ms_planner_set_junction_deviation(mplc_ms_planner_t *planner,
                                            uint32_t deviation_steps)
{
    if (planner) {
        planner->junction_deviation_steps = deviation_steps ? deviation_steps : 1U;
    }
}

void mplc_ms_planner_set_junction_dv_max(mplc_ms_planner_t *planner, uint32_t dv_max)
{
    if (planner) {
        planner->junction_dv_max = dv_max;
    }
}

void mplc_ms_planner_set_stop_at_queue_end(mplc_ms_planner_t *planner, bool stop)
{
    if (planner) {
        planner->stop_at_queue_end = stop;
    }
}

static int queue_push(mplc_ms_planner_t *planner, const mplc_ms_segment_t *seg)
{
    if (!planner || !seg) {
        return -1;
    }
    if (planner->count >= MPLC_MOTIONSTACK_QUEUE_DEPTH) {
        return -2;
    }
    planner->queue[planner->tail] = *seg;
    planner->tail = (uint8_t)((planner->tail + 1U) % MPLC_MOTIONSTACK_QUEUE_DEPTH);
    planner->count++;
    if (planner->use_grbl_planner) {
        mplc_ms_planner_recalculate(planner);
    }
    return 0;
}

int mplc_ms_planner_queue_linear(mplc_ms_planner_t *planner, const mplc_ms_segment_t *seg)
{
    mplc_ms_segment_t copy;

    if (!planner || !seg) {
        return -1;
    }
    copy = *seg;
    copy.type = MPLC_MS_SEGMENT_LINEAR;
    if (copy.nominal_speed == 0U) {
        copy.nominal_speed = planner->vel_max;
    }
    return queue_push(planner, &copy);
}

int mplc_ms_planner_queue_arc(mplc_ms_planner_t *planner, const mplc_ms_segment_t *seg)
{
    mplc_ms_segment_t copy;

    if (!planner || !seg) {
        return -1;
    }
    copy = *seg;
    copy.type = MPLC_MS_SEGMENT_ARC;
    if (copy.nominal_speed == 0U) {
        copy.nominal_speed = planner->vel_max;
    }
    return queue_push(planner, &copy);
}

void mplc_ms_planner_set_defer_start(mplc_ms_planner_t *planner, bool defer)
{
    if (planner) {
        planner->defer_start = defer;
    }
}

int mplc_ms_planner_start_deferred(mplc_ms_planner_t *planner)
{
    if (!planner) {
        return -1;
    }
    planner->defer_start = false;
    if (!planner->driving && planner->count > 0U) {
        return start_segment(planner, &planner->queue[planner->head]);
    }
    return 0;
}

void mplc_ms_planner_recalculate(mplc_ms_planner_t *planner)
{
    mplc_ms_planner_meta_t meta[MPLC_MOTIONSTACK_QUEUE_DEPTH];
    uint8_t order[MPLC_MOTIONSTACK_QUEUE_DEPTH];
    uint8_t idx;
    uint8_t count;
    uint8_t i;

    if (!planner || !planner->use_grbl_planner || planner->count == 0U) {
        return;
    }

    count = planner->count;
    idx = planner->head;
    for (i = 0U; i < count; i++) {
        order[i] = idx;
        segment_dirs(&planner->queue[idx],
                     &meta[i].entry_dir_x, &meta[i].entry_dir_y,
                     &meta[i].exit_dir_x, &meta[i].exit_dir_y,
                     &meta[i].length_steps);
        idx = (uint8_t)((idx + 1U) % MPLC_MOTIONSTACK_QUEUE_DEPTH);
    }

    for (i = 0U; i < count; i++) {
        mplc_ms_segment_t *cur = &planner->queue[order[i]];
        uint64_t nominal_sqr = (uint64_t)cur->nominal_speed * (uint64_t)cur->nominal_speed;
        uint64_t max_entry_sqr = nominal_sqr;

        if (i == 0U) {
            max_entry_sqr = 0ULL;
        } else {
            mplc_ms_segment_t *prev = &planner->queue[order[i - 1U]];
            int32_t d_dir_x = meta[i].entry_dir_x - meta[i - 1U].exit_dir_x;
            int32_t d_dir_y = meta[i].entry_dir_y - meta[i - 1U].exit_dir_y;
            uint64_t vmax_sqr = nominal_sqr;
            uint64_t prev_nominal_sqr = (uint64_t)prev->nominal_speed * (uint64_t)prev->nominal_speed;

            if (planner->junction_dv_max > 0U) {
                int32_t max_delta = d_dir_x < 0 ? -d_dir_x : d_dir_x;
                int32_t ady = d_dir_y < 0 ? -d_dir_y : d_dir_y;
                uint32_t v_junction;
                if (ady > max_delta) {
                    max_delta = ady;
                }
                if (max_delta < 2) {
                    v_junction = prev->nominal_speed < cur->nominal_speed ?
                        prev->nominal_speed : cur->nominal_speed;
                } else {
                    v_junction = planner->junction_dv_max / (uint32_t)max_delta;
                }
                vmax_sqr = (uint64_t)v_junction * (uint64_t)v_junction;
            } else {
                int64_t cos_theta = ((int64_t)meta[i - 1U].exit_dir_x * (int64_t)meta[i].entry_dir_x +
                                     (int64_t)meta[i - 1U].exit_dir_y * (int64_t)meta[i].entry_dir_y) >> 15;
                double sin_half;
                uint64_t prev_nominal_sqr = (uint64_t)prev->nominal_speed * (uint64_t)prev->nominal_speed;

                if (cos_theta > 32767) {
                    cos_theta = 32767;
                }
                if (cos_theta < -32768) {
                    cos_theta = -32768;
                }
                sin_half = sqrt(0.5 * (1.0 - ((double)cos_theta / 32768.0)));
                if (cos_theta > 30000 || sin_half < 1e-6) {
                    vmax_sqr = prev_nominal_sqr < nominal_sqr ? prev_nominal_sqr : nominal_sqr;
                } else if (sin_half > 0.999) {
                    vmax_sqr = (uint64_t)planner->accel_max * (uint64_t)planner->junction_deviation_steps / 2ULL;
                } else {
                    double denom = 1.0 - sin_half;
                    if (denom < 1e-6) {
                        denom = 1e-6;
                    }
                    vmax_sqr = (uint64_t)(((double)planner->accel_max *
                                           (double)planner->junction_deviation_steps * sin_half) / denom);
                }
            }

            if (vmax_sqr > prev_nominal_sqr) {
                vmax_sqr = prev_nominal_sqr;
            }
            if (vmax_sqr > nominal_sqr) {
                vmax_sqr = nominal_sqr;
            }
            max_entry_sqr = vmax_sqr;
        }

        if (meta[i].length_steps == 0U) {
            cur->entry_speed = 0U;
        } else {
            if (max_entry_sqr > nominal_sqr) {
                max_entry_sqr = nominal_sqr;
            }
            cur->entry_speed = mplc_ms_u64_sqrt(max_entry_sqr);
        }
        meta[i].junction_entry_cap = cur->entry_speed;
    }

    for (i = 0U; i < count; i++) {
        mplc_ms_segment_t *cur = &planner->queue[order[i]];
        if (i + 1U < count) {
            cur->exit_speed = planner->queue[order[i + 1U]].entry_speed;
        } else {
            cur->exit_speed = planner->stop_at_queue_end ? 0U : cur->entry_speed;
        }
    }

    if (count > 0U) {
        mplc_ms_segment_t *last = &planner->queue[order[count - 1U]];
        if (meta[count - 1U].length_steps > 0U && planner->accel_max > 0U) {
            uint64_t exit_sqr = (uint64_t)last->exit_speed * (uint64_t)last->exit_speed;
            uint64_t max_entry_sqr = exit_sqr + 2ULL * (uint64_t)planner->accel_max *
                                     (uint64_t)meta[count - 1U].length_steps;
            uint64_t cur_entry_sqr = (uint64_t)last->entry_speed * (uint64_t)last->entry_speed;
            if (cur_entry_sqr > max_entry_sqr) {
                last->entry_speed = mplc_ms_u64_sqrt(max_entry_sqr);
            }
        }
        if (last->entry_speed > last->nominal_speed) {
            last->entry_speed = last->nominal_speed;
        }
    }

    {
        int bi;
        for (bi = (int)count - 2; bi >= 0; bi--) {
        mplc_ms_segment_t *cur = &planner->queue[order[(uint8_t)bi]];
        mplc_ms_segment_t *next = &planner->queue[order[(uint8_t)bi + 1]];
        uint64_t next_entry_sqr = (uint64_t)next->entry_speed * (uint64_t)next->entry_speed;
        uint64_t max_entry_sqr;
        uint64_t cur_entry_sqr;
        uint64_t nominal_sqr;

        if (meta[i].length_steps > 0U && planner->accel_max > 0U) {
            max_entry_sqr = next_entry_sqr + 2ULL * (uint64_t)planner->accel_max *
                            (uint64_t)meta[(uint8_t)i].length_steps;
        } else {
            max_entry_sqr = next_entry_sqr;
        }
        cur_entry_sqr = (uint64_t)cur->entry_speed * (uint64_t)cur->entry_speed;
        nominal_sqr = (uint64_t)cur->nominal_speed * (uint64_t)cur->nominal_speed;
        if (max_entry_sqr > cur_entry_sqr) {
            max_entry_sqr = cur_entry_sqr;
        }
        if (max_entry_sqr > nominal_sqr) {
            max_entry_sqr = nominal_sqr;
        }
        cur->entry_speed = mplc_ms_u64_sqrt(max_entry_sqr);
        cur->exit_speed = next->entry_speed;
        }
    }

    for (i = 1U; i < count; i++) {
        mplc_ms_segment_t *prev = &planner->queue[order[i - 1U]];
        mplc_ms_segment_t *cur = &planner->queue[order[i]];
        uint64_t prev_entry_sqr = (uint64_t)prev->entry_speed * (uint64_t)prev->entry_speed;
        uint64_t cur_entry_sqr = (uint64_t)cur->entry_speed * (uint64_t)cur->entry_speed;
        uint64_t nominal_sqr = (uint64_t)cur->nominal_speed * (uint64_t)cur->nominal_speed;
        uint64_t max_from_prev;

        if (meta[i - 1U].length_steps > 0U && planner->accel_max > 0U) {
            max_from_prev = prev_entry_sqr + 2ULL * (uint64_t)planner->accel_max *
                            (uint64_t)meta[i - 1U].length_steps;
            if (cur_entry_sqr < max_from_prev) {
                cur_entry_sqr = max_from_prev < nominal_sqr ? max_from_prev : nominal_sqr;
                if (cur_entry_sqr > (uint64_t)meta[i].junction_entry_cap *
                    (uint64_t)meta[i].junction_entry_cap) {
                    cur_entry_sqr = (uint64_t)meta[i].junction_entry_cap *
                                      (uint64_t)meta[i].junction_entry_cap;
                }
                cur->entry_speed = mplc_ms_u64_sqrt(cur_entry_sqr);
                prev->exit_speed = cur->entry_speed;
            } else if (cur_entry_sqr > max_from_prev) {
                cur->entry_speed = mplc_ms_u64_sqrt(max_from_prev);
                prev->exit_speed = cur->entry_speed;
            }
        }
    }
}

static void pop_head(mplc_ms_planner_t *planner)
{
    if (!planner || planner->count == 0U) {
        return;
    }
    planner->head = (uint8_t)((planner->head + 1U) % MPLC_MOTIONSTACK_QUEUE_DEPTH);
    planner->count--;
}

int mplc_ms_planner_tick(mplc_ms_planner_t *planner)
{
    int32_t sx = 0;
    int32_t sy = 0;
    bool cont;

    if (!planner) {
        return -1;
    }

    if (!planner->driving) {
        if (planner->defer_start || planner->count == 0U) {
            return 0;
        }
        if (start_segment(planner, &planner->queue[planner->head]) != 0) {
            pop_head(planner);
            return 0;
        }
    }

    if (planner->active_type == MPLC_MS_SEGMENT_LINEAR) {
        cont = mplc_ms_linear_interp_next(&planner->linear, &sx, &sy);
    } else if (planner->active_type == MPLC_MS_SEGMENT_ARC) {
        cont = mplc_ms_arc_interp_next(&planner->arc, &sx, &sy);
    } else {
        planner->driving = false;
        return 0;
    }

    planner->pos_x += sx;
    planner->pos_y += sy;
    emit_axis_steps(planner, MPLC_MOTIONSTACK_AXIS_X, sx);
    emit_axis_steps(planner, MPLC_MOTIONSTACK_AXIS_Y, sy);

    if (!cont) {
        if (planner->active_type == MPLC_MS_SEGMENT_LINEAR) {
            mplc_ms_segment_t *seg = &planner->queue[planner->head];
            planner->pos_x = seg->end_x;
            planner->pos_y = seg->end_y;
        } else if (planner->active_type == MPLC_MS_SEGMENT_ARC) {
            mplc_ms_segment_t *seg = &planner->queue[planner->head];
            planner->pos_x = seg->end_x;
            planner->pos_y = seg->end_y;
        }
        pop_head(planner);
        planner->driving = false;
        if (planner->count > 0U && !planner->defer_start) {
            (void)start_segment(planner, &planner->queue[planner->head]);
        }
    }
    return 0;
}

bool mplc_ms_planner_is_busy(const mplc_ms_planner_t *planner)
{
    return planner ? (planner->driving || planner->count > 0U) : false;
}

uint8_t mplc_ms_planner_queue_count(const mplc_ms_planner_t *planner)
{
    return planner ? planner->count : 0U;
}
