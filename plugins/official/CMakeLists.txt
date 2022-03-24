
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O2 -g -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rtsp/release/inc"
    )
link_directories(
    "${PROJECT_ROOT_PATH}/work/3rdparty/rtsp/release/lib"
    )

add_compile_options(-fPIC)

add_library(decode SHARED decode/dl_decode.cpp)
add_library(preview SHARED preview/preview.cpp)
add_library(rtsp SHARED rtsp/dl_rtsp.cpp)

target_link_libraries(rtsp
    -lplayer
    -lliveMedia
    -lgroupsock
    -lBasicUsageEnvironment
    -lUsageEnvironment
    )

