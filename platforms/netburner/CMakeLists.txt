add_library(mplc_hal_netburner STATIC
    mod54415/hal_netburner.c
    mod54415/mplc_plc_runtime.c
)

target_include_directories(mplc_hal_netburner PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mod54415
    ${CMAKE_SOURCE_DIR}/hal/include
    ${CMAKE_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/schema
)

option(MPLC_NETBURNER_HOST_TEST "Build MOD54415 host smoke test" ON)

if(MPLC_NETBURNER_HOST_TEST)
    add_executable(mplc_netburner_test
        mod54415/mplc_task.c
        mod54415/embedded_stub.c
        mod54415/host_test_main.c
    )
    target_compile_definitions(mplc_netburner_test PRIVATE MPLC_NETBURNER_HOST_TEST=1)
    target_link_libraries(mplc_netburner_test PRIVATE mplc_runtime mplc_hal_netburner mplc_hal_generic)
    target_include_directories(mplc_netburner_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mod54415
        ${CMAKE_SOURCE_DIR}/platforms/generic
    )
endif()
