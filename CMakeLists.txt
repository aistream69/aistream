
cmake_minimum_required(VERSION 3.5)

project (AISTREAM)

if(NOT EXISTS "cfg")
    execute_process(COMMAND ln -s ../cfg cfg)
endif()
if(NOT EXISTS "plugins")
    execute_process(COMMAND ln -s ../plugins plugins)
endif()
if(NOT EXISTS "samples")
    execute_process(COMMAND ln -s ../samples samples)
endif()

set(PROJECT_ROOT_PATH "${AISTREAM_SOURCE_DIR}")

set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(src)
add_subdirectory(work/cjson)

include(cmake/libevent.cmake)

