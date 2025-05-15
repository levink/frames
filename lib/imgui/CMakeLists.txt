cmake_minimum_required(VERSION 3.20)

project(imgui)

file(GLOB IMGUI_SOURCES "*.h" "*.cpp")

add_library(imgui STATIC ${IMGUI_SOURCES} )
