#ifndef MPLC_HAL_NETBURNER_HOST_H
#define MPLC_HAL_NETBURNER_HOST_H

#include <stdint.h>
#include <stdbool.h>

void mplc_hal_netburner_host_set_digital_in(uint16_t channel, bool value);
bool mplc_hal_netburner_host_get_digital_out(uint16_t channel);
void mplc_hal_netburner_host_advance_time_us(uint64_t delta);

int  mplc_netburner_plc_run_cycles(uint32_t count);

#endif
