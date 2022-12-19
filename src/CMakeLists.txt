
set(target aistream)

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -O2 -rdynamic")
#set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -g -rdynamic")

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(PLUGINS_OFFICIAL "${PROJECT_ROOT_PATH}/plugins/official")

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    "${PROJECT_ROOT_PATH}/src/backend/dylib"
    "${PLUGINS_OFFICIAL}/rtsp"
    "${PLUGINS_OFFICIAL}/rtmp"
    "${PLUGINS_OFFICIAL}/gat1400"
    "${PLUGINS_OFFICIAL}/httpfile"
    "${PROJECT_ROOT_PATH}/work/cjson/inc"
    "${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/include"
    "${PROJECT_ROOT_PATH}/work/3rdparty/mongodb/release/include/libmongoc-1.0"
    "${PROJECT_ROOT_PATH}/work/3rdparty/mongodb/release/include/libbson-1.0"
    )

link_directories(
    "${PROJECT_ROOT_PATH}/work/cjson/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/mongodb/release/lib"
    "${PROJECT_ROOT_PATH}/work/lib"
    )

add_executable(
    ${target} 
    main.cpp
    stream.cpp
    master.cpp
    slave.cpp
    config.cpp
    rest.cpp
    rest_slave.cpp
    pipeline.cpp
    obj.cpp
    task.cpp
    share.cpp
    db.cpp
    ../src/backend/dylib/dylib.cpp
    "${PLUGINS_OFFICIAL}/gat1400/gat1400.cpp"
    )

add_dependencies(${target} cjson)
add_dependencies(${target} libevent)
add_dependencies(${target} mongodb)

set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_link_libraries(
    ${target}
    -lcjson
    -levent_pthreads
    -levent
    -lmongoc-1.0
    -lbson-1.0
    -ldl
    -lpthread
    -fPIE
    -Wl,-rpath,lib
    )

install (TARGETS aistream DESTINATION .)
install (FILES 
    ${PROJECT_ROOT_PATH}/cfg/config.json  
    ${PROJECT_ROOT_PATH}/cfg/object.json  
    ${PROJECT_ROOT_PATH}/cfg/task.json 
    DESTINATION cfg)
install (FILES 
    ${PROJECT_ROOT_PATH}/samples/face_detection.json
    ${PROJECT_ROOT_PATH}/samples/face_osd.json
    ${PROJECT_ROOT_PATH}/samples/preview.json
    DESTINATION samples)
install (FILES 
    ${PROJECT_ROOT_PATH}/work/data/model/libfacedetection/yunet_120x160.onnx
    DESTINATION data/model/libfacedetection)
install (FILES 
    ${PROJECT_ROOT_PATH}/work/data/img/default.png
    ${PROJECT_ROOT_PATH}/work/data/img/screen.jpg
    DESTINATION data/img)
install (FILES 
    ${PROJECT_ROOT_PATH}/build/plugins/official/librtsp.so
    ${PROJECT_ROOT_PATH}/build/plugins/official/librtmp.so
    ${PROJECT_ROOT_PATH}/build/plugins/official/libcpuyuvdec.so
    ${PROJECT_ROOT_PATH}/build/plugins/official/libosd.so
    ${PROJECT_ROOT_PATH}/build/plugins/official/librabbitmqq.so
    ${PROJECT_ROOT_PATH}/build/plugins/official/libpreview.so
    ${PROJECT_ROOT_PATH}/build/plugins/official/libcpurgbdec.so
    DESTINATION plugins/official)
install (FILES 
    ${PROJECT_ROOT_PATH}/build/plugins/libfacedetection/libdetection.so
    ${PROJECT_ROOT_PATH}/build/plugins/libfacedetection/libdetection2.so
    DESTINATION plugins/libfacedetection)
install (FILES 
    ${PROJECT_ROOT_PATH}/build/plugins/tracker/libtracker.so
    DESTINATION plugins/tracker)
install (FILES 
    ${PROJECT_ROOT_PATH}/plugins/lib/libcommon.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent-2.1.so.7
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent-2.1.so.7.0.1
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent_pthreads.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent_pthreads-2.1.so.7
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent_pthreads-2.1.so.7.0.1
    ${PROJECT_ROOT_PATH}/work/3rdparty/rabbitmq/release/lib/x86_64-linux-gnu/librabbitmq.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/rabbitmq/release/lib/x86_64-linux-gnu/librabbitmq.so.4
    ${PROJECT_ROOT_PATH}/work/3rdparty/rabbitmq/release/lib/x86_64-linux-gnu/librabbitmq.so.4.5.0
    ${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib/libavformat.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib/libavformat.so.57
    ${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib/libavformat.so.57.83.100
    ${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib/libavcodec.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib/libavcodec.so.57
    ${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib/libavcodec.so.57.107.100
    ${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib/libavutil.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib/libavutil.so.55
    ${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib/libavutil.so.55.78.100
    ${PROJECT_ROOT_PATH}/work/3rdparty/x264/release/lib/libx264.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/x264/release/lib/libx264.so.157
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_core.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_core.so.405
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_core.so.4.5.5
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_imgproc.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_imgproc.so.405
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_imgproc.so.4.5.5
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_imgcodecs.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_imgcodecs.so.405
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_imgcodecs.so.4.5.5
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_highgui.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_highgui.so.405
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_highgui.so.4.5.5
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_videoio.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_videoio.so.405
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_videoio.so.4.5.5
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_calib3d.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_calib3d.so.405
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_calib3d.so.4.5.5
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_features2d.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_features2d.so.405
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_features2d.so.4.5.5
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_flann.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_flann.so.405
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_flann.so.4.5.5
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_dnn.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_dnn.so.405
    ${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib/libopencv_dnn.so.4.5.5
    ${PROJECT_ROOT_PATH}/work/3rdparty/mongodb/release/lib/libmongoc-1.0.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/mongodb/release/lib/libmongoc-1.0.so.0
    ${PROJECT_ROOT_PATH}/work/3rdparty/mongodb/release/lib/libmongoc-1.0.so.0.0.0
    ${PROJECT_ROOT_PATH}/work/3rdparty/mongodb/release/lib/libbson-1.0.so 
    ${PROJECT_ROOT_PATH}/work/3rdparty/mongodb/release/lib/libbson-1.0.so.0
    ${PROJECT_ROOT_PATH}/work/3rdparty/mongodb/release/lib/libbson-1.0.so.0.0.0
    DESTINATION lib)

