cmake_minimum_required(VERSION 3.28)
project(frames)

set(CMAKE_CXX_STANDARD 20)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(lib/glfw-3.3.4)
add_subdirectory(lib/glad)
add_subdirectory(lib/ffmpeg)

add_executable(app 
	src/lodepng.cpp	
	src/main.cpp
)

target_precompile_headers(app PRIVATE src/ffmpeg.h)

target_include_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_include_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/include/ffmpeg/)

target_link_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/lib/ffmpeg)

target_link_libraries(app PRIVATE glad glfw ffmpeg)

file(GLOB FFMPEG_DLLS ${FFMPEG_DLL_PATH}/*.dll)

file(COPY ${FFMPEG_DLLS} DESTINATION ${CMAKE_BINARY_DIR})
