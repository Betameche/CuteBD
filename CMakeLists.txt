cmake_minimum_required(VERSION 3.16)

project(CuteBDQt5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set include path early for MOC
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS 
    Core
    Gui
    Widgets
    Concurrent
)

# Core sources organized by module
set(CORE_SOURCES
    # Main entry
    src/main.cpp
    
    # Compliance module
    src/compliance/compliance.cpp
    
    # Tools module
    src/tools/toolchecker.cpp
    
    # Inspection module
    src/inspection/inspector.cpp
    
    # Conversion module
    src/conversion/converter.cpp
    
    # Output/Burning module
    src/output/bdbuilder.cpp
    
    # Common utilities
    src/common/common.cpp
    
    # UI module
    src/ui/mainwindow.cpp
    src/ui/mainwindow_tables.cpp
    src/ui/mainwindow_pipeline.cpp
)

add_executable(cutebd
    ${CORE_SOURCES}
)

# Include directories BEFORE linking
target_include_directories(cutebd PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/compliance
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools
    ${CMAKE_CURRENT_SOURCE_DIR}/src/inspection
    ${CMAKE_CURRENT_SOURCE_DIR}/src/conversion
    ${CMAKE_CURRENT_SOURCE_DIR}/src/output
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

target_link_libraries(cutebd PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Concurrent
)
