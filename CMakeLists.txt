cmake_minimum_required(VERSION 3.16)
project(MPLC VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(MPLC_BUILD_COMPILER "Build host compiler toolchain" ON)
option(MPLC_BUILD_RUNTIME  "Build portable runtime library" ON)
option(MPLC_BUILD_TESTS    "Build unit tests" ON)
option(MPLC_NO_FPU           "Use integer-only paths where possible" OFF)

include_directories(
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/hal/include
    ${CMAKE_SOURCE_DIR}/runtime/include
)

if(MPLC_BUILD_RUNTIME)
    add_subdirectory(runtime)
    add_subdirectory(platforms/generic)
    if(UNIX AND NOT APPLE)
        add_subdirectory(platforms/linux)
    endif()
    add_subdirectory(platforms/baremetal)
    add_subdirectory(platforms/netburner)
endif()

if(MPLC_BUILD_COMPILER)
    add_subdirectory(compiler)
endif()

if(MPLC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
