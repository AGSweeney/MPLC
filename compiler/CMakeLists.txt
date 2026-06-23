add_executable(mplc
    cli/main.c
    common/ir.c
    common/diag.c
    semantic/semantic.c
    codegen/codegen.c
    linker/linker.c
    frontend/st/st_parser.c
    frontend/il/il_parser.c
    frontend/plcopen/plcopen_parser.c
    frontend/ld/ld_parser.c
    frontend/fbd/fbd_parser.c
    frontend/sfc/sfc_parser.c
)

target_include_directories(mplc PRIVATE
    ${CMAKE_SOURCE_DIR}/compiler
    ${CMAKE_SOURCE_DIR}/compiler/common
    ${CMAKE_SOURCE_DIR}/compiler/semantic
    ${CMAKE_SOURCE_DIR}/schema
    ${CMAKE_SOURCE_DIR}/runtime/include
)

target_link_libraries(mplc PRIVATE mplc_runtime mplc_hal_generic)
