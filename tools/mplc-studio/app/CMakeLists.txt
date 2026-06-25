# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg Concurrent)
find_package(Qt6SerialPort QUIET)

qt_standard_project_setup()

set(MPLC_STUDIO_LADDER_SOURCES
    src/ladder/LadderElementEditorDialog.cpp
    src/ladder/LadderElementEditorDialog.h
    src/ladder/LadderVariableUi.cpp
    src/ladder/LadderVariableUi.h
    src/ladder/LadderIoCatalog.cpp
    src/ladder/LadderIoCatalog.h
    src/ladder/LadderModel.cpp
    src/ladder/LadderModel.h
    src/ladder/LadderBranchLogic.cpp
    src/ladder/LadderBranchLogic.h
    src/ladder/BranchRegionItem.cpp
    src/ladder/BranchRegionItem.h
    src/ladder/LadderPlcopenSerializer.cpp
    src/ladder/LadderPlcopenSerializer.h
    src/ladder/LadderValidator.cpp
    src/ladder/LadderValidator.h
    src/ladder/LadderStExporter.cpp
    src/ladder/LadderStExporter.h
    src/ladder/StLadderImporter.cpp
    src/ladder/StLadderImporter.h
    src/ladder/LadderElementItem.cpp
    src/ladder/LadderElementItem.h
    src/ladder/ContactItem.cpp
    src/ladder/ContactItem.h
    src/ladder/CoilItem.cpp
    src/ladder/CoilItem.h
    src/ladder/FunctionBlockItem.cpp
    src/ladder/FunctionBlockItem.h
    src/ladder/WireItem.cpp
    src/ladder/WireItem.h
    src/ladder/PowerRailItem.cpp
    src/ladder/PowerRailItem.h
    src/ladder/LadderCanvasView.cpp
    src/ladder/LadderCanvasView.h
    src/ladder/LadderInstructionBar.cpp
    src/ladder/LadderInstructionBar.h
    src/ladder/LadderPropertiesPanel.cpp
    src/ladder/LadderPropertiesPanel.h
    src/ladder/LadderScene.cpp
    src/ladder/LadderScene.h
    src/ladder/RungBandItem.cpp
    src/ladder/RungBandItem.h
    src/ladder/LadderToolbox.cpp
    src/ladder/LadderToolbox.h
    src/ladder/LadderToolboxItem.cpp
    src/ladder/LadderToolboxItem.h
    src/ladder/LadderSymbolIcons.cpp
    src/ladder/LadderSymbolIcons.h
    src/ladder/LadderVariableTable.cpp
    src/ladder/LadderVariableTable.h
    src/ladder/LadderVariableBrowserDialog.cpp
    src/ladder/LadderVariableBrowserDialog.h
    src/ladder/LadderEditorWidget.cpp
    src/ladder/LadderEditorWidget.h
)

qt_add_executable(mplc_studio WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/DocumentEditor.h
    src/DocumentEditor.cpp
    src/StTextEditor.cpp
    src/StTextEditor.h
    src/CompilerService.cpp
    src/CompilerService.h
    src/StImportService.cpp
    src/StImportService.h
    src/WorkerPool.cpp
    src/WorkerPool.h
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
    ${MPLC_STUDIO_LADDER_SOURCES}
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
        Qt6::Concurrent
        mplc_compiler
)

if(TARGET Qt6::SerialPort)
    target_link_libraries(mplc_studio PRIVATE Qt6::SerialPort)
    target_compile_definitions(mplc_studio PRIVATE MPLC_STUDIO_HAS_SERIALPORT=1)
else()
    message(WARNING "Qt6 SerialPort not found — serial monitor tab will show an install hint.")
endif()
