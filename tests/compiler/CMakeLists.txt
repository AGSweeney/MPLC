# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_test(NAME compiler_roundtrip COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample.st
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)

add_test(NAME compiler_st_features COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/st_features.st
    -D OUT=${CMAKE_BINARY_DIR}/tests/st_features.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)
