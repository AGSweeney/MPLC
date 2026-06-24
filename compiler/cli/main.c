/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_compile.h"
#include "mplc_abi.h"
#include "mplc/loader.h"
#include "diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        if (mplc_compile_file(input, output, &diag) != 0) {
            for (i = 0; i < (int)diag.count; i++) {
                fprintf(stderr, "%s:%u: %s\n", diag.msgs[i].file, diag.msgs[i].line,
                        diag.msgs[i].message);
            }
            diag_free(&diag);
            return 1;
        }
        printf("Compiled %s -> %s\n", input, output);
        diag_free(&diag);
        return 0;
    }

    fprintf(stderr, "Unknown command: %s\n", argv[1]);
    diag_free(&diag);
    return 1;
}
