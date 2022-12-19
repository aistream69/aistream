
set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -O2 -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")
#set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -g -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")
add_compile_options(-fPIC)

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    "${PROJECT_ROOT_PATH}/work/cjson/inc"
    )

include(${PROJECT_ROOT_PATH}/cmake/libtorch.cmake)
add_subdirectory(bert/tokenizers)
add_subdirectory(bert/ftokenizers)
add_subdirectory(bert)
add_dependencies(bertcpu libtorch cjson tokenizers ftokenizers)

include_directories(
    "${PROJECT_ROOT_PATH}/plugins/common"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rtsp/release/inc"
    "${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/include"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rabbitmq/release/include"
    "${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/include"
    "${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/include/opencv4"
    "${PROJECT_ROOT_PATH}/work/3rdparty/freetype/release/include/freetype2"
    )
link_directories(
    "${PROJECT_ROOT_PATH}/plugins/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rtsp/release/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/rabbitmq/release/lib/x86_64-linux-gnu"
    "${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/freetype/release/lib"
    )

add_library(rtsp SHARED rtsp/rtsp.cpp)
add_library(rtmp SHARED rtmp/rtmp.cpp)
add_library(httpfile SHARED httpfile/httpfile.cpp)
add_library(cpurgbdec SHARED decode/decode_cpu_rgb.cpp)
add_library(cpuyuvdec SHARED decode/decode_cpu_yuv.cpp)
add_library(preview SHARED preview/preview.cpp)
add_library(osd SHARED osd/osd.cpp)
add_library(rabbitmqq SHARED output/rabbitmq.cpp)
add_library(resnet50opencv SHARED resnet50/resnet50_opencv.cpp)
add_library(yolov3opencv SHARED yolov3/yolov3_opencv.cpp)

add_dependencies(rtsp rtsplib)
add_dependencies(rtmp common)
add_dependencies(httpfile libevent)
add_dependencies(cpurgbdec common)
add_dependencies(cpuyuvdec common)
add_dependencies(preview common)
add_dependencies(osd common freetype)
add_dependencies(rabbitmqq rabbitmq)
add_dependencies(resnet50opencv cjson opencv)
add_dependencies(yolov3opencv cjson opencv)

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
target_link_libraries(httpfile
    -levent_pthreads
    -levent
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
    -lfreetype
    -Wl,-rpath,lib
    )
target_link_libraries(rabbitmqq
    -lrabbitmq
    -Wl,-rpath,lib
    )
target_link_libraries(resnet50opencv
    -lopencv_highgui
    -lopencv_core
    -lopencv_imgproc
    -lopencv_imgcodecs
    -lopencv_dnn
    -Wl,-rpath,lib
    )
target_link_libraries(yolov3opencv
    -lopencv_highgui
    -lopencv_core
    -lopencv_imgproc
    -lopencv_imgcodecs
    -lopencv_dnn
    -Wl,-rpath,lib
    )

