# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_executable(test_vm test_vm.c)
target_link_libraries(test_vm PRIVATE mplc_runtime mplc_hal_generic)
target_include_directories(test_vm PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/platforms/generic
)

add_executable(test_runtime_e2e test_runtime_e2e.c)
target_link_libraries(test_runtime_e2e PRIVATE mplc_runtime mplc_hal_generic)
target_include_directories(test_runtime_e2e PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/platforms/generic
)

add_executable(test_blink_package test_blink_package.c)
target_link_libraries(test_blink_package PRIVATE mplc_runtime mplc_hal_generic)
target_include_directories(test_blink_package PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/platforms/generic
)

add_executable(test_dips_to_leds_package test_dips_to_leds_package.c)
target_link_libraries(test_dips_to_leds_package PRIVATE mplc_runtime mplc_hal_generic)
target_include_directories(test_dips_to_leds_package PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/platforms/generic
)

add_executable(test_motion test_motion.c)
target_link_libraries(test_motion PRIVATE mplc_runtime mplc_hal_generic)
target_include_directories(test_motion PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/hal/include
)

add_test(NAME motion COMMAND test_motion)

add_test(NAME dips_to_leds_package COMMAND test_dips_to_leds_package)

add_executable(test_ld_led8_package test_ld_led8_package.c)
target_link_libraries(test_ld_led8_package PRIVATE mplc_runtime mplc_hal_generic)
target_include_directories(test_ld_led8_package PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/platforms/generic
)
target_compile_definitions(test_ld_led8_package PRIVATE
    TEST_LD_LED8_PKG="${CMAKE_BINARY_DIR}/tests/sample_ld_led8.mplc")

add_test(NAME ld_led8_package_compile COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_led8.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_led8.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)
add_test(NAME ld_led8_package COMMAND test_ld_led8_package)
set_tests_properties(ld_led8_package PROPERTIES DEPENDS ld_led8_package_compile)

add_executable(test_ld_ton_delay test_ld_ton_delay.c)
target_link_libraries(test_ld_ton_delay PRIVATE mplc_runtime mplc_hal_generic)
target_include_directories(test_ld_ton_delay PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/platforms/generic
)
target_compile_definitions(test_ld_ton_delay PRIVATE
    TEST_LD_TON_PKG="${CMAKE_BINARY_DIR}/tests/sample_ld_ton5s.mplc")

add_test(NAME ld_ton_delay_compile COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_ton5s.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_ton5s.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)
add_test(NAME ld_ton_delay COMMAND test_ld_ton_delay)
set_tests_properties(ld_ton_delay PROPERTIES DEPENDS ld_ton_delay_compile)

add_executable(test_ld_set_reset test_ld_set_reset.c)
target_link_libraries(test_ld_set_reset PRIVATE mplc_runtime mplc_hal_generic)
target_include_directories(test_ld_set_reset PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/platforms/generic
)

add_test(NAME ld_set_reset_compile COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_set_reset.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_set_reset.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)
add_test(NAME ld_set_reset COMMAND test_ld_set_reset ${CMAKE_BINARY_DIR}/tests/sample_ld_set_reset.mplc)
set_tests_properties(ld_set_reset PROPERTIES DEPENDS ld_set_reset_compile)

add_executable(test_ld_ton_set_reset test_ld_ton_set_reset.c)
target_link_libraries(test_ld_ton_set_reset PRIVATE mplc_runtime mplc_hal_generic)
target_include_directories(test_ld_ton_set_reset PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/platforms/generic
)
target_compile_definitions(test_ld_ton_set_reset PRIVATE
    TEST_LD_TON_SET_RESET_PKG="${CMAKE_BINARY_DIR}/tests/sample_ld_ton_set_reset.mplc")

add_test(NAME ld_ton_set_reset_compile COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_ton_set_reset.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_ton_set_reset.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)
add_test(NAME ld_ton_set_reset COMMAND test_ld_ton_set_reset)
set_tests_properties(ld_ton_set_reset PROPERTIES DEPENDS ld_ton_set_reset_compile)

add_executable(test_ld_ton_cold_start test_ld_ton_cold_start.c)
target_link_libraries(test_ld_ton_cold_start PRIVATE mplc_runtime mplc_hal_generic)
target_include_directories(test_ld_ton_cold_start PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/platforms/generic
)
target_compile_definitions(test_ld_ton_cold_start PRIVATE
    TEST_LD_TON_COLD_START_PKG="${CMAKE_BINARY_DIR}/tests/sample_ld_ton_set_reset.mplc")

add_test(NAME ld_ton_cold_start COMMAND test_ld_ton_cold_start)
set_tests_properties(ld_ton_cold_start PROPERTIES DEPENDS ld_ton_set_reset_compile)

add_executable(test_ld_branch_ton_or test_ld_branch_ton_or.c)
target_link_libraries(test_ld_branch_ton_or PRIVATE mplc_runtime mplc_hal_generic)
target_include_directories(test_ld_branch_ton_or PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/platforms/generic
)
target_compile_definitions(test_ld_branch_ton_or PRIVATE
    TEST_LD_BRANCH_TON_OR_PKG="${CMAKE_BINARY_DIR}/tests/sample_ld_branch_ton_or.mplc")

add_test(NAME ld_branch_ton_or_compile COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample_ld_branch_ton_or.xml
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample_ld_branch_ton_or.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)
add_test(NAME ld_branch_ton_or COMMAND test_ld_branch_ton_or)
set_tests_properties(ld_branch_ton_or PROPERTIES DEPENDS ld_branch_ton_or_compile)
