/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HAL_NETBURNER_CONFIG_H
#define HAL_NETBURNER_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/*
 * MOD-DEV-70 on MOD54415 (MOD5441X platform)
 *
 * Inputs: 8 DIP switches — read via ADC (not GPIO). J2 pins 8,6,7,10,9,11,12,13
 *         map to ADC channels {7,6,5,3,4,1,0,2} for switches 1..8.
 *
 * Outputs: 8 LEDs on J2 GPIO pins 15,16,31,23,37,19,20,24 (LED 1..8).
 *          LEDs are active-low (drive 0 = ON, 1 = OFF).
 *
 * MPLC digital channels 0..7 correspond to DIP/LED 1..8 on the carrier.
 * Channels 8..15 are unused unless you extend the pin tables in hal_netburner.cpp.
 */

typedef struct {
    int16_t  nb_pin;       /* J2 pin number for GPIO outputs, or -1 */
    uint8_t  is_analog;
    uint8_t  adc_channel;  /* MOD5441X ADC result index for DIP inputs */
    uint8_t  dac_channel;
} mplc_nb_pin_map_t;

#define MPLC_NB_MAX_DIGITAL_IN   16U
#define MPLC_NB_MAX_DIGITAL_OUT  16U
#define MPLC_NB_MAX_ANALOG_IN     8U
#define MPLC_NB_MAX_ANALOG_OUT    2U

#define MPLC_NB_MOD_DEV70_DIP_COUNT   8U
#define MPLC_NB_MOD_DEV70_LED_COUNT   8U

/*
 * DIP reading uses NNDK Mod5441xFactoryApp SimpleAD.cpp + ReadSwitch() in simple_ad.cpp.
 * Raw mask is still from ReadSwitch() (switch 1 = bit 0). This flag maps raw bit to
 * logical ON for MPLC digital inputs / console labels:
 *   0 -> bit clear is ON  (factory-app convention)
 *   1 -> bit set is ON    (invert polarity for board labeling)
 */
#define MPLC_NB_DIP_BIT_SET_IS_ON  1

#define MPLC_NB_RETAIN_BASE  0x00020000U  /* User Params flash region (see platform ref) */
#define MPLC_NB_RETAIN_SIZE  8192U

#define MPLC_NB_DEFAULT_CYCLE_US  10000U

#define MPLC_NB_TASK_PRIORITY     10U
#define MPLC_NB_TASK_STACK_SIZE   4096U

/*
 * Optional file-based PLC package boot:
 * 1) try loading .mplc from filesystem (persisted in flash),
 * 2) fallback to embedded package if file is missing/invalid.
 */
#define MPLC_NB_PACKAGE_FILE_BOOT       1
#define MPLC_NB_PACKAGE_FILE_PATH       "mplc_startup.mplc"
#define MPLC_NB_PACKAGE_FILE_MAX_BYTES  (512U * 1024U)

/* Serial console IO logging for MOD-DEV-70 bring-up (set 0 for production) */
#define MPLC_NB_IO_DEBUG          0

/*
 * LEAP MOD54415LC leaves DIP ADC disabled and mirrors DO->DI for protocol tests.
 * Keep 1 to read physical DIP switches via ReadSwitch() (Mod5441xFactoryApp / SimpleAD).
 */
#define MPLC_NB_ENABLE_BOARD_DIP  1

#endif /* HAL_NETBURNER_CONFIG_H */
