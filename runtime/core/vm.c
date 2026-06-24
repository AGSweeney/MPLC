/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/vm.h"
#include "mplc/stdlib.h"
#include "mplc_endian.h"
#include <stdlib.h>
#include <string.h>

typedef union {
    bool     b;
    int32_t  i32;
    double   r64;
} mplc_stack_val_t;

struct mplc_vm {
    mplc_vm_config_t  cfg;
    mplc_stack_val_t  stack[MPLC_MAX_STACK_DEPTH];
    int32_t           sp;
    mplc_vm_stats_t   stats;
    uint16_t          current_pou;
};

static void push_i32(mplc_vm_t *vm, int32_t v)
{
    if (vm->sp >= (int32_t)MPLC_MAX_STACK_DEPTH) {
        return;
    }
    vm->stack[vm->sp++].i32 = v;
}

static void push_r64(mplc_vm_t *vm, double v)
{
    if (vm->sp >= (int32_t)MPLC_MAX_STACK_DEPTH) {
        return;
    }
    vm->stack[vm->sp++].r64 = v;
}

static void push_bool(mplc_vm_t *vm, bool v)
{
    if (vm->sp >= (int32_t)MPLC_MAX_STACK_DEPTH) {
        return;
    }
    vm->stack[vm->sp++].b = v;
}

static int32_t pop_i32(mplc_vm_t *vm)
{
    if (vm->sp <= 0) {
        return 0;
    }
    return vm->stack[--vm->sp].i32;
}

static double pop_r64(mplc_vm_t *vm)
{
    if (vm->sp <= 0) {
        return 0.0;
    }
    return vm->stack[--vm->sp].r64;
}

static bool pop_bool(mplc_vm_t *vm)
{
    if (vm->sp <= 0) {
        return false;
    }
    return vm->stack[--vm->sp].b;
}

static bool read_bool_at(mplc_vm_t *vm, uint32_t offset)
{
    if (!vm->cfg.global_data || offset >= vm->cfg.global_size) {
        return false;
    }
    return vm->cfg.global_data[offset] != 0U;
}

static void write_bool_at(mplc_vm_t *vm, uint32_t offset, bool v)
{
    if (!vm->cfg.global_data || offset >= vm->cfg.global_size) {
        return;
    }
    vm->cfg.global_data[offset] = v ? 1U : 0U;
}

static int32_t read_i32_at(mplc_vm_t *vm, uint32_t offset)
{
    int32_t v = 0;
    if (!vm->cfg.global_data || offset + sizeof(int32_t) > vm->cfg.global_size) {
        return 0;
    }
    memcpy(&v, vm->cfg.global_data + offset, sizeof(v));
    return v;
}

static void write_i32_at(mplc_vm_t *vm, uint32_t offset, int32_t v)
{
    if (!vm->cfg.global_data || offset + sizeof(int32_t) > vm->cfg.global_size) {
        return;
    }
    memcpy(vm->cfg.global_data + offset, &v, sizeof(v));
}

static double read_r64_at(mplc_vm_t *vm, uint32_t offset)
{
    double v = 0.0;
    if (!vm->cfg.global_data || offset + sizeof(double) > vm->cfg.global_size) {
        return 0.0;
    }
    memcpy(&v, vm->cfg.global_data + offset, sizeof(v));
    return v;
}

static void write_r64_at(mplc_vm_t *vm, uint32_t offset, double v)
{
    if (!vm->cfg.global_data || offset + sizeof(double) > vm->cfg.global_size) {
        return;
    }
    memcpy(vm->cfg.global_data + offset, &v, sizeof(v));
}

static const mplc_io_entry_t *find_io_slot(mplc_vm_t *vm, uint16_t slot)
{
    if (slot < vm->cfg.io_count) {
        return &vm->cfg.io_map[slot];
    }
    return NULL;
}

static uint16_t read_u16_pc(const uint8_t **pc)
{
    uint16_t v = mplc_read_le16(*pc);
    *pc += sizeof(uint16_t);
    return v;
}

static uint32_t read_u32_pc(const uint8_t **pc)
{
    uint32_t v = mplc_read_le32(*pc);
    *pc += sizeof(uint32_t);
    return v;
}

static int32_t read_i32_pc(const uint8_t **pc)
{
    return (int32_t)read_u32_pc(pc);
}

static double read_r64_pc(const uint8_t **pc)
{
    union {
        uint64_t u;
        double d;
    } cvt;
    cvt.u = mplc_read_le64(*pc);
    *pc += sizeof(uint64_t);
    return cvt.d;
}

static int exec_native_fb(mplc_vm_t *vm, uint16_t fb_type, int32_t instance_offset,
                          int32_t param_count)
{
    const mplc_fb_vtable_t *vt;
    void *inst;
    int32_t params[8];
    int32_t i;

    vt = mplc_fb_get_vtable((mplc_native_fb_t)fb_type);
    if (!vt || !vt->cycle) {
        return -1;
    }
    if (!vm->cfg.fb_arena || (uint32_t)instance_offset + vt->instance_size > vm->cfg.fb_arena_size) {
        return -2;
    }
    inst = vm->cfg.fb_arena + instance_offset;
    if (param_count > 8) {
        param_count = 8;
    }
    for (i = param_count - 1; i >= 0; i--) {
        params[i] = pop_i32(vm);
    }
    vt->cycle(vm, inst, params);
    return 0;
}

int mplc_vm_create(mplc_vm_t **out_vm, const mplc_vm_config_t *cfg)
{
    mplc_vm_t *vm;
    if (!out_vm || !cfg) {
        return -1;
    }
    vm = (mplc_vm_t *)calloc(1, sizeof(*vm));
    if (!vm) {
        return -2;
    }
    vm->cfg = *cfg;
    *out_vm = vm;
    return 0;
}

void mplc_vm_destroy(mplc_vm_t *vm)
{
    free(vm);
}

void mplc_vm_reset(mplc_vm_t *vm)
{
    if (!vm) {
        return;
    }
    vm->sp = 0;
    memset(&vm->stats, 0, sizeof(vm->stats));
}

void mplc_vm_get_stats(mplc_vm_t *vm, mplc_vm_stats_t *stats)
{
    if (!vm || !stats) {
        return;
    }
    *stats = vm->stats;
}

int mplc_vm_run_pou(mplc_vm_t *vm, uint16_t pou_id, uint32_t code_offset, uint32_t code_size)
{
    const uint8_t *pc;
    const uint8_t *end;
    uint32_t ip;

    if (!vm || !vm->cfg.code_base) {
        return -1;
    }

    vm->current_pou = pou_id;
    vm->sp = 0;
    pc = vm->cfg.code_base + code_offset;
    end = pc + code_size;
    ip = 0;

    while (pc < end) {
        mplc_opcode_t op = (mplc_opcode_t)*pc++;
        vm->stats.instructions_executed++;

        switch (op) {
        case MPLC_OP_NOP:
            break;

        case MPLC_OP_PUSH_BOOL: {
            bool v = *pc++ != 0U;
            push_bool(vm, v);
            break;
        }
        case MPLC_OP_PUSH_I32: {
            int32_t v = read_i32_pc(&pc);
            push_i32(vm, v);
            break;
        }
        case MPLC_OP_PUSH_R64: {
            double v = read_r64_pc(&pc);
            push_r64(vm, v);
            break;
        }

        case MPLC_OP_LOAD_VAR: {
            uint8_t type = *pc++;
            uint32_t off = read_u32_pc(&pc);
            if (type == MPLC_TYPE_BOOL) {
                push_bool(vm, read_bool_at(vm, off));
            } else if (type == MPLC_TYPE_REAL || type == MPLC_TYPE_LREAL) {
                push_r64(vm, read_r64_at(vm, off));
            } else {
                push_i32(vm, read_i32_at(vm, off));
            }
            break;
        }
        case MPLC_OP_STORE_VAR: {
            uint8_t type = *pc++;
            uint32_t off = read_u32_pc(&pc);
            if (type == MPLC_TYPE_BOOL) {
                write_bool_at(vm, off, pop_bool(vm));
            } else if (type == MPLC_TYPE_REAL || type == MPLC_TYPE_LREAL) {
                write_r64_at(vm, off, pop_r64(vm));
            } else {
                write_i32_at(vm, off, pop_i32(vm));
            }
            break;
        }
        case MPLC_OP_LOAD_IO: {
            uint16_t slot = read_u16_pc(&pc);
            const mplc_io_entry_t *entry = find_io_slot(vm, slot);
            if (!entry) {
                push_i32(vm, 0);
            } else if (entry->type == MPLC_TYPE_BOOL) {
                uint32_t off = MPLC_LE32(entry->data_offset);
                bool val = read_bool_at(vm, off);
                push_bool(vm, val);
            } else if (entry->type == MPLC_TYPE_REAL || entry->type == MPLC_TYPE_LREAL) {
                push_r64(vm, read_r64_at(vm, MPLC_LE32(entry->data_offset)));
            } else {
                push_i32(vm, read_i32_at(vm, MPLC_LE32(entry->data_offset)));
            }
            break;
        }
        case MPLC_OP_STORE_IO: {
            uint16_t slot = read_u16_pc(&pc);
            const mplc_io_entry_t *entry = find_io_slot(vm, slot);
            if (entry) {
                if (entry->type == MPLC_TYPE_BOOL) {
                    uint32_t off = MPLC_LE32(entry->data_offset);
                    bool val = pop_bool(vm);
                    write_bool_at(vm, off, val);
                } else if (entry->type == MPLC_TYPE_REAL || entry->type == MPLC_TYPE_LREAL) {
                    write_r64_at(vm, MPLC_LE32(entry->data_offset), pop_r64(vm));
                } else {
                    write_i32_at(vm, MPLC_LE32(entry->data_offset), pop_i32(vm));
                }
            }
            break;
        }

        case MPLC_OP_AND:
            push_bool(vm, pop_bool(vm) && pop_bool(vm));
            break;
        case MPLC_OP_OR:
            push_bool(vm, pop_bool(vm) || pop_bool(vm));
            break;
        case MPLC_OP_XOR:
            push_bool(vm, pop_bool(vm) ^ pop_bool(vm));
            break;
        case MPLC_OP_NOT:
            push_bool(vm, !pop_bool(vm));
            break;

        case MPLC_OP_ADD_I32:
            push_i32(vm, pop_i32(vm) + pop_i32(vm));
            break;
        case MPLC_OP_SUB_I32:
            { int32_t b = pop_i32(vm); push_i32(vm, pop_i32(vm) - b); }
            break;
        case MPLC_OP_MUL_I32:
            push_i32(vm, pop_i32(vm) * pop_i32(vm));
            break;
        case MPLC_OP_DIV_I32:
            { int32_t b = pop_i32(vm); int32_t a = pop_i32(vm); push_i32(vm, b ? a / b : 0); }
            break;
        case MPLC_OP_MOD_I32:
            { int32_t b = pop_i32(vm); int32_t a = pop_i32(vm); push_i32(vm, b ? a % b : 0); }
            break;
        case MPLC_OP_ADD_R64:
            push_r64(vm, pop_r64(vm) + pop_r64(vm));
            break;
        case MPLC_OP_SUB_R64:
            { double b = pop_r64(vm); push_r64(vm, pop_r64(vm) - b); }
            break;
        case MPLC_OP_MUL_R64:
            push_r64(vm, pop_r64(vm) * pop_r64(vm));
            break;
        case MPLC_OP_DIV_R64:
            { double b = pop_r64(vm); double a = pop_r64(vm); push_r64(vm, b ? a / b : 0.0); }
            break;

        case MPLC_OP_EQ_I32: {
            int32_t right = pop_i32(vm);
            push_bool(vm, pop_i32(vm) == right);
            break;
        }
        case MPLC_OP_NE_I32: {
            int32_t right = pop_i32(vm);
            push_bool(vm, pop_i32(vm) != right);
            break;
        }
        case MPLC_OP_LT_I32: {
            int32_t right = pop_i32(vm);
            push_bool(vm, pop_i32(vm) < right);
            break;
        }
        case MPLC_OP_LE_I32: {
            int32_t right = pop_i32(vm);
            push_bool(vm, pop_i32(vm) <= right);
            break;
        }
        case MPLC_OP_GT_I32: {
            int32_t right = pop_i32(vm);
            push_bool(vm, pop_i32(vm) > right);
            break;
        }
        case MPLC_OP_GE_I32: {
            int32_t right = pop_i32(vm);
            push_bool(vm, pop_i32(vm) >= right);
            break;
        }
        case MPLC_OP_EQ_BOOL:
            push_bool(vm, pop_bool(vm) == pop_bool(vm));
            break;

        case MPLC_OP_I32_TO_R64:
            push_r64(vm, (double)pop_i32(vm));
            break;
        case MPLC_OP_R64_TO_I32:
            push_i32(vm, (int32_t)pop_r64(vm));
            break;

        case MPLC_OP_JMP: {
            uint32_t target = read_u32_pc(&pc);
            pc = vm->cfg.code_base + code_offset + target;
            break;
        }
        case MPLC_OP_JMP_IF: {
            uint32_t target = read_u32_pc(&pc);
            if (pop_bool(vm)) {
                pc = vm->cfg.code_base + code_offset + target;
            }
            break;
        }
        case MPLC_OP_JMP_IFNOT: {
            uint32_t target = read_u32_pc(&pc);
            if (!pop_bool(vm)) {
                pc = vm->cfg.code_base + code_offset + target;
            }
            break;
        }
        case MPLC_OP_CALL: {
            uint16_t target_pou = read_u16_pc(&pc);
            uint32_t target_off = read_u32_pc(&pc);
            uint32_t target_size = read_u32_pc(&pc);
            if (mplc_vm_run_pou(vm, target_pou, target_off, target_size) != 0) {
                return -2;
            }
            break;
        }
        case MPLC_OP_CALL_NATIVE_FB: {
            uint16_t fb_type = read_u16_pc(&pc);
            int32_t inst_off = read_i32_pc(&pc);
            int32_t param_count = read_i32_pc(&pc);
            if (exec_native_fb(vm, fb_type, inst_off, param_count) != 0) {
                return -3;
            }
            break;
        }
        case MPLC_OP_RET:
            return 0;

        case MPLC_OP_SFC_ENTER:
        case MPLC_OP_SFC_EXIT:
        case MPLC_OP_SFC_ACTION:
        case MPLC_OP_SFC_TRANS:
            pc += 4;
            break;

        default:
            return -4;
        }

        (void)ip;
        ip++;
    }

    return 0;
}
