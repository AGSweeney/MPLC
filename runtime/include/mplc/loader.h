#ifndef MPLC_LOADER_H
#define MPLC_LOADER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "mplc_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const uint8_t *image;
    size_t         image_size;
    const mplc_pkg_header_t *header;
    const mplc_pou_desc_t   *pous;
    uint32_t       pou_count;
    const mplc_task_desc_t  *tasks;
    uint32_t       task_count;
    const mplc_io_entry_t   *io_map;
    uint32_t       io_count;
    const mplc_fb_instance_t *fb_instances;
    uint32_t       fb_count;
    const uint8_t *code_base;
    uint32_t       code_size;
    const uint8_t *data_base;
    uint32_t       data_size;
    const mplc_sfc_step_t *sfc_steps;
    uint32_t       sfc_step_count;
} mplc_loaded_pkg_t;

int  mplc_loader_parse(const uint8_t *data, size_t size, mplc_loaded_pkg_t *out);
bool mplc_loader_validate(const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_LOADER_H */
