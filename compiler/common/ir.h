#ifndef MPLC_IR_H
#define MPLC_IR_H

#include "mplc_abi.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    IR_EXPR_LITERAL,
    IR_EXPR_VAR,
    IR_EXPR_IO,
    IR_EXPR_BINOP,
    IR_EXPR_UNOP,
    IR_EXPR_CALL,
    IR_EXPR_CALL_NATIVE_FB
} ir_expr_kind_t;

typedef enum {
    IR_BIN_ADD, IR_BIN_SUB, IR_BIN_MUL, IR_BIN_DIV,
    IR_BIN_AND, IR_BIN_OR, IR_BIN_XOR,
    IR_BIN_EQ, IR_BIN_NE, IR_BIN_LT, IR_BIN_LE, IR_BIN_GT, IR_BIN_GE
} ir_binop_t;

typedef enum {
    IR_UN_NOT, IR_UN_NEG
} ir_unop_t;

typedef struct ir_expr ir_expr_t;

struct ir_expr {
    ir_expr_kind_t kind;
    mplc_type_t    type;
    ir_expr_t     *left;
    ir_expr_t     *right;
    union {
        bool     lit_bool;
        int32_t  lit_i32;
        double   lit_r64;
        uint32_t var_offset;
        uint16_t io_index;
        ir_binop_t binop;
        ir_unop_t  unop;
        struct {
            uint16_t pou_id;
            uint32_t code_offset;
            uint32_t code_size;
        } call;
        struct {
            mplc_native_fb_t fb_type;
            int32_t          instance_offset;
            int32_t          param_count;
            ir_expr_t      **params;
        } native_fb;
    } u;
};

typedef enum {
    IR_STMT_ASSIGN,
    IR_STMT_IF,
    IR_STMT_WHILE,
    IR_STMT_RETURN,
    IR_STMT_EXPR
} ir_stmt_kind_t;

typedef struct ir_stmt ir_stmt_t;

typedef struct {
    ir_expr_t *cond;
    ir_stmt_t *then_stmts;
    uint32_t   then_count;
    ir_stmt_t *else_stmts;
    uint32_t   else_count;
} ir_if_t;

typedef struct {
    ir_expr_t *cond;
    ir_stmt_t *body;
    uint32_t   body_count;
} ir_while_t;

struct ir_stmt {
    ir_stmt_kind_t kind;
    union {
        struct {
            ir_expr_t *target;
            ir_expr_t *value;
        } assign;
        ir_if_t    if_stmt;
        ir_while_t while_stmt;
        ir_expr_t *expr;
    } u;
};

typedef enum {
    IR_POU_PROGRAM,
    IR_POU_FUNCTION,
    IR_POU_FUNCTION_BLOCK
} ir_pou_kind_t;

typedef struct {
    char          name[64];
    ir_pou_kind_t kind;
    uint16_t      pou_id;
    ir_stmt_t    *stmts;
    uint32_t      stmt_count;
    mplc_type_t   return_type;
} ir_pou_t;

typedef struct {
    char       name[64];
    uint32_t   offset;
    mplc_type_t type;
    mplc_io_direction_t direction;
    uint16_t   io_index;
} ir_var_t;

typedef struct {
    ir_pou_t  *pous;
    uint32_t   pou_count;
    ir_var_t  *globals;
    uint32_t   global_count;
    uint32_t   default_cycle_us;
} ir_module_t;

ir_expr_t *ir_expr_literal_bool(bool v);
ir_expr_t *ir_expr_literal_i32(int32_t v);
ir_expr_t *ir_expr_var(uint32_t offset, mplc_type_t type);
ir_expr_t *ir_expr_io(uint16_t index, mplc_type_t type);
ir_expr_t *ir_expr_binop(ir_binop_t op, ir_expr_t *l, ir_expr_t *r, mplc_type_t type);
ir_expr_t *ir_expr_unop(ir_unop_t op, ir_expr_t *e, mplc_type_t type);
ir_expr_t *ir_expr_call_native_fb(mplc_native_fb_t fb, int32_t inst_off, ir_expr_t **params, int n);

void ir_module_init(ir_module_t *mod);
void ir_module_free(ir_module_t *mod);
int  ir_module_add_pou(ir_module_t *mod, const ir_pou_t *pou);
int  ir_module_add_global(ir_module_t *mod, const ir_var_t *var);

#endif
