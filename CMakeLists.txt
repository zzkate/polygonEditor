cmake_minimum_required(VERSION 3.5)
project(image_polygons LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(image_polygons
    main.cpp
    ImageViewer.h
    ImageViewer.cpp
)

target_link_libraries(image_polygons Qt5::Widgets)