#ifndef MPLC_SEMANTIC_H
#define MPLC_SEMANTIC_H

#include "ir.h"
#include "diag.h"

typedef struct {
    char        name[64];
    mplc_type_t type;
    uint32_t    offset;
    uint16_t    io_index;
    uint16_t    io_slot;
    mplc_io_direction_t direction;
} sym_entry_t;

typedef struct {
    sym_entry_t *entries;
    uint32_t     count;
    uint32_t     data_size;
} sym_table_t;

void sym_table_init(sym_table_t *tab);
void sym_table_free(sym_table_t *tab);
int  sym_table_add(sym_table_t *tab, const char *name, mplc_type_t type,
                  mplc_io_direction_t dir, uint16_t io_index);

int semantic_analyze(ir_module_t *mod, sym_table_t *globals, diag_ctx_t *diag);

#endif
