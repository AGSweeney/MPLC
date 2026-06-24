/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SIMPLE_AD_H
#define SIMPLE_AD_H

#include <stdint.h>
#include <stdbool.h>

/*
 * MOD5441X ADC for MOD-DEV-70 DIP switches.
 * From NNDK Mod5441xFactoryApp src/SimpleAD.h + ReadSwitch() in webfuncs.cpp.
 */

#ifdef __cplusplus
extern "C" {
#endif

void InitSingleEndAD(void);
void StartAD(void);
bool ADDone(void);
uint16_t GetADResult(int ch);

/* Raw 8-bit mask: bit set = switch OFF, bit clear = switch ON (switch 1 = bit 0). */
uint8_t ReadSwitch(void);

/* DIP switch index 0..7 (switch 1..8); raw ADC result for debug/diagnostics. */
uint16_t ReadDIPSwitchADC(uint8_t switch_index);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_AD_H */
