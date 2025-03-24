cmake_minimum_required(VERSION 3.28)
project(frames)

set(CMAKE_CXX_STANDARD 20)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(lib/glfw-3.3.4)
add_subdirectory(lib/glad)
add_subdirectory(lib/ffmpeg)

add_executable(app 
	src/image/lodepng.cpp
	src/model/camera.cpp
	src/model/mesh.cpp
	src/shader/shader.cpp
	src/shader/shaderBase.cpp
	src/shader/shaderLog.cpp
	src/shader/shaderSource.cpp
	src/ui/ui.cpp
	src/main.cpp
	src/render.cpp
)

target_precompile_headers(app PRIVATE src/video/ffmpeg.h)

target_include_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_include_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/include/ffmpeg/)

target_link_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/lib/ffmpeg)

target_link_libraries(app PRIVATE glad glfw ffmpeg)

file(COPY ${FFMPEG_DLL_PATH} DESTINATION ${CMAKE_BINARY_DIR})

# Put all resources in a correct folder in the Release mode
# file(COPY ${PROJECT_SOURCE_DIR}/data/shaders/ DESTINATION ${CMAKE_BINARY_DIR}/data/shaders/)
