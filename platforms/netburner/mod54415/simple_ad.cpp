/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * MOD5441X single-ended ADC for MOD-DEV-70 DIP switches.
 * From NNDK Mod5441xFactoryApp src/SimpleAD.cpp and ReadSwitch() in webfuncs.cpp.
 */

#include "simple_ad.h"
#include <basictypes.h>
#include <sim.h>

/* J2 DIP switch 1..8 → ADC result index 0..7 (factory app PinNumber table). */
static const uint8_t g_dip_adc_result_index[8] = {7, 6, 5, 3, 4, 1, 0, 2};

/* Half of 16-bit ADC range; values above => switch OFF in raw mask. */
#define DIP_ADC_THRESHOLD  (0x7FFF / 2)

extern "C" void InitSingleEndAD(void)
{
    volatile uint16_t vw;

    /* See MCF5441X RM Chapter 29 */
    sim2.adc.cr1 = 0;
    sim2.adc.cr2 = 0;
    sim2.adc.zccr = 0;
    sim2.adc.lst1 = 0x3210;
    sim2.adc.lst2 = 0x7654;
    sim2.adc.sdis = 0;
    sim2.adc.sr = 0xFFFF;
    for (int i = 0; i < 8; i++) {
        vw = sim2.adc.rslt[i];
        (void)vw;
        sim2.adc.ofs[i] = 0;
    }

    sim2.adc.lsr = 0xFFFF;
    sim2.adc.zcsr = 0xFFFF;
    sim2.adc.pwr = 0;
    sim2.adc.cal = 0x0000;
    sim2.adc.pwr2 = 0x0005;
    sim2.adc.div = 0x505;
    sim2.adc.asdiv = 0x13;
}

extern "C" void StartAD(void)
{
    sim2.adc.sr = 0xffff;
    sim2.adc.cr1 = 0x2000;
}

extern "C" bool ADDone(void)
{
    return (sim2.adc.sr & 0x0800) != 0;
}

extern "C" uint16_t GetADResult(int ch)
{
    return sim2.adc.rslt[ch];
}

extern "C" uint16_t ReadDIPSwitchADC(uint8_t switch_index)
{
    if (switch_index >= 8U) {
        return 0U;
    }
    return GetADResult((int)g_dip_adc_result_index[switch_index]);
}

extern "C" uint8_t ReadSwitch(void)
{
    static bool read_switch_init = false;
    uint8_t bit_mask = 0U;

    if (!read_switch_init) {
        InitSingleEndAD();
        read_switch_init = true;
    }

    StartAD();
    while (!ADDone()) {
        asm("nop");
    }

    for (int bit_pos = 0x01, i = 0; bit_pos < 256; bit_pos <<= 1, i++) {
        if (GetADResult((int)g_dip_adc_result_index[i]) > DIP_ADC_THRESHOLD) {
            bit_mask |= (uint8_t)(0xFF & bit_pos);
        }
    }

    return bit_mask;
}
