add_library(mplc_runtime STATIC
    core/loader.c
    core/vm.c
    core/scheduler.c
    core/runtime.c
    io/io.c
    stdlib/stdlib.c
)

target_include_directories(mplc_runtime PUBLIC
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/hal/include
)

if(MSVC)
    target_compile_options(mplc_runtime PRIVATE /W4)
else()
    target_compile_options(mplc_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MPLC_NO_FPU)
    target_compile_definitions(mplc_runtime PUBLIC MPLC_NO_FPU=1)
endif()
