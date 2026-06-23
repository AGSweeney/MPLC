#ifndef MPLC_RUNTIME_H
#define MPLC_RUNTIME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "mplc_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mplc_runtime mplc_runtime_t;

typedef struct {
    const uint8_t *image;
    size_t         image_size;
    void          *data_segment;
    size_t         data_size;
    void          *fb_arena;
    size_t         fb_arena_size;
} mplc_runtime_config_t;

int  mplc_runtime_create(mplc_runtime_t **out_rt, const mplc_runtime_config_t *cfg);
void mplc_runtime_destroy(mplc_runtime_t *rt);

int  mplc_runtime_load_package(mplc_runtime_t *rt, const uint8_t *data, size_t size);
int  mplc_runtime_run_cycle(mplc_runtime_t *rt);
int  mplc_runtime_run(mplc_runtime_t *rt, uint32_t max_cycles);

bool mplc_runtime_get_bool(mplc_runtime_t *rt, uint32_t offset);
void mplc_runtime_set_bool(mplc_runtime_t *rt, uint32_t offset, bool value);
int32_t mplc_runtime_get_i32(mplc_runtime_t *rt, uint32_t offset);
void mplc_runtime_set_i32(mplc_runtime_t *rt, uint32_t offset, int32_t value);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_RUNTIME_H */
