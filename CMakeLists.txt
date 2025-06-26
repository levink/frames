cmake_minimum_required(VERSION 3.28)
project(frames)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(lib/ffmpeg)
add_subdirectory(lib/glad)
add_subdirectory(lib/glfw-3.3.4)
add_subdirectory(lib/gtest)
add_subdirectory(lib/imgui)

add_executable(app 
	
	lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
	lib/nfd/nfd_common.c
	lib/nfd/nfd_win.cpp

	src/image/lodepng.cpp
	src/io/io.cpp
	src/model/camera.cpp
	src/model/frame.cpp
	src/model/mesh.cpp
	src/shader/shader.cpp
	src/shader/shaderBase.cpp
	src/shader/shaderLog.cpp
	src/shader/shaderSource.cpp
	src/video/frame.cpp
	src/video/video.cpp
	src/main.cpp
	src/render.cpp
)
target_include_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/include/ffmpeg/)
target_include_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/lib/imgui/)
target_include_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/lib/nfd/include/)
target_link_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/lib/ffmpeg)
target_link_libraries(app PRIVATE glad glfw ffmpeg imgui)
#set_target_properties(app PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")

file(COPY ${FFMPEG_DLL_PATH} DESTINATION ${CMAKE_BINARY_DIR})

# For RELEASE: Set correct paths for resources 
# file(COPY ${PROJECT_SOURCE_DIR}/data/shaders/ DESTINATION ${CMAKE_BINARY_DIR}/data/shaders/)


add_executable(tests
	tests/CircleBufferTest.cpp
)
target_include_directories(tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(tests PRIVATE ${gtest_SOURCE_DIR}/include)
target_link_libraries(tests gtest gtest_main)