cmake_minimum_required(VERSION 3.28)

project(imgui)

file(GLOB IMGUI_SOURCES "*.h" "*.cpp")

add_library(imgui STATIC ${IMGUI_SOURCES} )
