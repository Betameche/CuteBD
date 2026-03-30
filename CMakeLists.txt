cmake_minimum_required(VERSION 3.16)

project(CuteBDQt5Minimal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(cutebd_qt5_minimal
    src/main.cpp
)

target_link_libraries(cutebd_qt5_minimal PRIVATE Qt5::Widgets)
