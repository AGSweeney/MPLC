#ifndef HAL_NETBURNER_CONFIG_H
#define HAL_NETBURNER_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/*
 * MOD54415 I/O channel map — edit for your carrier board wiring.
 *
 * Default layout assumes MOD-DEV-70 demo pins (adjust before production):
 *   Digital IN  channels 0..7  → Port J pins (example)
 *   Digital OUT channels 0..7  → Port H pins (example)
 *   Analog IN   channels 0..7  → ADC channels 0..7
 *   Analog OUT  channels 0..1  → DAC channels 0..1
 *
 * Set nb_pin to -1 to mark a channel as unused/disabled.
 */

typedef struct {
    int16_t  nb_pin;       /* NetBurner GPIO pin number, or ADC/DAC index */
    uint8_t  is_analog;
    uint8_t  adc_channel;  /* Valid when is_analog && input */
    uint8_t  dac_channel;  /* Valid when is_analog && output */
} mplc_nb_pin_map_t;

#define MPLC_NB_MAX_DIGITAL_IN   16U
#define MPLC_NB_MAX_DIGITAL_OUT  16U
#define MPLC_NB_MAX_ANALOG_IN     8U
#define MPLC_NB_MAX_ANALOG_OUT    2U

#define MPLC_NB_RETAIN_BASE  0x00020000U  /* User Params flash region (see platform ref) */
#define MPLC_NB_RETAIN_SIZE  8192U

/* Default cycle time when not specified by .mplc package */
#define MPLC_NB_DEFAULT_CYCLE_US  10000U

/* uC/OS task parameters */
#define MPLC_NB_TASK_PRIORITY     10U
#define MPLC_NB_TASK_STACK_SIZE   4096U

#endif /* HAL_NETBURNER_CONFIG_H */
