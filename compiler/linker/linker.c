#include "linker.h"
#include "mplc_abi.h"
#include <stdlib.h>
#include <string.h>

static int append_bytes(uint8_t **buf, size_t *size, size_t *cap, const void *src, size_t n)
{
    if (*size + n > *cap) {
        size_t nc = *cap ? *cap * 2U : 512U;
        while (nc < *size + n) nc *= 2U;
        uint8_t *nb = (uint8_t *)realloc(*buf, nc);
        if (!nb) return -1;
        *buf = nb;
        *cap = nc;
    }
    memcpy(*buf + *size, src, n);
    *size += n;
    return 0;
}

int linker_build_package(const ir_module_t *mod, const codegen_module_t *cg,
                         const sym_table_t *globals, linker_output_t *out)
{
    mplc_pkg_header_t hdr;
    mplc_section_header_t sections[7];
    mplc_pou_desc_t *pous = NULL;
    mplc_task_desc_t task;
    mplc_io_entry_t *io_entries = NULL;
    uint8_t *image = NULL;
    size_t image_size = 0U;
    size_t image_cap = 0U;
    uint8_t *code_blob = NULL;
    size_t code_size = 0U;
    size_t code_cap = 0U;
    uint8_t *data_blob = NULL;
    size_t data_size = 0U;
    uint32_t i;
    uint32_t code_offset_base = 0U;
    uint32_t section_table_size;

    if (!mod || !cg || !globals || !out) return -1;
    memset(out, 0, sizeof(*out));
    memset(&hdr, 0, sizeof(hdr));
    memset(sections, 0, sizeof(sections));

    hdr.magic = MPLC_MAGIC;
    hdr.abi_version = MPLC_ABI_VERSION;
    hdr.section_count = 7U;
    hdr.default_cycle_us = mod->default_cycle_us;
    hdr.data_size = globals->data_size;
    hdr.fb_arena_size = 1024U;
    hdr.max_stack_depth = MPLC_MAX_STACK_DEPTH;

    pous = (mplc_pou_desc_t *)calloc(cg->pou_count, sizeof(*pous));
    io_entries = (mplc_io_entry_t *)calloc(globals->count, sizeof(*io_entries));
    if (!pous || !io_entries) {
        free(pous);
        free(io_entries);
        return -2;
    }

    for (i = 0; i < cg->pou_count; i++) {
        pous[i].pou_id = cg->pous[i].pou_id;
        pous[i].kind = (uint8_t)mod->pous[i].kind;
        pous[i].code_offset = code_offset_base;
        pous[i].code_size = cg->pous[i].code.size;
        if (append_bytes(&code_blob, &code_size, &code_cap,
                         cg->pous[i].code.bytes, cg->pous[i].code.size) != 0) {
            free(pous); free(io_entries); free(code_blob);
            return -3;
        }
        code_offset_base += cg->pous[i].code.size;
    }

    data_size = globals->data_size;
    if (data_size > 0U) {
        data_blob = (uint8_t *)calloc(1, data_size);
        if (!data_blob) {
            free(pous); free(io_entries); free(code_blob);
            return -4;
        }
    }

    for (i = 0; i < globals->count; i++) {
        io_entries[i].index = globals->entries[i].io_index;
        io_entries[i].type = (uint8_t)globals->entries[i].type;
        io_entries[i].direction = (uint8_t)globals->entries[i].direction;
        io_entries[i].data_offset = globals->entries[i].offset;
    }

    memset(&task, 0, sizeof(task));
    task.period_us = mod->default_cycle_us;
    task.program_id = cg->pou_count > 0U ? pous[0].pou_id : 0U;

    section_table_size = (uint32_t)(sizeof(mplc_pkg_header_t) +
                                    hdr.section_count * sizeof(mplc_section_header_t));

    uint32_t off = section_table_size;

    sections[0].type = MPLC_SECTION_CODE;
    sections[0].offset = off;
    sections[0].size = (uint32_t)code_size;
    off += sections[0].size;

    sections[1].type = MPLC_SECTION_DATA;
    sections[1].offset = off;
    sections[1].size = (uint32_t)data_size;
    off += sections[1].size;

    sections[2].type = MPLC_SECTION_SYMTAB;
    sections[2].offset = off;
    sections[2].size = (uint32_t)(cg->pou_count * sizeof(mplc_pou_desc_t));
    off += sections[2].size;

    sections[3].type = MPLC_SECTION_IOMAP;
    sections[3].offset = off;
    sections[3].size = (uint32_t)(globals->count * sizeof(mplc_io_entry_t));
    off += sections[3].size;

    sections[4].type = MPLC_SECTION_FBINST;
    sections[4].offset = off;
    sections[4].size = 0U;

    sections[5].type = MPLC_SECTION_TASKS;
    sections[5].offset = off;
    sections[5].size = (uint32_t)sizeof(mplc_task_desc_t);
    off += sections[5].size;

    sections[6].type = MPLC_SECTION_SFC;
    sections[6].offset = off;
    sections[6].size = 0U;

    if (append_bytes(&image, &image_size, &image_cap, &hdr, sizeof(hdr)) != 0 ||
        append_bytes(&image, &image_size, &image_cap, sections, sizeof(sections)) != 0 ||
        append_bytes(&image, &image_size, &image_cap, code_blob, code_size) != 0 ||
        append_bytes(&image, &image_size, &image_cap, data_blob, data_size) != 0 ||
        append_bytes(&image, &image_size, &image_cap, pous,
                     cg->pou_count * sizeof(mplc_pou_desc_t)) != 0 ||
        append_bytes(&image, &image_size, &image_cap, io_entries,
                     globals->count * sizeof(mplc_io_entry_t)) != 0 ||
        append_bytes(&image, &image_size, &image_cap, &task, sizeof(task)) != 0) {
        free(pous); free(io_entries); free(code_blob); free(data_blob); free(image);
        return -5;
    }

    out->data = image;
    out->size = image_size;

    free(pous);
    free(io_entries);
    free(code_blob);
    free(data_blob);
    return 0;
}

void linker_output_free(linker_output_t *out)
{
    if (!out) return;
    free(out->data);
    memset(out, 0, sizeof(*out));
}
