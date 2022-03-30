
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O2 -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")
#set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -g -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rtsp/release/inc"
    "${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/include"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rabbitmq/release/include"
    )
link_directories(
    "${PROJECT_ROOT_PATH}/work/3rdparty/rtsp/release/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rabbitmq/release/lib/x86_64-linux-gnu"
    )

add_compile_options(-fPIC)

add_library(rtsp SHARED rtsp/dl_rtsp.cpp)
add_library(cpudecode SHARED decode/decode_cpu.cpp)
add_library(preview SHARED preview/preview.cpp)
add_library(rabbitmqq SHARED output/rabbitmq.cpp)

add_dependencies(rtsp rtsplib cjson)
add_dependencies(cpudecode ffmpeg)
add_dependencies(rabbitmqq rabbitmq)

target_link_libraries(rtsp
    -lplayer
    -lliveMedia
    -lgroupsock
    -lBasicUsageEnvironment
    -lUsageEnvironment
    )
target_link_libraries(cpudecode
    -lavformat 
    -lavfilter 
    -lavcodec 
    -lavutil 
    )
target_link_libraries(rabbitmqq
    -lrabbitmq
    )

