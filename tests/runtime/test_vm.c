#include "mplc/vm.h"
#include "mplc_abi.h"
#include "mplc_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t code[] = {
    MPLC_OP_LOAD_IO, 0, 0,
    MPLC_OP_STORE_IO, 1, 0,
    MPLC_OP_RET
};

static mplc_io_entry_t io_map[] = {
    { 0, MPLC_TYPE_BOOL, MPLC_IO_IN,  0, 0 },
    { 0, MPLC_TYPE_BOOL, MPLC_IO_OUT, 1, 0 }
};

int main(void)
{
    mplc_vm_t *vm = NULL;
    mplc_vm_config_t cfg;
    uint8_t data[8] = {0};

    mplc_hal_init(NULL);
    mplc_hal_digital_write(0, false);

    memset(&cfg, 0, sizeof(cfg));
    cfg.global_data = data;
    cfg.global_size = sizeof(data);
    cfg.code_base = code;
    cfg.code_size = sizeof(code);
    cfg.io_map = io_map;
    cfg.io_count = 2;

    if (mplc_vm_create(&vm, &cfg) != 0) {
        return 1;
    }

    data[0] = 1;
    if (mplc_vm_run_pou(vm, 1, 0, sizeof(code)) != 0) {
        return 2;
    }

    if (data[1] != 1) {
        fprintf(stderr, "Expected output true, got %u\n", data[1]);
        return 3;
    }

    mplc_vm_destroy(vm);
    printf("test_vm OK\n");
    return 0;
}
