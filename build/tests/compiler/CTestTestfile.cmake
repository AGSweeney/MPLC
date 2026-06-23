# CMake generated Testfile for 
# Source directory: C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler
# Build directory: C:/Users/agswe/OneDrive/Desktop/MPLC/build/tests/compiler
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(compiler_roundtrip "C:/Program Files/CMake/bin/cmake.exe" "-D" "MPLC=C:/Users/agswe/OneDrive/Desktop/MPLC/build/compiler/Debug/mplc.exe" "-D" "SRC=C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/sample.st" "-D" "OUT=C:/Users/agswe/OneDrive/Desktop/MPLC/build/tests/sample.mplc" "-P" "C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/run_compile.cmake")
  set_tests_properties(compiler_roundtrip PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/CMakeLists.txt;1;add_test;C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(compiler_roundtrip "C:/Program Files/CMake/bin/cmake.exe" "-D" "MPLC=C:/Users/agswe/OneDrive/Desktop/MPLC/build/compiler/Release/mplc.exe" "-D" "SRC=C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/sample.st" "-D" "OUT=C:/Users/agswe/OneDrive/Desktop/MPLC/build/tests/sample.mplc" "-P" "C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/run_compile.cmake")
  set_tests_properties(compiler_roundtrip PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/CMakeLists.txt;1;add_test;C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(compiler_roundtrip "C:/Program Files/CMake/bin/cmake.exe" "-D" "MPLC=C:/Users/agswe/OneDrive/Desktop/MPLC/build/compiler/MinSizeRel/mplc.exe" "-D" "SRC=C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/sample.st" "-D" "OUT=C:/Users/agswe/OneDrive/Desktop/MPLC/build/tests/sample.mplc" "-P" "C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/run_compile.cmake")
  set_tests_properties(compiler_roundtrip PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/CMakeLists.txt;1;add_test;C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(compiler_roundtrip "C:/Program Files/CMake/bin/cmake.exe" "-D" "MPLC=C:/Users/agswe/OneDrive/Desktop/MPLC/build/compiler/RelWithDebInfo/mplc.exe" "-D" "SRC=C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/sample.st" "-D" "OUT=C:/Users/agswe/OneDrive/Desktop/MPLC/build/tests/sample.mplc" "-P" "C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/run_compile.cmake")
  set_tests_properties(compiler_roundtrip PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/CMakeLists.txt;1;add_test;C:/Users/agswe/OneDrive/Desktop/MPLC/tests/compiler/CMakeLists.txt;0;")
else()
  add_test(compiler_roundtrip NOT_AVAILABLE)
endif()
