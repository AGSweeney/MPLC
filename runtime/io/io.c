#include "mplc/io.h"
#include "mplc_hal.h"
#include <string.h>

static void read_channel(const mplc_io_entry_t *entry, uint8_t *data)
{
    switch (entry->type) {
    case MPLC_TYPE_BOOL: {
        bool val = false;
        mplc_hal_digital_read(entry->index, &val);
        data[entry->data_offset] = val ? 1U : 0U;
        break;
    }
    case MPLC_TYPE_REAL:
    case MPLC_TYPE_LREAL: {
        float val = 0.0f;
        mplc_hal_analog_read(entry->index, &val);
        memcpy(data + entry->data_offset, &val, sizeof(float));
        break;
    }
    default:
        break;
    }
}

static void write_channel(const mplc_io_entry_t *entry, const uint8_t *data)
{
    switch (entry->type) {
    case MPLC_TYPE_BOOL: {
        bool val = data[entry->data_offset] != 0U;
        mplc_hal_digital_write(entry->index, val);
        break;
    }
    case MPLC_TYPE_REAL:
    case MPLC_TYPE_LREAL: {
        float val = 0.0f;
        memcpy(&val, data + entry->data_offset, sizeof(float));
        mplc_hal_analog_write(entry->index, val);
        break;
    }
    default:
        break;
    }
}

void mplc_io_init(mplc_io_ctx_t *ctx, const mplc_io_entry_t *entries, uint32_t count,
                  uint8_t *data, size_t data_size)
{
    ctx->entries = entries;
    ctx->count = count;
    ctx->data_segment = data;
    ctx->data_size = data_size;
}

void mplc_io_read_inputs(mplc_io_ctx_t *ctx)
{
    uint32_t i;
    if (!ctx || !ctx->entries || !ctx->data_segment) {
        return;
    }
    for (i = 0; i < ctx->count; i++) {
        if (ctx->entries[i].direction == MPLC_IO_IN) {
            read_channel(&ctx->entries[i], ctx->data_segment);
        }
    }
}

void mplc_io_write_outputs(mplc_io_ctx_t *ctx)
{
    uint32_t i;
    if (!ctx || !ctx->entries || !ctx->data_segment) {
        return;
    }
    for (i = 0; i < ctx->count; i++) {
        if (ctx->entries[i].direction == MPLC_IO_OUT) {
            write_channel(&ctx->entries[i], ctx->data_segment);
        }
    }
}
