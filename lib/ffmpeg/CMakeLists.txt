cmake_minimum_required(VERSION 3.28)

project(ffmpeg)

set(FFMPEG_DLL_PATH ${PROJECT_SOURCE_DIR}/shared/ PARENT_SCOPE)
set(FFMPEG_LIB_PATH ${PROJECT_SOURCE_DIR}/interface/)

add_library(ffmpeg INTERFACE)

target_link_directories(ffmpeg INTERFACE ${FFMPEG_LIB_PATH})

target_link_libraries(ffmpeg INTERFACE avformat)
target_link_libraries(ffmpeg INTERFACE avcodec)
target_link_libraries(ffmpeg INTERFACE avutil)
target_link_libraries(ffmpeg INTERFACE avdevice)
target_link_libraries(ffmpeg INTERFACE avfilter)
target_link_libraries(ffmpeg INTERFACE swresample)
target_link_libraries(ffmpeg INTERFACE swscale)