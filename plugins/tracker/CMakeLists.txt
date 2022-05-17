
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O2 -rdynamic -Wno-deprecated-declarations -Wno-format-truncation -Wno-sign-compare -Wno-unused-result")

include_directories(
    "bytetrack/include"
    "${PROJECT_ROOT_PATH}/include"
    "${PROJECT_ROOT_PATH}/work/cjson/inc"
    "${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/include/opencv4"
    "${PROJECT_ROOT_PATH}/work/3rdparty/eigen/release/include/eigen3"
    )
link_directories(
    "${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib"
    )

add_compile_options(-fPIC)

add_library(tracker SHARED 
    tracker.cpp 
    bytetrack/src/BYTETracker.cpp
    bytetrack/src/STrack.cpp
    bytetrack/src/kalmanFilter.cpp
    bytetrack/src/lapjv.cpp
    bytetrack/src/utils.cpp
    )

add_dependencies(tracker opencv eigen)

target_link_libraries(tracker
    -lopencv_highgui
    -lopencv_core
    -lopencv_imgproc
    -lopencv_imgcodecs
    -lopencv_dnn
    )

