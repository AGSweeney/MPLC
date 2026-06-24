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
