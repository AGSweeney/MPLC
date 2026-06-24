# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED MPLC OR NOT DEFINED SRC OR NOT DEFINED OUT)
    message(FATAL_ERROR "MPLC, SRC, and OUT must be defined")
endif()

execute_process(
    COMMAND ${MPLC} compile ${SRC} -o ${OUT}
    RESULT_VARIABLE rc
)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "Compile failed")
endif()

execute_process(
    COMMAND ${MPLC} inspect ${OUT}
    RESULT_VARIABLE rc2
)
if(NOT rc2 EQUAL 0)
    message(FATAL_ERROR "Inspect failed")
endif()
