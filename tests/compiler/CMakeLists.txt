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

add_test(NAME compiler_sample_ld COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)

add_test(NAME compiler_sample_ld_ton COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_ton.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_ton.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)

add_test(NAME compiler_sample_ld_ton5s COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_ton5s.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_ton5s.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)

add_test(NAME compiler_sample_ld_ton_stale_bypass COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_ton_stale_bypass.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_ton_stale_bypass.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)

add_test(NAME compiler_sample_ld_branch_or COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_branch_or.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_branch_or.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)

add_test(NAME compiler_sample_ld_output_branch COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_output_branch.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_output_branch.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)

add_test(NAME compiler_sample_ld_branch_ton_or COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_branch_ton_or.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_branch_ton_or.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)

add_test(NAME compiler_sample_ld_led8 COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_led8.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_led8.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)

add_test(NAME compiler_sample_ld_set_reset COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_set_reset.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_set_reset.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)

add_test(NAME compiler_sample_ld_ton_set_reset COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_ton_set_reset.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_ton_set_reset.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)
