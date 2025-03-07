cmake_minimum_required(VERSION 3.28)
project(frames)

set(CMAKE_CXX_STANDARD 20)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(lib/glfw-3.3.4)
add_subdirectory(lib/glad)

#add_library(ffmpeg STATIC IMPORTED)
#set_target_properties(ffmpeg PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/lib/ffmpeg/libavcodec.a)
#set_target_properties(ffmpeg PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/lib/ffmpeg/libavformat.a)
#set_target_properties(ffmpeg PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include/ffmpeg)

add_executable(app 
	src/main.cpp
)

target_include_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_include_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/include/ffmpeg/)

target_link_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/lib/ffmpeg)

target_link_libraries(app PRIVATE glad glfw avformat)

target_precompile_headers(app PUBLIC src/ffmpeg.h)

file(GLOB FFMPEG_DLLS "${PROJECT_SOURCE_DIR}/lib/ffmpeg/*.dll")

file(COPY ${FFMPEG_DLLS} DESTINATION ${CMAKE_BINARY_DIR})