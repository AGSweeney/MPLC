add_test(NAME compiler_roundtrip COMMAND ${CMAKE_COMMAND}
    -D MPLC=$<TARGET_FILE:mplc>
    -D SRC=${CMAKE_SOURCE_DIR}/tests/compiler/sample.st
    -D OUT=${CMAKE_BINARY_DIR}/tests/sample.mplc
    -P ${CMAKE_SOURCE_DIR}/tests/compiler/run_compile.cmake
)
