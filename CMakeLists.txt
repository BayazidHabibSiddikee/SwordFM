cmake_minimum_required(VERSION 3.20)
project(swordfilemanager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

add_executable(swordfm
    src/main.cpp
    src/mainwindow.cpp
    src/filemodel.cpp
    src/sidebar.cpp
    src/fileview.cpp
    src/toolbar.cpp
    src/statusbar.cpp
    src/fileops.cpp
    src/contextmenu.cpp
    src/termutil.cpp
    src/previewpanel.cpp
    src/filefilter.cpp
    src/openwith.cpp
    resources/resources.qrc
)

target_include_directories(swordfm PRIVATE src)
target_link_libraries(swordfm PRIVATE Qt6::Widgets Qt6::Core)

install(TARGETS swordfm RUNTIME DESTINATION bin)
