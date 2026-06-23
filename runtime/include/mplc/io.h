#ifndef MPLC_IO_H
#define MPLC_IO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "mplc_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const mplc_io_entry_t *entries;
    uint32_t               count;
    uint8_t               *data_segment;
    size_t                 data_size;
} mplc_io_ctx_t;

void mplc_io_init(mplc_io_ctx_t *ctx, const mplc_io_entry_t *entries, uint32_t count,
                  uint8_t *data, size_t data_size);
void mplc_io_read_inputs(mplc_io_ctx_t *ctx);
void mplc_io_write_outputs(mplc_io_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_IO_H */
