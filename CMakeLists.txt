
cmake_minimum_required(VERSION 3.5)

project (AISTREAM)

set(PROJECT_ROOT_PATH "${AISTREAM_SOURCE_DIR}")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_INSTALL_PREFIX "${PROJECT_ROOT_PATH}/build/release")

if(NOT EXISTS "cfg")
    execute_process(COMMAND ln -s ../cfg cfg)
endif()
if(NOT EXISTS "lib")
    execute_process(COMMAND ln -s ../plugins/lib lib)
endif()
if(NOT EXISTS "samples")
    execute_process(COMMAND ln -s ../samples samples)
endif()
if(NOT EXISTS "data")
    execute_process(COMMAND ln -s ../work/data data)
endif()
if(NOT EXISTS ${PROJECT_ROOT_PATH}/plugins/lib)
    execute_process(COMMAND mkdir -p ${PROJECT_ROOT_PATH}/plugins/lib)
endif()

option(PLUGINS "Plugins" ON)
message(STATUS "BUILD INFO:")
message(STATUS "\tPlugins: ${PLUGINS}")
message(STATUS "\tPrefix: ${CMAKE_INSTALL_PREFIX}")

add_subdirectory(src)
add_subdirectory(work/cjson)
add_subdirectory(plugins)

include(cmake/libevent.cmake)
include(cmake/opencv.cmake)
include(cmake/ffmpeg.cmake)
include(cmake/x264.cmake)
include(cmake/rtsp.cmake)
include(cmake/eigen.cmake)
include(cmake/rabbitmq.cmake)
include(cmake/mongodb.cmake)
add_dependencies(ffmpeg x264)

