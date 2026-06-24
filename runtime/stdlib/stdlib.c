/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/stdlib.h"
#include "mplc/vm.h"
#include "mplc_hal.h"
#include <string.h>

typedef struct {
    bool     in_prev;
    uint64_t start_us;
    bool     q;
} mplc_fb_ton_t;

typedef struct {
    bool     in_prev;
    uint64_t start_us;
    bool     q;
} mplc_fb_tof_t;

typedef struct {
    bool     in_prev;
    uint64_t start_us;
    bool     q;
    bool     active;
} mplc_fb_tp_t;

typedef struct {
    bool  cu_prev;
    int32_t cv;
    bool  q;
} mplc_fb_ctu_t;

typedef struct {
    bool  cd_prev;
    int32_t cv;
    bool  q;
} mplc_fb_ctd_t;

typedef struct {
    bool  cu_prev;
    bool  cd_prev;
    int32_t cv;
    bool  qu;
    bool  qd;
} mplc_fb_ctud_t;

typedef struct {
    bool clk_prev;
    bool q;
} mplc_fb_trig_t;

typedef struct {
    bool s1;
    bool r;
    bool q1;
} mplc_fb_sr_t;

static uint64_t vm_time_us(mplc_vm_t *vm)
{
    (void)vm;
    return mplc_hal_time_us();
}

static void ton_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_ton_t));
}

static void ton_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_ton_t *fb = (mplc_fb_ton_t *)inst;
    bool in = params[0] != 0;
    int32_t pt_ms = params[1];
    uint64_t now = vm_time_us(vm);

    if (in && !fb->in_prev) {
        fb->start_us = now;
    }
    if (!in) {
        fb->q = false;
        fb->start_us = now;
    } else if ((now - fb->start_us) >= (uint64_t)pt_ms * 1000ULL) {
        fb->q = true;
    }
    fb->in_prev = in;
}

static void tof_init(void *inst) { memset(inst, 0, sizeof(mplc_fb_tof_t)); }

static void tof_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_tof_t *fb = (mplc_fb_tof_t *)inst;
    bool in = params[0] != 0;
    int32_t pt_ms = params[1];
    uint64_t now = vm_time_us(vm);

    if (in) {
        fb->q = true;
        fb->start_us = now;
    } else if (fb->in_prev) {
        fb->start_us = now;
    } else if ((now - fb->start_us) >= (uint64_t)pt_ms * 1000ULL) {
        fb->q = false;
    }
    fb->in_prev = in;
}

static void tp_init(void *inst) { memset(inst, 0, sizeof(mplc_fb_tp_t)); }

static void tp_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_tp_t *fb = (mplc_fb_tp_t *)inst;
    bool in = params[0] != 0;
    int32_t pt_ms = params[1];
    uint64_t now = vm_time_us(vm);

    if (in && !fb->in_prev && !fb->active) {
        fb->active = true;
        fb->start_us = now;
        fb->q = true;
    }
    if (fb->active && (now - fb->start_us) >= (uint64_t)pt_ms * 1000ULL) {
        fb->active = false;
        fb->q = false;
    }
    fb->in_prev = in;
}

static void ctu_init(void *inst) { memset(inst, 0, sizeof(mplc_fb_ctu_t)); }

static void ctu_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_ctu_t *fb = (mplc_fb_ctu_t *)inst;
    bool cu = params[0] != 0;
    bool reset = params[1] != 0;
    int32_t pv = params[2];
    (void)vm;

    if (reset) {
        fb->cv = 0;
        fb->q = false;
    } else if (cu && !fb->cu_prev) {
        fb->cv++;
        if (fb->cv >= pv) {
            fb->q = true;
        }
    }
    fb->cu_prev = cu;
}

static void ctd_init(void *inst) { memset(inst, 0, sizeof(mplc_fb_ctd_t)); }

static void ctd_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_ctd_t *fb = (mplc_fb_ctd_t *)inst;
    bool cd = params[0] != 0;
    bool load = params[1] != 0;
    int32_t pv = params[2];
    (void)vm;

    if (load) {
        fb->cv = pv;
        fb->q = false;
    } else if (cd && !fb->cd_prev) {
        fb->cv--;
        if (fb->cv <= 0) {
            fb->q = true;
        }
    }
    fb->cd_prev = cd;
}

static void ctud_init(void *inst) { memset(inst, 0, sizeof(mplc_fb_ctud_t)); }

static void ctud_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_ctud_t *fb = (mplc_fb_ctud_t *)inst;
    bool cu = params[0] != 0;
    bool cd = params[1] != 0;
    bool reset = params[2] != 0;
    bool load = params[3] != 0;
    int32_t pv = params[4];
    (void)vm;

    if (reset) {
        fb->cv = 0;
        fb->qu = false;
        fb->qd = false;
    } else if (load) {
        fb->cv = pv;
    } else {
        if (cu && !fb->cu_prev) {
            fb->cv++;
        }
        if (cd && !fb->cd_prev) {
            fb->cv--;
        }
    }
    fb->qu = fb->cv >= pv;
    fb->qd = fb->cv <= 0;
    fb->cu_prev = cu;
    fb->cd_prev = cd;
}

static void r_trig_init(void *inst) { memset(inst, 0, sizeof(mplc_fb_trig_t)); }

static void r_trig_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_trig_t *fb = (mplc_fb_trig_t *)inst;
    bool clk = params[0] != 0;
    (void)vm;
    fb->q = clk && !fb->clk_prev;
    fb->clk_prev = clk;
}

static void f_trig_init(void *inst) { memset(inst, 0, sizeof(mplc_fb_trig_t)); }

static void f_trig_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_trig_t *fb = (mplc_fb_trig_t *)inst;
    bool clk = params[0] != 0;
    (void)vm;
    fb->q = !clk && fb->clk_prev;
    fb->clk_prev = clk;
}

static void sr_init(void *inst) { memset(inst, 0, sizeof(mplc_fb_sr_t)); }

static void sr_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_sr_t *fb = (mplc_fb_sr_t *)inst;
    bool s1 = params[0] != 0;
    bool r = params[1] != 0;
    (void)vm;
    if (r) {
        fb->q1 = false;
    } else if (s1) {
        fb->q1 = true;
    }
}

static void rs_init(void *inst) { memset(inst, 0, sizeof(mplc_fb_sr_t)); }

static void rs_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_sr_t *fb = (mplc_fb_sr_t *)inst;
    bool s = params[0] != 0;
    bool r1 = params[1] != 0;
    (void)vm;
    if (r1) {
        fb->q1 = false;
    } else if (s) {
        fb->q1 = true;
    }
}

static const mplc_fb_vtable_t g_vtables[] = {
    { MPLC_FB_TON,    sizeof(mplc_fb_ton_t),  ton_init,  ton_cycle  },
    { MPLC_FB_TOF,    sizeof(mplc_fb_tof_t),  tof_init,  tof_cycle  },
    { MPLC_FB_TP,     sizeof(mplc_fb_tp_t),   tp_init,   tp_cycle   },
    { MPLC_FB_CTU,    sizeof(mplc_fb_ctu_t),  ctu_init,  ctu_cycle  },
    { MPLC_FB_CTD,    sizeof(mplc_fb_ctd_t),  ctd_init,  ctd_cycle  },
    { MPLC_FB_CTUD,   sizeof(mplc_fb_ctud_t), ctud_init, ctud_cycle },
    { MPLC_FB_R_TRIG, sizeof(mplc_fb_trig_t), r_trig_init, r_trig_cycle },
    { MPLC_FB_F_TRIG, sizeof(mplc_fb_trig_t), f_trig_init, f_trig_cycle },
    { MPLC_FB_SR,     sizeof(mplc_fb_sr_t),   sr_init,   sr_cycle   },
    { MPLC_FB_RS,     sizeof(mplc_fb_sr_t),   rs_init,   rs_cycle   },
};

const mplc_fb_vtable_t *mplc_stdlib_get_vtable(mplc_native_fb_t type)
{
    uint32_t i;
    for (i = 0; i < sizeof(g_vtables) / sizeof(g_vtables[0]); i++) {
        if (g_vtables[i].type == type) {
            return &g_vtables[i];
        }
    }
    return NULL;
}

uint32_t mplc_stdlib_instance_size(mplc_native_fb_t type)
{
    const mplc_fb_vtable_t *vt = mplc_stdlib_get_vtable(type);
    return vt ? vt->instance_size : 0U;
}
