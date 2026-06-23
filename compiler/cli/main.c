#include "frontend/st/st_parser.h"
#include "frontend/il/il_parser.h"
#include "frontend/plcopen/plcopen_parser.h"
#include "semantic/semantic.h"
#include "codegen/codegen.h"
#include "linker/linker.h"
#include "diag.h"
#include "mplc_abi.h"
#include "mplc/loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define str_eq_nocase _stricmp
#else
#include <strings.h>
#define str_eq_nocase strcasecmp
#endif

static int compile_source(const char *input, const char *output, diag_ctx_t *diag)
{
    ir_module_t mod;
    sym_table_t globals;
    codegen_module_t cg;
    linker_output_t pkg;
    int rc = 0;
    const char *ext = strrchr(input, '.');

    ir_module_init(&mod);
    sym_table_init(&globals);
    memset(&cg, 0, sizeof(cg));
    memset(&pkg, 0, sizeof(pkg));

    if (ext && str_eq_nocase(ext, ".st") == 0) {
        rc = st_parse_file(input, &mod, &globals, diag);
    } else if (ext && str_eq_nocase(ext, ".il") == 0) {
        rc = il_parse_file(input, &mod, &globals, diag);
    } else if (ext && str_eq_nocase(ext, ".xml") == 0) {
        rc = plcopen_parse_file(input, &mod, &globals, diag);
    } else {
        rc = st_parse_file(input, &mod, &globals, diag);
    }

    if (rc == 0) {
        rc = semantic_analyze(&mod, &globals, diag);
    }
    if (rc == 0) {
        rc = codegen_module_from_ir(&mod, &cg);
    }
    if (rc == 0) {
        rc = linker_build_package(&mod, &cg, &globals, &pkg);
    }
    if (rc == 0) {
        FILE *f = fopen(output, "wb");
        if (!f) {
            diag_emit(diag, DIAG_ERROR, output, 0, "Cannot write output");
            rc = -1;
        } else {
            fwrite(pkg.data, 1, pkg.size, f);
            fclose(f);
            printf("Compiled %s -> %s (%zu bytes)\n", input, output, (size_t)pkg.size);
        }
    }

    linker_output_free(&pkg);
    codegen_module_free(&cg);
    sym_table_free(&globals);
    ir_module_free(&mod);
    return rc;
}

static int inspect_package(const char *path)
{
    FILE *f = fopen(path, "rb");
    long sz;
    uint8_t *buf;
    mplc_loaded_pkg_t loaded;
    uint32_t i;

    if (!f) {
        fprintf(stderr, "Cannot open %s\n", path);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    rewind(f);
    buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) {
        fclose(f);
        return 1;
    }
    fread(buf, 1, (size_t)sz, f);
    fclose(f);

    if (mplc_loader_parse(buf, (size_t)sz, &loaded) != 0) {
        fprintf(stderr, "Invalid package\n");
        free(buf);
        return 1;
    }

    printf("MPLC package: %s\n", path);
    printf("  ABI version: %u\n", loaded.header->abi_version);
    printf("  Cycle: %u us\n", loaded.header->default_cycle_us);
    printf("  POUs: %u\n", loaded.pou_count);
    for (i = 0; i < loaded.pou_count; i++) {
        printf("    POU %u: kind=%u offset=%u size=%u\n",
               loaded.pous[i].pou_id, loaded.pous[i].kind,
               loaded.pous[i].code_offset, loaded.pous[i].code_size);
    }
    printf("  IO channels: %u\n", loaded.io_count);
    printf("  Tasks: %u\n", loaded.task_count);

    free(buf);
    return 0;
}

int main(int argc, char **argv)
{
    diag_ctx_t diag;
    int i;

    if (argc < 3) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s compile <input> -o <output.mplc>\n", argv[0]);
        fprintf(stderr, "  %s inspect <package.mplc>\n", argv[0]);
        return 1;
    }

    diag_init(&diag);

    if (strcmp(argv[1], "inspect") == 0) {
        int rc = inspect_package(argv[2]);
        diag_free(&diag);
        return rc;
    }

    if (strcmp(argv[1], "compile") == 0) {
        const char *input = argv[2];
        const char *output = "out.mplc";
        for (i = 3; i < argc - 1; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                output = argv[i + 1];
            }
        }
        if (compile_source(input, output, &diag) != 0) {
            for (i = 0; i < (int)diag.count; i++) {
                fprintf(stderr, "%s:%u: %s\n", diag.msgs[i].file, diag.msgs[i].line,
                        diag.msgs[i].message);
            }
            diag_free(&diag);
            return 1;
        }
        diag_free(&diag);
        return 0;
    }

    fprintf(stderr, "Unknown command: %s\n", argv[1]);
    diag_free(&diag);
    return 1;
}
