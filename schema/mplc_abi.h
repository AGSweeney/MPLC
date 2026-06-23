#ifndef MPLC_ABI_H
#define MPLC_ABI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MPLC_MAGIC           0x434C504DU /* 'MPLC' little-endian */
#define MPLC_ABI_VERSION     1U
#define MPLC_MAX_STACK_DEPTH 64U
#define MPLC_MAX_POUS        256U
#define MPLC_MAX_TASKS       32U
#define MPLC_MAX_IO_CHANNELS 4096U
#define MPLC_STRING_MAX      256U

typedef enum {
    MPLC_SECTION_HEADER = 0,
    MPLC_SECTION_CODE   = 1,
    MPLC_SECTION_DATA   = 2,
    MPLC_SECTION_SYMTAB = 3,
    MPLC_SECTION_IOMAP  = 4,
    MPLC_SECTION_FBINST = 5,
    MPLC_SECTION_TASKS  = 6,
    MPLC_SECTION_RELOC  = 7,
    MPLC_SECTION_SFC    = 8
} mplc_section_type_t;

typedef enum {
    MPLC_TYPE_BOOL  = 0,
    MPLC_TYPE_SINT  = 1,
    MPLC_TYPE_INT   = 2,
    MPLC_TYPE_DINT  = 3,
    MPLC_TYPE_LINT  = 4,
    MPLC_TYPE_USINT = 5,
    MPLC_TYPE_UINT  = 6,
    MPLC_TYPE_UDINT = 7,
    MPLC_TYPE_ULINT = 8,
    MPLC_TYPE_REAL  = 9,
    MPLC_TYPE_LREAL = 10,
    MPLC_TYPE_BYTE  = 11,
    MPLC_TYPE_WORD  = 12,
    MPLC_TYPE_DWORD = 13,
    MPLC_TYPE_LWORD = 14,
    MPLC_TYPE_TIME  = 15,
    MPLC_TYPE_STRING = 16
} mplc_type_t;

typedef enum {
    MPLC_IO_IN  = 0,
    MPLC_IO_OUT = 1,
    MPLC_IO_MEM = 2
} mplc_io_direction_t;

typedef enum {
    MPLC_POU_PROGRAM        = 0,
    MPLC_POU_FUNCTION       = 1,
    MPLC_POU_FUNCTION_BLOCK = 2
} mplc_pou_kind_t;

typedef enum {
    MPLC_OP_NOP = 0,

    /* Stack literals */
    MPLC_OP_PUSH_BOOL  = 1,
    MPLC_OP_PUSH_I32   = 2,
    MPLC_OP_PUSH_R64   = 3,

    /* Memory */
    MPLC_OP_LOAD_VAR   = 10,
    MPLC_OP_STORE_VAR  = 11,
    MPLC_OP_LOAD_IO    = 12,
    MPLC_OP_STORE_IO   = 13,

    /* Logic */
    MPLC_OP_AND = 20,
    MPLC_OP_OR  = 21,
    MPLC_OP_XOR = 22,
    MPLC_OP_NOT = 23,

    /* Arithmetic */
    MPLC_OP_ADD_I32 = 30,
    MPLC_OP_SUB_I32 = 31,
    MPLC_OP_MUL_I32 = 32,
    MPLC_OP_DIV_I32 = 33,
    MPLC_OP_ADD_R64 = 34,
    MPLC_OP_SUB_R64 = 35,
    MPLC_OP_MUL_R64 = 36,
    MPLC_OP_DIV_R64 = 37,

    /* Compare */
    MPLC_OP_EQ_I32 = 40,
    MPLC_OP_NE_I32 = 41,
    MPLC_OP_LT_I32 = 42,
    MPLC_OP_LE_I32 = 43,
    MPLC_OP_GT_I32 = 44,
    MPLC_OP_GE_I32 = 45,
    MPLC_OP_EQ_BOOL = 46,

    /* Control flow */
    MPLC_OP_JMP      = 50,
    MPLC_OP_JMP_IF   = 51,
    MPLC_OP_JMP_IFNOT = 52,
    MPLC_OP_CALL     = 53,
    MPLC_OP_CALL_NATIVE_FB = 54,
    MPLC_OP_RET      = 55,

    /* Cast / convert */
    MPLC_OP_I32_TO_R64 = 60,
    MPLC_OP_R64_TO_I32 = 61,

    /* SFC */
    MPLC_OP_SFC_ENTER  = 70,
    MPLC_OP_SFC_EXIT   = 71,
    MPLC_OP_SFC_ACTION = 72,
    MPLC_OP_SFC_TRANS  = 73,

    MPLC_OP_DEBUG_TRAP = 255
} mplc_opcode_t;

typedef enum {
    MPLC_FB_TON   = 0,
    MPLC_FB_TOF   = 1,
    MPLC_FB_TP    = 2,
    MPLC_FB_CTU   = 3,
    MPLC_FB_CTD   = 4,
    MPLC_FB_CTUD  = 5,
    MPLC_FB_R_TRIG = 6,
    MPLC_FB_F_TRIG = 7,
    MPLC_FB_SR    = 8,
    MPLC_FB_RS    = 9,
    MPLC_FB_CUSTOM = 255
} mplc_native_fb_t;

typedef struct {
    uint32_t magic;
    uint16_t abi_version;
    uint16_t flags;
    uint32_t section_count;
    uint32_t default_cycle_us;
    uint32_t data_size;
    uint32_t fb_arena_size;
    uint32_t max_stack_depth;
    uint32_t reserved;
} mplc_pkg_header_t;

typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t size;
} mplc_section_header_t;

typedef struct {
    uint16_t pou_id;
    uint8_t  kind;
    uint8_t  flags;
    uint32_t code_offset;
    uint32_t code_size;
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t name_offset;
} mplc_pou_desc_t;

typedef struct {
    uint32_t period_us;
    uint32_t priority;
    uint16_t program_id;
    uint8_t  watchdog_factor;
    uint8_t  reserved;
} mplc_task_desc_t;

typedef struct {
    uint16_t index;
    uint8_t  type;
    uint8_t  direction;
    uint32_t data_offset;
    uint32_t name_offset;
} mplc_io_entry_t;

typedef struct {
    uint16_t fb_type;
    uint16_t instance_id;
    uint32_t instance_offset;
    uint32_t instance_size;
} mplc_fb_instance_t;

typedef struct {
    uint32_t offset;
    uint8_t  type;
    uint8_t  pou_id;
    uint16_t flags;
    uint32_t name_offset;
} mplc_symbol_t;

typedef struct {
    uint16_t step_id;
    uint16_t action_pou_id;
    uint32_t transition_offset;
    uint32_t action_offset;
    uint8_t  qualifier;
    uint8_t  reserved[3];
} mplc_sfc_step_t;

#ifdef __cplusplus
}
#endif

#endif /* MPLC_ABI_H */
