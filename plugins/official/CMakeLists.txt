
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O2 -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")
#set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -g -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    "${PROJECT_ROOT_PATH}/plugins/common"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rtsp/release/inc"
    "${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/include"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rabbitmq/release/include"
    )
link_directories(
    "${PROJECT_ROOT_PATH}/plugins/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rtsp/release/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rabbitmq/release/lib/x86_64-linux-gnu"
    )

add_compile_options(-fPIC)

add_library(rtsp SHARED rtsp/rtsp.cpp)
add_library(rtmp SHARED rtmp/rtmp.cpp)
add_library(cpurgbdec SHARED decode/decode_cpu_rgb.cpp)
add_library(cpuyuvdec SHARED decode/decode_cpu_yuv.cpp)
add_library(preview SHARED preview/preview.cpp)
add_library(osd SHARED osd/osd.cpp)
add_library(rabbitmqq SHARED output/rabbitmq.cpp)

add_dependencies(rtsp rtsplib)
add_dependencies(rtmp common)
add_dependencies(cpurgbdec common)
add_dependencies(cpuyuvdec common)
add_dependencies(preview common)
add_dependencies(osd common)
add_dependencies(rabbitmqq rabbitmq)

target_link_libraries(rtsp
    -lplayer
    -lliveMedia
    -lgroupsock
    -lBasicUsageEnvironment
    -lUsageEnvironment
    -Wl,-rpath,lib
    )
target_link_libraries(rtmp
    -lcommon
    -Wl,-rpath,lib
    )
target_link_libraries(cpurgbdec
    -lcommon
    -Wl,-rpath,lib
    )
target_link_libraries(cpuyuvdec
    -lcommon
    -Wl,-rpath,lib
    )
target_link_libraries(preview
    -lcommon
    -Wl,-rpath,lib
    )
target_link_libraries(osd
    -lcommon
    -Wl,-rpath,lib
    )
target_link_libraries(rabbitmqq
    -lrabbitmq
    -Wl,-rpath,lib
    )

