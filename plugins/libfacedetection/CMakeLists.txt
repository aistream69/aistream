
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O2 -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    "${PROJECT_ROOT_PATH}/plugins/common"
    "${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/include/opencv4"
    )
link_directories(
    "${PROJECT_ROOT_PATH}/plugins/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib"
    )

add_compile_options(-fPIC)

add_library(detection SHARED detection.cpp)
add_library(detection2 SHARED detection2.cpp)

add_dependencies(detection opencv)
add_dependencies(detection2 opencv common)

target_link_libraries(detection
    -lopencv_highgui
    -lopencv_core
    -lopencv_imgproc
    -lopencv_imgcodecs
    -lopencv_dnn
    -Wl,-rpath,lib
    )
target_link_libraries(detection2
    -lopencv_highgui
    -lopencv_core
    -lopencv_imgproc
    -lopencv_imgcodecs
    -lopencv_dnn
    -lcommon
    -Wl,-rpath,lib
    )

