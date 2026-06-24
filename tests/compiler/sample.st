# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

VAR
    Input AT %IX0.0 : BOOL;
    Output AT %QX0.0 : BOOL;
END_VAR

PROGRAM Main
    Output := Input;
END_PROGRAM
