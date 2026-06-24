# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg)
find_package(Qt6SerialPort QUIET)

qt_standard_project_setup()

qt_add_executable(mplc_studio WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/CompilerService.cpp
    src/CompilerService.h
    src/DeviceClient.cpp
    src/DeviceClient.h
    src/DeviceDiscoveryService.cpp
    src/DeviceDiscoveryService.h
    src/DiscoveryDialog.cpp
    src/DiscoveryDialog.h
    src/DiscoveredDevice.h
    src/StudioTheme.cpp
    src/StudioTheme.h
    src/RibbonUi.cpp
    src/RibbonUi.h
    src/StSyntaxHighlighter.cpp
    src/StSyntaxHighlighter.h
    src/StInstructionToolbox.cpp
    src/StInstructionToolbox.h
    src/SerialMonitorWidget.cpp
    src/SerialMonitorWidget.h
    resources/win32ui.rc
)

set_target_properties(mplc_studio PROPERTIES OUTPUT_NAME "MPLCStudio")

qt_add_resources(mplc_studio app_icons
    PREFIX
        "/icons"
    BASE
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons"
    FILES
        resources/icons/app-icon-64.png
        resources/icons/app-icon-256.png
        resources/icons/win32ui.ico
        resources/icons/chevron-right.svg
        resources/icons/chevron-down.svg
        resources/icons/chevron-up.svg
        resources/icons/search.svg
        resources/icons/link.svg
        resources/icons/unlink.svg
        resources/icons/list.svg
        resources/icons/star-plus.svg
        resources/icons/trash.svg
)

target_include_directories(mplc_studio
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(mplc_studio
    PRIVATE
        Qt6::Widgets
        Qt6::Network
        Qt6::Svg
        mplc_compiler
)

if(TARGET Qt6::SerialPort)
    target_link_libraries(mplc_studio PRIVATE Qt6::SerialPort)
    target_compile_definitions(mplc_studio PRIVATE MPLC_STUDIO_HAS_SERIALPORT=1)
else()
    message(WARNING "Qt6 SerialPort not found — serial monitor tab will show an install hint.")
endif()
