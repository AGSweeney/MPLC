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

add_test(NAME vm_smoke COMMAND test_vm)
add_test(NAME runtime_e2e COMMAND test_runtime_e2e)
