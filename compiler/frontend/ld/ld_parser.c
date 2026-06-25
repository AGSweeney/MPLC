/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ld_parser.h"
#include "../../semantic/semantic.h"
#include "mplc/stdlib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define LD_MAX_PINS 8U

typedef enum {
    LD_ELEM_CONTACT = 0,
    LD_ELEM_COIL,
    LD_ELEM_BLOCK
} ld_elem_kind_t;

typedef enum {
    LD_COIL_NORMAL = 0,
    LD_COIL_SET,
    LD_COIL_RESET
} ld_coil_kind_t;

typedef struct {
    char formal[32];
    char variable[64];
    char literal[64];
} ld_pin_t;

typedef struct {
    int           local_id;
    ld_elem_kind_t kind;
    char          variable[64];
    int           negated;
    ld_coil_kind_t coil_kind;
    char          type_name[32];
    int           rung_index;
    int           row;
    int           col;
    ld_pin_t      pins[LD_MAX_PINS];
    int           pin_count;
    int32_t       instance_offset;
} ld_element_t;

typedef struct {
    int from_id;
    int to_id;
    char from_pin[32];
    char to_pin[32];
} ld_connection_t;

typedef struct {
    int fork_col;
    int join_col;
    int path_row;
} ld_input_branch_region_t;

#define LD_MAX_INPUT_BRANCH_REGIONS 8

typedef struct {
    ld_element_t    *elements;
    uint32_t         element_count;
    ld_connection_t *connections;
    uint32_t         connection_count;
    int              branch_fork_col;
    int              branch_join_col;
    int              output_branch_fork_col;
    ld_input_branch_region_t input_branch_regions[LD_MAX_INPUT_BRANCH_REGIONS];
    int              input_branch_region_count;
} ld_rung_t;

typedef struct {
    ld_rung_t *rungs;
    uint32_t   rung_count;
} ld_program_t;

typedef struct {
    const char *formal;
    mplc_type_t type;
    const char *default_lit;
} ld_fb_param_spec_t;

static const ld_fb_param_spec_t g_ton_params[] = {
    {"IN", MPLC_TYPE_BOOL, NULL},
    {"PT", MPLC_TYPE_DINT, "T#1s"},
    {NULL, MPLC_TYPE_BOOL, NULL}
};

static const ld_fb_param_spec_t g_tof_params[] = {
    {"IN", MPLC_TYPE_BOOL, NULL},
    {"PT", MPLC_TYPE_DINT, "T#1s"},
    {NULL, MPLC_TYPE_BOOL, NULL}
};

static const ld_fb_param_spec_t g_tp_params[] = {
    {"IN", MPLC_TYPE_BOOL, NULL},
    {"PT", MPLC_TYPE_DINT, "T#500ms"},
    {NULL, MPLC_TYPE_BOOL, NULL}
};

static const ld_fb_param_spec_t g_ctu_params[] = {
    {"CU", MPLC_TYPE_BOOL, NULL},
    {"R", MPLC_TYPE_BOOL, NULL},
    {"PV", MPLC_TYPE_DINT, "10"},
    {NULL, MPLC_TYPE_BOOL, NULL}
};

static const ld_fb_param_spec_t g_ctd_params[] = {
    {"CD", MPLC_TYPE_BOOL, NULL},
    {"LD", MPLC_TYPE_BOOL, NULL},
    {"PV", MPLC_TYPE_DINT, "10"},
    {NULL, MPLC_TYPE_BOOL, NULL}
};

static const ld_fb_param_spec_t g_ctud_params[] = {
    {"CU", MPLC_TYPE_BOOL, NULL},
    {"CD", MPLC_TYPE_BOOL, NULL},
    {"R", MPLC_TYPE_BOOL, NULL},
    {"LD", MPLC_TYPE_BOOL, NULL},
    {"PV", MPLC_TYPE_DINT, "10"},
    {NULL, MPLC_TYPE_BOOL, NULL}
};

static const ld_fb_param_spec_t g_r_trig_params[] = {
    {"CLK", MPLC_TYPE_BOOL, NULL},
    {NULL, MPLC_TYPE_BOOL, NULL}
};

static const ld_fb_param_spec_t g_f_trig_params[] = {
    {"CLK", MPLC_TYPE_BOOL, NULL},
    {NULL, MPLC_TYPE_BOOL, NULL}
};

static const ld_fb_param_spec_t g_sr_params[] = {
    {"S1", MPLC_TYPE_BOOL, NULL},
    {"R", MPLC_TYPE_BOOL, NULL},
    {NULL, MPLC_TYPE_BOOL, NULL}
};

static const ld_fb_param_spec_t g_rs_params[] = {
    {"S", MPLC_TYPE_BOOL, NULL},
    {"R1", MPLC_TYPE_BOOL, NULL},
    {NULL, MPLC_TYPE_BOOL, NULL}
};

static const sym_entry_t *ld_lookup(sym_table_t *tab, const char *name)
{
    uint32_t i;
    for (i = 0; i < tab->count; i++) {
        if (strcmp(tab->entries[i].name, name) == 0) {
            return &tab->entries[i];
        }
    }
    return NULL;
}

static ir_expr_t *ld_var_expr(sym_table_t *globals, const char *name)
{
    const sym_entry_t *sym = ld_lookup(globals, name);
    if (sym) {
        if (sym->direction == MPLC_IO_IN || sym->direction == MPLC_IO_OUT) {
            return ir_expr_io(sym->io_slot, sym->type);
        }
        return ir_expr_var(sym->offset, sym->type);
    }
    return ir_expr_var(0, MPLC_TYPE_BOOL);
}

static const char *ld_tag_end(const char *tag)
{
    return strchr(tag, '>');
}

static const char *ld_attr_value_start(const char *tag, const char *name)
{
    char bounded[512];
    char pattern[64];
    const char *close = ld_tag_end(tag);
    const char *found;
    size_t tag_len;

    if (tag == NULL || name == NULL || name[0] == '\0') {
        return NULL;
    }

    tag_len = close != NULL ? (size_t)(close - tag) : strlen(tag);
    if (tag_len >= sizeof(bounded)) {
        tag_len = sizeof(bounded) - 1U;
    }
    memcpy(bounded, tag, tag_len);
    bounded[tag_len] = '\0';

    snprintf(pattern, sizeof(pattern), "%s=\"", name);
    found = strstr(bounded, pattern);
    if (found == NULL) {
        return NULL;
    }
    return tag + (size_t)(found - bounded) + strlen(pattern);
}

static int ld_attr_int(const char *tag, const char *name, int default_value)
{
    const char *value = ld_attr_value_start(tag, name);
    if (value == NULL) {
        return default_value;
    }
    return (int)strtol(value, NULL, 10);
}

static void ld_attr_str(const char *tag, const char *name, char *out, size_t out_sz)
{
    const char *value = ld_attr_value_start(tag, name);
    size_t i;

    if (out_sz == 0U) {
        return;
    }
    out[0] = '\0';
    if (value == NULL) {
        return;
    }
    for (i = 0; value[i] != '"' && value[i] != '\0' && i + 1U < out_sz; i++) {
        out[i] = value[i];
    }
    out[i] = '\0';
}

static int ld_attr_bool(const char *tag, const char *name)
{
    char value[16];
    ld_attr_str(tag, name, value, sizeof(value));
    return (strcmp(value, "true") == 0 || strcmp(value, "TRUE") == 0) ? 1 : 0;
}

static int ld_tag_is_self_closing(const char *tag)
{
    const char *close = strchr(tag, '>');
    if (!close) {
        return 0;
    }
    return close[-1] == '/';
}

static ld_coil_kind_t ld_coil_kind_from_attr(const char *tag)
{
    char value[16];
    ld_attr_str(tag, "coilAction", value, sizeof(value));
    if (strcmp(value, "set") == 0) {
        return LD_COIL_SET;
    }
    if (strcmp(value, "reset") == 0) {
        return LD_COIL_RESET;
    }
    return LD_COIL_NORMAL;
}

static ld_element_t *ld_find_element(const ld_rung_t *rung, int local_id)
{
    uint32_t i;
    for (i = 0; i < rung->element_count; i++) {
        if (rung->elements[i].local_id == local_id) {
            return &rung->elements[i];
        }
    }
    return NULL;
}

static int ld_push_element(ld_rung_t *rung, const ld_element_t *element)
{
    ld_element_t *tmp = (ld_element_t *)realloc(rung->elements,
                                                (rung->element_count + 1U) * sizeof(ld_element_t));
    if (!tmp) {
        return -1;
    }
    rung->elements = tmp;
    rung->elements[rung->element_count++] = *element;
    return 0;
}

static int ld_push_connection(ld_rung_t *rung, const ld_connection_t *connection)
{
    ld_connection_t *tmp = (ld_connection_t *)realloc(rung->connections,
                                                    (rung->connection_count + 1U) * sizeof(ld_connection_t));
    if (!tmp) {
        return -1;
    }
    rung->connections = tmp;
    rung->connections[rung->connection_count++] = *connection;
    return 0;
}

static int ld_push_rung(ld_program_t *program, const ld_rung_t *rung)
{
    ld_rung_t *tmp = (ld_rung_t *)realloc(program->rungs, (program->rung_count + 1U) * sizeof(ld_rung_t));
    if (!tmp) {
        return -1;
    }
    program->rungs = tmp;
    program->rungs[program->rung_count++] = *rung;
    return 0;
}

static const char *ld_find_tag_end(const char *tag_start)
{
    const char *close = strchr(tag_start, '>');
    return close ? close + 1 : tag_start;
}

static void ld_parse_element_tag(const char *tag, int rung_index, ld_rung_t *rung)
{
    ld_element_t element;
    memset(&element, 0, sizeof(element));
    element.rung_index = rung_index;
    element.row = ld_attr_int(tag, "row", 0);
    element.col = ld_attr_int(tag, "col", 0);
    element.local_id = ld_attr_int(tag, "localId", 0);

    if (strncmp(tag, "<contact", 8) == 0) {
        element.kind = LD_ELEM_CONTACT;
        ld_attr_str(tag, "variable", element.variable, sizeof(element.variable));
        element.negated = ld_attr_bool(tag, "negated");
        if (element.local_id == 0) {
            element.local_id = (int)rung->element_count + 1;
        }
        ld_push_element(rung, &element);
    } else if (strncmp(tag, "<coil", 5) == 0) {
        element.kind = LD_ELEM_COIL;
        ld_attr_str(tag, "variable", element.variable, sizeof(element.variable));
        element.coil_kind = ld_coil_kind_from_attr(tag);
        if (element.local_id == 0) {
            element.local_id = (int)rung->element_count + 1;
        }
        ld_push_element(rung, &element);
    } else if (strncmp(tag, "<block", 6) == 0) {
        element.kind = LD_ELEM_BLOCK;
        ld_attr_str(tag, "typeName", element.type_name, sizeof(element.type_name));
        if (element.local_id == 0) {
            element.local_id = (int)rung->element_count + 1;
        }
        ld_push_element(rung, &element);
    }
}

static void ld_parse_pin_binding_tag(const char *tag, ld_element_t *block)
{
    ld_pin_t pin;
    if (!block || block->pin_count >= (int)LD_MAX_PINS) {
        return;
    }
    memset(&pin, 0, sizeof(pin));
    ld_attr_str(tag, "formalParameter", pin.formal, sizeof(pin.formal));
    ld_attr_str(tag, "variable", pin.variable, sizeof(pin.variable));
    ld_attr_str(tag, "literal", pin.literal, sizeof(pin.literal));
    if (pin.formal[0] == '\0') {
        return;
    }
    block->pins[block->pin_count++] = pin;
}

static void ld_parse_connection_tag(const char *tag, ld_rung_t *rung)
{
    ld_connection_t connection;
    memset(&connection, 0, sizeof(connection));
    connection.from_id = ld_attr_int(tag, "fromLocalId", 0);
    connection.to_id = ld_attr_int(tag, "toLocalId", 0);
    ld_attr_str(tag, "fromPin", connection.from_pin, sizeof(connection.from_pin));
    ld_attr_str(tag, "toPin", connection.to_pin, sizeof(connection.to_pin));
    if (connection.from_id != 0 && connection.to_id != 0) {
        ld_push_connection(rung, &connection);
    }
}

static mplc_type_t ld_type_from_attr(const char *type_name)
{
    if (strcmp(type_name, "INT") == 0 || strcmp(type_name, "DINT") == 0 ||
        strcmp(type_name, "TIME") == 0) {
        return MPLC_TYPE_DINT;
    }
    if (strcmp(type_name, "REAL") == 0 || strcmp(type_name, "LREAL") == 0) {
        return MPLC_TYPE_REAL;
    }
    return MPLC_TYPE_BOOL;
}

static void ld_register_var_declaration(const char *tag, sym_table_t *globals)
{
    char name[64];
    char type_name[16];
    char address[32];
    mplc_type_t ty;
    mplc_io_direction_t dir = MPLC_IO_MEM;
    uint16_t io_idx = 0U;
    const char *p;

    ld_attr_str(tag, "name", name, sizeof(name));
    if (name[0] == '\0') {
        return;
    }
    if (ld_lookup(globals, name) != NULL) {
        return;
    }

    ld_attr_str(tag, "type", type_name, sizeof(type_name));
    ld_attr_str(tag, "address", address, sizeof(address));
    ty = ld_type_from_attr(type_name);

    if (address[0] == '%') {
        p = address + 1;
        if (*p == 'I') {
            dir = MPLC_IO_IN;
        } else if (*p == 'Q') {
            dir = MPLC_IO_OUT;
        }
        p++;
        while (*p && isalpha((unsigned char)*p)) {
            p++;
        }
        io_idx = (uint16_t)strtoul(p, (char **)&p, 10);
        if (*p == '.') {
            uint16_t bit;
            p++;
            bit = (uint16_t)strtoul(p, NULL, 10);
            if (bit != 0U) {
                io_idx = (uint16_t)(io_idx * 8U + bit);
            }
        }
    }

    sym_table_add(globals, name, ty, dir, io_idx);
}

static void ld_register_var_declarations(const char *xml, sym_table_t *globals)
{
    const char *p = xml;
    while (p && *p) {
        if (strncmp(p, "<varDeclaration", 15) == 0) {
            ld_register_var_declaration(p, globals);
        }
        p++;
    }
}

static void ld_auto_wire_rung(ld_rung_t *rung)
{
    uint32_t i;
    ld_element_t *coils[16];
    int coil_count = 0;
    ld_element_t *prefix[32];
    int prefix_count = 0;
    int output_fork_col = 0;
    int max_shared_col = 0;
    int min_coil_col = 999999;

    if (rung->element_count == 0U || rung->connection_count > 0U) {
        return;
    }

    for (i = 0; i < rung->element_count; i++) {
        if (rung->elements[i].kind == LD_ELEM_COIL) {
            if (coil_count < 16) {
                coils[coil_count++] = &rung->elements[i];
            }
            if (rung->elements[i].col < min_coil_col) {
                min_coil_col = rung->elements[i].col;
            }
        }
    }

    if (coil_count > 1 || rung->output_branch_fork_col > 0) {
        output_fork_col = rung->output_branch_fork_col;
        if (output_fork_col <= 0) {
            for (i = 0; i < rung->element_count; i++) {
                if (rung->elements[i].row == 0 && rung->elements[i].kind != LD_ELEM_COIL &&
                    rung->elements[i].col < min_coil_col && rung->elements[i].col > max_shared_col) {
                    max_shared_col = rung->elements[i].col;
                }
            }
            output_fork_col = max_shared_col > 0 ? max_shared_col + 1 : min_coil_col;
        }

        for (i = 0; i < rung->element_count; i++) {
            if (rung->elements[i].row == 0 && rung->elements[i].kind != LD_ELEM_COIL &&
                rung->elements[i].col < output_fork_col && prefix_count < 32) {
                prefix[prefix_count++] = &rung->elements[i];
            }
        }
        for (i = 0; i + 1U < (uint32_t)prefix_count; i++) {
            uint32_t j;
            for (j = i + 1U; j < (uint32_t)prefix_count; j++) {
                if (prefix[j]->col < prefix[i]->col) {
                    ld_element_t *tmp = prefix[i];
                    prefix[i] = prefix[j];
                    prefix[j] = tmp;
                }
            }
        }
        for (i = 0; i + 1U < (uint32_t)prefix_count; i++) {
            ld_connection_t connection;
            memset(&connection, 0, sizeof(connection));
            connection.from_id = prefix[i]->local_id;
            connection.to_id = prefix[i + 1U]->local_id;
            ld_push_connection(rung, &connection);
        }

        for (i = 0; i < (uint32_t)coil_count; i++) {
            ld_element_t *row_path[32];
            int row_path_count = 0;
            uint32_t j;
            ld_element_t *coil = coils[i];
            ld_element_t *prefix_tail = prefix_count > 0 ? prefix[prefix_count - 1] : NULL;

            for (j = 0; j < rung->element_count; j++) {
                ld_element_t *element = &rung->elements[j];
                if (element->row == coil->row && element->kind != LD_ELEM_COIL &&
                    element->col >= output_fork_col && element->col < coil->col && row_path_count < 32) {
                    row_path[row_path_count++] = element;
                }
            }
            for (j = 0; j + 1U < (uint32_t)row_path_count; j++) {
                uint32_t k;
                for (k = j + 1U; k < (uint32_t)row_path_count; k++) {
                    if (row_path[k]->col < row_path[j]->col) {
                        ld_element_t *tmp = row_path[j];
                        row_path[j] = row_path[k];
                        row_path[k] = tmp;
                    }
                }
            }
            for (j = 0; j + 1U < (uint32_t)row_path_count; j++) {
                ld_connection_t connection;
                memset(&connection, 0, sizeof(connection));
                connection.from_id = row_path[j]->local_id;
                connection.to_id = row_path[j + 1U]->local_id;
                ld_push_connection(rung, &connection);
            }

            if (row_path_count == 0) {
                if (prefix_tail != NULL) {
                    ld_connection_t connection;
                    memset(&connection, 0, sizeof(connection));
                    connection.from_id = prefix_tail->local_id;
                    connection.to_id = coil->local_id;
                    ld_push_connection(rung, &connection);
                }
            } else {
                if (prefix_tail != NULL) {
                    ld_connection_t connection;
                    memset(&connection, 0, sizeof(connection));
                    connection.from_id = prefix_tail->local_id;
                    connection.to_id = row_path[0]->local_id;
                    ld_push_connection(rung, &connection);
                }
                {
                    ld_connection_t connection;
                    memset(&connection, 0, sizeof(connection));
                    connection.from_id = row_path[row_path_count - 1]->local_id;
                    connection.to_id = coil->local_id;
                    ld_push_connection(rung, &connection);
                }
            }
        }
        return;
    }

    {
        int fork_col = 0;
        int join_col = 0;
        ld_element_t *coil = coil_count == 1 ? coils[0] : NULL;

        if (coil != NULL && ld_branch_span(rung, &fork_col, &join_col)) {
            ld_element_t *prefix[32];
            ld_element_t *suffix[32];
            ld_element_t *span0[32];
            int prefix_count = 0;
            int suffix_count = 0;
            int span0_count = 0;
            int max_row = 0;
            int row;

            for (i = 0; i < rung->element_count; i++) {
                ld_element_t *el = &rung->elements[i];
                if (el->row > max_row) {
                    max_row = el->row;
                }
                if (el->row == 0 && el->kind != LD_ELEM_COIL) {
                    if (el->col < fork_col && prefix_count < 32) {
                        prefix[prefix_count++] = el;
                    } else if (el->col >= join_col && (coil->col <= 0 || el->col < coil->col) &&
                               suffix_count < 32) {
                        suffix[suffix_count++] = el;
                    }
                }
            }

            for (i = 0; i + 1U < (uint32_t)prefix_count; i++) {
                uint32_t j;
                for (j = i + 1U; j < (uint32_t)prefix_count; j++) {
                    if (prefix[j]->col < prefix[i]->col) {
                        ld_element_t *tmp = prefix[i];
                        prefix[i] = prefix[j];
                        prefix[j] = tmp;
                    }
                }
            }
            for (i = 0; i + 1U < (uint32_t)suffix_count; i++) {
                uint32_t j;
                for (j = i + 1U; j < (uint32_t)suffix_count; j++) {
                    if (suffix[j]->col < suffix[i]->col) {
                        ld_element_t *tmp = suffix[i];
                        suffix[i] = suffix[j];
                        suffix[j] = tmp;
                    }
                }
            }

            span0_count = 0;
            for (i = 0; i < rung->element_count && span0_count < 32; i++) {
                ld_element_t *el = &rung->elements[i];
                if (el->row == 0 && el->kind != LD_ELEM_COIL && el->col >= fork_col && el->col < join_col) {
                    span0[span0_count++] = el;
                }
            }
            for (i = 0; i + 1U < (uint32_t)span0_count; i++) {
                uint32_t j;
                for (j = i + 1U; j < (uint32_t)span0_count; j++) {
                    if (span0[j]->col < span0[i]->col) {
                        ld_element_t *tmp = span0[i];
                        span0[i] = span0[j];
                        span0[j] = tmp;
                    }
                }
            }

            for (i = 0; i + 1U < (uint32_t)prefix_count; i++) {
                ld_connection_t connection;
                memset(&connection, 0, sizeof(connection));
                connection.from_id = prefix[i]->local_id;
                connection.to_id = prefix[i + 1U]->local_id;
                ld_push_connection(rung, &connection);
            }
            for (i = 0; i + 1U < (uint32_t)span0_count; i++) {
                ld_connection_t connection;
                memset(&connection, 0, sizeof(connection));
                connection.from_id = span0[i]->local_id;
                connection.to_id = span0[i + 1U]->local_id;
                ld_push_connection(rung, &connection);
            }
            if (prefix_count > 0 && span0_count > 0) {
                ld_connection_t connection;
                memset(&connection, 0, sizeof(connection));
                connection.from_id = prefix[prefix_count - 1U]->local_id;
                connection.to_id = span0[0]->local_id;
                ld_push_connection(rung, &connection);
            }
            for (i = 0; i + 1U < (uint32_t)suffix_count; i++) {
                ld_connection_t connection;
                memset(&connection, 0, sizeof(connection));
                connection.from_id = suffix[i]->local_id;
                connection.to_id = suffix[i + 1U]->local_id;
                ld_push_connection(rung, &connection);
            }

            if (suffix_count > 0) {
                if (span0_count > 0) {
                    ld_connection_t connection;
                    memset(&connection, 0, sizeof(connection));
                    connection.from_id = span0[span0_count - 1U]->local_id;
                    connection.to_id = suffix[0]->local_id;
                    ld_push_connection(rung, &connection);
                }
                {
                    ld_connection_t connection;
                    memset(&connection, 0, sizeof(connection));
                    connection.from_id = suffix[suffix_count - 1U]->local_id;
                    connection.to_id = coil->local_id;
                    ld_push_connection(rung, &connection);
                }
            } else if (span0_count > 0) {
                ld_connection_t connection;
                memset(&connection, 0, sizeof(connection));
                connection.from_id = span0[span0_count - 1U]->local_id;
                connection.to_id = coil->local_id;
                ld_push_connection(rung, &connection);
            }

            for (row = 1; row <= max_row; row++) {
                ld_element_t *branch_span[32];
                int branch_count = 0;
                uint32_t j;
                for (j = 0; j < rung->element_count && branch_count < 32; j++) {
                    ld_element_t *el = &rung->elements[j];
                    if (el->row == row && el->kind != LD_ELEM_COIL && el->col >= fork_col &&
                        el->col < join_col) {
                        branch_span[branch_count++] = el;
                    }
                }
                for (j = 0; j + 1U < (uint32_t)branch_count; j++) {
                    uint32_t k;
                    for (k = j + 1U; k < (uint32_t)branch_count; k++) {
                        if (branch_span[k]->col < branch_span[j]->col) {
                            ld_element_t *tmp = branch_span[j];
                            branch_span[j] = branch_span[k];
                            branch_span[k] = tmp;
                        }
                    }
                }
                for (j = 0; j + 1U < (uint32_t)branch_count; j++) {
                    ld_connection_t connection;
                    memset(&connection, 0, sizeof(connection));
                    connection.from_id = branch_span[j]->local_id;
                    connection.to_id = branch_span[j + 1U]->local_id;
                    ld_push_connection(rung, &connection);
                }
                if (branch_count > 0) {
                    ld_connection_t connection;
                    memset(&connection, 0, sizeof(connection));
                    connection.from_id = branch_span[branch_count - 1U]->local_id;
                    connection.to_id = coil->local_id;
                    ld_push_connection(rung, &connection);
                }
            }
            return;
        }

        ld_element_t *simple_coil = NULL;
        ld_element_t *ordered[64];
        uint32_t count = 0U;

        for (i = 0; i < rung->element_count; i++) {
            if (rung->elements[i].kind == LD_ELEM_COIL) {
                simple_coil = &rung->elements[i];
            } else if (rung->elements[i].row == 0 && count < 64U) {
                ordered[count++] = &rung->elements[i];
            }
        }

        for (i = 0; i + 1U < count; i++) {
            uint32_t j;
            for (j = i + 1U; j < count; j++) {
                if (ordered[j]->col < ordered[i]->col) {
                    ld_element_t *tmp = ordered[i];
                    ordered[i] = ordered[j];
                    ordered[j] = tmp;
                }
            }
        }

        for (i = 0; i + 1U < count; i++) {
            ld_connection_t connection;
            memset(&connection, 0, sizeof(connection));
            connection.from_id = ordered[i]->local_id;
            connection.to_id = ordered[i + 1U]->local_id;
            ld_push_connection(rung, &connection);
        }

        if (simple_coil != NULL && count > 0U) {
            ld_connection_t connection;
            memset(&connection, 0, sizeof(connection));
            connection.from_id = ordered[count - 1U]->local_id;
            connection.to_id = simple_coil->local_id;
            ld_push_connection(rung, &connection);
        }
    }
}

static int ld_count_incoming(const ld_rung_t *rung, int local_id, int *out_ids, int max_ids)
{
    uint32_t i;
    int count = 0;
    for (i = 0; i < rung->connection_count; i++) {
        if (rung->connections[i].to_id == local_id && count < max_ids) {
            out_ids[count++] = rung->connections[i].from_id;
        }
    }
    return count;
}

static mplc_native_fb_t ld_native_fb_type(const char *type_name)
{
    if (strcmp(type_name, "TON") == 0) return MPLC_FB_TON;
    if (strcmp(type_name, "TOF") == 0) return MPLC_FB_TOF;
    if (strcmp(type_name, "TP") == 0) return MPLC_FB_TP;
    if (strcmp(type_name, "CTU") == 0) return MPLC_FB_CTU;
    if (strcmp(type_name, "CTD") == 0) return MPLC_FB_CTD;
    if (strcmp(type_name, "CTUD") == 0) return MPLC_FB_CTUD;
    if (strcmp(type_name, "R_TRIG") == 0) return MPLC_FB_R_TRIG;
    if (strcmp(type_name, "F_TRIG") == 0) return MPLC_FB_F_TRIG;
    if (strcmp(type_name, "SR") == 0) return MPLC_FB_SR;
    if (strcmp(type_name, "RS") == 0) return MPLC_FB_RS;
    return MPLC_FB_TON;
}

static const ld_fb_param_spec_t *ld_fb_param_specs(mplc_native_fb_t fb_type)
{
    switch (fb_type) {
    case MPLC_FB_TON: return g_ton_params;
    case MPLC_FB_TOF: return g_tof_params;
    case MPLC_FB_TP: return g_tp_params;
    case MPLC_FB_CTU: return g_ctu_params;
    case MPLC_FB_CTD: return g_ctd_params;
    case MPLC_FB_CTUD: return g_ctud_params;
    case MPLC_FB_R_TRIG: return g_r_trig_params;
    case MPLC_FB_F_TRIG: return g_f_trig_params;
    case MPLC_FB_SR: return g_sr_params;
    case MPLC_FB_RS: return g_rs_params;
    default: return g_ton_params;
    }
}

static int ld_count_fb_params(const ld_fb_param_spec_t *specs)
{
    int count = 0;
    while (specs[count].formal != NULL) {
        count++;
    }
    return count;
}

static const ld_pin_t *ld_find_pin(const ld_element_t *element, const char *formal)
{
    int i;
    for (i = 0; i < element->pin_count; i++) {
        if (strcmp(element->pins[i].formal, formal) == 0) {
            return &element->pins[i];
        }
    }
    return NULL;
}

static int32_t ld_parse_time_ms(const char *lit)
{
    const char *p;
    char *end;
    long val;

    if (!lit || lit[0] == '\0') {
        return 1000;
    }
    if (strncmp(lit, "T#", 2) != 0) {
        return (int32_t)strtol(lit, NULL, 10);
    }
    p = lit + 2;
    val = strtol(p, &end, 10);
    if (end != NULL) {
        if (strcmp(end, "ms") == 0 || strcmp(end, "MS") == 0) {
            return (int32_t)val;
        }
        if (strcmp(end, "s") == 0 || strcmp(end, "S") == 0) {
            return (int32_t)(val * 1000L);
        }
        if (strcmp(end, "m") == 0 || strcmp(end, "M") == 0) {
            return (int32_t)(val * 60000L);
        }
    }
    return (int32_t)val;
}

static int ld_parse_bool_literal(const char *lit, bool *out)
{
    if (!lit || lit[0] == '\0') {
        return -1;
    }
    if (strcmp(lit, "TRUE") == 0 || strcmp(lit, "true") == 0 ||
        strcmp(lit, "1") == 0) {
        *out = true;
        return 0;
    }
    if (strcmp(lit, "FALSE") == 0 || strcmp(lit, "false") == 0 ||
        strcmp(lit, "0") == 0) {
        *out = false;
        return 0;
    }
    return -1;
}

static int ld_is_rung_bool_param(const char *formal)
{
    return formal != NULL && (
        strcmp(formal, "IN") == 0 ||
        strcmp(formal, "CU") == 0 ||
        strcmp(formal, "CD") == 0 ||
        strcmp(formal, "CLK") == 0 ||
        strcmp(formal, "S1") == 0 ||
        strcmp(formal, "S") == 0 ||
        strcmp(formal, "R") == 0 ||
        strcmp(formal, "R1") == 0 ||
        strcmp(formal, "LD") == 0);
}

static ir_expr_t *ld_element_expr(sym_table_t *globals, const ld_element_t *element);

static ir_expr_t *ld_pin_expr(sym_table_t *globals,
                              const ld_pin_t *pin,
                              mplc_type_t type,
                              const char *default_lit)
{
    const char *lit = pin && pin->literal[0] ? pin->literal : default_lit;

    if (type == MPLC_TYPE_DINT) {
        if (lit && lit[0]) {
            return ir_expr_literal_i32(ld_parse_time_ms(lit));
        }
        if (pin && pin->variable[0]) {
            return ld_var_expr(globals, pin->variable);
        }
        return ir_expr_literal_i32(ld_parse_time_ms(default_lit));
    }

    if (pin && pin->variable[0]) {
        return ld_var_expr(globals, pin->variable);
    }
    if (type == MPLC_TYPE_BOOL) {
        bool value = false;
        if (lit && ld_parse_bool_literal(lit, &value) == 0) {
            return ir_expr_literal_bool(value);
        }
        return ir_expr_literal_bool(false);
    }
    return ir_expr_literal_i32(0);
}

static ir_expr_t *ld_build_feed_expr(const ld_rung_t *rung,
                                     sym_table_t *globals,
                                     int node_id,
                                     int *visited,
                                     int visited_cap)
{
    int incoming[16];
    int incoming_count;
    int i;
    ld_element_t *element;
    ir_expr_t *result;

    for (i = 0; i < visited_cap; i++) {
        if (visited[i] == node_id) {
            return ir_expr_literal_bool(false);
        }
    }
    for (i = 0; i < visited_cap; i++) {
        if (visited[i] == 0) {
            visited[i] = node_id;
            break;
        }
    }

    element = ld_find_element(rung, node_id);
    if (element == NULL) {
        return ir_expr_literal_bool(true);
    }

    if (element->kind == LD_ELEM_COIL) {
        return ir_expr_literal_bool(true);
    }

    if (element->kind == LD_ELEM_CONTACT) {
        ir_expr_t *node_expr = ld_element_expr(globals, element);

        incoming_count = ld_count_incoming(rung, node_id, incoming, 16);
        if (incoming_count == 0) {
            return node_expr;
        }

        result = ir_expr_literal_bool(true);
        for (i = 0; i < incoming_count; i++) {
            int child_visited[16];
            ir_expr_t *part;

            memset(child_visited, 0, sizeof(child_visited));
            memcpy(child_visited, visited, sizeof(child_visited));
            part = ld_build_feed_expr(rung, globals, incoming[i], child_visited, 16);
            result = ir_expr_binop(IR_BIN_AND, result, part, MPLC_TYPE_BOOL);
        }
        return ir_expr_binop(IR_BIN_AND, result, node_expr, MPLC_TYPE_BOOL);
    }

    incoming_count = ld_count_incoming(rung, node_id, incoming, 16);
    if (incoming_count == 0) {
        return ir_expr_literal_bool(true);
    }

    result = ir_expr_literal_bool(true);
    for (i = 0; i < incoming_count; i++) {
        int child_visited[16];
        ir_expr_t *part;
        memset(child_visited, 0, sizeof(child_visited));
        memcpy(child_visited, visited, sizeof(child_visited));
        part = ld_build_feed_expr(rung, globals, incoming[i], child_visited, 16);
        result = ir_expr_binop(IR_BIN_AND, result, part, MPLC_TYPE_BOOL);
    }
    return result;
}

static ir_expr_t **ld_build_fb_params(const ld_rung_t *rung,
                                      const ld_element_t *element,
                                      sym_table_t *globals,
                                      mplc_native_fb_t fb_type,
                                      int *out_count)
{
    const ld_fb_param_spec_t *specs = ld_fb_param_specs(fb_type);
    int count = ld_count_fb_params(specs);
    ir_expr_t **params;
    int i;

    params = (ir_expr_t **)calloc((size_t)count, sizeof(*params));
    if (!params) {
        return NULL;
    }
    for (i = 0; i < count; i++) {
        const ld_pin_t *pin = ld_find_pin(element, specs[i].formal);
        if (ld_is_rung_bool_param(specs[i].formal)) {
            int visited[16];
            memset(visited, 0, sizeof(visited));
            params[i] = ld_build_feed_expr(rung, globals, element->local_id, visited, 16);
        } else {
            params[i] = ld_pin_expr(globals, pin, specs[i].type, specs[i].default_lit);
        }
        if (!params[i]) {
            free(params);
            return NULL;
        }
    }
    *out_count = count;
    return params;
}

static int ld_assign_fb_instances(ld_program_t *program, ir_module_t *mod)
{
    int32_t next_offset = 0;
    uint32_t r;
    uint32_t e;

    for (r = 0; r < program->rung_count; r++) {
        ld_rung_t *rung = &program->rungs[r];
        for (e = 0; e < rung->element_count; e++) {
            ld_element_t *element = &rung->elements[e];
            mplc_native_fb_t fb_type;
            uint32_t inst_size;
            uint32_t aligned;

            if (element->kind != LD_ELEM_BLOCK) {
                continue;
            }
            fb_type = ld_native_fb_type(element->type_name);
            inst_size = mplc_stdlib_instance_size(fb_type);
            if (inst_size == 0U) {
                return -1;
            }
            aligned = (inst_size + 3U) & ~3U;
            element->instance_offset = next_offset;
            if (ir_module_add_fb_instance(mod, fb_type, next_offset,
                                          (uint16_t)element->local_id) != 0) {
                return -1;
            }
            next_offset += (int32_t)aligned;
        }
    }
    if (next_offset > 0 && mod->fb_arena_size < (uint32_t)next_offset + 256U) {
        mod->fb_arena_size = (uint32_t)next_offset + 256U;
    }
    return 0;
}

static ir_expr_t *ld_element_expr(sym_table_t *globals, const ld_element_t *element)
{
    if (element->kind == LD_ELEM_CONTACT) {
        ir_expr_t *expr = ld_var_expr(globals, element->variable);
        if (element->negated) {
            return ir_expr_unop(IR_UN_NOT, expr, MPLC_TYPE_BOOL);
        }
        return expr;
    }
    if (element->kind == LD_ELEM_BLOCK) {
        return ir_expr_fb_output(ld_native_fb_type(element->type_name),
                                 element->instance_offset, 0U);
    }
    return ir_expr_literal_bool(true);
}

static ir_expr_t *ld_build_path_expr(const ld_rung_t *rung,
                                     sym_table_t *globals,
                                     int node_id,
                                     int *visited,
                                     int visited_cap)
{
    int incoming[16];
    int incoming_count;
    int i;
    ld_element_t *element;
    ir_expr_t *node_expr;
    ir_expr_t *result;

    for (i = 0; i < visited_cap; i++) {
        if (visited[i] == node_id) {
            return ir_expr_literal_bool(false);
        }
    }
    for (i = 0; i < visited_cap; i++) {
        if (visited[i] == 0) {
            visited[i] = node_id;
            break;
        }
    }

    element = ld_find_element(rung, node_id);
    if (element == NULL) {
        return ir_expr_literal_bool(true);
    }

    if (element->kind == LD_ELEM_COIL) {
        return ir_expr_literal_bool(true);
    }

    node_expr = ld_element_expr(globals, element);
    incoming_count = ld_count_incoming(rung, node_id, incoming, 16);
    if (incoming_count == 0) {
        return node_expr;
    }

    result = ir_expr_literal_bool(true);
    for (i = 0; i < incoming_count; i++) {
        int child_visited[16];
        ir_expr_t *part;
        memset(child_visited, 0, sizeof(child_visited));
        memcpy(child_visited, visited, sizeof(child_visited));
        part = ld_build_path_expr(rung, globals, incoming[i], child_visited, 16);
        result = ir_expr_binop(IR_BIN_AND, result, part, MPLC_TYPE_BOOL);
    }
    return ir_expr_binop(IR_BIN_AND, result, node_expr, MPLC_TYPE_BOOL);
}

static int ld_branch_span(const ld_rung_t *rung, int *fork_col, int *join_col)
{
    int min_col = 999999;
    int max_col = 0;
    int found = 0;
    uint32_t i;

    if (rung->branch_fork_col > 0 && rung->branch_join_col > rung->branch_fork_col) {
        *fork_col = rung->branch_fork_col;
        *join_col = rung->branch_join_col;
        return 1;
    }

    for (i = 0; i < rung->element_count; i++) {
        const ld_element_t *el = &rung->elements[i];
        if (el->row > 0 && el->kind != LD_ELEM_COIL) {
            found = 1;
            if (el->col < min_col) {
                min_col = el->col;
            }
            if (el->col > max_col) {
                max_col = el->col;
            }
        }
    }
    if (!found) {
        return 0;
    }
    *fork_col = min_col;
    *join_col = max_col + 1;
    return 1;
}

static int ld_element_col_compare(const void *a, const void *b)
{
    const ld_element_t *left = *(const ld_element_t *const *)a;
    const ld_element_t *right = *(const ld_element_t *const *)b;
    return left->col - right->col;
}

static ir_expr_t *ld_and_elements(sym_table_t *globals, const ld_element_t **elements, int count)
{
    ir_expr_t *result = NULL;
    int i;

    for (i = 0; i < count; i++) {
        ir_expr_t *part = ld_element_expr(globals, elements[i]);
        result = result ? ir_expr_binop(IR_BIN_AND, result, part, MPLC_TYPE_BOOL) : part;
    }
    return result;
}

static ir_expr_t *ld_and_row_span(const ld_rung_t *rung,
                                  sym_table_t *globals,
                                  int row,
                                  int fork_col,
                                  int join_col)
{
    const ld_element_t *elems[32];
    int count = 0;
    uint32_t i;

    for (i = 0; i < rung->element_count; i++) {
        const ld_element_t *el = &rung->elements[i];
        if (el->row == row && el->kind != LD_ELEM_COIL &&
            el->col >= fork_col && el->col < join_col) {
            if (count < 32) {
                elems[count++] = el;
            }
        }
    }
    if (count == 0) {
        return NULL;
    }
    qsort(elems, (size_t)count, sizeof(elems[0]), ld_element_col_compare);
    return ld_and_elements(globals, elems, count);
}

static void ld_migrate_legacy_input_branch(ld_rung_t *rung)
{
    if (rung == NULL || rung->input_branch_region_count > 0) {
        return;
    }
    if (rung->branch_fork_col > 0 && rung->branch_join_col > rung->branch_fork_col) {
        ld_input_branch_region_t region;
        region.fork_col = rung->branch_fork_col;
        region.join_col = rung->branch_join_col;
        region.path_row = 0;
        rung->input_branch_regions[rung->input_branch_region_count++] = region;
    }
}

static int ld_region_nest_contains(const ld_input_branch_region_t *parent,
                                   const ld_input_branch_region_t *child)
{
    if (child->fork_col < parent->fork_col || child->join_col > parent->join_col) {
        return 0;
    }
    if (child->path_row < parent->path_row) {
        return 0;
    }
    if (child->path_row == parent->path_row) {
        return child->fork_col > parent->fork_col && child->join_col < parent->join_col;
    }
    return 1;
}

static int ld_max_parallel_row_for_region(const ld_rung_t *rung, const ld_input_branch_region_t *region)
{
    int max_row = region->path_row;
    uint32_t i;

    for (i = 0; i < rung->element_count; i++) {
        const ld_element_t *el = &rung->elements[i];
        if (el->kind == LD_ELEM_COIL || el->row <= region->path_row) {
            continue;
        }
        if (el->col < region->fork_col || el->col >= region->join_col) {
            continue;
        }
        if (el->row > max_row) {
            max_row = el->row;
        }
    }

    for (i = 0; i < (uint32_t)rung->input_branch_region_count; i++) {
        if (ld_region_nest_contains(region, &rung->input_branch_regions[i])) {
            int child_max = ld_max_parallel_row_for_region(rung, &rung->input_branch_regions[i]);
            if (child_max > max_row) {
                max_row = child_max;
            }
        }
    }

    return max_row;
}

static const ld_input_branch_region_t *ld_nested_input_region_on_path_row(
    const ld_rung_t *rung,
    const ld_input_branch_region_t *parent)
{
    const ld_input_branch_region_t *best = NULL;
    int best_span = 999999;
    int i;

    for (i = 0; i < rung->input_branch_region_count; i++) {
        const ld_input_branch_region_t *region = &rung->input_branch_regions[i];
        int span;

        if (region->path_row != parent->path_row) {
            continue;
        }
        if (!ld_region_nest_contains(parent, region)) {
            continue;
        }
        if (region->fork_col <= parent->fork_col && region->join_col >= parent->join_col) {
            continue;
        }
        span = region->join_col - region->fork_col;
        if (span < best_span) {
            best_span = span;
            best = region;
        }
    }

    return best;
}

static const ld_input_branch_region_t *ld_child_input_region_on_row(
    const ld_rung_t *rung,
    const ld_input_branch_region_t *parent,
    int row)
{
    const ld_input_branch_region_t *best = NULL;
    int best_span = 999999;
    int i;

    for (i = 0; i < rung->input_branch_region_count; i++) {
        const ld_input_branch_region_t *region = &rung->input_branch_regions[i];
        int span;

        if (region->path_row != row) {
            continue;
        }
        if (!ld_region_nest_contains(parent, region)) {
            continue;
        }
        span = region->join_col - region->fork_col;
        if (span < best_span) {
            best_span = span;
            best = region;
        }
    }

    return best;
}

static int ld_is_root_input_region(const ld_rung_t *rung, const ld_input_branch_region_t *region)
{
    int i;

    for (i = 0; i < rung->input_branch_region_count; i++) {
        if (ld_region_nest_contains(&rung->input_branch_regions[i], region)) {
            return 0;
        }
    }
    return 1;
}

static ir_expr_t *ld_build_input_region_span(const ld_rung_t *rung,
                                             sym_table_t *globals,
                                             const ld_input_branch_region_t *region)
{
    ir_expr_t *merged = NULL;
    const ld_input_branch_region_t *nested;
    int max_row;
    int row;

    nested = ld_nested_input_region_on_path_row(rung, region);
    if (nested != NULL) {
        ir_expr_t *path = ld_and_row_span(rung, globals, region->path_row,
                                          region->fork_col, nested->fork_col);
        ir_expr_t *inner = ld_build_input_region_span(rung, globals, nested);
        ir_expr_t *suffix = ld_and_row_span(rung, globals, region->path_row,
                                            nested->join_col, region->join_col);
        ir_expr_t *series = path;

        series = series ? ir_expr_binop(IR_BIN_AND, series, inner, MPLC_TYPE_BOOL) : inner;
        series = suffix ? (series ? ir_expr_binop(IR_BIN_AND, series, suffix, MPLC_TYPE_BOOL) : suffix) : series;
        if (series != NULL) {
            merged = series;
        }
    } else {
        ir_expr_t *path = ld_and_row_span(rung, globals, region->path_row,
                                          region->fork_col, region->join_col);
        if (path != NULL) {
            merged = path;
        }
    }

    max_row = ld_max_parallel_row_for_region(rung, region);
    for (row = region->path_row + 1; row <= max_row; row++) {
        const ld_input_branch_region_t *child = ld_child_input_region_on_row(rung, region, row);
        ir_expr_t *path;

        if (child != NULL) {
            path = ld_build_input_region_span(rung, globals, child);
        } else {
            path = ld_and_row_span(rung, globals, row, region->fork_col, region->join_col);
        }
        if (path != NULL) {
            merged = merged ? ir_expr_binop(IR_BIN_OR, merged, path, MPLC_TYPE_BOOL) : path;
        }
    }

    return merged ? merged : ir_expr_literal_bool(true);
}

static ir_expr_t *ld_build_parallel_span(const ld_rung_t *rung,
                                         sym_table_t *globals,
                                         int fork_col,
                                         int join_col)
{
    ir_expr_t *merged = NULL;
    int max_row = 0;
    int row;
    uint32_t i;

    if (rung->input_branch_region_count > 0) {
        for (i = 0; i < (uint32_t)rung->input_branch_region_count; i++) {
            if (ld_is_root_input_region(rung, &rung->input_branch_regions[i])) {
                return ld_build_input_region_span(rung, globals, &rung->input_branch_regions[i]);
            }
        }
    }

    for (i = 0; i < rung->element_count; i++) {
        if (rung->elements[i].row > max_row) {
            max_row = rung->elements[i].row;
        }
    }

    for (row = 0; row <= max_row; row++) {
        ir_expr_t *path = ld_and_row_span(rung, globals, row, fork_col, join_col);
        if (path != NULL) {
            merged = merged ? ir_expr_binop(IR_BIN_OR, merged, path, MPLC_TYPE_BOOL) : path;
        }
    }
    return merged ? merged : ir_expr_literal_bool(true);
}

static ir_expr_t *ld_build_fork_join_condition(const ld_rung_t *rung,
                                               sym_table_t *globals,
                                               const ld_element_t *coil,
                                               int fork_col,
                                               int join_col)
{
    const ld_element_t *prefix[32];
    const ld_element_t *suffix[32];
    int prefix_count = 0;
    int suffix_count = 0;
    ir_expr_t *result = NULL;
    uint32_t i;

    for (i = 0; i < rung->element_count; i++) {
        const ld_element_t *el = &rung->elements[i];
        if (el->kind == LD_ELEM_COIL || el->row != 0) {
            continue;
        }
        if (el->col < fork_col) {
            if (prefix_count < 32) {
                prefix[prefix_count++] = el;
            }
        } else if (el->col >= join_col) {
            int suffix_limit = coil->col;
            if (rung->output_branch_fork_col > 0) {
                suffix_limit = rung->output_branch_fork_col;
            } else if (suffix_limit <= 0) {
                suffix_limit = 999999;
            }
            if (el->col < suffix_limit) {
                if (suffix_count < 32) {
                    suffix[suffix_count++] = el;
                }
            }
        }
    }

    qsort(prefix, (size_t)prefix_count, sizeof(prefix[0]), ld_element_col_compare);
    qsort(suffix, (size_t)suffix_count, sizeof(suffix[0]), ld_element_col_compare);

    result = ld_and_elements(globals, prefix, prefix_count);
    {
        ir_expr_t *span = ld_build_parallel_span(rung, globals, fork_col, join_col);
        result = result ? ir_expr_binop(IR_BIN_AND, result, span, MPLC_TYPE_BOOL) : span;
    }
    {
        ir_expr_t *tail = ld_and_elements(globals, suffix, suffix_count);
        if (tail != NULL) {
            result = result ? ir_expr_binop(IR_BIN_AND, result, tail, MPLC_TYPE_BOOL) : tail;
        }
    }
    return result ? result : ir_expr_literal_bool(true);
}

static ld_element_t *ld_find_main_series_driver(const ld_rung_t *rung, const ld_element_t *coil)
{
    ld_element_t *driver = NULL;
    uint32_t i;

    for (i = 0; i < rung->element_count; i++) {
        ld_element_t *element = &rung->elements[i];
        if (element->kind == LD_ELEM_COIL || element->row != 0) {
            continue;
        }
        if (coil->col > 0 && element->col >= coil->col) {
            continue;
        }
        if (driver == NULL || element->col > driver->col) {
            driver = element;
        }
    }
    return driver;
}

static ir_expr_t *ld_build_series_by_column(const ld_rung_t *rung,
                                            sym_table_t *globals,
                                            const ld_element_t *coil)
{
    const ld_element_t *elems[32];
    int count = 0;
    uint32_t i;

    for (i = 0; i < rung->element_count; i++) {
        const ld_element_t *el = &rung->elements[i];
        if (el->kind == LD_ELEM_COIL || el->row != 0) {
            continue;
        }
        if (coil->col > 0 && el->col >= coil->col) {
            continue;
        }
        if (count < 32) {
            elems[count++] = el;
        }
    }
    if (count == 0) {
        return NULL;
    }
    qsort(elems, (size_t)count, sizeof(elems[0]), ld_element_col_compare);
    return ld_and_elements(globals, elems, count);
}

static ir_expr_t *ld_build_rung_condition(const ld_rung_t *rung, sym_table_t *globals, const ld_element_t *coil)
{
    int incoming[16];
    int incoming_count;
    int fork_col = 0;
    int join_col = 0;
    int i;
    ir_expr_t *result = NULL;

    if (ld_branch_span(rung, &fork_col, &join_col)) {
        return ld_build_fork_join_condition(rung, globals, coil, fork_col, join_col);
    }

    incoming_count = ld_count_incoming(rung, coil->local_id, incoming, 16);
    if (incoming_count > 0) {
        for (i = 0; i < incoming_count; i++) {
            int visited[16];
            ir_expr_t *part;

            memset(visited, 0, sizeof(visited));
            part = ld_build_path_expr(rung, globals, incoming[i], visited, 16);
            result = result ? ir_expr_binop(IR_BIN_OR, result, part, MPLC_TYPE_BOOL) : part;
        }
        return result ? result : ir_expr_literal_bool(true);
    }

    result = ld_build_series_by_column(rung, globals, coil);
    if (result != NULL) {
        return result;
    }

    return ir_expr_literal_bool(true);
}

static int ld_append_stmt(ir_stmt_t **stmts, uint32_t *stmt_count, const ir_stmt_t *stmt)
{
    ir_stmt_t *tmp = (ir_stmt_t *)realloc(*stmts, (*stmt_count + 1U) * sizeof(ir_stmt_t));
    if (!tmp) {
        return -1;
    }
    *stmts = tmp;
    (*stmts)[*stmt_count] = *stmt;
    (*stmt_count)++;
    return 0;
}

static int ld_append_coil_stmt(const ld_rung_t *rung,
                               sym_table_t *globals,
                               const ld_element_t *coil,
                               ir_stmt_t **stmts,
                               uint32_t *stmt_count)
{
    ir_expr_t *condition = ld_build_rung_condition(rung, globals, coil);
    ir_stmt_t st;

    memset(&st, 0, sizeof(st));
    if (coil->coil_kind == LD_COIL_SET || coil->coil_kind == LD_COIL_RESET) {
        st.kind = IR_STMT_IF;
        st.u.if_stmt.cond = condition;
        st.u.if_stmt.then_stmts = (ir_stmt_t *)calloc(1U, sizeof(ir_stmt_t));
        if (!st.u.if_stmt.then_stmts) {
            return -1;
        }
        st.u.if_stmt.then_count = 1U;
        st.u.if_stmt.then_stmts[0].kind = IR_STMT_ASSIGN;
        st.u.if_stmt.then_stmts[0].u.assign.target = ld_var_expr(globals, coil->variable);
        st.u.if_stmt.then_stmts[0].u.assign.value = ir_expr_literal_bool(coil->coil_kind == LD_COIL_SET);
    } else {
        st.kind = IR_STMT_ASSIGN;
        st.u.assign.target = ld_var_expr(globals, coil->variable);
        st.u.assign.value = condition;
    }

    return ld_append_stmt(stmts, stmt_count, &st);
}

static int ld_lower_rung(const ld_rung_t *rung, sym_table_t *globals, ir_stmt_t **stmts, uint32_t *stmt_count)
{
    uint32_t i;

    for (i = 0; i < rung->element_count; i++) {
        const ld_element_t *element = &rung->elements[i];
        if (element->kind == LD_ELEM_BLOCK) {
            ir_stmt_t st;
            mplc_native_fb_t fb_type = ld_native_fb_type(element->type_name);
            int param_count = 0;
            ir_expr_t **params = ld_build_fb_params(rung, element, globals, fb_type, &param_count);
            if (!params) {
                return -1;
            }
            memset(&st, 0, sizeof(st));
            st.kind = IR_STMT_EXPR;
            st.u.expr = ir_expr_call_native_fb(fb_type, element->instance_offset, params, param_count);
            if (!st.u.expr || ld_append_stmt(stmts, stmt_count, &st) != 0) {
                return -1;
            }
        }
    }

    for (i = 0; i < rung->element_count; i++) {
        const ld_element_t *element = &rung->elements[i];
        if (element->kind != LD_ELEM_COIL) {
            continue;
        }
        if (ld_append_coil_stmt(rung, globals, element, stmts, stmt_count) != 0) {
            return -1;
        }
    }

    return 0;
}

static void ld_free_program(ld_program_t *program)
{
    uint32_t i;
    if (!program) {
        return;
    }
    for (i = 0; i < program->rung_count; i++) {
        free(program->rungs[i].elements);
        free(program->rungs[i].connections);
    }
    free(program->rungs);
    memset(program, 0, sizeof(*program));
}

static int ld_parse_program(const char *xml, ld_program_t *program)
{
    const char *p = xml;
    ld_rung_t current_rung;
    ld_element_t *current_block = NULL;
    int rung_index = 0;
    int in_rung = 0;

    memset(program, 0, sizeof(*program));
    memset(&current_rung, 0, sizeof(current_rung));

    while (p && *p) {
        if (strncmp(p, "<rung", 5) == 0) {
            memset(&current_rung, 0, sizeof(current_rung));
            current_block = NULL;
            in_rung = 1;
            current_rung.output_branch_fork_col = ld_attr_int(p, "outputBranchForkCol", 0);
            current_rung.branch_fork_col = ld_attr_int(p, "branchForkCol", 0);
            current_rung.branch_join_col = ld_attr_int(p, "branchJoinCol", 0);
            p = ld_find_tag_end(p);
            continue;
        }
        if (in_rung && strncmp(p, "</rung>", 7) == 0) {
            ld_migrate_legacy_input_branch(&current_rung);
            ld_auto_wire_rung(&current_rung);
            ld_push_rung(program, &current_rung);
            memset(&current_rung, 0, sizeof(current_rung));
            current_block = NULL;
            in_rung = 0;
            rung_index++;
            p += 7;
            continue;
        }
        if (in_rung && strncmp(p, "<inputBranch", 12) == 0) {
            ld_input_branch_region_t region;
            region.fork_col = ld_attr_int(p, "forkCol", 0);
            region.join_col = ld_attr_int(p, "joinCol", 0);
            region.path_row = ld_attr_int(p, "pathRow", 0);
            if (region.fork_col > 0 && region.join_col > region.fork_col &&
                current_rung.input_branch_region_count < LD_MAX_INPUT_BRANCH_REGIONS) {
                current_rung.input_branch_regions[current_rung.input_branch_region_count++] = region;
            }
            p = ld_find_tag_end(p);
            continue;
        }
        if (in_rung && strncmp(p, "</block>", 8) == 0) {
            current_block = NULL;
            p += 8;
            continue;
        }
        if (strncmp(p, "<contact", 8) == 0 || strncmp(p, "<coil", 5) == 0 || strncmp(p, "<block", 6) == 0) {
            if (!in_rung) {
                if (program->rung_count == 0U && current_rung.element_count == 0U) {
                    memset(&current_rung, 0, sizeof(current_rung));
                    in_rung = 1;
                } else if (current_rung.element_count > 0U) {
                    ld_migrate_legacy_input_branch(&current_rung);
                    ld_auto_wire_rung(&current_rung);
                    ld_push_rung(program, &current_rung);
                    memset(&current_rung, 0, sizeof(current_rung));
                    in_rung = 1;
                    rung_index++;
                }
            }
            ld_parse_element_tag(p, rung_index, &current_rung);
            if (strncmp(p, "<block", 6) == 0 && !ld_tag_is_self_closing(p) &&
                current_rung.element_count > 0U) {
                current_block = &current_rung.elements[current_rung.element_count - 1U];
            } else {
                current_block = NULL;
            }
            p = ld_find_tag_end(p);
            continue;
        }
        if (in_rung && current_block != NULL && strncmp(p, "<variable", 9) == 0) {
            ld_parse_pin_binding_tag(p, current_block);
            p = ld_find_tag_end(p);
            continue;
        }
        if (strncmp(p, "<connection", 11) == 0) {
            ld_parse_connection_tag(p, &current_rung);
            p = ld_find_tag_end(p);
            continue;
        }
        p++;
    }

    if (in_rung && current_rung.element_count > 0U) {
        ld_migrate_legacy_input_branch(&current_rung);
        ld_auto_wire_rung(&current_rung);
        ld_push_rung(program, &current_rung);
    }

    return (program->rung_count > 0U) ? 0 : -1;
}

int ld_parse_xml_fragment(const char *xml, const char *file, ir_module_t *mod,
                          sym_table_t *globals, diag_ctx_t *diag)
{
    ld_program_t program;
    ir_pou_t pou;
    ir_stmt_t *stmts = NULL;
    uint32_t stmt_count = 0U;
    uint32_t i;
    int rc = 0;

    (void)file;

    ld_register_var_declarations(xml, globals);

    if (ld_parse_program(xml, &program) != 0) {
        diag_emit(diag, DIAG_WARNING, file, 0, "LD fragment had no rungs; emitting NOP rung");
        ir_stmt_t st;
        memset(&st, 0, sizeof(st));
        st.kind = IR_STMT_EXPR;
        st.u.expr = ir_expr_literal_bool(false);
        stmts = (ir_stmt_t *)malloc(sizeof(st));
        if (!stmts) {
            return -1;
        }
        stmts[0] = st;
        stmt_count = 1U;
    } else {
        if (ld_assign_fb_instances(&program, mod) != 0) {
            ld_free_program(&program);
            return -1;
        }
        for (i = 0; i < program.rung_count; i++) {
            if (ld_lower_rung(&program.rungs[i], globals, &stmts, &stmt_count) != 0) {
                rc = -1;
                break;
            }
        }
    }

    ld_free_program(&program);

    if (stmt_count == 0U) {
        diag_emit(diag, DIAG_WARNING, file, 0, "LD fragment produced no statements; emitting NOP rung");
        ir_stmt_t st;
        memset(&st, 0, sizeof(st));
        st.kind = IR_STMT_EXPR;
        st.u.expr = ir_expr_literal_bool(false);
        stmts = (ir_stmt_t *)malloc(sizeof(st));
        if (!stmts) {
            return -1;
        }
        stmts[0] = st;
        stmt_count = 1U;
    }

    memset(&pou, 0, sizeof(pou));
    strncpy(pou.name, "LDProgram", sizeof(pou.name) - 1U);
    pou.kind = IR_POU_PROGRAM;
    pou.pou_id = (uint16_t)(mod->pou_count + 1U);
    pou.stmts = stmts;
    pou.stmt_count = stmt_count;

    if (ir_module_add_pou(mod, &pou) != 0) {
        free(stmts);
        return -1;
    }

    return rc;
}
